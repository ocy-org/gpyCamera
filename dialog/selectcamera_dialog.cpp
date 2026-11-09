#include "selectcamera_dialog.h"
#include "ui_selectcamera_dialog.h"

selectcamera_Dialog::selectcamera_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::selectcamera_Dialog)
{
    ui->setupUi(this);
    getCameraList();
}

selectcamera_Dialog::~selectcamera_Dialog()
{
    delete ui;
}

void selectcamera_Dialog::init(){
    m_cameralist.clear();
    ui->cameraComboBox->clear();
    getCameraList();
    for (const QString &cameraname : m_cameralist){
        ui->cameraComboBox->addItem(cameraname);
    }
}

#include <QCameraInfo>
void selectcamera_Dialog::getCameraList(){
    // 获取所有可用的摄像头
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        //QString deviceName = cameraInfo.deviceName();
#ifdef _WIN32
        QString deviceName = cameraInfo.description();
#else
        QString deviceName = cameraInfo.deviceName();
#endif
        m_cameralist.append(deviceName);
    }
}

void selectcamera_Dialog::on_pushButton_save_clicked()
{
    int cameraindex=ui->cameraComboBox->currentIndex();
    hide();
    emit sendcameraindex(cameraindex);
    close();
}

void selectcamera_Dialog::on_pushButton_exit_clicked()
{
    close();
}
