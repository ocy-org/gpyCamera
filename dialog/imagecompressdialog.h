#ifndef IMAGECOMPRESSDIALOG_H
#define IMAGECOMPRESSDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QString>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
namespace Ui {
class ImageCompressDialog;
}

class ImageCompressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageCompressDialog(QWidget *parent = nullptr);
    ~ImageCompressDialog();
    void init(const QString& num,const QPixmap& srcimg);
    QString getImageSizeAsjpgBypercent(const QPixmap &pixmap, int percent=100,const QString& filepath="");

private slots:
    void on_pushButton_comp_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_exit_clicked();

private:
    Ui::ImageCompressDialog *ui;
    QPixmap m_srcimg;
    //QString getImageSizeAsJPG(const QPixmap &pixmap, int quality);
    //QString getSrcImageSizeAsJPG(const QPixmap &pixmap);

signals:
    void refreshpercent(const int& percent);

};

#endif // IMAGECOMPRESSDIALOG_H
