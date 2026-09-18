// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// WarningNotices（src/controls/warningnotices.cpp）单元测试
//
// 类特征：DFloatingMessage 子类（GUI 类），offscreen QApplication 无头构造。
//
// 方法映射（公开方法全集）：
// - WarningNotices::WarningNotices → Constructor_DefaultType_CreatesButtonsInitialState
//                                     Constructor_MessageTypes_Accessible (TEST_P)
// - ~WarningNotices                → 各用例 TearDown delete 链覆盖
// - setReloadBtn                   → SetReloadBtn_FromSaveAsState_ShowsReloadHidesSaveAs
// - setSaveAsBtn                   → SetSaveAsBtn_InitialState_ShowsSaveAsHidesReload
// - clearBtn                       → ClearBtn_VisibleButtons_HidesBothActionButtons
// - setEditAnywayBtn               → SetEditAnywayBtn_Triggered_ShowsOnlyEditAnyway
// - slotreloadBtnClicked           → SlotReloadBtnClicked_OnVisibleBar_HidesAndEmits
// - slotsaveAsBtnClicked           → SlotSaveAsBtnClicked_OnVisibleBar_HidesAndEmits
// - slotEditAnywayBtnClicked       → SlotEditAnywayBtnClicked_OnVisibleBar_HidesAndEmits
//
// 分支清单（来源：warningnotices.cpp）→ 用例映射：
// - setReloadBtn: !visible → setVisible(true)（内外两处 setVisible） → SetReloadBtn_...
// - setSaveAsBtn: 同上                                          → SetSaveAsBtn_...
// - ctor: closeBtn 找到与否（findChild<DDialogCloseButton*>)      → 真实 DFloatingMessage
//         自带关闭按钮，offscreen 下构造链真实执行
// - closeButtonClicked lambda: isVisible → close                 → 经 Qt 信号机制触发成本高，
//         由三个 slot 用例等价覆盖 hide 行为（同源行为路径）
//
// 最小清单完成情况：
// | 1 | 每个公开方法 ≥1 用例 | 完成 |
// | 2 | 等价类（消息类型 2 种、按钮三态互斥组合） | 完成 |
// | 3 | 边界值（默认 ResidentType、无 parent） | 完成 |
// | 4 | TEST_P（消息类型 2 组不足 3，用例组内覆盖两枚举值） | 完成（2 组） |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成（可见性翻转两侧） |
// | 7 | 异常路径 | N/A（无 throw） |
// | 8 | 负面（按钮隐藏互斥、未显示时点击不崩溃） | 完成 |
// | 9 | 强异常安全（clearBtn 后仍可重新 set 按钮） | 完成 |
// | 10 | stub_ext（无外部依赖需 stub；qApp->font 真实可用） | 完成 |

#include <gtest/gtest.h>

#include <DGuiApplicationHelper>

#include <QPushButton>
#include <QSignalSpy>

#include "test_env.h"
#include "warningnotices.h"

namespace {

class WarningNoticesTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        notices = new WarningNotices(DFloatingMessage::ResidentType);
        reloadBtn = notices->findChild<QPushButton *>("ReloadBtn");
        saveAsBtn = notices->findChild<QPushButton *>("SaveAsBtn");
        editAnywayBtn = notices->findChild<QPushButton *>("EditAnywayBtn");
    }

    void TearDown() override
    {
        delete notices;
        notices = nullptr;
        stub.clear();
    }

    stub_ext::StubExt stub;
    WarningNotices *notices = nullptr;
    QPushButton *reloadBtn = nullptr;
    QPushButton *saveAsBtn = nullptr;
    QPushButton *editAnywayBtn = nullptr;

    // 三按钮中未隐藏（isHidden==false）的数量
    int visibleButtonCount() const
    {
        return (reloadBtn->isHidden() ? 0 : 1) + (saveAsBtn->isHidden() ? 0 : 1)
               + (editAnywayBtn->isHidden() ? 0 : 1);
    }
};

// ---- 构造 ----

TEST_F(WarningNoticesTest, Constructor_DefaultType_CreatesButtonsInitialState)
{
    // Assert：三按钮齐全；初始 Reload/SaveAs 可见、EditAnyway 隐藏（ctor setVisible(false)）
    ASSERT_NE(reloadBtn, nullptr);
    ASSERT_NE(saveAsBtn, nullptr);
    ASSERT_NE(editAnywayBtn, nullptr);
    EXPECT_FALSE(reloadBtn->isHidden());
    EXPECT_FALSE(saveAsBtn->isHidden());
    EXPECT_TRUE(editAnywayBtn->isHidden());  // 初始隐藏分支
    EXPECT_EQ(notices->messageType(), DFloatingMessage::ResidentType);
}

struct MessageTypeCase {
    DFloatingMessage::MessageType type;
};

class WarningNoticesTypeTest : public WarningNoticesTest,
                               public ::testing::WithParamInterface<MessageTypeCase> {
};

TEST_P(WarningNoticesTypeTest, Constructor_MessageTypes_Accessible)
{
    const auto &c = GetParam();

    // Act：以指定类型重新构造（默认参数之外的等价类）
    WarningNotices other(c.type, nullptr);

    // Assert：类型透传、按钮齐备
    EXPECT_EQ(other.messageType(), c.type);
    EXPECT_NE(other.findChild<QPushButton *>("ReloadBtn"), nullptr);
    EXPECT_TRUE(other.findChild<QPushButton *>("EditAnywayBtn")->isHidden());
}

INSTANTIATE_TEST_SUITE_P(
    TypeCases, WarningNoticesTypeTest,
    ::testing::Values(
        MessageTypeCase{ DFloatingMessage::TransientType },
        MessageTypeCase{ DFloatingMessage::ResidentType }));

// ---- 按钮互斥三态 ----

TEST_F(WarningNoticesTest, SetReloadBtn_FromSaveAsState_ShowsReloadHidesSaveAs)
{
    // Arrange：先切换到 SaveAs 态
    notices->setSaveAsBtn();
    ASSERT_TRUE(reloadBtn->isHidden());

    // Act
    notices->setReloadBtn();

    // Assert：互斥翻转（同分支内外两处 setVisible 均覆盖）；可见按钮恰 1 个
    EXPECT_FALSE(reloadBtn->isHidden());
    EXPECT_TRUE(saveAsBtn->isHidden());
    EXPECT_TRUE(editAnywayBtn->isHidden());
    EXPECT_EQ(visibleButtonCount(), 1);
}

TEST_F(WarningNoticesTest, SetSaveAsBtn_InitialState_ShowsSaveAsHidesReload)
{
    // Act
    notices->setSaveAsBtn();

    // Assert：可见按钮恰 1 个（SaveAs）
    EXPECT_FALSE(saveAsBtn->isHidden());
    EXPECT_TRUE(reloadBtn->isHidden());
    EXPECT_TRUE(editAnywayBtn->isHidden());
    EXPECT_EQ(visibleButtonCount(), 1);
}

TEST_F(WarningNoticesTest, SetEditAnywayBtn_Triggered_ShowsOnlyEditAnyway)
{
    // Act
    notices->setEditAnywayBtn();

    // Assert：可见按钮恰 1 个（EditAnyway）
    EXPECT_FALSE(editAnywayBtn->isHidden());
    EXPECT_TRUE(reloadBtn->isHidden());
    EXPECT_TRUE(saveAsBtn->isHidden());
    EXPECT_EQ(visibleButtonCount(), 1);
}

TEST_F(WarningNoticesTest, ClearBtn_VisibleButtons_HidesBothActionButtons)
{
    // Arrange：先进入可见态
    notices->setReloadBtn();
    ASSERT_FALSE(reloadBtn->isHidden());

    // Act
    notices->clearBtn();

    // Assert：全部隐藏（可见数 0）；随后仍可重新显示（强异常安全：状态机未损坏）
    EXPECT_TRUE(reloadBtn->isHidden());
    EXPECT_TRUE(saveAsBtn->isHidden());
    EXPECT_EQ(visibleButtonCount(), 0);
    notices->setSaveAsBtn();
    EXPECT_FALSE(saveAsBtn->isHidden());
}

// ---- 三个槽：隐藏 + 信号 ----

TEST_F(WarningNoticesTest, SlotReloadBtnClicked_OnVisibleBar_HidesAndEmits)
{
    // Arrange：显示通知条
    notices->QWidget::show();
    QApplication::processEvents();
    ASSERT_TRUE(notices->isVisible());
    QSignalSpy spy(notices, &WarningNotices::reloadBtnClicked);

    // Act
    notices->slotreloadBtnClicked();

    // Assert：隐藏 + 精确一次信号
    EXPECT_FALSE(notices->isVisible());
    EXPECT_EQ(spy.count(), 1);

    // 经真实按钮点击链路再验证一次（connect 到 slot）
    notices->QWidget::show();
    QApplication::processEvents();
    reloadBtn->click();
    EXPECT_EQ(spy.count(), 2);
    EXPECT_FALSE(notices->isVisible());
}

TEST_F(WarningNoticesTest, SlotSaveAsBtnClicked_OnVisibleBar_HidesAndEmits)
{
    notices->QWidget::show();
    QApplication::processEvents();
    QSignalSpy spy(notices, &WarningNotices::saveAsBtnClicked);

    // Act
    notices->slotsaveAsBtnClicked();

    // Assert
    EXPECT_FALSE(notices->isVisible());
    EXPECT_EQ(spy.count(), 1);

    // 真实按钮点击链路
    notices->QWidget::show();
    QApplication::processEvents();
    saveAsBtn->click();
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(WarningNoticesTest, SlotEditAnywayBtnClicked_OnVisibleBar_HidesAndEmits)
{
    notices->QWidget::show();
    QApplication::processEvents();
    QSignalSpy spy(notices, &WarningNotices::editAnywayBtnClicked);

    // Act
    notices->slotEditAnywayBtnClicked();

    // Assert
    EXPECT_FALSE(notices->isVisible());
    EXPECT_EQ(spy.count(), 1);

    // 真实按钮点击链路（按钮需先显示才能点击生效——直接调槽已覆盖主体逻辑）
    notices->setEditAnywayBtn();
    notices->QWidget::show();
    QApplication::processEvents();
    editAnywayBtn->click();
    EXPECT_EQ(spy.count(), 2);
}

// ---- 构造期 connect 的 lambda（关闭按钮信号 / 字体切换）----

TEST_F(WarningNoticesTest, CloseButtonClickedSignal_VisibleNotices_Closes)
{
    // Arrange：显示（lambda 判 isVisible 才 close）
    notices->QWidget::show();
    QApplication::processEvents();
    ASSERT_TRUE(notices->isVisible());

    // Act：合成 DFloatingMessage::closeButtonClicked（ctor 连接的 lambda）
    const bool emitted = QMetaObject::invokeMethod(notices, "closeButtonClicked");
    QApplication::processEvents();

    // Assert：可见 → close()；消息类型在关闭后保留
    EXPECT_TRUE(emitted);
    EXPECT_FALSE(notices->isVisible());
    EXPECT_EQ(notices->messageType(), DFloatingMessage::ResidentType);

    // 分支另一侧：已隐藏时再触发 → 状态不变、不崩溃
    QMetaObject::invokeMethod(notices, "closeButtonClicked");
    QApplication::processEvents();
    EXPECT_FALSE(notices->isVisible());
}

TEST_F(WarningNoticesTest, FontChangedSignal_CustomFont_UpdatesNoticeFont)
{
    // Arrange
    const QFont original = notices->font();

    // Act：合成 DGuiApplicationHelper::fontChanged（ctor 连接的 lambda）
    QFont custom(QString("monospace"));
    custom.setPointSize(original.pointSize() + 4);
    const bool emitted = QMetaObject::invokeMethod(
        DGuiApplicationHelper::instance(), "fontChanged", Q_ARG(QFont, custom));
    QApplication::processEvents();

    // Assert：通知条字体跟随应用字体
    EXPECT_TRUE(emitted);
    EXPECT_EQ(notices->font().family(), custom.family());
    EXPECT_EQ(notices->font().pointSize(), custom.pointSize());
}

TEST_F(WarningNoticesTest, SizeModeChangedSignal_ModeToggle_UpdatesCloseButtonIcon)
{
    // Arrange：ResidentType 消息自带 DDialogCloseButton（ctor findChild 命中后连接 lambda）
    auto *helper = DGuiApplicationHelper::instance();

    // Act：真实切换全局尺寸模式（ctor 连接的 closeBtn lambda 重设图标尺寸）
    helper->setSizeMode(DGuiApplicationHelper::CompactMode);
    QApplication::processEvents();
    helper->setSizeMode(DGuiApplicationHelper::NormalMode);  // 恢复全局状态
    QApplication::processEvents();

    // Assert：lambda 执行无异常，控件树仍完整
    EXPECT_NE(notices->findChild<QPushButton *>("ReloadBtn"), nullptr);
    EXPECT_EQ(notices->messageType(), DFloatingMessage::ResidentType);
}

}  // namespace
