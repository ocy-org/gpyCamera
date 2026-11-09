#ifndef CAPTUREWORKER_H
#define CAPTUREWORKER_H
#include <QObject>
#include <QThread>
#include <atomic>
#include <QDebug>
#include "framepacket.h"
extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
}

class CaptureWorker : public QObject
{
    Q_OBJECT
public:
    explicit CaptureWorker(QObject* parent=nullptr);
    ~CaptureWorker() override;
    FrameMailbox* glMailbox()  const { return m_glMail; }   // 给渲染
    FrameMailbox* detMailbox() const { return m_detMail; }  // 给检测
    bool stopRequested() const { return m_stop.load(); }

public slots:
    void start(QString deviceName, QString resolution, QString codec, double fps);
    void stop(){ m_stop=true; }     // 原子标志,任意线程可调,采集循环一帧内退出

signals:
    void opened();
    void errorOccurred(const QString& msg);
    void statsUpdated(double fps, const QString& codec);
    void finished();

private:
    bool openDevice(const QString& dev,const QString& res,const QString& codec,double fps,QString& err);
    void closeDevice();
    FrameMailbox* m_glMail;
    FrameMailbox* m_detMail;
    std::atomic<bool> m_stop{false};
    std::atomic<quint64> m_seq{0};
    AVFormatContext* m_fmt=nullptr;
    AVCodecContext*  m_codecCtx=nullptr;
    int m_videoStream=-1;
};
#endif
