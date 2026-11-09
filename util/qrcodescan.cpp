#include "qrcodescan.h"


// 自定义 LuminanceSource 类（因为 ZXing 需要这个接口）
class QImageLuminanceSource {
private:
    QImage image;
    int width;
    int height;

public:
    QImageLuminanceSource(const QImage& img)
        : image(img), width(img.width()), height(img.height()) {
        if (image.format() != QImage::Format_Grayscale8) {
            image = image.convertToFormat(QImage::Format_Grayscale8);
        }
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // 简化版本，直接返回 QImage 数据
    const QImage& getImage() const { return image; }
};

QRCodeScanDialog::QRCodeScanDialog(QWidget *parent)
    : QDialog(parent), camera(nullptr), imageCapture(nullptr)
{
    setWindowTitle("QR Code Scanner");
    setModal(true);
    setFixedSize(600, 500);

    setupUI();
    setupConnections();
}

QRCodeScanDialog::~QRCodeScanDialog()
{
    stopCamera();
}

void QRCodeScanDialog::initcameraindex(int cameraindex){
    m_cameraindex=cameraindex;
    qDebug() << __FUNCTION__ << m_cameraindex;
    startCamera();
}

void QRCodeScanDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 视频显示区域
    viewfinder = new QCameraViewfinder(this);
    viewfinder->setMinimumSize(480, 360);
    mainLayout->addWidget(viewfinder);

    // 状态标签
    statusLabel = new QLabel("初始化摄像头...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 14px; color: blue; padding: 5px;");
    mainLayout->addWidget(statusLabel);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    acceptButton = new QPushButton("确认", this);
    acceptButton->setEnabled(false);
    cancelButton = new QPushButton("取消", this);
    //debugButton = new QPushButton("Load Image", this);

    //buttonLayout->addWidget(debugButton);
    //两个占位弹簧，把按钮挤到正中间
    buttonLayout->addStretch();//这是一个弹簧，占位的作用
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();//这是一个弹簧，占位的作用

    mainLayout->addLayout(buttonLayout);
}

void QRCodeScanDialog::setupConnections()
{
    connect(acceptButton, &QPushButton::clicked, this, &QRCodeScanDialog::onAcceptButton);
    connect(cancelButton, &QPushButton::clicked, this, &QRCodeScanDialog::onCancelButton);
    //connect(debugButton, &QPushButton::clicked, this, &QRCodeScanDialog::loadImageForDebug);

    // 处理定时器
    processTimer = new QTimer(this);
    processTimer->setInterval(500);
    connect(processTimer, &QTimer::timeout, this, &QRCodeScanDialog::processFrame);
}

void QRCodeScanDialog::startCamera()
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        statusLabel->setText("未找到摄像头!");
        return;
    }
    qDebug() << __FUNCTION__ << m_cameraindex;
    QCameraInfo cameraInfo=cameras.at(m_cameraindex);
    camera = new QCamera(cameraInfo, this);
    //camera = new QCamera(cameras.first(), this);
    camera->setViewfinder(viewfinder);

    imageCapture = new QCameraImageCapture(camera, this);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);

    connect(imageCapture, SIGNAL(imageCaptured(int,QImage)),
            this, SLOT(onImageCaptured(int,QImage)));

    camera->start();
    processTimer->start();

    statusLabel->setText("开始检测二维码...");
}

void QRCodeScanDialog::stopCamera()
{
    if (processTimer->isActive()) {
        processTimer->stop();
    }

    if (camera) {
        camera->stop();
        camera->deleteLater();
        camera = nullptr;
    }

    if (imageCapture) {
        imageCapture->deleteLater();
        imageCapture = nullptr;
    }
}

void QRCodeScanDialog::processFrame()
{
    if (imageCapture && camera && camera->state() == QCamera::ActiveState) {
        imageCapture->capture();
    }
}

void QRCodeScanDialog::onImageCaptured(int id, const QImage &image)
{
    Q_UNUSED(id)

    if (image.isNull()) {
        return;
    }

    QString result = decodeQRCodeWithZXing(image);
    if (!result.isEmpty()) {
        qrContent = result;
        QString showstr=QString("✓ 检测到二维码内容:%1").arg(result);
        statusLabel->setText(showstr);
        acceptButton->setEnabled(true);

        // 自动停止扫描
        stopCamera();
    } else {
        statusLabel->setText("正在检测二维码... (请把二维码放到摄像头摄像范围内)");
    }
}

//void QRCodeScanDialog::loadImageForDebug()
//{
//    QString fileName = QFileDialog::getOpenFileName(this,
//        "Open Image for Debug", QDir::homePath(),
//        "Image Files (*.png *.jpg *.jpeg *.bmp)");

//    if (fileName.isEmpty()) return;

//    QImage image(fileName);
//    if (image.isNull()) {
//        QMessageBox::warning(this, "Error", "Failed to load image!");
//        return;
//    }

//    QString result = decodeQRCodeWithZXing(image);
//    if (!result.isEmpty()) {
//        qrContent = result;
//        statusLabel->setText("✓ QR Code detected from image!");
//        acceptButton->setEnabled(true);
//    } else {
//        statusLabel->setText("No QR code found in image");
//    }
//}

QString QRCodeScanDialog::decodeQRCodeWithZXing(const QImage &image)
{
    try {
        // 预处理图像
        QImage processed = image;
        if (processed.width() > 800 || processed.height() > 600) {
            processed = processed.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        if (processed.format() != QImage::Format_Grayscale8) {
            processed = processed.convertToFormat(QImage::Format_Grayscale8);
        }

        // 使用 ImageView 类（新版本ZXing推荐方式）
        ZXing::ImageView imageView(
            processed.bits(),
            processed.width(),
            processed.height(),
            ZXing::ImageFormat::Lum  // 灰度格式
        );

        // 配置解码提示
        ZXing::DecodeHints hints;
        hints.setTryHarder(true);
        hints.setFormats(ZXing::BarcodeFormat::QRCode);

        // 正确的调用方式
        auto result = ZXing::ReadBarcode(imageView, hints);

        if (result.isValid()) {
            return QString::fromStdString(result.text());
        }

    } catch (const std::exception& e) {
        qDebug() << "ZXing decoding error:" << e.what();
    }

    return QString();
}

void QRCodeScanDialog::onAcceptButton()
{
    accept();
}

void QRCodeScanDialog::onCancelButton()
{
    reject();
}
