#ifndef MENU_MAININFODIALOG_H
#define MENU_MAININFODIALOG_H

#include <QDialog>

#include <QHBoxLayout>
#include <QMap>
#include <QDebug>

#include "mainconfdialog.h"
#include "util/comm_define.h"
#include "util/commfuntion.h"
namespace Ui {
class menu_maininfoDialog;
}

class menu_maininfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit menu_maininfoDialog(QWidget *parent = nullptr);
    ~menu_maininfoDialog();
    void init(const QList<QString>& CameraList);
signals:
    void refreshcombox();

private slots:
    void on_pushbutton_add_clicked();
    void on_pushButton_exit_clicked();
    void deldata();

private:
    Ui::menu_maininfoDialog *ui;
    mainconfDialog *m_confdialog;
    QMap<QString, stCameraMsg> m_cameramap;
    QList<QString> m_CameraList;
    void initUI();
    void ShowTableContent();
    void renderTable(const QStringList &rowData);
    commfuntion *m_comm;

};

#endif // MENU_MAININFODIALOG_H
