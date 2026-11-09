#ifndef PERMIT_H
#define PERMIT_H
#include <stdlib.h>
#include <QDebug>
#include <QString>

class PerMit : public QObject
{
    Q_OBJECT

public:
    explicit PerMit(QObject *parent = nullptr) : QObject(parent) {}

    // 加密
    QString encrypt(const QString &plainText)
    {
        return "123456";
    }

    // 解密
    QString decrypt(const QString &cipherText)
    {
        return "123456";
    }
};


#endif // PERMIT_H
