#include "update_controller.h"
#include "build_info.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
const QString repository = "https://api.github.com/repos/Nowaterisenough/Sunshine";
const QString releasePrefix = "https://github.com/Nowaterisenough/Sunshine/releases/";
constexpr qint64 maxPackageSize = 1024LL * 1024 * 1024;
bool validDigest(const QString &digest) {
    return QRegularExpression("^sha256:[0-9a-f]{64}$").match(digest).hasMatch();
}
}

UpdateController::Environment UpdateController::environment() {
    Environment result;
    result.commit = QStringLiteral(PANEL_COMMIT);
    result.commitTime = QDateTime::fromString(QStringLiteral(PANEL_COMMIT_TIME), Qt::ISODate);
    result.cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/updates";
#ifdef Q_OS_WIN
    if (QSysInfo::buildCpuArchitecture() == "x86_64") {
        const QDir host(QCoreApplication::applicationDirPath() + "/../..");
        result.package = !host.exists("sunshine.exe") ? Package::WindowsPanel :
            host.exists("unins000.exe") ? Package::WindowsInstaller : Package::WindowsPortable;
    }
#elif defined(Q_OS_MACOS)
    if (QSysInfo::buildCpuArchitecture() == "arm64") result.package = Package::MacArmPanel;
#endif
    result.openPackage = [](const QString &path, bool install) {
#ifdef Q_OS_WIN
        if (install) {
            // Inno keeps the existing installation directory and requests elevation.
            const auto native = QDir::toNativeSeparators(path).toStdWString();
            return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"runas", native.c_str(),
                nullptr, nullptr, SW_SHOWNORMAL)) > 32;
        }
#else
        Q_UNUSED(install);
#endif
        return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
    };
    return result;
}

UpdateController::UpdateController(QObject *parent) : UpdateController(environment(), nullptr, parent) {}

UpdateController::UpdateController(Environment environment, QNetworkAccessManager *network, QObject *parent)
    : QObject(parent), m_environment(std::move(environment)),
      m_network(network ? network : new QNetworkAccessManager(this)) {
    const QSettings settings;
    m_autoCheck = settings.value("updates/autoCheck", true).toBool();
    m_autoDownload = settings.value("updates/autoDownload", false).toBool();
    m_showButton = settings.value("updates/showButton", true).toBool();
    m_includePrerelease = settings.value("updates/includePrerelease", true).toBool();
    m_intervalMinutes = settings.value("updates/intervalMinutes", 60).toInt();
    if (!QList<int>{30, 60, 360, 1440}.contains(m_intervalMinutes)) m_intervalMinutes = 60;
    m_lastChecked = settings.value("updates/lastChecked").toString();
    connect(&m_timer, &QTimer::timeout, this, &UpdateController::check);
}

QString UpdateController::currentVersion() const {
    if (m_environment.commit.isEmpty()) return tr("开发版本");
    return m_environment.commitTime.date().toString("yyyy.MM.dd") + " · " + m_environment.commit.left(7);
}

QString UpdateController::packageDescription() const {
    switch (m_environment.package) {
    case Package::WindowsInstaller: return tr("Windows 安装版 · 下载后可覆盖安装，保留配置与配对");
    case Package::WindowsPortable: return tr("Windows 便携版 · 下载后退出程序，解压到原目录");
    case Package::WindowsPanel: return tr("Windows Qt 面板 · 下载后退出面板，替换原目录");
    case Package::MacArmPanel: return tr("macOS Apple 芯片 · 下载后退出面板，解压并替换 App");
    default: return tr("此平台暂无可用更新包");
    }
}

void UpdateController::status(const QString &state, const QString &message) {
    m_state = state; m_message = message; emit changed();
}

void UpdateController::savePreferences() {
    QSettings settings;
    settings.setValue("updates/autoCheck", m_autoCheck);
    settings.setValue("updates/autoDownload", m_autoDownload);
    settings.setValue("updates/showButton", m_showButton);
    settings.setValue("updates/includePrerelease", m_includePrerelease);
    settings.setValue("updates/intervalMinutes", m_intervalMinutes);
    if (m_autoCheck) m_timer.start(m_intervalMinutes * 60000); else m_timer.stop();
    emit preferencesChanged();
}
void UpdateController::setAutoCheck(bool value) { m_autoCheck = value; savePreferences(); }
void UpdateController::setAutoDownload(bool value) { m_autoDownload = value; savePreferences(); }
void UpdateController::setShowButton(bool value) { m_showButton = value; savePreferences(); }
void UpdateController::setIncludePrerelease(bool value) {
    if (busy()) cancel();
    m_includePrerelease = value; m_available = false; m_asset = {}; m_release = {};
    savePreferences(); status("idle", tr("更新渠道已切换，请检查更新"));
}
void UpdateController::setIntervalMinutes(int value) {
    if (!QList<int>{30, 60, 360, 1440}.contains(value)) return;
    m_intervalMinutes = value; savePreferences();
}
void UpdateController::start() {
    if (!m_autoCheck) return;
    m_timer.start(m_intervalMinutes * 60000);
    QTimer::singleShot(5000, this, [this] { if (m_autoCheck) check(); });
}

bool UpdateController::trustedUrl(const QUrl &url) {
    if (!url.isValid() || url.scheme() != "https" || !url.userInfo().isEmpty() || url.port(443) != 443) return false;
    if (url.host() == "api.github.com") return url.path().startsWith("/repos/Nowaterisenough/Sunshine/");
    if (url.host() == "github.com") return url.path().startsWith("/Nowaterisenough/Sunshine/releases/");
    return url.host() == "release-assets.githubusercontent.com" || url.host() == "objects.githubusercontent.com";
}

QJsonObject UpdateController::selectAsset(const QJsonArray &assets, Package package) {
    QString pattern;
    switch (package) {
    case Package::WindowsInstaller: pattern = "^Sunshine\\.v[0-9.]+\\.WindowsInstaller\\.exe$"; break;
    case Package::WindowsPortable: pattern = "^Sunshine\\.v[0-9.]+\\.Portable-x64\\.zip$"; break;
    case Package::WindowsPanel: pattern = "^Sunshine-Qt-Panel-6\\.12\\.[0-9]+-win-x64\\.zip$"; break;
    case Package::MacArmPanel: pattern = "^Sunshine-Qt-Panel-6\\.12\\.[0-9]+-macos-arm64\\.zip$"; break;
    default: return {};
    }
    QJsonObject found;
    for (const auto &value : assets) {
        const auto asset = value.toObject();
        if (!QRegularExpression(pattern).match(asset.value("name").toString()).hasMatch()) continue;
        if (!found.isEmpty()) return {}; // An ambiguous release must never launch the wrong package.
        found = asset;
    }
    return found;
}

QNetworkReply *UpdateController::get(const QUrl &url, int timeout) {
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Sunshine-Qt-Panel");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(timeout);
    request.setMaximumRedirectsAllowed(5);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::UserVerifiedRedirectPolicy);
    auto *reply = m_network->get(request);
    reply->setReadBufferSize(128 * 1024);
    connect(reply, &QNetworkReply::redirected, reply, [reply](const QUrl &redirect) {
        if (trustedUrl(redirect)) reply->redirectAllowed(); else reply->abort();
    });
    m_reply = reply;
    return reply;
}

void UpdateController::fetch(const QUrl &url, std::function<void(const QByteArray &)> callback) {
    if (!trustedUrl(url)) { status("error", tr("更新地址不受信任")); return; }
    auto *reply = get(url, 15000);
    const int generation = m_generation;
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QIODevice::readyRead, this, [reply, bytes] {
        bytes->append(reply->readAll());
        if (bytes->size() > 4 * 1024 * 1024) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, generation, callback] {
        reply->deleteLater();
        if (generation != m_generation) return;
        m_reply.clear();
        if (reply->error() != QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
            status("error", tr("检查更新失败，请稍后重试：%1").arg(reply->errorString())); return;
        }
        callback(*bytes);
    });
}

void UpdateController::check() {
    if (busy() || m_state == "ready") return;
    ++m_generation; m_available = false; m_release = {}; m_asset = {};
    status("checking", tr("正在检查更新…"));
    if (m_environment.package == Package::Unsupported) { status("error", packageDescription()); return; }
    if (m_environment.commit.isEmpty() || !m_environment.commitTime.isValid()) {
        status("error", tr("开发构建缺少版本信息，请从发布页获取正式构建")); return;
    }
    fetch(QUrl(repository + "/releases?per_page=100"), [this](const QByteArray &bytes) {
        const auto document = QJsonDocument::fromJson(bytes);
        if (!document.isArray()) { status("error", tr("更新服务器返回了无效数据")); return; }
        QList<QJsonObject> releases;
        for (const auto &value : document.array()) {
            const auto release = value.toObject();
            if (release.value("draft").toBool() || (!m_includePrerelease && release.value("prerelease").toBool())) continue;
            const auto tag = release.value("tag_name").toString();
            if (!QRegularExpression("^qt-[0-9a-f]{12}$").match(tag).hasMatch()) continue;
            releases.append(release);
        }
        std::sort(releases.begin(), releases.end(), [](const auto &a, const auto &b) {
            return a.value("published_at").toString() > b.value("published_at").toString();
        });
        m_lastChecked = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        QSettings().setValue("updates/lastChecked", m_lastChecked);
        if (releases.isEmpty()) { status("current", tr("所选渠道暂无 Qt 版本")); return; }
        inspectRelease(releases.first());
    });
}

void UpdateController::inspectRelease(const QJsonObject &release) {
    m_release = release;
    const auto tag = latestVersion();
    const auto commit = release.value("target_commitish").toString();
    if (!QRegularExpression("^[0-9a-f]{40}$").match(commit).hasMatch() || !commit.startsWith(tag.mid(3))) {
        status("error", tr("发布版本信息不完整，请在发布页确认")); return;
    }
    if (commit == m_environment.commit) { status("current", tr("已是最新版本")); return; }
    // Publication dates can be newer than a local build even for old binaries.
    // GitHub's ancestry comparison prevents a late re-publication from downgrading it.
    fetch(QUrl(repository + "/compare/" + m_environment.commit + "..." + commit), [this, tag](const QByteArray &bytes) {
        const auto comparison = QJsonDocument::fromJson(bytes).object();
        const auto relation = comparison.value("status").toString();
        if (relation == "behind" || relation == "identical") { status("current", tr("当前版本已包含最新发布的更改")); return; }
        if (relation != "ahead") { status("error", tr("发布版本与当前分支不兼容，请在发布页确认")); return; }
        m_asset = selectAsset(m_release.value("assets").toArray(), m_environment.package);
        const auto url = m_asset.value("browser_download_url").toString();
        const auto size = m_asset.value("size").toInteger();
        if (m_asset.isEmpty() || url != releasePrefix + "download/" + tag + "/" + packageName() ||
            QUrl(url).fileName() != packageName() || !trustedUrl(QUrl(url)) ||
            !validDigest(m_asset.value("digest").toString()) || size <= 0 || size > maxPackageSize) {
            m_asset = {}; status("error", tr("此版本缺少匹配的安装包或 SHA-256 校验信息")); return;
        }
        m_available = true;
        status("available", tr("有新版本可用"));
        if (m_autoDownload) download();
    });
}

void UpdateController::download() {
    if (!m_available || busy() || m_state == "ready") return;
    const auto folder = m_environment.cachePath + "/" + latestVersion();
    if (!QDir().mkpath(folder)) { status("error", tr("无法创建更新下载目录")); return; }
    m_downloadPath = folder + "/" + packageName();
    m_digest = QByteArray::fromHex(m_asset.value("digest").toString().mid(7).toLatin1());
    if (verifiedFile()) { m_progress = 1; status("ready", tr("下载完成，SHA-256 校验通过")); return; }
    m_file = std::make_unique<QSaveFile>(m_downloadPath);
    if (!m_file->open(QIODevice::WriteOnly)) { status("error", tr("无法写入下载文件")); return; }
    m_bytes = 0; m_progress = 0;
    status("downloading", tr("正在下载更新…"));
    const int generation = ++m_generation;
    auto hash = std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256);
    auto *reply = get(QUrl(m_asset.value("browser_download_url").toString()), 60000);
    connect(reply, &QIODevice::readyRead, this, [this, reply, hash, generation] {
        if (generation != m_generation) return;
        const auto bytes = reply->readAll();
        m_bytes += bytes.size();
        if (m_bytes > m_asset.value("size").toInteger() || m_file->write(bytes) != bytes.size()) {
            reply->abort(); return;
        }
        hash->addData(bytes);
        m_progress = double(m_bytes) / double(m_asset.value("size").toInteger());
        emit changed();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, hash, generation] {
        reply->deleteLater();
        if (generation != m_generation) return;
        m_reply.clear();
        const bool valid = reply->error() == QNetworkReply::NoError &&
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
            m_bytes == m_asset.value("size").toInteger() && hash->result() == m_digest;
        if (!valid || !m_file->commit()) {
            m_file->cancelWriting(); m_file.reset();
            status("error", tr("下载失败或校验不通过，请重试")); return;
        }
        m_file.reset(); m_progress = 1;
        status("ready", tr("下载完成，SHA-256 校验通过"));
    });
}

bool UpdateController::verifiedFile() const {
    QFile file(m_downloadPath);
    if (!file.open(QIODevice::ReadOnly) || file.size() != m_asset.value("size").toInteger()) return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return hash.addData(&file) && hash.result() == m_digest;
}

void UpdateController::cancel() {
    if (!busy()) return;
    ++m_generation;
    if (m_reply) { m_reply->abort(); m_reply.clear(); }
    if (m_file) { m_file->cancelWriting(); m_file.reset(); }
    m_progress = 0;
    status(m_available ? "available" : "idle", tr("已取消，可稍后重试"));
}

void UpdateController::openDownloaded() {
    if (m_state != "ready") return;
    if (!verifiedFile()) { status("error", tr("下载文件已变化，请重新下载")); return; }
    if (!m_environment.openPackage || !m_environment.openPackage(m_downloadPath, canInstall())) {
        status("ready", tr("未能打开更新包，或已取消系统授权；可以重试")); return;
    }
    if (canInstall()) emit installerStarted();
}

void UpdateController::openRelease() {
    const auto url = m_release.value("html_url").toString();
    QDesktopServices::openUrl(QUrl(url.startsWith(releasePrefix + "tag/qt-") && trustedUrl(QUrl(url)) ? url : releasePrefix));
}
