#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QObject>
#include <QTranslator>
#include <QString>

namespace SVNFileBox {

// P3 #4: load translation .qm at startup. Picks a language file based on
// ConfigService::language() which is one of:
//   - "auto" → detect from QLocale::system().name()
//   - "zh-CN" → i18n/zh_CN.qm
//   - "en"    → no translator (English is the source language)
//
// .qm files are embedded into the binary at build time via qt_add_translations
// (RESOURCE_PREFIX /i18n). We resolve them at runtime with QLocale.
class Translator : public QObject
{
    Q_OBJECT
public:
    explicit Translator(QObject *parent = nullptr);
    ~Translator();

    // Install a translator matching `languageCode`. Returns true if a .qm
    // was loaded (even if it was empty). Returns false when no .qm exists
    // for the language, in which case English is used.
    bool installForLanguage(const QString &languageCode);

    // Resolve the actual .qm to use: explicit code, or auto-detect from
    // the system locale. Returns the qm resource path (e.g. ":/i18n/zh_CN.qm"),
    // or empty string for "use source strings".
    static QString resolveQmPath(const QString &languageCode);

private:
    QTranslator *m_translator = nullptr;
};

} // namespace SVNFileBox

#endif // TRANSLATOR_H