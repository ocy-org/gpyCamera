#ifndef CUSTOMITEMDELEGATE_H
#define CUSTOMITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QLineEdit>

class CustomItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CustomItemDelegate(QObject *parent = nullptr);

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

#endif
