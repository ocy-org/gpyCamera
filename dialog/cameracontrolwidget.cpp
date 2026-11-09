#include "cameracontrolwidget.h"
#include "ui_cameracontrolwidget.h"
#include <QMessageBox>
#include <QMetaObject>
#include <QDebug>

CameraControlWidget::CameraControlWidget(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CameraControlWidget)
{
    ui->setupUi(this);
}

CameraControlWidget::~CameraControlWidget()
{
    delete ui;
    stopCamera();
}

void CameraControlWidget::init(const QString& cameraname){
    qDebug() << __FUNCTION__ << cameraname;
    m_cameraname = cameraname;
    getCameraList();
    readconfbyjson();
    initCamera(m_cameraname);
}
void CameraControlWidget::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    updateFrame(); // 窗口大小变化时重新缩放画面
}

void CameraControlWidget::closeEvent(QCloseEvent *event)
{
    if (!m_isApplied && m_cap && m_cap->isOpened()) {
        // 用户未点击 Apply，回滚到原始值
        m_cap->set(cv::CAP_PROP_BRIGHTNESS, m_originalParams.brightness);
        m_cap->set(cv::CAP_PROP_CONTRAST, m_originalParams.contrast);
        m_cap->set(cv::CAP_PROP_HUE, m_originalParams.hue);
        m_cap->set(cv::CAP_PROP_SATURATION, m_originalParams.saturation);
        m_cap->set(cv::CAP_PROP_SHARPNESS, m_originalParams.sharpness);
        m_cap->set(cv::CAP_PROP_GAMMA, m_originalParams.gamma);
        m_cap->set(cv::CAP_PROP_WB_TEMPERATURE, m_originalParams.whiteBalance);
        m_cap->set(cv::CAP_PROP_BACKLIGHT, m_originalParams.backlight);
        m_cap->set(cv::CAP_PROP_GAIN, m_originalParams.gain);
        m_cap->set(cv::CAP_PROP_EXPOSURE, m_originalParams.exposure);
        m_cap->set(cv::CAP_PROP_AUTO_WB, m_originalParams.autoWhiteBalance ? 1 : 0);
        m_cap->set(cv::CAP_PROP_AUTO_EXPOSURE, m_originalParams.autoExposure ? 0.75 : 0.25);
    }
    stopCamera(); // 再关闭摄像头
    // 继续关闭
    event->accept();
}

#include <QCameraInfo>
void CameraControlWidget::getCameraList(){
    m_cameramap.clear();
    int index=0;
    // 获取所有可用的摄像头
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        //QString deviceName = cameraInfo.deviceName();
#ifdef _WIN32
        QString deviceName = cameraInfo.description();
#else
        QString deviceName = cameraInfo.deviceName();
#endif
        m_cameramap[deviceName]=index;
        index++;
    }
}

void CameraControlWidget::initCamera(const QString& cameraname)
{
    int index=0;
    if(m_cameramap.contains(cameraname)>0){
        index=m_cameramap[cameraname];
        qDebug() << cameraname << index;
    }
#ifdef _WIN32
    m_cap = new cv::VideoCapture(index, cv::CAP_DSHOW);
#else
    m_cap = new cv::VideoCapture(index, cv::CAP_V4L2);
#endif

    if (!m_cap || !m_cap->isOpened()) {
        QMessageBox::critical(this, "错误", "无法打开摄像头！");
        ui->videoLabel->setText("❌ 摄像头未连接");
        return;
    }
    //默认分辨率是 640 x 480，这里设置分辨率，效果上是有问题的，注释掉
    //m_cap->set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    //m_cap->set(cv::CAP_PROP_FRAME_HEIGHT, 960);

    if (m_cap && m_cap->isOpened()) {
        int width  = static_cast<int>(m_cap->get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(m_cap->get(cv::CAP_PROP_FRAME_HEIGHT));
        double fps = m_cap->get(cv::CAP_PROP_FPS);

        qDebug() << "摄像头分辨率:" << width << "x" << height;
        qDebug() << "帧率 (FPS):" << fps;
    }
    // === 记录当前的硬件参数 ===
    m_originalParams.brightness      = static_cast<int>(m_cap->get(cv::CAP_PROP_BRIGHTNESS));
    m_originalParams.contrast        = static_cast<int>(m_cap->get(cv::CAP_PROP_CONTRAST));
    m_originalParams.hue             = static_cast<int>(m_cap->get(cv::CAP_PROP_HUE));
    m_originalParams.saturation      = static_cast<int>(m_cap->get(cv::CAP_PROP_SATURATION));
    m_originalParams.sharpness       = static_cast<int>(m_cap->get(cv::CAP_PROP_SHARPNESS));
    m_originalParams.gamma           = static_cast<int>(m_cap->get(cv::CAP_PROP_GAMMA));
    m_originalParams.whiteBalance    = static_cast<int>(m_cap->get(cv::CAP_PROP_WB_TEMPERATURE));
    m_originalParams.backlight       = static_cast<int>(m_cap->get(cv::CAP_PROP_BACKLIGHT));
    m_originalParams.gain            = static_cast<int>(m_cap->get(cv::CAP_PROP_GAIN));
    m_originalParams.exposure        = static_cast<int>(m_cap->get(cv::CAP_PROP_EXPOSURE));

    double autoWB = m_cap->get(cv::CAP_PROP_AUTO_WB);
    m_originalParams.autoWhiteBalance = (autoWB != 0);

    double autoExp = m_cap->get(cv::CAP_PROP_AUTO_EXPOSURE);
    m_originalParams.autoExposure = (autoExp == 0.75 || autoExp == 1.0);
    // ===================================

    // 初始化当前值
    initSliderRangeAndValue(cv::CAP_PROP_BRIGHTNESS, ui->brightnessSlider, ui->brightnessEdit);
    initSliderRangeAndValue(cv::CAP_PROP_CONTRAST, ui->contrastSlider, ui->contrastEdit);
    initSliderRangeAndValue(cv::CAP_PROP_HUE, ui->hueSlider, ui->hueEdit);
    initSliderRangeAndValue(cv::CAP_PROP_SATURATION, ui->saturationSlider, ui->saturationEdit);
    initSliderRangeAndValue(cv::CAP_PROP_SHARPNESS, ui->sharpnessSlider, ui->sharpnessEdit);
    initSliderRangeAndValue(cv::CAP_PROP_GAMMA, ui->gammaSlider, ui->gammaEdit);
    //第一版本无法操作白平衡
    //initSliderRangeAndValue(cv::CAP_PROP_WB_TEMPERATURE, ui->whiteBalanceSlider, ui->whiteBalanceEdit);
    ui->whiteBalanceEdit->setReadOnly(true);
    ui->autoWhiteBalance->setChecked(true);
    ui->autoWhiteBalance->setEnabled(false);
    m_originalParams.whiteBalance=0;
    initSliderRangeAndValue(cv::CAP_PROP_BACKLIGHT, ui->backlightCompensationSlider, ui->backlightCompensationEdit);
    initSliderRangeAndValue(cv::CAP_PROP_GAIN, ui->gainSlider, ui->gainEdit);
    initSliderRangeAndValue(cv::CAP_PROP_EXPOSURE, ui->exposureSlider, ui->exposureEdit);

    {//检测当前的硬件参数是不是与配置文件里面的一样
        if(m_conf_originalParamsmap.contains(m_cameraname)){//查找配置文件中有没有记录
            bool bdiff=false;
            st_originalParams stconf=m_conf_originalParamsmap[m_cameraname];
            //对比参数值
            if(stconf.brightness!=m_originalParams.brightness){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.contrast!=m_originalParams.contrast){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.hue!=m_originalParams.hue){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.saturation!=m_originalParams.saturation){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.sharpness!=m_originalParams.sharpness){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.gamma!=m_originalParams.gamma){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.whiteBalance!=m_originalParams.whiteBalance){ bdiff=true;qDebug()<<__LINE__<< m_originalParams.whiteBalance;}
            else if(stconf.backlight!=m_originalParams.backlight){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.gain!=m_originalParams.gain){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.exposure!=m_originalParams.exposure){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.autoWhiteBalance!=m_originalParams.autoWhiteBalance){ bdiff=true;qDebug()<<__LINE__;}
            else if(stconf.autoExposure!=m_originalParams.autoExposure){ bdiff=true;qDebug()<<__LINE__;}
            //发现有不一样的
            if(bdiff==true){
                ui->label_diffshow->setText("当前参数与配置文件不相同");
            }else ui->label_diffshow->setText("");
            qDebug() << "发现硬件配置信息";
        }else{//不存在记录时，按钮不能点击
            ui->confButton->setEnabled(false);
            ui->label_diffshow->setText("");
        }
    }

    // 自动模式状态
    //ui->autoWhiteBalance->setChecked(autoWB != 0);
    bool isAutoExp = (autoExp == 0.75 || autoExp == 1.0);
    ui->autoExposure->setChecked(isAutoExp);

    // 启用/禁用滑块
    ui->whiteBalanceSlider->setEnabled(!ui->autoWhiteBalance->isChecked());
    ui->whiteBalanceEdit->setEnabled(!ui->autoWhiteBalance->isChecked());
    ui->exposureSlider->setEnabled(!ui->autoExposure->isChecked());
    ui->exposureEdit->setEnabled(!ui->autoExposure->isChecked());

    connect(ui->brightnessSlider, &QSlider::valueChanged, this, &CameraControlWidget::onBrightnessChanged);
    connect(ui->contrastSlider, &QSlider::valueChanged, this, &CameraControlWidget::onContrastChanged);
    connect(ui->hueSlider, &QSlider::valueChanged, this, &CameraControlWidget::onHueChanged);
    connect(ui->saturationSlider, &QSlider::valueChanged, this, &CameraControlWidget::onSaturationChanged);
    connect(ui->sharpnessSlider, &QSlider::valueChanged, this, &CameraControlWidget::onSharpnessChanged);
    connect(ui->gammaSlider, &QSlider::valueChanged, this, &CameraControlWidget::onGammaChanged);
    connect(ui->whiteBalanceSlider, &QSlider::valueChanged, this, &CameraControlWidget::onWhiteBalanceChanged);
    connect(ui->backlightCompensationSlider, &QSlider::valueChanged, this, &CameraControlWidget::onBacklightChanged);
    connect(ui->gainSlider, &QSlider::valueChanged, this, &CameraControlWidget::onGainChanged);
    connect(ui->exposureSlider, &QSlider::valueChanged, this, &CameraControlWidget::onExposureChanged);

    // 自动复选框
    connect(ui->autoWhiteBalance, &QCheckBox::toggled, this, &CameraControlWidget::onAutoWhiteBalanceToggled);
    connect(ui->autoExposure, &QCheckBox::toggled, this, &CameraControlWidget::onAutoExposureToggled);

    // 按钮
    connect(ui->resetButton, &QPushButton::clicked, this, &CameraControlWidget::onResetToDefault);
    connect(ui->confButton, &QPushButton::clicked, this, &CameraControlWidget::onConfToDefault);
    connect(ui->applyButton, &QPushButton::clicked, this, &CameraControlWidget::onApplySettings);
    connect(ui->cancelButton, &QPushButton::clicked, this, &CameraControlWidget::onCancel);

    m_isApplied = false; // 初始未应用
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CameraControlWidget::updateFrame);
    m_timer->start(30);
}

void CameraControlWidget::stopCamera()
{
    // 停止定时器（防止继续读帧）
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }

    // 释放摄像头
    if (m_cap) {
        m_cap->release(); // 👈 关键！通知驱动释放硬件资源
        delete m_cap;     // 👈 释放堆内存
        m_cap = nullptr;
    }
}


void CameraControlWidget::readCurrentValue(int propId, QSlider* slider, QLineEdit* edit)
{
    if (!m_cap) return;
    double val = m_cap->get(propId);
    if (val == -1) return; // 不支持
    int iv = static_cast<int>(val);
    slider->setValue(iv);
    edit->setText(QString::number(iv));
}

//初始化滑块范围和当前值
void CameraControlWidget::initSliderRangeAndValue(int propId, QSlider* slider, QLineEdit* edit)
{
    if (!m_cap || !m_cap->isOpened()) {
        slider->setRange(0, 100);
        slider->setValue(50);
        edit->setText("50");
        return;
    }

    double current = m_cap->get(propId);
    if (current == -1) {
        // 属性不支持
        slider->setEnabled(false);
        edit->setText("N/A");
        return;
    }

    int minI, maxI;

    // === 使用超宽泛的探索范围（覆盖绝大多数摄像头）===
    switch (propId) {
        case cv::CAP_PROP_BRIGHTNESS:
            minI = -100; maxI = 300; break;
        case cv::CAP_PROP_CONTRAST:
            minI = 0; maxI = 300; break;
        case cv::CAP_PROP_HUE:
            minI = -2000; maxI = 2000; break;
        case cv::CAP_PROP_SATURATION:
            minI = 0; maxI = 300; break;
        case cv::CAP_PROP_SHARPNESS:
            minI = 0; maxI = 300; break;
        case cv::CAP_PROP_GAMMA:
            minI = 1; maxI = 1000; break;
        case cv::CAP_PROP_WB_TEMPERATURE:
            // 白平衡色温：常见 2800K ~ 10000K
            minI = 2500; maxI = 10000; break;
        case cv::CAP_PROP_BACKLIGHT:
            // 背光补偿：有些是 0/1，有些是 0~5
            minI = 0; maxI = 5; break;
        case cv::CAP_PROP_GAIN:
            minI = 0; maxI = 240; break;
        case cv::CAP_PROP_EXPOSURE:
            // 曝光最复杂：可能是负对数（-13~-1）或正绝对值（1~20000）
            minI = -20; maxI = 20000; break;
        default:
            minI = -1000; maxI = 1000; break;
    }

    // 如果当前值超出范围，自动扩展范围以包含它
    int curI = static_cast<int>(std::round(current));
    if (curI < minI) minI = curI - 10;
    if (curI > maxI) maxI = curI + 10;

    slider->setRange(minI, maxI);
    slider->setValue(curI);
    edit->setText(QString::number(curI));

    // 打印调试信息
    QString name;
    switch (propId) {
        case cv::CAP_PROP_BRIGHTNESS: name = "亮度"; break;
        case cv::CAP_PROP_CONTRAST: name = "对比度"; break;
        case cv::CAP_PROP_HUE: name = "色调"; break;
        case cv::CAP_PROP_SATURATION: name = "饱和度"; break;
        case cv::CAP_PROP_SHARPNESS: name = "锐度"; break;
        case cv::CAP_PROP_GAMMA: name = "伽玛"; break;
        case cv::CAP_PROP_WB_TEMPERATURE: name = "白平衡"; break;
        case cv::CAP_PROP_BACKLIGHT: name = "背光"; break;
        case cv::CAP_PROP_GAIN: name = "增益"; break;
        case cv::CAP_PROP_EXPOSURE: name = "曝光"; break;
        default: name = "未知";
    }
    qDebug().noquote() << QString("【%1】范围 [%2, %3] | 当前值 %4")
                          .arg(name).arg(minI).arg(maxI).arg(curI);
}

void CameraControlWidget::updateFrame()
{
    if (!m_cap || !m_cap->isOpened()) return;
    cv::Mat frame;
    *m_cap >> frame;
    if (frame.empty()) return;
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    ui->videoLabel->setPixmap(QPixmap::fromImage(img).scaled(
        ui->videoLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));
}

// ========== 槽函数 ==========
void CameraControlWidget::onBrightnessChanged(int value)
{
    ui->brightnessEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_BRIGHTNESS, value);
}

void CameraControlWidget::onContrastChanged(int value)
{
    ui->contrastEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_CONTRAST, value);
}

void CameraControlWidget::onHueChanged(int value)
{
    ui->hueEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_HUE, value);
}

void CameraControlWidget::onSaturationChanged(int value)
{
    ui->saturationEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_SATURATION, value);
}

void CameraControlWidget::onSharpnessChanged(int value)
{
    ui->sharpnessEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_SHARPNESS, value);
}

void CameraControlWidget::onGammaChanged(int value)
{
    ui->gammaEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_GAMMA, value);
}

void CameraControlWidget::onWhiteBalanceChanged(int value)
{
    ui->whiteBalanceEdit->setText(QString::number(value));
    if (m_cap) {
        m_cap->set(cv::CAP_PROP_AUTO_WB, 0);
        ui->autoWhiteBalance->setChecked(false);
        m_cap->set(cv::CAP_PROP_WB_TEMPERATURE, value);
    }
}

void CameraControlWidget::onBacklightChanged(int value)
{
    ui->backlightCompensationEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_BACKLIGHT, value);
}

void CameraControlWidget::onGainChanged(int value)
{
    ui->gainEdit->setText(QString::number(value));
    if (m_cap) m_cap->set(cv::CAP_PROP_GAIN, value);
}

void CameraControlWidget::onExposureChanged(int value)
{
    ui->exposureEdit->setText(QString::number(value));
    if (m_cap) {
        m_cap->set(cv::CAP_PROP_AUTO_EXPOSURE, 0.25);
        ui->autoExposure->setChecked(false);
        m_cap->set(cv::CAP_PROP_EXPOSURE, value);
    }
}

void CameraControlWidget::onAutoWhiteBalanceToggled(bool checked)
{
    if (!m_cap) return;
    m_cap->set(cv::CAP_PROP_AUTO_WB, checked ? 1 : 0);
    ui->whiteBalanceSlider->setEnabled(!checked);
    ui->whiteBalanceEdit->setEnabled(!checked);
    if (!checked) {
        double temp = m_cap->get(cv::CAP_PROP_WB_TEMPERATURE);
        if (temp > 0) {
            int t = static_cast<int>(temp);
            ui->whiteBalanceSlider->setValue(t);
            ui->whiteBalanceEdit->setText(QString::number(t));
        }
    }
}

void CameraControlWidget::onAutoExposureToggled(bool checked)
{
    if (!m_cap) return;
    m_cap->set(cv::CAP_PROP_AUTO_EXPOSURE, checked ? 0.75 : 0.25);
    ui->exposureSlider->setEnabled(!checked);
    ui->exposureEdit->setEnabled(!checked);
    if (!checked) {
        double exp = m_cap->get(cv::CAP_PROP_EXPOSURE);
        if (exp != -1) {
            int e = static_cast<int>(exp);
            ui->exposureSlider->setValue(e);
            ui->exposureEdit->setText(QString::number(e));
        }
    }
}

void CameraControlWidget::onResetToDefault()
{
    if (!m_cap) return;

    // 恢复原始值（不是“默认值”，而是启动时的值！）
    ui->brightnessSlider->setValue(m_originalParams.brightness);
    ui->contrastSlider->setValue(m_originalParams.contrast);
    ui->hueSlider->setValue(m_originalParams.hue);
    ui->saturationSlider->setValue(m_originalParams.saturation);
    ui->sharpnessSlider->setValue(m_originalParams.sharpness);
    ui->gammaSlider->setValue(m_originalParams.gamma);
    //ui->whiteBalanceSlider->setValue(m_originalParams.whiteBalance);
    ui->backlightCompensationSlider->setValue(m_originalParams.backlight);
    ui->gainSlider->setValue(m_originalParams.gain);
    ui->exposureSlider->setValue(m_originalParams.exposure);

    //ui->autoWhiteBalance->setChecked(m_originalParams.autoWhiteBalance);
    ui->autoExposure->setChecked(m_originalParams.autoExposure);

    // 触发设置
    onBrightnessChanged(m_originalParams.brightness);
    onContrastChanged(m_originalParams.contrast);
    onHueChanged(m_originalParams.hue);
    onSaturationChanged(m_originalParams.saturation);
    onSharpnessChanged(m_originalParams.sharpness);
    onGammaChanged(m_originalParams.gamma);
    //onWhiteBalanceChanged(m_originalParams.whiteBalance);
    onBacklightChanged(m_originalParams.backlight);
    onGainChanged(m_originalParams.gain);
    onExposureChanged(m_originalParams.exposure);

    // 自动模式状态也要恢复
    //m_cap->set(cv::CAP_PROP_AUTO_WB, m_originalParams.autoWhiteBalance ? 1 : 0);
    m_cap->set(cv::CAP_PROP_AUTO_EXPOSURE, m_originalParams.autoExposure ? 0.75 : 0.25);

    m_isApplied = false; // 重置后视为未应用
}

void CameraControlWidget::onConfToDefault()
{
    if (!m_cap) return;
    st_originalParams stconf=m_conf_originalParamsmap[m_cameraname];
    // 恢复原始值（不是“默认值”，而是启动时的值！）
    ui->brightnessSlider->setValue(stconf.brightness);
    ui->contrastSlider->setValue(stconf.contrast);
    ui->hueSlider->setValue(stconf.hue);
    ui->saturationSlider->setValue(stconf.saturation);
    ui->sharpnessSlider->setValue(stconf.sharpness);
    ui->gammaSlider->setValue(stconf.gamma);
    //ui->whiteBalanceSlider->setValue(stconf.whiteBalance);
    ui->backlightCompensationSlider->setValue(stconf.backlight);
    ui->gainSlider->setValue(stconf.gain);
    ui->exposureSlider->setValue(stconf.exposure);
    //ui->autoWhiteBalance->setChecked(stconf.autoWhiteBalance);
    ui->autoExposure->setChecked(stconf.autoExposure);

    // 触发设置
    onBrightnessChanged(stconf.brightness);
    onContrastChanged(stconf.contrast);
    onHueChanged(stconf.hue);
    onSaturationChanged(stconf.saturation);
    onSharpnessChanged(stconf.sharpness);
    onGammaChanged(stconf.gamma);
    //onWhiteBalanceChanged(stconf.whiteBalance);
    onBacklightChanged(m_originalParams.backlight);
    onGainChanged(stconf.gain);
    onExposureChanged(stconf.exposure);

    // 自动模式状态也要恢复
    //m_cap->set(cv::CAP_PROP_AUTO_WB, stconf.autoWhiteBalance ? 1 : 0);
    m_cap->set(cv::CAP_PROP_AUTO_EXPOSURE, stconf.autoExposure ? 0.75 : 0.25);

    m_isApplied = false; // 重置后视为未应用
}

void CameraControlWidget::onApplySettings()
{
    m_isApplied = true;
    st_originalParams cur_originalParams;
    cur_originalParams.brightness=ui->brightnessEdit->text().toInt();
    cur_originalParams.contrast=ui->contrastEdit->text().toInt();
    cur_originalParams.hue=ui->hueEdit->text().toInt();
    cur_originalParams.saturation=ui->saturationEdit->text().toInt();
    cur_originalParams.sharpness=ui->sharpnessEdit->text().toInt();
    cur_originalParams.gamma=ui->gammaEdit->text().toInt();
    cur_originalParams.whiteBalance=ui->whiteBalanceEdit->text().toInt();
    cur_originalParams.backlight=ui->backlightCompensationEdit->text().toInt();
    cur_originalParams.gain=ui->gainEdit->text().toInt();
    cur_originalParams.exposure=ui->exposureEdit->text().toInt();
    cur_originalParams.autoWhiteBalance=ui->autoWhiteBalance->isChecked();
    cur_originalParams.autoExposure=ui->autoExposure->isChecked();
    insertjsondata(cur_originalParams);
    ui->confButton->setEnabled(true);
    QMessageBox::information(this, "提示", "保存配置成功。");
}

void CameraControlWidget::onCancel()
{
    if (!m_isApplied && m_cap && m_cap->isOpened()) {
        // 用户未点击 Apply，回滚到原始值
        m_cap->set(cv::CAP_PROP_BRIGHTNESS, m_originalParams.brightness);
        m_cap->set(cv::CAP_PROP_CONTRAST, m_originalParams.contrast);
        m_cap->set(cv::CAP_PROP_HUE, m_originalParams.hue);
        m_cap->set(cv::CAP_PROP_SATURATION, m_originalParams.saturation);
        m_cap->set(cv::CAP_PROP_SHARPNESS, m_originalParams.sharpness);
        m_cap->set(cv::CAP_PROP_GAMMA, m_originalParams.gamma);
        m_cap->set(cv::CAP_PROP_WB_TEMPERATURE, m_originalParams.whiteBalance);
        m_cap->set(cv::CAP_PROP_BACKLIGHT, m_originalParams.backlight);
        m_cap->set(cv::CAP_PROP_GAIN, m_originalParams.gain);
        m_cap->set(cv::CAP_PROP_EXPOSURE, m_originalParams.exposure);
        m_cap->set(cv::CAP_PROP_AUTO_WB, m_originalParams.autoWhiteBalance ? 1 : 0);
        m_cap->set(cv::CAP_PROP_AUTO_EXPOSURE, m_originalParams.autoExposure ? 0.75 : 0.25);
    }
    stopCamera(); // 再关闭摄像头
    close();
}


bool CameraControlWidget::readconfbyjson(){
    m_conf_originalParamsmap.clear();
    // 检查文件是否存在
    QFile file(CAMERACONTROLFILE);
    if (!file.exists()) {
        // 文件不存在，创建文件
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
        }
    }else{//解析Json文件
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open the file";
            return false;
        }
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("data")) {
            QJsonArray dataArray = jsonObj["data"].toArray();
            //qDebug()<< __FUNCTION__<< __LINE__ << dataArray  ;
            foreach (const QJsonValue &value, dataArray) {
                QJsonObject obj = value.toObject();
                st_originalParams confstr;
                QString name = obj["name"].toString();
                confstr.brightness = obj["brightness"].toInt();
                confstr.contrast = obj["contrast"].toInt();
                confstr.hue = obj["hue"].toInt();
                confstr.saturation = obj["saturation"].toInt();
                confstr.sharpness = obj["sharpness"].toInt();
                confstr.gamma = obj["gamma"].toInt();
                confstr.whiteBalance = obj["whiteBalance"].toInt();
                confstr.backlight = obj["backlight"].toInt();
                confstr.gain = obj["gain"].toInt();
                confstr.exposure = obj["exposure"].toInt();
                confstr.autoWhiteBalance = obj["autoWhiteBalance"].toBool();
                confstr.autoExposure = obj["autoExposure"].toBool();
                m_conf_originalParamsmap[name]=confstr;
            }
        }
        file.close();
    }
    return true;
}

bool CameraControlWidget::insertjsondata(const st_originalParams& originalParams){
    // 读取JSON文件内容
    QFile rfile(CAMERACONTROLFILE);
    if (!rfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << rfile.errorString();
        return false;
    }
    QByteArray jsonData = rfile.readAll();
    rfile.close();
    if(jsonData.length()==0){
        // 创建 JSON 数据
        QJsonObject jsonObject;
        QJsonArray dataArray;
        // 插入一条记录
        QJsonObject newObject;
        newObject["name"] = m_cameraname;
        newObject["brightness"] = originalParams.brightness;
        newObject["contrast"] = originalParams.contrast;
        newObject["hue"] = originalParams.hue;
        newObject["saturation"] = originalParams.saturation;
        newObject["sharpness"] = originalParams.sharpness;
        newObject["gamma"] = originalParams.gamma;
        newObject["whiteBalance"] = originalParams.whiteBalance;
        newObject["backlight"] = originalParams.backlight;
        newObject["gain"] = originalParams.gain;
        newObject["exposure"] = originalParams.exposure;
        newObject["autoWhiteBalance"] = originalParams.autoWhiteBalance;
        newObject["autoExposure"] = originalParams.autoExposure;
        dataArray.append(newObject);
        jsonObject["data"] = dataArray;
        // 将 JSON 数据转换为字符串
        QJsonDocument jsonDocument(jsonObject);
        QByteArray insertData = jsonDocument.toJson();

        // 写入到文件
        QFile wfile(CAMERACONTROLFILE);
        if (!wfile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open file for writing." << wfile.errorString();
            return false;
        }
        wfile.write(insertData);
        wfile.close();
    }else{
        // 解析JSON数据
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "Failed to parse JSON:" << parseError.errorString();
            return false;
        }

        // 获取JSON数组
        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray dataArray = jsonObj["data"].toArray();
        //qDebug()<< __FUNCTION__<< __LINE__ << jsonArray ;
        // 在数组中查找并更新指定的数据
        bool bfind=false;
        QString name = m_cameraname;
        for (int i = 0; i < dataArray.size(); ++i) {
            QJsonObject item = dataArray[i].toObject();
            if (item["name"].toString() == name) {
                bfind = true;//此数据已存在,对数据进行更改
                item["brightness"] = originalParams.brightness;
                item["contrast"] = originalParams.contrast;
                item["hue"] = originalParams.hue;
                item["saturation"] = originalParams.saturation;
                item["sharpness"] = originalParams.sharpness;
                item["gamma"] = originalParams.gamma;
                item["whiteBalance"] = originalParams.whiteBalance;
                item["backlight"] = originalParams.backlight;
                item["gain"] = originalParams.gain;
                item["exposure"] = originalParams.exposure;
                item["autoWhiteBalance"] = originalParams.autoWhiteBalance;
                item["autoExposure"] = originalParams.autoExposure;
                dataArray[i] = item; // 更新数组中的数据
                qDebug()<< __LINE__ << "更新数据：" << m_cameraname;
                break;
            }
        }

        if(bfind==false){
            QJsonObject newObject;
            newObject["name"] = m_cameraname;
            newObject["brightness"] = originalParams.brightness;
            newObject["contrast"] = originalParams.contrast;
            newObject["hue"] = originalParams.hue;
            newObject["saturation"] = originalParams.saturation;
            newObject["sharpness"] = originalParams.sharpness;
            newObject["gamma"] = originalParams.gamma;
            newObject["whiteBalance"] = originalParams.whiteBalance;
            newObject["backlight"] = originalParams.backlight;
            newObject["gain"] = originalParams.gain;
            newObject["exposure"] = originalParams.exposure;
            newObject["autoWhiteBalance"] = originalParams.autoWhiteBalance;
            newObject["autoExposure"] = originalParams.autoExposure;
            qDebug()<< __LINE__ << "新增数据：" << m_cameraname;
            // 将新对象插入到数组中
            dataArray.append(newObject);
        }
        // 更新JSON对象中的"data"数组
        jsonObj["data"] = dataArray;
        // 将更新后的JSON写回文件
        jsonDoc = QJsonDocument(jsonObj);
        QFile wfile(CAMERACONTROLFILE);
        if (!wfile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qDebug() << "Failed to open file for writing:" << wfile.errorString();
            return false;
        }
        wfile.write(jsonDoc.toJson());
        wfile.close();
    }
    qDebug() << __LINE__ <<"JSON data written to file successfully.";

    return true;
}
