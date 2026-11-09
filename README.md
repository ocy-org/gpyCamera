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
以下仅列举核心的功能
## 视频预览
### 自动裁边
<img width="769" height="661" alt="image" src="https://github.com/user-attachments/assets/951ee566-678a-494a-a568-b3f775b8f04a" />
- OpenCV线程只负责图像处理：获取最大轮廓的物体，并且获取轮廓上四个能现成矩形的坐标；
- OpenGL线程负责图像渲染，包括获取OpenCV线程推送过来的四个顶点坐标，通过MVP矩阵把矩形绘制出来。
### 手动裁边
  用户可以自己在屏幕上绘制矩形，点击【拍照】/【延时拍照】后，OpenCV线程根据坐标位置进行图片裁剪。
注意：高拍仪是高清摄像头高曝光的场景，本项目进行了两项优化：（1）增加网格方式去除边缘的光噪；（2）高曝光下获取的轮廓四个点不一定刚好能形成矩形，需要判断并进一步拟合成矩形。
### 顺/逆时针旋转镜头
  有些大文档只能横着放，旋转镜头看到的文字才是竖向的。旋转功能由OpenGL线程的矩形变换实现。
## 拍照图片预览
<img width="686" height="628" alt="image" src="https://github.com/user-attachments/assets/92ff972c-d35a-4696-ad47-92ac4f2808cb" />
  此图片预览控件继承QListWidget，支持把桌面的文件拖拽进来，或者是视频预览拍照后，在此处生成一张图片。图片预览控件里的图片也支持拖拽，以及右键菜单功能。这个控件可以直接移植到其他项目中使用。
## 授权激活功能
<img width="731" height="390" alt="8b888be7-8b9f-45fc-900c-06c2d059bf28" src="https://github.com/user-attachments/assets/43d7ee14-14d5-4e10-8e2b-2be671771346" />
本项目包含的授权激活功能支持二维码拍照录入方式，适用于现场授权，当前版本的代码已把加/解密源码删除，可用自行下载第三方源码QT-AES来实现相关功能，维码拍照由ZXing完成，源码已集成到项目中，以下是QT-AES使用案例传送门：
https://blog.csdn.net/liangyuna8787/article/details/148172551?spm=1001.2014.3001.5502
## 其他功能
还有图片合并为png/pdf、图片压缩图像等基础功能

