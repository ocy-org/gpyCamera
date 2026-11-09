#include "mainconfdialog.h"
#include "ui_mainconfdialog.h"

mainconfDialog::mainconfDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mainconfDialog)
{
    ui->setupUi(this);
    //20260108黄总说不需要名称，先隐藏不可见
    ui->lineEdit_tname->hide();
    ui->label_name->hide();
}

mainconfDialog::~mainconfDialog()
{
    delete ui;
}

void mainconfDialog::init(const QList<QString>& CameraList,const int& mainindex,const int& subindex){
    qDebug() << __FUNCTION__ << mainindex << subindex;
    ui->maincomboBox->clear();
    ui->subcomboBox->clear();
    ui->lineEdit_tname->clear();
    for (const QString &cameraname : CameraList){
        ui->maincomboBox->addItem(cameraname);
    }
    for (const QString &cameraname : CameraList){
        ui->subcomboBox->addItem(cameraname);
    }
    //设置默认值
    if(mainindex>0 && mainindex<CameraList.count())ui->maincomboBox->setCurrentIndex(mainindex);
    if(subindex>0 && subindex<CameraList.count())ui->subcomboBox->setCurrentIndex(subindex);

    m_comm->getconfbyjson(m_cameramap);
}

void mainconfDialog::on_pushButton_save_clicked()
{
    QString mainCamera=ui->maincomboBox->currentText();
    QString subCamera=ui->subcomboBox->currentText();
    QString name="";//ui->lineEdit_tname->text();
    if(/*name==""||*/mainCamera==""||subCamera==""){
        QMessageBox::warning(this, "错误", "内容不能为空");
        return;
    }
    if(mainCamera==subCamera){
        QMessageBox::warning(this, "错误", "主、副摄像头名称不能相同");
        return;
    }
//    if(m_cameramap.count(name)>0){
//        QString showstr=QString("名称[%1]已存在，不能重名").arg(name);
//        QMessageBox::warning(this, "错误", showstr);
//        return;
//    }

    stCameraMsg cameramsg;
    cameramsg.name = name;
    cameramsg.maincamera = mainCamera;
    cameramsg.subcamera = subCamera;
    m_comm->insertjsondata(cameramsg);
    emit refreshtabledata();
    accept();
}

void mainconfDialog::on_pushButton_exit_clicked()
{
    close();
}


