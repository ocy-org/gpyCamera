QT       += core gui multimedia multimediawidgets printsupport concurrent opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = gpycamera
linux {
    CONFIG += link_pkgconfig
    #统信UOS的opencv库
    #PKGCONFIG += opencv
    #麒麟kylin的opencv库
    PKGCONFIG += opencv4
}
win32 {
    RC_ICONS = gpy_default.ico
}
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
DEFINES += WIN32_LEAN_AND_MEAN NOMINMAX
QMAKE_CXXFLAGS += -include windows.h

SOURCES += \
    cameraprobing.cpp \
    cameraworker.cpp \
    captureworker.cpp \
    dealcontours.cpp \
    detectworker.cpp \
    dialog/cameracontrolwidget.cpp \
    dialog/mainconfdialog.cpp \
    dialog/menu_activationdialog.cpp \
    dialog/menu_cameracontroldialog.cpp \
    dialog/menu_filesavedialog.cpp \
    dialog/menu_maininfodialog.cpp \
    dialog/menu_pdfconfdialog.cpp \
    dialog/menu_savefilenameruledialog.cpp \
    dialog/selectcamera_dialog.cpp \
    frameutils.cpp \
    groovecontour.cpp \
    dialog/imagecompressdialog.cpp \
    imagelistwidget.cpp \
    imagepreviewdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    util/commfuntion.cpp \
    util/countdownwidget.cpp \
    util/customitemdelegate.cpp \
    util/linebasedfilehandler.cpp \
    util/qrcodegen.cpp \
    util/qrcodescan.cpp \
    videoglwidget.cpp

HEADERS += \
    cameraprobing.h \
    cameraworker.h \
    captureworker.h \
    dealcontours.h \
    detectworker.h \
    dialog/cameracontrolwidget.h \
    dialog/mainconfdialog.h \
    dialog/menu_activationdialog.h \
    dialog/menu_cameracontroldialog.h \
    dialog/menu_filesavedialog.h \
    dialog/menu_maininfodialog.h \
    dialog/menu_pdfconfdialog.h \
    dialog/menu_savefilenameruledialog.h \
    dialog/selectcamera_dialog.h \
    framepacket.h \
    frameutils.h \
    groovecontour.h \
    dialog/imagecompressdialog.h \
    imagelistwidget.h \
    imagepreviewdialog.h \
    mainwindow.h \
    metatypes.h \
    util/LeftClickOnlyMenu.h \
    util/comm_define.h \
    util/commfuntion.h \
    util/countdownwidget.h \
    util/customitemdelegate.h \
    util/linebasedfilehandler.h \
    util/permit.h \
    util/qrcodegen.hpp \
    util/qrcodescan.h \
    util/ymoredes.h \
    util/zxing_compat.h \
    videoglwidget.h

FORMS += \
    dialog/cameracontrolwidget.ui \
    dialog/mainconfdialog.ui \
    dialog/menu_activationdialog.ui \
    dialog/menu_cameracontroldialog.ui \
    dialog/menu_filesavedialog.ui \
    dialog/menu_maininfodialog.ui \
    dialog/menu_pdfconfdialog.ui \
    dialog/menu_savefilenameruledialog.ui \
    dialog/selectcamera_dialog.ui \
    dialog/imagecompressdialog.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INSTALL_DIR = $$OUT_PWD/install

unix {
    # Linux下探测依赖 v4l-utils，需安装：sudo apt install v4l-utils
    QMAKE_CXXFLAGS += -fno-sized-deallocation
    QMAKE_LFLAGS += -fno-sized-deallocation
    # 头文件路径（系统默认路径，无需手动创建目录）
    INCLUDEPATH += /usr/include/x86_64-linux-gnu
    # 库文件路径
    LIBS += -L/usr/lib/x86_64-linux-gnu
    # FFmpeg 核心库（按依赖顺序链接）
    LIBS += -lavutil -lavcodec -lavformat -lswscale -lswresample -lavdevice

    #二维码失败库
    LIBS += -L$$PWD/ZXing/ -lZXing
    INCLUDEPATH += $$PWD/ZXing
    DEPENDPATH += $$PWD/ZXing
    PRE_TARGETDEPS += $$PWD/ZXing/libZXing.a
}
win32 {
    LIBS += -lavcodec -lavformat -lavutil -lswscale
    # 指定 ZXing 头文件路径
    INCLUDEPATH += "D:/software/otherdll/zxing-cpp-master/core/install/include"
    # 指定 ZXing 库文件路径
    LIBS += -L"D:/software/otherdll/zxing-cpp-master/core/install/lib" -lZXing
    #创建 install 目录
    QMAKE_POST_BUILD += $$QMAKE_MKDIR $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    include(D:\software\opencv\opencv.pri)
    include(D:\software\ffmpeg-8.0\v8ffmpeg.pri)
    # Release 模式
    EXE_FILE = $$OUT_PWD/release/$${TARGET}.exe
}


RESOURCES += \
    res.qrc
