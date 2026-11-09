#include "util/customitemdelegate.h"
#include <QLineEdit>
#include <QFontMetrics>

CustomItemDelegate::CustomItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* CustomItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);
    // 设置合理的最小尺寸（仅影响编辑时的输入框）
    editor->setMinimumWidth(180);
    editor->setMinimumHeight(28);
    editor->setFont(option.font);
    editor->setFrame(true);
    return editor;
}

void CustomItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString text = index.model()->data(index, Qt::DisplayRole).toString();
    static_cast<QLineEdit*>(editor)->setText(text);
}

void CustomItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString text = static_cast<QLineEdit*>(editor)->text();
    model->setData(index, text, Qt::DisplayRole);
}

