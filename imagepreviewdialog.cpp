#include "imagepreviewdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QKeyEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QShowEvent>
#include <QDesktopWidget>

ImagePreviewDialog::ImagePreviewDialog(const QString &imagePath, QWidget *parent)
    : QDialog(parent)
    , m_scaleFactor(1.0)// 初始化缩放因子为1.0（原始大小）
    , m_rotationAngle(0) // 初始化旋转角度为0度
    , m_currentImageIndex(-1)// 初始化当前图片索引为-1（单图模式）
{
    setWindowTitle("图片预览 - " + QFileInfo(imagePath).fileName());
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    setMinimumSize(600, 500);  // 增大最小尺寸

    setupUI();
    setupToolbar();// 设置工具栏
    loadImage(imagePath);// 加载图片
}

ImagePreviewDialog::ImagePreviewDialog(const QPixmap &pixmap, QWidget *parent)
    : QDialog(parent)
    , m_scaleFactor(1.0)
    , m_rotationAngle(0)
    , m_currentImageIndex(-1)
{
    setWindowTitle("图片预览");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    setMinimumSize(600, 500);

    setupUI();
    setupToolbar();
    loadPixmap(pixmap);
}

// 设置UI布局和组件
void ImagePreviewDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);  // 减少边距

    // 创建工具栏
    m_toolBar = new QToolBar(this);
    m_toolBar->setMovable(false); // 禁止拖动工具栏

    // 创建动作
    m_zoomInAction = new QAction("放大", this);
    m_zoomOutAction = new QAction("缩小", this);
    m_fitToWindowAction = new QAction("适应窗口", this);
    m_originalSizeAction = new QAction("原始大小", this);
    m_rotateLeftAction = new QAction("左旋转", this);
    m_rotateRightAction = new QAction("右旋转", this);

    // 设置快捷键
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_fitToWindowAction->setShortcut(QKeySequence(Qt::Key_F));
    m_originalSizeAction->setShortcut(QKeySequence(Qt::Key_R));

    // 添加动作到工具栏
    m_toolBar->addAction(m_zoomInAction);
    m_toolBar->addAction(m_zoomOutAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_fitToWindowAction);
    m_toolBar->addAction(m_originalSizeAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_rotateLeftAction);
    m_toolBar->addAction(m_rotateRightAction);

    // 创建滚动区域和图片标签
    m_scrollArea = new QScrollArea(this);
    m_imageLabel = new QLabel(this);
    m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setVisible(true);
    m_scrollArea->setWidgetResizable(true);  // 允许调整大小

    // 添加到布局
    mainLayout->addWidget(m_toolBar);
    mainLayout->addWidget(m_scrollArea, 1);  // 设置拉伸因子为1，让图片区域占据更多空间

    // 连接信号槽
    connect(m_zoomInAction, &QAction::triggered, this, &ImagePreviewDialog::zoomIn);
    connect(m_zoomOutAction, &QAction::triggered, this, &ImagePreviewDialog::zoomOut);
    connect(m_fitToWindowAction, &QAction::triggered, this, &ImagePreviewDialog::fitToWindow);
    connect(m_originalSizeAction, &QAction::triggered, this, &ImagePreviewDialog::originalSize);
    connect(m_rotateLeftAction, &QAction::triggered, this, &ImagePreviewDialog::rotateLeft);
    connect(m_rotateRightAction, &QAction::triggered, this, &ImagePreviewDialog::rotateRight);
}

void ImagePreviewDialog::setupToolbar()
{
    // 设置工具栏样式表
    m_toolBar->setStyleSheet(
        "QToolBar {"  // 工具栏样式
        "    background-color: #f5f5f5;"  // 背景色
        "    border-bottom: 1px solid #dddddd;"  // 底部边框
        "    spacing: 5px;"  // 间距
        "    padding: 3px;"  // 内边距
        "}"
        "QToolButton {"  // 工具按钮样式
        "    background-color: transparent;"  // 透明背景
        "    border: 1px solid transparent;"  // 透明边框
        "    border-radius: 3px;"  // 圆角
        "    padding: 5px 8px;"  // 内边距
        "    font-size: 12px;"  // 字体大小
        "}"
        "QToolButton:hover {"  // 鼠标悬停样式
        "    background-color: #e0e0e0;"  // 背景色
        "    border: 1px solid #cccccc;"  // 边框
        "}"
        "QToolButton:pressed {"  // 鼠标按下样式
        "    background-color: #d0d0d0;"  // 背景色
        "}"
    );
}

// 加载图片
void ImagePreviewDialog::loadImage(const QString &imagePath)
{
    m_originalPixmap = QPixmap(imagePath);// 从文件路径加载图片
    if (m_originalPixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片: " + imagePath);
        return;
    }

    m_currentImagePath = imagePath;
    m_rotationAngle = 0;

    // 显示图片
    updateImageDisplay();

    // 计算最佳显示尺寸
    calculateOptimalSize();
}

void ImagePreviewDialog::loadPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "传入的图片数据为空");
        return;
    }

    m_originalPixmap = pixmap;
    m_currentImagePath = "";  // 清空路径，表示不是从文件加载
    m_rotationAngle = 0;

    updateImageDisplay();
    calculateOptimalSize();
}

// 计算最佳显示尺寸
void ImagePreviewDialog::calculateOptimalSize()
{
    if (m_originalPixmap.isNull()) return;

    // 获取屏幕尺寸
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // 计算图片原始尺寸
    QSize imageSize = m_originalPixmap.size();

    // 计算对话框的理想尺寸（屏幕的70-80%）
    int maxWidth = screenGeometry.width() * 0.8;
    int maxHeight = screenGeometry.height() * 0.8;

    // 如果图片比最大尺寸小，则按图片尺寸显示
    if (imageSize.width() <= maxWidth && imageSize.height() <= maxHeight) {
        // 图片较小，按原始大小显示，但加上一些边距
        resize(imageSize.width() + 50, imageSize.height() + 100);
        m_scaleFactor = 1.0;
    } else {
        // 图片较大，适应屏幕
        double widthRatio = (double)maxWidth / imageSize.width();
        double heightRatio = (double)maxHeight / imageSize.height();
        m_scaleFactor = qMin(widthRatio, heightRatio) * 0.9;  // 稍微缩小一点留出边距

        // 设置对话框大小
        QSize dialogSize = imageSize * m_scaleFactor;
        dialogSize += QSize(50, 100);  // 加上工具栏和边距的空间
        resize(dialogSize);
    }

    updateImageDisplay();
}

// 更新图片显示
void ImagePreviewDialog::updateImageDisplay()
{
    if (m_originalPixmap.isNull()) return;

    // 应用旋转
    QTransform transform;
    transform.rotate(m_rotationAngle);
    QPixmap rotatedPixmap = m_originalPixmap.transformed(transform, Qt::SmoothTransformation);

    // 应用缩放
    if (qFuzzyCompare(m_scaleFactor, 1.0)) {
        m_imageLabel->setPixmap(rotatedPixmap);
    } else {
        QSize newSize = rotatedPixmap.size() * m_scaleFactor;
        QPixmap scaledPixmap = rotatedPixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_imageLabel->setPixmap(scaledPixmap);
    }

    m_imageLabel->adjustSize();// 调整标签大小以适应图片
}

void ImagePreviewDialog::zoomIn()
{
    scaleImage(1.25);// 放大25%
}

void ImagePreviewDialog::zoomOut()
{
    scaleImage(0.8);// 缩小到80%
}

void ImagePreviewDialog::scaleImage(double factor)
{
    m_scaleFactor *= factor;

    // 限制缩放范围
    if (m_scaleFactor < 0.1) m_scaleFactor = 0.1;
    if (m_scaleFactor > 10.0) m_scaleFactor = 10.0;

    updateImageDisplay();

    // 调整滚动条以保持视图中心
    adjustScrollBar(m_scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(m_scrollArea->verticalScrollBar(), factor);
}

void ImagePreviewDialog::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2)));
}

// 适应窗口大小
void ImagePreviewDialog::fitToWindow()
{
    if (m_originalPixmap.isNull()) return;

    QSize scrollAreaSize = m_scrollArea->viewport()->size();
    QSize imageSize = m_originalPixmap.size();

    // 如果旋转了90或270度，则交换宽高
    if (m_rotationAngle % 180 == 90) {
        imageSize.transpose();
    }

    // 计算适应窗口的比例
    double widthRatio = (double)scrollAreaSize.width() / imageSize.width();
    double heightRatio = (double)scrollAreaSize.height() / imageSize.height();
    m_scaleFactor = qMin(widthRatio, heightRatio) * 0.95;  // 留一点边距

    updateImageDisplay();
}
// 显示原始大小
void ImagePreviewDialog::originalSize()
{
    m_scaleFactor = 1.0;
    updateImageDisplay();
}
// 向左旋转
void ImagePreviewDialog::rotateLeft()
{
    m_rotationAngle -= 90;// 减少90度
    if (m_rotationAngle < 0) m_rotationAngle += 360;
    updateImageDisplay();
    // 旋转后重新适应窗口
    QTimer::singleShot(50, this, &ImagePreviewDialog::fitToWindow);
}

void ImagePreviewDialog::rotateRight()
{
    m_rotationAngle += 90;
    if (m_rotationAngle >= 360) m_rotationAngle -= 360;
    updateImageDisplay();
    // 旋转后重新适应窗口
    QTimer::singleShot(50, this, &ImagePreviewDialog::fitToWindow);
}

void ImagePreviewDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // 窗口显示后，确保图片正确适应
    QTimer::singleShot(100, this, [this]() {
        if (m_scaleFactor < 0.5) {  // 如果缩放比例太小，重新适应窗口
            fitToWindow();
        }
    });
}

// 键盘事件处理
void ImagePreviewDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:  // ESC键：关闭窗口
        close();
        break;
    case Qt::Key_Plus:    // +键：放大
    case Qt::Key_Equal:   // =键：放大（与+键相同）
        zoomIn();
        break;
    case Qt::Key_Minus:   // -键：缩小
        zoomOut();
        break;
    case Qt::Key_0:       // 0键：原始大小
        originalSize();
        break;
    case Qt::Key_1:       // 1键：适应窗口
        fitToWindow();
        break;
    case Qt::Key_Left:    // 左箭头键：预留（可添加上一张图片功能）
        break;
    case Qt::Key_Right:   // 右箭头键：预留（可添加下一张图片功能）
        break;
    default:
        // 其他按键交给父类处理
        QDialog::keyPressEvent(event);
    }
}

// 鼠标滚轮事件处理
void ImagePreviewDialog::wheelEvent(QWheelEvent *event)
{
    // 如果按住Ctrl键滚动滚轮，进行缩放操作
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            // 向上滚动：放大
            zoomIn();
        } else {
            // 向下滚动：缩小
            zoomOut();
        }
        event->accept();  // 标记事件已处理
    } else {
        // 其他情况交给父类处理
        QDialog::wheelEvent(event);
    }
}
