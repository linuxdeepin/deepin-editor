// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_ddropdownmenu.h"
#include "../stub.h"

#include <QKeyEvent>
#include <QFlags>
#include <QSignalSpy>
#include <QColor>
#include <QActionGroup>
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE

// 测试函数 DDropdownMenu::setFontEx
TEST_F(test_ddropdownmenu, setFontEx)
{

    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->setFontEx(QFont());
    EXPECT_EQ(dropMenu->m_font, QFont());

    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();

}

// 测试函数 DDropdownMenu::setCurrentAction
TEST_F(test_ddropdownmenu, setCurrentAction)
{

    // 测试场景1: 非空指针
    DDropdownMenu *dropMenu = new DDropdownMenu();
    QAction *action = new QAction();
    dropMenu->setCurrentAction(action);
    EXPECT_EQ(action->isCheckable(),false);

    EXPECT_NE(action,nullptr);
    EXPECT_NE(dropMenu,nullptr);
    action->deleteLater();
    dropMenu->deleteLater();


    // 测试场景2: 空指针
    dropMenu = new DDropdownMenu();
    dropMenu->setCurrentAction(nullptr);
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();

}

// 测试函数 DDropdownMenu::setCurrentTextOnly
TEST_F(test_ddropdownmenu, setCurrentTextOnly)
{

    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->setCurrentTextOnly("test");
    EXPECT_EQ(dropMenu->m_text,"test");

    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();

}

// 测试函数 DDropdownMenu::setText
TEST_F(test_ddropdownmenu, setText)
{

    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->setText("test");
    EXPECT_EQ(dropMenu->m_text, "test");

    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();


}

// 测试函数 DDropdownMenu::setTheme
TEST_F(test_ddropdownmenu, setTheme)
{

    // 场景1: 正确主题
    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->setTheme("dark");
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();




    // 场景2: 错误主题
    dropMenu = new DDropdownMenu();
    dropMenu->setTheme("error");
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();


}

// 测试函数 DDropdownMenu::setChildrenFocus
TEST_F(test_ddropdownmenu, setChildrenFocus)
{

    // 场景1: true
    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->setChildrenFocus(true);
    QFlags<Qt::FocusPolicy> flags = QFlags<Qt::FocusPolicy>(dropMenu->m_pToolButton->focusPolicy());
    EXPECT_EQ(flags.testFlag(Qt::StrongFocus), true);
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();




    // 场景2:false
    dropMenu = new DDropdownMenu();
    dropMenu->setChildrenFocus(false);
    QFlags<Qt::FocusPolicy> flags2 = QFlags<Qt::FocusPolicy>(dropMenu->m_pToolButton->focusPolicy());
    EXPECT_EQ(flags2.testFlag(Qt::NoFocus), true);
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();


}

// 测试函数 DDropdownMenu::getButton
TEST_F(test_ddropdownmenu, getButton)
{

    DDropdownMenu *dropMenu = new DDropdownMenu();
    DToolButton *button = dropMenu->getButton();
    EXPECT_NE(dropMenu->m_pToolButton, nullptr);
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();


}

bool createEncodeMenu_isEmpty_stub()
{
    return true;
}

// 测试函数 DDropdownMenu::createEncodeMenu
TEST_F(test_ddropdownmenu, createEncodeMenu)
{
    // 场景2: sm_groupEncodeVec为空
    Stub stub;
    typedef QVector<QPair<QString, QStringList> > VecType;
    stub.set(ADDR(VecType, isEmpty), createEncodeMenu_isEmpty_stub);
    DDropdownMenu *dropMenu = DDropdownMenu::createEncodeMenu();
    ASSERT_NE(dropMenu, nullptr);
    EXPECT_TRUE(dropMenu->m_menu->actions().isEmpty());
    dropMenu->deleteLater();

    // 场景1: sm_groupEncodeVec不为空
    stub.reset(ADDR(VecType, isEmpty));
    dropMenu = DDropdownMenu::createEncodeMenu();
    ASSERT_NE(dropMenu, nullptr);
    EXPECT_FALSE(dropMenu->m_menu->actions().isEmpty());
    dropMenu->deleteLater();
}

// 测试函数 DDropdownMenu::createHighLightMenu
TEST_F(test_ddropdownmenu, createHighLightMenu)
{

    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->createHighLightMenu();
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();

}

// 测试函数 DDropdownMenu::createIcon
TEST_F(test_ddropdownmenu, createIcon)
{

    // 场景1: m_bPressed为true
    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->m_bPressed = true;
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();


    // 场景2: m_bPressed为false
    dropMenu = new DDropdownMenu();
    dropMenu->m_bPressed = false;
    dropMenu->createIcon();
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();


}

// 测试函数 DDropdownMenu::OnFontChangedSlot
TEST_F(test_ddropdownmenu, OnFontChangedSlot)
{

    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->OnFontChangedSlot(QFont());
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();

}

// 测试函数 DDropdownMenu::eventFilter
// 测试函数 DDropdownMenu::setSvgColor
TEST_F(test_ddropdownmenu, setSvgColor)
{

    DDropdownMenu *dropMenu = new DDropdownMenu();
    EXPECT_NE(dropMenu->setSvgColor("#FF0000").isNull(),true);
    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();

}

// 测试函数 DDropdownMenu::SetSVGBackColor
TEST_F(test_ddropdownmenu, SetSVGBackColor)
{

    QByteArray data = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"
                      "<text x=\"0\" y=\"15\" fill=\"red\">I love SVG</text>"
                      "</svg>";
    QDomDocument doc;
    doc.setContent(data);
    QDomElement elem = doc.documentElement();
    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->SetSVGBackColor(elem, "fill", "#FF0000");

    EXPECT_NE(dropMenu,nullptr);
    dropMenu->deleteLater();

}

// Stub: 阻塞 exec() 弹窗，直接返回空
static QAction *stub_QMenu_exec_noarg()
{
    return nullptr;
}

// 测试函数 DDropdownMenu::slotRequestMenu
TEST_F(test_ddropdownmenu, slotRequestMenu)
{
    DDropdownMenu *dropMenu = new DDropdownMenu();

    Stub stub;
    stub.set((QAction * (QMenu::*)()) ADDR(QMenu, exec), stub_QMenu_exec_noarg);

    // 场景1: request 为 true
    dropMenu->slotRequestMenu(true);
    // 场景2: request 为 false
    dropMenu->slotRequestMenu(false);

    EXPECT_NE(dropMenu, nullptr);
    dropMenu->deleteLater();
}

// 测试函数 DDropdownMenu::getCurrentText
TEST_F(test_ddropdownmenu, getCurrentText)
{
    DDropdownMenu *dropMenu = new DDropdownMenu();
    dropMenu->setText("GB18030");
    EXPECT_EQ(dropMenu->getCurrentText().toStdString(), "GB18030");

    dropMenu->deleteLater();
}

// 测试 createEncodeMenu 内 lambda #1: DMenu::triggered
TEST_F(test_ddropdownmenu, createEncodeMenuLambda)
{
    DDropdownMenu *encMenu = DDropdownMenu::createEncodeMenu();
    ASSERT_NE(encMenu, nullptr);

    QSignalSpy spy(encMenu, &DDropdownMenu::currentActionChanged);
    QAction *action = new QAction("GBK");
    // 直接触发 DMenu 的 triggered 信号，调用 createEncodeMenu 中注册的 lambda
    emit encMenu->m_menu->triggered(action);
    // m_text("UTF-8") != action->text()，应发出 currentActionChanged
    EXPECT_EQ(spy.count(), 1);

    delete action;
    encMenu->deleteLater();
}

// 测试 createHighLightMenu 内 lambda #1: noHlAction::triggered(bool)
TEST_F(test_ddropdownmenu, createHighLightMenuNoneLambda)
{
    DDropdownMenu *hlMenu = DDropdownMenu::createHighLightMenu();
    ASSERT_NE(hlMenu, nullptr);
    ASSERT_FALSE(hlMenu->m_menu->actions().isEmpty());

    // noHlAction("None") 是 m_pMenu 的第一个 action
    QAction *noneAction = hlMenu->m_menu->actions().first();
    QSignalSpy spy(hlMenu, &DDropdownMenu::currentActionChanged);
    // 直接触发 triggered(true)，调用 createHighLightMenu 中 noHlAction 注册的 lambda
    emit noneAction->triggered(true);
    EXPECT_EQ(spy.count(), 1);

    hlMenu->deleteLater();
}

// 测试 createHighLightMenu 内 lambda #2: QActionGroup::triggered(QAction*)
TEST_F(test_ddropdownmenu, createHighLightMenuActionGroupLambda)
{
    DDropdownMenu *hlMenu = DDropdownMenu::createHighLightMenu();
    ASSERT_NE(hlMenu, nullptr);
    ASSERT_NE(hlMenu->m_actionGroup, nullptr);

    QSignalSpy spy(hlMenu, &DDropdownMenu::currentActionChanged);
    QAction *action = new QAction("");
    // 直接触发 QActionGroup 的 triggered 信号，调用 createHighLightMenu 中 actionGroup 注册的 lambda
    emit hlMenu->m_actionGroup->triggered(action);
    // defName 为空，def 无效，走 else 分支设置文本为 None
    EXPECT_EQ(hlMenu->getCurrentText().toStdString(), "None");

    delete action;
    hlMenu->deleteLater();
}

// 测试构造函数 lambda #1: DGuiApplicationHelper::sizeModeChanged
TEST_F(test_ddropdownmenu, constructorSizeModeLambda)
{
    DDropdownMenu *dropMenu = new DDropdownMenu();
    ASSERT_NE(dropMenu, nullptr);

    // 直接触发 sizeModeChanged 信号，调用构造函数中注册的 lambda
    DGuiApplicationHelper *helper = DGuiApplicationHelper::instance();
    emit helper->sizeModeChanged(DGuiApplicationHelper::CompactMode);

    EXPECT_NE(dropMenu, nullptr);
    dropMenu->deleteLater();
}
