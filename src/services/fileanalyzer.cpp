#include "fileanalyzer.h"
#include <QDir>
#include <QFileInfo>

// ── Static helpers ───────────────────────────────────────────────────────
namespace {

// Recursive walk to build items. Pre-order: directory itself first, then children.
void walkForPlan(const QString &src, const QString &rel, FileCopyPlan &plan)
{
    QFileInfo fi(src);
    if (!fi.exists()) return;

    // Skip .svn directories entirely
    if (fi.isDir() && fi.fileName() == ".svn") return;

    if (fi.isDir()) {
        FileCopyItem item;
        item.sourcePath = src;
        item.destPath = QDir(plan.destRoot).absoluteFilePath(rel);
        item.relativePath = rel;
        item.itemType = FileCopyItem::Directory;
        item.sizeBytes = 0;
        plan.items.append(item);

        for (const QFileInfo &child : QDir(src).entryInfoList(
                 QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden)) {
            QString childRel = rel.isEmpty() ? child.fileName() : (rel + "/" + child.fileName());
            walkForPlan(child.absoluteFilePath(), childRel, plan);
        }
    } else {
        FileCopyItem item;
        item.sourcePath = src;
        item.destPath = QDir(plan.destRoot).absoluteFilePath(rel);
        item.relativePath = rel;
        item.itemType = FileCopyItem::File;
        item.sizeBytes = fi.size();
        plan.totalBytes += fi.size();
        plan.items.append(item);
    }
}

} // namespace

FileCopyPlan FileAnalyzer::analyze(const QStringList &sources, const QString &destRoot)
{
    FileCopyPlan plan;
    plan.sourceRoot = sources.isEmpty() ? QString() : QFileInfo(sources.first()).absolutePath();
    plan.destRoot = destRoot;
    plan.totalBytes = 0;

    if (sources.isEmpty() || destRoot.isEmpty()) return plan;

    // Same-location check: if any source == destRoot exactly, refuse
    for (const QString &src : sources) {
        if (QFileInfo(src).absoluteFilePath() == QFileInfo(destRoot).absoluteFilePath()) {
            plan.isSameLocation = true;
            return plan;
        }
    }

    for (const QString &src : sources) {
        QFileInfo fi(src);
        if (!fi.exists()) continue;

        QString name = fi.fileName();
        walkForPlan(fi.absoluteFilePath(), name, plan);
    }

    return plan;
}

FileCopyPlan FileAnalyzer::analyzeOne(const QString &source, const QString &destRoot)
{
    return analyze(QStringList{source}, destRoot);
}
