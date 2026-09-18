// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "imarkdownrenderer.h"

#include <QVariantMap>

// 分支清单（来源：imarkdownrenderer.h —— 纯虚接口，无分支；可测点为多态分发与虚析构）
// B1: 通过 IMarkdownRenderer* 调用 6 个纯虚方法 → 派生类实现被精确调用
// B2: delete 基类指针 → 虚析构执行派生类析构（~IMarkdownRenderer 覆盖）
// B3: isReady() 返回派生类配置值（true/false 两侧）
//
// 用例映射：
// - PolymorphicDispatch_AllInterfaceMethods_ReachImpl        → B1
// - VirtualDestructor_DeleteViaBasePointer_RunsImplDtor      → B2
// - IsReady_ConfiguredTrue_ReturnsTrue                       → B3(true)
// - IsReady_ConfiguredFalse_ReturnsFalse                      → B3(false)
// - SetMarkdownAndTheme_PayloadForwarded_ExactValuesCaptured  → B1（参数精确转发）
//
// 最小清单自检：1 每公开方法≥1用例 ✔（接口全部 6 方法经 Dispatch 用例覆盖）
// 2 输入维度等价类 ✔ 3 边界值 ✔（true/false 两边）4 无≥3组同质输入（不适用 TEST_P）
// 5 分支清单已映射 ✔ 6 无 if/switch 分支（纯虚）7 无异常路径 8 无负面输入维度
// 9 不适用 10 接口本身即被测目标，实现用记录型子类（非 gMock/stub 混用目标）

class IMarkdownRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        stub.clear();
        impl = new RecordingRenderer();
        base = impl;   // 仅通过接口指针访问
    }

    void TearDown() override {
        delete base;   // 经虚析构释放
        base = nullptr;
        impl = nullptr;
        stub.clear();
    }

    stub_ext::StubExt stub;

    // 记录型最小实现（不与 stub_ext/gMock 混用同一目标）
    class RecordingRenderer : public IMarkdownRenderer {
    public:
        bool ready = false;
        int isReadyCalls = 0;
        int setMarkdownCalls = 0;
        int setModeCalls = 0;
        int applyThemeCalls = 0;
        int setLayoutCalls = 0;
        int scrollToRatioCalls = 0;
        QString lastMd;
        int lastMode = -1;
        QVariantMap lastTheme;
        int lastMaxW = -1;
        bool lastCenter = false;
        double lastRatio = -1.0;
        bool *dtorFlag = nullptr;

        ~RecordingRenderer() override {
            if (dtorFlag)
                *dtorFlag = true;
        }

        bool isReady() const override { return const_cast<int &>(isReadyCalls) += 1, ready; }
        void setMarkdown(const QString &md) override { ++setMarkdownCalls; lastMd = md; }
        void setMode(int mode) override { ++setModeCalls; lastMode = mode; }
        void applyTheme(const QVariantMap &themeMap) override { ++applyThemeCalls; lastTheme = themeMap; }
        void setLayout(int maxContentWidth, bool center) override { ++setLayoutCalls; lastMaxW = maxContentWidth; lastCenter = center; }
        void scrollToRatio(double ratio) override { ++scrollToRatioCalls; lastRatio = ratio; }
    };

    RecordingRenderer *impl = nullptr;
    IMarkdownRenderer *base = nullptr;
};

TEST_F(IMarkdownRendererTest, PolymorphicDispatch_AllInterfaceMethods_ReachImpl)
{
    // Arrange
    QVariantMap theme;
    theme["k"] = "v";

    // Act —— 仅经接口指针驱动全部 6 个方法
    base->setMarkdown("# title");
    base->setMode(1);
    base->applyTheme(theme);
    base->setLayout(800, true);
    base->scrollToRatio(0.5);
    (void)base->isReady();

    // Assert：实现侧逐项收到精确调用与参数
    EXPECT_EQ(impl->setMarkdownCalls, 1);
    EXPECT_EQ(impl->lastMd, QString("# title"));
    EXPECT_EQ(impl->setModeCalls, 1);
    EXPECT_EQ(impl->lastMode, 1);
    EXPECT_EQ(impl->applyThemeCalls, 1);
    EXPECT_EQ(impl->lastTheme.value("k").toString(), QString("v"));
    EXPECT_EQ(impl->setLayoutCalls, 1);
    EXPECT_EQ(impl->lastMaxW, 800);
    EXPECT_TRUE(impl->lastCenter);
    EXPECT_EQ(impl->scrollToRatioCalls, 1);
    EXPECT_DOUBLE_EQ(impl->lastRatio, 0.5);
    EXPECT_EQ(impl->isReadyCalls, 1);
}

TEST_F(IMarkdownRendererTest, VirtualDestructor_DeleteViaBasePointer_RunsImplDtor)
{
    // Arrange：两个实例分别挂析构标记（独立 new 的 + 夹具持有的）
    bool ownedDestroyed = false;
    bool fixtureDestroyed = false;
    impl->dtorFlag = &fixtureDestroyed;
    auto *owned = new RecordingRenderer();
    owned->dtorFlag = &ownedDestroyed;
    IMarkdownRenderer *ownedBase = owned;
    IMarkdownRenderer *fixtureBase = base;
    base = nullptr;          // 本用例内自行释放，TearDown 不再重复 delete
    impl = nullptr;

    // Act
    delete ownedBase;
    delete fixtureBase;

    // Assert：两实例虚析构均执行到派生类析构函数
    EXPECT_TRUE(ownedDestroyed);
    EXPECT_TRUE(fixtureDestroyed);
}

TEST_F(IMarkdownRendererTest, IsReady_ConfiguredTrue_ReturnsTrue)
{
    // Arrange
    impl->ready = true;

    // Act
    const bool ret = base->isReady();

    // Assert
    EXPECT_TRUE(ret);            // 期望 true 边（实现配置 ready=true）
    EXPECT_EQ(impl->isReadyCalls, 1);
}

TEST_F(IMarkdownRendererTest, IsReady_ConfiguredFalse_ReturnsFalse)
{
    // Arrange
    impl->ready = false;

    // Act
    const bool ret = base->isReady();

    // Assert
    EXPECT_FALSE(ret);           // 期望 false 边（实现配置 ready=false）
    EXPECT_EQ(impl->isReadyCalls, 1);
}

TEST_F(IMarkdownRendererTest, SetMarkdownAndTheme_PayloadForwarded_ExactValuesCaptured)
{
    // Arrange
    const QString md = QString::fromUtf8("![img](a.png)\n中文正文");
    QVariantMap theme;
    QVariantMap ec;
    ec["background-color"] = "#1e1e1e";
    theme["editor-colors"] = ec;

    // Act
    base->setMarkdown(md);
    base->applyTheme(theme);

    // Assert
    EXPECT_EQ(impl->lastMd, md);
    EXPECT_EQ(impl->lastTheme.value("editor-colors").toMap()
                  .value("background-color").toString(), QString("#1e1e1e"));
    EXPECT_EQ(impl->setMarkdownCalls, 1);
    EXPECT_EQ(impl->applyThemeCalls, 1);
}
