#include "util/linebasedfilehandler.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

LineBasedFileHandler::LineBasedFileHandler(const QString &filePath)
    : m_filePath(filePath)
{
}

bool LineBasedFileHandler::appendLine(const QString &line)
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "无法打开文件进行追加：" << file.errorString();
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << line << "\n";
    file.close();
    return true;
}

bool LineBasedFileHandler::writeLines(const QStringList &lines)
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件进行写入：" << file.errorString();
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    for (const QString &line : lines) {
        out << line << "\n";
    }
    file.close();
    return true;
}

QStringList LineBasedFileHandler::readAllLines() const
{
    // 检查文件是否存在
    if (!QFile::exists(m_filePath)) {
        // 文件不存在：尝试创建一个空文件
        QFile file(m_filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.close(); // 创建成功，关闭
            // qDebug() << "文件已创建：" << m_filePath;
        } else {
            qWarning() << "无法创建文件：" << m_filePath << file.errorString();
        }
        // 返回空列表（因为是新建的空文件）
        return QStringList();
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法读取文件：" << file.errorString();
        return QStringList();
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QStringList lines;
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }
    file.close();
    return lines;
}

bool LineBasedFileHandler::findLines(const QString &keyword) const
{
    QStringList allLines = readAllLines();
    for (const QString &line : allLines) {
        if (line == keyword) {
            return true;
        }
    }
    return false;
}

bool LineBasedFileHandler::removeLinesContaining(const QString &content, bool exactMatch)
{
    QStringList lines = readAllLines();
    QStringList newLines;

    for (const QString &line : lines) {
        bool shouldRemove = exactMatch ? (line == content) : line.contains(content, Qt::CaseSensitive);
        if (!shouldRemove) {
            newLines << line;
        }
    }

    // 重新写入剩余行
    return writeLines(newLines);
}
