#include "menu_cameracontroldialog.h"
#include "ui_menu_cameracontroldialog.h"

menu_cameracontrolDialog::menu_cameracontrolDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::menu_cameracontrolDialog)
{
    ui->setupUi(this);
    m_controldoalog = new CameraControlWidget(this);
}

menu_cameracontrolDialog::~menu_cameracontrolDialog()
{
    delete ui;
    delete m_controldoalog;
}

void menu_cameracontrolDialog::init(){
    m_cameralist.clear();
    ui->cameraComboBox->clear();
    getCameraList();
    for (const QString &cameraname : m_cameralist){
        ui->cameraComboBox->addItem(cameraname);
    }
}

#include <QCameraInfo>
void menu_cameracontrolDialog::getCameraList(){
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

void menu_cameracontrolDialog::on_pushButton_save_clicked()
{
    hide();
    QString curCamera=ui->cameraComboBox->currentText();
    m_controldoalog->setModal(false);
    m_controldoalog->setWindowTitle("摄像头硬件参数设置");
    m_controldoalog->init(curCamera);
    m_controldoalog->open();
    m_controldoalog->exec();
    close();
}

void menu_cameracontrolDialog::on_pushButton_exit_clicked()
{
    close();
}
