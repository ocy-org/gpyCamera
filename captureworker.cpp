#include "captureworker.h"
#include "frameutils.h"
#include <QElapsedTimer>

// 校验设备实际输出分辨率;返回空串=通过,非空=错误信息
static QString checkNegotiatedResolution(AVFormatContext *fmtCtx, int videoStream,
                                         const QString &requested)
{
    if(!fmtCtx || videoStream < 0) return QString();
    AVCodecParameters *par = fmtCtx->streams[videoStream]->codecpar;
    int reqW = 0, reqH = 0;
    if(sscanf(requested.toUtf8().constData(), "%dx%d", &reqW, &reqH) != 2
       || reqW <= 0 || reqH <= 0 || par->width <= 0)
        return QString();
    if(par->width == reqW && par->height == reqH) return QString();
    return QString("分辨率协商失败!\n请求: %1x%2\n设备实际输出: %3x%4\n\n"
                   "请重新选择该摄像头支持的分辨率。")
           .arg(reqW).arg(reqH).arg(par->width).arg(par->height);
}

static QString avErrStr(int e){ char b[256]; av_strerror(e,b,sizeof(b)); return QString::fromUtf8(b); }
static int interruptCallback(void* opaque){
    return static_cast<CaptureWorker*>(opaque)->stopRequested() ? 1 : 0;
}

CaptureWorker::CaptureWorker(QObject* parent):QObject(parent),
    m_glMail(new FrameMailbox), m_detMail(new FrameMailbox)
{ avdevice_register_all(); }

CaptureWorker::~CaptureWorker(){ delete m_glMail; delete m_detMail; closeDevice(); }

void CaptureWorker::closeDevice(){
    if(m_codecCtx){ avcodec_free_context(&m_codecCtx); m_codecCtx=nullptr; }
    if(m_fmt){ avformat_close_input(&m_fmt); m_fmt=nullptr; }
    m_videoStream=-1;
}

bool CaptureWorker::openDevice(const QString& deviceName,const QString& resolution,
                               const QString& codec,double fps,QString& err)
{
    AVDictionary* opts=nullptr;
    av_dict_set(&opts,"video_size",resolution.toUtf8().constData(),0);
    av_dict_set(&opts,"framerate",QString::number(fps,'f',0).toUtf8().constData(),0);
    if(!codec.isEmpty() && codec!="auto"){
        av_dict_set(&opts,"vcodec",codec.toUtf8().constData(),0); // dshow不识别此键(无害,忽略);解码器在下方按实际codec_id自动选
    }

#ifdef Q_OS_WINDOWS
    av_dict_set(&opts,"fflags","nobuffer+flush_packets",0);
    av_dict_set(&opts,"flags","low_delay",0);
    av_dict_set(&opts,"framedrop","1",0);
    av_dict_set(&opts,"rtbufsize","102400",0);
    av_dict_set(&opts,"analyzeduration","0",0);
    av_dict_set(&opts,"probesize","32768",0);
    // --- dshow 特有参数
    av_dict_set(&opts,"audio_buffer_size","1",0);
    av_dict_set(&opts,"crossbar_video_input_pin_number","0",0);
#elif defined(Q_OS_LINUX)
    // Linux参数
    av_dict_set(&options, "fflags", "nobuffer", 0);
    av_dict_set(&options, "flags", "low_delay", 0);
    av_dict_set(&options, "framedrop", "1", 0);
    av_dict_set(&options, "rtbufsize", "102400", 0);
    av_dict_set(&options, "analyzeduration", "10000", 0); // Linux下需要适当分析
    av_dict_set(&options, "probesize", "50000", 0);
#endif

    auto bail=[&](const QString& e){ err=e; av_dict_free(&opts); closeDevice(); return false; };

    m_fmt=avformat_alloc_context();
    if(!m_fmt) return bail("分配format context失败");

    m_fmt->interrupt_callback.callback=interruptCallback; // stop()能打断阻塞的open/read
    m_fmt->interrupt_callback.opaque=this;

#if defined(Q_OS_WINDOWS)
    const AVInputFormat* ifmt=av_find_input_format("dshow");
    const QByteArray dev="video="+deviceName.toUtf8();
#else
    const AVInputFormat* ifmt=av_find_input_format("v4l2");
    const QByteArray dev=deviceName.toUtf8();
#endif
    if(!ifmt) return bail("未找到dshow/v4l2输入格式");

    int ret=avformat_open_input(&m_fmt,dev.constData(),ifmt,&opts);
    av_dict_free(&opts); opts=nullptr;
    if(ret<0) return bail(QString("打开设备失败: %1").arg(avErrStr(ret)));

    if(avformat_find_stream_info(m_fmt,nullptr)<0)
        return bail("获取流信息失败");

    // 查找视频流
    m_videoStream=-1;
    for(unsigned i=0;i<m_fmt->nb_streams;++i)
        if(m_fmt->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){ m_videoStream=int(i); break; }
    if(m_videoStream<0) return bail("未找到视频流");

    //分辨率硬校验
    {
        const QString vErr=checkNegotiatedResolution(m_fmt,m_videoStream,resolution);
        if(!vErr.isEmpty()) return bail(vErr);   // bail内部会closeDevice并返回false
    }

    // 获取解码器
    const AVCodecParameters* par=m_fmt->streams[m_videoStream]->codecpar;
    const AVCodec* dec=avcodec_find_decoder(par->codec_id);
    if(!dec) return bail("未找到解码器");

    m_codecCtx=avcodec_alloc_context3(dec);
    if(!m_codecCtx || avcodec_parameters_to_context(m_codecCtx,par)<0)
        return bail("解码器初始化失败");
    if(avcodec_open2(m_codecCtx,dec,nullptr)<0)
        return bail("解码器打开失败");

    qDebug() << "[Capture] 协商完成:" << par->width << "x" << par->height
             << avcodec_get_name(par->codec_id);
    return true;
}


void CaptureWorker::start(QString deviceName,QString resolution,QString codec,double fps)
{
    m_stop=false;
    QString err;
    if(!openDevice(deviceName,resolution,codec,fps,err)){ emit errorOccurred(err); emit finished(); return; }
    emit opened();

    AVPacket* pkt=av_packet_alloc();
    AVFrame*  frm=av_frame_alloc();
    QElapsedTimer fpsClock; fpsClock.start();
    int cnt=0;
    while(!m_stop.load()){
        int ret=av_read_frame(m_fmt,pkt);
        if(ret<0){
            if(ret==AVERROR(EAGAIN)){ av_usleep(2000); continue; }
            emit errorOccurred(QString("读取帧失败: %1").arg(avErrStr(ret)));
            break;
        }
        if(pkt->stream_index==m_videoStream && avcodec_send_packet(m_codecCtx,pkt)>=0){
            while(!m_stop.load()){
                int r=avcodec_receive_frame(m_codecCtx,frm);
                if(r!=0) break;                       // EAGAIN/EOF/错误
                FramePacket p=makeFramePacket(frm);
                if(p.isValid()){
                    p.seq=m_seq.fetch_add(1);
                    m_glMail->publish(p);
                    m_detMail->publish(p);
                    if(++cnt>=30){
                        emit statsUpdated(cnt*1000.0/qMax<qint64>(1,fpsClock.elapsed()),
                                          QString("%1x%2 %3").arg(m_codecCtx->width).arg(m_codecCtx->height)
                                                                               .arg(avcodec_get_name(m_codecCtx->codec_id)));
                        cnt=0; fpsClock.restart();
                    }
                }
                av_frame_unref(frm);
            }
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt); av_frame_free(&frm);
    closeDevice();
    emit finished();
}
