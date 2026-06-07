#pragma once
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>

class SyncRecordService;

class CopyProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CopyProgressDialog(QWidget *parent = nullptr);
    ~CopyProgressDialog() override;

    void setProgress(int current, int total, qint64 bytesCopied, qint64 totalBytes, const QString &currentFile);
    void reset();

signals:
    void cancelled();

private slots:
    void onCancelClicked();

private:
    QProgressBar *m_progressBar;
    QLabel *m_fileLabel;
    QLabel *m_bytesLabel;
    QPushButton *m_cancelBtn;
    bool m_wasCancelled = false;
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