#ifndef FRAMEPACKET_H
#define FRAMEPACKET_H
#include <QByteArray>
#include <QMutex>
#include <atomic>

// 解码后的一帧,纯数据载体。平面数据已去除linesize padding,紧凑存放。
struct FramePacket
{
    enum Format { FMT_NONE=0, FMT_YUV420P=1, FMT_YUYV422=2 };
    int  width=0, height=0;
    int  fmt=FMT_NONE;
    bool fullRange=false;          // YUVJ420P=true
    quint64 seq=0;
    QByteArray plane[3];           // YUYV只用plane[0]
    bool isValid() const { return fmt!=FMT_NONE && width>0 && height>0; }
};

// 最新帧邮箱:生产者覆盖,消费者取走。内存上界=1帧,永不堆积。
class FrameMailbox
{
public:
    void publish(FramePacket pkt){
        QMutexLocker l(&m_mtx);
        m_pkt = std::move(pkt);
    }
    bool fetch(FramePacket& out){
        QMutexLocker l(&m_mtx);
        if(!m_pkt.isValid()) return false;
        out = std::move(m_pkt);
        m_pkt = FramePacket();
        return true;
    }
private:
    QMutex m_mtx;
    FramePacket m_pkt;
};
#endif
