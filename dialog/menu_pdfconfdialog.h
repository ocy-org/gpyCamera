#ifndef MENU_PDFCONFDIALOG_H
#define MENU_PDFCONFDIALOG_H

#include <QDialog>
#include <QIntValidator>
#include <QMessageBox>
#include <QDebug>
#include "util/comm_define.h"
#include "util/commfuntion.h"
namespace Ui {
class menu_pdfconfDialog;
}

class menu_pdfconfDialog : public QDialog
{
    Q_OBJECT

public:
    explicit menu_pdfconfDialog(QWidget *parent = nullptr);
    ~menu_pdfconfDialog();

private slots:
    void on_pushButton_save_clicked();
signals:
    void refreshpdfdata(const int& num);
    void refreshmergeimagedata(const int& num);
private:
    Ui::menu_pdfconfDialog *ui;
    QIntValidator *intValidator;
    commfuntion m_comm;
    QString m_pdfnum;
    QString m_mergeimagenum;
    void getconfmsg();
    void saveconfmsg(const QString& flag,const QString& value);
};

#endif // MENU_PDFCONFDIALOG_H
