#ifndef FRAMEUTILS_H
#define FRAMEUTILS_H
#include "framepacket.h"
#include <opencv2/core.hpp>
struct AVFrame;
FramePacket makeFramePacket(const AVFrame* f);       // 仅captureworker.cpp调用
cv::Mat framePacketToBgr(const FramePacket& pkt);    // 检测线程/拍照用
#endif
