#ifndef LINEFILEHANDLER_H
#define LINEFILEHANDLER_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

class LineBasedFileHandler
{
public:
    explicit LineBasedFileHandler(const QString &filePath);

    // 写入一行（追加模式）
    bool appendLine(const QString &line);

    // 覆盖整个文件内容（传入多行）
    bool writeLines(const QStringList &lines);

    // 读取所有行
    QStringList readAllLines() const;

    // 查找包含关键字的行（返回所有匹配行）
    bool findLines(const QString &keyword) const;

    // 删除包含某内容的行（支持精确或模糊）
    bool removeLinesContaining(const QString &content, bool exactMatch = false);

    // 获取文件路径
    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
};

#endif // LINEFILEHANDLER_H
