#pragma once
#include <QStyledItemDelegate>
#include <QPainter>

class FileCardDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit FileCardDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

signals:
    void doubleClicked(int row) const;

private:
    static const int CARD_HEIGHT = 64;
    static const int CARD_MARGIN = 8;
    static const int CARD_PADDING = 12;
    static const int CARD_RADIUS = 8;
};