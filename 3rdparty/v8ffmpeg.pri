FFMPEG_ROOT = $$PWD
INCLUDEPATH += $$FFMPEG_ROOT/include

LIBS += -L$$FFMPEG_ROOT/bin \
        -L$$FFMPEG_ROOT/lib \
        -lavutil \                  #核心工具库
        -lavdevice \                #输入输出设备库
        -lavcodec \                 #编解码库
        -lavformat \                #件格式和协议库
        -lswscale \                 #将图像进行格式转换
        -lswresample \              #用于音频重采样，可以对数字音频进行声道数、数据格式、采样率等多种基本信息的转换
        -lavfilter                  #音视频滤镜库

win32 {
    FFMPEG_DLL_PATH = $$FFMPEG_ROOT/bin
    INSTALL_DIR = $$OUT_PWD/install
    #复制 FFmpeg DLL 文件
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$FFMPEG_DLL_PATH/avutil-60.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$FFMPEG_DLL_PATH/avdevice-62.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$FFMPEG_DLL_PATH/avcodec-62.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$FFMPEG_DLL_PATH/avformat-62.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$FFMPEG_DLL_PATH/swscale-9.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$FFMPEG_DLL_PATH/swresample-6.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$FFMPEG_DLL_PATH/avfilter-11.dll) $$shell_path($$INSTALL_DIR) $$escape_expand(\\n\\t)
}

HEADERS +=

SOURCES +=
