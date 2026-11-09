"# gpyCamera" 
  本项目在windows(64位)、国产Linux桌面操作系统（统信UOS、麒麟kylin）上通用，基于OpenCV+FFmpeg+OpenGL进行开发的高清摄像头场景下（高拍仪）的视频预览、物体裁边、图片合并的功能。
<img width="1505" height="830" alt="image" src="https://github.com/user-attachments/assets/673711e4-5130-41f7-94ec-aafce4982b63" />
# 环境安装
## windows
  为了多平台通用，window端使用WinGW版本的ffmpeg和opencv库,opencv库需要自己进行MinGW编译，ffmpeg有现成的MinGW版本。
### MinGW编译OpenCV
  https://blog.csdn.net/liangyuna8787/article/details/149693144?spm=1001.2014.3001.5502
  我编译的是4.5.2版本，在otherdll目录下有.pro文件要引用的opencv.pri，根据自己编译的版本和所在路径，修改里面的配置信息：OPENCV_ROOT及LIBS的库名称
### 加入FFmpeg
  https://blog.csdn.net/liangyuna8787/article/details/155307636?spm=1001.2014.3001.5502
  在otherdll目录下有.pro文件要引用的v8ffmpeg.pri，这个文件放在安装好的ffmpeg目录下即可。
## 国产Linux桌面操作系统
1、安装opencv库
sudo apt-get install libopencv-dev
2、安装ffmpeg相关的库
sudo apt-get install libavdevice-dev
3、安装 PulseAudio 相关的开发库
sudo apt-get install libpulse-dev
4、安装v4l2-ctl工具
sudo apt-get install v4l-utils
这里要说明下，在windows下，打开摄像头设备是用DirectShow，在Linux下则是用v4l2
# 功能说明
## 视频预览

## 授权激活功能
<img width="731" height="390" alt="8b888be7-8b9f-45fc-900c-06c2d059bf28" src="https://github.com/user-attachments/assets/43d7ee14-14d5-4e10-8e2b-2be671771346" />
本项目包含的授权激活功能支持二维码拍照录入方式，适用于现场授权，当前版本的代码已把加/解密源码删除，可用自行下载第三方源码QT-AES来实现相关功能，维码拍照由ZXing完成，源码已集成到项目中，以下是QT-AES使用案例传送门：
https://blog.csdn.net/liangyuna8787/article/details/148172551?spm=1001.2014.3001.5502

