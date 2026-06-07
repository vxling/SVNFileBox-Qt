#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class FileModel;

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
    FileModel *m_fileModel = nullptr;
    QString m_currentPath;
    QString m_ext;

    QLineEdit *m_nameInput;
    QLabel *m_typeLabel;
};

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
    FileModel *m_fileModel = nullptr;
    QString m_currentPath;

    QLineEdit *m_nameInput;
};

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
    QLineEdit *m_nameInput;
    QString m_oldPath;
};

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
    QLineEdit *m_nameInput;
    QString m_oldName;
};

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
    QLineEdit *m_urlInput;
    QString m_name;
    QLabel *m_statusLabel;
};

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