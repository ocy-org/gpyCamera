#ifndef MENU_SAVEFILENAMERULEDIALOG_H
#define MENU_SAVEFILENAMERULEDIALOG_H
#include <QDebug>
#include <QDialog>
#include <QSettings>
#include <QMessageBox>
#include "util/comm_define.h"
namespace Ui {
class menu_savefilenameruledialog;
}

class menu_savefilenameruledialog : public QDialog
{
    Q_OBJECT

public:
    explicit menu_savefilenameruledialog(QWidget *parent = nullptr);
    ~menu_savefilenameruledialog();
    void getconfnamerulemsg();

private slots:
    void on_savebutton_clicked();

    void on_Cancelbutton_clicked();
signals:
    void refreshnamerule();
private:
    Ui::menu_savefilenameruledialog *ui;

    void initUI();
    void setconnet();
    QString m_oneimg_format="JPG";
    QString m_oneimg_name_type="0";
    QString m_oneimg_pre_type="0";
    QString m_oneimg_pre_name="";

    QString m_moreimg_format="JPG";
    QString m_moreimg_pre_type="0";
    QString m_moreimg_pre_name="";

    QString m_onepdf_pdfname_type="0";
    QString m_onepdf_pre_type="0";
    QString m_onepdf_pre_name="";

    QString m_morepdf_pre_type="0";
    QString m_morepdf_pre_name="";
};

#endif // MENU_SAVEFILENAMERULEDIALOG_H
