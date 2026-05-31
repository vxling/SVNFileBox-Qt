#ifndef NEWFILESERVICE_H
#define NEWFILESERVICE_H

#include <QObject>
#include <QString>

class NewFileService : public QObject
{
    Q_OBJECT

public:
    explicit NewFileService(QObject *parent = nullptr) : QObject(parent) {}

    // Static: no instance state needed, QML singleton forwards here
    Q_INVOKABLE static bool create(const QString &fullPath);
};

#endif // NEWFILESERVICE_H
