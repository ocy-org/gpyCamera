#ifndef COMM_DEFINE_H
#define COMM_DEFINE_H
#define JSONFILEPATH "./camera.json"
#define CONFFILE "./base.ini"
#define MANUALCROPFILE "./manualcrop.conf"
#define CAMERACONTROLFILE "./cameracontrol.json"
#define ZEROCODEFILESAVEPATH "D:/ymore/GpyCamera/gpycamera/ZeroCodeFilesBuf"
#include <QString>
#include <QSettings>
#include <QPixmap>
typedef struct  CameraMsg
{
    QString name;
    QString maincamera;
    QString subcamera;  //时间戳
    CameraMsg(){
        name="";
        maincamera="";
        subcamera="";
    }
} stCameraMsg;
typedef struct  ImgMsg
{
    QString name;
    QPixmap pixmap;
    ImgMsg(){
        name="";
    }
} stImgMsg;



typedef struct OriginalParams {
    int brightness = -1;
    int contrast = -1;
    int hue = -1;
    int saturation = -1;
    int sharpness = -1;
    int gamma = -1;
    int whiteBalance = -1;
    int backlight = -1;
    int gain = -1;
    int exposure = -1;
    bool autoWhiteBalance = true;
    bool autoExposure = true;
} st_originalParams;

#endif // COMM_DEFINE_H
