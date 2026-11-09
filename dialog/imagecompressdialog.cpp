#include "imagecompressdialog.h"
#include "ui_imagecompressdialog.h"
#include <QRegExp>
#include <QRegExpValidator>
#include <QDebug>
#include <QBuffer>
#include <QMessageBox>
// 辅助函数：格式化文件大小
QString formatFileSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes < KB) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < MB) {
        double kb = static_cast<double>(bytes) / KB;
        return QString("%1 KB").arg(kb, 0, 'f', 2);
    } else if (bytes < GB) {
        double mb = static_cast<double>(bytes) / MB;
        return QString("%1 MB").arg(mb, 0, 'f', 2);
    } else {
        double gb = static_cast<double>(bytes) / GB;
        return QString("%1 GB").arg(gb, 0, 'f', 2);
    }
}

ImageCompressDialog::ImageCompressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageCompressDialog)
{
    ui->setupUi(this);
    //设置只能输入1~99
    QRegExp regExp("^([1-9][0-9]?|)$");
    QRegExpValidator *validator = new QRegExpValidator(regExp, ui->lineEdit_comp);
    ui->lineEdit_comp->setValidator(validator);
}

ImageCompressDialog::~ImageCompressDialog()
{
    delete ui;
}

#include <iostream>
void ImageCompressDialog::init(const QString& num,const QPixmap& srcimg){
    m_srcimg=srcimg;
    int quality=50;
    if(num!="") quality=num.toInt();
    QString srcImageSize=getImageSizeAsjpgBypercent(m_srcimg);
    qDebug() << srcImageSize;
    ui->lineEdit_comp->setText(QString::number(quality));
    ui->srcimgsize->setText(srcImageSize);
    ui->compimgsize->setText("");
}


//QString ImageCompressDialog::getImageSizeAsJPG(const QPixmap &pixmap, int quality)
//{
//    if (pixmap.isNull()) {
//        return "0 B";
//    }

//    // 获取系统临时目录
//    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
//    if (tempDir.isEmpty()) {
//        tempDir = QDir::tempPath();
//    }

//    // 创建唯一的临时文件名
//    QString tempFileName = QString("%1/temp_jpg_%2.jpg")
//        .arg(tempDir)
//        .arg(QDateTime::currentMSecsSinceEpoch());
//    qDebug()<< tempFileName;
//    // 保存为JPG格式
//    if (!pixmap.save(tempFileName, "JPG", quality)) {
//        qWarning() << "无法保存图片到临时文件:" << tempFileName;
//        return "0 B";
//    }

//    // 获取文件真实大小
//    QFileInfo fileInfo(tempFileName);
//    qint64 fileSize = fileInfo.size();

//    // 立即删除临时文件
//    //QFile::remove(tempFileName);

//    // 格式化输出
//    return formatFileSize(fileSize);
//}

//QString ImageCompressDialog::getSrcImageSizeAsJPG(const QPixmap &pixmap)
//{
//    if (pixmap.isNull()) {
//        return "0 B";
//    }

//    // 1. 计算缩放后的目标尺寸
//    // 目标：将图片体积压缩到 100KB 以内。
//    // 经验值：对于普通场景照片，将长宽各缩小到原来的 50%（即总像素缩小到 25%），
//    // 再配合 75% 的 JPG 质量，通常能轻松达到 1/5 甚至更小的体积压缩率。
//    int targetWidth = pixmap.width() * 0.5;  // 缩放比例可根据实际效果微调，如 0.4 或 0.6
//    int targetHeight = pixmap.height() * 0.5;

//    // 使用 SmoothTransformation 保证缩小后的图片依然清晰，没有锯齿
//    QPixmap scaledPixmap = pixmap.scaled(targetWidth, targetHeight,
//                                         Qt::KeepAspectRatio,
//                                         Qt::SmoothTransformation);

//    // 2. 获取系统临时目录并创建唯一的临时文件名
//    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
//    if (tempDir.isEmpty()) {
//        tempDir = QDir::tempPath();
//    }

//    QString tempFileName = QString("%1/temp_jpg_%2.jpg")
//        .arg(tempDir)
//        .arg(QDateTime::currentMSecsSinceEpoch());

//    qDebug() << "临时文件路径:" << tempFileName;

//    // 3. 保存缩放后的图片为 JPG 格式（默认质量 75%）
//    if (!scaledPixmap.save(tempFileName/*, "JPG", 75*/)) {
//        qWarning() << "无法保存图片到临时文件:" << tempFileName;
//        return "0 B";
//    }

//    // 4. 获取文件真实大小
//    QFileInfo fileInfo(tempFileName);
//    qint64 fileSize = fileInfo.size();
//    qDebug() << "压缩后文件大小:" << fileSize << "Bytes";

//    // 5. 立即删除临时文件（根据你的业务需求决定是否保留此行）
//    // QFile::remove(tempFileName);

//    // 6. 格式化输出
//    return formatFileSize(fileSize);
//}

QString ImageCompressDialog::getImageSizeAsjpgBypercent(const QPixmap &pixmap, int percent,const QString& filepath)
{
    if (pixmap.isNull()) {
        return "0 B";
    }

    // 1. 计算缩放后的目标尺寸
    // 目标：将图片体积压缩到 100KB 以内。
    // 经验值：对于普通场景照片，将长宽各缩小到原来的 50%（即总像素缩小到 25%），
    // 再配合 75% 的 JPG 质量，通常能轻松达到 1/5 甚至更小的体积压缩率。
    int targetWidth = pixmap.width() * percent/100;  // 缩放比例可根据实际效果微调，如 0.4 或 0.6
    int targetHeight = pixmap.height() * percent/100;

    // 使用 SmoothTransformation 保证缩小后的图片依然清晰，没有锯齿
    QPixmap scaledPixmap = pixmap.scaled(targetWidth, targetHeight,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    QString savefilepath=filepath;
    if(savefilepath==""){
        // 2. 获取系统临时目录并创建唯一的临时文件名
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (tempDir.isEmpty()) {
            tempDir = QDir::tempPath();
        }

        QString tempFileName = QString("%1/temp_%2.jpg")
            .arg(tempDir)
            .arg(QDateTime::currentMSecsSinceEpoch());

        qDebug() << "临时文件路径:" << tempFileName;
        savefilepath=tempFileName;
    }

    // 3. 保存缩放后的图片为 JPG 格式（默认质量 75%）
    if (!scaledPixmap.save(savefilepath/*, "JPG", 75*/)) {
        qWarning() << "无法保存图片到文件:" << savefilepath;
        return "0 B";
    }

    // 4. 获取文件真实大小
    QFileInfo fileInfo(savefilepath);
    qint64 fileSize = fileInfo.size();
    //qDebug() << "压缩后文件大小:" << fileSize << "Bytes";

    // 5. 立即删除临时文件（根据你的业务需求决定是否保留此行）
    QFile::remove(savefilepath);

    // 6. 格式化输出
    return formatFileSize(fileSize);
}

void ImageCompressDialog::on_pushButton_comp_clicked()
{
    QString num=ui->lineEdit_comp->text();
    if(num==""){
        QMessageBox::information(this, "提示", "百分比不能为空");
        return;
    }
    int quality=num.toInt();
    QString compImageSize=getImageSizeAsjpgBypercent(m_srcimg,quality);
    qDebug() << compImageSize;
    ui->compimgsize->setText(compImageSize);
}




void ImageCompressDialog::on_pushButton_save_clicked()
{
    QString num=ui->lineEdit_comp->text();
    if(num==""){
        QMessageBox::information(this, "提示", "百分比不能为空");
        return;
    }
    int percent=num.toInt();
    emit refreshpercent(percent);
    close();
}

void ImageCompressDialog::on_pushButton_exit_clicked()
{
    close();
}
