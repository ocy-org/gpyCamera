#ifndef VIDEOGLWIDGET_H
#define VIDEOGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QPolygonF>
#include <QTimer>
#include <opencv2/core.hpp>
#include "framepacket.h"

class VideoGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit VideoGLWidget(QWidget* parent=nullptr);
    void setMailbox(FrameMailbox* m){ m_mail=m; }
    void setContour(const QPolygonF& polyVideo){ m_contour=polyVideo; update(); }
    void setManualRect(const QRectF& rcVideo){ m_manualRect=rcVideo; m_manualVisible=!rcVideo.isNull(); update(); }
    void setDragRect(const QRect& rcWidget,bool visible){ m_dragRect=rcWidget; m_dragVisible=visible; update(); }
    void zoomBy(double f){ m_zoom=qBound(1.0,m_zoom*f,3.0); updateXform(); update(); }
    void setRotation(int deg){ m_rot=((deg%360)+360)%360; updateXform(); update(); }
    void clearFrame();
    QPointF widgetToVideo(const QPointF& p) const;   // 手动矩形坐标反算
    cv::Mat grabFrameMat(bool rotated=true);         // 拍照:回读整帧(可选已旋转)
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int,int) override { updateXform(); }
private:
    void uploadFrame(const FramePacket& f);
    void ensureTexSize();
    void updateXform();
    void drawVideo(const GLfloat* mvp9);
    void drawContour();
    FrameMailbox* m_mail=nullptr;
    FramePacket m_cur;                               // 当前已上传帧的参数
    QPolygonF m_contour;
    QRectF m_manualRect; bool m_manualVisible=false;
    QRect  m_dragRect;   bool m_dragVisible=false;
    double m_zoom=1.0; int m_rot=0; bool m_bt709=true;   // 与原sws的ITU709一致
    double m_a[6]={1,0,0,0,1,0};                     // 视频像素→窗口像素 仿射
    GLfloat m_mvp[9]={1,0,0,0,1,0,0,0,1};            // 视频像素→clip (列主序)
    QOpenGLShaderProgram *m_prog=nullptr,*m_lineProg=nullptr;
    QOpenGLBuffer m_vbo, m_lineVbo;
    GLuint m_texY=0,m_texU=0,m_texV=0;
    int m_aPos=-1,m_mvpLoc=-1,m_lineApos=-1,m_lineMvpLoc=-1;
    int m_texW=0,m_texH=0,m_texFmtKey=0;
    QTimer* m_repaintTimer = nullptr;
    QImage m_phImg;   // 预渲染占位文字(物理分辨率栅格图,避免GL字形发虚)
};
#endif
