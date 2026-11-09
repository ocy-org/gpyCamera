#ifndef LEFTCLICKONLYMENU_H
#define LEFTCLICKONLYMENU_H

#include <QMenu>
#include <QMouseEvent>

class LeftClickOnlyMenu : public QMenu
{
    Q_OBJECT

public:
    explicit LeftClickOnlyMenu(QWidget *parent = nullptr) : QMenu(parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            QMenu::mousePressEvent(event); // 允许左键
        } else {
            // 忽略右键、中键等
            event->accept(); // 吞掉事件，不关闭菜单也不触发动作
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            QMenu::mouseReleaseEvent(event);
        } else {
            event->accept(); // 吞掉
        }
    }
};

#endif // LEFTCLICKONLYMENU_H
