#ifndef IMAGELISTWIDGET_H
#define IMAGELISTWIDGET_H

#include <QListWidget>
#include <QString>
#include <QSize>
#include <QDebug>
#include <QPixmap>
#include <QMap>
class ImageListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit ImageListWidget(QWidget *parent = nullptr);

    void addPixmap(const QPixmap &pixmap,const QString &mainid);  // 添加单个图片
    void addImage(const QString &filePath);  // 添加单个图片
    void addImages(const QStringList &filePaths);  // 添加多个图片
    void appendImage(const QString& filePath);//在末尾增加电脑本地的图片
    void insertImagesAt(const QStringList& filePaths, int index);//插入电脑本地的图片
    void clearAllImages();  // 清空所有图片
    int imageCount() const;  // 获取图片数量
    QStringList getImageKeys() const;  // 获取所有图片的关键标识
    QStringList getSelectImageKeys() const;  // 获取所选中图片的关键标识
    QMap<QString,QString> getImageMaps() const;

signals:
    void imageSelected(const QString &filePath);  // 图片被选中信号
    void imageCountChanged(int count);  // 图片数量变化信号
    void imageOrderChanged();  // 图片顺序变化信号
    void requestContextMenu(const QPoint &pos);  // 请求上下文菜单信号
    void imageDoubleClicked(const QString &filePath);  // 图片双击信号

public slots:
    void verifyDragOrder();
    void deleteSelectedImage();// 删除选中图片

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;  // 拖拽进入事件
    void dragMoveEvent(QDragMoveEvent *event) override;  // 拖拽移动事件
    void dropEvent(QDropEvent *event) override;  // 拖放事件
    void startDrag(Qt::DropActions supportedActions) override;  // 开始拖拽
    void mousePressEvent(QMouseEvent *event) override;  // 鼠标按下事件
    void mouseDoubleClickEvent(QMouseEvent *event) override;  // 鼠标双击事件

private:
    void initializeUI();// 初始化UI
    int m_number=0;
    QSize m_gridSize;  // 网格大小
    QSize m_iconSize;  // 图标大小
};

#endif // IMAGELISTWIDGET_H
