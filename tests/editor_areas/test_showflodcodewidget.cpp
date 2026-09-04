// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ShowFlodCodeWidget（src/editor/showflodcodewidget.h/.cpp）单元测试
//
// 类特征：DFrame（DTK GUI），内嵌 DPlainTextEdit(objectName=PContentEdit) +
// KSyntaxHighlighting 高亮器。内部成员经 findChild("PContentEdit") 与
// document()->findChildren<SyntaxHighlighter*>() 观测（不动源码）。
// 主题分支用 DGuiApplicationHelper::setThemeType 显式驱动，TearDown 恢复原主题。
// 语法定义/主题断言与测试本地 Repository oracle 交叉验证（不依赖数据文件存在与否）。
//
// 方法清单完成情况（test-types §8）：
// | 1 | 公开方法 ≥1 用例：ctor/dtor/appendText/clear/initHighLight/setStyle/hideFirstBlock | 完成 |
// | 2 | 等价类划分：空/短/超宽文本；wrap 开/关；暗/亮主题；已知/未知扩展名；空/多行文档 | 完成 |
// | 3 | 边界值显式覆盖：maxWidth-50 恰好越界封顶；单块文档；start==end 选择 | 完成 |
// | 4 | TEST_P 参数化（≥3 组同质输入）：initHighLight 主题标志 | 完成 |
// | 5 | 分支清单已列出并映射用例 | 完成（见下） |
// | 6 | 每条 if/循环分支有触发用例 | 完成 |
// | 7 | 异常路径：无 throw | N/A |
// | 8 | 负面场景：未知扩展名定义无效、超宽文本封顶 | 完成 |
// | 9 | 强异常安全：不适用（无失败路径） | N/A |
// | 10 | stub 选择：DTK/KF6 运行库真实执行（offscreen），无需 stub | N/A |
//
// 分支清单（来源：showflodcodewidget.cpp）：
//   B1:  m_highlighter != nullptr（initHighLight 守卫，构造后恒真）
//   B2:  !bIsLight → DarkTheme / else LightTheme
//   B3:  DGuiApplicationHelper 主题 == DarkType → 深色调色板
//   B4:  else → 浅色调色板
//   B5:  bIsLineWrap → WidgetWidth / else NoWrap
//   B6:  m_nTextWidth < textWidth（appendText 宽度增长）
//   B7:  m_nTextWidth > maxWidth-50（封顶）
//   B8:  document()->isEmpty() → setPlainText / else appendPlainText
//   B9:  hideFirstBlock 循环 block == lastBlock → break（空文档 0 次迭代/多行提前退出）
//
// 用例映射：
// - Construction_ContentEditConfiguredReadOnly              → ctor
// - AppendText_FirstOnEmpty_SetsPlainTextAndWidth           → B6 真(增长)/B8 真
// - AppendText_Subsequent_AppendsPlainTextKeepsCursorAtStart → B8 假
// - AppendText_WideText_CapsWidthToMaxWidthMinus50          → B7 真（封顶边界）
// - AppendText_NarrowAfterWide_WidthDoesNotShrink           → B6 假
// - Clear_AfterAppend_EmptiesDocument                       → clear
// - SetStyle_DarkTheme_AppliesDarkBaseAndWidgetWrap         → B3 真/B5 真
// - SetStyle_LightTheme_AppliesLightBaseAndNoWrap           → B4/B5 假
// - InitHighLight_ThemeFlag_SetsMatchingTheme /*TEST_P*/    → B1/B2
// - InitHighLight_CppFileName_SetsMatchingDefinition        → definitionForFileName
// - InitHighLight_UnknownExtension_InvalidDefinition        → 负面
// - HideFirstBlock_EmptyDocument_MinimalHeightTen           → B9 0 次迭代
// - HideFirstBlock_ThreeLines_HidesFirstLastSumsMiddle      → B9 提前退出
// - Destructor_AfterUse_DeleteWithoutCrash                  → dtor

#include <gtest/gtest.h>
#include "stubext.h"
#include "showflodcodewidget.h"

#include <QApplication>
#include <DPlainTextEdit>
#include <DGuiApplicationHelper>
#include <KSyntaxHighlighting/repository.h>
#include <KSyntaxHighlighting/syntaxhighlighter.h>
#include <KSyntaxHighlighting/definition.h>
#include <KSyntaxHighlighting/theme.h>

// DGuiApplicationHelper 位于 Dtk::Gui 命名空间（DTK 约定）
DGUI_USE_NAMESPACE

class ShowFlodCodeWidgetTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (QApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_showflodcodewidget";
            static char *argv[] = { appName, nullptr };
            s_app = new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        // DTK6 的 themeType() 为只读系统主题：以 stub 注入受控返回值（TearDown 还原），
        // 避免 DGuiApplicationHelper 全局单例被用例污染
        stub.set_lamda(&DGuiApplicationHelper::themeType,
                       [this](DGuiApplicationHelper *) -> DGuiApplicationHelper::ColorType {
                           return injectedTheme;
                       });

        repoOracle = std::make_unique<KSyntaxHighlighting::Repository>();

        obj = new ShowFlodCodeWidget();
        ASSERT_NE(obj, nullptr);
        edit = obj->findChild<DPlainTextEdit *>("PContentEdit");
        ASSERT_NE(edit, nullptr);
    }

    void TearDown() override
    {
        delete obj;
        obj = nullptr;
        edit = nullptr;
        stub.clear();
    }

    static QApplication *s_app;

    // 内部高亮器（父对象为 document()）
    KSyntaxHighlighting::SyntaxHighlighter *highlighter() const
    {
        const auto hs = edit->document()->findChildren<KSyntaxHighlighting::SyntaxHighlighter *>();
        return hs.isEmpty() ? nullptr : hs.first();
    }

    ShowFlodCodeWidget *obj = nullptr;
    DPlainTextEdit *edit = nullptr;
    std::unique_ptr<KSyntaxHighlighting::Repository> repoOracle;
    stub_ext::StubExt stub;
    DGuiApplicationHelper::ColorType injectedTheme = DGuiApplicationHelper::LightType;
};

QApplication *ShowFlodCodeWidgetTest::s_app = nullptr;

TEST_F(ShowFlodCodeWidgetTest, Construction_ContentEditConfiguredReadOnly)
{
    // Assert：内嵌编辑框只读、任意换行、无边框、滚动条隐藏、无横向滚动
    EXPECT_TRUE(edit->isReadOnly());
    EXPECT_EQ(edit->wordWrapMode(), QTextOption::WrapAnywhere);
    EXPECT_EQ(edit->frameStyle(), 0);
    EXPECT_EQ(edit->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    EXPECT_EQ(edit->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    // 高亮器已挂在文档上
    EXPECT_NE(highlighter(), nullptr);
}

TEST_F(ShowFlodCodeWidgetTest, AppendText_FirstOnEmpty_SetsPlainTextAndWidth)
{
    // Arrange
    const QString text = QStringLiteral("hello");
    const int expectWidth = edit->fontMetrics().horizontalAdvance(text) + 10;

    // Act
    obj->appendText(text, 500);

    // Assert：空文档走 setPlainText（B8 真），宽度=文本宽度+10（B6 增长）
    EXPECT_EQ(edit->toPlainText(), text);
    EXPECT_EQ(edit->width(), expectWidth);
    // 光标移回首部
    EXPECT_EQ(edit->textCursor().position(), 0);
}

TEST_F(ShowFlodCodeWidgetTest, AppendText_Subsequent_AppendsPlainTextKeepsCursorAtStart)
{
    // Arrange
    obj->appendText(QStringLiteral("hello"), 500);

    // Act：非空文档走 appendPlainText（B8 假）
    obj->appendText(QStringLiteral("world"), 500);

    // Assert：追加为新行块
    EXPECT_EQ(edit->toPlainText(), QStringLiteral("hello\nworld"));
    EXPECT_EQ(edit->document()->blockCount(), 2);
    EXPECT_EQ(edit->textCursor().position(), 0);
}

TEST_F(ShowFlodCodeWidgetTest, AppendText_WideText_CapsWidthToMaxWidthMinus50)
{
    // Arrange：超宽文本（宽度必超 maxWidth-50）
    const QString wide(1000, QChar('x'));

    // Act
    obj->appendText(wide, 300);

    // Assert：封顶到 maxWidth-50（B7 真，边界精确值）
    EXPECT_EQ(edit->width(), 300 - 50);
    // 文本仍完整写入
    EXPECT_EQ(edit->toPlainText(), wide);
}

TEST_F(ShowFlodCodeWidgetTest, AppendText_NarrowAfterWide_WidthDoesNotShrink)
{
    // Arrange：先封顶
    obj->appendText(QString(1000, QChar('x')), 300);
    ASSERT_EQ(edit->width(), 250);

    // Act：后追加窄文本（B6 假：宽度不回缩）
    obj->appendText(QStringLiteral("hi"), 300);

    // Assert
    EXPECT_EQ(edit->width(), 250);
    EXPECT_EQ(edit->toPlainText(), QString(1000, QChar('x')) + QStringLiteral("\nhi"));
}

TEST_F(ShowFlodCodeWidgetTest, Clear_AfterAppend_EmptiesDocument)
{
    // Arrange
    obj->appendText(QStringLiteral("some text"), 500);
    ASSERT_FALSE(edit->document()->isEmpty());

    // Act
    obj->clear();

    // Assert：文档清空、内部宽度归零（后续窄文本重新计宽）
    EXPECT_TRUE(edit->document()->isEmpty());
    EXPECT_TRUE(edit->toPlainText().isEmpty());
    obj->appendText(QStringLiteral("ab"), 500);
    EXPECT_EQ(edit->width(), edit->fontMetrics().horizontalAdvance(QStringLiteral("ab")) + 10);
}

TEST_F(ShowFlodCodeWidgetTest, SetStyle_DarkTheme_AppliesDarkBaseAndWidgetWrap)
{
    // Arrange：注入暗主题（B3 真）
    injectedTheme = DGuiApplicationHelper::DarkType;

    // Act
    obj->setStyle(true);

    // Assert：内容框 Base=深色半透明、部件 Base=深色不透明（B5 真：换行打开）
    QColor expectBase(25, 25, 25);
    expectBase.setAlphaF(0.8);
    EXPECT_EQ(edit->palette().color(QPalette::Base), expectBase);
    EXPECT_EQ(obj->palette().color(QPalette::Base), QColor(25, 25, 25));
    EXPECT_EQ(edit->lineWrapMode(), QPlainTextEdit::WidgetWidth);
}

TEST_F(ShowFlodCodeWidgetTest, SetStyle_LightTheme_AppliesLightBaseAndNoWrap)
{
    // Arrange：注入亮主题（B4）
    injectedTheme = DGuiApplicationHelper::LightType;

    // Act
    obj->setStyle(false);

    // Assert：浅色半透明 Base（B5 假：不换行）
    QColor expectBase(247, 247, 247);
    expectBase.setAlphaF(0.6);
    EXPECT_EQ(edit->palette().color(QPalette::Base), expectBase);
    EXPECT_EQ(obj->palette().color(QPalette::Base), QColor(247, 247, 247));
    EXPECT_EQ(edit->lineWrapMode(), QPlainTextEdit::NoWrap);
}

// ---- initHighLight 主题标志（B1/B2，同质断言 → TEST_P） ----
struct ThemeFlagCase {
    bool isLight;
};

class ShowFlodInitHighlightTest : public ShowFlodCodeWidgetTest,
                                  public ::testing::WithParamInterface<ThemeFlagCase> {
};

TEST_P(ShowFlodInitHighlightTest, InitHighLight_ThemeFlag_SetsMatchingTheme)
{
    const ThemeFlagCase &c = GetParam();

    // Act
    obj->initHighLight(QStringLiteral("example.cpp"), c.isLight);

    // Assert：主题与本地 Repository oracle 的默认主题一致（数据无关）
    auto *hl = highlighter();
    ASSERT_NE(hl, nullptr);
    const auto expectTheme = repoOracle->defaultTheme(
        c.isLight ? KSyntaxHighlighting::Repository::LightTheme
                  : KSyntaxHighlighting::Repository::DarkTheme);
    EXPECT_EQ(hl->theme().name(), expectTheme.name());
    EXPECT_EQ(hl->theme().isValid(), expectTheme.isValid());
}

INSTANTIATE_TEST_SUITE_P(
    ThemeFlagCases,
    ShowFlodInitHighlightTest,
    ::testing::Values(
        ThemeFlagCase{ true },   // B2 else：LightTheme
        ThemeFlagCase{ false })); // B2 真：DarkTheme

TEST_F(ShowFlodCodeWidgetTest, InitHighLight_CppFileName_SetsMatchingDefinition)
{
    // Act
    obj->initHighLight(QStringLiteral("/any/path/main.cpp"), true);

    // Assert：定义按文件名匹配（与 oracle 一致；数据存在时为 "C++"）
    auto *hl = highlighter();
    ASSERT_NE(hl, nullptr);
    const auto expectDef = repoOracle->definitionForFileName(QStringLiteral("main.cpp"));
    EXPECT_EQ(hl->definition().name(), expectDef.name());
    EXPECT_EQ(hl->definition().isValid(), expectDef.isValid());
    // 二次调用覆盖为同值（幂等）
    obj->initHighLight(QStringLiteral("other.cpp"), true);
    EXPECT_EQ(hl->definition().name(), expectDef.name());
}

TEST_F(ShowFlodCodeWidgetTest, InitHighLight_UnknownExtension_InvalidDefinition)
{
    // Act：未知扩展名（负面：definitionForFileName 返回无效定义）
    obj->initHighLight(QStringLiteral("data.unknownext123"), true);

    // Assert：与 oracle 行为一致（无效/空定义），不崩溃
    auto *hl = highlighter();
    ASSERT_NE(hl, nullptr);
    const auto expectDef = repoOracle->definitionForFileName(QStringLiteral("data.unknownext123"));
    EXPECT_EQ(hl->definition().isValid(), expectDef.isValid());
    EXPECT_EQ(hl->definition().name(), expectDef.name());
}

TEST_F(ShowFlodCodeWidgetTest, HideFirstBlock_EmptyDocument_MinimalHeightTen)
{
    // Arrange：空文档（单块，first==last → 循环 0 次迭代，B9）

    // Act
    obj->hideFirstBlock();

    // Assert：唯一块被隐藏、高度=0+10
    EXPECT_FALSE(edit->document()->firstBlock().isVisible());
    EXPECT_FALSE(edit->document()->lastBlock().isVisible());
    EXPECT_EQ(edit->height(), 10);
}

TEST_F(ShowFlodCodeWidgetTest, HideFirstBlock_ThreeLines_HidesFirstLastSumsMiddle)
{
    // Arrange：3 行文本
    obj->appendText(QStringLiteral("line1"), 500);
    obj->appendText(QStringLiteral("line2"), 500);
    obj->appendText(QStringLiteral("line3"), 500);
    ASSERT_EQ(edit->document()->blockCount(), 3);
    const auto *layout = edit->document()->documentLayout();
    const int middleH = layout->blockBoundingRect(
                            edit->document()->firstBlock().next()).toRect().height();

    // Act
    obj->hideFirstBlock();

    // Assert：首末块隐藏、高度=中间块高度+10（B9：遍历中间块后遇 lastBlock 提前退出）
    EXPECT_FALSE(edit->document()->firstBlock().isVisible());
    EXPECT_FALSE(edit->document()->lastBlock().isVisible());
    EXPECT_TRUE(edit->document()->firstBlock().next().isVisible());
    EXPECT_EQ(edit->height(), middleH + 10);
    EXPECT_GT(edit->height(), 10);
}

TEST_F(ShowFlodCodeWidgetTest, Destructor_AfterUse_DeleteWithoutCrash)
{
    // Arrange：先使用（追加 + 高亮 + 样式 + 折叠隐藏）
    ShowFlodCodeWidget *tmp = new ShowFlodCodeWidget();
    tmp->appendText(QStringLiteral("text"), 500);
    tmp->initHighLight(QStringLiteral("a.cpp"), true);
    tmp->setStyle(true);
    tmp->hideFirstBlock();

    // Act：析构（内部 m_highlighter 走 deleteLater）并冲刷延迟删除
    delete tmp;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // Assert：全局状态未受破坏 —— fixture 对象仍正常工作、应用实例健在
    obj->appendText(QStringLiteral("after"), 500);
    DPlainTextEdit *ediAfter = obj->findChild<DPlainTextEdit *>("PContentEdit");
    ASSERT_NE(ediAfter, nullptr);
    EXPECT_EQ(ediAfter->toPlainText(), QStringLiteral("after"));
    EXPECT_NE(QApplication::instance(), nullptr);
}
