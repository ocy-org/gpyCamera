#ifndef COMMFUNTION_H
#define COMMFUNTION_H
#include<QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "util/comm_define.h"

#include <string>
#include <QString>
class commfuntion
{
public:
    commfuntion();
    bool getconfbyjson(QMap<QString, stCameraMsg>& dataMap);
    bool insertjsondata(const stCameraMsg& cameramsg);
    bool deljsondata(const QString& key);

    std::string utf8ToGbk(const std::string& utf8);
    QString getDiskSerial();//获取硬盘序列号
    QString getCpuId();//获取CPU ID

    bool savePixmapToFile(const QString &filePath,const QPixmap &pixmap);//保存图片
    bool savePixmapToFilebypercent(const QString &filePath,const QPixmap &pixmap,const int& percent=100);//保存图片
    bool isImageFile(const QString &filePath);  // 检查是否为图片文件
    bool isInteger(const QString &str);// 验证是否是整数

private:
    QString executeCommand(const QString &command, const QStringList &args = QStringList());
    // Windows平台专用方法
#ifdef Q_OS_WIN
    QString getCpuIdWindows();
#endif
    // Linux平台专用方法
#ifdef Q_OS_LINUX
    QString getCpuIdLinux();
#endif
};

#endif // COMMFUNTION_H
