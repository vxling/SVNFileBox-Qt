#include "i18n/translator.h"

#include <QCoreApplication>
#include <QLocale>
#include <QFile>
#include <QDebug>

namespace SVNFileBox {

Translator::Translator(QObject *parent)
    : QObject(parent)
    , m_translator(nullptr)
{
}

Translator::~Translator()
{
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
    }
}

QString Translator::resolveQmPath(const QString &languageCode)
{
    QString code = languageCode.trimmed();

    // "auto" / "" → detect from system locale
    if (code.isEmpty() || code.compare("auto", Qt::CaseInsensitive) == 0) {
        code = QLocale::system().name(); // e.g. "zh_CN", "en_US", "de_DE"
    }

    // Normalize: "zh-CN" → "zh_CN"
    code.replace('-', '_');

    // English is the source language; no .qm needed.
    if (code.compare("en", Qt::CaseInsensitive) == 0
        || code.startsWith("en_", Qt::CaseInsensitive)) {
        return QString();
    }

    // Try exact match (zh_CN), then language-only (zh).
    const QStringList candidates {
        QStringLiteral(":/i18n/") + code + QStringLiteral(".qm"),
        QStringLiteral(":/i18n/") + code.left(2) + QStringLiteral(".qm"),
    };
    for (const QString &p : candidates) {
        if (QFile::exists(p)) return p;
    }
    return QString();
}

bool Translator::installForLanguage(const QString &languageCode)
{
    // Tear down any previous translator first.
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    const QString qmPath = resolveQmPath(languageCode);
    if (qmPath.isEmpty()) {
        qDebug() << "[Translator] No qm for" << languageCode << "→ using source (English)";
        return false;
    }

    m_translator = new QTranslator(this);
    if (!m_translator->load(qmPath)) {
        qWarning() << "[Translator] Failed to load" << qmPath;
        delete m_translator;
        m_translator = nullptr;
        return false;
    }
    if (!QCoreApplication::installTranslator(m_translator)) {
        qWarning() << "[Translator] installTranslator failed for" << qmPath;
        delete m_translator;
        m_translator = nullptr;
        return false;
    }
    qDebug() << "[Translator] Loaded" << qmPath;
    return true;
}

} // namespace SVNFileBox