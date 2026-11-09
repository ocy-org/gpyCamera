#ifndef IMAGEPREVIEWDIALOG_H
#define IMAGEPREVIEWDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QScrollArea>
#include <QAction>
#include <QToolBar>
#include <QString>
#include <QPixmap>
#include <QTimer>
class ImagePreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImagePreviewDialog(const QString &imagePath, QWidget *parent = nullptr);
    explicit ImagePreviewDialog(const QPixmap &pixmap, QWidget *parent = nullptr);
    ~ImagePreviewDialog() = default;

protected:
    void keyPressEvent(QKeyEvent *event) override;// 重写键盘事件处理函数
    void wheelEvent(QWheelEvent *event) override;// 重写鼠标滚轮事件处理函数
    void showEvent(QShowEvent *event) override;// 重写显示事件处理函数（窗口显示时触发）

private:
    void setupUI();
    void setupToolbar();
    void loadImage(const QString &imagePath);
    void loadPixmap(const QPixmap &pixmap);
    void updateImageDisplay();
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void calculateOptimalSize();

    // 界面控件
    QLabel *m_imageLabel;        // 显示图片的标签
    QScrollArea *m_scrollArea;   // 滚动区域（用于显示大图片）
    QToolBar *m_toolBar;         // 工具栏

    // 图片数据
    QPixmap m_originalPixmap;    // 原始图片数据
    double m_scaleFactor;        // 当前缩放比例
    int m_rotationAngle;         // 旋转角度（0, 90, 180, 270度）
    QString m_currentImagePath;  // 当前图片路径
    int m_currentImageIndex;     // 当前图片索引（预留，用于多图片浏览）

    // 工具栏动作
    QAction *m_zoomInAction;        // 放大动作
    QAction *m_zoomOutAction;       // 缩小动作
    QAction *m_fitToWindowAction;   // 适应窗口动作
    QAction *m_originalSizeAction;  // 原始大小动作
    QAction *m_rotateLeftAction;    // 左旋转动作
    QAction *m_rotateRightAction;   // 右旋转动作

private slots:
    void zoomIn();        // 放大图片
    void zoomOut();       // 缩小图片
    void fitToWindow();   // 适应窗口显示
    void originalSize();  // 原始大小显示
    void rotateLeft();    // 向左旋转90度
    void rotateRight();   // 向右旋转90度
};

#endif // IMAGEPREVIEWDIALOG_H
