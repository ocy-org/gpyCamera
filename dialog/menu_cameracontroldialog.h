#ifndef MENU_CAMERACONTROLDIALOG_H
#define MENU_CAMERACONTROLDIALOG_H

#include <QDialog>
#include <QMap>
#include "dialog/cameracontrolwidget.h"
namespace Ui {
class menu_cameracontrolDialog;
}

class menu_cameracontrolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit menu_cameracontrolDialog(QWidget *parent = nullptr);
    ~menu_cameracontrolDialog();
    void init();

private slots:
    void on_pushButton_save_clicked();

    void on_pushButton_exit_clicked();

private:
    Ui::menu_cameracontrolDialog *ui;
    void getCameraList();
    QString m_cameraname;
    QList<QString> m_cameralist;
    CameraControlWidget *m_controldoalog;
};

#endif // MENU_CAMERACONTROLDIALOG_H
