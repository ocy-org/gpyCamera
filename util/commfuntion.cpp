#include "commfuntion.h"
#include <QDebug>

commfuntion::commfuntion()
{

}

bool commfuntion::getconfbyjson(QMap<QString, stCameraMsg>& dataMap){
    // 检查文件是否存在
    QFile file(JSONFILEPATH);
    if (!file.exists()) {
        // 文件不存在，创建文件
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
        }
    }else{//解析Json文件
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open the file";
            return false;
        }
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("data")) {
            QJsonArray dataArray = jsonObj["data"].toArray();
            //qDebug()<< __FUNCTION__<< __LINE__ << dataArray  ;
            foreach (const QJsonValue &value, dataArray) {
                QJsonObject obj = value.toObject();
                QString name = obj["name"].toString();
                QString main = obj["main"].toString();
                QString sub = obj["sub"].toString();
                stCameraMsg addrmsg;
                addrmsg.name=name;
                addrmsg.maincamera=main;
                addrmsg.subcamera=sub;
                dataMap.insert(name,addrmsg);
            }
        }
        file.close();
    }
    return true;
}

bool commfuntion::insertjsondata(const stCameraMsg& cameramsg){
    // 读取JSON文件内容
    QFile rfile(JSONFILEPATH);
    if (!rfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << rfile.errorString();
        return false;
    }
    QByteArray jsonData = rfile.readAll();
    rfile.close();
    if(jsonData.length()==0){
        // 创建 JSON 数据
        QJsonObject jsonObject;
        QJsonArray dataArray;
        // 插入一条记录
        QJsonObject newObject;
        newObject["name"] = cameramsg.name;
        newObject["main"] = cameramsg.maincamera;
        newObject["sub"] = cameramsg.subcamera;
        dataArray.append(newObject);
        jsonObject["data"] = dataArray;
        // 将 JSON 数据转换为字符串
        QJsonDocument jsonDocument(jsonObject);
        QByteArray insertData = jsonDocument.toJson();

        // 写入到文件
        QFile wfile(JSONFILEPATH);
        if (!wfile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open file for writing." << wfile.errorString();
            return false;
        }
        wfile.write(insertData);
        wfile.close();
    }else{
        // 解析JSON数据
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "Failed to parse JSON:" << parseError.errorString();
            return false;
        }

        // 获取JSON数组
        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray dataArray = jsonObj["data"].toArray();
        //qDebug()<< __FUNCTION__<< __LINE__ << jsonArray ;
        // 在数组中查找并更新指定的数据
        bool bfind=false;
        QString name = cameramsg.name;
        for (int i = 0; i < dataArray.size(); ++i) {
            QJsonObject item = dataArray[i].toObject();
            if (item["name"].toString() == name) {
                bfind = true;//此数据已存在,对数据进行更改
                item["main"]=cameramsg.maincamera;
                item["sub"] = cameramsg.subcamera;
                dataArray[i] = item; // 更新数组中的数据
                break;
            }
        }

        if(bfind==false){
            QJsonObject newObject;
            newObject["name"] = cameramsg.name;
            newObject["main"] = cameramsg.maincamera;
            newObject["sub"] = cameramsg.subcamera;
            qDebug()<< __LINE__ << "新增数据：" << cameramsg.name <<cameramsg.maincamera<<cameramsg.subcamera;
            // 将新对象插入到数组中
            dataArray.append(newObject);
        }
        // 更新JSON对象中的"data"数组
        jsonObj["data"] = dataArray;
        // 将更新后的JSON写回文件
        jsonDoc = QJsonDocument(jsonObj);
        QFile wfile(JSONFILEPATH);
        if (!wfile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qDebug() << "Failed to open file for writing:" << wfile.errorString();
            return false;
        }
        wfile.write(jsonDoc.toJson());
        wfile.close();
    }
    qDebug() << __LINE__ <<"JSON data written to file successfully.";

    return true;
}

bool commfuntion::deljsondata(const QString& key){
    // 读取JSON文件内容
    QFile file(JSONFILEPATH);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << file.errorString();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // 解析JSON数据
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse JSON:" << parseError.errorString();
        return false;
    }

    // 获取JSON数组
    QJsonObject jsonObj = jsonDoc.object();
    QJsonArray dataArray = jsonObj["data"].toArray();
    //qDebug()<< __FUNCTION__<< __LINE__ << jsonArray ;
    // 在数组中查找并删除指定的数据
    for (int i = 0; i < dataArray.size(); ++i) {
        QJsonObject item = dataArray[i].toObject();
        if (item["name"].toString() == key) {
            dataArray.removeAt(i);// 找到并删除数据
            break;
        }
    }
    // 更新JSON对象中的"data"数组
    jsonObj["data"] = dataArray;

    // 将更新后的JSON写回文件
    jsonDoc = QJsonDocument(jsonObj);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "Failed to open file for writing:" << file.errorString();
        return false;
    }

    file.write(jsonDoc.toJson());
    file.close();
    return true;
}

#include <windows.h>  // ← 只在 .cpp 中包含
std::string commfuntion::utf8ToGbk(const std::string& utf8) {
    // UTF-8 -> UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen == 0) return "";

    std::wstring wstr(wlen - 1, 0); // -1 to exclude null terminator in size
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wlen);

    // UTF-16 -> GBK (CP_ACP = system default ANSI code page, e.g., GBK on Chinese Windows)
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) return "";

    std::string gbk(len - 1, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &gbk[0], len, nullptr, nullptr);

    return gbk;
}

#include <QProcess>
QString commfuntion::executeCommand(const QString &command, const QStringList &args)
{
    QProcess process;
    process.start(command, args);
    process.waitForFinished(5000); // 5秒超时

    if (process.exitCode() != 0) {
        qDebug() << "Command failed:" << command << args << "Error:" << process.errorString();
        return "Unknown";
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    return output.trimmed();
}

// 获取CPU ID
QString commfuntion::getCpuId()
{
    QString cpuId = "";
#ifdef Q_OS_WIN
    cpuId = getCpuIdWindows();
#elif defined(Q_OS_LINUX)
    cpuId = getCpuIdLinux();
#endif
    return cpuId;
}


#ifdef Q_OS_WIN
// Windows平台获取CPU ID
QString commfuntion::getCpuIdWindows()
{
    QString cpuId = "";
    QString wmicOutput = executeCommand("wmic", QStringList() << "cpu" << "get" << "ProcessorId");
    QStringList lines = wmicOutput.split("\n", QString::SkipEmptyParts);

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed != "ProcessorId" && !trimmed.isEmpty() && trimmed != "Unknown") {
            cpuId = trimmed;
            break;
        }
    }
    return cpuId;
}
#endif

#ifdef Q_OS_LINUX
// Linux平台获取CPU ID
QString commfuntion::getCpuIdLinux()
{
    QString cpuId = "";
    QFile cpuinfo("/proc/cpuinfo");
    if (cpuinfo.open(QIODevice::ReadOnly)) {
        QTextStream in(&cpuinfo);
        QStringList processorIds;

        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("processor") || line.startsWith("vendor_id") ||
                line.startsWith("cpu family") || line.startsWith("model") ||
                line.startsWith("stepping") || line.startsWith("microcode")) {
                processorIds.append(line);
            }
        }
        cpuinfo.close();

        if (!processorIds.isEmpty()) {
            cpuId = calculateHash(processorIds.join("|"));
        }
    }
    return cpuId;
}
#endif


bool commfuntion::savePixmapToFile(const QString &filePath,const QPixmap &pixmap) {
    if (pixmap.isNull()) {
        qDebug() << "图片数据为空！";
        return false;
    }

    // 保存到指定路径
    if (pixmap.save(filePath)) {
        return true;
    } else {
        qDebug() << "保存图片失败";
        return false;
    }
}

bool commfuntion::savePixmapToFilebypercent(const QString &filePath,const QPixmap &pixmap,const int& percent){
    if (pixmap.isNull()) {
        qDebug() << "图片数据为空！";
        return false;
    }

    int targetWidth = pixmap.width() * percent/100;  // 缩放比例可根据实际效果微调，如 0.4 或 0.6
    int targetHeight = pixmap.height() * percent/100;
    // 使用 SmoothTransformation 保证缩小后的图片依然清晰，没有锯齿
    QPixmap scaledPixmap = pixmap.scaled(targetWidth, targetHeight,
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
    // 保存到指定路径
    if (scaledPixmap.save(filePath)) {
        return true;
    } else {
        qDebug() << "保存图片失败,percent:" << percent;
        return false;
    }
}

// 检查文件是否为图片格式
bool commfuntion::isImageFile(const QString &filePath)
{
    QString extension = QFileInfo(filePath).suffix().toLower();  // 获取文件扩展名并转为小写
    return (extension == "png" || extension == "jpg" ||
            extension == "jpeg" || extension == "bmp" || extension == "gif");
}

// 验证是否是整数
bool commfuntion::isInteger(const QString &str)
{
    bool ok;
    str.toInt(&ok);
    return ok;
}
