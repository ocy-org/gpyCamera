#ifndef QRCODESCAN_H
#define QRCODESCAN_H

#include <QWidget>
#include <QCamera>
#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include "zxing_compat.h"  // 必须在包含ZXing头文件之前
// ZXing 头文件
#include <ZXing/BarcodeFormat.h>
#include <ZXing/DecodeHints.h>
#include <ZXing/Result.h>
#include <ZXing/ReadBarcode.h>

class QRCodeScanDialog  : public QDialog
{
    Q_OBJECT
public:
    explicit QRCodeScanDialog(QWidget *parent = nullptr);
    ~QRCodeScanDialog();
    QString getQRContent() const { return qrContent; }
    void initcameraindex(int cameraindex);

private slots:
    void onImageCaptured(int id, const QImage &preview);
    void processFrame();
    void startCamera();
    void stopCamera();
    void onAcceptButton();
    void onCancelButton();
    //void loadImageForDebug();

private:
    int m_cameraindex=0;
    // UI组件
    QCameraViewfinder *viewfinder;
    QLabel *statusLabel;
    QPushButton *acceptButton;
    QPushButton *cancelButton;
    //QPushButton *debugButton;

    // 摄像头相关
    QCamera *camera;
    QCameraImageCapture *imageCapture;
    QTimer *processTimer;

    // 识别结果
    QString qrContent;

    // 二维码识别
    QString decodeQRCodeWithZXing(const QImage &image);
    void setupUI();
    void setupConnections();
};

#endif // QRCODESCAN_H
