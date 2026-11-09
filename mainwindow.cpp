#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "videoglwidget.h"      // GL渲染控件
#include "captureworker.h"      // 采集解码线程
#include "detectworker.h"       // OpenCV检测线程
#include "framepacket.h"

#include <QMessageBox>
#include <QSettings>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QDateTime>
#include <QFileDialog>
#include <QImageReader>
#include <iostream>
#include "util/permit.h"

MainWindow::MainWindow(bool flag, QWidget *parent)
    : QMainWindow(parent)
    , m_saveflag(flag)
    , m_dealContours(new dealContours)
    , manualcropfileop(new LineBasedFileHandler(MANUALCROPFILE))
    , ui(new Ui::MainWindow)
{
    qRegisterMetaType<QPolygonF>("QPolygonF");   // 跨线程轮廓信号

    ui->setupUi(this);
    //1.初始化
    initsize();
    //1.2 手动设置的UI布局
    initUI();
    //1.3 获取配置信息
    getconfmsg();
    //2.更新摄像头列表信息---多线程方式
    refreshCameraList();
    //3.连接信号槽
    setupConnections();

    // 检测是否有记录的窗口尺寸
    QStringList list = m_winsize.split('x');
    if (list.size() == 2) {
        int w = list[0].toInt();
        int h = list[1].toInt();
        this->resize(w, h);
        QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
        int x = (screenGeometry.width() - this->width()) / 2;
        int y = (screenGeometry.height() - this->height()) / 2;
        this->move(x, y);
    }

    // 延迟检查激活状态
    QTimer::singleShot(100, this, [this]() {
        bool ret = m_activationdialog->isActivation();
        if (0) ret = true;
        if (ret == false) {
            setactivationUI(false);
            onActivationaction();// 此时主窗口已经显示
        }
    });

    Creatdelaycapture();
    ui->label_cameraconf->installEventFilter(this);//安装事件过滤器
}

MainWindow::~MainWindow()
{
    closeCamera();   // 确保采集/检测线程已退出（幂等）

    delete ui;
    delete m_dealContours;
    delete m_confdialog;
    delete m_filesavedialog;
    delete m_activationdialog;
    delete m_pdfconfdialog;
    delete m_cameracontroldialog;
    delete m_savefilenameruledialog;
    delete m_deleteImageAction;

    // 摄像头枚举线程（原有CameraWorker）
    if (m_cameraThread && m_cameraThread->isRunning()) {
        m_cameraThread->quit();
        if (!m_cameraThread->wait(3000)) {
            qWarning() << "线程无法正常退出，强制终止";
            m_cameraThread->terminate();
            m_cameraThread->wait();
        }
    }
    m_cameraWorker->deleteLater();

    delete m_countdownWidget;
    delete manualcropfileop;
    delete m_compdialog;
}

// 初始化UI
void MainWindow::initUI()
{
    this->setWindowTitle("高拍仪拍照工具");
    this->setWindowIcon(QIcon(":/index/img/default.png"));
    ui->label_version->setText(VERSION);
    ui->radioAutoCrop->setChecked(isAutoCropMode);//自动切边功能，默认是选中
    // 初始禁用描边控制控件
    ui->captureBtn->setEnabled(false);
    ui->delaycaptureBtn->setEnabled(false);
    ui->changecameraButton->setEnabled(false);
    ui->turnleftButton->setEnabled(false);
    ui->turnrightButton->setEnabled(false);
    setAcceptDrops(true);// 允许主窗口接收拖拽

    setupImageListShortKey();// 设置右键菜单

    // videoLabel现为VideoGLWidget（QOpenGLWidget）
    ui->videoLabel->setMouseTracking(true);// 设置videoLabel的鼠标跟踪
    ui->videoLabel->installEventFilter(this);//安装事件
    //设置按钮样式
    opencameraStyleSheet();
    // 创建工具栏
    m_confdialog = new mainconfDialog(this);
    m_filesavedialog = new menu_filesaveDialog(this);
    m_activationdialog = new ActivationDialog(this);
    m_pdfconfdialog = new menu_pdfconfDialog(this);
    m_cameracontroldialog = new menu_cameracontrolDialog(this);
    m_savefilenameruledialog = new menu_savefilenameruledialog(this);
    setupToolBar();

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    QString filesavepath = m_filesavedialog->getfilesavepath();
    if (m_saveflag == true) filesavepath = ZEROCODEFILESAVEPATH;
    ui->label_filesavepath->setText(filesavepath);
    ui->label_filesavepath->installEventFilter(this);

    ui->turnleftButton->setIcon(QIcon(":/index/img/left.png"));
    ui->turnrightButton->setIcon(QIcon(":/index/img/right.png"));
    setMinimumSize(1512, 798);
    //初始化窗口侦测定时器
    m_windowResizeTimer = new QTimer(this);
    connect(m_windowResizeTimer, &QTimer::timeout, this, &MainWindow::onWindowResizeDetected);
    // 启动定时器，每 200ms 检查一次（频率不需要太高，避免占用资源）
    m_windowResizeTimer->start(1000);

    m_compdialog = new ImageCompressDialog(this);
}

void MainWindow::opencameraStyleSheet()
{
    ui->cameraToggleButton->setText("打开摄像头");
    ui->cameraToggleButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
}

void MainWindow::closecameraStyleSheet()
{
    ui->cameraToggleButton->setText("关闭摄像头");
    ui->cameraToggleButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; }");
}

void MainWindow::setupToolBar()
{
    connect(ui->maininfoaction, &QAction::triggered, this, &MainWindow::onMaininfoaction);
    connect(ui->controlaction, &QAction::triggered, this, &MainWindow::onControlaction);
    connect(ui->filesaveaction, &QAction::triggered, this, &MainWindow::onfilesaveaction);
    connect(ui->nameruleaction, &QAction::triggered, this, &MainWindow::onNameruleaction);
    connect(ui->activationaction, &QAction::triggered, this, &MainWindow::onActivationaction);
    connect(ui->pdfconfaction, &QAction::triggered, this, &MainWindow::onPdfconfaction);
    connect(ui->helpdocaction, &QAction::triggered, this, &MainWindow::openHelpDocument);
}

void MainWindow::onMaininfoaction()
{
    qDebug() << __FUNCTION__;
    m_confdialog->setModal(false);
    m_confdialog->setWindowTitle("主/副摄像头配置");
    m_confdialog->init(m_lastCameraList, m_mainCameraindex, m_subCameraindex);
    m_confdialog->open();
    m_confdialog->exec();
}

void MainWindow::onControlaction()
{
    closeCamera();
    m_cameracontroldialog->setModal(false);
    m_cameracontroldialog->setWindowTitle("选择摄像头");
    m_cameracontroldialog->init();
    m_cameracontroldialog->open();
    m_cameracontroldialog->exec();
}

void MainWindow::onfilesaveaction()
{
    qDebug() << __FUNCTION__;
    m_filesavedialog->setModal(false);
    m_filesavedialog->setWindowTitle("文件保存设置");
    m_filesavedialog->open();
    m_filesavedialog->exec();
}

void MainWindow::onNameruleaction()
{
    qDebug() << __FUNCTION__;
    m_savefilenameruledialog->setModal(false);
    m_savefilenameruledialog->setWindowTitle("保存文件命名规则设置");
    m_savefilenameruledialog->open();
    m_savefilenameruledialog->exec();
}

void MainWindow::onActivationaction()
{
    qDebug() << __FUNCTION__;
    m_activationdialog->setModal(false);
    m_activationdialog->setWindowTitle("注册激活");
    m_activationdialog->open();
    m_activationdialog->exec();
}

void MainWindow::onPdfconfaction()
{
    qDebug() << __FUNCTION__;
    m_pdfconfdialog->setModal(false);
    m_pdfconfdialog->setWindowTitle("PDF配置");
    m_pdfconfdialog->open();
    m_pdfconfdialog->exec();
}

void MainWindow::openHelpDocument()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString helpdocFilePath = QDir(appDir).filePath("功能介绍.pdf");
    QFileInfo fileInfo(helpdocFilePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "文件未找到", QString("介绍文档文件丢失。").arg(helpdocFilePath));
        return;
    }
    QUrl fileUrl = QUrl::fromLocalFile(helpdocFilePath);
    if (!QDesktopServices::openUrl(fileUrl)) {
        QMessageBox::warning(this, "打开失败", "无法打开帮助文档，请检查系统默认浏览器设置。");
    }
}

void MainWindow::getconfmsg()
{
    getsavefileconf();
    getconfnamerulemsg();
}

void MainWindow::getsavefileconf()
{
    QSettings settings(CONFFILE, QSettings::IniFormat);
    QString DocPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_filesaveadddirstatus = settings.value("adddir", "0").toString();
    m_pdf_pagenum = settings.value("pdf_image_num", 2).toInt();
    m_mergeimagenum = settings.value("image_merge_num", 2).toInt();
    m_winsize = settings.value("winsize", "771x531").toString();
    if (m_saveflag) {
        m_filesavepath = ZEROCODEFILESAVEPATH;
    } else
        m_filesavepath = settings.value("filepath", "").toString();

    qDebug() << __LINE__ << m_filesavepath << m_saveflag;
    if (m_filesavepath == "") m_filesavepath = DocPath;
    if (m_filesaveadddirstatus == "1" && m_saveflag == false) {
        QString today = QDate::currentDate().toString("yyyyMMdd");
        m_filesavepath = m_filesavepath + "/" + today;
        if (m_filesavepath.isEmpty()) {
            QMessageBox::warning(nullptr, "路径错误", "目录路径为空！");
            m_filesavepath = DocPath;
        }
    }

    QDir dir(m_filesavepath);
    if (dir.exists()) {
        qDebug() << "目录已存在：" << m_filesavepath;
    } else {
        if (dir.mkpath(".")) {
            qDebug() << "成功创建目录：" << m_filesavepath;
        } else {
            QMessageBox::critical(nullptr, "创建目录失败",
                QString("无法创建目录：\n%1\n\n请检查路径是否合法，或是否有写入权限。将影响预览图片生成文件的功能")
                    .arg(m_filesavepath));
            m_filesavepath = DocPath;
        }
    }
    m_imgpath = m_filesavepath;
    m_pdfpath = m_filesavepath;
    qDebug() << "m_filesavepath:" << m_filesavepath << m_saveflag;
}

// 摄像头枚举线程--多线程方式获取摄像头信息
void MainWindow::setupThreadmove()
{
    m_cameraThread = new QThread(this);// 创建后台线程
    m_cameraWorker = new CameraWorker();//创建worker对象
    m_cameraWorker->moveToThread(m_cameraThread);// 将worker对象移至后台线程
    //接收线程启动信号，并触发槽函数CameraWorker::startDet
    connect(m_cameraThread, &QThread::started, m_cameraWorker, &CameraWorker::startDetection);
    //接收线程发送的信号，触发正常/异常检查结果
    connect(m_cameraWorker, &CameraWorker::camerasDetected, this, &MainWindow::onCamerasDetected);
    connect(m_cameraWorker, &CameraWorker::detectionFailed, this, &MainWindow::onDetectionFailed);
    // 启动线程
    m_cameraThread->start();
}

void MainWindow::onCamerasDetected(const QList<CameraInfo> &cameras)
{
    QList<QString> newcameras;
    bool bfind = false;
    qDebug() << "在主线程中接收到摄像头列表，数量:" << cameras.size();
    for (const CameraInfo &camera : cameras) {
        QString cameraname = camera.description;
        QString devicename = camera.deviceName;
        newcameras.append(cameraname);
        //看看是不是新的摄像头
        if (!m_lastCameraList.contains(cameraname)) bfind = true;
        if (m_camerasinfomap.count(cameraname) == 0) {
            QList<CameraCapability> capabilities = camera.formats;
            m_camerasinfomap[cameraname] = capabilities;
            m_cameradestodevnamemap[cameraname] = devicename;
        }
    }
    for (const QString &camera : m_lastCameraList) {
        if (!newcameras.contains(camera)) bfind = true;
    }
    if (bfind) {//有新增的，说明出现变化了
        // 更新最新的摄像头下拉列表框
        m_lastCameraList = newcameras;
        updateCameraComboBox();
        updateResolutionComboBox();
    }
}

void MainWindow::onDetectionFailed(const QString &error)
{
    qDebug() << "摄像头检测失败:" << error;
}

//更新摄像头名称下拉框信息
void MainWindow::updateCameraComboBox()
{
    //匹配主、副摄像头配置信息
    bool bconf = false;
    //读取配置信息
    m_camerajsondatamap.clear();
    m_comm->getconfbyjson(m_camerajsondatamap);
    qDebug() << __FUNCTION__ << m_camerajsondatamap.size();
    //查看是否存在主副摄像头配置信息
    for (auto it = m_camerajsondatamap.begin(); it != m_camerajsondatamap.end(); ++it) {
        stCameraMsg cameramsg = it.value();
        QString main = cameramsg.maincamera;
        QString sub = cameramsg.subcamera;
        if (m_lastCameraList.contains(main) > 0 && m_lastCameraList.contains(sub) > 0) bconf = true;
    }

    QList<QString> newnamelist;
    QString curCamera = ui->cameraComboBox->currentText();
    QString src_curCamera = "";
    qDebug() << "curCamera" << curCamera;
    //重新生成摄像头列表
    ui->cameraComboBox->clear();
    int index = 0;
    for (const QString &cameraname : m_lastCameraList) {
        QString newname = cameraname;
        if (bconf == true) {
            bool bfind = false;
            for (auto it = m_camerajsondatamap.begin(); it != m_camerajsondatamap.end(); ++it) {
                stCameraMsg cameramsg = it.value();
                QString main = cameramsg.maincamera;
                QString sub = cameramsg.subcamera;
                if (main == cameraname) {
                    m_mainCameraindex = index;
                    newname = QString("【主摄像头】%1").arg(cameraname);
                    bfind = true;
                    break;
                } else if (sub == cameraname) {
                    m_subCameraindex = index;
                    newname = QString("【副摄像头】%1").arg(cameraname);
                    bfind = true;
                    break;
                }
            }
            if (bfind == false) newname = QString("【未知摄像头】%1").arg(cameraname);
        } else {
            newname = QString("【未知摄像头】%1").arg(cameraname);
            qDebug() << "【未知摄像头】" << newname;
        }
        if (curCamera.contains(cameraname) > 0) src_curCamera = cameraname;
        ui->cameraComboBox->addItem(newname);
        newnamelist.append(newname);
        qDebug() << "append newname:" << newname;
        index++;
    }

    if (curCamera.contains("主摄像头") || curCamera.contains("副摄像头")) {
        qDebug() << "src_curCamera:" << src_curCamera << "curCamera:" << curCamera;
        for (const QString &cameraname : newnamelist) {
            if (cameraname.contains(src_curCamera)) {
                qDebug() << "src_curCamera:" << src_curCamera << "cameraname:" << cameraname;
                ui->cameraComboBox->setCurrentText(cameraname);
            }
        }
    } else {
        if (!m_isCameraOpened && bconf == true) {
            qDebug() << "m_mainCameraindex:" << m_mainCameraindex;
            ui->cameraComboBox->setCurrentIndex(m_mainCameraindex);
        }
    }

    // v1.1.1 检测并更新裁边设置
    QString cameraName = ui->cameraComboBox->currentText();
    if (cameraName.contains("】")) {
        QStringList parts = cameraName.split("】");
        if (parts.size() > 1) cameraName = parts.last();
    }
    updateManualCropbyconf(cameraName);
}

//更新分辨率下拉框信息
void MainWindow::updateResolutionComboBox()
{
    qDebug() << __FUNCTION__;
    statusBar()->showMessage("更新摄像头支持的分辨率...");// 在状态栏显示加载信息
    // 更新分辨率下拉列表框
    ui->resolutionComboBox->clear();
    QString curCamera = ui->cameraComboBox->currentText();
    //查看是不是主副摄像头的标识
    if (curCamera.contains("】")) {
        QStringList parts = curCamera.split("】");
        if (parts.size() > 1) curCamera = parts.last();
    }

    QList<CameraCapability> resolutionInfos = m_camerasinfomap[curCamera];
    for (const CameraCapability &info : resolutionInfos) {
        QString displayText = QString("%1x%2 - %3-%4fps")
                                  .arg(info.resolution.width())
                                  .arg(info.resolution.height())
                                  .arg(info.codec)
                                  .arg(info.maxFps, 0, 'f', 1);
        ui->resolutionComboBox->addItem(displayText);
        // 存储相关信息
        QVariant var;
        var.setValue(info);
        int index = ui->resolutionComboBox->count() - 1;
        ui->resolutionComboBox->setItemData(index, var);
    }
    statusBar()->showMessage(QString("找到 %1 个支持的分辨率").arg(resolutionInfos.size()), 3000);
}

void MainWindow::refreshCameraList()
{
    ui->cameraComboBox->clear();
    setupThreadmove();
}

void MainWindow::setupConnections()
{
    // UI控件响应
    {
        connect(ui->cameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index) {
            QString cameraName = ui->cameraComboBox->currentText();
            if (cameraName.contains("】")) {
                QStringList parts = cameraName.split("】");
                if (parts.size() > 1) cameraName = parts.last();
            }
            updateManualCropbyconf(cameraName);
            updateResolutionComboBox();
        });
        connect(ui->radioAutoCrop, &QRadioButton::toggled, this, &MainWindow::onAutoCropToggled);
        connect(ui->radioManualCrop, &QRadioButton::toggled, this, &MainWindow::onManualCropToggled);
    }

    // 图片列表
    {
        connect(ui->imageListWidget, &ImageListWidget::requestContextMenu, this, &MainWindow::showImageListContextMenu);
        connect(ui->imageListWidget, &ImageListWidget::imageDoubleClicked, this, &MainWindow::previewImage);
    }

    connect(m_activationdialog, &ActivationDialog::refreshactivation, this, [=]() {
        qDebug() << "主框架接收到激活信号";
        setactivationUI(true);
    });

    connect(m_confdialog, &mainconfDialog::refreshtabledata, this, [=]() {
        qDebug() << "主框架接收到配置信息变化的信号";
        updateCameraComboBox();
    });

    connect(m_filesavedialog, &menu_filesaveDialog::refreshfilesavepath, this, [=](const QString& filesavepath) {
        qDebug() << "主框架接收到文件保存路径信息变化:" << filesavepath;
        getsavefileconf();
        ui->label_filesavepath->setText(m_filesavepath);
    });

    connect(m_pdfconfdialog, &menu_pdfconfDialog::refreshpdfdata, this, [=](const int& pdfimagenum) {
        qDebug() << "主框架接收到PDF信息变化:" << pdfimagenum;
        m_pdf_pagenum = pdfimagenum;
    });

    connect(m_pdfconfdialog, &menu_pdfconfDialog::refreshmergeimagedata, this, [=](const int& mergeimagenum) {
        qDebug() << "主框架接收到图片合并数量变化:" << mergeimagenum;
        m_mergeimagenum = mergeimagenum;
    });

    connect(m_savefilenameruledialog, &menu_savefilenameruledialog::refreshnamerule, this, [=]() {
        qDebug() << "主框架接收到文件命名规则信息变化";
        getconfnamerulemsg();
    });

    connect(m_compdialog, &ImageCompressDialog::refreshpercent, this, [=](const int& percent) {
        qDebug() << "图片压缩返回的百分比值：" << percent;
        m_saveimagecompmap[m_curcompimagekey] = percent;
    });
}

// 摄像头开关
void MainWindow::on_cameraToggleButton_clicked()
{
    if (m_isCameraOpened) {//在开启摄像头的情况下操作时，当前操作就是关闭摄像头
        closeCamera();
        ui->captureBtn->setEnabled(false);
        ui->delaycaptureBtn->setEnabled(false);
        ui->changecameraButton->setEnabled(false);
        ui->turnleftButton->setEnabled(false);
        ui->turnrightButton->setEnabled(false);
    } else {
        m_displayRotation = 0;//摄像头角度清空
        ui->videoLabel->setRotation(m_displayRotation);
        openCamera();
        ui->captureBtn->setEnabled(true);
        ui->delaycaptureBtn->setEnabled(true);
        ui->changecameraButton->setEnabled(true);
        ui->turnleftButton->setEnabled(true);
        ui->turnrightButton->setEnabled(true);
        QString curcameraname = ui->cameraComboBox->currentText();
        if (!curcameraname.contains("主摄像头")) {
            ui->changecameraButton->setText("切换主摄像头");
        } else {
            ui->changecameraButton->setText("切换副摄像头");
        }
    }
}

void MainWindow::openCamera()
{
    // ---------- 校验 ----------
    if (m_isCameraOpened) return;
    if (ui->cameraComboBox->currentIndex() < 0 || m_lastCameraList.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择摄像头");
        return;
    }
    if (ui->resolutionComboBox->currentIndex() < 0) {
        QMessageBox::warning(this, "错误", "请先选择分辨率");
        return;
    }
    // 从下拉框获取选中的分辨率信息
    QVariant var = ui->resolutionComboBox->currentData();
    CameraCapability selectedInfo;
    if (var.isValid()) {
        selectedInfo = var.value<CameraCapability>();
    } else {
        QMessageBox::warning(this, "错误", "无法获取分辨率");
        return;
    }
    QString resolution = QString("%1x%2").arg(selectedInfo.resolution.width())
                                         .arg(selectedInfo.resolution.height());
    QString cameraName = ui->cameraComboBox->currentText();
    if (cameraName.contains("】")) {
        QStringList parts = cameraName.split("】");
        if (parts.size() > 1) cameraName = parts.last();
    }
    QString deviceName = m_cameradestodevnamemap[cameraName];
    if (deviceName != "") cameraName = deviceName;
    // Windows DShow的pnp路径怪字符处理（原版逻辑）
    if (deviceName.contains("device:pnp:")) deviceName.replace("device:pnp:", "device_pnp_");
    if (deviceName != "") cameraName = deviceName;

    QApplication::processEvents();
    qDebug() << __LINE__ << cameraName << resolution << selectedInfo.codec << selectedInfo.maxFps;

    closeCamera();   // 防御：清理可能的残留资源（幂等）

    // ---------- 采集线程：接管原openCameraWithBestParams + updateFrame的读取 ----------
    m_capThread = new QThread(this);
    m_capWorker = new CaptureWorker();
    m_capWorker->moveToThread(m_capThread);
    ui->videoLabel->setMailbox(m_capWorker->glMailbox());

    CaptureWorker *w = m_capWorker;
    const QString dev = cameraName;
    const QString res = resolution;
    const QString cd  = selectedInfo.codec;
    const double  fps = selectedInfo.maxFps;

    connect(m_capThread, &QThread::started, w,
            [w, dev, res, cd, fps]() { w->start(dev, res, cd, fps); });
    connect(w, &CaptureWorker::opened, this, [this]() {
        qDebug() << "摄像头打开成功（采集线程）";
    }, Qt::QueuedConnection);
    connect(w, &CaptureWorker::errorOccurred, this, [this](const QString &e) {
        QMessageBox::warning(this, "错误", e);
        closeCamera();
    }, Qt::QueuedConnection);
    connect(w, &CaptureWorker::statsUpdated, this, [this](double fps, const QString &codec) {
        ui->fpsLabel->setText(QString("FPS: %1 | 编码: %2").arg(fps, 0, 'f', 1).arg(codec));
    }, Qt::QueuedConnection);

    m_capThread->start();

    //检测轮廓线程
    m_detThread = new QThread(this);
    m_detWorker = new DetectWorker();
    m_detWorker->moveToThread(m_detThread);
    m_detWorker->setMailbox(m_capWorker->detMailbox());
    m_detWorker->setIntervalMs(150);   // ~7fps，UI无感，算法零改动

    DetectWorker *dw = m_detWorker;
    connect(m_detThread, &QThread::started, dw, &DetectWorker::startLoop);
    connect(dw, &DetectWorker::contourReady, this, [this](const QPolygonF &poly) {
        if (isAutoCropMode) ui->videoLabel->setContour(poly);   // 视频坐标,GL矩阵自动贴合
    }, Qt::QueuedConnection);

    m_detThread->start();

    // ---------- UI状态 ----------
    m_isCameraOpened = true;
    updateCameraButtonState();
    ui->cameraComboBox->setEnabled(false);
    ui->resolutionComboBox->setEnabled(false);
    qDebug() << "摄像头启动:" << cameraName << resolution << cd << fps;
}

void MainWindow::closeCamera()
{
    if (!m_capWorker && !m_detWorker) {
        // 未开启过也允许调用（幂等），但仍需复位UI
    }

    // 1.先断开渲染邮箱，保证GL不再触碰即将销毁的mailbox
    ui->videoLabel->setMailbox(nullptr);

    // 2.停检测线程
    if (m_detWorker)
        QMetaObject::invokeMethod(m_detWorker, "stopLoop", Qt::QueuedConnection);
    if (m_detThread) { m_detThread->quit(); m_detThread->wait(2000); }
    delete m_detWorker;  m_detWorker = nullptr;
    delete m_detThread;  m_detThread = nullptr;

    // 3.停采集线程（原子标志+中断回调，av_read_frame一帧内退出）
    if (m_capWorker) m_capWorker->stop();
    if (m_capThread) { m_capThread->quit(); m_capThread->wait(3000); }
    delete m_capWorker;  m_capWorker = nullptr;
    delete m_capThread;  m_capThread = nullptr;

    m_isCameraOpened = false;
    m_manualRectVideo = QRectF();
    updateCameraButtonState();
    ui->cameraComboBox->setEnabled(true);
    ui->resolutionComboBox->setEnabled(true);
    ui->videoLabel->clearFrame();
    ui->fpsLabel->setText("FPS: 0.00 | 状态: 已关闭");
    qDebug() << "摄像头已关闭";
}

void MainWindow::updateCameraButtonState()
{
    if (m_isCameraOpened) closecameraStyleSheet();
    else opencameraStyleSheet();
}

// ============================================================================
// 裁边模式切换
// ============================================================================
void MainWindow::onAutoCropToggled(bool checked)
{
    if (checked) {
        isAutoCropMode = true;
        ui->videoLabel->setCursor(Qt::ArrowCursor);
        clearManualRect();
        qDebug() << "切换到自动切边模式";

        QString cameraName = ui->cameraComboBox->currentText();
        if (cameraName.contains("】")) {
            QStringList parts = cameraName.split("】");
            if (parts.size() > 1) cameraName = parts.last();
        }
        deleteManualCroptoconf(cameraName);
    }
}

void MainWindow::onManualCropToggled(bool checked)
{
    ui->videoLabel->setContour(QPolygonF());

    if (checked) {
        isAutoCropMode = false;
        ui->videoLabel->setCursor(Qt::CrossCursor);
        qDebug() << "切换到手动切边模式";

        QString cameraName = ui->cameraComboBox->currentText();
        if (cameraName.contains("】")) {
            QStringList parts = cameraName.split("】");
            if (parts.size() > 1) cameraName = parts.last();
        }
        writeManualCroptoconf(cameraName);
    }
}

void MainWindow::clearManualRect()
{
    m_manualDragging = false;
    m_manualRectVideo = QRectF();
    ui->videoLabel->setDragRect(QRect(), false);
    ui->videoLabel->setManualRect(QRectF());
}

// ============================================================================
// 事件过滤：滚轮缩放 / 手动矩形（坐标经widgetToVideo反算到视频坐标系）
// ============================================================================
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->videoLabel) {
        if (m_isCameraOpened == true) {
            // 滚轮缩放：所有模式启用（GL内部clamp 1.0~3.0）
            if (event->type() == QEvent::Wheel) {
                QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
                const double factor = wheelEvent->angleDelta().y() > 0 ? 1.1 : 0.9;
                ui->videoLabel->zoomBy(factor);
                return true;
            }
            // 鼠标拖框：仅手动模式
            if (!isAutoCropMode) {
                switch (event->type()) {
                case QEvent::MouseButtonPress:
                    handleMousePressEvent(static_cast<QMouseEvent*>(event));
                    return true;
                case QEvent::MouseMove:
                    handleMouseMoveEvent(static_cast<QMouseEvent*>(event));
                    return true;
                case QEvent::MouseButtonRelease:
                    handleMouseReleaseEvent(static_cast<QMouseEvent*>(event));
                    return true;
                default: break;
                }
            }
        }
    } else if (obj == ui->label_cameraconf) {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                qDebug() << "Label clicked==>label_cameraconf !";
                if (m_lastCameraList.count() > 0) {
                    m_confdialog->setModal(false);
                    m_confdialog->setWindowTitle("主/副摄像头配置");
                    m_confdialog->init(m_lastCameraList, m_mainCameraindex, m_subCameraindex);
                    m_confdialog->open();
                }
                return true;
            }
        }
    } else if (obj == ui->label_filesavepath) {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                qDebug() << "Label clicked==>label_filesavepath !";
                QString dirpath = ui->label_filesavepath->text();
                QDir dir(dirpath);
                if (!dir.exists()) {
                    QMessageBox::warning(nullptr, "错误", "目录不存在: " + dirpath);
                    return true;
                }
                QString localPath = QDir::toNativeSeparators(dirpath);
                bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
                if (!success) QMessageBox::warning(nullptr, "错误", "无法打开目录: " + dirpath);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    if (!isAutoCropMode && !m_manualRectVideo.isEmpty()) {
        QMenu menu(this);
        QAction *clearAction = menu.addAction("清除矩形");
        QAction *selected = menu.exec(event->globalPos());
        if (selected == clearAction) clearManualRect();
    }
}

void MainWindow::handleMousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_manualDragging = true;
        m_manualDragStart = event->pos();
        ui->videoLabel->setDragRect(QRect(), false);
        qDebug() << "手动模式：开始绘制矩形，起点:" << m_manualDragStart;
    } else if (event->button() == Qt::RightButton) {
        clearManualRect();
        qDebug() << "手动模式：清除矩形";
    }
}

void MainWindow::handleMouseMoveEvent(QMouseEvent *event)
{
    if (m_manualDragging)
        ui->videoLabel->setDragRect(QRect(m_manualDragStart, event->pos()).normalized(), true);
}

void MainWindow::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_manualDragging) return;
    m_manualDragging = false;
    ui->videoLabel->setDragRect(QRect(), false);

    const QRect widgetRect = QRect(m_manualDragStart, event->pos()).normalized();
    if (widgetRect.width() < 20 || widgetRect.height() < 20) {
        clearManualRect();
        qDebug() << "手动模式：矩形太小，已清空";
        return;
    }

    // 核心：widget坐标 → 视频像素坐标（替代原来80行的手工换算）
    const QPointF v1 = ui->videoLabel->widgetToVideo(m_manualDragStart);
    const QPointF v2 = ui->videoLabel->widgetToVideo(event->pos());
    m_manualRectVideo = QRectF(v1, v2).normalized();
    ui->videoLabel->setManualRect(m_manualRectVideo);
    qDebug() << "手动矩形完成(视频坐标):" << m_manualRectVideo;
}

// ============================================================================
// 拍照（数据源从latestHighResFrame换成GL回读，业务逻辑不变）
// ============================================================================
void MainWindow::on_captureBtn_clicked()
{
    if (!m_isCameraOpened) return;

    if (isAutoCropMode) {
        // 原始传感器方向帧
        cv::Mat frame = ui->videoLabel->grabFrameMat(false);
        if (frame.empty()) {
            ui->fpsLabel->setText("FPS: -- | 提醒: 暂无图像可保存！");
            return;
        }
        saveAutoCroppedImage(frame);
    } else {
        cv::Mat raw = ui->videoLabel->grabFrameMat(false);
        if (raw.empty()) {
            qDebug() << "暂无图像可保存！";
            return;
        }
        if (!m_manualRectVideo.isEmpty()) {
            saveManualCroppedImage(raw);
        } else {
            cv::Mat rotatedFrame = rotateMatByDegrees(raw, m_displayRotation);
            int num = m_saveimagemap.size() + 1;
            QString mainid = QString("%1").arg(num, 3, 10, QChar('0'));
            QPixmap pixmap = cvMatToQPixmap(rotatedFrame);
            ui->imageListWidget->addPixmap(pixmap, mainid);
            stImgMsg st_imgmsg;
            st_imgmsg.name = mainid;
            st_imgmsg.pixmap = pixmap;
            m_saveimagemap[mainid] = st_imgmsg;
        }
    }
}

void MainWindow::saveManualCroppedImage(cv::Mat &rawFrame)
{
    if (rawFrame.empty() || m_manualRectVideo.isEmpty()) return;

    // 视频坐标矩形直接裁剪原帧，再按显示方向旋转
    QRectF rc = m_manualRectVideo;
    cv::Rect r(int(rc.x() + 0.5), int(rc.y() + 0.5),
               int(rc.width() + 0.5), int(rc.height() + 0.5));
    r &= cv::Rect(0, 0, rawFrame.cols, rawFrame.rows);
    if (r.width <= 2 || r.height <= 2) {
        QMessageBox::warning(this, "错误", "裁剪区域无效！");
        return;
    }

    cv::Mat cropped = rawFrame(r);
    cv::Mat rotated = rotateMatByDegrees(cropped, m_displayRotation);

    int num = m_saveimagemap.size() + 1;
    QString mainid = QString("main_%1").arg(num, 3, 10, QChar('0'));
    QPixmap pixmap = cvMatToQPixmap(rotated);
    ui->imageListWidget->addPixmap(pixmap, mainid);
    stImgMsg st_imgmsg;
    st_imgmsg.name = mainid;
    st_imgmsg.pixmap = pixmap;
    m_saveimagemap[mainid] = st_imgmsg;
}

void MainWindow::saveAutoCroppedImage(cv::Mat &dealResFrame)
{
    // ===== 与旧版逐行一致：detectContour → reorder → 透视变换 =====
    dealContours detector;
    std::vector<cv::Point> contourPoints = m_dealContours->detectContour(dealResFrame);
    if (contourPoints.size() != 4) {
        ui->fpsLabel->setText("FPS: 0.00 | 提醒: 未检测到有效矩形区域，无法自动裁边！");
        return;
    }
    cout << "points:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "  (" << contourPoints[i].x << ", " << contourPoints[i].y << ")" << endl;
    }
    // 优化：重新排序点，确保得到边长高短的矩形
    reorderPointsForWideRectangle(contourPoints);
    // 构建 srcPoints（按顺序：左上、右上、右下、左下）
    std::vector<cv::Point2f> srcPoints(contourPoints.begin(), contourPoints.end());
    // 计算目标矩形尺寸
    float width1 = cv::norm(srcPoints[0] - srcPoints[1]);
    float width2 = cv::norm(srcPoints[2] - srcPoints[3]);
    float maxWidth = std::max(width1, width2);
    float height1 = cv::norm(srcPoints[0] - srcPoints[3]);
    float height2 = cv::norm(srcPoints[1] - srcPoints[2]);
    float maxHeight = std::max(height1, height2);
    // 目标四边形
    std::vector<cv::Point2f> dstPoints = {
        cv::Point2f(0, 0),
        cv::Point2f(maxWidth - 1, 0),
        cv::Point2f(maxWidth - 1, maxHeight - 1),
        cv::Point2f(0, maxHeight - 1)
    };
    // 透视变换
    cv::Mat transform = cv::getPerspectiveTransform(srcPoints, dstPoints);
    cv::Mat cropped;
    cv::warpPerspective(dealResFrame, cropped, transform,
                        cv::Size((int)maxWidth, (int)maxHeight), cv::INTER_CUBIC);

    int num = m_saveimagemap.size() + 1;
    QString mainid = QString("main_%1").arg(num, 3, 10, QChar('0'));
    //转换格式，并且把图片方式到imageListWidget中，用于制作PDF文件
    QPixmap pixmap = cvMatToQPixmap(cropped);
    if (m_displayRotation != 0) {//看看是否有旋转要求
        QPixmap rotatedPixmap = rotatePixmap(pixmap, m_displayRotation);
        pixmap = rotatedPixmap;
        qDebug() << __FUNCTION__ << m_displayRotation;
    }
    ui->imageListWidget->addPixmap(pixmap, mainid);
    stImgMsg st_imgmsg;
    st_imgmsg.name = mainid;
    st_imgmsg.pixmap = pixmap;
    m_saveimagemap[mainid] = st_imgmsg;
}

float MainWindow::calculateDistance(const cv::Point& p1, const cv::Point& p2)
{
    float dx = static_cast<float>(p2.x - p1.x);
    float dy = static_cast<float>(p2.y - p1.y);
    return std::sqrt(dx * dx + dy * dy);
}

void MainWindow::reorderPointsForWideRectangle(std::vector<cv::Point>& contourPoints)
{
    if (contourPoints.size() != 4) return;

    float len1 = calculateDistance(contourPoints[0], contourPoints[1]);
    float len2 = calculateDistance(contourPoints[1], contourPoints[2]);
    std::cout << "Original lengths: len1=" << len1 << ", len2=" << len2 << std::endl;

    if (len2 > len1) {
        cv::Point firstPoint = contourPoints[0];
        for (int i = 0; i < 3; i++) contourPoints[i] = contourPoints[i + 1];
        contourPoints[3] = firstPoint;
        std::cout << "Points reordered: len2 > len1, moved first point to end" << std::endl;
        len1 = calculateDistance(contourPoints[0], contourPoints[1]);
        len2 = calculateDistance(contourPoints[1], contourPoints[2]);
        std::cout << "New lengths: len1=" << len1 << ", len2=" << len2 << std::endl;
    } else {
        std::cout << "Points order is correct: len1 >= len2" << std::endl;
    }
}

void MainWindow::saveconfmsg(const QString& flag, const QString& value)
{
    QSettings settings(CONFFILE, QSettings::IniFormat);
    settings.setValue(flag, value);
}

// ============================================================================
// 格式转换（原有）
// ============================================================================
QPixmap MainWindow::cvMatToQPixmap(const cv::Mat &mat)
{
    if (mat.empty()) return QPixmap();
    switch (mat.type()) {
    case CV_8UC1: {
        QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);
        return QPixmap::fromImage(image);
    }
    case CV_8UC3: {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        QImage image(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
        return QPixmap::fromImage(image.copy());
    }
    case CV_8UC4: {
        cv::Mat rgba;
        cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
        QImage image(rgba.data, rgba.cols, rgba.rows, static_cast<int>(rgba.step), QImage::Format_RGBA8888);
        return QPixmap::fromImage(image.copy());
    }
    default: {
        cv::Mat temp;
        mat.convertTo(temp, CV_8UC3);
        return cvMatToQPixmap(temp);
    }
    }
}

// ============================================================================
// 图片列表操作（原有）
// ============================================================================
void MainWindow::moveUpImage()
{
    int currentRow = ui->imageListWidget->currentRow();
    if (currentRow > 0) {
        QListWidgetItem *currentItem = ui->imageListWidget->takeItem(currentRow);
        ui->imageListWidget->insertItem(currentRow - 1, currentItem);
        ui->imageListWidget->setCurrentRow(currentRow - 1);
        emit ui->imageListWidget->imageOrderChanged();
    }
}

void MainWindow::moveDownImage()
{
    int currentRow = ui->imageListWidget->currentRow();
    if (currentRow >= 0 && currentRow < ui->imageListWidget->count() - 1) {
        QListWidgetItem *currentItem = ui->imageListWidget->takeItem(currentRow);
        ui->imageListWidget->insertItem(currentRow + 1, currentItem);
        ui->imageListWidget->setCurrentRow(currentRow + 1);
        emit ui->imageListWidget->imageOrderChanged();
    }
}

void MainWindow::openfilesavepath(const QString& filedir)
{
    QString path = m_filesavepath;
    if (filedir != "") path = filedir;
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::on_clearButton_clicked()
{
    ui->imageListWidget->clearAllImages();
}

QString MainWindow::GetPrefixName()
{
    bool ok;
    QString prefix_name = QInputDialog::getText(this, "文件前缀名称", "请输入文件前缀名称:",
                                                QLineEdit::Normal, "", &ok);
    if (ok && !prefix_name.isEmpty()) {
        qDebug() << "文件名称前缀: " << prefix_name;
    } else {
        return "";
    }
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1/%2_%3").arg(m_filesavepath).arg(prefix_name).arg(timestamp);
}

QString MainWindow::GetAskName()
{
    bool ok;
    QString prefix_name = QInputDialog::getText(this, "文件名称", "请输入文件名称:",
                                                QLineEdit::Normal, "", &ok);
    if (ok && !prefix_name.isEmpty()) {
        qDebug() << "文件名称前缀: " << prefix_name;
        return QString("%1/%2").arg(m_filesavepath).arg(prefix_name);
    } else {
        return "";
    }
}

// ============================================================================
// PDF生成（原有，零改动）
// ============================================================================
void MainWindow::on_generatePdfButton_clicked()
{
    QStringList imageKeys = ui->imageListWidget->getImageKeys();
    if (imageKeys.size() < 1) {
        QMessageBox::warning(this, "提示", "没有图片可合并！");
        return;
    }
    if (imageKeys.size() == 1) {
        on_generatemorePdfButton_clicked();
        return;
    }
    closeCamera();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    if (ui->imageListWidget->imageCount() == 0) {
        QMessageBox::information(this, "提示", "预览框中没有图片项！");
        return;
    }

    QString public_pdffileName = QString("%1/%2_%3").arg(m_filesavepath).arg(m_onepdf_pre_name).arg(timestamp);
    if (m_onepdf_pdfname_type == "0") {
        public_pdffileName = GetAskName();
    } else {
        if (m_onepdf_pre_type == "0") public_pdffileName = GetPrefixName();
    }
    if (public_pdffileName == "") return;
    QString pdffileName = public_pdffileName + ".pdf";

    bool success = generatePdf(pdffileName);
    if (success) {
        QString openddir = QFileInfo(pdffileName).absolutePath();
        openfilesavepath(openddir);
    } else {
        QMessageBox::critical(this, "错误", "PDF生成失败，请检查文件路径和权限");
    }
}

bool MainWindow::generatePdf(const QString &filePath)
{
    try {
        QStringList imageKeys = ui->imageListWidget->getImageKeys();
        if (imageKeys.isEmpty()) return false;

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        printer.setPageSize(QPageSize(QPageSize::A4));
        printer.setPageOrientation(QPageLayout::Portrait);
        printer.setFullPage(false);

        QPainter painter;
        if (!painter.begin(&printer)) return false;

        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        const int horizontalMargin = 50;
        const int verticalMargin = 50;
        const int imagesPerPage = m_pdf_pagenum;
        const int verticalSpacing = 30;

        QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);
        QRectF contentRect = pageRect.adjusted(horizontalMargin, verticalMargin,
                                               -horizontalMargin, -verticalMargin);
        const qreal imageAreaHeight = (contentRect.height() - verticalSpacing) / imagesPerPage;
        int totalPages = (imageKeys.size() + imagesPerPage - 1) / imagesPerPage;
        int currentPage = 1;

        for (int i = 0; i < imageKeys.size(); ++i) {
            QString imagekey = imageKeys[i];
            QPixmap pixmap;
            if (m_saveimagemap.count(imagekey) > 0) {
                pixmap = m_saveimagemap[imagekey].pixmap;
            } else {
                QFileInfo fileInfo(imagekey);
                if (!fileInfo.exists()) continue;
                if (!pixmap.load(imagekey)) {
                    qDebug() << "无法加载图片:" << imagekey;
                    continue;
                }
            }
            if (pixmap.isNull()) continue;

            int positionInPage = i % imagesPerPage;
            if (positionInPage == 0 && i > 0) { printer.newPage(); currentPage++; }

            qreal y = contentRect.top() + positionInPage * (imageAreaHeight + verticalSpacing);
            double widthRatio = contentRect.width() / pixmap.width();
            double heightRatio = imageAreaHeight / pixmap.height();
            double scaleFactor = qMin(widthRatio, heightRatio);
            qreal scaledWidth = pixmap.width() * scaleFactor;
            qreal scaledHeight = pixmap.height() * scaleFactor;
            qreal x = contentRect.left() + (contentRect.width() - scaledWidth) / 2;

            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(250, 250, 250));
            painter.drawRect(QRectF(x - 5, y - 5, scaledWidth + 10, scaledHeight + 10));
            painter.restore();

            painter.drawPixmap(QRectF(x, y, scaledWidth, scaledHeight), pixmap,
                               QRectF(0, 0, pixmap.width(), pixmap.height()));

            painter.save();
            painter.setPen(QPen(QColor(200, 200, 200), 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(x, y, scaledWidth, scaledHeight));
            painter.restore();

            painter.save();
            painter.setPen(Qt::white);
            painter.setBrush(QColor(70, 130, 180));
            painter.drawRect(QRectF(x, y, 40, 25));
            painter.setPen(Qt::white);
            QFont indexFont = painter.font();
            indexFont.setBold(true);
            painter.setFont(indexFont);
            painter.drawText(QRectF(x, y, 40, 25), Qt::AlignCenter, QString::number(i + 1));

            QFileInfo fileInfo(imagekey);
            QString fileName = fileInfo.fileName();
            if (fileName.length() > 30) fileName = fileName.left(30) + "...";
            QFont infoFont = painter.font();
            infoFont.setPointSize(9);
            painter.setFont(infoFont);
            painter.setPen(QColor(80, 80, 80));
            painter.drawText(QRectF(x + 45, y, scaledWidth - 45, 15),
                             Qt::AlignLeft | Qt::AlignVCenter, fileName);
            QString sizeText = QString("%1 × %2").arg(pixmap.width()).arg(pixmap.height());
            painter.drawText(QRectF(x + 45, y + 15, scaledWidth - 45, 10),
                             Qt::AlignLeft | Qt::AlignVCenter, sizeText);
            painter.restore();

            if (positionInPage == 0 && i < imageKeys.size() - 1) {
                painter.save();
                painter.setPen(QPen(QColor(220, 220, 220), 1, Qt::DashLine));
                qreal lineY = y + scaledHeight + verticalSpacing / 2;
                painter.drawLine(QPointF(contentRect.left(), lineY),
                                 QPointF(contentRect.right(), lineY));
                painter.restore();
            }
        }

        painter.save();
        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);
        painter.setPen(QColor(120, 120, 120));
        QString pageInfo = QString("第 %1 页 / 共 %2 页").arg(currentPage).arg(totalPages);
        painter.drawText(QRectF(pageRect.left(), pageRect.bottom() - 30, pageRect.width(), 20),
                         Qt::AlignCenter, pageInfo);
        painter.restore();

        painter.end();
        qDebug() << "PDF生成成功:" << filePath;
        qDebug() << "图片总数:" << imageKeys.size();
        qDebug() << "总页数:" << totalPages;
        return true;
    } catch (const std::exception& e) {
        qDebug() << "PDF生成异常:" << e.what();
        return false;
    }
}

void MainWindow::on_generatemorePdfButton_clicked()
{
    closeCamera();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    if (ui->imageListWidget->imageCount() == 0) {
        QMessageBox::information(this, "提示", "预览框中没有图片项！");
        return;
    }

    QString public_pdffileName = QString("%1/%2_%3").arg(m_filesavepath).arg(m_morepdf_pre_name).arg(timestamp);
    if (m_morepdf_pre_type == "0") public_pdffileName = GetPrefixName();
    if (public_pdffileName == "") return;

    QMap<QString, QString> imagemap = ui->imageListWidget->getImageMaps();
    if (imagemap.isEmpty()) return;

    QList<QString> tmpimageNo;
    int success_num = 0;
    for (auto it = imagemap.constBegin(); it != imagemap.constEnd(); ++it) {
        QString imagekey = it.key();
        QString imageNo = it.value();
        qDebug() << "imageNo:" << imageNo << "imagekey:" << imagekey;
        tmpimageNo.append(imageNo);
        int num = tmpimageNo.count(imageNo);
        if (num > 1) imageNo = QString("%1_%2").arg(imageNo).arg(QString::number(num));

        QString filePath = QString("%1_%2.pdf").arg(public_pdffileName).arg(imageNo);
        QPixmap pixmap;
        if (m_saveimagemap.count(imagekey) > 0) {
            pixmap = m_saveimagemap[imagekey].pixmap;
        } else {
            QFileInfo fileInfo(imagekey);
            if (!fileInfo.exists()) continue;
            if (!pixmap.load(imagekey)) {
                qDebug() << "无法加载图片:" << imagekey;
                continue;
            }
        }

        QPixmap savepixmap = pixmap;
        int percent = 100;
        if (m_saveimagecompmap.count(imagekey)) {
            percent = m_saveimagecompmap[imagekey];
            int targetWidth = pixmap.width() * percent / 100;
            int targetHeight = pixmap.height() * percent / 100;
            savepixmap = pixmap.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
        }
        bool success = generatePdfbyimg(filePath, savepixmap);
        if (success) success_num++;
    }
    if (success_num > 0) openfilesavepath();
}

bool MainWindow::generatePdfbyimg(const QString &filePath, const QPixmap &pixmap)
{
    try {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        printer.setPageSize(QPageSize(QPageSize::A4));
        printer.setPageOrientation(QPageLayout::Portrait);
        printer.setFullPage(false);

        QPainter painter;
        if (!painter.begin(&printer)) return false;

        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::Antialiasing, true);

        int margin = 40;
        QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);
        QRectF contentRect = pageRect.adjusted(margin, margin, -margin, -margin);

        double widthRatio = contentRect.width() / pixmap.width();
        double heightRatio = contentRect.height() / pixmap.height();
        double scaleFactor = qMin(widthRatio, heightRatio);
        QSizeF scaledSize = pixmap.size() * scaleFactor;
        qreal x = contentRect.left() + (contentRect.width() - scaledSize.width()) / 2;
        qreal y = contentRect.top() + (contentRect.height() - scaledSize.height()) / 2;

        painter.drawPixmap(QRectF(x, y, scaledSize.width(), scaledSize.height()), pixmap,
                           QRectF(0, 0, pixmap.width(), pixmap.height()));
        painter.end();
        return true;
    } catch (const std::exception& e) {
        qDebug() << "PDF生成异常:" << e.what();
        return false;
    }
}

void MainWindow::on_generateIamgeButton_clicked()
{
    closeCamera();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    if (ui->imageListWidget->imageCount() == 0) {
        QMessageBox::information(this, "提示", "预览框中没有图片项！");
        return;
    }

    QString public_ImgfileName = QString("%1/%2_%3").arg(m_filesavepath).arg(m_moreimg_pre_name).arg(timestamp);
    if (m_moreimg_pre_type == "0") public_ImgfileName = GetPrefixName();
    if (public_ImgfileName == "") return;

    QMap<QString, QString> imagemap = ui->imageListWidget->getImageMaps();
    if (imagemap.isEmpty()) return;

    QList<QString> tmpimageNo;
    int success_num = 0;
    for (auto it = imagemap.constBegin(); it != imagemap.constEnd(); ++it) {
        QString imagekey = it.key();
        QString imageNo = it.value();
        qDebug() << "imageNo:" << imageNo << "imagekey:" << imagekey;
        tmpimageNo.append(imageNo);
        int num = tmpimageNo.count(imageNo);
        if (num > 1) imageNo = QString("%1_%2").arg(imageNo).arg(QString::number(num));

        QString filePath = QString("%1_%2.%3").arg(public_ImgfileName).arg(imageNo).arg(m_moreimg_format);
        QPixmap pixmap;
        if (m_saveimagemap.count(imagekey) > 0) {
            pixmap = m_saveimagemap[imagekey].pixmap;
        } else {
            QFileInfo fileInfo(imagekey);
            if (!fileInfo.exists()) continue;
            if (!pixmap.load(imagekey)) {
                qDebug() << "无法加载图片:" << imagekey;
                continue;
            }
        }

        int percent = 100;
        if (m_saveimagecompmap.count(imagekey)) percent = m_saveimagecompmap[imagekey];
        qDebug() << filePath << percent;
        bool success = m_comm->savePixmapToFilebypercent(filePath, pixmap, percent);
        if (success) success_num++;
    }
    if (success_num > 0) openfilesavepath();
}

// ============================================================================
// 拖拽事件（原有）
// ============================================================================
void MainWindow::dragEnterEvent(QDragEnterEvent *event) { qDebug() << __FUNCTION__ << __LINE__; }
void MainWindow::dragMoveEvent(QDragMoveEvent *event)   { qDebug() << __FUNCTION__ << __LINE__; }
void MainWindow::dropEvent(QDropEvent *event)           { qDebug() << __FUNCTION__ << __LINE__; }

// ============================================================================
// 图片预览/旋转（原有）
// ============================================================================
void MainWindow::previewImage()
{
    qDebug() << __FUNCTION__ << __LINE__;
    QList<QListWidgetItem*> selected = ui->imageListWidget->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一张图片！");
        return;
    }
    QListWidgetItem* item = selected.first();
    QString imagekey = item->data(Qt::UserRole).toString();

    if (!m_saveimagemap.contains(imagekey)) {
        if (imagekey.contains("main_") == false) {
            QFileInfo fileInfo(imagekey);
            if (!fileInfo.exists()) return;
            QPixmap pixmap;
            if (!pixmap.load(imagekey)) {
                return;
            } else {
                qDebug() << "这是拖拽进来的图片，新增一个pixmap";
                m_saveimagemap[imagekey].pixmap = pixmap;
            }
        } else return;
    }
    QPixmap originalPixmap = m_saveimagemap[imagekey].pixmap;
    ImagePreviewDialog *previewDialog = new ImagePreviewDialog(originalPixmap, this);
    previewDialog->setAttribute(Qt::WA_DeleteOnClose);
    previewDialog->exec();
}

void MainWindow::rotateLeftImage()  { rotateSelectedImage(-90); }
void MainWindow::rotateRightImage() { rotateSelectedImage(90); }

void MainWindow::rotateSelectedImage(int angle)
{
    QList<QListWidgetItem*> selectedItems = ui->imageListWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一张图片！");
        return;
    }
    for (QListWidgetItem *item : selectedItems) {
        QString imagekey = item->data(Qt::UserRole).toString();
        if (!m_saveimagemap.contains(imagekey)) {
            if (imagekey.contains("main_") == false) {
                QFileInfo fileInfo(imagekey);
                if (!fileInfo.exists()) return;
                QPixmap pixmap;
                if (!pixmap.load(imagekey)) {
                    return;
                } else {
                    qDebug() << "这是拖拽进来的图片，新增一个pixmap";
                    m_saveimagemap[imagekey].pixmap = pixmap;
                }
            } else return;
        }
        QPixmap originalPixmap = m_saveimagemap[imagekey].pixmap;
        QPixmap rotatedPixmap = rotatePixmap(originalPixmap, angle);
        if (rotatedPixmap.isNull()) return;

        QPixmap thumbnail = rotatedPixmap.scaled(ui->imageListWidget->iconSize(),
                                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
        item->setIcon(QIcon(thumbnail));
        m_saveimagemap[imagekey].pixmap = rotatedPixmap;
    }
}

QPixmap MainWindow::rotatePixmap(const QPixmap &pixmap, int angle)
{
    if (pixmap.isNull()) return QPixmap();
    angle = angle % 360;
    if (angle < 0) angle += 360;
    if (angle == 0) return pixmap.copy();
    else if (angle == 90)  return pixmap.transformed(QTransform().rotate(90));
    else if (angle == 180) return pixmap.transformed(QTransform().rotate(180));
    else if (angle == 270) return pixmap.transformed(QTransform().rotate(270));

    QTransform transform;
    transform.rotate(angle);
    return pixmap.transformed(transform, Qt::SmoothTransformation);
}

// ============================================================================
// 摄像头切换（原有）
// ============================================================================
void MainWindow::on_changecameraButton_clicked()
{
    qDebug() << __FUNCTION__ << m_mainCameraindex << m_subCameraindex;
    QString curbtnname = ui->changecameraButton->text();
    QString curcameraname;
    if (curbtnname == "切换主摄像头") {
        QString cameraname = ui->cameraComboBox->itemText(m_mainCameraindex);
        if (!cameraname.contains("主摄像头")) {
            QMessageBox::warning(this, "错误", "列表中没有主摄像头，请在工具栏中配置");
            return;
        }
        curcameraname = cameraname;
        ui->cameraComboBox->setCurrentIndex(m_mainCameraindex);
    } else {
        QString cameraname = ui->cameraComboBox->itemText(m_subCameraindex);
        if (!cameraname.contains("副摄像头")) {
            QMessageBox::warning(this, "错误", "列表中没有副摄像头，请在工具栏中配置");
            return;
        }
        curcameraname = cameraname;
        ui->cameraComboBox->setCurrentIndex(m_subCameraindex);
    }
    closeCamera();
    {
        QString cameraName = ui->cameraComboBox->currentText();
        if (cameraName.contains("】")) {
            QStringList parts = cameraName.split("】");
            if (parts.size() > 1) cameraName = parts.last();
        }
        updateManualCropbyconf(cameraName);
    }
    openCamera();
    if (!curcameraname.contains("主摄像头")) {
        ui->changecameraButton->setText("切换主摄像头");
    } else {
        ui->changecameraButton->setText("切换副摄像头");
    }
}

// ============================================================================
// 快捷键 / 激活UI（原有）
// ============================================================================
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        qDebug() << "Enter key pressed!";
        if (m_isCameraOpened) on_captureBtn_clicked();
    }
    QWidget::keyPressEvent(event);
}

void MainWindow::setactivationUI(bool flag)
{
    ui->cameraComboBox->setEnabled(flag);
    ui->resolutionComboBox->setEnabled(flag);
    ui->cameraToggleButton->setEnabled(flag);
    ui->generatePdfButton->setEnabled(flag);
    ui->generatemorePdfButton->setEnabled(flag);
}

void MainWindow::onImageDescriptionChanged(QListWidgetItem *item)
{
    if (!item) return;
    QString newDescription = item->text();
    QString filePath = item->data(Qt::UserRole).toString();
    qDebug() << "描述已更新:" << filePath << "=>" << newDescription;
}

void MainWindow::deleteSelectedImage()
{
    QStringList imageKeys = ui->imageListWidget->getImageKeys();
    if (imageKeys.isEmpty()) return;
    int result = QMessageBox::question(this, "确认删除", "确定要删除选中的图片吗？",
                                       QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) ui->imageListWidget->deleteSelectedImage();
}

void MainWindow::setupImageListShortKey()
{
    m_deleteImageAction = new QAction("删除图片", this);
    m_deleteImageAction->setShortcut(QKeySequence::Delete);
    this->addAction(m_deleteImageAction);
    connect(m_deleteImageAction, &QAction::triggered, this, &MainWindow::deleteSelectedImage);
}

void MainWindow::showImageListContextMenu(const QPoint &pos)
{
    QStringList selectedKeys = ui->imageListWidget->getSelectImageKeys();
    int count = selectedKeys.size();
    qDebug() << __FUNCTION__ << count;
    if (count == 0) return;

    LeftClickOnlyMenu contextMenu(this);
    if (count == 1) {
        contextMenu.addAction("上移", this, &MainWindow::moveUpImage);
        contextMenu.addAction("下移", this, &MainWindow::moveDownImage);
        contextMenu.addAction("删除选中图片", this, &MainWindow::deleteSelectedImage);
        contextMenu.addAction("顺时针转90°", this, &MainWindow::rotateRightImage);
        contextMenu.addAction("逆时针转90°", this, &MainWindow::rotateLeftImage);
        contextMenu.addAction("从电脑中载入图片到前面", this, &MainWindow::insertImageBeforeCur);
        contextMenu.addAction("压缩图像", this, &MainWindow::CompressImage);
    } else if (count > 1) {
        contextMenu.addAction("删除选中图片", this, &MainWindow::deleteSelectedImage);
        contextMenu.addAction("顺时针转90°", this, &MainWindow::rotateRightImage);
        contextMenu.addAction("逆时针转90°", this, &MainWindow::rotateLeftImage);
        contextMenu.addAction("合并所选择的图片并删除原图", this, &MainWindow::mrgimgdeletesrcimg);
        contextMenu.addAction("合并所选择的图片，保留原图", this, &MainWindow::mrgimgsavesrcimg);
    }
    QPoint globalPos = ui->imageListWidget->mapToGlobal(pos);
    contextMenu.exec(globalPos);
}

void MainWindow::insertImageBeforeCur()
{
    int currentRow = ui->imageListWidget->currentRow();
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this, "选择要插入的图片", m_imgpath, "Images (*.png *.jpg *.jpeg)");
    if (!filePaths.isEmpty()) ui->imageListWidget->insertImagesAt(filePaths, currentRow);
}

// ============================================================================
// 延时拍照（原有）
// ============================================================================
void MainWindow::Creatdelaycapture()
{
    m_countdownWidget = new CountdownWidget(ui->videoLabel);
    m_countdownWidget->resize(300, 200);
    m_countdownWidget->move((width() - m_countdownWidget->width()) / 2,
                            (height() - m_countdownWidget->height()) / 2);
    connect(m_countdownWidget, &CountdownWidget::finished, this, &MainWindow::onDelayTimeout);
}

void MainWindow::on_delaycaptureBtn_clicked()
{
    if (m_isCameraOpened == false) {
        QMessageBox::warning(this, "错误", "请先开启摄像头");
        return;
    }
    ui->delaycaptureBtn->setEnabled(false);
    if (m_countdownWidget->isVisible()) return;

    QSize labelSize = ui->videoLabel->size();
    QSize widgetSize = QSize(200, 200);
    m_countdownWidget->setFixedSize(widgetSize);
    int x = (labelSize.width() - widgetSize.width()) / 2;
    int y = (labelSize.height() - widgetSize.height()) / 2;
    qDebug() << x << y << widgetSize;
    m_countdownWidget->move(x, y);
    m_countdownWidget->start(5);
}

void MainWindow::onDelayTimeout()
{
    on_captureBtn_clicked();
    ui->delaycaptureBtn->setEnabled(true);
}

// ============================================================================
// 手动裁边配置（原有）
// ============================================================================
void MainWindow::updateManualCropbyconf(QString cameraName)
{
    bool bfind = manualcropfileop->findLines(cameraName);
    if (bfind == true) {
        qDebug() << cameraName << "存在手动裁边配置";
        m_lastmunualcrop_cameraname = cameraName;
        ui->radioManualCrop->setChecked(true);
    } else {
        ui->radioAutoCrop->setChecked(true);
    }
}

bool MainWindow::writeManualCroptoconf(QString cameraName)
{
    bool bfind = manualcropfileop->findLines(cameraName);
    if (bfind == true) return true;
    qDebug() << __FUNCTION__ << "新增手动裁边配置" << cameraName;
    return manualcropfileop->appendLine(cameraName);
}

bool MainWindow::deleteManualCroptoconf(QString cameraName)
{
    bool bfind = manualcropfileop->findLines(cameraName);
    if (bfind != true) return true;
    m_lastmunualcrop_cameraname = "";
    qDebug() << __FUNCTION__ << "删除手动裁边配置" << cameraName;
    return manualcropfileop->removeLinesContaining(cameraName);
}

void MainWindow::getconfnamerulemsg()
{
    QSettings settings(CONFFILE, QSettings::IniFormat);
    settings.beginGroup("NAMERULE");
    m_oneimg_format = settings.value("oneimg_format", "jpg").toString().toLower();
    m_oneimg_name_type = settings.value("oneimg_name_type", "1").toString();
    m_oneimg_pre_type = settings.value("oneimg_pre_type", "1").toString();
    m_oneimg_pre_name = settings.value("oneimg_pre_name", "IMG_").toString();
    m_moreimg_format = settings.value("moreimg_format", "jpg").toString().toLower();
    m_moreimg_pre_type = settings.value("moreimg_pre_type", "1").toString();
    m_moreimg_pre_name = settings.value("moreimg_pre_name", "IMG_").toString();
    m_onepdf_pdfname_type = settings.value("onepdf_pdfname_type", "1").toString();
    m_onepdf_pre_type = settings.value("onepdf_pre_type", "1").toString();
    m_onepdf_pre_name = settings.value("onepdf_pre_name", "IMG_").toString();
    m_morepdf_pre_type = settings.value("morepdf_pre_type", "1").toString();
    m_morepdf_pre_name = settings.value("morepdf_pre_name", "IMG_").toString();
    qDebug() << __FUNCTION__ << m_oneimg_format << m_moreimg_format;
}

// ============================================================================
// 预览旋转（改用GL矩阵，矩形不再丢失）
// ============================================================================
cv::Mat MainWindow::rotateMatByDegrees(const cv::Mat& src, int degrees) const
{
    if (src.empty()) return src;
    cv::Mat dst = src.clone();
    int rot = ((degrees % 360) + 360) % 360;
    switch (rot) {
    case 0: break;
    case 90:  cv::rotate(src, dst, cv::ROTATE_90_CLOCKWISE); break;
    case 180: cv::rotate(src, dst, cv::ROTATE_180); break;
    case 270: cv::rotate(src, dst, cv::ROTATE_90_COUNTERCLOCKWISE); break;
    default:
        qDebug() << "Warning: Rotation angle not multiple of 90, ignored.";
        break;
    }
    return dst;
}

void MainWindow::on_turnrightButton_clicked()
{
    m_displayRotation += m_turn_right;
    if (m_displayRotation == 360) m_displayRotation = 0;
    ui->videoLabel->setRotation(m_displayRotation);   // GL矩阵处理，绿框自动跟随
    qDebug() << __FUNCTION__ << m_displayRotation;
}

void MainWindow::on_turnleftButton_clicked()
{
    m_displayRotation += m_turn_left;
    if (m_displayRotation == -360) m_displayRotation = 0;
    ui->videoLabel->setRotation(m_displayRotation);
    qDebug() << __FUNCTION__ << m_displayRotation;
}

void MainWindow::on_generateOneIamgeButton_clicked()
{
    QStringList imageKeys = ui->imageListWidget->getImageKeys();
    if (imageKeys.size() < 1) {
        QMessageBox::warning(this, "提示", "没有图片可合并！");
        return;
    }
    if (imageKeys.size() == 1) {
        on_generateIamgeButton_clicked();
        return;
    }
    closeCamera();

    if (ui->imageListWidget->imageCount() == 0) {
        QMessageBox::information(this, "提示", "预览框中没有图片项！");
        return;
    }
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString public_ImgfileName = QString("%1/%2_%3").arg(m_filesavepath).arg(m_oneimg_pre_name).arg(timestamp);
    QString fileFormat = m_oneimg_format.toLower();

    if (m_oneimg_name_type == "0") {
        QString fileName = GetAskName();
        if (fileName == "") return;
        public_ImgfileName = fileName;
    } else {
        if (m_oneimg_pre_type == "0") {
            QString prefixName = GetPrefixName();
            if (prefixName == "") return;
            public_ImgfileName = prefixName;
        }
    }
    if (public_ImgfileName == "") return;
    QString filePath = QString("%1.%2").arg(public_ImgfileName).arg(fileFormat);

    QList<QPixmap> pixmaps;
    for (const QString &key : imageKeys) {
        if (m_saveimagemap.contains(key)) {
            pixmaps.append(m_saveimagemap[key].pixmap);
        } else {
            QPixmap tmp;
            if (tmp.load(key)) pixmaps.append(tmp);
        }
    }
    if (pixmaps.size() < 1) {
        QMessageBox::warning(this, "错误", "无法加载图片数据。");
        return;
    }

    int maxCols = m_mergeimagenum;
    int targetWidth = 0;
    for (const QPixmap &p : pixmaps) targetWidth = qMax(targetWidth, p.width());

    struct RowInfo { int scaledHeight; int totalWidth; };
    QList<RowInfo> rows;
    int totalHeight = 0;
    for (int i = 0; i < pixmaps.size(); i += maxCols) {
        RowInfo row;
        row.scaledHeight = 0;
        row.totalWidth = 0;
        int actualCols = qMin(maxCols, pixmaps.size() - i);
        for (int j = 0; j < actualCols; ++j) {
            const QPixmap &p = pixmaps[i + j];
            int newHeight = (p.height() * targetWidth) / p.width();
            row.scaledHeight = qMax(row.scaledHeight, newHeight);
            row.totalWidth += targetWidth;
        }
        rows.append(row);
        totalHeight += row.scaledHeight;
    }

    int canvasWidth = maxCols * targetWidth;
    QPixmap resultPixmap(canvasWidth, totalHeight);
    resultPixmap.fill(Qt::white);
    QPainter painter(&resultPixmap);
    int currentY = 0;
    for (int i = 0; i < pixmaps.size(); i += maxCols) {
        int actualCols = qMin(maxCols, pixmaps.size() - i);
        RowInfo currentRow = rows[i / maxCols];
        int startX = 0;
        if (actualCols < maxCols) {
            int rowRealWidth = actualCols * targetWidth;
            startX = (canvasWidth - rowRealWidth) / 2;
        }
        int currentX = startX;
        for (int j = 0; j < actualCols; ++j) {
            const QPixmap &p = pixmaps[i + j];
            int scaledHeight = (p.height() * targetWidth) / p.width();
            QRect targetRect(currentX, currentY, targetWidth, scaledHeight);
            painter.drawPixmap(targetRect, p);
            currentX += targetWidth;
        }
        currentY += currentRow.scaledHeight;
    }
    painter.end();

    bool success = m_comm->savePixmapToFile(filePath, resultPixmap);
    if (success) {
        QMessageBox::information(this, "成功",
            QString("已合并 %1 张图片并保存为: %2").arg(pixmaps.size()).arg(QFileInfo(filePath).fileName()));
        openfilesavepath(QFileInfo(filePath).absolutePath());
    } else {
        QMessageBox::critical(this, "错误", "保存合并图片失败，请检查文件路径和权限");
    }
}

// ============================================================================
// 图片合并（原有）
// ============================================================================
void MainWindow::mrgimgdeletesrcimg() { wgmergeSelectedImages(true); }
void MainWindow::mrgimgsavesrcimg()   { wgmergeSelectedImages(false); }

void MainWindow::mergeSelectedImages(bool deleteOriginals)
{
    QStringList selectedKeys = ui->imageListWidget->getSelectImageKeys();
    if (selectedKeys.size() < 2) {
        QMessageBox::warning(this, "提示", "请至少选中2张图片进行合并。");
        return;
    }
    QList<QPixmap> pixmaps;
    for (const QString &key : selectedKeys) {
        if (m_saveimagemap.contains(key)) {
            pixmaps.append(m_saveimagemap[key].pixmap);
        } else {
            QPixmap tmp;
            if (tmp.load(key)) pixmaps.append(tmp);
        }
    }
    if (pixmaps.size() < 2) {
        QMessageBox::warning(this, "错误", "无法加载选中的图片数据。");
        return;
    }

    int maxWidth = 0;
    int totalHeight = 0;
    for (const QPixmap &pixmap : pixmaps) {
        maxWidth = qMax(maxWidth, pixmap.width());
        totalHeight += pixmap.height();
    }

    QPixmap resultPixmap(maxWidth, totalHeight);
    resultPixmap.fill(Qt::white);
    QPainter painter(&resultPixmap);
    int currentY = 0;
    for (const QPixmap &pixmap : pixmaps) {
        int x = (maxWidth - pixmap.width()) / 2;
        painter.drawPixmap(x, currentY, pixmap);
        currentY += pixmap.height();
    }
    painter.end();

    int num = m_saveimagemap.size() + 1;
    QString newId = QString("main_%1").arg(num, 3, 10, QChar('0'));
    stImgMsg st_imgmsg;
    st_imgmsg.name = newId;
    st_imgmsg.pixmap = resultPixmap;
    m_saveimagemap[newId] = st_imgmsg;
    ui->imageListWidget->addPixmap(resultPixmap, newId);

    if (deleteOriginals) {
        ui->imageListWidget->deleteSelectedImage();
        QMessageBox::information(this, "完成", QString("合并完成，并删除了 %1 张原图。").arg(selectedKeys.size()));
    } else {
        QMessageBox::information(this, "完成", QString("合并完成，共合并 %1 张图片。").arg(selectedKeys.size()));
    }
}

void MainWindow::wgmergeSelectedImages(bool deleteOriginals)
{
    QStringList selectedKeys = ui->imageListWidget->getSelectImageKeys();
    if (selectedKeys.size() < 2) {
        QMessageBox::warning(this, "提示", "请至少选中2张图片进行合并。");
        return;
    }
    QList<QPixmap> pixmaps;
    for (const QString &key : selectedKeys) {
        if (m_saveimagemap.contains(key)) {
            pixmaps.append(m_saveimagemap[key].pixmap);
        } else {
            QPixmap tmp;
            if (tmp.load(key)) pixmaps.append(tmp);
        }
    }
    if (pixmaps.size() < 2) {
        QMessageBox::warning(this, "错误", "无法加载选中的图片数据。");
        return;
    }

    int maxCols = m_mergeimagenum;
    int targetWidth = 0;
    for (const QPixmap &p : pixmaps) targetWidth = qMax(targetWidth, p.width());

    struct RowInfo { int scaledHeight; int totalWidth; };
    QList<RowInfo> rows;
    int totalHeight = 0;
    for (int i = 0; i < pixmaps.size(); i += maxCols) {
        RowInfo row;
        row.scaledHeight = 0;
        row.totalWidth = 0;
        int actualCols = qMin(maxCols, pixmaps.size() - i);
        for (int j = 0; j < actualCols; ++j) {
            const QPixmap &p = pixmaps[i + j];
            int newHeight = (p.height() * targetWidth) / p.width();
            row.scaledHeight = qMax(row.scaledHeight, newHeight);
            row.totalWidth += targetWidth;
        }
        rows.append(row);
        totalHeight += row.scaledHeight;
    }

    int canvasWidth = maxCols * targetWidth;
    QPixmap resultPixmap(canvasWidth, totalHeight);
    resultPixmap.fill(Qt::white);
    QPainter painter(&resultPixmap);
    int currentY = 0;
    for (int i = 0; i < pixmaps.size(); i += maxCols) {
        int actualCols = qMin(maxCols, pixmaps.size() - i);
        RowInfo currentRow = rows[i / maxCols];
        int startX = 0;
        if (actualCols < maxCols) {
            int rowRealWidth = actualCols * targetWidth;
            startX = (canvasWidth - rowRealWidth) / 2;
        }
        int currentX = startX;
        for (int j = 0; j < actualCols; ++j) {
            const QPixmap &p = pixmaps[i + j];
            int scaledHeight = (p.height() * targetWidth) / p.width();
            QRect targetRect(currentX, currentY, targetWidth, scaledHeight);
            painter.drawPixmap(targetRect, p);
            currentX += targetWidth;
        }
        currentY += currentRow.scaledHeight;
    }
    painter.end();

    int num = m_saveimagemap.size() + 1;
    QString newId = QString("main_%1").arg(num, 3, 10, QChar('0'));
    stImgMsg st_imgmsg;
    st_imgmsg.name = newId;
    st_imgmsg.pixmap = resultPixmap;
    m_saveimagemap[newId] = st_imgmsg;
    ui->imageListWidget->addPixmap(resultPixmap, newId);

    if (deleteOriginals) ui->imageListWidget->deleteSelectedImage();
}

// ============================================================================
// 窗口尺寸自适应（保留adapt系列，删除轮廓重映射与applyZoom——GL自动处理）
// ============================================================================
void MainWindow::adaptWidgetPosX(QWidget* widget, int src_X, int adapt_width)
{
    if (!widget) return;
    widget->setGeometry(src_X + (adapt_width > 0 ? adapt_width : 0),
                        widget->y(), widget->width(), widget->height());
}

void MainWindow::adaptWidgetPosXHeight(QWidget* widget, int src_X, int src_h, int adapt_width, int adapt_height)
{
    if (!widget) return;
    widget->setGeometry(src_X + (adapt_width > 0 ? adapt_width : 0),
                        widget->y(), widget->width(), src_h + adapt_height);
}

void MainWindow::adaptWidgetPosY(QWidget* widget, int adapt_height, int src_Y)
{
    if (!widget) return;
    widget->setGeometry(widget->x(), src_Y + (adapt_height > 0 ? adapt_height : 0),
                        widget->width(), widget->height());
}

void MainWindow::adaptWidgetPosXY(QWidget* widget, int src_w, int src_h, int adapt_width, int adapt_height)
{
    if (!widget) return;
    widget->setGeometry(widget->x(), widget->y(), src_w + adapt_width, src_h + adapt_height);
}

void MainWindow::adaptWidgetWH(QWidget* widget, int src_X, int src_Y, int src_w, int src_h, int adapt_width, int adapt_height)
{
    if (!widget) return;
    widget->setGeometry(src_X + adapt_width, src_Y + adapt_height, src_w, src_h);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    int adapt_width = width() - m_srcwidth;
    int adapt_height = height() - m_srcheight;

    adaptWidgetPosXHeight(ui->line, m_line_posx, m_line_height, adapt_width, adapt_height);
    adaptWidgetPosXHeight(ui->imgGroup, m_imgGroup_posx, m_imgGroup_height, adapt_width, adapt_height);
    adaptWidgetPosX(ui->saveGroup, m_saveGroup_posx, adapt_width);
    adaptWidgetPosY(ui->fpsLabel, adapt_height, m_fpsLabel_posy);
    adaptWidgetPosXY(ui->imageListWidget, m_imageList_width, m_imageList_height, 0, adapt_height);
    adaptWidgetPosXY(ui->videoGroup, m_videoGroup_width, m_videoGroup_height, adapt_width, adapt_height);
    adaptWidgetPosXY(ui->videoLabel, m_videoLabel_width, m_videoLabel_height, adapt_width, adapt_height);
    adaptWidgetWH(ui->label_version, m_versionlabel_posx, m_versionlabel_posy,
                  m_versionlabel_width, m_versionlabel_height, adapt_width, adapt_height);

    // 注：原latestContour重映射整段删除——轮廓存视频坐标，GL的uMVP自动适配窗口缩放
    // 注：手动矩形也不需要清除——存视频坐标，窗口缩放后仍然贴合内容

    m_width = this->width();
    m_height = this->height();
}

void MainWindow::initsize()
{
    m_width = this->width();
    m_height = this->height();
    m_srcwidth = m_width;
    m_srcheight = m_height;
    m_videoLabel_width = ui->videoLabel->width();
    m_videoLabel_height = ui->videoLabel->height();
    m_imageList_posx = ui->imageListWidget->pos().x();
    m_imageList_width = ui->imageListWidget->width();
    m_imageList_height = ui->imageListWidget->height();
    m_saveGroup_width = ui->saveGroup->width();
    m_saveGroup_height = ui->saveGroup->height();
    m_saveGroup_posx = ui->saveGroup->pos().x();
    m_imgGroup_width = ui->imgGroup->width();
    m_imgGroup_height = ui->imgGroup->height();
    m_imgGroup_posx = ui->imgGroup->pos().x();
    m_videoGroup_width = ui->videoGroup->width();
    m_videoGroup_height = ui->videoGroup->height();
    m_line_posx = ui->line->pos().x();
    m_line_height = ui->line->height();
    m_versionlabel_width = ui->label_version->width();
    m_versionlabel_height = ui->label_version->height();
    m_versionlabel_posx = ui->label_version->pos().x();
    m_versionlabel_posy = ui->label_version->pos().y();
    m_fpsLabel_width = ui->fpsLabel->width();
    m_fpsLabel_height = ui->fpsLabel->height();
    m_fpsLabel_posx = ui->fpsLabel->pos().x();
    m_fpsLabel_posy = ui->fpsLabel->pos().y();
}

void MainWindow::onWindowResizeDetected()
{
    int currentW = this->width();
    int currentH = this->height();
    if (currentW != m_lastwindowWidth || currentH != m_lastwindowHeight) {
        m_lastwindowWidth = currentW;
        m_lastwindowHeight = currentH;
        qDebug() << "[Timer] 窗口尺寸变化侦测到:" << currentW << "x" << currentH;
        savewindowsize();
    }
}

void MainWindow::savewindowsize()
{
    QSettings settings(CONFFILE, QSettings::IniFormat);
    QString winsize = QString("%1x%2").arg(m_lastwindowWidth).arg(m_lastwindowHeight);
    settings.setValue("winsize", winsize);
}

// ============================================================================
// 图片压缩（原有）
// ============================================================================
void MainWindow::CompressImage()
{
    QList<QListWidgetItem*> selected = ui->imageListWidget->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一张图片！");
        return;
    }
    if (m_oneimg_format != "jpg" || m_moreimg_format != "jpg") {
        QMessageBox::warning(this, "提示",
            "压缩图片时，图片必须设置为jpg格式，\n请在工具栏【系统设置】/【保存文件命名规则】中进行设置");
        return;
    }
    QListWidgetItem* item = selected.first();
    QString imagekey = item->data(Qt::UserRole).toString();
    m_curcompimagekey = imagekey;

    if (!m_saveimagemap.contains(imagekey)) {
        if (imagekey.contains("main_") == false) {
            QFileInfo fileInfo(imagekey);
            if (!fileInfo.exists()) return;
            QPixmap pixmap;
            if (!pixmap.load(imagekey)) {
                return;
            } else {
                m_saveimagemap[imagekey].pixmap = pixmap;
            }
        } else return;
    }
    QPixmap originalPixmap = m_saveimagemap[imagekey].pixmap;
    int quality = 100;
    if (m_saveimagecompmap.contains(imagekey)) quality = m_saveimagecompmap[imagekey];

    m_compdialog->setModal(false);
    m_compdialog->setWindowTitle("压缩图片");
    m_compdialog->init(QString::number(quality), originalPixmap);
    m_compdialog->open();
    m_compdialog->exec();
}
