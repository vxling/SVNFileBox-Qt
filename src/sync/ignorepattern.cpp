#include "ignorepattern.h"

namespace SVNFileBox {

QList<QRegularExpression> compileIgnorePatterns(const QStringList &patterns)
{
    QList<QRegularExpression> out;
    for (const QString &p : patterns) {
        QString pat = p.trimmed();
        if (pat.isEmpty() || pat.startsWith(QLatin1String("#"))) continue;
        QString rx = QStringLiteral("^");
        for (QChar c : pat) {
            if (c == QLatin1Char('*'))      rx += QStringLiteral(".*");
            else if (c == QLatin1Char('?')) rx += QStringLiteral(".");
            else if (c == QLatin1Char('.')) rx += QStringLiteral("\\.");
            else if (c == QLatin1Char('\\')) rx += QStringLiteral("\\\\");
            else if (c == QLatin1Char('+')
                  || c == QLatin1Char('(')
                  || c == QLatin1Char(')')
                  || c == QLatin1Char('[')
                  || c == QLatin1Char(']')
                  || c == QLatin1Char('{')
                  || c == QLatin1Char('}')
                  || c == QLatin1Char('|')
                  || c == QLatin1Char('^')
                  || c == QLatin1Char('$')) rx += QStringLiteral("\\") + c;
            else rx += c;
        }
        rx += QStringLiteral("$");
        out.append(QRegularExpression(rx, QRegularExpression::CaseInsensitiveOption));
    }
    return out;
}

} // namespace SVNFileBox