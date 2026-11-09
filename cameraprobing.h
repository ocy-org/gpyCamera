#ifndef CAMERAPROBING_H
#define CAMERAPROBING_H

#include <QObject>
#include <QString>
#include <QSize>
#include <QList>
#include <QProcess>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QMap>
#include <QCamera>
#include <QCameraInfo>
#include "metatypes.h"

class CameraProbing : public QObject
{
    Q_OBJECT

public:
    explicit CameraProbing(QObject *parent = nullptr);
    QList<CameraInfo> getCameraList();
    QList<CameraCapability> probeCameraCapabilities(const QString &cameraName);
    QList<CameraCapability> probeWithV4L2(const QString &cameraName);
private:
    QList<CameraCapability> parseFFmpegOutput(const QString &output, const QString &cameraName);
    QList<CameraCapability> parseV4L2Output(const QString &output, const QString &cameraName);
    QList<CameraCapability> probeWithDirectShow(const QString &cameraName);

    static QString executeFFmpegCommand(const QStringList &arguments);
    static QString executeProcess(const QString &program, const QStringList &arguments, int timeoutMs = 10000);
};

#endif // CAMERAPROBING_H
