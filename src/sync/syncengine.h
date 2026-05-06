#ifndef SYNCENGINE_H
#define SYNCENGINE_H

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>

class SyncEngine : public QObject
{
    Q_OBJECT

public:
    explicit SyncEngine(QObject *parent = nullptr);
    ~SyncEngine();

    Q_INVOKABLE void startSync(const QString &localPath, const QString &remotePath);
    Q_INVOKABLE void stopSync();
    Q_INVOKABLE QString status() const;

signals:
    void syncStarted();
    void syncProgress(const QString &file);
    void syncFinished();
    void syncError(const QString &error);

private slots:
    void onFileChanged(const QString &path);

private:
    void scanLocalChanges(const QString &path);
    void uploadChanges();

    QString m_localPath;
    QString m_remotePath;
    QFileSystemWatcher *m_watcher = nullptr;
    bool m_syncing = false;
};

#endif // SYNCENGINE_H
