#include "menu_activationdialog.h"
#include "ui_menu_activationdialog.h"

#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include <QDateTime>
#include "util/commfuntion.h"
#include "util/permit.h"

ActivationDialog::ActivationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActivationDialog),
    m_formattingProductCode(false),
    m_formattingActivationCode(false)
{
    ui->setupUi(this);

    // 设置窗口属性
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //setFixedSize(500, 300);

    // 创建格式化定时器
    m_productFormatTimer = new QTimer(this);
    m_productFormatTimer->setSingleShot(true);
    m_productFormatTimer->setInterval(300);

    m_activationFormatTimer = new QTimer(this);
    m_activationFormatTimer->setSingleShot(true);
    m_activationFormatTimer->setInterval(300);

    setupConnections();
    initializeProductCode();//获取产品码
    //如果是成功激活状态
    if(isActivation()==true){
        checkOKUI();
    }else{
        ui->showlabel->setText("未激活，请输入激活码进行注册");
    }

     m_selectcamera=new selectcamera_Dialog(this);
     connect(m_selectcamera, &selectcamera_Dialog::sendcameraindex,this, [=](const int& cameraindex){
         //更新摄像头名称列表框
         qDebug() << "获取到摄像头ID:" << cameraindex;
         QRCodeScanDialog dialog(this);
         dialog.initcameraindex(cameraindex);
         // 显示对话框并等待结果
         if (dialog.exec() == QDialog::Accepted) {
             QString qrContent = dialog.getQRContent();
             if (!qrContent.isEmpty()) {
                 ui->activationCodeEdit->setText(qrContent);
             }
         }
     });
}

ActivationDialog::~ActivationDialog()
{
    delete ui;
    delete m_selectcamera;
}

void ActivationDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // 产品码是只读的，不需要设置焦点
}

void ActivationDialog::setupConnections()
{
    // 连接定时器信号
    connect(m_activationFormatTimer, &QTimer::timeout, this, &ActivationDialog::formatActivationCode);
}

void ActivationDialog::initializeProductCode()
{
    // 获取系统CPUID并设置到产品码输入框
    m_productcode = getMacAddress();
    //qDebug() << __FUNCTION__ << "m_productcode:" << m_productcode;
    setProductCode(m_productcode);
}

void ActivationDialog::setProductCode(const QString &code)
{
    if (code.isEmpty()) {
        ui->productCodeEdit->setText("无法获取系统CPUID");
        return;
    }
    // 格式化显示（每4个字符加-）,这是页面给用户看的产品码
    QString formattedCode = formatStringWithSpaces(code);
    //qDebug() << __FUNCTION__ << "formattedCode:" << formattedCode;
    ui->productCodeEdit->setText(formattedCode);
}

QString ActivationDialog::getProductCode()
{
    return removeSpaces(ui->productCodeEdit->text());
}

QString ActivationDialog::getSystemCpuId()
{
    commfuntion comm;
    QString cpuId = comm.getCpuId();
    return cpuId;
}

QString ActivationDialog::getMacAddress(){
    QString macAddress = "0000-0000-0000";
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces) {
        // 跳过回环接口
        if (interface.flags() & QNetworkInterface::IsLoopBack) {
            continue;
        }

        // 跳过无效的MAC地址
        QString hwAddr = interface.hardwareAddress();
        if (hwAddr.isEmpty() || hwAddr == "00:00:00:00:00:00") {
            continue;
        }

        // 通过接口名称过滤虚拟接口（兼容老版本Qt）
        QString interfaceName = interface.name().toLower();
        if (interfaceName.contains("virtual") ||
            interfaceName.contains("vmware") ||
            interfaceName.contains("vbox") ||
            interfaceName.contains("tunnel") ||
            interfaceName.contains("loopback") ||
            interfaceName.startsWith("veth") ||
            interfaceName.startsWith("docker") ||
            interfaceName.startsWith("br-")) {
            continue;
        }

        macAddress = hwAddr.remove(':');
        break;
    }

    return macAddress;
}

void ActivationDialog::on_generateButton_clicked()
{
    QString activationCodeEdit = removeSpaces(ui->activationCodeEdit->text());
    if (activationCodeEdit.isEmpty()) {
        QMessageBox::warning(this, "错误", "激活码\n不能为空！");
        return;
    }
    QString productCode = removeSpaces(ui->productCodeEdit->text());
    if (productCode.isEmpty()) {
        QMessageBox::warning(this, "错误", "无法获取\n有效产品码！");
        return;
    }

    // 禁用生成按钮，防止重复点击
    ui->generateButton->setEnabled(false);

    QTimer::singleShot(800, this, [this, productCode,activationCodeEdit]() {
        bool ret=false;
        try {
            ret=checkactivation(productCode,activationCodeEdit);
            if(ret==false)
                QMessageBox::warning(this, "失败", "激活码错误\n注册失败！");
            else{
                saveconfmsg();
                QMessageBox::information(this, "成功", "注册成功！");
                checkOKUI();
                emit refreshactivation();
            }
        } catch (const std::exception &e) {
            QMessageBox::critical(this, "错误", QString("生成激活码\n时发生错误：%1").arg(e.what()));
        }
        if(ret==false)// 重新启用生成按钮
            ui->generateButton->setEnabled(true);
    });

    // 显示处理中的提示
    //QMessageBox::information(this, "处理中", "正在生成激活码，请稍候...");
}

void ActivationDialog::on_copyButton_clicked()
{
    QString productCode = ui->productCodeEdit->text();

    if (productCode.isEmpty()) {
        QMessageBox::warning(this, "提示", "产品码为空，请连续运维工程师！");
        return;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(productCode);
    //qDebug() << __FUNCTION__ << productCode;
    QMessageBox::information(this, "成功", "复制成功");
}

void ActivationDialog::on_activationCodeEdit_textChanged(const QString &text)
{
    if (m_formattingActivationCode) {
        return;
    }
    m_activationFormatTimer->start();
}

void ActivationDialog::formatActivationCode()
{
    m_formattingActivationCode = true;

    QString currentText = ui->activationCodeEdit->text();
    QString cleanText = removeSpaces(currentText);
    QString formattedText = formatStringWithSpaces(cleanText);

    if (currentText != formattedText) {
        ui->activationCodeEdit->setText(formattedText);
    }

    m_formattingActivationCode = false;
}

QString ActivationDialog::formatStringWithSpaces(const QString &str)
{
    QString formatted;
    QString cleanStr = removeSpaces(str);

    for (int i = 0; i < cleanStr.length(); ++i) {
        if (i > 0 && i % 4 == 0) {
            formatted += '-';
        }
        formatted += cleanStr.at(i);
    }

    return formatted;
}

QString ActivationDialog::removeSpaces(const QString &formattedStr)
{
    QString result = formattedStr;
    return result.remove('-');
}


void ActivationDialog::getconfmsg(){
    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    //获取激活码
    m_activationconf = settings.value("activation","").toString();
}

void ActivationDialog::saveconfmsg(){

    QSettings settings(CONFFILE, QSettings::IniFormat);//打开文件
    QString activationCode = ui->activationCodeEdit->text();
    settings.setValue("activation", activationCode);
}

//对原始的加密结果进行比较（不加入-，-是为了让用户看起来方便）
bool ActivationDialog::checkactivation(const QString& productCode,const QString& src_activationcode){
    PerMit permit;
    QString activationCode=permit.encrypt(productCode);
    QString des_productCode=permit.decrypt(src_activationcode);//解密进行比较
    if(activationCode!=des_productCode)
        return false;
    return true;
}

bool ActivationDialog::isActivation(){
    getconfmsg();
    if(m_activationconf=="") return false;
    m_productcode = getMacAddress();
    QString activationCodeEdit = removeSpaces(m_activationconf);//去掉-
    if (activationCodeEdit.isEmpty()) {
        return false;
    }
    QString productCode = removeSpaces(m_productcode);
    if (productCode.isEmpty()) {
        return false;
    }
    bool ret=checkactivation(productCode,activationCodeEdit);
    return ret;
}

void ActivationDialog::checkOKUI(){
    ui->activationCodeEdit->setText("********************************");
    ui->activationCodeEdit->setEnabled(false);
    ui->generateButton->setEnabled(false);
    ui->showlabel->setText("已激活");
}

void ActivationDialog::on_codegenpushButton_clicked()
{
    QString productCode = ui->productCodeEdit->text();
    QImage qrImage = generateQRCode(productCode, 300);
    if (qrImage.isNull()) {
        QMessageBox::critical(this, "生成失败", "无法生成二维码，请检查输入内容。");
        return;
    }

    // 弹出二维码对话框
    showQRCodeDialog(qrImage, productCode);
}


QImage ActivationDialog::generateQRCode(const QString &text, int size)
{
    std::string utf8Text = text.toUtf8().toStdString();

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
        utf8Text.c_str(),
        qrcodegen::QrCode::Ecc::MEDIUM  // 使用 MEDIUM 纠错率，平衡容错与密度
    );

    QImage image(size, size, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    int border = 4;
    int qrSize = qr.getSize();
    double scale = static_cast<double>(size) / (qrSize + 2 * border);
    int offset = static_cast<int>(border * scale);

    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            if (qr.getModule(x, y)) {
                QRectF rect(
                    offset + x * scale,
                    offset + y * scale,
                    scale, scale
                );
                painter.drawRect(rect);
            }
        }
    }

    return image;
}

void ActivationDialog::showQRCodeDialog(const QImage &qrImage, const QString &text)
{
    QDialog dialog(this);
    dialog.setWindowTitle("二维码预览");
    dialog.setModal(true);

    // 创建 QLabel 显示二维码
    QLabel *qrLabel = new QLabel(&dialog);
    qrLabel->setPixmap(QPixmap::fromImage(qrImage));
    qrLabel->setAlignment(Qt::AlignCenter);
    qrLabel->setStyleSheet("QLabel { background-color: white; border: 1px solid #ccc; }");

    // 可选：显示原始文本（小字）
    QLabel *textLabel = new QLabel(&dialog);
    textLabel->setText(QString("内容: %1").arg(text.left(50) + (text.size() > 50 ? "..." : "")));
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    textLabel->setStyleSheet("font-size: 10pt; color: gray; margin-top: 8px;");

    QPushButton *closeBtn = new QPushButton("关闭", &dialog);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(qrLabel);
    layout->addWidget(textLabel);
    layout->addWidget(closeBtn);
    layout->setAlignment(Qt::AlignCenter);

    dialog.setLayout(layout);

    // 设置对话框大小（略大于二维码）
    dialog.resize(qrImage.width() + 40, qrImage.height() + 100);

    // 居中显示（相对于主窗口或屏幕）
    dialog.exec();  // 模态对话框
}


void ActivationDialog::on_qrscan_pushButton_clicked()
{
    qDebug() << __FUNCTION__;
    m_selectcamera->setModal(false);
    m_selectcamera->setWindowTitle("选择摄像头");
    m_selectcamera->open();
    m_selectcamera->init();
    m_selectcamera->exec();
    return;
}

