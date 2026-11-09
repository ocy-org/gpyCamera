#include "frameutils.h"
#include <QString>
#include <QDebug>
#include <opencv2/imgproc.hpp>
extern "C" {
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <string.h>

static QString avErrStr(int e){ char b[256]; av_strerror(e,b,sizeof(b)); return QString::fromUtf8(b); }

FramePacket makeFramePacket(const AVFrame* f)
{
    FramePacket p;
    p.width=f->width; p.height=f->height;
    if(f->format==AV_PIX_FMT_YUVJ420P || f->format==AV_PIX_FMT_YUV420P){
        p.fmt=FramePacket::FMT_YUV420P;
        p.fullRange=(f->format==AV_PIX_FMT_YUVJ420P);
        const int cw=f->width/2, ch=f->height/2;
        for(int i=0;i<3;++i){
            const int w=(i==0)?f->width:cw, h=(i==0)?f->height:ch;
            p.plane[i].resize(size_t(w)*h);
            const uint8_t* src=f->data[i];
            uint8_t* dst=reinterpret_cast<uint8_t*>(p.plane[i].data());
            if(f->linesize[i]==w) memcpy(dst,src,size_t(w)*h);   // 无padding,整块拷
            else for(int r=0;r<h;++r)                             // 按行去掉padding
                memcpy(dst+size_t(r)*w, src+size_t(r)*f->linesize[i], size_t(w));
        }
    } else if(f->format==AV_PIX_FMT_YUYV422){
        p.fmt=FramePacket::FMT_YUYV422;
        p.fullRange=true;                                         // UVC的YUYV恒为full range
        const int w=f->width*2, h=f->height;
        p.plane[0].resize(size_t(w)*h);
        const uint8_t* src=f->data[0];
        uint8_t* dst=reinterpret_cast<uint8_t*>(p.plane[0].data());
        if(f->linesize[0]==w) memcpy(dst,src,size_t(w)*h);
        else for(int r=0;r<h;++r) memcpy(dst+size_t(r)*w, src+size_t(r)*f->linesize[0], size_t(w));
    }

    // ★ 兜底: 其它格式(典型如YUVJ422P——部分摄像头mjpeg是4:2:2)统一sws转YUV420P
    static thread_local SwsContext* cvt=nullptr;
    cvt=sws_getCachedContext(cvt,f->width,f->height,(AVPixelFormat)f->format,
                             f->width,f->height,AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,nullptr,nullptr,nullptr);
    if(!cvt){ qWarning()<<"makeFramePacket: 创建转换器失败 format="<<f->format; return p; }
    static int warned=0;
    if(warned++<3) qWarning()<<"makeFramePacket: 使用兜底转换 format="<<f->format;

    p.fmt=FramePacket::FMT_YUV420P;
    p.fullRange=true;
    const int cw=f->width/2, ch=f->height/2;
    p.plane[0].resize(size_t(f->width)*f->height);
    p.plane[1].resize(size_t(cw)*ch);
    p.plane[2].resize(size_t(cw)*ch);
    uint8_t* d[3]={ (uint8_t*)p.plane[0].data(),
                    (uint8_t*)p.plane[1].data(),
                    (uint8_t*)p.plane[2].data() };
    int ds[3]={ f->width, cw, cw };
    sws_scale(cvt, f->data, f->linesize, 0, f->height, d, ds);
    return p;
}

cv::Mat framePacketToBgr(const FramePacket& pkt)
{
    if(!pkt.isValid()) return {};
    const AVPixelFormat in = (pkt.fmt==FramePacket::FMT_YUV420P)
        ? (pkt.fullRange?AV_PIX_FMT_YUVJ420P:AV_PIX_FMT_YUV420P)
        : AV_PIX_FMT_YUYV422;
    static thread_local SwsContext* ctx=nullptr;                  // 每线程缓存,避免反复创建
    ctx=sws_getCachedContext(ctx,pkt.width,pkt.height,in,
                             pkt.width,pkt.height,AV_PIX_FMT_BGR24,
                             SWS_BILINEAR,nullptr,nullptr,nullptr);
    if(!ctx) return {};
    cv::Mat dst(pkt.height,pkt.width,CV_8UC3);
    const uint8_t* sp[3]={ reinterpret_cast<const uint8_t*>(pkt.plane[0].constData()),
                           reinterpret_cast<const uint8_t*>(pkt.plane[1].constData()),
                           reinterpret_cast<const uint8_t*>(pkt.plane[2].constData()) };
    int sl[3];
    if(pkt.fmt==FramePacket::FMT_YUV420P){ sl[0]=pkt.width; sl[1]=sl[2]=pkt.width/2; }
    else { sl[0]=pkt.width*2; sl[1]=sl[2]=0; }
    uint8_t* dp[3]={dst.data,nullptr,nullptr};
    int dl[3]={int(dst.step),0,0};
    sws_scale(ctx,sp,sl,0,pkt.height,dp,dl);
    return dst;
}
