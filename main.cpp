#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 获取命令行参数列表
    QStringList args = QCoreApplication::arguments();
    QString param1 = (args.size() > 1) ? args.at(1) : "";
    bool flag=false;
    if(param1=="-FromZeroCode")flag=true;
    qDebug() << args.size() << param1 << flag;
    // 注册自定义类型，以便在信号槽中使用
    qRegisterMetaType<CameraCapability>("CameraCapability");
    qRegisterMetaType<QList<CameraInfo>>("QList<CameraInfo>");
    MainWindow w(flag);
    w.show();
    return a.exec();
}
