#ifndef SELECTCAMERA_DIALOG_H
#define SELECTCAMERA_DIALOG_H

#include <QDialog>

namespace Ui {
class selectcamera_Dialog;
}

class selectcamera_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit selectcamera_Dialog(QWidget *parent = nullptr);
    ~selectcamera_Dialog();
    void init();

private slots:
    void on_pushButton_save_clicked();

    void on_pushButton_exit_clicked();

signals:
    void sendcameraindex(const int& cameraindex);

private:
    Ui::selectcamera_Dialog *ui;
    void getCameraList();
    QString m_cameraname;
    QList<QString> m_cameralist;
};

#endif // SELECTCAMERA_DIALOG_H
