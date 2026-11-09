# 头文件路径
OPENCV_ROOT = $$PWD/opencvMinGW/OpenCV-MinGW-Build-OpenCV-4.5.2-x64
INCLUDEPATH += $$OPENCV_ROOT/include

# 库链接配置（标准-l语法代替绝对路径）
OPENCV_LIB_PATH = $$OPENCV_ROOT/x64/mingw/bin
LIBS += -L$$OPENCV_LIB_PATH \
        -lopencv_core452 \
        -lopencv_highgui452 \
        -lopencv_imgproc452 \
        -lopencv_calib3d452 \
        -lopencv_imgcodecs452 \
        -lopencv_videoio452

win32 {
    OPENCV_DLL_PATH = $$OPENCV_ROOT/x64/mingw/bin
    INSTALL_DIR = $$OUT_PWD/install
    #以下是windeployqt打包引用dll文件
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OPENCV_DLL_PATH/libopencv_core452.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OPENCV_DLL_PATH/libopencv_highgui452.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OPENCV_DLL_PATH/libopencv_imgproc452.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OPENCV_DLL_PATH/libopencv_calib3d452.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OPENCV_DLL_PATH/libopencv_imgcodecs452.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OPENCV_DLL_PATH/libopencv_videoio452.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
}
