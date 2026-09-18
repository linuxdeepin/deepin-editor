// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// BookMarkWidget（src/editor/bookmarkwidget.h/.cpp）单元测试
//
// 类特征：QWidget（GUI），持 LeftAreaTextEdit 指针，paintEvent 纯委托。
// 绘制验证双通道：直驱 protected paintEvent（委托计数）+ grab() 真实渲染。
//
// 方法清单完成情况（test-types §8）：
// | 1 | 公开方法 ≥1 用例：ctor/dtor/paintEvent(protected 直驱+渲染) | 完成 |
// | 2 | 等价类划分：null/真实 LeftAreaTextEdit 构造；空/非空绘制区域 | 完成 |
// | 3 | 边界值显式覆盖：(0,0) 空矩形与典型矩形 | 完成 |
// | 4 | TEST_P：无 ≥3 组同质输入（单委托方法，两组断言逻辑不同走 TEST_F） | N/A |
// | 5 | 分支清单已列出并映射用例 | N/A（无控制流分支） |
// | 6 | 每条 if 分支有触发用例 | N/A |
// | 7 | 异常路径：无 throw | N/A |
// | 8 | 负面场景：null 委托目标构造不崩溃 | 完成 |
// | 9 | 强异常安全：不适用 | N/A |
// | 10 | stub 选择：项目内具体类 → stub_ext | 完成 |
//
// 分支清单（来源：bookmarkwidget.cpp）：无控制流分支（构造存指针 + 绘制纯委托）。
//
// 用例映射：
// - Construction_StoresLeftAreaWidget                  → ctor
// - Construction_NullLeftArea_NoCrash                  → ctor 负面
// - PaintEvent_DelegatesToBookMarkAreaPaintEvent       → paintEvent 委托计数
// - PaintEvent_GrabRendering_DelegatesOnce             → paintEvent 真实渲染通道

#include <gtest/gtest.h>
#include "stubext.h"
#include "bookmarkwidget.h"
#include "leftareaoftextedit.h"
#include "dtextedit.h"

#include <QApplication>
#include <QDebug>
#include <QPaintEvent>

namespace {

TextEdit *const kFakeEdit = reinterpret_cast<TextEdit *>(quintptr(0x1000));

} // namespace

class BookMarkWidgetTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (QApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_bookmarkwidget";
            static char *argv[] = { appName, nullptr };
            s_app = new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        bookMarkPaintCalls = 0;

        // LeftAreaTextEdit 构造 qDebug 流式输出 fake TextEdit* → 拦截 QWidget 流式运算符
        stub.set_lamda(static_cast<QDebug (*)(QDebug, const QWidget *)>(&operator<<),
                       [](QDebug d, const QWidget *) -> QDebug { return d; });
        stub.set_lamda(&LeftAreaTextEdit::bookMarkAreaPaintEvent,
                       [this](LeftAreaTextEdit *, QPaintEvent *) { ++bookMarkPaintCalls; });

        leftArea = new LeftAreaTextEdit(kFakeEdit);
        obj = new BookMarkWidget(leftArea);
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
    BookMarkWidget *obj = nullptr;
    int bookMarkPaintCalls = 0;
};

QApplication *BookMarkWidgetTest::s_app = nullptr;

TEST_F(BookMarkWidgetTest, Construction_StoresLeftAreaWidget)
{
    // Assert：持有委托目标
    EXPECT_EQ(obj->m_leftAreaWidget, leftArea);
    EXPECT_EQ(obj->m_leftAreaWidget->getEdit(), kFakeEdit);
}

TEST_F(BookMarkWidgetTest, Construction_NullLeftArea_NoCrash)
{
    // Arrange/Act：null 委托目标（负面场景：构造仅存指针不解引用）
    BookMarkWidget nullArea(nullptr);

    // Assert：指针原样保存、部件可用
    EXPECT_EQ(nullArea.m_leftAreaWidget, nullptr);
    EXPECT_FALSE(nullArea.isVisible());
}

TEST_F(BookMarkWidgetTest, PaintEvent_DelegatesToBookMarkAreaPaintEvent)
{
    // Arrange：典型矩形
    QPaintEvent event(QRect(0, 0, 20, 200));

    // Act：直驱 protected paintEvent（纯委托）
    obj->paintEvent(&event);

    // Assert：精确转发一次；重复调用计数累加
    EXPECT_EQ(bookMarkPaintCalls, 1);
    obj->paintEvent(&event);
    EXPECT_EQ(bookMarkPaintCalls, 2);
}

TEST_F(BookMarkWidgetTest, PaintEvent_EmptyRect_DelegatesWithoutCrash)
{
    // Arrange：空矩形边界 (0,0)
    QPaintEvent event(QRect(0, 0, 0, 0));

    // Act
    obj->paintEvent(&event);

    // Assert：空区域仍走委托链；重复调用计数累加（委托可重复）
    EXPECT_EQ(bookMarkPaintCalls, 1);
    obj->paintEvent(&event);
    EXPECT_EQ(bookMarkPaintCalls, 2);
}

TEST_F(BookMarkWidgetTest, PaintEvent_GrabRendering_DelegatesOnce)
{
    // Arrange：给定尺寸，走真实渲染管线
    obj->resize(18, 120);

    // Act
    const QPixmap pm = obj->grab();

    // Assert：渲染触发一次委托，输出尺寸与部件一致
    EXPECT_EQ(bookMarkPaintCalls, 1);
    EXPECT_EQ(pm.size(), QSize(18, 120));
}
