#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class FileModel;

namespace Ui {
class NewFileDialog;
class NewFolderDialog;
class RenameDialog;
class RenameRepoDialog;
class EditRepoDialog;
class ConfirmDeleteDialog;
class ConflictDialog;
}

// ── NewFileDialog ──────────────────────────────────────────────
class NewFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewFileDialog(FileModel *fileModel, const QString &currentPath, QWidget *parent = nullptr);
    ~NewFileDialog() override;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    Ui::NewFileDialog *ui = nullptr;
    FileModel *m_fileModel = nullptr;
    QString m_currentPath;
    QString m_ext = QStringLiteral("txt");
};

// ── NewFolderDialog ─────────────────────────────────────────────
class NewFolderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewFolderDialog(FileModel *fileModel, const QString &currentPath, QWidget *parent = nullptr);
    ~NewFolderDialog() override;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    Ui::NewFolderDialog *ui = nullptr;
    FileModel *m_fileModel = nullptr;
    QString m_currentPath;
};

// ── RenameDialog ───────────────────────────────────────────────
class RenameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameDialog(const QString &oldPath, const QString &oldName, QWidget *parent = nullptr);
    QString newName() const;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    QLineEdit *m_nameInput = nullptr;
    QString m_oldPath;
};

// ── RenameRepoDialog ────────────────────────────────────────────
class RenameRepoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameRepoDialog(const QString &oldName, QWidget *parent = nullptr);
    QString newName() const;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    QLineEdit *m_nameInput = nullptr;
    QString m_oldName;
};

// ── EditRepoDialog ─────────────────────────────────────────────
class EditRepoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditRepoDialog(const QString &name, const QString &url, QWidget *parent = nullptr);
    QString newUrl() const;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    QLineEdit *m_urlInput = nullptr;
    QString m_name;
    QLabel *m_statusLabel = nullptr;
};

// ── ConfirmDeleteDialog ────────────────────────────────────────
class ConfirmDeleteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmDeleteDialog(const QString &itemName, QWidget *parent = nullptr);

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    QString m_itemName;
};

// ── ConflictDialog ─────────────────────────────────────────────
class ConflictDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConflictDialog(const QStringList &files, QWidget *parent = nullptr);

private slots:
    void onResolvedClicked();
    void onCancelClicked();

private:
    QStringList m_files;
};