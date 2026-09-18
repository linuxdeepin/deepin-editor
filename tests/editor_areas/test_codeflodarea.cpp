// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// CodeFlodArea（src/editor/codeflodarea.h/.cpp）单元测试
//
// 类特征：QWidget（GUI），持 LeftAreaTextEdit 指针，paintEvent 为 public 纯委托。
//
// 方法清单完成情况（test-types §8）：
// | 1 | 公开方法 ≥1 用例：ctor/dtor/paintEvent(public) | 完成 |
// | 2 | 等价类划分：真实/null 委托目标；空/典型绘制区域 | 完成 |
// | 3 | 边界值显式覆盖：(0,0) 空矩形与典型矩形 | 完成 |
// | 4 | TEST_P：无 ≥3 组同质输入（委托断言逻辑单一） | N/A |
// | 5 | 分支清单已列出并映射用例 | N/A（无控制流分支） |
// | 6 | 每条 if 分支有触发用例 | N/A |
// | 7 | 异常路径：无 throw | N/A |
// | 8 | 负面场景：null 委托目标构造不崩溃 | 完成 |
// | 9 | 强异常安全：不适用 | N/A |
// | 10 | stub 选择：项目内具体类 → stub_ext | 完成 |
//
// 分支清单（来源：codeflodarea.cpp）：无控制流分支（构造存指针 + 绘制纯委托）。
//
// 用例映射：
// - Construction_StoresLeftAreaWidget                  → ctor
// - Construction_NullLeftArea_NoCrash                  → ctor 负面
// - PaintEvent_DelegatesToCodeFlodAreaPaintEvent       → paintEvent 委托计数
// - PaintEvent_EmptyRect_DelegatesWithoutCrash         → 空矩形边界
// - PaintEvent_GrabRendering_DelegatesOnce             → 真实渲染通道

#include <gtest/gtest.h>
#include "stubext.h"
#include "codeflodarea.h"
#include "leftareaoftextedit.h"
#include "dtextedit.h"

#include <QApplication>
#include <QDebug>
#include <QPaintEvent>

namespace {

TextEdit *const kFakeEdit = reinterpret_cast<TextEdit *>(quintptr(0x1000));

} // namespace

class CodeFlodAreaTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (QApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_codeflodarea";
            static char *argv[] = { appName, nullptr };
            s_app = new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        flodPaintCalls = 0;

        // LeftAreaTextEdit 构造 qDebug 流式输出 fake TextEdit* → 拦截 QWidget 流式运算符
        stub.set_lamda(static_cast<QDebug (*)(QDebug, const QWidget *)>(&operator<<),
                       [](QDebug d, const QWidget *) -> QDebug { return d; });
        stub.set_lamda(&LeftAreaTextEdit::codeFlodAreaPaintEvent,
                       [this](LeftAreaTextEdit *, QPaintEvent *) { ++flodPaintCalls; });

        leftArea = new LeftAreaTextEdit(kFakeEdit);
        obj = new CodeFlodArea(leftArea);
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
    CodeFlodArea *obj = nullptr;
    int flodPaintCalls = 0;
};

QApplication *CodeFlodAreaTest::s_app = nullptr;

TEST_F(CodeFlodAreaTest, Construction_StoresLeftAreaWidget)
{
    // Assert：持有委托目标
    EXPECT_EQ(obj->m_pLeftAreaWidget, leftArea);
    EXPECT_EQ(obj->m_pLeftAreaWidget->getEdit(), kFakeEdit);
}

TEST_F(CodeFlodAreaTest, Construction_NullLeftArea_NoCrash)
{
    // Arrange/Act：null 委托目标（负面场景）
    CodeFlodArea nullArea(nullptr);

    // Assert
    EXPECT_EQ(nullArea.m_pLeftAreaWidget, nullptr);
    EXPECT_FALSE(nullArea.isVisible());
}

TEST_F(CodeFlodAreaTest, PaintEvent_DelegatesToCodeFlodAreaPaintEvent)
{
    // Arrange
    QPaintEvent event(QRect(0, 0, 16, 300));

    // Act：paintEvent 为 public，直接调用
    obj->paintEvent(&event);

    // Assert：精确转发一次；重复调用计数累加
    EXPECT_EQ(flodPaintCalls, 1);
    obj->paintEvent(&event);
    EXPECT_EQ(flodPaintCalls, 2);
}

TEST_F(CodeFlodAreaTest, PaintEvent_EmptyRect_DelegatesWithoutCrash)
{
    // Arrange：空矩形边界
    QPaintEvent event(QRect(0, 0, 0, 0));

    // Act
    obj->paintEvent(&event);

    // Assert：空区域仍走委托链；重复调用计数累加（委托可重复）
    EXPECT_EQ(flodPaintCalls, 1);
    obj->paintEvent(&event);
    EXPECT_EQ(flodPaintCalls, 2);
}

TEST_F(CodeFlodAreaTest, PaintEvent_GrabRendering_DelegatesOnce)
{
    // Arrange
    obj->resize(16, 300);

    // Act
    const QPixmap pm = obj->grab();

    // Assert
    EXPECT_EQ(flodPaintCalls, 1);
    EXPECT_EQ(pm.size(), QSize(16, 300));
}
