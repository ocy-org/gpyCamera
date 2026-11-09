#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QTimer>
#include "cameraprobing.h"

class CameraWorker : public QObject
{
    Q_OBJECT

public:
    explicit CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

public slots:
    void startDetection();//多线程执行的任务

signals:
    void camerasDetected(const QList<CameraInfo> &cameras);
    void detectionFailed(const QString &error);

private slots:
    void performDetection();  // 执行检测的槽函数

private:
    CameraProbing m_probing;
    QTimer *m_detecttimer;
};

#endif // CAMERAWORKER_H
