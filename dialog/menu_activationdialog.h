#ifndef MENU_ACTIVATIONDIALOG_H
#define MENU_ACTIVATIONDIALOG_H
#include <QDialog>
#include <QTimer>
#include <QNetworkInterface>
#include <QPainter>
#include "util/comm_define.h"
#include "util/qrcodegen.hpp"
#include "util/qrcodescan.h"
#include "dialog/selectcamera_dialog.h"

// 前向声明UI类
namespace Ui {
class ActivationDialog;
}

class ActivationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActivationDialog(QWidget *parent = nullptr);
    ~ActivationDialog();

    // 设置产品码（CPUID）
    void setProductCode(const QString &code);
    // 获取当前产品码（不含空格）
    QString getProductCode();
    bool isActivation();


protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_generateButton_clicked();
    void on_copyButton_clicked();

    void on_activationCodeEdit_textChanged(const QString &text);
    void formatActivationCode();

    void on_codegenpushButton_clicked();

    void on_qrscan_pushButton_clicked();

signals:
    void refreshactivation();

private:
    Ui::ActivationDialog *ui;
    selectcamera_Dialog *m_selectcamera;
    void setupConnections();
    void initializeProductCode();

    // 字符串格式化函数
    QString formatStringWithSpaces(const QString &str);//每4个字符加空格
    QString removeSpaces(const QString &formattedStr);
    QString getSystemCpuId();
    QString getMacAddress();
    void getconfmsg();
    void saveconfmsg();
    //检测激活码是否正确
    bool checkactivation(const QString& productCode,const QString& src_activationcode);
    void checkOKUI();

    QString m_productcode="";
    QString m_activationconf="";

    // 定时器用于延迟格式化
    QTimer *m_productFormatTimer;
    QTimer *m_activationFormatTimer;

    // 防止格式化过程中递归调用
    bool m_formattingProductCode;
    bool m_formattingActivationCode;

    QImage generateQRCode(const QString &text, int size = 300);
    void showQRCodeDialog(const QImage &qrImage, const QString &text);
};

#endif // MENU_ACTIVATIONDIALOG_H
