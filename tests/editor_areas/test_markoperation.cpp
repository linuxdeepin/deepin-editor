// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// TextEdit::MarkOperation / TextEdit::MarkReplaceInfo（src/editor/dtextedit.h 标记
// 操作数据结构）单元测试，附带覆盖两个纯逻辑静态转换函数
// TextEdit::convertReplaceToMark / TextEdit::convertMarkToReplace。
//
// 类特征：POD 型结构体（inline 默认构造），无信号无分支；
// 转换函数为 static 纯数据变换（cursor 绝对位置 ↔ 选中区间），无实例依赖。
//
// 方法清单完成情况（test-types §8）：
// | 1 | 公开方法 ≥1 用例：两结构体默认构造/字段语义 + convertReplaceToMark/
//     convertMarkToReplace | 完成 |
// | 2 | 等价类划分：空列表/单元素/多元素；null 光标/文档光标；start==end/跨行区间 | 完成 |
// | 3 | 边界值显式覆盖：(0,0) 零长选择、(0,len) 全选、中间区间、极值时间戳 | 完成 |
// | 4 | TEST_P 参数化（≥3 组同质输入）：区间边界往返 | 完成 |
// | 5 | 分支清单已列出并映射用例 | N/A（结构体无分支；转换函数仅 for 循环） |
// | 6 | 每条 if 分支：无 if；for 0/1/N 次迭代均覆盖 | 完成 |
// | 7 | 异常路径：无 throw | N/A |
// | 8 | 负面场景：空列表、null 光标（setPosition 无副作用） | 完成 |
// | 9 | 强异常安全：null 光标转换后不产生虚假选中 | 完成 |
// | 10 | stub 选择：纯数据结构 + Qt 容器，无需 stub | N/A |
//
// 分支清单：
//   B1: convertReplaceToMark for 循环（空列表 0 次 / 单元素 1 次 / 三元素 3 次）
//   B2: convertMarkToReplace for 循环（同上）
//
// 用例映射：
// - DefaultConstruction_MarkOperation_InitialValues            → MarkOperation()
// - DefaultConstruction_MarkReplaceInfo_ZeroPositions          → MarkReplaceInfo()
// - FieldAssignment_AllEnumTypes_RoundTrip /*TEST_P*/          → 字段语义（4 枚举值）
// - ConvertReplaceToMark_AbsoluteRange_SetsCursorSelection     → B1 + 光标绝对化
// - ConvertReplaceToMark_EmptyList_ReturnsEmpty                → B1 0 次
// - ConvertReplaceToMark_NullCursor_KeepsCursorNull            → 负面/强异常安全
// - ConvertReplaceToMark_MultiElements_PreservesOrderAndTime   → B1 3 次
// - ConvertMarkToReplace_RecordsSelectionBounds                → B2
// - ConvertMarkToReplace_EmptyList_ReturnsEmpty                → B2 0 次
// - RoundTrip_ReplaceToMarkToReplace_PreservesRange /*TEST_P*/ → 往返不变量（边界组）

#include <gtest/gtest.h>
#include "dtextedit.h"

#include <QApplication>
#include <QTextCursor>
#include <QTextDocument>
#include <climits>

class MarkOperationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // QTextCursor/QTextDocument 属 Gui 模块，统一 offscreen QApplication
        if (QApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_markoperation";
            static char *argv[] = { appName, nullptr };
            s_app = new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        doc = std::make_unique<QTextDocument>();
        doc->setPlainText(QStringLiteral("abcdef"));
        textLength = doc->characterCount() - 1;   // 6
    }

    static QApplication *s_app;

    std::unique_ptr<QTextDocument> doc;
    int textLength = 0;
};

QApplication *MarkOperationTest::s_app = nullptr;

TEST_F(MarkOperationTest, DefaultConstruction_MarkOperation_InitialValues)
{
    // Arrange/Act
    TextEdit::MarkOperation opt;

    // Assert：默认标记单次操作、空颜色/匹配文本、空光标
    EXPECT_EQ(opt.type, TextEdit::MarkOnce);
    EXPECT_TRUE(opt.cursor.isNull());
    EXPECT_TRUE(opt.color.isEmpty());
    EXPECT_TRUE(opt.matchText.isEmpty());
}

TEST_F(MarkOperationTest, DefaultConstruction_MarkReplaceInfo_ZeroPositions)
{
    // Arrange/Act
    TextEdit::MarkReplaceInfo info;

    // Assert：零区间、零时间戳、内嵌操作默认
    EXPECT_EQ(info.start, 0);
    EXPECT_EQ(info.end, 0);
    EXPECT_EQ(info.time, 0);
    EXPECT_EQ(info.opt.type, TextEdit::MarkOnce);
}

// ---- 字段语义（4 个枚举值同质往返 → TEST_P） ----
struct MarkTypeCase {
    TextEdit::MarkOperationType type;
};

class MarkTypeRoundTripTest : public MarkOperationTest,
                              public ::testing::WithParamInterface<MarkTypeCase> {
};

TEST_P(MarkTypeRoundTripTest, FieldAssignment_AllEnumTypes_RoundTrip)
{
    const MarkTypeCase &c = GetParam();

    // Arrange
    TextEdit::MarkOperation opt;

    // Act：全字段赋值
    opt.type = c.type;
    opt.color = QStringLiteral("#ff0000");
    opt.matchText = QStringLiteral("keyword");
    opt.cursor = QTextCursor(doc.get());

    // Assert：字段独立往返保值
    EXPECT_EQ(opt.type, c.type);
    EXPECT_EQ(opt.color, QStringLiteral("#ff0000"));
    EXPECT_EQ(opt.matchText, QStringLiteral("keyword"));
    EXPECT_FALSE(opt.cursor.isNull());
}

INSTANTIATE_TEST_SUITE_P(
    MarkTypeCases,
    MarkTypeRoundTripTest,
    ::testing::Values(
        MarkTypeCase{ TextEdit::MarkOnce },
        MarkTypeCase{ TextEdit::MarkAllMatch },
        MarkTypeCase{ TextEdit::MarkLine },
        MarkTypeCase{ TextEdit::MarkAll }));

TEST_F(MarkOperationTest, ConvertReplaceToMark_AbsoluteRange_SetsCursorSelection)
{
    // Arrange：绝对区间 [1,4) → 选中 "bcd"
    TextEdit::MarkReplaceInfo info;
    info.opt.type = TextEdit::MarkAllMatch;
    info.opt.color = QStringLiteral("#00ff00");
    info.opt.matchText = QStringLiteral("bcd");
    info.opt.cursor = QTextCursor(doc.get());
    info.start = 1;
    info.end = 4;
    info.time = 1700000000123LL;

    // Act
    const QList<QPair<TextEdit::MarkOperation, qint64>> marks =
        TextEdit::convertReplaceToMark({ info });

    // Assert：光标按绝对位置重选、其余字段与时间戳原样保留
    ASSERT_EQ(marks.size(), 1);
    EXPECT_EQ(marks[0].first.cursor.selectedText(), QStringLiteral("bcd"));
    EXPECT_EQ(marks[0].first.cursor.selectionStart(), 1);
    EXPECT_EQ(marks[0].first.cursor.selectionEnd(), 4);
    EXPECT_EQ(marks[0].first.type, TextEdit::MarkAllMatch);
    EXPECT_EQ(marks[0].first.color, QStringLiteral("#00ff00"));
    EXPECT_EQ(marks[0].first.matchText, QStringLiteral("bcd"));
    EXPECT_EQ(marks[0].second, 1700000000123LL);
}

TEST_F(MarkOperationTest, ConvertReplaceToMark_EmptyList_ReturnsEmpty)
{
    // Act
    const auto marks = TextEdit::convertReplaceToMark({});

    // Assert：空入空出（循环 0 次迭代）
    EXPECT_TRUE(marks.isEmpty());
    EXPECT_EQ(marks.size(), 0);
}

TEST_F(MarkOperationTest, ConvertReplaceToMark_NullCursor_KeepsCursorNull)
{
    // Arrange：默认构造的 null 光标（负面：setPosition 对 null 光标无副作用）
    TextEdit::MarkReplaceInfo info;
    ASSERT_TRUE(info.opt.cursor.isNull());
    info.start = 2;
    info.end = 3;
    info.time = 42;

    // Act
    const auto marks = TextEdit::convertReplaceToMark({ info });

    // Assert：强异常安全 —— 光标保持 null、时间戳保留，无虚假选中产生
    ASSERT_EQ(marks.size(), 1);
    EXPECT_TRUE(marks[0].first.cursor.isNull());
    EXPECT_EQ(marks[0].first.cursor.selectedText(), QString());
    EXPECT_EQ(marks[0].second, 42);
}

TEST_F(MarkOperationTest, ConvertReplaceToMark_MultiElements_PreservesOrderAndTime)
{
    // Arrange：三个不同区间的元素
    QList<TextEdit::MarkReplaceInfo> infos;
    for (int i = 0; i < 3; ++i) {
        TextEdit::MarkReplaceInfo info;
        info.opt.cursor = QTextCursor(doc.get());
        info.opt.type = TextEdit::MarkLine;
        info.start = i;
        info.end = i + 1;
        info.time = qint64(i + 1) * 1000;
        infos.append(info);
    }

    // Act
    const auto marks = TextEdit::convertReplaceToMark(infos);

    // Assert：顺序与时间戳逐项对应（循环 3 次迭代）
    ASSERT_EQ(marks.size(), 3);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(marks[i].second, qint64(i + 1) * 1000);
        EXPECT_EQ(marks[i].first.cursor.selectionStart(), i);
        EXPECT_EQ(marks[i].first.cursor.selectionEnd(), i + 1);
        EXPECT_EQ(marks[i].first.type, TextEdit::MarkLine);
    }
}

TEST_F(MarkOperationTest, ConvertMarkToReplace_RecordsSelectionBounds)
{
    // Arrange：光标选中 [2,5) → "cde"
    TextEdit::MarkOperation opt;
    opt.type = TextEdit::MarkOnce;
    opt.color = QStringLiteral("#0000ff");
    opt.cursor = QTextCursor(doc.get());
    opt.cursor.setPosition(2);
    opt.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 3);

    // Act
    const auto replaces = TextEdit::convertMarkToReplace({ qMakePair(opt, qint64(99)) });

    // Assert：区间与时间戳被绝对化记录
    ASSERT_EQ(replaces.size(), 1);
    EXPECT_EQ(replaces[0].start, 2);
    EXPECT_EQ(replaces[0].end, 5);
    EXPECT_EQ(replaces[0].time, 99);
    EXPECT_EQ(replaces[0].opt.color, QStringLiteral("#0000ff"));
    EXPECT_EQ(replaces[0].opt.type, TextEdit::MarkOnce);
}

TEST_F(MarkOperationTest, ConvertMarkToReplace_EmptyList_ReturnsEmpty)
{
    // Act
    const auto replaces = TextEdit::convertMarkToReplace({});

    // Assert：空入空出（循环 0 次迭代），无虚假区间产生
    EXPECT_TRUE(replaces.isEmpty());
    EXPECT_EQ(replaces.size(), 0);
}

// ---- 往返不变量（边界区间组 → TEST_P） ----
struct RangeCase {
    int start;
    int end;
};

class MarkRoundTripTest : public MarkOperationTest,
                          public ::testing::WithParamInterface<RangeCase> {
};

TEST_P(MarkRoundTripTest, RoundTrip_ReplaceToMarkToReplace_PreservesRange)
{
    const RangeCase &c = GetParam();

    // Arrange
    TextEdit::MarkReplaceInfo info;
    info.opt.cursor = QTextCursor(doc.get());
    info.start = c.start;
    info.end = c.end;
    info.time = qint64(INT_MAX) + 1;   // 极值时间戳边界

    // Act：replace → mark → replace
    const auto marks = TextEdit::convertReplaceToMark({ info });
    const auto back = TextEdit::convertMarkToReplace(marks);

    // Assert：区间与时间戳往返不变
    ASSERT_EQ(back.size(), 1);
    EXPECT_EQ(back[0].start, c.start);
    EXPECT_EQ(back[0].end, c.end);
    EXPECT_EQ(back[0].time, qint64(INT_MAX) + 1);
}

INSTANTIATE_TEST_SUITE_P(
    RangeCases,
    MarkRoundTripTest,
    ::testing::Values(
        RangeCase{ 0, 0 },                  // 零长选择（边界）
        RangeCase{ 0, 6 },                  // 全选（上边界）
        RangeCase{ 2, 5 },                  // 中间区间（典型）
        RangeCase{ 5, 6 }));                // 末字符（末边界）

TEST_F(MarkOperationTest, Structs_IndependentCopies_DoNotAlias)
{
    // Arrange
    TextEdit::MarkReplaceInfo a;
    a.start = 3;
    a.end = 4;
    a.time = 777;
    a.opt.color = QStringLiteral("#123456");

    // Act：值拷贝后修改副本
    TextEdit::MarkReplaceInfo b = a;
    b.start = 0;
    b.opt.color = QStringLiteral("#ffffff");

    // Assert：拷贝独立（QTextCursor 隐式共享按值语义安全）
    EXPECT_EQ(a.start, 3);
    EXPECT_EQ(a.opt.color, QStringLiteral("#123456"));
    EXPECT_EQ(b.end, 4);
    EXPECT_EQ(b.time, 777);
}
