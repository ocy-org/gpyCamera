#include "videoglwidget.h"
#include <QPainter>
#include <opencv2/imgproc.hpp>
#include <string.h>
extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}
#include <QDebug>

// GLSL 100风格,Qt在桌面端自动把highp/mediump映射为空宏,兼容GL2.1~4.6
static const char* kVS = R"(
attribute highp vec2 aPos;   // 视频像素坐标,CPU每帧生成
attribute highp vec2 aUV;    // 0..1
uniform highp mat3 uMVP;
varying highp vec2 vUV;
void main(){
    gl_Position = vec4((uMVP*vec3(aPos,1.0)).xy, 0.0, 1.0);
    vUV = aUV;
})";

static const char* kFS = R"(
uniform sampler2D texY;
uniform sampler2D texUV;
uniform sampler2D texV;
uniform highp vec2 uVideoSize;
uniform lowp int uFmt;          // 1=YUV420平面 2=YUYV打包
uniform lowp int uFullRange;
uniform lowp int uBt709;
varying highp vec2 vUV;
void main(){
    mediump float y; mediump vec2 ch;
    if(uFmt==2){
        highp float fx = vUV.x*uVideoSize.x;
        highp vec2 tuv = vec2((floor(fx*0.5)+0.5)/(uVideoSize.x*0.5), vUV.y);
        vec4 t = texture2D(texY,tuv);            // R=Y0 G=U B=Y1 A=V
        y  = mix(t.r, t.b, step(1.0, mod(fx,2.0)));
        ch = t.ga - 0.5;                         // YUYV恒为full range
    }else{
        y = texture2D(texY,vUV).r;
        if(uFullRange==0) y = (y*255.0-16.0)/219.0;   // limited range展开
        ch = vec2(texture2D(texUV,vUV).r, texture2D(texV,vUV).r) - 0.5;
    }
    mediump vec3 rgb;
    if(uBt709==1){
        rgb.r = 1.1644*y + 1.7927*ch.y;
        rgb.g = 1.1644*y - 0.2132*ch.x - 0.5329*ch.y;
        rgb.b = 1.1644*y + 2.1124*ch.x;
    }else{
        rgb.r = 1.1644*y + 1.5960*ch.y;
        rgb.g = 1.1644*y - 0.3918*ch.x - 0.8130*ch.y;
        rgb.b = 1.1644*y + 2.0172*ch.x;
    }
    gl_FragColor = vec4(clamp(rgb,0.0,1.0),1.0);
})";
static const char* kLineVS = R"(
attribute highp vec2 aPos;              // 视频像素坐标
uniform highp mat3 uMVP;
void main(){ gl_Position=vec4((uMVP*vec3(aPos,1.0)).xy,0.0,1.0); })";

static const char* kLineFS = R"(
uniform lowp vec4 uColor;
void main(){ gl_FragColor=uColor; })";

VideoGLWidget::VideoGLWidget(QWidget* p):QOpenGLWidget(p)
  ,m_vbo(QOpenGLBuffer::VertexBuffer)
  ,m_lineVbo(QOpenGLBuffer::VertexBuffer){
    // ★ 关键: QOpenGLWidget不会自动重绘,paintGL是唯一取帧入口,必须有持续的重绘驱动
    m_repaintTimer = new QTimer(this);
    connect(m_repaintTimer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
    m_repaintTimer->start(33);   // ~30fps,与摄像头帧率匹配
}

void VideoGLWidget::clearFrame()
{
    m_cur=FramePacket(); m_texW=0;
    m_contour.clear(); m_manualVisible=false; m_dragVisible=false;
    update();
}

void VideoGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.07f,0.08f,0.09f,1.f);
    //把 YUV 纹理算成彩色视频画面
    m_prog=new QOpenGLShaderProgram(this);
    m_prog->addShaderFromSourceCode(QOpenGLShader::Vertex,kVS);
    m_prog->addShaderFromSourceCode(QOpenGLShader::Fragment,kFS);
    m_prog->link();
    m_aPos=m_prog->attributeLocation("aPos");
    m_mvpLoc=m_prog->uniformLocation("uMVP");
    //用纯色描出一圈检测绿框
    m_lineProg=new QOpenGLShaderProgram(this);
    m_lineProg->addShaderFromSourceCode(QOpenGLShader::Vertex,kLineVS);
    m_lineProg->addShaderFromSourceCode(QOpenGLShader::Fragment,kLineFS);
    m_lineProg->link();
    m_lineApos=m_lineProg->attributeLocation("aPos");
    m_lineMvpLoc=m_lineProg->uniformLocation("uMVP");

    static const GLfloat quad[]={-0.5f,-0.5f, 0.5f,-0.5f, -0.5f,0.5f, 0.5f,0.5f};//四个顶点
    m_vbo.create(); m_vbo.bind(); m_vbo.allocate(quad,sizeof(quad)); m_vbo.release();
    m_lineVbo.create();

    glGenTextures(1,&m_texY); glGenTextures(1,&m_texU); glGenTextures(1,&m_texV);
    for(GLuint t:{m_texY,m_texU,m_texV}){
        glBindTexture(GL_TEXTURE_2D,t);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    }
}

void VideoGLWidget::ensureTexSize()
{
    const int key=(m_cur.fmt==FramePacket::FMT_YUYV422)?2:1;
    if(m_texW==m_cur.width && m_texH==m_cur.height && m_texFmtKey==key) return;
    m_texW=m_cur.width; m_texH=m_cur.height; m_texFmtKey=key;
    if(key==1){   // GL_LUMINANCE兼容GL2.1~4.6;若将来上core profile改GL_R8/GL_RED
        glBindTexture(GL_TEXTURE_2D,m_texY);
        glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,m_texW,m_texH,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,nullptr);
        glBindTexture(GL_TEXTURE_2D,m_texU);
        glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,m_texW/2,m_texH/2,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,nullptr);
        glBindTexture(GL_TEXTURE_2D,m_texV);
        glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,m_texW/2,m_texH/2,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,nullptr);
    }else{
        glBindTexture(GL_TEXTURE_2D,m_texY);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,m_texW/2,m_texH,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    }
}

void VideoGLWidget::uploadFrame(const FramePacket& f)
{
    static int lastW=0, lastH=0;
    if(f.width!=lastW || f.height!=lastH){
        lastW=f.width; lastH=f.height;
        qDebug()<<"[GL] 实际解码帧:"<<f.width<<"x"<<f.height
                <<" fmt="<<f.fmt
                <<" widget="<<width()<<"x"<<height()
                <<" dpr="<<devicePixelRatioF();
    }

    m_cur = f;   // ★ 原来只拷贝了宽高参数,把plane原始数据丢了。整包保存,拍照直接用原始数据
    ensureTexSize();
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    if(m_cur.fmt==FramePacket::FMT_YUV420P){
        glBindTexture(GL_TEXTURE_2D,m_texY);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,m_cur.width,m_cur.height,GL_LUMINANCE,GL_UNSIGNED_BYTE,m_cur.plane[0].constData());
        const int cw=m_cur.width/2, ch=m_cur.height/2;
        glBindTexture(GL_TEXTURE_2D,m_texU);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,cw,ch,GL_LUMINANCE,GL_UNSIGNED_BYTE,m_cur.plane[1].constData());
        glBindTexture(GL_TEXTURE_2D,m_texV);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,cw,ch,GL_LUMINANCE,GL_UNSIGNED_BYTE,m_cur.plane[2].constData());
    }else{
        glBindTexture(GL_TEXTURE_2D,m_texY);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,m_cur.width/2,m_cur.height,GL_RGBA,GL_UNSIGNED_BYTE,m_cur.plane[0].constData());
    }
}


// 视频像素→旋转→等比缩放居中→clip。m_a同时保存"视频→窗口"仿射,供鼠标反算。
void VideoGLWidget::updateXform()
{
    const double winW=qMax(1,width()), winH=qMax(1,height());
    const bool swap=(m_rot==90||m_rot==270);
    const double cw=swap?m_cur.height:m_cur.width;
    const double ch=swap?m_cur.width:m_cur.height;
    if(cw<1||ch<1) return;
    const double s=qMin(winW*m_zoom/cw, winH*m_zoom/ch);       // KeepAspectRatio,只算一次
    const double ox=(winW-s*cw)/2.0, oy=(winH-s*ch)/2.0;

    double r00=1,r01=0,r02=0,r10=0,r11=1,r12=0;                // y向下坐标系,顺时针
    switch(m_rot){
    case 90:  r00=0; r01=-1; r02=m_cur.height; r10=1;  r11=0;  r12=0;            break;
    case 180: r00=-1;r01=0;  r02=m_cur.width;  r10=0;  r11=-1; r12=m_cur.height; break;
    case 270: r00=0; r01=1;  r02=0;            r10=-1; r11=0;  r12=m_cur.width;  break;
    }
    m_a[0]=s*r00; m_a[1]=s*r01; m_a[2]=s*r02+ox;
    m_a[3]=s*r10; m_a[4]=s*r11; m_a[5]=s*r12+oy;
    const double c0=2.0/winW, c1=-2.0/winH;                    // 窗口像素→clip(y翻转)
    m_mvp[0]=GLfloat(c0*m_a[0]); m_mvp[1]=GLfloat(c1*m_a[3]); m_mvp[2]=0;
    m_mvp[3]=GLfloat(c0*m_a[1]); m_mvp[4]=GLfloat(c1*m_a[4]); m_mvp[5]=0;
    m_mvp[6]=GLfloat(c0*m_a[2]-1); m_mvp[7]=GLfloat(c1*m_a[5]+1); m_mvp[8]=1;
}

void VideoGLWidget::drawVideo(const GLfloat* mvp9)
{
    // CPU生成视频像素坐标quad: 与updateXform同一数据源(m_cur)
    const GLfloat w = GLfloat(m_cur.width), h = GLfloat(m_cur.height);
    const GLfloat quad[16] = { 0,0, 0,0,   w,0, 1,0,   0,h, 0,1,   w,h, 1,1 };

    m_prog->bind();
    m_vbo.bind();
    m_vbo.allocate(quad, sizeof(quad));
    m_prog->enableAttributeArray(m_aPos);
    m_prog->setAttributeBuffer(m_aPos, GL_FLOAT, 0, 2, 4*sizeof(GLfloat));
    const int aUV = m_prog->attributeLocation("aUV");
    m_prog->enableAttributeArray(aUV);
    m_prog->setAttributeBuffer(aUV, GL_FLOAT, 2*sizeof(GLfloat), 2, 4*sizeof(GLfloat));

    // uVideoSize 只剩fragment里YUYV分支在用,几何已不依赖它
    m_prog->setUniformValue("uVideoSize", QVector2D(float(m_cur.width), float(m_cur.height)));
    m_prog->setUniformValue("uFmt", m_cur.fmt);
    m_prog->setUniformValue("uFullRange", m_cur.fullRange?1:0);
    m_prog->setUniformValue("uBt709", m_bt709?1:0);
    glUniformMatrix3fv(m_mvpLoc, 1, GL_FALSE, mvp9);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_texY); m_prog->setUniformValue("texY", 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_cur.fmt==2?m_texY:m_texU); m_prog->setUniformValue("texUV", 1);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_cur.fmt==2?m_texY:m_texV); m_prog->setUniformValue("texV", 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_prog->disableAttributeArray(m_aPos);
    m_prog->disableAttributeArray(aUV);
    m_vbo.release(); m_prog->release();
}


void VideoGLWidget::drawContour()
{
    if(m_contour.size()!=4 || m_cur.width==0) return;
    GLfloat pts[8];
    for(int i=0;i<4;++i){ pts[2*i]=float(m_contour[i].x()); pts[2*i+1]=float(m_contour[i].y()); }
    m_lineVbo.bind(); m_lineVbo.allocate(pts,sizeof(pts));
    m_lineProg->bind();
    m_lineProg->enableAttributeArray(m_lineApos);
    m_lineProg->setAttributeBuffer(m_lineApos,GL_FLOAT,0,2,0);
    glUniformMatrix3fv(m_lineMvpLoc,1,GL_FALSE,m_mvp);   // 与视频同一矩阵→永远贴合
    m_lineProg->setUniformValue("uColor",QVector4D(0.f,1.f,0.f,1.f));
    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_LOOP,0,4);
    m_lineProg->disableAttributeArray(m_lineApos);
    m_lineVbo.release(); m_lineProg->release();
}

void VideoGLWidget::paintGL()
{
    // ★ QPainter会改写viewport为逻辑尺寸,而FBO是物理尺寸(逻辑×DPR)。
    //   每帧原生GL绘制前必须恢复为整个FBO,否则画面被压进左上角。
    const qreal dpr = devicePixelRatioF();
    glViewport(0, 0, int(width()*dpr), int(height()*dpr));
    // ★ QPainter不保证恢复其改动的GL状态,这些是我们纹理上传/绘制依赖的
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_SCISSOR_TEST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glClear(GL_COLOR_BUFFER_BIT);
    FramePacket pkt;
    // 从邮箱取最新帧 → 上传纹理 → 算矩阵
    if(m_mail && m_mail->fetch(pkt)){
        uploadFrame(pkt); //把 YUV 数据上传成 GPU 纹理
        updateXform();
    }
    if(m_cur.width>0) drawVideo(m_mvp);//绑定着色器、绑定纹理
    drawContour();

    if(m_cur.width == 0){
        // ★占位文字在GL控件内实现:先栅格化到QImage(CPU光栅渲染,和QLabel同一引擎,清晰),
        //   再整图1:1贴上。只在无帧分支出现,物理上不可能遮挡视频画面。
        const qreal dpr = devicePixelRatioF();
        const int iw = qMax(1,int(width()*dpr)), ih = qMax(1,int(height()*dpr));
        if(m_phImg.isNull() || m_phImg.width()!=iw || m_phImg.height()!=ih){
            m_phImg = QImage(iw,ih,QImage::Format_RGBA8888_Premultiplied);
            m_phImg.setDevicePixelRatio(dpr);
            m_phImg.fill(Qt::transparent);
            QPainter ip(&m_phImg);
            ip.setRenderHint(QPainter::TextAntialiasing,true);
            QFont f=ip.font(); f.setPointSize(16); f.setBold(true);
            ip.setFont(f);
            ip.setPen(QColor(235,235,235));
            ip.drawText(QRect(0,0,width(),height()),Qt::AlignCenter,
                        QStringLiteral("点击\"打开摄像头\"开始预览"));
            ip.end();
        }
        QPainter p(this);
        p.fillRect(rect(),QColor(18,18,18));      // 黑底(和原GL清屏色一致)
        p.drawImage(rect(),m_phImg);              // 1:1物理像素贴图,不糊
        p.end();
    }else{
        QPainter p(this);                                     // QPainter叠加层(只画矩形)
        p.setRenderHint(QPainter::Antialiasing);
        if(m_manualVisible && m_manualRect.width()>1){        // 视频坐标→窗口坐标
            const QPointF a=QPointF(m_a[0]*m_manualRect.left()+m_a[1]*m_manualRect.top()+m_a[2],
                                    m_a[3]*m_manualRect.left()+m_a[4]*m_manualRect.top()+m_a[5]);
            const QPointF b=QPointF(m_a[0]*m_manualRect.right()+m_a[1]*m_manualRect.bottom()+m_a[2],
                                    m_a[3]*m_manualRect.right()+m_a[4]*m_manualRect.bottom()+m_a[5]);
            p.setPen(QPen(Qt::red,2)); p.setBrush(QColor(255,0,0,30));
            p.drawRect(QRectF(a,b).normalized());
        }
        if(m_dragVisible){
            p.setPen(QPen(Qt::yellow,2,Qt::DashLine)); p.setBrush(QColor(255,255,0,50));
            p.drawRect(m_dragRect);
        }
        p.end();
    }
}

QPointF VideoGLWidget::widgetToVideo(const QPointF& p) const
{
    const double det=m_a[0]*m_a[4]-m_a[1]*m_a[3];
    if(qFuzzyIsNull(det)) return QPointF();
    const double dx=p.x()-m_a[2], dy=p.y()-m_a[5];
    return QPointF(( m_a[4]*dx-m_a[1]*dy)/det,
                   (-m_a[3]*dx+m_a[0]*dy)/det);
}

// ===== YUV→BGR整帧CPU转换(sws,无任何缩放,和旧版latestHighResFrame完全同源) =====
static cv::Mat packetToBgr(const FramePacket& f)
{
    if(f.width<=0 || f.height<=0) return cv::Mat();
    AVPixelFormat src = AV_PIX_FMT_NONE;
    int stride[3] = { f.width, f.width/2, f.width/2 };
    if(f.fmt==FramePacket::FMT_YUV420P){
        src = AV_PIX_FMT_YUV420P;
    }else if(f.fmt==FramePacket::FMT_YUYV422){
        src = AV_PIX_FMT_YUYV422;
        stride[0] = f.width*2;      // 打包格式,单平面
    }else{
        return cv::Mat();
    }

    // 仅UI线程调用,静态复用;尺寸/格式变化时自动重建
    static SwsContext* sws = nullptr;
    static int lastW=0, lastH=0;
    static AVPixelFormat lastFmt = AV_PIX_FMT_NONE;
    if(!sws || lastW!=f.width || lastH!=f.height || lastFmt!=src){
        sws_freeContext(sws);
        sws = sws_getContext(f.width,f.height,src,
                             f.width,f.height,AV_PIX_FMT_BGR24,
                             SWS_BICUBIC,nullptr,nullptr,nullptr);
        lastW=f.width; lastH=f.height; lastFmt=src;
    }
    if(!sws) return cv::Mat();

    cv::Mat bgr(f.height, f.width, CV_8UC3);
    const uint8_t* srcPlanes[3] = {
        (const uint8_t*)f.plane[0].constData(),
        (const uint8_t*)f.plane[1].constData(),
        (const uint8_t*)f.plane[2].constData() };
    uint8_t* dst[1]      = { bgr.data };
    int      dstStride[1]= { int(bgr.step) };
    sws_scale(sws, srcPlanes, stride, 0, f.height, dst, dstStride);
    return bgr;
}

cv::Mat VideoGLWidget::grabFrameMat(bool rotated)
{
    // ★ 拍照不经过GPU:直接用摄像头原始帧整帧转换。
    //   选中什么分辨率,拍出来就是什么分辨率;裁剪也是从这张全分辨率原图上裁。
    if(m_cur.width<=0 || m_cur.height<=0) return cv::Mat();
    qDebug() << "[Capture] 原图帧:" << m_cur.width << "x" << m_cur.height;
    cv::Mat bgr = packetToBgr(m_cur);
    if(bgr.empty()) return cv::Mat();
    if(rotated){
        const int r = ((m_rot%360)+360)%360;
        if(r==90)  cv::rotate(bgr,bgr,cv::ROTATE_90_CLOCKWISE);
        if(r==180) cv::rotate(bgr,bgr,cv::ROTATE_180);
        if(r==270) cv::rotate(bgr,bgr,cv::ROTATE_90_COUNTERCLOCKWISE);
    }
    return bgr;   // BGR24、全分辨率,与旧版latestHighResFrame同构
}

