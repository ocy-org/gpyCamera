#ifndef COUNTDOWNWIDGET_H
#define COUNTDOWNWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>

class CountdownWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CountdownWidget(QWidget *parent = nullptr);
    void start(int seconds = 5); // 启动倒计时

signals:
    void finished(); // 倒计时结束信号

private slots:
    void onTimeout();

private:
    void updateDisplay(int value);
    void hideAndClean();

    QLabel* m_label;
    QTimer* m_timer;
    int m_remaining;
};

#endif // COUNTDOWNWIDGET_H
