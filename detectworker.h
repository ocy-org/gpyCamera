#ifndef DETECTWORKER_H
#define DETECTWORKER_H
#include <QObject>
#include <QTimer>
#include <QPolygonF>
#include "framepacket.h"
#include "dealcontours.h"

class DetectWorker : public QObject
{
    Q_OBJECT
public:
    explicit DetectWorker(QObject* parent=nullptr);
    void setMailbox(FrameMailbox* m){ m_mail=m; }
    void setIntervalMs(int ms){ m_intervalMs=ms; }   // 检测节流,默认150ms≈7fps
public slots:
    void startLoop();                                // 在本线程内创建QTimer
    void stopLoop(){ if(m_timer) m_timer->stop(); }
signals:
    void contourReady(const QPolygonF& poly);        // 视频像素坐标
private slots:
    void onTick();
private:
    FrameMailbox* m_mail=nullptr;
    QTimer* m_timer=nullptr;
    int m_intervalMs=150;
    dealContours m_detector;                         // 线程私有实例,无共享状态
};
#endif
