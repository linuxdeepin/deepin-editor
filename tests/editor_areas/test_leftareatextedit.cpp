// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// LeftAreaTextEdit（src/editor/leftareaoftextedit.h/.cpp）单元测试
//
// 类特征：QWidget（GUI），构造仅持有 TextEdit 指针不解引用；三个子区域
// （行号/书签/折叠）在构造时创建。TextEdit 为重依赖具体类（非虚接口），
// 按 stub 决策矩阵用 fake 指针 + stub_ext 拦截其全部被调方法：
//   lineNumberAreaPaintEvent / bookMarkAreaPaintEvent / codeFLodAreaPaintEvent /
//   getBackColor / onPressedLineNumber / QPlainTextEdit::document
// 构造/QWidget 行为在 QT_QPA_PLATFORM=offscreen 下真实执行。
//
// 方法清单完成情况（test-types §8）：
// | 1 | 公开方法 ≥1 用例：ctor/dtor/getEdit/lineNumberAreaPaintEvent/lineNumberAreaWidth/
//     bookMarkAreaPaintEvent/codeFlodAreaPaintEvent/updateLineNumber/updateBookMark/
//     updateCodeFlod/updateAll/paintEvent(protected 直驱) | 完成 |
// | 2 | 等价类划分：行数 1~9/10~99/100~999/1000+；背景亮/暗；子区域空/非空 | 完成 |
// | 3 | 边界值显式覆盖：blockCount 1/9/10/99/100/1000（进位边界） | 完成 |
// | 4 | TEST_P 参数化（≥3 组同质输入）：lineNumberAreaWidth 行数 | 完成 |
// | 5 | 分支清单已列出并映射用例 | 完成（见下） |
// | 6 | 每条 if 分支有触发用例 | 完成 |
// | 7 | 异常路径：无 throw | N/A |
// | 8 | 负面场景：null TextEdit 构造、子区域指针置空守卫 | 完成 |
// | 9 | 强异常安全：守卫分支不触发任何子区域重绘 | 完成 |
// | 10 | stub 选择：Qt 内置类/项目内不可注入类 → stub_ext | 完成 |
//
// 分支清单（来源：leftareaoftextedit.cpp）：
//   B1: max>=10 循环（位数计算，含 0 次与多次迭代）
//   B2: getBackColor().lightness() < 128（暗背景 → alphaF 0.06）
//   B3: else（亮背景 → alphaF 0.03）
//   B4~B6: updateLineNumber/updateBookMark/updateCodeFlod 的子区域判空守卫
//   B7~B9: updateAll 的三个子区域判空守卫
//
// 用例映射：
// - Construction_WithNullEdit_CreatesThreeChildAreas        → ctor
// - GetEdit_ReturnsInjectedEditPointer                      → getEdit
// - LineNumberAreaWidth_BlockCounts_ComputeDigits /*TEST_P*/ → B1
// - PaintDelegation_ThreeAreas_ForwardToEdit                → 三个委托方法
// - PaintEvent_DarkBackground_FillsWithAlpha006             → B2
// - PaintEvent_LightBackground_FillsWithAlpha003            → B3
// - UpdateLineNumber_OnlyLineAreaRepainted                  → B4 真
// - UpdateBookMark_OnlyBookMarkRepainted                    → B5 真
// - UpdateCodeFlod_OnlyFlodAreaRepainted                    → B6 真
// - UpdateAll_AllThreeAreasRepainted                        → B7~B9 真
// - UpdateMethods_NullChildGuard_NoRepaintNoCrash           → B4~B9 假

#include <gtest/gtest.h>
#include "stubext.h"
#include "leftareaoftextedit.h"
#include "dtextedit.h"

#include <QApplication>
#include <QDebug>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QHBoxLayout>
#include <QImage>
#include <climits>

namespace {

// fake TextEdit 指针：构造仅存储不解引用；所有被调方法均被 stub 拦截
TextEdit *const kFakeEdit = reinterpret_cast<TextEdit *>(quintptr(0x1000));

} // namespace

class LeftAreaTextEditTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // QWidget 子类：offscreen QApplication（Wave1 已验证无头构造可行）
        if (QApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_leftareatextedit";
            static char *argv[] = { appName, nullptr };
            s_app = new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        paintLineCalls = 0;
        paintBookMarkCalls = 0;
        paintFlodCalls = 0;
        capturedFillColors.clear();
        updateCounts.clear();

        doc = std::make_unique<QTextDocument>();

        // 源码构造/各方法 qDebug 会流式输出 TextEdit*(QWidget*)：
        // operator<<(QDebug, QWidget*) 会解引用取 metaObject → fake 指针会被解引用，
        // 拦截该流式运算符保证日志安全（返回流本身，行为等价静默）
        stub.set_lamda(static_cast<QDebug (*)(QDebug, const QWidget *)>(&operator<<),
                       [](QDebug d, const QWidget *) -> QDebug { return d; });

        // ---- stub 矩阵（TearDown 统一 clear） ----
        // TextEdit 委托目标（本项目具体类、不可注入 → stub_ext）
        stub.set_lamda(&TextEdit::lineNumberAreaPaintEvent,
                       [this](TextEdit *, QPaintEvent *) { ++paintLineCalls; });
        stub.set_lamda(&TextEdit::bookMarkAreaPaintEvent,
                       [this](TextEdit *, QPaintEvent *) { ++paintBookMarkCalls; });
        stub.set_lamda(&TextEdit::codeFLodAreaPaintEvent,
                       [this](TextEdit *, QPaintEvent *) { ++paintFlodCalls; });
        // 背景色由测试注入（分支 B2/B3）
        stub.set_lamda(&TextEdit::getBackColor,
                       [this](TextEdit *) -> QColor { return injectedBackColor; });
        // lineNumberAreaWidth 经 document()->blockCount() 取行数 → 喂入受控文档
        stub.set_lamda(&QPlainTextEdit::document,
                       [this](QPlainTextEdit *) -> QTextDocument * { return doc.get(); });
        // 子区域重绘调度计数（update() 无参重载）
        stub.set_lamda(static_cast<void (QWidget::*)()>(&QWidget::update),
                       [this](QWidget *w) { ++updateCounts[w]; });
        // paintEvent 填充色捕获（验证 B2/B3 的 alphaF 分支值）
        stub.set_lamda(
            static_cast<void (QPainter::*)(const QRect &, const QColor &)>(&QPainter::fillRect),
            [this](QPainter *, const QRect &, const QColor &c) {
                capturedFillColors.append(c);
            });

        injectedBackColor = QColor(10, 10, 10);   // 默认暗背景
        obj = new LeftAreaTextEdit(kFakeEdit);
        ASSERT_NE(obj, nullptr);
    }

    void TearDown() override
    {
        delete obj;
        obj = nullptr;
        stub.clear();
    }

    static QApplication *s_app;

    // 生成 blockCount 个块的文档（N-1 个换行符）
    void makeDocWithBlocks(int blockCount)
    {
        doc = std::make_unique<QTextDocument>();
        if (blockCount > 1)
            doc->setPlainText(QString(blockCount - 1, QChar('\n')));
        ASSERT_EQ(doc->blockCount(), blockCount);
    }

    int updatesOf(const QWidget *w) const { return updateCounts.value(const_cast<QWidget *>(w), 0); }

    stub_ext::StubExt stub;
    LeftAreaTextEdit *obj = nullptr;
    std::unique_ptr<QTextDocument> doc;
    QColor injectedBackColor;

    int paintLineCalls = 0;
    int paintBookMarkCalls = 0;
    int paintFlodCalls = 0;
    QVector<QColor> capturedFillColors;
    QMap<QWidget *, int> updateCounts;

    // 在捕获列表中查找"基色=期望 rgb 且 alphaF≈期望值"的填充
    //（Qt6 QColor alpha 按 8bit 量化往返，0.06→0.0599985，容差取 1e-4 仍可区分 0.06/0.03）
    bool containsFill(const QColor &expectRgb, qreal expectAlpha) const
    {
        for (const QColor &c : capturedFillColors) {
            if (c.rgb() == expectRgb.rgb() && qAbs(c.alphaF() - expectAlpha) < 1e-4)
                return true;
        }
        return false;
    }
};

QApplication *LeftAreaTextEditTest::s_app = nullptr;

TEST_F(LeftAreaTextEditTest, Construction_WithNullEdit_CreatesThreeChildAreas)
{
    // Arrange：null TextEdit（构造仅存指针，负面场景）
    LeftAreaTextEdit area(nullptr);

    // Assert：三个子区域全部创建并接入布局（负面输入不破坏构造）
    ASSERT_NE(area.m_pLineNumberArea, nullptr);
    ASSERT_NE(area.m_pBookMarkArea, nullptr);
    ASSERT_NE(area.m_pFlodArea, nullptr);
    EXPECT_EQ(area.getEdit(), nullptr);
    EXPECT_NE(area.layout(), nullptr);
    // 布局零边距零间距（区域紧贴排布约定）
    auto *lay = static_cast<QHBoxLayout *>(area.layout());
    EXPECT_EQ(lay->contentsMargins(), QMargins(0, 0, 0, 0));
    EXPECT_EQ(lay->spacing(), 0);
}

TEST_F(LeftAreaTextEditTest, GetEdit_ReturnsInjectedEditPointer)
{
    // Arrange 在 SetUp（注入 kFakeEdit）

    // Act & Assert：原样返回注入指针
    EXPECT_EQ(obj->getEdit(), kFakeEdit);
    // 独立实例互不影响（指针隔离）
    LeftAreaTextEdit other(nullptr);
    EXPECT_EQ(other.getEdit(), nullptr);
}

// ---- lineNumberAreaWidth 位数计算（B1 循环：0 次/1 次/多次迭代）----
struct BlockCountCase {
    int blockCount;
};

class LeftAreaWidthTest : public LeftAreaTextEditTest,
                          public ::testing::WithParamInterface<BlockCountCase> {
};

TEST_P(LeftAreaWidthTest, LineNumberAreaWidth_BlockCounts_ComputeDigits)
{
    const BlockCountCase &c = GetParam();

    // Arrange：受控文档喂入指定行数
    makeDocWithBlocks(c.blockCount);
    const int digits = QString::number(qMax(1, c.blockCount)).length();   // 位数独立 oracle
    const int advance = obj->fontMetrics().horizontalAdvance(QLatin1Char('9'));

    // Act
    const int width = obj->lineNumberAreaWidth();

    // Assert：13 + 9 宽度*位数 + 40（源算法），进位边界逐点验证
    EXPECT_EQ(width, 13 + advance * digits + 40);
    // 分解断言：净宽（去固定内边距 53）= 单字符宽 × 位数
    EXPECT_EQ(width - (13 + 40), advance * digits);
    EXPECT_GT(width, 53);
}

INSTANTIATE_TEST_SUITE_P(
    BlockCountCases,
    LeftAreaWidthTest,
    ::testing::Values(
        BlockCountCase{ 1 },     // 下边界：单行（循环 0 次迭代）
        BlockCountCase{ 9 },     // 边界：仍 1 位
        BlockCountCase{ 10 },    // 边界：进位 2 位
        BlockCountCase{ 99 },
        BlockCountCase{ 100 },   // 进位 3 位
        BlockCountCase{ 1000 })); // 进位 4 位

TEST_F(LeftAreaTextEditTest, PaintDelegation_ThreeAreas_ForwardToEdit)
{
    // Arrange
    QPaintEvent event(QRect(0, 0, 40, 120));

    // Act：三个绘制委托依次触发
    obj->lineNumberAreaPaintEvent(&event);
    obj->bookMarkAreaPaintEvent(&event);
    obj->codeFlodAreaPaintEvent(&event);

    // Assert：每次调用精确转发到 TextEdit 对应方法一次
    EXPECT_EQ(paintLineCalls, 1);
    EXPECT_EQ(paintBookMarkCalls, 1);
    EXPECT_EQ(paintFlodCalls, 1);
}

TEST_F(LeftAreaTextEditTest, PaintEvent_DarkBackground_FillsWithAlpha006)
{
    // Arrange：暗背景（lightness 10 < 128 → B2）
    injectedBackColor = QColor(10, 10, 10);
    obj->resize(60, 80);

    // Act：真实渲染管线触发 paintEvent
    const QPixmap pm = obj->grab();

    // Assert：填充色 = brightText 且 alphaF=0.06
    ASSERT_FALSE(capturedFillColors.isEmpty());
    EXPECT_TRUE(containsFill(obj->palette().brightText().color(), 0.06));
    // 渲染输出尺寸与部件一致
    EXPECT_EQ(pm.size(), QSize(60, 80));
}

TEST_F(LeftAreaTextEditTest, PaintEvent_LightBackground_FillsWithAlpha003)
{
    // Arrange：亮背景（lightness 250 >= 128 → B3）
    injectedBackColor = QColor(250, 250, 250);
    obj->resize(60, 80);

    // Act
    const QPixmap pm = obj->grab();

    // Assert：同一基色、alphaF 减半为 0.03
    ASSERT_FALSE(capturedFillColors.isEmpty());
    EXPECT_TRUE(containsFill(obj->palette().brightText().color(), 0.03));
    EXPECT_EQ(pm.size(), QSize(60, 80));
}

TEST_F(LeftAreaTextEditTest, UpdateLineNumber_OnlyLineAreaRepainted)
{
    // Act
    obj->updateLineNumber();

    // Assert：仅行号区域被调度重绘（精确 1 次，其余区域 0 次）
    EXPECT_EQ(updatesOf(obj->m_pLineNumberArea), 1);
    EXPECT_EQ(updatesOf(obj->m_pBookMarkArea), 0);
    EXPECT_EQ(updatesOf(obj->m_pFlodArea), 0);
}

TEST_F(LeftAreaTextEditTest, UpdateBookMark_OnlyBookMarkRepainted)
{
    // Act
    obj->updateBookMark();

    // Assert
    EXPECT_EQ(updatesOf(obj->m_pBookMarkArea), 1);
    EXPECT_EQ(updatesOf(obj->m_pLineNumberArea), 0);
    EXPECT_EQ(updatesOf(obj->m_pFlodArea), 0);
}

TEST_F(LeftAreaTextEditTest, UpdateCodeFlod_OnlyFlodAreaRepainted)
{
    // Act
    obj->updateCodeFlod();

    // Assert
    EXPECT_EQ(updatesOf(obj->m_pFlodArea), 1);
    EXPECT_EQ(updatesOf(obj->m_pLineNumberArea), 0);
    EXPECT_EQ(updatesOf(obj->m_pBookMarkArea), 0);
}

TEST_F(LeftAreaTextEditTest, UpdateAll_AllThreeAreasRepainted)
{
    // Act
    obj->updateAll();

    // Assert：三个区域各恰好 1 次
    EXPECT_EQ(updatesOf(obj->m_pLineNumberArea), 1);
    EXPECT_EQ(updatesOf(obj->m_pBookMarkArea), 1);
    EXPECT_EQ(updatesOf(obj->m_pFlodArea), 1);
}

TEST_F(LeftAreaTextEditTest, UpdateMethods_NullChildGuard_NoRepaintNoCrash)
{
    // Arrange：子区域指针全部置空（覆盖判空守卫假分支，测后恢复）
    LineNumberArea *lineArea = obj->m_pLineNumberArea;
    BookMarkWidget *bookArea = obj->m_pBookMarkArea;
    CodeFlodArea *flodArea = obj->m_pFlodArea;
    obj->m_pLineNumberArea = nullptr;
    obj->m_pBookMarkArea = nullptr;
    obj->m_pFlodArea = nullptr;

    // Act：守卫路径不应崩溃
    obj->updateLineNumber();
    obj->updateBookMark();
    obj->updateCodeFlod();
    obj->updateAll();

    // Assert：强异常安全 —— 无任何子区域被调度重绘（真实区域 0 次）
    EXPECT_EQ(updatesOf(lineArea), 0);
    EXPECT_EQ(updatesOf(bookArea), 0);
    EXPECT_EQ(updatesOf(flodArea), 0);

    // Cleanup：恢复指针（TearDown 释放）
    obj->m_pLineNumberArea = lineArea;
    obj->m_pBookMarkArea = bookArea;
    obj->m_pFlodArea = flodArea;
    EXPECT_EQ(updatesOf(lineArea), 0);
}
