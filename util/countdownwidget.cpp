#include "countdownwidget.h"
#include <QVBoxLayout>
#include <QFont>
#include <QSizePolicy>

CountdownWidget::CountdownWidget(QWidget *parent)
    : QWidget(parent)
    , m_remaining(0)
{
    // 设置透明背景
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background: transparent;");

    // 创建 label
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet("background: transparent; color: white;");
    m_label->setFont(QFont("Arial", 80, QFont::Bold));
    m_label->setAttribute(Qt::WA_TranslucentBackground);

    // 布局（充满整个 widget）
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_label);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // 定时器
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &CountdownWidget::onTimeout);

    // 初始隐藏
    hide();
}

void CountdownWidget::start(int seconds)
{
    if (seconds <= 0) seconds = 5;
    m_remaining = seconds;
    updateDisplay(m_remaining);
    show();
    m_timer->start();
}

void CountdownWidget::onTimeout()
{
    m_remaining--;
    if (m_remaining > 0) {
        updateDisplay(m_remaining);
    } else {
        m_timer->stop();
        //m_label->setText("✅");
        // 0.5秒后自动隐藏
        QTimer::singleShot(500, this, &CountdownWidget::hideAndClean);
        emit finished();
    }
}

void CountdownWidget::updateDisplay(int value)
{
    QString color;
    color = "#0000FF";//
    /*if (value >= 4) color = "#FF4444";      // 鲜红
    else if (value == 3) color = "#FF9900"; // 橙
    else if (value == 2) color = "#FFFF00"; // 黄
    else color = "#00FF00";                // 绿*/

    m_label->setStyleSheet(QString("background: transparent; color: %1; font-weight: bold;").arg(color));
    m_label->setText(QString::number(value));
}

void CountdownWidget::hideAndClean()
{
    hide();
    m_label->setText(""); // 清空内容
}
