#ifndef MENU_FILESAVEDIALOG_H
#define MENU_FILESAVEDIALOG_H

#include <QDialog>
#include "util/comm_define.h"
#include "util/commfuntion.h"
namespace Ui {
class menu_filesaveDialog;
}

class menu_filesaveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit menu_filesaveDialog(QWidget *parent = nullptr);
    ~menu_filesaveDialog();
    QString getfilesavepath();

private slots:
    void on_getdirButton_clicked();

    void on_pushButton_exit_clicked();
    void on_pushButton_save_clicked();

signals:
    void refreshfilesavepath(const QString& filesavepath);
private:
    Ui::menu_filesaveDialog *ui;
    void getconfmsg();
    void saveconfmsg(const QString& flag,const QString& value);
    QString m_filesavepath;
};

#endif // MENU_FILESAVEDIALOG_H
