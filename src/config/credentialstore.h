#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QString>
#include <QPair>

// Machine-bound credential storage.
// Encrypts passwords using XOR + Base64 with a per-machine key,
// so the config file cannot be copied to another machine and decrypted.
// Credentials are stored in ~/.svnfilebox/credentials.json.

class CredentialStore
{
public:
    // Store credentials for a repository (encrypts password, keeps username plaintext)
    static bool store(const QString &repoName, const QString &username, const QString &password);

    // Retrieve credentials for a repository
    // Returns {username, password} or empty strings if not found
    static QPair<QString, QString> retrieve(const QString &repoName);

    // Remove credentials for a repository
    static bool remove(const QString &repoName);

    // Check if credentials exist for a repository
    static bool has(const QString &repoName);

private:
    static QString credentialsFilePath();
    static QString machineId();
    static QString encrypt(const QString &plaintext);
    static QString decrypt(const QString &ciphertext);
};

#endif // CREDENTIALSTORE_H