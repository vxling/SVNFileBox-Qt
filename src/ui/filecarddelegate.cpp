#include "filecarddelegate.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QPainterPath>
#include <QRect>

FileCardDelegate::FileCardDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize FileCardDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    Q_UNUSED(index);
    Q_UNUSED(option);
    return QSize(200, CARD_HEIGHT);
}

void FileCardDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    if (!index.isValid()) return;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    QRect r = option.rect;
    bool isSelected = (option.state & QStyle::State_Selected);
    bool isAlternate = (index.row() % 2 == 1);

    // ── Card background (white card on gray content area)
    QColor cardBg = isSelected ? QColor(0xDC, 0xF5, 0xE8) : Qt::white;
    QColor cardBorder = isSelected ? QColor(0x07, 0xC1, 0x60) : QColor(0xEE, 0xEE, 0xEE);

    QPainterPath path;
    path.addRoundedRect(r.adjusted(4, 4, -4, -4), CARD_RADIUS, CARD_RADIUS);
    painter->fillPath(path, cardBg);

    QPen borderPen(cardBorder, isSelected ? 1.5 : 0.8);
    painter->setPen(borderPen);
    painter->drawPath(path);

    // ── Inner content
    QRect inner = r.adjusted(12, 8, -12, -8);

    // Column 0: icon (folder/file emoji)
    QString iconText = index.data(Qt::UserRole + 1).toString(); // isDir
    QString icon = iconText == "true" ? QStringLiteral("📁") : QStringLiteral("📄");

    QFont iconFont(icon);
    iconFont.setPixelSize(28);
    QRect iconRect(inner.left(), inner.top(), 36, inner.height());
    painter->setFont(iconFont);
    painter->setPen(Qt::NoPen);
    painter->drawText(iconRect, Qt::AlignCenter, icon);

    // Column 1: name + status + size
    QRect textRect(inner.left() + 44, inner.top(), inner.width() - 44 - 80, inner.height());

    QString name = index.data(Qt::DisplayRole).toString();
    QString status = index.data(Qt::UserRole + 2).toString(); // svnStatus
    QString size = index.data(Qt::UserRole + 3).toString();    // size

    // File name
    QFont nameFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Normal);
    nameFont.setWeight(QFont::Medium);
    painter->setFont(nameFont);
    painter->setPen(QColor(0x33, 0x33, 0x33));
    QRect nameRect(textRect.left(), textRect.top(), textRect.width(), 20);
    painter->drawText(nameRect, Qt::AlignVCenter | Qt::TextSingleLine, name);

    // Status badge (small colored tag)
    QRect statusRect(textRect.left(), textRect.top() + 22, 40, 16);
    if (!status.isEmpty() && status != QStringLiteral("Normal")) {
        QColor statusColor(0x07, 0xC1, 0x60); // WeChat green for modified
        if (status == QStringLiteral("Added")) statusColor = QColor(0x19, 0x84, 0xE6);
        if (status == QStringLiteral("Conflicted")) statusColor = QColor(0xE5, 0x39, 0x35);
        if (status == QStringLiteral("Deleted")) statusColor = QColor(0xFF, 0x98, 0x00);

        QPainterPath sp;
        sp.addRoundedRect(statusRect, 3, 3);
        painter->fillPath(sp, statusColor);

        QFont statusFont(QStringLiteral("Microsoft YaHei"), 9);
        painter->setFont(statusFont);
        painter->setPen(Qt::white);
        painter->drawText(statusRect, Qt::AlignCenter, status.left(4));
    }

    // Size
    if (!size.isEmpty()) {
        QFont sizeFont(QStringLiteral("Microsoft YaHei"), 11);
        sizeFont.setWeight(QFont::Normal);
        painter->setFont(sizeFont);
        painter->setPen(QColor(0xB2, 0xB2, 0xB2));
        QRect sizeRect(textRect.left() + 46, textRect.top() + 22, 60, 16);
        painter->drawText(sizeRect, Qt::AlignVCenter, size);
    }

    // Column 2: modified time (right side)
    QString time = index.data(Qt::UserRole + 4).toString(); // lastModified
    if (!time.isEmpty()) {
        QFont timeFont(QStringLiteral("Microsoft YaHei"), 11);
        painter->setFont(timeFont);
        painter->setPen(QColor(0xB2, 0xB2, 0xB2));
        QRect timeRect(inner.right() - 70, inner.top(), 70, inner.height());
        painter->drawText(timeRect, Qt::AlignVCenter | Qt::AlignRight, time);
    }
}