#include "repoitemdelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QFontMetrics>

RepoItemDelegate::RepoItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void RepoItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QString name = index.data(Qt::UserRole + 1).toString();
    QString url = index.data(Qt::UserRole + 3).toString();
    QString type = index.data(Qt::UserRole + 4).toString();

    bool isNetwork = (type == QStringLiteral("Network"));
    QString iconChar = isNetwork ? QStringLiteral("N") : QStringLiteral("L");

    painter->save();

    // Background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, QColor(0xDC, 0xF5, 0xE8));
    }

    // Icon circle
    int iconAreaLeft = option.rect.left() + 8;
    int iconAreaTop = option.rect.top() +8;
    int iconSize = 28;
    QColor iconBg = isNetwork ? QColor(0x1E, 0x88, 0xE5) : QColor(0x07, 0xC1, 0x60);
    painter->setBrush(iconBg);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(iconAreaLeft, iconAreaTop, iconSize, iconSize);

    // Icon letter
    QFont iconFont(QStringLiteral("Arial"));
    iconFont.setPixelSize(13);
    iconFont.setWeight(QFont::Bold);
    painter->setFont(iconFont);
    painter->setPen(Qt::white);
    painter->drawText(QRect(iconAreaLeft, iconAreaTop, iconSize, iconSize),
                     Qt::AlignCenter, iconChar);

    // Text area
    int textLeft = option.rect.left() + 44;
    int textWidth = option.rect.right() - textLeft - 8;

    // Repo name
    QFont nameFont(QStringLiteral("Microsoft YaHei UI"));
    nameFont.setPixelSize(13);
    nameFont.setWeight(QFont::Bold);
    painter->setFont(nameFont);
    painter->setPen(option.state & QStyle::State_Selected
                        ? option.palette.highlightedText().color()
                        : QColor(0x1A, 0x1A, 0x2E));

    QRect nameRect(textLeft, option.rect.top() + 6, textWidth, 18);
    QFontMetrics fm(nameFont);
    QString elidedName = fm.elidedText(name, Qt::ElideRight, textWidth);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignBottom, elidedName);

    // URL
    QFont urlFont(QStringLiteral("Microsoft YaHei UI"));
    urlFont.setPixelSize(10);
    painter->setFont(urlFont);
    painter->setPen(option.state & QStyle::State_Selected
                        ? option.palette.highlightedText().color().lighter()
                        : QColor(0x99, 0x99, 0x99));

    QRect urlRect(textLeft, option.rect.top() + 26, textWidth, 14);
    QFontMetrics urlFm(urlFont);
    QString elidedUrl = urlFm.elidedText(url, Qt::ElideMiddle, textWidth);
    painter->drawText(urlRect, Qt::AlignLeft | Qt::AlignTop, elidedUrl);

    painter->restore();
}

QSize RepoItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(0, 44);
}