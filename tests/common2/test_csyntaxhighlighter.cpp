// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * CSyntaxHighlighter 单元测试
 *
 * 分支清单（来源：CSyntaxHighlighter::highlightBlock / setInvalidCharHighlight）
 * B1 : !m_bHighlight                     → 直接 return（不高亮，无格式区间）
 * B2 : m_bHighlight && !invalid          → 仅执行 KSyntaxHighlighting 基类高亮
 * B3 : m_bHighlight && invalid           → 基类高亮 + "\\00" 序列红底白字格式
 * B4 : setInvalidCharHighlight(true)     → 联动开启 m_bHighlight 并 rehighlight
 * B5 : setInvalidCharHighlight(false)    → 仅关闭 invalid，联动 rehighlight
 * B6 : 多个 "\\00" 匹配                  → while 迭代为每处设置格式
 *
 * 用例映射：
 * - Ctor_QObjectParent_DefaultDisabled_NoFormatRange   → B1（QObject* 构造）
 * - Ctor_DocumentParent_DefaultDisabled_NoFormatRange  → B1（QTextDocument* 构造）
 * - SetEnableHighlight_TrueWithoutInvalidFlag_NoRedMarks → B2
 * - SetInvalidCharHighlight_True_MarksEachMatchRed     → B3+B4+B6
 * - SetInvalidCharHighlight_False_RemovesRedMarks      → B5
 * - SetEnableHighlight_Toggle_MarksFollowLatestState   → B1/B3 组合
 *
 * 隔离：QTextDocument + offscreen QGuiApplication，不涉及窗口系统；
 * 断言经由块布局 QTextLayout::formats()（高亮器最终把格式区间应用到块布局，
 * 为最稳定的可观察出口）；高亮驱动统一使用公开接口 rehighlight()。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include "CSyntaxHighlighter.h"

#include <QGuiApplication>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextLayout>
#include <QColor>

namespace {

QCoreApplication *ensureApp()
{
    if (!QCoreApplication::instance()) {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
            qputenv("QT_QPA_PLATFORM", "offscreen");
        static int argc = 1;
        static char argv0[] = "test_common2";
        static char *argv[2] = { argv0, nullptr };
        static QGuiApplication app(argc, argv);
        return &app;
    }
    return QCoreApplication::instance();
}

const QColor kInvalidBg(QStringLiteral("#FF0000"));
const QColor kInvalidFg(QStringLiteral("#FFFFFF"));

// 统计块布局中红底格式区间数量（无效字符标记的可观察出口）
int redMarkCount(const QTextDocument *doc)
{
    int count = 0;
    const QList<QTextLayout::FormatRange> ranges = doc->firstBlock().layout()->formats();
    for (const QTextLayout::FormatRange &r : ranges) {
        if (r.format.background().color() == kInvalidBg)
            ++count;
    }
    return count;
}

// 统计白字前景区间数量（仅无效字符标记会设置白前景）
int whiteForegroundCount(const QTextDocument *doc)
{
    int count = 0;
    const QList<QTextLayout::FormatRange> ranges = doc->firstBlock().layout()->formats();
    for (const QTextLayout::FormatRange &r : ranges) {
        if (r.format.foreground().color() == kInvalidFg)
            ++count;
    }
    return count;
}

QList<QTextLayout::FormatRange> redMarks(const QTextDocument *doc)
{
    QList<QTextLayout::FormatRange> out;
    const QList<QTextLayout::FormatRange> ranges = doc->firstBlock().layout()->formats();
    for (const QTextLayout::FormatRange &r : ranges) {
        if (r.format.background().color() == kInvalidBg)
            out.append(r);
    }
    return out;
}

} // namespace

class CSyntaxHighlighterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensureApp();
        stub.clear();
        doc = new QTextDocument;
    }

    void TearDown() override
    {
        stub.clear();
        delete doc;
        doc = nullptr;
    }

    QTextDocument *doc = nullptr;
    stub_ext::StubExt stub;
};

// B1（QObject* 父对象构造：默认不高亮）
TEST_F(CSyntaxHighlighterTest, Ctor_QObjectParent_DefaultDisabled_NoFormatRange)
{
    // Arrange
    CSyntaxHighlighter highlighter;
    highlighter.setDocument(doc);
    doc->setPlainText(QStringLiteral("a\\00b"));
    // Act
    highlighter.rehighlight();
    // Assert: 未开启高亮 → 不产生任何格式区间（含红底标记）
    EXPECT_EQ(redMarkCount(doc), 0);
    EXPECT_TRUE(doc->firstBlock().layout()->formats().isEmpty());
}

// B1（QTextDocument* 构造）
TEST_F(CSyntaxHighlighterTest, Ctor_DocumentParent_DefaultDisabled_NoFormatRange)
{
    // Arrange
    CSyntaxHighlighter highlighter(doc);
    doc->setPlainText(QStringLiteral("x\\00y"));
    // Act
    highlighter.rehighlight();
    // Assert
    EXPECT_EQ(redMarkCount(doc), 0);
    EXPECT_TRUE(doc->firstBlock().layout()->formats().isEmpty());
}

// B2
TEST_F(CSyntaxHighlighterTest, SetEnableHighlight_TrueWithoutInvalidFlag_NoRedMarks)
{
    // Arrange
    CSyntaxHighlighter highlighter(doc);
    doc->setPlainText(QStringLiteral("a\\00b"));
    highlighter.setEnableHighlight(true);
    // Act
    highlighter.rehighlight();
    // Assert: 已开启高亮但未开启无效字符标记 → 无红底/白字区间
    //（K 基类无 Definition 时可能产生整块默认格式区间，属正常，不算无效字符标记）
    EXPECT_EQ(redMarkCount(doc), 0);
    EXPECT_EQ(whiteForegroundCount(doc), 0);
}

// B3+B4+B6
TEST_F(CSyntaxHighlighterTest, SetInvalidCharHighlight_True_MarksEachMatchRed)
{
    // Arrange: 两个 "\\00" 匹配（覆盖 while 迭代为每处设置格式）
    CSyntaxHighlighter highlighter(doc);
    doc->setPlainText(QStringLiteral("a\\00b\\00c"));
    // Act: setInvalidCharHighlight(true) 联动开启高亮并自动 rehighlight
    highlighter.setInvalidCharHighlight(true);
    // Assert: 两处 "\\00"（位置 1、5）各产生一个红底区间
    ASSERT_EQ(redMarkCount(doc), 2);
    const QList<QTextLayout::FormatRange> marks = redMarks(doc);
    EXPECT_EQ(marks.first().start, 1);
    EXPECT_EQ(marks.first().length, 3); // '\\' '0' '0'
    EXPECT_EQ(marks.last().start, 5);
    EXPECT_EQ(marks.last().length, 3);
    EXPECT_EQ(marks.first().format.foreground().color(), kInvalidFg);
}

// B5
TEST_F(CSyntaxHighlighterTest, SetInvalidCharHighlight_False_RemovesRedMarks)
{
    // Arrange: 先开启标记产生格式
    CSyntaxHighlighter highlighter(doc);
    doc->setPlainText(QStringLiteral("a\\00b"));
    highlighter.setInvalidCharHighlight(true);
    ASSERT_EQ(redMarkCount(doc), 1);
    // Act
    highlighter.setInvalidCharHighlight(false);
    // Assert: 关闭后 rehighlight 清除标记；高亮总开关仍为开（基类高亮继续不崩溃）
    EXPECT_EQ(redMarkCount(doc), 0);
    EXPECT_EQ(whiteForegroundCount(doc), 0);
}

// B1/B3 组合：显式 enable 后切换，格式跟随最新状态
TEST_F(CSyntaxHighlighterTest, SetEnableHighlight_Toggle_MarksFollowLatestState)
{
    // Arrange
    CSyntaxHighlighter highlighter(doc);
    doc->setPlainText(QStringLiteral("\\00"));
    highlighter.setEnableHighlight(true);
    highlighter.setInvalidCharHighlight(true);
    ASSERT_EQ(redMarkCount(doc), 1);
    // Act: 关闭总开关后重新 rehighlight
    highlighter.setEnableHighlight(false);
    highlighter.rehighlight();
    // Assert: 总开关关闭 → 任何格式都不再设置（B1 分支）
    EXPECT_EQ(redMarkCount(doc), 0);
    EXPECT_TRUE(doc->firstBlock().layout()->formats().isEmpty());
}
