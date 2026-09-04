// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// LineNumberArea（src/editor/linenumberarea.h/.cpp）单元测试
//
// 类特征：QWidget（GUI），持 LeftAreaTextEdit 指针委托绘制/点击。
// 鼠标事件用 QMouseEvent 合成 + QApplication::sendEvent 真实派发；
// paintEvent/sizeHint（protected）经 -fno-access-control 直驱并计数委托。
//
// 方法清单完成情况（test-types §8）：
// | 1 | 公开方法 ≥1 用例：ctor/dtor/getPressPoint/paintEvent/sizeHint/mousePressEvent | 完成 |
// | 2 | 等价类划分：行号宽度（行数位数）；鼠标左键/右键；坐标原点/典型值 | 完成 |
// | 3 | 边界值显式覆盖：点击 (0,0) 原点与 (17,42) 典型点；行数进位边界 | 完成 |
// | 4 | TEST_P 参数化（≥3 组同质输入）：鼠标按键类型 | 完成 |
// | 5 | 分支清单已列出并映射用例 | 完成（见下） |
// | 6 | 每条 if 分支有触发用例：无 if（委托链） | N/A |
// | 7 | 异常路径：无 throw | N/A |
// | 8 | 负面场景：未初始化 pressPoint、左/右键均转发 | 完成 |
// | 9 | 强异常安全：不适用（无状态突变） | N/A |
// | 10 | stub 选择：项目内具体类 LeftAreaTextEdit/TextEdit → stub_ext | 完成 |
//
// 分支清单（来源：linenumberarea.cpp）：无控制流分支（纯委托）。
//
// 用例映射：
// - Construction_StoresLeftAreaWidget_ZeroMargins      → ctor
// - GetPressPoint_InitialDefault_ReturnsOrigin        → getPressPoint（m_pressPoint 赋值
//   在源码中已注释，恒为默认构造值 —— 记录该行为约定）
// - SizeHint_WidthFromLineNumberArea_HeightZero       → sizeHint
// - MousePressEvent_AnyButton_ForwardsPositionToEdit /*TEST_P*/ → mousePressEvent
// - MousePressEvent_OriginPoint_ForwardsZero          → 边界 (0,0)
// - PaintEvent_DelegatesToLeftArea_PaintEvent         → paintEvent

#include <gtest/gtest.h>
#include "stubext.h"
#include "linenumberarea.h"
#include "leftareaoftextedit.h"
#include "dtextedit.h"

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPlainTextEdit>

namespace {

TextEdit *const kFakeEdit = reinterpret_cast<TextEdit *>(quintptr(0x1000));

} // namespace

class LineNumberAreaTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (QApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_linenumberarea";
            static char *argv[] = { appName, nullptr };
            s_app = new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        onPressedCalls = 0;
        capturedPoint = QPoint(-1, -1);
        areaPaintCalls = 0;

        doc = std::make_unique<QTextDocument>();

        // LeftAreaTextEdit 构造 qDebug 流式输出 fake TextEdit* → 拦截 QWidget 流式运算符
        stub.set_lamda(static_cast<QDebug (*)(QDebug, const QWidget *)>(&operator<<),
                       [](QDebug d, const QWidget *) -> QDebug { return d; });

        // LeftAreaTextEdit::lineNumberAreaWidth 经 m_pTextEdit->document() 取行数 → 喂受控文档
        stub.set_lamda(&QPlainTextEdit::document,
                       [this](QPlainTextEdit *) -> QTextDocument * { return doc.get(); });
        // TextEdit::onPressedLineNumber 捕获点击位置（委托终点）
        stub.set_lamda(&TextEdit::onPressedLineNumber,
                       [this](TextEdit *, const QPoint &p) {
                           ++onPressedCalls;
                           capturedPoint = p;
                       });
        // LeftAreaTextEdit::lineNumberAreaPaintEvent 委托计数（paintEvent 终点）
        stub.set_lamda(&LeftAreaTextEdit::lineNumberAreaPaintEvent,
                       [this](LeftAreaTextEdit *, QPaintEvent *) { ++areaPaintCalls; });

        leftArea = new LeftAreaTextEdit(kFakeEdit);
        obj = new LineNumberArea(leftArea);
        ASSERT_NE(obj, nullptr);
    }

    void TearDown() override
    {
        delete obj;
        obj = nullptr;
        delete leftArea;
        leftArea = nullptr;
        stub.clear();
    }

    static QApplication *s_app;

    stub_ext::StubExt stub;
    LeftAreaTextEdit *leftArea = nullptr;
    LineNumberArea *obj = nullptr;
    std::unique_ptr<QTextDocument> doc;

    int onPressedCalls = 0;
    QPoint capturedPoint { -1, -1 };
    int areaPaintCalls = 0;
};

QApplication *LineNumberAreaTest::s_app = nullptr;

TEST_F(LineNumberAreaTest, Construction_StoresLeftAreaWidget_ZeroMargins)
{
    // Assert：持有委托目标、内容边距清零（行号紧贴排布约定）
    EXPECT_EQ(obj->m_leftAreaWidget, leftArea);
    EXPECT_EQ(obj->contentsMargins(), QMargins(0, 0, 0, 0));
}

TEST_F(LineNumberAreaTest, GetPressPoint_InitialDefault_ReturnsOrigin)
{
    // Act（无任何交互）

    // Assert：m_pressPoint 赋值语句在源码中处于注释状态 → 恒为默认值 (0,0)
    EXPECT_EQ(obj->getPressPoint(), QPoint(0, 0));
    EXPECT_EQ(obj->m_pressPoint.x(), 0);
    EXPECT_EQ(obj->m_pressPoint.y(), 0);
}

TEST_F(LineNumberAreaTest, SizeHint_WidthFromLineNumberArea_HeightZero)
{
    // Arrange：99 行 → 2 位行号
    doc->setPlainText(QString(98, QChar('\n')));
    ASSERT_EQ(doc->blockCount(), 99);
    const int digits = 2;
    const int advance = leftArea->fontMetrics().horizontalAdvance(QLatin1Char('9'));

    // Act
    const QSize hint = obj->sizeHint();

    // Assert：宽 = 行号区域宽度公式结果，高 = 0（交由布局纵向拉伸）
    EXPECT_EQ(hint.width(), 13 + advance * digits + 40);
    EXPECT_EQ(hint.height(), 0);
}

// ---- mousePressEvent 委托（左/右/中键同质输入 → TEST_P） ----
struct MouseButtonCase {
    Qt::MouseButton button;
};

class LineNumberAreaMouseTest : public LineNumberAreaTest,
                                public ::testing::WithParamInterface<MouseButtonCase> {
};

TEST_P(LineNumberAreaMouseTest, MousePressEvent_AnyButton_ForwardsPositionToEdit)
{
    const MouseButtonCase &c = GetParam();

    // Arrange：合成鼠标按下事件
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(17, 42),
                      c.button, c.button, Qt::NoModifier);

    // Act：真实事件派发
    const bool delivered = QApplication::sendEvent(obj, &event);

    // Assert：位置精确转发到 Edit::onPressedLineNumber
    EXPECT_TRUE(delivered);
    EXPECT_EQ(onPressedCalls, 1);
    EXPECT_EQ(capturedPoint, QPoint(17, 42));
}

INSTANTIATE_TEST_SUITE_P(
    MouseButtonCases,
    LineNumberAreaMouseTest,
    ::testing::Values(
        MouseButtonCase{ Qt::LeftButton },
        MouseButtonCase{ Qt::RightButton },
        MouseButtonCase{ Qt::MiddleButton }));

TEST_F(LineNumberAreaTest, MousePressEvent_OriginPoint_ForwardsZero)
{
    // Arrange：边界坐标 (0,0)
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    QApplication::sendEvent(obj, &event);

    // Assert：原点坐标无损转发
    EXPECT_EQ(onPressedCalls, 1);
    EXPECT_EQ(capturedPoint, QPoint(0, 0));
}

TEST_F(LineNumberAreaTest, PaintEvent_DelegatesToLeftArea_PaintEvent)
{
    // Arrange
    QPaintEvent event(QRect(0, 0, 30, 100));

    // Act：直驱 protected paintEvent（纯委托，不涉及真实绘制）
    obj->paintEvent(&event);

    // Assert：精确转发 LeftAreaTextEdit::lineNumberAreaPaintEvent 一次
    EXPECT_EQ(areaPaintCalls, 1);
    // 二次调用计数累加（委托幂等可重复）
    obj->paintEvent(&event);
    EXPECT_EQ(areaPaintCalls, 2);
}
