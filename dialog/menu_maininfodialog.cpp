#include "menu_maininfodialog.h"
#include "ui_menu_maininfodialog.h"
#include <iostream>
menu_maininfoDialog::menu_maininfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::menu_maininfoDialog)
{
    ui->setupUi(this);
    initUI();
    m_confdialog = new mainconfDialog(this);
    ShowTableContent();
    //connect(m_confdialog, &mainconfDialog::refreshtabledata,this, &menu_maininfoDialog::addcamerainfo);
    connect(m_confdialog, &mainconfDialog::refreshtabledata,this, [=](){
        qDebug() << "mainifo接收到配置信息变化的信号";
        ShowTableContent();
        emit refreshcombox();
    });
}

menu_maininfoDialog::~menu_maininfoDialog()
{
    delete ui;
}

void menu_maininfoDialog::initUI(){
    QStringList  tablelHeader;
    tablelHeader<<"序号"<< "名称" <<"主摄像头"<<"副摄像头"<<"操作";
    ui->tableWidget->setColumnCount(tablelHeader.size()); // 设置列数
    ui->tableWidget->setHorizontalHeaderLabels(tablelHeader);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);// 设置选择行为整行选择
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//单元格不可编辑
    ui->tableWidget->setStyleSheet("QTableView::item:selected { color:#000000;background-color: #ADD8E6; }");//设置选择后的颜色
    //设置表格表头
    for (int i = 0; i < tablelHeader.size(); ++i) {
        QTableWidgetItem* headerItem = new QTableWidgetItem(tablelHeader.at(i));
        QFont font = headerItem->font();
        // font.setBold(true);  // 设置字体加粗
        headerItem->setFont(font);
        ui->tableWidget->setHorizontalHeaderItem(i, headerItem);
        if(tablelHeader[i]=="序号" || tablelHeader[i]=="操作")
        {
            ui->tableWidget->horizontalHeader()->setSectionResizeMode(i,QHeaderView::Fixed);
            int width = 40;//默认宽度
            if(tablelHeader[i]=="操作")  width=60;
            ui->tableWidget->setColumnWidth(i,width);
        }
    }
    QString headerStyleSheet = "QHeaderView::section { background-color: rgb(242,242,242); color: #333;padding-top:3px;padding-bottom:3px; }";
    ui->tableWidget->horizontalHeader()->setStyleSheet(headerStyleSheet);
    ui->tableWidget->setWordWrap(false);
}

void menu_maininfoDialog::init(const QList<QString>& CameraList){
    m_CameraList=CameraList;
}

void menu_maininfoDialog::ShowTableContent(){
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);//清空表格
    m_cameramap.clear();
    m_comm->getconfbyjson(m_cameramap);
    qDebug() << __FUNCTION__ << m_cameramap.size();
    int index = 1;
    // 使用迭代器遍历map
    for (auto it = m_cameramap.begin(); it != m_cameramap.end(); ++it,index++) {
        stCameraMsg cameramsg=it.value();
        QStringList rowData;
        rowData << QString::number(index) << cameramsg.name << cameramsg.maincamera << cameramsg.subcamera <<"";
        renderTable(rowData);
    }
}

void menu_maininfoDialog::renderTable(const QStringList &rowData){
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    for (int column = 0; column < rowData.size(); ++column) {
        QTableWidgetItem* item = new QTableWidgetItem(rowData.at(column));
        item->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(row, column, item);

        QTableWidgetItem* headerItem = ui->tableWidget->horizontalHeaderItem(column);
        QString headerText = headerItem ? headerItem->text() : QString();
        if (headerText == "操作") {
            //删除按钮
            QPushButton* deleteButton = new QPushButton();
            deleteButton->setIcon(QIcon(":/index/img/delete.png"));
            deleteButton->setIconSize(QSize(24, 24));
            deleteButton->setStyleSheet("QPushButton { border: none; }");
            deleteButton->setCursor(Qt::PointingHandCursor);
            //按钮信号监听
            connect(deleteButton,SIGNAL(clicked()),this,SLOT(deldata()));

            QWidget* widget = new QWidget();
            QHBoxLayout* layout = new QHBoxLayout(widget);
            layout->addWidget(deleteButton);
            layout->setAlignment(Qt::AlignCenter);
            layout->setContentsMargins(0, 0, 0, 0);
            widget->setLayout(layout);
            ui->tableWidget->setCellWidget(row, column, widget);
        }
    }
}

void menu_maininfoDialog::deldata(){
    QPushButton *senderObj = qobject_cast<QPushButton*>(sender());
    if(senderObj == nullptr){
        qDebug() << "senderObj null";
        return;
    }
    QModelIndex delIndex = ui->tableWidget->indexAt(QPoint(senderObj->parentWidget()->frameGeometry().x(),senderObj->parentWidget()->frameGeometry().y()));
    int row = delIndex.row();
    QString name = ui->tableWidget->item(row, 1)->text();
    qDebug() << __FUNCTION__ << name;
    m_comm->deljsondata(name);
    ShowTableContent();
    emit refreshcombox();
}

void menu_maininfoDialog::on_pushbutton_add_clicked()
{
    m_confdialog->setModal(false);
    m_confdialog->setWindowTitle("新增");
    m_confdialog->init(m_CameraList,0,1);
    m_confdialog->open();
    m_confdialog->exec();
}

void menu_maininfoDialog::on_pushButton_exit_clicked()
{
    close();
}
