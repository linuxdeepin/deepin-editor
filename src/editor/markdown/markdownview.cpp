// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "markdownview.h"
#include "themeserializer.h"
#include "scrollsync.h"
#include <QTimer>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlScheme>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestJob>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDesktopServices>
#include <QVariant>

namespace {
// mdimg scheme 必须在 QApplication 构造前注册（静态初始化先于 main，且本翻译单元
// 由 CMake glob 强制收编、被 EditWrapper 引用，不存在"未被拉入"的时序风险），
// LocalAccessAllowed 允许 qrc 来源的渲染页请求本 scheme（file:// 被 Chromium 策略硬禁）
const bool kMdImgSchemeRegistered = []() {
    QWebEngineUrlScheme scheme("mdimg");
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    // 不注册 LocalScheme：Chromium 对 local 类 scheme 有 qrc→file 同款来源限制；
    // 注册为普通 SecureScheme 时 <img> 跨源加载无需 CORS，请求直达我们的 handler
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme
                    | QWebEngineUrlScheme::CorsEnabled
                    | QWebEngineUrlScheme::ServiceWorkersAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
    return true;
}();

// mdimg:///abs/path → 读本地文件回给渲染进程（图片等静态资源）
class MarkdownImageHandler : public QWebEngineUrlSchemeHandler
{
public:
    using QWebEngineUrlSchemeHandler::QWebEngineUrlSchemeHandler;

protected:
    void requestStarted(QWebEngineUrlRequestJob *job) override
    {
        const QUrl url = job->requestUrl();
        // Blink 对空 host 的标准 scheme URL 会把绝对路径首段规范化为 host
        // （mdimg:///home/x → mdimg://home/x），此处还原完整本地路径
        QString path = url.path();
        if (!url.host().isEmpty())
            path = QLatin1Char('/') + url.host() + path;
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isFile()) {
            qWarning() << "mdimg: resource not found:" << path;
            job->fail(QWebEngineUrlRequestJob::UrlNotFound);
            return;
        }
        // 路径遍历防护（安全审查 2026-08-19）：恶意 md 可构造 mdimg://../.. 类 URL。
        // 1. canonicalFilePath 规范化路径（解析 .. 与符号链接），拒绝规范化失败；
        // 2. 不做目录白名单（md 文件可位于任意挂载点，图片相对其目录解析，限制会破坏
        //    合法功能），改以 MIME 白名单收窄暴露面：mdimg 仅服务 <img> 渲染，
        //    非 image/* 一律拒绝——文本类敏感文件（如 /etc/passwd）无法被读取
        const QString canonical = fi.canonicalFilePath();
        const QString mime = QMimeDatabase().mimeTypeForFile(fi).name();
        if (canonical.isEmpty() || !mime.startsWith(QLatin1String("image/"))) {
            qWarning() << "mdimg: request denied (traversal or non-image):" << path << "mime:" << mime;
            job->fail(QWebEngineUrlRequestJob::RequestDenied);
            return;
        }
        QFile *file = new QFile(canonical, job);
        if (!file->open(QIODevice::ReadOnly)) {
            job->fail(QWebEngineUrlRequestJob::UrlNotFound);
            return;
        }
        job->reply(mime.toUtf8(), file);
    }
};
} // namespace

MarkdownView::MarkdownView(QWidget *parent)
    : QWebEngineView(parent)
    , m_bridge(new MarkdownBridge(this))
{
    connect(m_bridge, &MarkdownBridge::ready, this, &MarkdownView::onBridgeReady);
    connect(m_bridge, &MarkdownBridge::contentChanged, this, &MarkdownView::contentChanged);
    connect(m_bridge, &MarkdownBridge::scrollRatioChanged, this, &MarkdownView::scrollRatioChanged);
    // 外部链接转交系统浏览器打开，不在预览内导航（JS 点击拦截的主路径）。
    // scheme 白名单（安全审查 2026-08-19）：仅放行 http/https——JS 传入的 url 来自
    // md 内容，若直接交 QDesktopServices 会触发系统注册的任意协议处理程序
    // （如 file://、自定义协议），构成命令执行/敏感操作风险
    connect(m_bridge, &MarkdownBridge::openLinkRequested, this, [](const QString &urlStr) {
        const QUrl url(urlStr);
        const QString scheme = url.scheme().toLower();
        if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
            qInfo() << "MarkdownView: open external link in system browser:" << urlStr;
            QDesktopServices::openUrl(url);
        } else {
            qWarning() << "MarkdownView: blocked link with non-whitelisted scheme:" << urlStr;
        }
    });
    // §4 阶段 4 崩溃恢复：WebEngine 渲染进程终止时重新加载页面
    connect(this, &QWebEngineView::renderProcessTerminated, this, [this](QWebEnginePage::RenderProcessTerminationStatus, int) {
        qWarning() << "MarkdownView: render process terminated, reloading";
        m_ready = false;
        QTimer::singleShot(300, this, [this]() {
            page()->triggerAction(QWebEnginePage::Reload);
            // reload 后 bridge.onReady 会重新触发，重新 setMarkdown 由上层 RenderThrottle 的 ready 钩子处理
        });
    });
}

MarkdownView::~MarkdownView() = default;

void MarkdownView::init()
{
    m_channel = new QWebChannel(this);
    m_channel->registerObject(QStringLiteral("bridge"), m_bridge);
    page()->setWebChannel(m_channel);
    // 页面来源为 qrc:/：Chromium 硬禁 file:// 子资源（Not allowed to load local resource），
    // 本地图片改写为 mdimg:// 自定义 scheme，由下方 handler 在 Qt 侧供给文件内容
    QWebEngineProfile *profile = page()->profile();
    // 按显式存在性+取值判断（不依赖无效 QVariant::toBool() 恒 false 的隐式行为）
    const QVariant installed = profile->property("mdimgHandlerInstalled");
    if (!installed.isValid() || !installed.toBool()) {
        profile->installUrlSchemeHandler("mdimg", new MarkdownImageHandler(profile));
        profile->setProperty("mdimgHandlerInstalled", true);
    }
    settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    // 兜底（JS 点击拦截失效时）：页面若已导航到外部地址，转系统浏览器并 reload 回渲染页。
    // reload 后 onReady 重新触发，缓存四项（内容/布局/主题/滚动）按 §5.3 自动重放。
    // 不替换 page 子类：冒烟测试需在 init() 前 setPage(SpyPage) 抓 console。
    connect(page(), &QWebEnginePage::urlChanged, this, [this](const QUrl &url) {
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
            qWarning() << "MarkdownView: external navigation blocked, delegating to browser:" << url;
            QDesktopServices::openUrl(url);
            load(QUrl(QStringLiteral("qrc:/editor/markdown/web/milkdown.html")));
        }
    });
    load(QUrl(QStringLiteral("qrc:/editor/markdown/web/milkdown.html")));
}

void MarkdownView::onBridgeReady()
{
    m_ready = true;
    // 页面（重新）就绪时补发缓存状态：JS 未接线期间到达的请求全部丢失（§5.3 时序）。
    // 顺序：内容 → 布局 → 主题 → 滚动（内容先渲染出高度，滚动比例才有意义；
    // JS 侧 renderMarkdown 完成后还会重放 lastRequestedRatio 二次对齐）
    if (m_hasPendingMd) {
        m_hasPendingMd = false;
        emit m_bridge->setMarkdownRequested(m_pendingMd);
    }
    if (m_hasLayout)
        emit m_bridge->setLayoutRequested(m_lastLayoutMaxW, m_lastLayoutCenter);
    if (m_hasTheme)
        emit m_bridge->applyThemeRequested(m_lastThemeJson, m_lastThemeDark);
    // 比例为 0.0（回顶部）同样是有效状态，须与其他三项一样以独立标记判定，
    // 不可用 > 0 判断（否则回顶部语义被静默吞掉，安全/代码审查 2026-08-19）
    if (m_hasScroll)
        emit m_bridge->scrollToRatioRequested(m_lastScrollRatio);
    emit ready();
}

void MarkdownView::setMarkdown(const QString &md)
{
    // ready 前缓存，onBridgeReady 补发（上层 RenderThrottle 亦有缓存，此处为协议层双保险）
    m_pendingMd = md;
    m_hasPendingMd = true;
    emit m_bridge->setMarkdownRequested(md);
}

void MarkdownView::setMode(int mode)
{
    emit m_bridge->setModeRequested(mode == Editable);
}

void MarkdownView::applyTheme(const QVariantMap &themeMap)
{
    // §4.7：经 ThemeSerializer 序列化 themeMap 为 CSS 变量 JSON + 深浅色，emit 给 bridge。
    // 同时缓存：ready 前发出的信号 JS 侧尚未接线，onBridgeReady 时补发（§5.3 时序）
    const QString json = ThemeSerializer::serialize(themeMap);
    const bool dark = ThemeSerializer::isDark(themeMap);
    m_lastThemeJson = json;
    m_lastThemeDark = dark;
    m_hasTheme = true;
    emit m_bridge->applyThemeRequested(json, dark);
}

void MarkdownView::setLayout(int maxContentWidth, bool center)
{
    m_lastLayoutMaxW = maxContentWidth;
    m_lastLayoutCenter = center;
    m_hasLayout = true;
    emit m_bridge->setLayoutRequested(maxContentWidth, center);
}

void MarkdownView::scrollToRatio(double ratio)
{
    // 缓存：ready 前到达的初始对齐比例不丢失（恢复光标后左栏非 0 时初始同步）；
    // 以标记而非值判断有效性，0.0（回顶部）同样是可重放状态
    m_lastScrollRatio = ratio;
    m_hasScroll = true;
    emit m_bridge->scrollToRatioRequested(ratio);
}

// 反向：读取当前页面滚动比例（右栏滚动回传，供上层同步左栏）
// 注：QWebEngine 异步，同步读取仅作粗略估计；精准同步走 JS scroll 事件 → onScrollRatio 槽。
double MarkdownView::scrollRatio() const
{
    QSizeF contents = page()->contentsSize();
    double h = contents.height();
    if (h <= 0) return 0.0;
    return ScrollSync::clampRatio(page()->scrollPosition().y() / h);
}
