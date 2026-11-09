#ifndef METATYPES_H
#define METATYPES_H
#include <QSize>
#include <QString>
#include <QStringList>
#include <QMetaType>
// 摄像头能力信息结构体
struct CameraCapability {
    QSize resolution;
    QString codec;          // 编码格式
    double maxFps;          // 最大帧率
    QString pixelFormat;    // 像素格式
    bool isPreferred;       // 是否为首选（MJPG等）
    //默认值
    CameraCapability() : resolution(640, 480), codec("mjpeg"), maxFps(30.0),
                        pixelFormat("yuyv422"), isPreferred(false) {}
    //参数传值
    CameraCapability(QSize res, QString c, double fps, QString pf = "yuyv422", bool preferred = false)
        : resolution(res), codec(c), maxFps(fps), pixelFormat(pf), isPreferred(preferred) {}
};

// 摄像头信息结构体
struct CameraInfo {
    QString deviceName;
    QString description;
    QList<CameraCapability> formats;
};

Q_DECLARE_METATYPE(CameraCapability)
Q_DECLARE_METATYPE(CameraInfo)

#endif // METATYPES_H
