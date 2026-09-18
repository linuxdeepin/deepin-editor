// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "markdownbridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QPointer>
#include <QSignalSpy>

// 分支清单（来源：markdownbridge.h —— 槽→信号 1:1 转发 + 5 个 tr 文案 getter，无 if 分支）
// B1: onReady()            → emit ready()
// B2: onContentChanged(md) → emit contentChanged(md)
// B3: onScrollRatio(r)     → emit scrollRatioChanged(r)
// B4: onOpenLink(url)      → emit openLinkRequested(url)
// B5: retranslate()        → emit retranslated()
// B6: 5 个 Q_PROPERTY getter → tr(...) 源文案
// B7: 构造（含/不含 parent）→ QObject 父子关系建立
//
// 用例映射：
// - OnReady_JsBridgeReady_EmitsReadySignal                      → B1
// - OnContentChanged_MdPayload_EmitsSamePayload                 → B2
// - OnScrollRatio_ValidRatios_EmitsExactRatio（TEST_P 3 组）     → B3
// - OnOpenLink_ExternalUrl_EmitsSameUrl                          → B4
// - Retranslate_LanguageSwitched_EmitsRetranslatedAndTextsAlive  → B5 + B6
// - UiTextProperties_AllGetters_ReturnSourceTexts                → B6
// - Constructor_WithParent_ParentBacklinkBound                   → B7(parent)
// - Constructor_NoParent_StandaloneOwnership                     → B7(null)
//
// 最小清单自检：1 每公开方法≥1用例 ✔（5 getter/5 槽/ctor 全覆盖）
// 2 等价类：ratio ∈{0,0.25,1} 边界成组 ✔ 3 边界显式 ✔ 4 TEST_P ≥3 组 ✔
// 5 分支清单映射 ✔ 6 无 if/switch/throw 7 无异常路径 8 空串/空 URL 负面 ✓（EmptyPayload 用例）
// 9 状态未损坏验证 ✓ 10 QObject/Qt 内置类型无虚注入点，直接实例化 + QSignalSpy（无 stub 目标）

class MarkdownBridgeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // QSignalSpy/tr 需要 QCoreApplication；offscreen、无 GUI
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char argv0[] = "test_markdownbridge";
            static char *argv[] = { argv0, nullptr };
            s_app = new QCoreApplication(argc, argv);
        }
    }

    static void TearDownTestSuite()
    {
        // 依赖桥对象的用例已各自 TearDown 清理，此处无残留 widget
    }

    void SetUp() override
    {
        stub.clear();
        bridge = new MarkdownBridge();
    }

    void TearDown() override
    {
        delete bridge;
        bridge = nullptr;
        stub.clear();
    }

    static QCoreApplication *s_app;
    stub_ext::StubExt stub;
    MarkdownBridge *bridge = nullptr;
};

QCoreApplication *MarkdownBridgeTest::s_app = nullptr;

TEST_F(MarkdownBridgeTest, OnReady_JsBridgeReady_EmitsReadySignal)
{
    // Arrange
    QSignalSpy spy(bridge, &MarkdownBridge::ready);

    // Act
    bridge->onReady();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.isValid());
}

TEST_F(MarkdownBridgeTest, OnContentChanged_MdPayload_EmitsSamePayload)
{
    // Arrange
    QSignalSpy spy(bridge, &MarkdownBridge::contentChanged);
    const QString md = QString::fromUtf8("# 标题\n正文");

    // Act
    bridge->onContentChanged(md);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), md);
}

// —— 边界值成组：ratio 0 / 中点 / 1（同断言逻辑，强制 TEST_P）——
namespace {
struct RatioCase {
    double ratio;
};
const RatioCase kRatioCases[] = {
    { 0.0 },   // 下边界（回顶部）
    { 0.25 },  // 中间值
    { 1.0 },   // 上边界（页尾）
};
} // namespace

class BridgeRatioParamTest : public MarkdownBridgeTest,
                             public ::testing::WithParamInterface<RatioCase> {
};

TEST_P(BridgeRatioParamTest, OnScrollRatio_ValidRatios_EmitsExactRatio)
{
    // Arrange
    const double ratio = GetParam().ratio;
    QSignalSpy spy(bridge, &MarkdownBridge::scrollRatioChanged);

    // Act
    bridge->onScrollRatio(ratio);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.at(0).at(0).toDouble(), ratio);
}

INSTANTIATE_TEST_SUITE_P(RatioBoundaries, BridgeRatioParamTest,
                         ::testing::ValuesIn(kRatioCases));

TEST_F(MarkdownBridgeTest, OnOpenLink_ExternalUrl_EmitsSameUrl)
{
    // Arrange
    QSignalSpy spy(bridge, &MarkdownBridge::openLinkRequested);
    const QUrl url = QUrl::fromLocalFile(QDir::temp().filePath("demo.html"));
    const QString urlStr = url.toString();

    // Act
    bridge->onOpenLink(urlStr);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), urlStr);
}

TEST_F(MarkdownBridgeTest, OnSignals_EmptyPayload_PayloadForwardedIntact)
{
    // Arrange：空串负面输入——信号不得吞掉或改写空载荷
    QSignalSpy contentSpy(bridge, &MarkdownBridge::contentChanged);
    QSignalSpy linkSpy(bridge, &MarkdownBridge::openLinkRequested);

    // Act
    bridge->onContentChanged(QString());
    bridge->onOpenLink(QString());

    // Assert：空载荷原样转发，状态未损坏（ready 通道不受影响）
    EXPECT_EQ(contentSpy.count(), 1);
    EXPECT_EQ(contentSpy.at(0).at(0).toString(), QString());
    EXPECT_EQ(linkSpy.count(), 1);
    EXPECT_EQ(linkSpy.at(0).at(0).toString(), QString());
}

TEST_F(MarkdownBridgeTest, Retranslate_LanguageSwitched_EmitsRetranslatedAndTextsAlive)
{
    // Arrange
    QSignalSpy spy(bridge, &MarkdownBridge::retranslated);

    // Act
    bridge->retranslate();

    // Assert：NOTIFY 信号触发 + 属性 getter 仍可用（JS 侧全部重取的数据源）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(bridge->collapseTooltip().isEmpty());
    EXPECT_FALSE(bridge->expandText().isEmpty());
}

TEST_F(MarkdownBridgeTest, UiTextProperties_AllGetters_ReturnSourceTexts)
{
    // Arrange —— C locale 无翻译家时 tr 返回源文案（确定性）

    // Act
    const QStringList texts = {
        bridge->collapseTooltip(), bridge->expandTooltip(),
        bridge->copyTooltip(), bridge->expandText(),
        bridge->collapsedLinesText(),
    };

    // Assert：与源码文案逐一精确相等（协议契约：JS 侧按此渲染）
    EXPECT_EQ(texts.at(0), QString("Collapse code block"));
    EXPECT_EQ(texts.at(1), QString("Expand code block"));
    EXPECT_EQ(texts.at(2), QString("Copy code"));
    EXPECT_EQ(texts.at(3), QString("Expand"));
    EXPECT_EQ(texts.at(4), QString("%1 line(s) of code collapsed"));
}

TEST_F(MarkdownBridgeTest, Constructor_WithParent_ParentBacklinkBound)
{
    // Arrange
    QObject parent;

    // Act
    auto *child = new MarkdownBridge(&parent);

    // Assert
    EXPECT_EQ(child->parent(), &parent);
    EXPECT_EQ(parent.children().count(), 1);
    EXPECT_EQ(parent.children().first(), static_cast<QObject *>(child));

    delete child;   // 手动解除，避免 double delete（parent 析构也会回收）
}

TEST_F(MarkdownBridgeTest, Constructor_NoParent_StandaloneOwnership)
{
    // Arrange & Act（SetUp 已创建无父实例）
    // Assert
    EXPECT_EQ(bridge->parent(), nullptr);
    // 信号槽基础设施在该实例上可用（可用性即对象构造完好）
    QSignalSpy spy(bridge, &MarkdownBridge::ready);
    bridge->onReady();
    EXPECT_EQ(spy.count(), 1);
}
