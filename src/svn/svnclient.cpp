#include "svnclient.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

extern "C" {
#include <svn_client.h>
#include <svn_wc.h>
#include <svn_error.h>
#include <svn_path.h>
#include <svn_repos.h>
#include <svn_ra.h>
#include <svn_auth.h>
#include <svn_time.h>
#include <svn_xml.h>
#include <svn_io.h>
#include <svn_delta.h>
#include <svn_diff.h>
#include <svn_opt.h>
#include <svn_dirent_uri.h>
#include <svn_pools.h>
}

namespace {

// Helper to get pointers to svn_opt_revision_t constants
static svn_opt_revision_t makeOptRev(svn_opt_revision_kind kind)
{
    static svn_opt_revision_t unspecifiedRev = { svn_opt_revision_unspecified };
    static svn_opt_revision_t headRev = { svn_opt_revision_head };
    switch (kind) {
        case svn_opt_revision_unspecified: return unspecifiedRev;
        case svn_opt_revision_head: return headRev;
        default: {
            static svn_opt_revision_t var;
            var.kind = kind;
            return var;
        }
    }
}

// Return address of a static svn_opt_revision_t for a given kind
static const svn_opt_revision_t *optRevPtr(svn_opt_revision_kind kind)
{
    static svn_opt_revision_t unspecifiedRev = { svn_opt_revision_unspecified };
    static svn_opt_revision_t headRev = { svn_opt_revision_head };
    if (kind == svn_opt_revision_unspecified) return &unspecifiedRev;
    if (kind == svn_opt_revision_head) return &headRev;
    return &unspecifiedRev; // fallback
}


static QString statusKindToQStr(svn_wc_status_kind kind)
{
    switch (kind) {
        case svn_wc_status_none:          return QStringLiteral("None");
        case svn_wc_status_unversioned:   return QStringLiteral("Unversioned");
        case svn_wc_status_normal:        return QStringLiteral("Normal");
        case svn_wc_status_added:         return QStringLiteral("Added");
        case svn_wc_status_missing:       return QStringLiteral("Missing");
        case svn_wc_status_deleted:       return QStringLiteral("Deleted");
        case svn_wc_status_replaced:     return QStringLiteral("Replaced");
        case svn_wc_status_modified:     return QStringLiteral("Modified");
        case svn_wc_status_merged:       return QStringLiteral("Merged");
        case svn_wc_status_conflicted:    return QStringLiteral("Conflicted");
        case svn_wc_status_ignored:       return QStringLiteral("Ignored");
        case svn_wc_status_obstructed:    return QStringLiteral("Obstructed");
        case svn_wc_status_external:     return QStringLiteral("External");
        case svn_wc_status_incomplete:    return QStringLiteral("Incomplete");
        default:                          return QStringLiteral("Unknown");
    }
}

// ── Status baton for batch operations ───────────────────────────

struct StatusBaton {
    QVariantMap *result = nullptr;
    QMutex mutex;
    bool batchMode = false;
};

// Callback for svn_client_status5
static svn_error_t *
status_catcher(void *baton, const char *localPath, const svn_client_status_t *status, apr_pool_t *)
{
    StatusBaton *b = static_cast<StatusBaton *>(baton);
    QString path = QString::fromUtf8(localPath);
    // Use node_status as primary; fall back to text_status
    svn_wc_status_kind itemKind = status->node_status != svn_wc_status_none
        ? status->node_status : status->text_status;
    QString itemStr = statusKindToQStr(itemKind);
    QString propStr = statusKindToQStr(status->prop_status);
    QString combined = itemStr;
    if (status->prop_status != svn_wc_status_none && status->prop_status != svn_wc_status_normal) {
        if (!combined.isEmpty() && !propStr.isEmpty())
            combined += QLatin1Char(':');
        combined += propStr;
    }
    QMutexLocker locker(&b->mutex);
    if (b->batchMode) {
        if (status->node_status != svn_wc_status_normal || status->prop_status != svn_wc_status_normal) {
            if (b->result)
                (*b->result)[path] = combined;
        }
    } else {
        if (b->result)
            (*b->result)[path] = combined;
    }
    return SVN_NO_ERROR;
}

} // anonymous namespace

// ── Private ─────────────────────────────────────────────────────

struct SVNClient::Private {
    apr_pool_t *pool = nullptr;
    svn_client_ctx_t *ctx = nullptr;
    QString username;
    QString password;
    QString configDir;
    bool trustedMode = false;

    struct CacheEntry {
        qint64 revision = -1;
        QElapsedTimer timer;
    };
    QMap<QString, CacheEntry> headRevCache;
    mutable QMutex cacheMutex;
    static constexpr int HEAD_REV_TTL_MS = 30'000;

    ~Private() {
        if (ctx) {
            // No svn_client_ctx_destroy; just free the pool
            ctx = nullptr;
        }
        if (pool) {
            apr_pool_destroy(pool);
            pool = nullptr;
        }
    }

    bool ensureInitialized() {
        if (pool)
            return true;
        pool = svn_pool_create(NULL);
        return initCtx();
    }

    bool initCtx() {
        if (!pool)
            return false;
        svn_error_t *err = svn_client_create_context(&ctx, pool);
        if (err) {
            svn_error_clear(err);
            ctx = nullptr;
            return false;
        }
        setupAuth();
        return true;
    }

    void setupAuth() {
        if (!ctx || !pool)
            return;

        apr_array_header_t *providers = apr_array_make(pool, 8, sizeof(svn_auth_provider_object_t *));

        svn_auth_provider_object_t *provider = nullptr;

        svn_auth_get_simple_provider(&provider, pool);
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

        svn_auth_get_username_provider(&provider, pool);
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

        // No SSL server trust file provider in this SVN version without prompt

        svn_auth_open(&ctx->auth_baton, providers, pool);

        if (!username.isEmpty()) {
            svn_auth_set_parameter(ctx->auth_baton, SVN_AUTH_PARAM_DEFAULT_USERNAME,
                                   username.toUtf8().constData());
        }
        if (!password.isEmpty()) {
            svn_auth_set_parameter(ctx->auth_baton, SVN_AUTH_PARAM_DEFAULT_PASSWORD,
                                   password.toUtf8().constData());
        }
        if (!configDir.isEmpty()) {
            svn_auth_set_parameter(ctx->auth_baton, SVN_AUTH_PARAM_CONFIG_DIR,
                                   configDir.toUtf8().constData());
        }
    }

    svn_client_ctx_t *ctxForOp() {
        if (!ensureInitialized())
            return nullptr;
        return ctx;
    }

    qint64 getHeadRevCached(const QString &url) {
        {
            QMutexLocker locker(&cacheMutex);
            auto it = headRevCache.find(url);
            if (it != headRevCache.end()) {
                if (it.value().timer.elapsed() < HEAD_REV_TTL_MS)
                    return it.value().revision;
            }
        }
        qint64 rev = fetchHeadRev(url);
        {
            QMutexLocker locker(&cacheMutex);
            CacheEntry entry;
            entry.revision = rev;
            entry.timer.start();
            headRevCache.insert(url, entry);
        }
        return rev;
    }

    qint64 fetchHeadRev(const QString &url) {
        if (!ctxForOp())
            return -1;

        svn_opt_revision_t rev;
        rev.kind = svn_opt_revision_head;
        svn_opt_revision_t peg;
        peg.kind = svn_opt_revision_unspecified;

        struct InfoBaton {
            qint64 rev = -1;
        } infoBaton;

        svn_error_t *err = svn_client_info3(url.toUtf8().constData(),
                                            &peg, &rev,
                                            svn_depth_empty,
                                            false, false, nullptr,
                                            [](void *baton, const char *, const svn_client_info2_t *info, apr_pool_t *) -> svn_error_t * {
                if (info) {
                    InfoBaton *b = static_cast<InfoBaton *>(baton);
                    b->rev = info->rev;
                }
                return SVN_NO_ERROR;
            },
            &infoBaton, ctx, pool);
        if (err) {
            svn_error_clear(err);
            return -1;
        }
        return infoBaton.rev;
    }
};

// ── SVNClient ───────────────────────────────────────────────────

SVNClient::SVNClient(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

SVNClient::~SVNClient() = default;

void SVNClient::setUsername(const QString &u)
{
    d->username = u;
    if (d->ctx)
        d->setupAuth();
}

void SVNClient::setPassword(const QString &p)
{
    d->password = p;
    if (d->ctx)
        d->setupAuth();
}

void SVNClient::setConfigDir(const QString &dir)
{
    d->configDir = dir;
    if (d->ctx)
        d->setupAuth();
}

void SVNClient::setTrustedMode(bool on)
{
    d->trustedMode = on;
    if (d->ctx)
        d->setupAuth();
}

// ── Write operations ────────────────────────────────────────────

bool SVNClient::add(const QString &path)
{
    if (!d->ctxForOp())
        return false;
    svn_error_t *err = svn_client_add5(path.toUtf8().constData(),
                                       svn_depth_infinity,
                                       false, // force
                                       false, // no_ignore
                                       false, // no_autoprops
                                       false, // add_parents
                                       d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] add failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::remove(const QString &path)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *targets = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(targets, const char *) = apr_pstrdup(d->pool, path.toUtf8().constData());
    svn_error_t *err = svn_client_delete4(targets, false, false, nullptr, nullptr, nullptr, d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] remove failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::commit(const QString &path, const QString &message)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *targets = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(targets, const char *) = apr_pstrdup(d->pool, path.toUtf8().constData());

    // Use deprecated svn_client_commit4 which returns commit_info directly
    svn_commit_info_t *info = nullptr;
    svn_error_t *err = svn_client_commit4(&info, targets,
                                          svn_depth_infinity,
                                          false, // keep_locks
                                          false, // keep_changelists
                                          nullptr, // changelists
                                          nullptr, // revprop_table
                                          d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] commit failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::update(const QString &path)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *targets = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(targets, const char *) = apr_pstrdup(d->pool, path.toUtf8().constData());
    apr_array_header_t *resultRevs = nullptr;
    svn_opt_revision_t headRev;
    headRev.kind = svn_opt_revision_head;
    svn_error_t *err = svn_client_update4(&resultRevs, targets,
                                          &headRev,
                                          svn_depth_infinity,
                                          false, // depth_is_sticky
                                          false, // ignore_externals
                                          false, // allow_unver_obstructions
                                          true,  // adds_as_modification
                                          false, // make_parents
                                          d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] update failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::mkdir(const QString &path)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *targets = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(targets, const char *) = apr_pstrdup(d->pool, path.toUtf8().constData());
    svn_commit_info_t *info = nullptr;
    svn_error_t *err = svn_client_mkdir3(&info, targets,
                                          false, // make_parents
                                          nullptr, // revprop_table
                                          d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] mkdir failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::move(const QString &src, const QString &dst)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *targets = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(targets, const char *) = apr_pstrdup(d->pool, src.toUtf8().constData());
    svn_commit_info_t *info = nullptr;
    svn_error_t *err = svn_client_move5(&info, targets,
                                         dst.toUtf8().constData(),
                                         false, // force
                                         false, // move_as_child
                                         false, // make_parents
                                         nullptr, // revprop_table
                                         d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] move failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::revert(const QString &path, bool recursive)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *targets = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(targets, const char *) = apr_pstrdup(d->pool, path.toUtf8().constData());
    svn_error_t *err = svn_client_revert4(targets,
                                           recursive ? svn_depth_infinity : svn_depth_empty,
                                           nullptr, // changelists
                                           false, // clear_changelists
                                           false, // metadata_only
                                           false, // added_keep_local
                                           d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] revert failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::cleanup(const QString &path)
{
    if (!d->ctxForOp())
        return false;
    svn_error_t *err = svn_client_cleanup(path.toUtf8().constData(), d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] cleanup failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::unlock(const QString &path)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *targets = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(targets, const char *) = apr_pstrdup(d->pool, path.toUtf8().constData());
    svn_error_t *err = svn_client_unlock(targets, false, d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] unlock failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::checkout(const QString &url, const QString &localPath)
{
    if (!d->ctxForOp())
        return false;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_head;
    svn_revnum_t resultRev = SVN_INVALID_REVNUM;
    svn_error_t *err = svn_client_checkout3(&resultRev,
                                             url.toUtf8().constData(),
                                             localPath.toUtf8().constData(),
                                             &rev, &rev,
                                             svn_depth_infinity,
                                             false, // allow_unver_obstructions
                                             false, // add_discovered_mat_info
                                             d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] checkout failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::resolveConflict(const QString &path, const QString &accept)
{
    if (!d->ctxForOp())
        return false;
    svn_wc_conflict_choice_t choice;
    if (accept == QLatin1String("mine-conflict"))
        choice = svn_wc_conflict_choose_mine_conflict;
    else if (accept == QLatin1String("theirs-conflict"))
        choice = svn_wc_conflict_choose_theirs_conflict;
    else if (accept == QLatin1String("mine-full"))
        choice = svn_wc_conflict_choose_mine_full;
    else if (accept == QLatin1String("theirs-full"))
        choice = svn_wc_conflict_choose_theirs_full;
    else if (accept == QLatin1String("base"))
        choice = svn_wc_conflict_choose_base;
    else if (accept == QLatin1String("working"))
        choice = svn_wc_conflict_choose_merged;
    else
        choice = svn_wc_conflict_choose_postpone;

    svn_error_t *err = svn_client_resolve(path.toUtf8().constData(),
                                          svn_depth_empty,
                                          choice,
                                          d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] resolve failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::copyFileOrFolder(const QString &src, const QString &dest)
{
    if (!d->ctxForOp())
        return false;
    apr_array_header_t *sources = apr_array_make(d->pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(sources, const char *) = apr_pstrdup(d->pool, src.toUtf8().constData());
    svn_commit_info_t *info = nullptr;
    svn_error_t *err = svn_client_copy4(&info, sources,
                                         dest.toUtf8().constData(),
                                         false, // copy_as_child
                                         false, // make_parents
                                         nullptr, // revprop_table
                                         d->ctx, d->pool);
    if (err) {
        qWarning() << "[SVNClient] copy failed:" << err->message;
        svn_error_clear(err);
        return false;
    }
    return true;
}

bool SVNClient::breakWriteLock(const QString &path)
{
    if (path.isEmpty())
        return false;
    return cleanup(path);
}

// ── Read operations ─────────────────────────────────────────────

QString SVNClient::getRepoUrl(const QString &path)
{
    if (!d->ctxForOp())
        return QString();
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    struct InfoBaton {
        QString url;
    } infoBaton;
    svn_error_t *err = svn_client_info3(path.toUtf8().constData(),
                                        &rev, &rev,
                                        svn_depth_empty,
                                        false, false, nullptr,
                                        [](void *baton, const char *, const svn_client_info2_t *info, apr_pool_t *) -> svn_error_t * {
            if (info && info->URL) {
                InfoBaton *b = static_cast<InfoBaton *>(baton);
                b->url = QString::fromUtf8(info->URL);
            }
            return SVN_NO_ERROR;
        },
                                        &infoBaton, d->ctx, d->pool);
    if (err) {
        svn_error_clear(err);
        return QString();
    }
    return infoBaton.url;
}

QVariantMap SVNClient::getInfo(const QString &path)
{
    if (!d->ctxForOp())
        return QVariantMap();
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    struct InfoBaton {
        QVariantMap result;
    } infoBaton;

    svn_error_t *err = svn_client_info3(path.toUtf8().constData(),
                                        &rev, &rev,
                                        svn_depth_empty,
                                        false, false, nullptr,
                                        [](void *baton, const char *, const svn_client_info2_t *info, apr_pool_t *pool) -> svn_error_t * {
            if (!info) return SVN_NO_ERROR;
            InfoBaton *b = static_cast<InfoBaton *>(baton);
            b->result[QStringLiteral("revision")] = static_cast<int>(info->rev);
            b->result[QStringLiteral("lastChangedRev")] = static_cast<int>(info->last_changed_rev);
            b->result[QStringLiteral("lastChangedDate")] = static_cast<qlonglong>(info->last_changed_date);
            if (info->last_changed_author)
                b->result[QStringLiteral("author")] = QString::fromUtf8(info->last_changed_author);
            if (info->URL)
                b->result[QStringLiteral("url")] = QString::fromUtf8(info->URL);
            if (info->last_changed_date > 0) {
                const char *date_cstr = svn_time_to_cstring(info->last_changed_date, pool);
                if (date_cstr)
                    b->result[QStringLiteral("lastChangedDateStr")] = QString::fromUtf8(date_cstr);
            }
            return SVN_NO_ERROR;
        },
                                        &infoBaton, d->ctx, d->pool);
    if (err) {
        svn_error_clear(err);
        return QVariantMap();
    }
    return infoBaton.result;
}


QString SVNClient::getStatusString(const QString &path)
{
    if (!d->ctxForOp())
        return QStringLiteral("Unknown");

    StatusBaton baton;
    QVariantMap result;
    baton.result = &result;
    baton.batchMode = false;

    svn_revnum_t dummyRev = SVN_INVALID_REVNUM;
    svn_error_t *err = svn_client_status5(&dummyRev,
                                          d->ctx,
                                          path.toUtf8().constData(),
                                          optRevPtr(svn_opt_revision_unspecified),
                                          svn_depth_empty,
                                          false, // get_all
                                          false, // update
                                          false, // no_ignore
                                          false, // ignore_externals
                                          false, // depth_as_sticky
                                          nullptr, // changelists
                                          status_catcher,
                                          &baton,
                                          d->pool);
    if (err) {
        svn_error_clear(err);
        return QStringLiteral("Unknown");
    }
    if (result.isEmpty())
        return QStringLiteral("Normal");
    return result.constBegin().value().toString();
}

QString SVNClient::getLastChangedTime(const QString &path)
{
    if (!d->ctxForOp())
        return QString();
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    struct InfoBaton {
        QString dateStr;
    } infoBaton;
    svn_error_t *err = svn_client_info3(path.toUtf8().constData(),
                                        &rev, &rev,
                                        svn_depth_empty,
                                        false, false, nullptr,
                                        [](void *baton, const char *, const svn_client_info2_t *info, apr_pool_t *pool) -> svn_error_t * {
            if (info && info->last_changed_date != 0) {
                InfoBaton *b = static_cast<InfoBaton *>(baton);
                const char *date_cstr = svn_time_to_cstring(info->last_changed_date, pool);
                if (date_cstr)
                    b->dateStr = QString::fromUtf8(date_cstr);
            }
            return SVN_NO_ERROR;
        },
                                        &infoBaton, d->ctx, d->pool);
    if (err) {
        svn_error_clear(err);
        return QString();
    }
    return infoBaton.dateStr;
}

int SVNClient::getWorkingCopyRevision(const QString &path)
{
    if (!d->ctxForOp())
        return -1;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    struct InfoBaton {
        qint64 rev = -1;
    } infoBaton;
    svn_error_t *err = svn_client_info3(path.toUtf8().constData(),
                                        &rev, &rev,
                                        svn_depth_empty,
                                        false, false, nullptr,
                                        [](void *baton, const char *, const svn_client_info2_t *info, apr_pool_t *) -> svn_error_t * {
            if (info) {
                InfoBaton *b = static_cast<InfoBaton *>(baton);
                b->rev = info->rev;
            }
            return SVN_NO_ERROR;
        },
                                        &infoBaton, d->ctx, d->pool);
    if (err) {
        svn_error_clear(err);
        return -1;
    }
    return static_cast<int>(infoBaton.rev);
}

int SVNClient::getHeadRevision(const QString &url)
{
    return static_cast<int>(d->getHeadRevCached(url));
}

bool SVNClient::isVersioned(const QString &path)
{
    if (isValidWorkingCopy(path))
        return true;
    QFileInfo fi(path);
    if (fi.isDir()) {
        return QFile::exists(path + "/.svn/entries") || QFile::exists(path + "/.svn/wc.db");
    } else {
        return QFile::exists(fi.dir().absolutePath() + "/.svn/entries") ||
               QFile::exists(fi.dir().absolutePath() + "/.svn/wc.db");
    }
}

bool SVNClient::isValidWorkingCopy(const QString &path)
{
    if (!QDir(path).exists())
        return false;
    return QFile::exists(path + "/.svn/entries") || QFile::exists(path + "/.svn/wc.db");
}

bool SVNClient::hasIncompleteWorkingCopy(const QString &path)
{
    if (!d->ctxForOp())
        return false;
    StatusBaton baton;
    QVariantMap result;
    baton.result = &result;
    baton.batchMode = false;
    svn_revnum_t dummyRev = SVN_INVALID_REVNUM;
    svn_error_t *err = svn_client_status5(&dummyRev,
                                          d->ctx,
                                          path.toUtf8().constData(),
                                          optRevPtr(svn_opt_revision_unspecified),
                                          svn_depth_infinity,
                                          false, false, false, false, false,
                                          nullptr,
                                          status_catcher,
                                          &baton,
                                          d->pool);
    if (err) {
        svn_error_clear(err);
        return false;
    }
    for (auto it = result.constBegin(); it != result.constEnd(); ++it) {
        if (it.value().toString().contains(QStringLiteral("Incomplete")))
            return true;
    }
    return false;
}

bool SVNClient::testConnection(const QString &url)
{
    if (!d->ctxForOp())
        return false;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_head;
    struct InfoBaton {
        bool ok = false;
    } infoBaton;
    svn_error_t *err = svn_client_info3(url.toUtf8().constData(),
                                        &rev, &rev,
                                        svn_depth_empty,
                                        false, false, nullptr,
                                        [](void *baton, const char *, const svn_client_info2_t *, apr_pool_t *) -> svn_error_t * {
            InfoBaton *b = static_cast<InfoBaton *>(baton);
            b->ok = true;
            return SVN_NO_ERROR;
        },
                                        &infoBaton, d->ctx, d->pool);
    if (err) {
        svn_error_clear(err);
        return false;
    }
    return infoBaton.ok;
}

bool SVNClient::isCredentialValid(const QString &repoUrl)
{
    return testConnection(repoUrl);
}

int SVNClient::cachedHeadRevision(const QString &url) const
{
    QMutexLocker locker(&d->cacheMutex);
    auto it = d->headRevCache.find(url);
    if (it != d->headRevCache.end())
        return static_cast<int>(it.value().revision);
    return -1;
}

QStringList SVNClient::list(const QString &path)
{
    if (!d->ctxForOp())
        return QStringList();
    struct ListBaton {
        QStringList names;
        QMutex mutex;
    } listBaton;
    svn_error_t *err = svn_client_list2(path.toUtf8().constData(),
                                         optRevPtr(svn_opt_revision_unspecified),
                                         optRevPtr(svn_opt_revision_unspecified),
                                         svn_depth_immediates,
                                         0, // dirent_fields (0 = all)
                                         false, // fetch_locks
                                         [](void *baton, const char *path, const svn_dirent_t *,
                                            const svn_lock_t *, const char *, apr_pool_t *) -> svn_error_t * {
            if (path) {
                ListBaton *b = static_cast<ListBaton *>(baton);
                QMutexLocker locker(&b->mutex);
                b->names.append(QString::fromUtf8(path));
            }
            return SVN_NO_ERROR;
        },
                                         &listBaton,
                                         d->ctx, d->pool);
    if (err) {
        svn_error_clear(err);
        return QStringList();
    }
    return listBaton.names;
}

QStringList SVNClient::getConflictedFiles(const QString &path)
{
    if (!d->ctxForOp())
        return QStringList();
    StatusBaton baton;
    QVariantMap result;
    baton.result = &result;
    baton.batchMode = false;
    svn_revnum_t dummyRev = SVN_INVALID_REVNUM;
    svn_error_t *err = svn_client_status5(&dummyRev,
                                          d->ctx,
                                          path.toUtf8().constData(),
                                          optRevPtr(svn_opt_revision_unspecified),
                                          svn_depth_infinity,
                                          false, false, false, true, false,
                                          nullptr,
                                          status_catcher,
                                          &baton,
                                          d->pool);
    if (err) {
        svn_error_clear(err);
        return QStringList();
    }
    QStringList conflicted;
    for (auto it = result.constBegin(); it != result.constEnd(); ++it) {
        if (it.value().toString().contains(QStringLiteral("Conflicted")))
            conflicted.append(it.key());
    }
    return conflicted;
}

QStringList SVNClient::getServerUpdatePaths(const QString &path)
{
    // svn_client_status5 with update=true shows repos update info
    if (!d->ctxForOp())
        return QStringList();
    StatusBaton baton;
    QVariantMap result;
    baton.result = &result;
    baton.batchMode = false;
    svn_revnum_t dummyRev = SVN_INVALID_REVNUM;
    svn_error_t *err = svn_client_status5(&dummyRev,
                                          d->ctx,
                                          path.toUtf8().constData(),
                                          optRevPtr(svn_opt_revision_unspecified),
                                          svn_depth_infinity,
                                          false, // get_all
                                          true,  // update - repos info
                                          false, false, false,
                                          nullptr,
                                          status_catcher,
                                          &baton,
                                          d->pool);
    if (err) {
        svn_error_clear(err);
        return QStringList();
    }
    return QStringList();
}

QVariantMap SVNClient::getStatus(const QString &path, bool depth)
{
    if (!d->ctxForOp())
        return QVariantMap();
    StatusBaton baton;
    QVariantMap result;
    baton.result = &result;
    baton.batchMode = false;
    svn_revnum_t dummyRev = SVN_INVALID_REVNUM;
    svn_error_t *err = svn_client_status5(&dummyRev,
                                          d->ctx,
                                          path.toUtf8().constData(),
                                          optRevPtr(svn_opt_revision_unspecified),
                                          depth ? svn_depth_infinity : svn_depth_empty,
                                          false, false, false, true, false,
                                          nullptr,
                                          status_catcher,
                                          &baton,
                                          d->pool);
    if (err) {
        svn_error_clear(err);
        return QVariantMap();
    }
    return result;
}

QVariantMap SVNClient::batchGetStatus(const QString &dirPath)
{
    if (!d->ctxForOp())
        return QVariantMap();
    StatusBaton baton;
    QVariantMap result;
    baton.result = &result;
    baton.batchMode = true;
    svn_revnum_t dummyRev = SVN_INVALID_REVNUM;
    svn_error_t *err = svn_client_status5(&dummyRev,
                                          d->ctx,
                                          dirPath.toUtf8().constData(),
                                          optRevPtr(svn_opt_revision_unspecified),
                                          svn_depth_immediates,
                                          false, false, false, true, false,
                                          nullptr,
                                          status_catcher,
                                          &baton,
                                          d->pool);
    if (err) {
        svn_error_clear(err);
        return QVariantMap();
    }
    return result;
}