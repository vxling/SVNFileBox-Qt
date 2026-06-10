#pragma once
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>
#include <QThread>

class SyncRecordService;
class SVNClient;

// ── PasteTask ──────────────────────────────────────────────────
// Worker thread that scans then copies files, emitting progress
class PasteTask : public QThread
{
    Q_OBJECT
public:
    struct Item {
        QString srcPath;
        QString destPath;
        bool isDir;
        qint64 size = 0;
    };

    explicit PasteTask(const QList<Item> &items, QObject *parent = nullptr);
    void run() override;

signals:
    void stageChanged(int stage);  // 0=analyzing, 1=copying
    void progressChanged(int current, int total, qint64 bytesDone, qint64 totalBytes, const QString &currentFile, const QString &stageLabel);
    void copyFinished(int copied, int failed);

protected:
    QList<Item> m_items;
    qint64 m_totalBytes = 0;
    QString m_wcPath;
};

// ── CopyProgressDialog ─────────────────────────────────────────
class CopyProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CopyProgressDialog(QWidget *parent = nullptr);
    ~CopyProgressDialog() override;

    // stage: 0 = analyzing, 1 = copying
    void setStage(int stage);
    void setProgress(int current, int total, qint64 bytesDone, qint64 totalBytes,
                     const QString &currentFile, const QString &stageLabel);
    void reset();
    bool wasCancelled() const { return m_cancelled; }
 void markDone(const QString &message);

public slots:
    void reject() override;

signals:
    void cancelled();

private slots:
    void onCancelClicked();

private:
    void showEvent(QShowEvent *event) override;

    QLabel *m_stageLabel = nullptr;
    QLabel *m_fileCountLabel = nullptr;
    QLabel *m_bytesLabel = nullptr;
    friend class MainWindow;
    QProgressBar *m_progressBar = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    bool m_cancelled = false;
    bool m_done = false;
};

class SyncRecordsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SyncRecordsDialog(QWidget *parent = nullptr);
    ~SyncRecordsDialog() override;

private:
    QTableView *m_tableView = nullptr;
};

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() override;
};
