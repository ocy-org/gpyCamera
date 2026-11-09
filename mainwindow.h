#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QDesktopServices>
#include <QPrinter>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QToolBar>
#include <QMenuBar>
#include <QAction>
#include <QTimer>
#include <QUrl>
#include <QInputDialog>
#include <QDir>
#include <QScreen>
#include <QThread>
#include <QPolygonF>
#include <QMetaType>
#include <opencv2/opencv.hpp>

#include "cameraworker.h"           // 多线程摄像头枚举（原有，零改动）
#include "dealcontours.h"           // OpenCV轮廓算法（原有，零改动）
#include "imagelistwidget.h"
#include "imagepreviewdialog.h"
#include "util/comm_define.h"
#include "util/commfuntion.h"
#include "util/countdownwidget.h"
#include "util/LeftClickOnlyMenu.h"
#include "util/linebasedfilehandler.h"
#include "dialog/menu_maininfodialog.h"
#include "dialog/menu_filesavedialog.h"
#include "dialog/menu_activationdialog.h"
#include "dialog/menu_pdfconfdialog.h"
#include "dialog/menu_cameracontroldialog.h"
#include "dialog/menu_savefilenameruledialog.h"
#include "dialog/imagecompressdialog.h"

// ===== 新架构前置声明：像素管线移出MainWindow =====
class CaptureWorker;    // FFmpeg采集解码线程（framepacket/captureworker.h）
class DetectWorker;     // OpenCV检测线程（detectworker.h）
// VideoGLWidget 的完整类型由 ui_mainwindow.h（.ui提升后）提供

#define VERSION "v2.0.0_gl"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(bool flag, QWidget *parent = nullptr);
    ~MainWindow();
    bool m_saveflag = false;
    //-----启动时的一次性操作----↓↓↓↓↓
private:
    void initUI();
    void opencameraStyleSheet();
    void closecameraStyleSheet();
    void setupToolBar();
    void getconfmsg();
    void getsavefileconf();
    void setupConnections();
    void openHelpDocument();

    QString m_imgpath;
    QString m_pdfpath;
    QString m_filesavepath;
    QString m_filesaveadddirstatus;
    int m_pdf_pagenum = 2;
    int m_mergeimagenum = 2;
    menu_maininfoDialog *m_maininfodialog;
    menu_filesaveDialog *m_filesavedialog;
    ActivationDialog *m_activationdialog;
    menu_pdfconfDialog *m_pdfconfdialog;
    menu_cameracontrolDialog *m_cameracontroldialog;
    menu_savefilenameruledialog *m_savefilenameruledialog;
    commfuntion *m_comm;
    dealContours *m_dealContours = nullptr;
    QMap<QString, stCameraMsg> m_camerajsondatamap;

private slots:
    void onMaininfoaction();
    void onControlaction();
    void onfilesaveaction();
    void onNameruleaction();
    void onActivationaction();
    void onPdfconfaction();
    //-----启动时的一次性操作----↑↑↑↑↑

    //-----多线程获取摄像头信息（原有CameraWorker，零改动）----↓↓↓↓↓
private:
    void setupThreadmove();
    QThread *m_cameraThread = nullptr;
    CameraWorker *m_cameraWorker = nullptr;
    QList<QString> m_lastCameraList;
    QMap<QString, QList<CameraCapability>> m_camerasinfomap;
    QMap<QString, QString> m_cameradestodevnamemap;
    mainconfDialog *m_confdialog;
    int m_mainCameraindex = 0;
    int m_subCameraindex = 0;
    void updateCameraComboBox();
    void updateResolutionComboBox();

private slots:
    void refreshCameraList();
    void onCamerasDetected(const QList<CameraInfo> &cameras);
    void onDetectionFailed(const QString &error);
    void on_changecameraButton_clicked();
    //-----多线程获取摄像头信息----↑↑↑↑↑

    //-----摄像头处理（重构：GL渲染 + 双线程管线）----↓↓↓↓↓
private:
    void openCamera();
    void closeCamera();
    void updateCameraButtonState();

    // ===== 新增：采集/检测线程（替代原initffmpeg/openCameraWithBestParams/updateFrame）=====
    CaptureWorker *m_capWorker = nullptr;   // 采集线程worker（拥有两个FrameMailbox）
    QThread      *m_capThread = nullptr;
    DetectWorker *m_detWorker = nullptr;    // 检测线程worker
    QThread      *m_detThread = nullptr;

    // ===== 新增：手动矩形改存“视频像素坐标”，缩放/旋转/窗口变化自动贴合 =====
    QRectF m_manualRectVideo;               // 最终矩形（视频坐标）
    QPoint m_manualDragStart;               // 拖拽起点（widget坐标，仅拖拽期间用）
    bool   m_manualDragging = false;
    void clearManualRect();

    bool isAutoCropMode = true;
    bool m_isCameraOpened = false;
    //-----摄像头处理----↑↑↑↑↑

private slots:
    void on_cameraToggleButton_clicked();
    void onAutoCropToggled(bool checked);
    void onManualCropToggled(bool checked);

    //-----裁边和滚轮响应----↓↓↓↓↓
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
private:
    void handleMousePressEvent(QMouseEvent *event);
    void handleMouseMoveEvent(QMouseEvent *event);
    void handleMouseReleaseEvent(QMouseEvent *event);
    //-----裁边和滚轮响应----↑↑↑↑↑

    //-----拍照功能----↓↓↓↓↓
private:
    void saveManualCroppedImage(cv::Mat &rawFrame);   // 手动切边：视频坐标裁剪
    void saveAutoCroppedImage(cv::Mat &dealResFrame); // 自动切边：原有warp逻辑零改动
    float calculateDistance(const cv::Point& p1, const cv::Point& p2);
    void reorderPointsForWideRectangle(std::vector<cv::Point>& contourPoints);

private slots:
    void on_captureBtn_clicked();
    //-----拍照功能----↑↑↑↑↑

    //-----文件保存路径----↓↓↓↓↓
private:
    void saveconfmsg(const QString& flag, const QString& value);
    //-----文件保存路径----↑↑↑↑↑

    //-----PDF生成操作----↓↓↓↓↓
private:
    QMap<QString, stImgMsg> m_saveimagemap;
    QPixmap cvMatToQPixmap(const cv::Mat &mat);
    bool generatePdf(const QString &filePath);
    bool generatePdfbyimg(const QString &filePath, const QPixmap &pixmap);
    void rotateSelectedImage(int angle);
    QPixmap rotatePixmap(const QPixmap &pixmap, int angle);
    void openfilesavepath(const QString& filedir = "");
    QString GetPrefixName();
    QString GetAskName();

private slots:
    void on_clearButton_clicked();
    void on_generatePdfButton_clicked();
    void on_generatemorePdfButton_clicked();
    void on_generateIamgeButton_clicked();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    //-----PDF生成操作----↑↑↑↑↑

    //-----响应Enter键拍照----↓↓↓↓↓
protected:
    void keyPressEvent(QKeyEvent *event) override;
    //-----响应Enter键拍照----↑↑↑↑↑

    //-----注册激活功能----↓↓↓↓↓
private:
    void setactivationUI(bool flag);
    //-----注册激活功能----↑↑↑↑↑

    //-----图片处理框右键/快捷键----↓↓↓↓↓
private:
    void setupImageListShortKey();
    void insertImageBeforeCur();
    void moveUpImage();
    void moveDownImage();
    void rotateLeftImage();
    void rotateRightImage();
    void previewImage();
    QAction *m_deleteImageAction;

private slots:
    void showImageListContextMenu(const QPoint &pos);
    void deleteSelectedImage();
    void onImageDescriptionChanged(QListWidgetItem *item);
    //-----图片处理框右键/快捷键----↑↑↑↑↑
    //-----延时5秒拍照功能----↓↓↓↓↓
private:
    CountdownWidget* m_countdownWidget;
    void Creatdelaycapture();

private slots:
    void on_delaycaptureBtn_clicked();
    void onDelayTimeout();
    //-----延时5秒拍照功能----↑↑↑↑↑

    //-----手动裁边写入配置----↓↓↓↓↓
private:
    LineBasedFileHandler* manualcropfileop;
    QString m_lastmunualcrop_cameraname = "";
    void updateManualCropbyconf(QString cameraName);
    bool writeManualCroptoconf(QString cameraName);
    bool deleteManualCroptoconf(QString cameraName);
    //-----手动裁边写入配置----↑↑↑↑↑

    //-----文件命名配置菜单----↓↓↓↓↓
private:
    void getconfnamerulemsg();
    QString m_oneimg_format = "JPG";
    QString m_oneimg_name_type = "0";
    QString m_oneimg_pre_type = "0";
    QString m_oneimg_pre_name = "";
    QString m_moreimg_format = "JPG";
    QString m_moreimg_pre_type = "0";
    QString m_moreimg_pre_name = "";
    QString m_onepdf_pdfname_type = "0";
    QString m_onepdf_pre_type = "0";
    QString m_onepdf_pre_name = "";
    QString m_morepdf_pre_type = "0";
    QString m_morepdf_pre_name = "";
    //-----文件命名配置菜单----↑↑↑↑↑

    //-----预览旋转v1.2.2----↓↓↓↓↓
private slots:
    void on_turnrightButton_clicked();
    void on_turnleftButton_clicked();
    void on_generateOneIamgeButton_clicked();

private:
    Mat rotateMatByDegrees(const cv::Mat& src, int degrees) const;
    int m_turn_right = 90;
    int m_turn_left = -90;
    int m_displayRotation = 0;
    //-----预览旋转----↑↑↑↑↑

private:
    void mrgimgdeletesrcimg();
    void mrgimgsavesrcimg();
    void mergeSelectedImages(bool deleteOriginals = false);
    void wgmergeSelectedImages(bool deleteOriginals = false);

    //-----窗口尺寸自适应v1.2.4----↓↓↓↓↓
private:
    QTimer *m_windowResizeTimer;
    int m_lastwindowWidth;
    int m_lastwindowHeight;
    QString m_winsize;
    void savewindowsize();
    void initsize();
    void adaptWidgetPosX(QWidget* widget, int src_X, int adapt_width);
    void adaptWidgetPosXHeight(QWidget* widget, int src_X, int src_h, int adapt_width, int adapt_height);
    void adaptWidgetPosY(QWidget* widget, int adapt_height, int src_Y);
    void adaptWidgetPosXY(QWidget* widget, int src_w, int src_h, int adapt_width, int adapt_height);
    void adaptWidgetWH(QWidget* widget, int src_X, int src_Y, int src_w, int src_h, int adapt_width, int adapt_height);

    int m_width;
    int m_height;
    int m_srcwidth;
    int m_srcheight;
    int m_videoLabel_width;
    int m_videoLabel_height;
    int m_imageList_width;
    int m_imageList_height;
    int m_imageList_posx;
    int m_saveGroup_width;
    int m_saveGroup_height;
    int m_saveGroup_posx;
    int m_imgGroup_width;
    int m_imgGroup_height;
    int m_imgGroup_posx;
    int m_videoGroup_width;
    int m_videoGroup_height;
    int m_line_posx;
    int m_line_height;
    int m_logolabel_width;
    int m_logolabel_height;
    int m_logolabel_posx;
    int m_logolabel_posy;
    int m_versionlabel_width;
    int m_versionlabel_height;
    int m_versionlabel_posx;
    int m_versionlabel_posy;
    int m_fpsLabel_width;
    int m_fpsLabel_height;
    int m_fpsLabel_posx;
    int m_fpsLabel_posy;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void onWindowResizeDetected();
    //-----窗口尺寸自适应----↑↑↑↑↑

    //-----右键菜单图片压缩v1.2.5----↓↓↓↓↓
private:
    ImageCompressDialog* m_compdialog;
    QString m_curcompimagekey;
    QMap<QString, int> m_saveimagecompmap;

private slots:
    void CompressImage();
    //-----右键菜单图片压缩----↑↑↑↑↑

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
