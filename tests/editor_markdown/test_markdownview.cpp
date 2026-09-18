// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "markdownview.h"
#include "themeserializer.h"

#include <QApplication>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaEnum>
#include <QMetaType>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

// 分支清单（来源：markdownview.cpp）
// ctor: 连接 bridge 四信号 + renderProcessTerminated（无分支）
// init():
//   I1: profile 无 mdimgHandlerInstalled 属性 → installUrlSchemeHandler + setProperty
//   I2: 属性已为 true（同 profile 第二个视图）→ 跳过安装
//   I3: load(qrc:/editor/markdown/web/milkdown.html)
//   I4: page urlChanged → http/https → openUrl + load(qrc)；其他 scheme → 仅告警
// onBridgeReady():
//   R1: m_hasPendingMd → 补发内容并清标记
//   R2: m_hasLayout → 补发布局
//   R3: m_hasTheme → 补发主题
//   R4: m_hasScroll（含 0.0 有效）→ 补发滚动
//   R5: emit ready()
// setMarkdown/setLayout/applyTheme/scrollToRatio: 缓存 + 即时 emit（无分支，参数与标记）
// setMode: mode==Editable → true，否则 false（含越界值）
// openLinkRequested lambda: scheme http/https → QDesktopServices::openUrl；否则拦截
// renderProcessTerminated lambda: m_ready=false + 300ms 后 Reload
// scrollRatio(): h<=0 → 0.0；h>0 → clampRatio(scrollY/h)
//
// 用例映射：
// - Constructor_BareCreation_BridgeChildNotReady                       → ctor
// - BridgeSignals_ForwardedToViewSignals_PayloadIntact                  → ctor 连接
// - SetMarkdown_BeforeReady_EmitsAndCachesForReplay                    → setMarkdown/R1
// - SetMode_ReadOnlyEditableAndInvalid_EmitsMappedFlag                  → setMode 分支
// - ApplyTheme_DarkAndLightMaps_EmitsSerializedJsonAndFlag（TEST_P 2）  → applyTheme
// - SetLayout_MaxWidthAndCenter_EmitsExactParams                        → setLayout/R2
// - ScrollToRatio_BoundaryValues_EmitsExactRatio（TEST_P 3）             → scrollToRatio/R4
// - OnBridgeReady_FullCache_ReplaysAllAndEmitsReady                     → R1-R5
// - OnBridgeReady_ZeroRatioValid_ReplaysZeroScroll                      → R4（0.0 边界）
// - OnBridgeReady_NoCache_EmitsOnlyReady                                 → R1-R4 反例
// - ScrollRatio_EmptyContents_ReturnsZero                                → h<=0
// - ScrollRatio_NonEmptyContents_ReturnsClampedRatio                     → h>0 + clamp
// - OpenLink_HttpAndHttpsSchemes_DelegatedToDesktopServices             → 白名单
// - OpenLink_NonHttpScheme_BlockedWithoutDelegation                      → 拦截
// - RenderProcessTerminated_AliveView_MarksUnreadyAndReloads            → 恢复 lambda
// - Init_FreshProfile_InstallsHandlerRegistersChannelLoadsQrc           → I1/I3
// - Init_SecondViewSameProfile_SkipsHandlerReinstall                     → I2
// - UrlChanged_ExternalHttp_DelegatesToBrowserAndReloadsRenderPage       → I4 白名单
// - UrlChanged_NonHttpScheme_StaysInRenderPage                            → I4 反例
//
// 最小清单自检：1 公开方法全覆盖（ctor/dtor/init/bridge/isReady/setMarkdown/setMode/
// applyTheme/setLayout/scrollToRatio/scrollRatio；onBridgeReady private 经 bridge 公开槽触发）
// ✔ 2 等价类 ✔ 3 边界（ratio 0/1、mode 越界、空缓存）✔ 4 TEST_P ×2（≥3 组）✔
// 5 分支映射 ✔ 6 I1-I4/R1-R5/模式分支全覆盖 ✔ 7 无异常路径 8 越界/拦截负面 ✔
// 9 fail/拦截后状态一致 ✔ 10 QDesktopServices/openUrl、QWebChannel::registerObject、
// QWebEngineView::load、QWebEnginePage::contentsSize/scrollPosition、profile 安装等
// 外部依赖一律 stub_ext（虚函数 triggerAction 用 VADDR）；QSignalSpy 验信号；不与 gMock 混用

class MarkdownViewTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // QWidget 派生类必须 QApplication；offscreen 平台不产生真实窗口
        if (!QApplication::instance()) {
            static int argc = 1;
            static char argv0[] = "test_markdownview";
            static char *argv[] = { argv0, nullptr };
            s_app = new QApplication(argc, argv);
        }
        // renderProcessTerminated 信号参数类型（直接调用需注册）
        qRegisterMetaType<QWebEnginePage::RenderProcessTerminationStatus>(
            "QWebEnginePage::RenderProcessTerminationStatus");
    }

    static void TearDownTestSuite()
    {
        // 各用例 TearDown 已删除视图；销毁应用保证 WebEngine 干净退出码
        if (s_app) {
            s_app->processEvents(QEventLoop::AllEvents, 100);
            delete s_app;
            s_app = nullptr;
        }
    }

    void SetUp() override
    {
        stub.clear();
        openUrlCount = 0;
        loadCount = 0;
        registerCount = 0;
        installCount = 0;
        reloadCount = 0;
        lastOpenedUrl.clear();
        lastLoadedUrl.clear();

        // QDesktopServices::openUrl 静态：禁止真实调起系统浏览器
        stub.set_lamda(&QDesktopServices::openUrl,
                       [this](const QUrl &url) -> bool {
                           ++openUrlCount;
                           lastOpenedUrl = url;
                           return true;
                       });
        // load 重载：精确 static_cast 到 QUrl 版本
        stub.set_lamda(static_cast<void (QWebEngineView::*)(const QUrl &)>(&QWebEngineView::load),
                       [this](QWebEngineView *, const QUrl &url) {
                           ++loadCount;
                           lastLoadedUrl = url;
                       });
        // WebChannel 注册（避免真实 JS 侧握手状态）
        stub.set_lamda(&QWebChannel::registerObject,
                       [this](QWebChannel *, const QString &id, QObject *) {
                           ++registerCount;
                           lastRegisteredId = id;
                       });
        // mdimg handler 安装（真实安装无必要，计数区分 I1/I2）
        stub.set_lamda(&QWebEngineProfile::installUrlSchemeHandler,
                       [this](QWebEngineProfile *, const QByteArray &, QWebEngineUrlSchemeHandler *) {
                           ++installCount;
                       });
        // 崩溃恢复 Reload（虚函数 → VADDR）
        stub.set_lamda(VADDR(QWebEnginePage, triggerAction),
                       [this](QWebEnginePage *, QWebEnginePage::WebAction action, bool) {
                           if (action == QWebEnginePage::Reload)
                               ++reloadCount;
                       });

        obj = new MarkdownView();
    }

    void TearDown() override
    {
        delete obj;   // 触发析构函数覆盖；QWebEngineView/页面递归删除
        obj = nullptr;
        stub.clear();
    }

    static QApplication *s_app;
    stub_ext::StubExt stub;
    MarkdownView *obj = nullptr;
    int openUrlCount = 0;
    int loadCount = 0;
    int registerCount = 0;
    int installCount = 0;
    int reloadCount = 0;
    QUrl lastOpenedUrl;
    QUrl lastLoadedUrl;
    QString lastRegisteredId;
};

QApplication *MarkdownViewTest::s_app = nullptr;

TEST_F(MarkdownViewTest, Constructor_BareCreation_BridgeChildNotReady)
{
    // Arrange & Act（SetUp 已构造，未 init）

    // Assert：bridge 为子对象且未就绪（构造分离契约：不启动 WebEngine 渲染）
    EXPECT_NE(obj->bridge(), nullptr);
    EXPECT_EQ(obj->bridge()->parent(), obj);
    EXPECT_FALSE(obj->isReady());   // 期望 false 边：ready 仅在 onBridgeReady 后
}

TEST_F(MarkdownViewTest, ModeEnum_MetaObjectRegistration_KeysAndValuesMapped)
{
    // Arrange：Q_ENUM(Mode) 生成的元信息（QMetaEnum/QMetaType 注册路径）

    // Act
    const QMetaEnum me = QMetaEnum::fromType<MarkdownView::Mode>();
    const QMetaType mt = QMetaType::fromType<MarkdownView::Mode>();

    // Assert：枚举↔字符串双向映射与所属元对象
    EXPECT_EQ(me.keyCount(), 2);
    EXPECT_STREQ(me.valueToKey(MarkdownView::ReadOnly), "ReadOnly");
    EXPECT_STREQ(me.valueToKey(MarkdownView::Editable), "Editable");
    EXPECT_EQ(me.keyToValue("Editable"), int(MarkdownView::Editable));
    EXPECT_EQ(mt.metaObject(), &MarkdownView::staticMetaObject);

    // QVariant 打包/解包枚举（QMetaType 名称注册路径）
    const QVariant packed = QVariant::fromValue(MarkdownView::Editable);
    EXPECT_EQ(packed.userType(), mt.id());
    EXPECT_EQ(packed.value<MarkdownView::Mode>(), MarkdownView::Editable);
    EXPECT_EQ(mt.name(), QByteArray("MarkdownView::Mode"));

    // Q_ENUM 生成的 friend 辅助函数（ADL 可达；constexpr 常规路径编译期折叠，
    // 显式调用保证运行时执行覆盖）
    EXPECT_STREQ(qt_getEnumName(MarkdownView::ReadOnly), "Mode");
    EXPECT_EQ(qt_getEnumMetaObject(MarkdownView::Editable), &MarkdownView::staticMetaObject);
}

TEST_F(MarkdownViewTest, BridgeSignals_ForwardedToViewSignals_PayloadIntact)
{
    // Arrange：ctor 中 bridge→view 的 contentChanged/scrollRatioChanged 转发
    QSignalSpy contentSpy(obj, &MarkdownView::contentChanged);
    QSignalSpy scrollSpy(obj, &MarkdownView::scrollRatioChanged);

    // Act：经 bridge 公开槽注入（JS 侧来源等价路径）
    obj->bridge()->onContentChanged(QStringLiteral("# live"));
    obj->bridge()->onScrollRatio(0.4);

    // Assert
    ASSERT_EQ(contentSpy.count(), 1);
    EXPECT_EQ(contentSpy.at(0).at(0).toString(), QString("# live"));
    ASSERT_EQ(scrollSpy.count(), 1);
    EXPECT_DOUBLE_EQ(scrollSpy.at(0).at(0).toDouble(), 0.4);
}

TEST_F(MarkdownViewTest, SetMarkdown_BeforeReady_EmitsAndCachesForReplay)
{
    // Arrange
    QSignalSpy mdSpy(obj->bridge(), &MarkdownBridge::setMarkdownRequested);

    // Act：ready 前推送（JS 未接线，协议层双保险缓存）
    obj->setMarkdown(QStringLiteral("# cached"));

    // Assert：即时 emit 一次 + 未就绪（缓存待补发）
    EXPECT_EQ(mdSpy.count(), 1);
    EXPECT_EQ(mdSpy.at(0).at(0).toString(), QString("# cached"));
    EXPECT_FALSE(obj->isReady());

    // Act 2：内核就绪 → 补发缓存内容
    QSignalSpy readySpy(obj, &MarkdownView::ready);
    obj->bridge()->onReady();

    // Assert 2：累计两次（即时+补发）且 ready
    EXPECT_EQ(mdSpy.count(), 2);
    EXPECT_EQ(mdSpy.at(1).at(0).toString(), QString("# cached"));
    EXPECT_EQ(readySpy.count(), 1);
    EXPECT_TRUE(obj->isReady());   // 期望 true 边：onBridgeReady 置位
}

TEST_F(MarkdownViewTest, SetMode_ReadOnlyEditableAndInvalid_EmitsMappedFlag)
{
    // Arrange
    QSignalSpy modeSpy(obj->bridge(), &MarkdownBridge::setModeRequested);

    // Act：ReadOnly(0) / Editable(1) / 越界值(99)
    obj->setMode(MarkdownView::ReadOnly);
    obj->setMode(MarkdownView::Editable);
    obj->setMode(99);

    // Assert：映射为 editable 布尔，仅 Editable(1) 为 true
    ASSERT_EQ(modeSpy.count(), 3);
    EXPECT_FALSE(modeSpy.at(0).at(0).toBool());
    EXPECT_TRUE(modeSpy.at(1).at(0).toBool());
    EXPECT_FALSE(modeSpy.at(2).at(0).toBool());   // 越界按非 Editable 处理
}

// —— applyTheme 深浅两态（同断言逻辑成组）——
namespace {
struct ThemeCase {
    QString bg;
    QString fg;
    bool expectedDark;
};
const ThemeCase kThemeCases[] = {
    { "#1e1e1e", "#eeeeee", true },
    { "#ffffff", "#1f1f1f", false },
    { "#252525", "#d0d0d0", true },
};
} // namespace

class ViewThemeParamTest : public MarkdownViewTest,
                           public ::testing::WithParamInterface<ThemeCase> {
};

TEST_P(ViewThemeParamTest, ApplyTheme_DarkAndLightMaps_EmitsSerializedJsonAndFlag)
{
    // Arrange
    const auto &c = GetParam();
    QVariantMap theme;
    QVariantMap ec;
    ec["background-color"] = c.bg;
    ec["text-color"] = c.fg;
    theme["editor-colors"] = ec;
    const QString expectedJson = ThemeSerializer::serialize(theme);
    QSignalSpy themeSpy(obj->bridge(), &MarkdownBridge::applyThemeRequested);

    // Act
    obj->applyTheme(theme);

    // Assert：序列化 JSON 与深浅标志精确（阶段 3 ThemeSerializer 集成契约）
    EXPECT_EQ(themeSpy.count(), 1);
    EXPECT_EQ(themeSpy.at(0).at(0).toString(), expectedJson);
    EXPECT_EQ(themeSpy.at(0).at(1).toBool(), c.expectedDark);
    EXPECT_EQ(ThemeSerializer::isDark(theme), c.expectedDark);
}

INSTANTIATE_TEST_SUITE_P(ViewThemes, ViewThemeParamTest,
                         ::testing::ValuesIn(kThemeCases));

TEST_F(MarkdownViewTest, SetLayout_MaxWidthAndCenter_EmitsExactParams)
{
    // Arrange
    QSignalSpy layoutSpy(obj->bridge(), &MarkdownBridge::setLayoutRequested);

    // Act：居中约束与半幅无限宽两种布局
    obj->setLayout(800, true);
    obj->setLayout(0, false);

    // Assert
    ASSERT_EQ(layoutSpy.count(), 2);
    EXPECT_EQ(layoutSpy.at(0).at(0).toInt(), 800);
    EXPECT_TRUE(layoutSpy.at(0).at(1).toBool());
    EXPECT_EQ(layoutSpy.at(1).at(0).toInt(), 0);   // 0=不限最大宽边界
    EXPECT_FALSE(layoutSpy.at(1).at(1).toBool());
}

// —— scrollToRatio 边界值成组（0/中点/1）——
namespace {
struct ViewRatioCase {
    double ratio;
};
const ViewRatioCase kViewRatioCases[] = {
    { 0.0 },
    { 0.5 },
    { 1.0 },
};
} // namespace

class ViewRatioParamTest : public MarkdownViewTest,
                           public ::testing::WithParamInterface<ViewRatioCase> {
};

TEST_P(ViewRatioParamTest, ScrollToRatio_BoundaryValues_EmitsExactRatio)
{
    // Arrange
    const double ratio = GetParam().ratio;
    QSignalSpy scrollSpy(obj->bridge(), &MarkdownBridge::scrollToRatioRequested);

    // Act
    obj->scrollToRatio(ratio);

    // Assert
    EXPECT_EQ(scrollSpy.count(), 1);
    EXPECT_DOUBLE_EQ(scrollSpy.at(0).at(0).toDouble(), ratio);
}

INSTANTIATE_TEST_SUITE_P(ViewRatioBoundaries, ViewRatioParamTest,
                         ::testing::ValuesIn(kViewRatioCases));

TEST_F(MarkdownViewTest, OnBridgeReady_FullCache_ReplaysAllAndEmitsReady)
{
    // Arrange：ready 前塞满四类缓存（内容/布局/主题/滚动）
    QSignalSpy mdSpy(obj->bridge(), &MarkdownBridge::setMarkdownRequested);
    QSignalSpy layoutSpy(obj->bridge(), &MarkdownBridge::setLayoutRequested);
    QSignalSpy themeSpy(obj->bridge(), &MarkdownBridge::applyThemeRequested);
    QSignalSpy scrollSpy(obj->bridge(), &MarkdownBridge::scrollToRatioRequested);
    QSignalSpy readySpy(obj, &MarkdownView::ready);
    QVariantMap theme;
    QVariantMap ec;
    ec["background-color"] = "#1e1e1e";
    theme["editor-colors"] = ec;
    obj->setMarkdown(QStringLiteral("body"));
    obj->setLayout(800, true);
    obj->applyTheme(theme);
    obj->scrollToRatio(0.25);
    EXPECT_EQ(mdSpy.count(), 1);   // 前置即时 emit

    // Act：内核就绪
    obj->bridge()->onReady();

    // Assert：四类全部补发（即时 1 次 + 补发 1 次 = 2）+ ready 恰一次
    EXPECT_EQ(mdSpy.count(), 2);
    EXPECT_EQ(mdSpy.at(1).at(0).toString(), QString("body"));
    EXPECT_EQ(layoutSpy.count(), 2);
    EXPECT_EQ(layoutSpy.at(1).at(0).toInt(), 800);
    EXPECT_TRUE(layoutSpy.at(1).at(1).toBool());
    EXPECT_EQ(themeSpy.count(), 2);
    EXPECT_EQ(themeSpy.at(1).at(1).toBool(), true);
    EXPECT_EQ(scrollSpy.count(), 2);
    EXPECT_DOUBLE_EQ(scrollSpy.at(1).at(0).toDouble(), 0.25);
    EXPECT_EQ(readySpy.count(), 1);
    EXPECT_TRUE(obj->isReady());
}

TEST_F(MarkdownViewTest, OnBridgeReady_ZeroRatioValid_ReplaysZeroScroll)
{
    // Arrange：0.0（回顶部）是有效状态，以独立标记而非值判断
    QSignalSpy scrollSpy(obj->bridge(), &MarkdownBridge::scrollToRatioRequested);
    QSignalSpy readySpy(obj, &MarkdownView::ready);
    obj->scrollToRatio(0.0);

    // Act
    obj->bridge()->onReady();

    // Assert：0.0 未被吞掉（即时 1 次 + 补发 1 次）
    EXPECT_EQ(scrollSpy.count(), 2);
    EXPECT_DOUBLE_EQ(scrollSpy.at(1).at(0).toDouble(), 0.0);
    EXPECT_EQ(readySpy.count(), 1);
}

TEST_F(MarkdownViewTest, OnBridgeReady_NoCache_EmitsOnlyReady)
{
    // Arrange：从未 set 过任何状态
    QSignalSpy mdSpy(obj->bridge(), &MarkdownBridge::setMarkdownRequested);
    QSignalSpy layoutSpy(obj->bridge(), &MarkdownBridge::setLayoutRequested);
    QSignalSpy themeSpy(obj->bridge(), &MarkdownBridge::applyThemeRequested);
    QSignalSpy scrollSpy(obj->bridge(), &MarkdownBridge::scrollToRatioRequested);
    QSignalSpy readySpy(obj, &MarkdownView::ready);

    // Act
    obj->bridge()->onReady();

    // Assert：只发 ready，不补发任何缓存
    EXPECT_EQ(readySpy.count(), 1);
    EXPECT_EQ(mdSpy.count(), 0);
    EXPECT_EQ(layoutSpy.count(), 0);
    EXPECT_EQ(themeSpy.count(), 0);
    EXPECT_EQ(scrollSpy.count(), 0);
    EXPECT_TRUE(obj->isReady());
}

TEST_F(MarkdownViewTest, ScrollRatio_EmptyContents_ReturnsZero)
{
    // Arrange：未 load 任何内容（contentsSize 无效/负，h<=0 分支）

    // Act
    const double ratio = obj->scrollRatio();

    // Assert：除零保护 → 0.0（期望精确值）
    EXPECT_DOUBLE_EQ(ratio, 0.0);
    EXPECT_GE(ratio, 0.0);
}

TEST_F(MarkdownViewTest, ScrollRatio_NonEmptyContents_ReturnsClampedRatio)
{
    // Arrange：stub 页面几何（contentsSize/scrollPosition 均非虚属性 getter）
    stub.set_lamda(&QWebEnginePage::contentsSize,
                   [](QWebEnginePage *) -> QSizeF { return QSizeF(0, 1000); });
    stub.set_lamda(&QWebEnginePage::scrollPosition,
                   [](QWebEnginePage *) -> QPointF { return QPointF(0, 250); });

    // Act
    const double ratio = obj->scrollRatio();

    // Assert：250/1000
    EXPECT_NEAR(ratio, 0.25, 1e-12);

    // Arrange 2：越界滚动位置 → 钳制
    stub.set_lamda(&QWebEnginePage::scrollPosition,
                   [](QWebEnginePage *) -> QPointF { return QPointF(0, 1500); });

    // Act 2 & Assert 2：1500/1000=1.5 → clamp 1.0
    EXPECT_NEAR(obj->scrollRatio(), 1.0, 1e-12);
}

TEST_F(MarkdownViewTest, OpenLink_HttpAndHttpsSchemes_DelegatedToDesktopServices)
{
    // Arrange：JS 点击拦截主路径 → bridge openLinkRequested → 系统浏览器（已 stub）

    // Act
    obj->bridge()->onOpenLink(QStringLiteral("https://example.org/doc"));
    obj->bridge()->onOpenLink(QStringLiteral("http://example.org/other"));

    // Assert：白名单内两 scheme 均转交且 URL 原样
    EXPECT_EQ(openUrlCount, 2);
    EXPECT_EQ(lastOpenedUrl.toString(), QString("http://example.org/other"));
}

TEST_F(MarkdownViewTest, OpenLink_NonHttpScheme_BlockedWithoutDelegation)
{
    // Arrange：安全审查——任意协议处理程序风险，非白名单一律拦截
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString fileUrl = QUrl::fromLocalFile(dir.filePath("x.html")).toString();

    // Act
    obj->bridge()->onOpenLink(fileUrl);
    obj->bridge()->onOpenLink(QStringLiteral("javascript:alert(1)"));

    // Assert：无转交、无崩溃、就绪状态未受影响（强安全不变式）
    EXPECT_EQ(openUrlCount, 0);
    EXPECT_FALSE(obj->isReady());
}

TEST_F(MarkdownViewTest, RenderProcessTerminated_AliveView_MarksUnreadyAndReloads)
{
    // Arrange：先就绪，再模拟渲染进程终止
    obj->bridge()->onReady();
    ASSERT_TRUE(obj->isReady());

    // Act：发出 renderProcessTerminated（异步恢复经 300ms singleShot）
    QMetaObject::invokeMethod(obj, "renderProcessTerminated",
                              Q_ARG(QWebEnginePage::RenderProcessTerminationStatus,
                                    QWebEnginePage::NormalTerminationStatus),
                              Q_ARG(int, 0));
    EXPECT_FALSE(obj->isReady());   // 同步置位：期望 false 边

    // Arrange 2：等待 300ms 定时器触发（页面仍在，事件循环驱动）
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < 700 && reloadCount == 0)
        QApplication::processEvents(QEventLoop::AllEvents, 50);

    // Assert：恢复路径执行了页面 Reload
    EXPECT_EQ(reloadCount, 1);
    EXPECT_EQ(openUrlCount, 0);   // 恢复不触外部浏览器
}

TEST_F(MarkdownViewTest, Init_FreshProfile_InstallsHandlerRegistersChannelLoadsQrc)
{
    // Arrange：默认 profile 尚未安装标记（进程内首视图；若先前用例已装则走 I2 用例断言）
    QWebEngineProfile *profile = obj->page()->profile();
    profile->setProperty("mdimgHandlerInstalled", QVariant());   // 重置为无效，确保 I1 路径

    // Act
    obj->init();

    // Assert：I1 安装 + 属性置位 + 通道注册 + 加载渲染页
    EXPECT_EQ(installCount, 1);
    EXPECT_EQ(profile->property("mdimgHandlerInstalled").toBool(), true);
    EXPECT_EQ(registerCount, 1);
    EXPECT_EQ(lastRegisteredId, QString("bridge"));
    EXPECT_EQ(loadCount, 1);
    EXPECT_EQ(lastLoadedUrl.toString(), QString("qrc:/editor/markdown/web/milkdown.html"));
}

TEST_F(MarkdownViewTest, Init_SecondViewSameProfile_SkipsHandlerReinstall)
{
    // Arrange：首个视图已完成安装（属性置位）
    QWebEngineProfile *profile = obj->page()->profile();
    profile->setProperty("mdimgHandlerInstalled", true);
    MarkdownView second;

    // Act
    second.init();

    // Assert：I2 分支——不再安装，但通道注册与页面加载照常
    EXPECT_EQ(installCount, 0);
    EXPECT_EQ(registerCount, 1);
    EXPECT_EQ(loadCount, 1);
    EXPECT_EQ(profile->property("mdimgHandlerInstalled").toBool(), true);
}

TEST_F(MarkdownViewTest, UrlChanged_ExternalHttp_DelegatesToBrowserAndReloadsRenderPage)
{
    // Arrange：init（load 已 stub，不会真实导航）
    obj->init();
    const int loadBefore = loadCount;

    // Act：页面若导航到外部地址（兜底路径）
    QMetaObject::invokeMethod(obj->page(), "urlChanged",
                              Q_ARG(QUrl, QUrl(QStringLiteral("https://example.org/away"))));

    // Assert：转系统浏览器 + reload 回渲染页
    EXPECT_EQ(openUrlCount, 1);
    EXPECT_EQ(lastOpenedUrl.toString(), QString("https://example.org/away"));
    EXPECT_EQ(loadCount, loadBefore + 1);
    EXPECT_EQ(lastLoadedUrl.toString(), QString("qrc:/editor/markdown/web/milkdown.html"));
}

TEST_F(MarkdownViewTest, UrlChanged_NonHttpScheme_StaysInRenderPage)
{
    // Arrange
    obj->init();
    const int loadBefore = loadCount;
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    // Act：file 导航（非白名单 scheme）
    QMetaObject::invokeMethod(obj->page(), "urlChanged",
                              Q_ARG(QUrl, QUrl::fromLocalFile(dir.path())));

    // Assert：既不转浏览器也不 reload（保持当前渲染页）
    EXPECT_EQ(openUrlCount, 0);
    EXPECT_EQ(loadCount, loadBefore);
}
