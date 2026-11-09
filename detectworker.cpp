#include "detectworker.h"
#include "frameutils.h"

DetectWorker::DetectWorker(QObject* parent):QObject(parent){}

void DetectWorker::startLoop(){
    if(!m_timer){ m_timer=new QTimer(this); connect(m_timer,&QTimer::timeout,this,&DetectWorker::onTick); }
    m_timer->start(m_intervalMs);
}

void DetectWorker::onTick()
{
    if(!m_mail) return;
    FramePacket pkt;
    if(!m_mail->fetch(pkt)) return;
    cv::Mat bgr=framePacketToBgr(pkt);           // 仅~7fps发生,且不在UI线程
    if(bgr.empty()) return;
    const std::vector<cv::Point> pts=m_detector.detectContour(bgr);  // 算法零改动
    if(pts.size()==4){
        QPolygonF poly;
        for(const auto& p:pts) poly<<QPointF(qreal(p.x),qreal(p.y));
        emit contourReady(poly);                 // 传感器像素坐标,旋转由GL矩阵处理
    }
}
