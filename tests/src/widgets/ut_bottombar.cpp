// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_bottombar.h"

#include <QPushButton>
#include <QPaintEvent>
#include <QAction>
#include <QSignalSpy>
#include "../../src/widgets/window.h"
#include "../../src/editor/editwrapper.h"
#include "../stub.h"

// Stub: 强制 getFileLoading 返回 true，覆盖编码 lambda 的回退分支
static bool stub_getFileLoading_true()
{
    return true;
}

// Stub: reloadFileHighlight 空实现，避免外部副作用
static void stub_reloadFileHighlight(QString)
{
}

// 测试函数 BottomBar::updatePosition
TEST_F(TestBottomBar, checkUpdatePosition)
{
    auto bottomBar = new BottomBar;
    bottomBar->updatePosition(1, 1);

    EXPECT_EQ(bottomBar->m_pPositionLabel->text().contains("1"),true);

    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();
}

// 测试函数 BottomBar::updateWordCount
TEST_F(TestBottomBar, checkUpdateWordCount)
{
    auto bottomBar = new BottomBar;
    bottomBar->updateWordCount(1);

    EXPECT_EQ(bottomBar->m_pCharCountLabel->text().contains("0"),true);

    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();
}

// 测试函数 BottomBar::setEncodeName
TEST_F(TestBottomBar, checkSetEncodeName)
{
    auto bottomBar = new BottomBar;
    bottomBar->setEncodeName("UTF-8");


    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();
}

// 测试函数 BottomBar::setPalette
TEST_F(TestBottomBar, checkSetPalette)
{
    auto bottomBar = new BottomBar;

    QString backgroundColor = "#f8f8f8";
    QString textColor = "#1f1c1b";
    QPalette palette = bottomBar->palette();
    palette.setColor(QPalette::Window, backgroundColor);
    palette.setColor(QPalette::Text, textColor);
    bottomBar->setPalette(palette);

    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();
}

// 测试函数 BottomBar::updateSize
TEST_F(TestBottomBar, checkUpdateSize)
{
    auto bottomBar = new BottomBar;
    bottomBar->updateSize(32, false);
    EXPECT_EQ(bottomBar->height(),32);


    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();
}

// 测试函数 BottomBar::setChildrenFocus
TEST_F(TestBottomBar, checkSetChildrenFocus)
{

    // 场景1: ok = false, preOrderWidget = nullptr
    auto bottomBar = new BottomBar;
    bottomBar->setChildrenFocus(false, nullptr);
    EXPECT_NE(bottomBar->m_pEncodeMenu->hasFocus(),true);
    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();




    // 场景2: ok = true, preOrderWidget = nullptr
    bottomBar = new BottomBar;
    bottomBar->setChildrenFocus(true, nullptr);
    EXPECT_NE(bottomBar->m_pEncodeMenu->hasFocus(),true);
    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();



    // 场景3: ok = true, preOrderWidget = new QWidget()
    bottomBar = new BottomBar;
    QPushButton *button = new QPushButton();
    bottomBar->setChildrenFocus(true, button);
    EXPECT_NE(button->hasFocus(),true);
    EXPECT_NE(bottomBar,nullptr);
    EXPECT_NE(button,nullptr);
    bottomBar->deleteLater();
    button->deleteLater();



    // 场景4: ok = false, preOrderWidget = new QWidget()
    bottomBar = new BottomBar;
    button = new QPushButton();
    bottomBar->setChildrenFocus(false, button);
    EXPECT_NE(button->hasFocus(),true);
    EXPECT_NE(bottomBar,nullptr);
    EXPECT_NE(button,nullptr);
    bottomBar->deleteLater();
    button->deleteLater();

}

// 测试函数 BottomBar::getEncodeMenu
TEST_F(TestBottomBar, checkGetEncodeMenu)
{

    auto bottomBar = new BottomBar;
    DDropdownMenu *menu = bottomBar->getEncodeMenu();
    EXPECT_NE(menu, nullptr);
    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();

}

// 测试函数 BottomBar::getHighlightMenu
TEST_F(TestBottomBar, checkGetHighlightMenu)
{
    auto bottomBar = new BottomBar;
    DDropdownMenu *menu = bottomBar->getHighlightMenu();
    EXPECT_NE(menu, nullptr);
    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();

}

// 测试函数 BottomBar::paintEvent
TEST_F(TestBottomBar, checkPaintEvent)
{
    auto bottomBar = new BottomBar;
    QPaintEvent event(bottomBar->rect());
    bottomBar->paintEvent(&event);
    EXPECT_NE(bottomBar,nullptr);
    bottomBar->deleteLater();

}

// 测试函数 BottomBar::onFormatMenuTrigged
TEST_F(TestBottomBar, checkOnFormatMenuTrigged)
{
    // 使用无 parent 的 BottomBar(m_pWrapper=nullptr)，覆盖提前返回分支(不触碰 m_pWrapper)
    // 场景1: action 为空，提前返回
    bottomBar->onFormatMenuTrigged(nullptr);

    // 场景2: action 的格式与当前一致(Unix)，提前返回
    QAction sameAction;
    sameAction.setProperty(FormatActionType, BottomBar::EndlineFormat::Unix);
    bottomBar->onFormatMenuTrigged(&sameAction);
    EXPECT_EQ(bottomBar->getEndlineFormat(), BottomBar::EndlineFormat::Unix);
}

// 测试函数 BottomBar::slotSetTextEditFocus
TEST_F(TestBottomBar, checkSlotSetTextEditFocus)
{
    // 仅在此处创建唯一的 Window：源码中 anchors_findbar 为函数内静态变量，
    // 多个 Window 实例会触发其析构冲突，故整个用例集只创建一个 Window 并有意泄漏。
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    BottomBar *bar = pWindow->currentWrapper()->m_pBottomBar;
    ASSERT_NE(bar, nullptr);

    // 触发槽函数，通过真实的 window 发出 pressEsc 信号
    bar->slotSetTextEditFocus();
}

// 测试构造函数 lambda #1: 切换编码 (currentActionChanged on m_pEncodeMenu)
TEST_F(TestBottomBar, checkEncodeMenuLambda)
{
    // 独立的 EditWrapper 作为 BottomBar 的 parent(即 m_pWrapper)，避免依赖 Window
    EditWrapper *wrapper = new EditWrapper;
    BottomBar *bar = wrapper->m_pBottomBar;
    ASSERT_NE(bar, nullptr);

    // getFileLoading 返回 true 触发回退分支
    Stub stub;
    stub.set(ADDR(EditWrapper, getFileLoading), stub_getFileLoading_true);

    QString previousText = bar->m_pEncodeMenu->getCurrentText();
    QAction action("GBK");
    emit bar->m_pEncodeMenu->currentActionChanged(&action);
    // 回退分支：文本应保持为之前的值
    EXPECT_EQ(bar->m_pEncodeMenu->getCurrentText(), previousText);

    delete wrapper;
}

// 测试构造函数 lambda #2: 切换文件类型 (currentActionChanged on m_pHighlightMenu)
TEST_F(TestBottomBar, checkHighlightMenuLambda)
{
    EditWrapper *wrapper = new EditWrapper;
    BottomBar *bar = wrapper->m_pBottomBar;
    ASSERT_NE(bar, nullptr);

    // 避免 reloadFileHighlight 产生外部副作用
    Stub stub;
    stub.set(ADDR(EditWrapper, reloadFileHighlight), stub_reloadFileHighlight);

    QAction action("C++");
    emit bar->m_pHighlightMenu->currentActionChanged(&action);
    // lambda 将高亮菜单文本设置为 action 的文本
    EXPECT_EQ(bar->m_pHighlightMenu->getCurrentText(), QString("C++"));

    delete wrapper;
}

// 视图模式 combobox（§8.2）：setViewMode 同步折叠态文案与选中项
TEST_F(TestBottomBar, checkViewModeMenu_SetViewMode)
{
    auto bottomBar = new BottomBar;
    ASSERT_NE(bottomBar->m_pViewModeMenu, nullptr);

    bottomBar->setViewMode(ViewMode::LivePreview);
    EXPECT_EQ(bottomBar->m_pViewModeMenu->getCurrentText(), bottomBar->m_actLivePreview->text());
    EXPECT_TRUE(bottomBar->m_actLivePreview->isChecked());

    bottomBar->setViewMode(ViewMode::ReadView);
    EXPECT_EQ(bottomBar->m_pViewModeMenu->getCurrentText(), bottomBar->m_actReadView->text());
    EXPECT_TRUE(bottomBar->m_actReadView->isChecked());

    bottomBar->setViewMode(ViewMode::Edit);
    EXPECT_EQ(bottomBar->m_pViewModeMenu->getCurrentText(), bottomBar->m_actEditView->text());
    EXPECT_TRUE(bottomBar->m_actEditView->isChecked());

    bottomBar->deleteLater();
}

// 视图模式 combobox（§8.2）：setMarkdownAvailable(false) 仅置灰「实时预览」
TEST_F(TestBottomBar, checkViewModeMenu_SetMarkdownAvailable)
{
    auto bottomBar = new BottomBar;

    bottomBar->setMarkdownAvailable(false);
    EXPECT_FALSE(bottomBar->m_actLivePreview->isEnabled());
    EXPECT_TRUE(bottomBar->m_actEditView->isEnabled());
    EXPECT_TRUE(bottomBar->m_actReadView->isEnabled());

    bottomBar->setMarkdownAvailable(true);
    EXPECT_TRUE(bottomBar->m_actLivePreview->isEnabled());

    bottomBar->deleteLater();
}

// 视图模式 combobox（§8.2）：点击菜单项上抛 viewModeRequested
TEST_F(TestBottomBar, checkViewModeMenu_ActionTriggerEmitsRequested)
{
    auto bottomBar = new BottomBar;
    QSignalSpy spy(bottomBar, &BottomBar::viewModeRequested);

    bottomBar->m_actReadView->trigger();
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).value<ViewMode>(), ViewMode::ReadView);

    bottomBar->deleteLater();
}
