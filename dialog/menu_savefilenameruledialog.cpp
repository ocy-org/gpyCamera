#include "menu_savefilenameruledialog.h"
#include "ui_menu_savefilenameruledialog.h"

menu_savefilenameruledialog::menu_savefilenameruledialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::menu_savefilenameruledialog)
{
    ui->setupUi(this);
    getconfnamerulemsg();
    setconnet();
    initUI();
}

menu_savefilenameruledialog::~menu_savefilenameruledialog()
{
    delete ui;
}

void menu_savefilenameruledialog::getconfnamerulemsg()
{
    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    settings.beginGroup("NAMERULE");
    m_oneimg_format=settings.value("oneimg_format","JPG").toString();
    m_oneimg_name_type=settings.value("oneimg_name_type","1").toString();
    m_oneimg_pre_type=settings.value("oneimg_pre_type","1").toString();
    m_oneimg_pre_name=settings.value("oneimg_pre_name","IMG_").toString();

    m_moreimg_format=settings.value("moreimg_format","JPG").toString();
    m_moreimg_pre_type=settings.value("moreimg_pre_type","1").toString();
    m_moreimg_pre_name=settings.value("moreimg_pre_name","IMG_").toString();

    m_onepdf_pdfname_type=settings.value("onepdf_pdfname_type","1").toString();
    m_onepdf_pre_type=settings.value("onepdf_pre_type","1").toString();
    m_onepdf_pre_name=settings.value("onepdf_pre_name","IMG_").toString();

    m_morepdf_pre_type=settings.value("morepdf_pre_type","1").toString();
    m_morepdf_pre_name=settings.value("morepdf_pre_name","IMG_").toString();
}

void menu_savefilenameruledialog::initUI(){
    ui->comboxOneImageFormat->setCurrentText(m_oneimg_format);
    if(m_oneimg_name_type=="1"){
        ui->radioSingleImgPattern->setChecked(true);
        if(m_oneimg_pre_type=="0"){
            ui->radioSinglePrefixAsk->setChecked(true);
            ui->radioSinglePrefixNow->setChecked(false);
        }else{
            ui->radioSinglePrefixNow->setChecked(true);
            ui->radioSinglePrefixAsk->setChecked(false);
            ui->lineEditPrefix_singleimg->setText(m_oneimg_pre_name);
        }
    }else{
        ui->radioSingleImgName->setChecked(true);
    }


    ui->comboImageFormat->setCurrentText(m_moreimg_format);
    if(m_moreimg_pre_type=="0"){
        ui->radioPrefixAsk->setChecked(true);
        ui->radioPrefixNow->setChecked(false);
    }else{
        ui->radioPrefixNow->setChecked(true);
        ui->radioPrefixAsk->setChecked(false);
        ui->lineEditPrefix_moreimg->setText(m_moreimg_pre_name);
    }

    if(m_onepdf_pdfname_type=="1"){
        ui->radioOnePDFPattern->setChecked(true);
        if(m_onepdf_pdfname_type=="0"){
            ui->radioOnePrefixAsk->setChecked(true);
            ui->radioOnePrefixNow->setChecked(false);
        }else{
            ui->radioOnePrefixNow->setChecked(true);
            ui->radioOnePrefixAsk->setChecked(false);
            ui->lineEditPrefix_onepdf->setText(m_onepdf_pre_name);
        }
    }else{
        ui->radioOnePDFName->setChecked(true);
    }

    if(m_morepdf_pre_type=="0"){
        ui->radioMultiPrefixAsk->setChecked(true);
        ui->radioMultiPrefixNow->setChecked(false);
    }else{
        ui->radioMultiPrefixNow->setChecked(true);
        ui->radioMultiPrefixAsk->setChecked(false);
        ui->lineEditPrefix_morepdf->setText(m_morepdf_pre_name);
    }
}

void menu_savefilenameruledialog::setconnet(){
    //生成单张图片
    connect(ui->radioMultiPrefixNow, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->lineEditPrefix_morepdf->setStyleSheet("");
        }else{
            ui->lineEditPrefix_morepdf->setStyleSheet("color: transparent; background: transparent; border: none;");
        }
    });
    connect(ui->radioMultiPrefixAsk, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->lineEditPrefix_morepdf->setStyleSheet("");
        }else{
            ui->lineEditPrefix_morepdf->setStyleSheet("color: transparent; background: transparent; border: none;");
        }
    });
    //生成多张图片
    connect(ui->radioPrefixNow, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->lineEditPrefix_moreimg->setStyleSheet("");
        }else{
            ui->lineEditPrefix_moreimg->setStyleSheet("color: transparent; background: transparent; border: none;");
        }
    });
    connect(ui->radioOnePrefixNow, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->lineEditPrefix_onepdf->setStyleSheet("");
        }else{
            ui->lineEditPrefix_onepdf->setStyleSheet("color: transparent; background: transparent; border: none;");
        }
    });
    connect(ui->radioMultiPrefixNow, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->lineEditPrefix_morepdf->setStyleSheet("");
        }else{
            ui->lineEditPrefix_morepdf->setStyleSheet("color: transparent; background: transparent; border: none;");
        }
    });


    connect(ui->radioOnePDFName, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->label_10->hide();
            ui->radioOnePrefixNow->hide();
            ui->lineEditPrefix_onepdf->hide();
            ui->radioOnePrefixAsk->hide();
        }else{
            ui->label_10->show();
            ui->radioOnePrefixNow->show();
            ui->lineEditPrefix_onepdf->show();
            ui->radioOnePrefixAsk->show();
        }
    });

    connect(ui->radioSingleImgName, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->label_0_prefixSource->hide();
            ui->radioSinglePrefixNow->hide();
            ui->lineEditPrefix_singleimg->hide();
            ui->radioSinglePrefixAsk->hide();
        }else{
            ui->label_0_prefixSource->show();
            ui->radioSinglePrefixNow->show();
            ui->lineEditPrefix_singleimg->show();
            ui->radioSinglePrefixAsk->show();
        }
    });
}

void menu_savefilenameruledialog::on_savebutton_clicked()
{
    QString oneimg_format = ui->comboxOneImageFormat->currentText();
    QString oneimg_name_type="0";
    QString oneimg_pre_type="0";
    QString oneimg_pre_name="IMG_";
    if(ui->radioSingleImgPattern->isChecked()){
        oneimg_name_type="1";
        if(ui->radioSinglePrefixNow->isChecked()){
            qDebug()<< "选择现在定义" << __LINE__;
            oneimg_pre_type="1";
            oneimg_pre_name = ui->lineEditPrefix_singleimg->text();
            if(oneimg_pre_name==""){
                QMessageBox::information(this, "提示", "【前缀来源】不能为空");
                return;
            }
        }
    }

    QString moreimg_format = ui->comboImageFormat->currentText();
    QString moreimg_pre_type="0";
    QString moreimg_pre_name="IMG_";
    if(ui->radioPrefixNow->isChecked()){
        qDebug()<< "选择现在定义" << __LINE__;
        moreimg_pre_type="1";
        moreimg_pre_name = ui->lineEditPrefix_moreimg->text();
        if(moreimg_pre_name==""){
            QMessageBox::information(this, "提示", "【前缀来源】不能为空");
            return;
        }else{
            qDebug() << __LINE__ << moreimg_pre_name;
        }
    }

    QString onepdf_pdfname_type="0";
    QString onepdf_pre_type="0";
    QString onepdf_pre_name="IMG_";
    if(ui->radioOnePDFPattern->isChecked()){
        onepdf_pdfname_type="1";
        if(ui->radioOnePrefixNow->isChecked()){
            qDebug()<< "选择现在定义" << __LINE__;
            onepdf_pre_type="1";
            onepdf_pre_name = ui->lineEditPrefix_onepdf->text();
            if(onepdf_pre_name==""){
                QMessageBox::information(this, "提示", "【前缀来源】不能为空");
                return;
            }
        }
    }

    QString morepdf_pre_type="0";
    QString morepdf_pre_name="IMG_";
    if(ui->radioMultiPrefixNow->isChecked()){
        qDebug()<< "选择现在定义" << __LINE__;
        morepdf_pre_type="1";
        morepdf_pre_name = ui->lineEditPrefix_morepdf->text();
        if(morepdf_pre_name==""){
            QMessageBox::information(this, "提示", "【前缀来源】不能为空");
            return;
        }
    }

    //保存到配置文件
    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    settings.beginGroup("NAMERULE");
    // 保存中文字符
    settings.setValue("oneimg_format", oneimg_format);
    settings.setValue("oneimg_name_type", oneimg_name_type);
    settings.setValue("oneimg_pre_type", oneimg_pre_type);
    settings.setValue("oneimg_pre_name", oneimg_pre_name);

    settings.setValue("moreimg_format", moreimg_format);
    settings.setValue("moreimg_pre_type", moreimg_pre_type);
    settings.setValue("moreimg_pre_name", moreimg_pre_name);

    settings.setValue("onepdf_pdfname_type", onepdf_pdfname_type);
    settings.setValue("onepdf_pre_type", onepdf_pre_type);
    settings.setValue("onepdf_pre_name", onepdf_pre_name);

    settings.setValue("morepdf_pre_type", morepdf_pre_type);
    settings.setValue("morepdf_pre_name", morepdf_pre_name);
    settings.sync();
    QMessageBox::information(this, "提示", "保存成功。");

    emit refreshnamerule();
    close();
}

void menu_savefilenameruledialog::on_Cancelbutton_clicked()
{
    close();
}
