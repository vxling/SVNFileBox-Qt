#ifndef IGNOREPATTERN_H
#define IGNOREPATTERN_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QRegularExpression>

namespace SVNFileBox {

// P3 #2 / P3 #3: compile glob patterns (with * and ? wildcards) to
// anchored, case-insensitive regular expressions. Extracted as a free
// helper so it can be unit-tested without instantiating SyncEngine.
// - Empty lines and lines starting with '#' are skipped.
// - Patterns containing "/" are matched against the full relative path
//   (you should call matchIgnore() with the full path).
// - Patterns without "/" are matched against the basename only.
QList<QRegularExpression> compileIgnorePatterns(const QStringList &patterns);

// Returns true if any compiled regex matches either:
//   - the file's basename (always tested), or
//   - relPath (only tested if non-empty).
inline bool matchIgnore(const QList<QRegularExpression> &regexes,
                        const QString &basename,
                        const QString &relPath = QString())
{
    if (regexes.isEmpty()) return false;
    if (basename.isEmpty()) return false;
    for (const QRegularExpression &rx : regexes) {
        if (rx.match(basename).hasMatch()) return true;
        if (!relPath.isEmpty() && rx.match(relPath).hasMatch()) return true;
    }
    return false;
}

} // namespace SVNFileBox

#endif // IGNOREPATTERN_H