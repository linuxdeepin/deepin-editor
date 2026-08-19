// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// it_markdownview_smoke —— 真实 WebEngine 集成冒烟（§9.1）
// 验证：qrc 页面 + file:// 本地图片 + 主题变量注入端到端可用。
// 运行需真实渲染进程：QT_QPA_PLATFORM=offscreen 下可跑，必要时
//   QTWEBENGINE_CHROMIUM_FLAGS="--no-sandbox --disable-gpu"

#include "markdown/markdownview.h"
#include "markdown/markdownbridge.h"
#include "markdown/markdownlogic.h"

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTest>
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebEnginePage>
#include <functional>

namespace {
// 抓取 Chromium console 输出，定位资源加载被拒的确切原因
class SpyPage : public QWebEnginePage
{
public:
    using QWebEnginePage::QWebEnginePage;
protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                                  int lineNumber, const QString &sourceID) override
    {
        fprintf(stderr, "[console] %s (%s:%d)\n", qPrintable(message), qPrintable(sourceID), lineNumber);
    }
};

// 等待条件成立（事件循环驱动），超时返回 false
bool waitFor(std::function<bool()> cond, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < timeoutMs) {
        if (cond()) return true;
        QApplication::processEvents(QEventLoop::AllEvents, 50);
        QTest::qWait(10);
    }
    return cond();
}
}

// 端到端：ready → setMarkdown（含 file:// 图片）→ 图片实际解码加载
TEST(IT_MarkdownViewSmoke, DISABLED_RenderLocalImage_EndToEnd)
{
    const QString imgDir = QStringLiteral("/home/uos/work/deepin-editor/docs/markdown-demo");
    if (!QFileInfo::exists(imgDir + QStringLiteral("/sample.png"))) {
        GTEST_SKIP() << "sample.png not present, skip";
    }

    MarkdownView v;
    SpyPage *page = new SpyPage(&v);   // 接管 page 以抓 console
    v.setPage(page);
    v.init();
    QSignalSpy readySpy(v.bridge(), &MarkdownBridge::ready);
    ASSERT_TRUE(waitFor([&readySpy]() { return readySpy.count() > 0; }, 15000))
            << "WebEngine page not ready in 15s";

    // 模拟 EditWrapper 完整链路：相对路径经 resolveImagePaths 改写为 mdimg://（自定义 scheme 供给本地文件）
    const QString md = MarkdownLogic::resolveImagePaths(
        QStringLiteral("# 标题\n\n![渐变图](sample.png)\n\n正文。"), imgDir);
    qInfo() << "[smoke] rewritten md:" << md;
    v.setMarkdown(md);
    // 等渲染 + 图片子资源加载
    QTest::qWait(3000);

    QString json;
    bool ok = false;
    v.page()->runJavaScript(
        QStringLiteral("JSON.stringify(Array.from(document.images).map(i=>"
                       "({src:i.src, complete:i.complete, w:i.naturalWidth, h:i.naturalHeight})))"),
        [&json, &ok](const QVariant &result) { json = result.toString(); ok = true; });
    ASSERT_TRUE(waitFor([&ok]() { return ok; }, 5000));
    qInfo() << "[smoke] images:" << json;

    EXPECT_TRUE(json.contains(QStringLiteral("\"w\":400")))      // 真实解码出原始宽度
            << "image not decoded/rendered: " << json.toStdString();
}

// 端到端：外部链接点击不在预览内导航，经 bridge 上抛 openLinkRequested（JS 拦截主路径）
TEST(IT_MarkdownViewSmoke, DISABLED_ExternalLinkClick_DelegatedToBrowser)
{
    MarkdownView v;
    SpyPage *page = new SpyPage(&v);
    v.setPage(page);
    v.init();
    QSignalSpy readySpy(v.bridge(), &MarkdownBridge::ready);
    ASSERT_TRUE(waitFor([&readySpy]() { return readySpy.count() > 0; }, 15000))
            << "WebEngine page not ready in 15s";

    v.setMarkdown(QStringLiteral("# 链接测试\n\n[deepin 官网](https://www.deepin.org/index.shtml)\n"));
    QTest::qWait(1500);

    // 模拟点击渲染出的 <a>：JS 捕获拦截 → preventDefault → bridge.onOpenLink 上抛
    QSignalSpy linkSpy(v.bridge(), &MarkdownBridge::openLinkRequested);
    bool anchorFound = false;
    v.page()->runJavaScript(
        QStringLiteral("(function(){const a=document.querySelector('a[href]');"
                       "if(!a) return 'no-anchor'; a.click(); return 'clicked';})()"),
        [&anchorFound](const QVariant &result) {
            anchorFound = result.toString() != QLatin1String("no-anchor");
        });
    ASSERT_TRUE(waitFor([&anchorFound]() { return anchorFound; }, 5000)) << "anchor not rendered";

    ASSERT_TRUE(waitFor([&linkSpy]() { return linkSpy.count() > 0; }, 5000))
            << "openLinkRequested not emitted (click interception broken)";
    EXPECT_EQ(linkSpy.takeFirst().at(0).toString(),
              QStringLiteral("https://www.deepin.org/index.shtml"));
}

// 端到端时序复现（初始对齐）：ready 之前发出的 scrollToRatio 不得丢失，
// 页面 ready + 首次渲染完成后应应用该比例（生产：恢复光标后左栏非顶部 → setViewMode 发真实比例）
TEST(IT_MarkdownViewSmoke, DISABLED_ScrollRatioBeforeReady_AppliedAfterRender)
{
    const QString imgDir = QStringLiteral("/home/uos/work/deepin-editor/docs/markdown-demo");

    MarkdownView v;
    SpyPage *page = new SpyPage(&v);
    v.setPage(page);
    v.init();

    // —— 不等 ready，模拟生产时序：内容 + 滚动比例先到 ——
    QString longMd = QStringLiteral("# t\n\n");
    for (int i = 0; i < 200; ++i)
        longMd += QStringLiteral("段落 %1 一些内容文本。\n\n").arg(i);
    v.setMarkdown(longMd);
    v.scrollToRatio(0.5);          // 左栏恢复光标后在中间

    QSignalSpy readySpy(v.bridge(), &MarkdownBridge::ready);
    ASSERT_TRUE(waitFor([&readySpy]() { return readySpy.count() > 0; }, 15000))
            << "WebEngine page not ready in 15s";

    // 等首次渲染 + reapplyScroll 生效
    QTest::qWait(2500);

    QString json;
    bool ok = false;
    v.page()->runJavaScript(
        QStringLiteral("(function(){const m=document.documentElement.scrollHeight-window.innerHeight;"
                       "return JSON.stringify({y:window.scrollY,max:m});})()"),
        [&json, &ok](const QVariant &result) { json = result.toString(); ok = true; });
    ASSERT_TRUE(waitFor([&ok]() { return ok; }, 5000));
    qInfo() << "[smoke] scroll:" << json;

    // 解析并断言实际滚动比例 ≈ 0.5（允许 ±0.1 容差）
    const auto doc = QJsonDocument::fromJson(json.toUtf8());
    const double y = doc.object().value("y").toDouble();
    const double max = doc.object().value("max").toDouble();
    ASSERT_GT(max, 0);
    EXPECT_NEAR(y / max, 0.5, 0.1)
            << "initial scroll ratio lost: " << json.toStdString();
}
