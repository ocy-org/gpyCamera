#ifndef MAINCONFDIALOG_H
#define MAINCONFDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QDebug>
#include "util/comm_define.h"
#include "util/commfuntion.h"

namespace Ui {
class mainconfDialog;
}

class mainconfDialog : public QDialog
{
    Q_OBJECT

public:
    explicit mainconfDialog(QWidget *parent = nullptr);
    ~mainconfDialog();
    void init(const QList<QString>& CameraList,const int& mainindex,const int& subindex);
signals:
    void refreshtabledata();
private slots:
    void on_pushButton_save_clicked();
    void on_pushButton_exit_clicked();

private:
    Ui::mainconfDialog *ui;
    commfuntion *m_comm;
    QMap<QString, stCameraMsg> m_cameramap;
};

#endif // MAINCONFDIALOG_H
