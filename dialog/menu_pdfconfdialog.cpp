#include "menu_pdfconfdialog.h"
#include "ui_menu_pdfconfdialog.h"

menu_pdfconfDialog::menu_pdfconfDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::menu_pdfconfDialog)
{
    ui->setupUi(this);
    intValidator = new QIntValidator(this);
    // 设置范围（可选）
    intValidator->setRange(0, 9);  // 只能输入0-100
    ui->lineEdit_num->setValidator(intValidator);
    getconfmsg();
}

menu_pdfconfDialog::~menu_pdfconfDialog()
{
    delete ui;
    delete intValidator;
}

void menu_pdfconfDialog::getconfmsg(){
    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    //获取PDF保存路径
    m_pdfnum = settings.value("pdf_image_num","2").toString();
    ui->lineEdit_num->setText(m_pdfnum);
    m_mergeimagenum = settings.value("image_merge_num","2").toString();
    ui->lineEdit_mergeingnum->setText(m_mergeimagenum);
}

void menu_pdfconfDialog::saveconfmsg(const QString& flag,const QString& value){
    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    settings.setValue(flag, value);
}

void menu_pdfconfDialog::on_pushButton_save_clicked()
{
    QString pdfnum=ui->lineEdit_num->text();
    if(pdfnum==""){
        QMessageBox::warning(this, "错误", "内容不能为空！");
        return;
    }

    QString mergeimagenum=ui->lineEdit_mergeingnum->text();
    if(mergeimagenum==""){
        QMessageBox::warning(this, "错误", "内容不能为空！");
        return;
    }

    if(m_comm.isInteger(pdfnum)==false || m_comm.isInteger(mergeimagenum)==false){
        QMessageBox::warning(this, "错误", "内容必须为整数");
        return;
    }

    int num=pdfnum.toInt();
    if(num!=m_pdfnum.toInt()){
        saveconfmsg("pdf_image_num",QString::number(num));
        emit refreshpdfdata(num);
    }
    int mergenum=mergeimagenum.toInt();
    if(mergenum!=m_mergeimagenum.toInt()){
        saveconfmsg("image_merge_num",QString::number(mergenum));
        emit refreshmergeimagedata(mergenum);
    }

    accept();
}
