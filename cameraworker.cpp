// cameraworker.cpp
#include "cameraworker.h"
#include <QDebug>

CameraWorker::CameraWorker(QObject *parent) : QObject(parent)
{
    // 创建定时器
    m_detecttimer = new QTimer(this);
    m_detecttimer->setInterval(3000); // 3秒间隔
    // 连接定时器超时信号到检测槽函数
    connect(m_detecttimer, &QTimer::timeout, this, &CameraWorker::performDetection);
}

CameraWorker::~CameraWorker()
{
    // 停止定时器
    if (m_detecttimer && m_detecttimer->isActive()) {
        m_detecttimer->stop();
    }
}
void CameraWorker::startDetection()
{
    qDebug() << "启动定时摄像头检测，每隔5秒检测一次";

    // 立即执行一次检测
    performDetection();

    // 启动定时器
    m_detecttimer->start();
}

void CameraWorker::performDetection()
{
    //qDebug() << "在后台线程中开始摄像头检测...";
    try {
        CameraProbing camerapro;
        QList<CameraInfo> cameras = camerapro.getCameraList();

        if (cameras.isEmpty()) {
            emit detectionFailed("未检测到任何摄像头设备");
        } else {
            emit camerasDetected(cameras);
        }
    } catch (const std::exception &e) {
        emit detectionFailed(QString("摄像头检测异常: %1").arg(e.what()));
    }
}
