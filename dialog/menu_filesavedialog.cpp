#include "menu_filesavedialog.h"
#include "ui_menu_filesavedialog.h"
#include <QStandardPaths>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
menu_filesaveDialog::menu_filesaveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::menu_filesaveDialog)
{
    ui->setupUi(this);
    ui->savepdfEdit->setReadOnly(true);//设置为只读
    getconfmsg();
}

menu_filesaveDialog::~menu_filesaveDialog()
{
    delete ui;
}

void menu_filesaveDialog::getconfmsg(){
    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    //获取保存拍照图片的路径
    QString docpath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);//默认是“我的文档”目录
    //获取文件保存路径
    QString filepath = settings.value("filepath","").toString();
    QString adddirstatus = settings.value("adddir","0").toString();
    if(filepath==""){
        //默认在桌面
        ui->savepdfEdit->setText(docpath);
        m_filesavepath = docpath;
    }else{
        ui->savepdfEdit->setText(filepath);
        m_filesavepath = filepath;
    }
    if(adddirstatus=="1")
        ui->adddirCBox->setChecked(true);
}

void menu_filesaveDialog::saveconfmsg(const QString& flag,const QString& value){
    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    settings.setValue(flag, value);
}

void menu_filesaveDialog::on_getdirButton_clicked()
{
    QString docpath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);//默认是“我的文档”目录
    qDebug() << __FUNCTION__ << docpath;
    // 弹出目录选择对话框
    QString selectedDir = QFileDialog::getExistingDirectory(
        nullptr, // 父窗口
        tr("选择目录"), // 对话框标题
        docpath, // 默认路径（系统文档目录）
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    // 检查用户是否选择了目录（点击了取消按钮会返回空字符串）
    if (!selectedDir.isEmpty()) {
        qDebug() << "选择的目录:" << selectedDir;
        ui->savepdfEdit->setText(selectedDir);
    } else {
        qDebug() << "用户取消了选择";
    }
}

void menu_filesaveDialog::on_pushButton_exit_clicked()
{
    accept();
}

QString menu_filesaveDialog::getfilesavepath(){
    return m_filesavepath;
}

void menu_filesaveDialog::on_pushButton_save_clicked()
{
    QString selectedDir=ui->savepdfEdit->toPlainText();
//    if(selectedDir==""){
//        return;
//    }
    QString adddirstatus="0";
    //查看是不是选中状态
    if(ui->adddirCBox->isChecked()){
        adddirstatus = "1";
    }
    saveconfmsg("filepath",selectedDir);
    saveconfmsg("adddir",adddirstatus);
    emit refreshfilesavepath(selectedDir);
    close();
}
