#include "imagelistwidget.h"
#include <QListWidgetItem>
#include <QPixmap>
#include <QFileInfo>
#include <QMessageBox>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPainter>
#include <QMouseEvent>
#include "util/customitemdelegate.h"
ImageListWidget::ImageListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_gridSize(120, 120)//初始化网格大小为150x150像素
    , m_iconSize(100, 100)//初始化图标大小为120x120像素
{
    initializeUI();
}
// UI初始化函数
void ImageListWidget::initializeUI()
{
    setViewMode(QListWidget::IconMode);// 设置视图模式为图标模式（显示图标而非列表）
    setGridSize(m_gridSize);// 设置网格大小
    setIconSize(m_iconSize);// 设置图标大小
    setResizeMode(QListWidget::Adjust); // 设置调整模式为自动调整
    setMovement(QListWidget::Snap);// 设置移动模式为对齐网格
    //setSelectionMode(QListWidget::SingleSelection);// 设置选择模式为单选
    setSelectionMode(QListWidget::ExtendedSelection);// 设置选择模式为多选---v1.2.3版本调整为支持多选项
    setDragDropMode(QListWidget::InternalMove);// 设置拖放模式为内部移动
    setDefaultDropAction(Qt::MoveAction);// 设置默认拖放动作为移动
    setSpacing(0);// 设置项目间距为0
    setDropIndicatorShown(true);// 显示拖放指示器
    setUniformItemSizes(true);// 设置统一的项目大小
    //setWordWrap(true);// 启用文本自动换行

    // 👇 关键：设置自定义委托
    //setItemDelegate(new CustomItemDelegate(this));
    //setEditTriggers(QAbstractItemView::DoubleClicked);//允许双击编辑文本
}

// 鼠标双击事件处理函数--双击弹出预览图片
void ImageListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {//检测双击事件
        QListWidgetItem *item = itemAt(event->pos());// 获取点击位置的项目
        if (item) {
            QString filePath = item->data(Qt::UserRole).toString();//获取文件路径
            qDebug() << "双击预览图片:" << filePath;
            emit imageDoubleClicked(filePath);// 发送信号--由主框架统一处理
            event->accept();// 吸收事件
            return;
        }
    }
    //其他的事件调用父类的默认处理
    QListWidget::mouseDoubleClickEvent(event);
}

//鼠标右键单击--菜单功能
void ImageListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {//测右键单击事件
        emit requestContextMenu(event->pos());// 发送信号--由主框架统一处理
        event->accept();// 吸收事件
        return;
    }
    //其他的事件调用父类的默认处理
    QListWidget::mousePressEvent(event);
}

// 删除选中图片的函数
void ImageListWidget::deleteSelectedImage()
{
//    {//单个选项的处理方法
//        QListWidgetItem *currentItem = this->currentItem();// 获取当前选中的项目
//        if (!currentItem) {
//            return;
//        }
//        int row = this->row(currentItem);// 获取项目所在行
//        delete takeItem(row); // 删除项目
//    }
    {//多个选项的处理方法---v1.2.3版本调整为支持多选项
        QList<QListWidgetItem*> selectedItems = this->selectedItems();
        if (selectedItems.isEmpty()) {
            return;
        }
        // 倒序遍历删除（防止索引错乱）
        // 👇 倒序删除非常重要，如果正序删除，删除第一项后，后面的索引都会改变，导致漏删
        for (int i = selectedItems.size() - 1; i >= 0; --i) {
            QListWidgetItem *item = selectedItems.at(i);
            int row = this->row(item);
            delete takeItem(row);
        }
    }

    emit imageCountChanged(count());// 发射图片数量变化信号--如果主框架有数量统计的需求，可以同步更新
    emit imageOrderChanged();// 发射图片顺序变化信号

    //qDebug() << "删除图片，当前剩余数量:" << count();
}

// 添加单个图片--主框架调用的函数
void ImageListWidget::addPixmap(const QPixmap &pixmap,const QString &mainid)
{
    // 缩放图片到图标大小，保持宽高比
    QPixmap scaledPixmap = pixmap.scaled(m_iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int num=imageCount()+1;//序号累加
    QString imageNo = QString("%1").arg(num, 3, 10, QChar('0'));//填充为：001
    // 创建列表项目，显示图标和文件名
    QListWidgetItem *item = new QListWidgetItem(QIcon(scaledPixmap),imageNo);
    item->setData(Qt::UserRole, mainid);// 记录主框架的ID，方便主框架获取到修改的信息
    item->setSizeHint(m_gridSize);// 设置项目大小提示
    // 设置项目标志Qt::ItemIsEnabled:启用;Qt::ItemIsSelectable:可选;Qt::ItemIsDragEnabled:可拖拽
    //item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
    addItem(item);// 添加项目到列表
    emit imageCountChanged(count());// 发射图片数量变化信号
}

// 添加单个图片
void ImageListWidget::addImage(const QString &filePath)
{
    QPixmap pixmap(filePath);// 从文件路径加载图片
    if (pixmap.isNull()) return;

    // 缩放图片到图标大小，保持宽高比
    QPixmap scaledPixmap = pixmap.scaled(m_iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // 创建列表项目，显示图标和文件名
    //int num=imageCount()+1;//序号累加
    m_number++;
    QString imageNo = QString("%1").arg(m_number, 3, 10, QChar('0'));//填充为：001
    qDebug() << __FUNCTION__ << imageNo;
    QListWidgetItem *item = new QListWidgetItem(QIcon(scaledPixmap), imageNo);
    item->setData(Qt::UserRole, filePath);// 注意这里存储完整文件路径，主框架自己去取
    item->setSizeHint(m_gridSize);// 设置项目大小提示
    // 设置项目标志Qt::ItemIsEnabled:启用;Qt::ItemIsSelectable:可选;Qt::ItemIsDragEnabled:可拖拽
    //item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
    addItem(item);// 添加项目到列表
    emit imageCountChanged(count());// 发射图片数量变化信号
}

// 添加多个图片的函数---外部拖拽调用
void ImageListWidget::addImages(const QStringList &filePaths)
{
    for (const QString &filePath : filePaths) {
        addImage(filePath);
    }
}

// 清空所有图片的函数
void ImageListWidget::clearAllImages()
{
    clear();
    emit imageCountChanged(0);
}

// 获取图片数量的函数
int ImageListWidget::imageCount() const
{
    return count();
}

// 获取所有图片路径的函数
QStringList ImageListWidget::getImageKeys() const
{
    QStringList keys;
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *item = this->item(i);
        keys.append(item->data(Qt::UserRole).toString());// 从项目数据中获取路径并添加到列表
    }
    return keys;
}

// 获取所选图片路径的函数
QStringList ImageListWidget::getSelectImageKeys() const
{
    QStringList keys;
    QList<QListWidgetItem*> selectedItems = this->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        // 从每一项的 UserRole 中提取存储的图片路径
        // toString() 确保数据安全转换为字符串
        keys.append(item->data(Qt::UserRole).toString());
    }
    return keys;
}

QMap<QString,QString> ImageListWidget::getImageMaps() const
{
    QMap<QString,QString> msgmap;
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *item = this->item(i);
        QString imageNo=item->text();
        QString imagekey=item->data(Qt::UserRole).toString();
        msgmap[imagekey]=imageNo;
    }
    return msgmap;
}

// 拖拽进入事件处理函数
void ImageListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // 检查拖拽数据是否包含URL或来自自身
    if (event->mimeData()->hasUrls() || event->source() == this) {
        event->acceptProposedAction();// 接受动作
    } else {
        event->ignore();// 忽略事件
    }
}
// 拖拽移动事件处理函数
void ImageListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    // 检查拖拽数据是否包含URL或来自自身
    if (event->mimeData()->hasUrls() || event->source() == this) {
        event->acceptProposedAction();// 接受动作
    } else {
        event->ignore();// 忽略事件
    }
}

// 拖放事件处理函数
void ImageListWidget::dropEvent(QDropEvent *event)
{
    qDebug() << __FUNCTION__<< __LINE__;
    const QMimeData *mimeData = event->mimeData();// 获取拖拽的MIME数据
    // 处理来自自身的拖拽（内部重排序）
    if (event->source() == this) {
        QListWidgetItem *draggedItem = currentItem();// 获取被拖拽的项目
        if (!draggedItem) return;

        QPoint pos = event->pos();// 获取拖放位置
        QListWidgetItem *targetItem = itemAt(pos);// 获取目标位置的项目
        //项目拖拽了，但不代表item中的顺序会同步变化，以下手动方式排列
        // 如果目标项目存在且不是被拖拽项目本身
        if (targetItem && targetItem != draggedItem) {
            // 获取拖动前和拖动后的索引值，比如从2->6
            int draggedIndex = row(draggedItem);
            int targetIndex = row(targetItem);
            // 提取出被拖拽项目,把2提取出（比如一个list链表，2被提取之后，2之后的就会自动往前排）
            QListWidgetItem *item = takeItem(draggedIndex);
            //把提取出的被拖拽项目插入到目标位置，2项的数据就插入到6位置了
            insertItem(targetIndex, item);
            setCurrentItem(item);//设置当前选中项目,设置6位置为选中项，保持原来选中的项内容不变
            // 发射图片顺序变化信号
            emit imageOrderChanged();
        }

        event->acceptProposedAction();
    } else if (mimeData->hasUrls()) {// 处理来自外部的文件拖放
        QStringList filePaths;
        QList<QUrl> urlList = mimeData->urls();// 获取所有URL

        for (const QUrl &url : urlList) {
            QString filePath = url.toLocalFile();// 转换为本地文件路径
            QFileInfo fileInfo(filePath);
            QString extension = fileInfo.suffix().toLower();// 获取文件扩展名并转换为小写
            // 检查是否为支持的图片格式
            if (extension == "png" || extension == "jpg" ||
            extension == "jpeg" || extension == "bmp" || extension == "gif") {
                filePaths.append(filePath);// 添加到文件路径列表
            }
        }

        if (!filePaths.isEmpty()) {
            addImages(filePaths);//添加图片
        }

        event->acceptProposedAction();
    } else {
        event->ignore();// 忽略不支持的拖放
    }
}

// 开始拖拽操作函数
void ImageListWidget::startDrag(Qt::DropActions supportedActions)
{
    QListWidgetItem *item = currentItem();// 获取当前选中的项目
    if (!item) return;

    QDrag *drag = new QDrag(this);// 创建拖拽对象
    QMimeData *mimeData = new QMimeData(); // 创建MIME数据对象
    // 从项目数据中获取文件路径
    QString filePath = item->data(Qt::UserRole).toString();
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(filePath));// 添加文件URL到URL列表
    mimeData->setUrls(urls);// 设置MIME数据的URL

    // 获取项目图标并转换为指定大小的像素图
    QPixmap pixmap = item->icon().pixmap(m_iconSize);
    // 创建稍大的透明拖拽图标
    QPixmap dragPixmap(m_iconSize + QSize(20, 20));
    dragPixmap.fill(Qt::transparent);
    // 在拖拽图标上绘制半透明的原图标
    QPainter painter(&dragPixmap);
    painter.setRenderHint(QPainter::Antialiasing);// 设置抗锯齿
    painter.setOpacity(0.7);// 设置透明度为70%
    painter.drawPixmap(10, 10, pixmap); // 绘制图标（偏移10像素）
    painter.end();

    drag->setMimeData(mimeData);// 设置拖拽对象的MIME数据
    drag->setPixmap(dragPixmap);// 设置拖拽时显示的图标
    // 设置拖拽热点（中心点）
    drag->setHotSpot(QPoint(dragPixmap.width()/2, dragPixmap.height()/2));
    // 执行拖拽操作
    drag->exec(supportedActions, Qt::MoveAction);
}
// 验证拖拽顺序的函数（用于调试）
void ImageListWidget::verifyDragOrder()
{
    qDebug() << "验证拖拽顺序 - 项目数量:" << count();
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *item = this->item(i);
        qDebug() << "位置" << i << ":" << item->text()
                 << "路径:" << item->data(Qt::UserRole).toString();
    }
}


void ImageListWidget::insertImagesAt(const QStringList& filePaths, int index)
{
    if (index < 0) index = 0;
    if (index > count()) index = count(); // 安全边界

    for (int i = 0; i < filePaths.size(); ++i) {
        const QString& path = filePaths[i];
        QPixmap pixmap(path);
        if (pixmap.isNull()) continue; // 跳过无效图片

        QPixmap scaledPixmap = pixmap.scaled(m_iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int num=imageCount()+1;//序号累加
        QString imageNo = QString("%1").arg(num, 3, 10, QChar('0'));//填充为：001
        // 创建列表项目，显示图标和文件名
        QListWidgetItem *item = new QListWidgetItem(QIcon(scaledPixmap),imageNo);
        item->setData(Qt::UserRole, path);// 记录主框架的ID，方便主框架获取到修改的信息
        item->setSizeHint(m_gridSize);// 设置项目大小提示
        // 设置项目标志Qt::ItemIsEnabled:启用;Qt::ItemIsSelectable:可选;Qt::ItemIsDragEnabled:可拖拽
        //item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
        // 插入到指定位置
        insertItem(index + i, item); // 注意：每插入一个，后续索引+1，所以用 index+i
    }

    // 可选：自动选中新插入的第一张
    if (!filePaths.isEmpty()) {
        setCurrentRow(index);
    }
}


void ImageListWidget::appendImage(const QString& filePath)
{
    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        // 可选：弹出错误提示
        QMessageBox::warning(this, "无效图片", "无法加载所选图片。");
        return;
    }
    addImage(filePath);
}
