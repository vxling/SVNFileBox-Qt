#include "credentialstore.h"
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QCoreApplication>

QString CredentialStore::credentialsFilePath()
{
    return QDir::home().absoluteFilePath(".svnfilebox/credentials.json");
}

QString CredentialStore::machineId()
{
    // Read Linux machine-id (first 32 hex chars =128 bits)
    QFile file(QStringLiteral("/etc/machine-id"));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray id = file.readLine().simplified();
        file.close();
        if (!id.isEmpty())
            return QString::fromLatin1(id);
    }
    // Fallback: use a hash of the app path + home dir as machine fingerprint
    QString fallback = QCoreApplication::applicationFilePath()
                       + QDir::home().absolutePath();
    return QCryptographicHash::hash(fallback.toUtf8(),
                                    QCryptographicHash::Sha256).toHex().left(32);
}

QString CredentialStore::encrypt(const QString &plaintext)
{
    if (plaintext.isEmpty()) return QString();

    QString key = machineId();
    QByteArray keyBytes = QCryptographicHash::hash(
        key.toUtf8(), QCryptographicHash::Sha256);

    QByteArray data = plaintext.toUtf8();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= keyBytes[i % keyBytes.size()];

    return QString::fromLatin1(data.toBase64());
}

QString CredentialStore::decrypt(const QString &ciphertext)
{
    if (ciphertext.isEmpty()) return QString();

    QString key = machineId();
    QByteArray keyBytes = QCryptographicHash::hash(
        key.toUtf8(), QCryptographicHash::Sha256);

    QByteArray data = QByteArray::fromBase64(ciphertext.toLatin1());
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= keyBytes[i % keyBytes.size()];

    return QString::fromUtf8(data);
}

bool CredentialStore::store(const QString &repoName, const QString &username, const QString &password)
{
    QString path = credentialsFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject root;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        root = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
    }

    QJsonObject entry;
    entry[QStringLiteral("username")] = username;
    entry[QStringLiteral("password")] = encrypt(password);
    root[repoName] = entry;

    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

QPair<QString, QString> CredentialStore::retrieve(const QString &repoName)
{
    QPair<QString, QString> result;

    QString path = credentialsFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return result;

    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    QJsonObject entry = root[repoName].toObject();
    if (entry.isEmpty()) return result;

    result.first = entry[QStringLiteral("username")].toString();
    result.second = decrypt(entry[QStringLiteral("password")].toString());
    return result;
}

bool CredentialStore::remove(const QString &repoName)
{
    QString path = credentialsFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    if (!root.contains(repoName)) return true; // nothing to remove

    root.remove(repoName);

    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

bool CredentialStore::has(const QString &repoName)
{
    QString path = credentialsFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
    return root.contains(repoName);
}