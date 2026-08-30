// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_tabbar.h"
#include "QStyleOptionTab"
#include "../src/widgets/window.h"
#include "../src/editor/editwrapper.h"
#include <QMouseEvent>
#include <QAction>
#include <DMenu>
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#include "src/stub.h"


namespace tabbarstub {

int retintstub()
{
    return 1;
}

}

using namespace tabbarstub;

UT_Tabbar::UT_Tabbar()
{

}

TEST(UT_Tabbar_Tabbar, UT_Tabbar_Tabbar)
{
    Tabbar* tab = new Tabbar;
    tab->m_moreWaysCloseMenu = new QMenu(tab);
    tab->m_rightMenu = new QMenu(tab);

    EXPECT_NE(tab,nullptr);
    EXPECT_NE(tab->m_moreWaysCloseMenu,nullptr);
    EXPECT_NE(tab->m_rightMenu,nullptr);

    tab->deleteLater();


}

//addTabWithIndex
//closeTab
TEST(UT_Tabbar_closeTab, UT_Tabbar_closeTab)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->closeTab(index);
    window->getTabbar()->closeTab(-1);


    EXPECT_EQ(window->getTabbar()->count(),0);
    EXPECT_NE(window->getTabbar(),nullptr);
    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();

}
//void closeCurrentTab();
TEST(UT_Tabbar_closeCurrentTab, UT_Tabbar_closeCurrentTab)
{
    Tabbar * tab = new Tabbar();
    tab->closeCurrentTab();

    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}
//void closeOtherTabs();
TEST(UT_Tabbar_closeOtherTabs, UT_Tabbar_closeOtherTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeOtherTabs();

    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);


    tab->deleteLater();

}

//void closeLeftTabs(const QString &filePath);
TEST(UT_Tabbar_closeLeftTabs, UT_Tabbar_closeLeftTabs)
{
    Tabbar * tab = new Tabbar();
    tab->m_tabPaths.push_back("aa");
    tab->closeLeftTabs("aa");
    tab->closeLeftTabs("bb");


    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}
//void closeRightTabs(const QString &filePath);
TEST(UT_Tabbar_closeRightTabs, UT_Tabbar_closeRightTabs)
{
    Tabbar * tab = new Tabbar();
    tab->m_tabPaths.push_back("aa");
    tab->closeRightTabs("aa");
    tab->closeLeftTabs("bb");

    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}

//void closeOtherTabsExceptFile(const QString &filePath);
TEST(UT_Tabbar_closeOtherTabsExceptFile, UT_Tabbar_closeOtherTabsExceptFile)
{
    Tabbar * tab = new Tabbar();
    tab->m_tabPaths.push_back("bb");
    tab->closeOtherTabsExceptFile("aa");

    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}

//void updateTab(int index, const QString &filePath, const QString &tabName);
//void previousTab();
//void nextTab();
TEST(UT_Tabbar_nextTab, UT_Tabbar_nextTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    tab->nextTab();

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}

//int indexOf(const QString &filePath);
TEST(UT_Tabbar_indexOf, UT_Tabbar_indexOf)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);
    EXPECT_NE(tab->indexOf("/.cache/deepin/deepin-editor"),2);

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}

//QString currentName() const;
TEST(UT_Tabbar_currentName, UT_Tabbar_currentName)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_NE(tab->currentName(),"");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}
//QString currentPath() const;
TEST(UT_Tabbar_currentPath, UT_Tabbar_currentPath)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_NE(tab->currentPath(),"");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}
//QString fileAt(int index) const;
TEST(UT_Tabbar_fileAt, UT_Tabbar_fileAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_NE(tab->fileAt(0),"");
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}
//QString textAt(int index) const;
TEST(UT_Tabbar_textAt, UT_Tabbar_textAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);
    EXPECT_NE(tab->textAt(0),"");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}

//void setTabPalette(const QString &activeColor, const QString &inactiveColor);
TEST(UT_Tabbar_setTabPalette, UT_Tabbar_setTabPalette)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);
    tab->setTabPalette("red","red");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}
//void setBackground(const QString &startColor, const QString &endColor);
TEST(UT_Tabbar_setBackground, UT_Tabbar_setBackground)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    tab->setBackground("red","red");

    EXPECT_EQ(tab->m_backgroundStartColor,"red");

    tab->deleteLater();

}
//void setDNDColor(const QString &startColor, const QString &endColor);
TEST(UT_Tabbar_setDNDColor, UT_Tabbar_setDNDColor)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    tab->setDNDColor("red","red");

    EXPECT_EQ(tab->m_dndStartColor,"red");
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}
//bool canInsertFromMimeData(int index, const QMimeData *source) const;
//bool eventFilter(QObject *, QEvent *event);
//QSize minimumTabSizeHint(int index) const;
TEST(UT_Tabbar_minimumTabSizeHint, UT_Tabbar_minimumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_EQ(tab->minimumTabSizeHint(0),QSize(110, 40));


    EXPECT_NE(tab,nullptr);

    tab->deleteLater();


}
//QSize maximumTabSizeHint(int index) const;
TEST(UT_Tabbar_maximumTabSizeHint, UT_Tabbar_maximumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");

    EXPECT_EQ(tab->maximumTabSizeHint(0),QSize(200, 40));

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

}

TEST(UT_Tabbar_createDragPixmapFromTab, UT_Tabbar_createDragPixmapFromTab)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->createDragPixmapFromTab(index,option,&p);


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}


TEST(UT_Tabbar_createMimeDataFromTab, UT_Tabbar_createMimeDataFromTab)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->createMimeDataFromTab(index,option);



    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);


    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_insertFromMimeDataOnDragEnter, UT_Tabbar_insertFromMimeDataOnDragEnter)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("dedit/tabbar","test");

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->insertFromMimeDataOnDragEnter(index,mimeData);
    EXPECT_EQ(wrapper->getFileLoading(),false);

    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);
    EXPECT_NE(mimeData,nullptr);

    delete mimeData;mimeData = nullptr;
    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_insertFromMimeData, UT_Tabbar_insertFromMimeData)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("dedit/tabbar","test");

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->insertFromMimeData(index,mimeData);
    EXPECT_NE(window->getTabbar()->count(),0);



    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);
    EXPECT_NE(mimeData,nullptr);

    delete mimeData;mimeData = nullptr;
    window->deleteLater();
    wrapper->deleteLater();


}


TEST(UT_Tabbar_canInsertFromMimeData, UT_Tabbar_canInsertFromMimeData)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("dedit/tabbar","test");

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    EXPECT_EQ(window->getTabbar()->canInsertFromMimeData(index,mimeData),true);



    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);
    EXPECT_NE(mimeData,nullptr);


    delete mimeData;mimeData = nullptr;
    window->deleteLater();
    wrapper->deleteLater();


}


TEST(UT_Tabbar_handleDragActionChanged, UT_Tabbar_handleDragActionChanged)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    Qt::DropAction actions[2] = {Qt::IgnoreAction,Qt::MoveAction};

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleDragActionChanged(actions[0]);
    window->getTabbar()->handleDragActionChanged(actions[1]);


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_mousePressEvent, UT_Tabbar_mousePressEvent)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QMouseEvent* event = new QMouseEvent(QEvent::None, QPoint(), Qt::MidButton, Qt::MidButton, Qt::NoModifier);
#else
    QMouseEvent* event = new QMouseEvent(QEvent::None, QPoint(), QPoint(), Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
#endif


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->mousePressEvent(event);


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    delete event;event = nullptr;
    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_dropEvent, UT_Tabbar_dropEvent)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("dedit/tabbar","test");
    QDropEvent* event = new QDropEvent(QPointF(100,100),Qt::CopyAction,mimeData,Qt::LeftButton,Qt::NoModifier, QEvent::Drop);


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->dropEvent(event);


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);
    EXPECT_NE(event,nullptr);
    EXPECT_NE(mimeData,nullptr);

    delete mimeData;mimeData = nullptr;
    delete event;event = nullptr;
    window->deleteLater();
    wrapper->deleteLater();



}

TEST(UT_Tabbar_tabSizeHint, UT_Tabbar_tabSizeHint)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    EXPECT_NE(window->getTabbar()->tabSizeHint(index),QSize(1,1));




    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_handleTabMoved, UT_Tabbar_handleTabMoved)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleTabMoved(index,index);

    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_handleTabReleased, UT_Tabbar_handleTabReleased)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->m_listOldTabPath.push_back("/.cache/deepin/deepin-editor");
    EXPECT_EQ(window->getTabbar()->m_listOldTabPath.contains("/.cache/deepin/deepin-editor"),true);
    window->getTabbar()->handleTabReleased(index);


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_handleTabIsRemoved, UT_Tabbar_handleTabIsRemoved)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleTabIsRemoved(index);


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_handleTabDroped, UT_Tabbar_handleTabDroped)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    Qt::DropAction actions[2] = {Qt::IgnoreAction,Qt::MoveAction};


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleTabDroped(index,actions[0],nullptr);
    window->getTabbar()->handleTabDroped(index,actions[0],window->getTabbar());


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_onTabDrapStart, UT_Tabbar_onTabDrapStart)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    Qt::DropAction actions[2] = {Qt::IgnoreAction,Qt::MoveAction};


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);


    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->onTabDrapStart();
    EXPECT_EQ(window->isVisible(),true);

    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);

    window->deleteLater();
    wrapper->deleteLater();


}

TEST(UT_Tabbar_resizeEvent, UT_Tabbar_resizeEvent)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    Qt::DropAction actions[2] = {Qt::IgnoreAction,Qt::MoveAction};
    QResizeEvent* e = new QResizeEvent(QSize(),QSize());


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EXPECT_EQ(window->m_tabbar->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->resizeEvent(e);


    EXPECT_NE(window,nullptr);
    EXPECT_NE(wrapper,nullptr);
    EXPECT_NE(e,nullptr);



    delete e;  e = nullptr;
    window->deleteLater();
    wrapper->deleteLater();


}

// ===================== Appended tests for uncovered functions =====================

namespace {
QAction *stub_QMenu_exec_null(const QPoint &, QAction *)
{
    return nullptr;
}

int stub_DTabBar_tabAt_zero(const QPoint &)
{
    return 0;
}
}

// void previousTab();
TEST(UT_Tabbar_previousTab_Append, UT_Tabbar_previousTab)
{
    Tabbar *tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor", "aa");
    tab->addTab("/.cache/deepin/deepin-editor2", "bb");
    EXPECT_EQ(tab->count(), 2);
    EXPECT_EQ(tab->currentIndex(), 1);

    // currentIndex > 0, decrement to 0
    tab->previousTab();
    EXPECT_EQ(tab->currentIndex(), 0);

    // currentIndex <= 0, wrap around to count - 1
    tab->previousTab();
    EXPECT_EQ(tab->currentIndex(), 1);

    EXPECT_NE(tab, nullptr);
    tab->deleteLater();
}

// void showTabs();
TEST(UT_Tabbar_showTabs_Append, UT_Tabbar_showTabs)
{
    Tabbar *tab = new Tabbar();
    tab->m_closeLeftTabAction = new QAction(tab);
    tab->m_closeRightTabAction = new QAction(tab);
    tab->addTab("/.cache/deepin/deepin-editor", "aa");

    EXPECT_NO_FATAL_FAILURE(tab->showTabs());
    // currentIndex 0 <= 0, left action disabled
    EXPECT_EQ(tab->m_closeLeftTabAction->isEnabled(), false);
    // currentIndex 0 >= count - 1, right action disabled
    EXPECT_EQ(tab->m_closeRightTabAction->isEnabled(), false);

    EXPECT_NE(tab, nullptr);
    tab->deleteLater();
}

// bool eventFilter(QObject *, QEvent *event) - triggers all 5 action lambdas
TEST(UT_Tabbar_eventFilter_Append, UT_Tabbar_eventFilter)
{
    Tabbar *tab = new Tabbar();

    Stub s;
    s.set(ADDR(DTabBar, tabAt), stub_DTabBar_tabAt_zero);
    s.set(static_cast<QAction *(QMenu::*)(const QPoint &, QAction *)>(&QMenu::exec), stub_QMenu_exec_null);

    QMouseEvent *e = new QMouseEvent(QEvent::MouseButtonPress, QPointF(5, 5),
                                     QPointF(5, 5), QPointF(5, 5),
                                     Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    // Right button press on a tab -> builds menu and connects 5 lambdas, calls tr()
    EXPECT_EQ(tab->eventFilter(tab, e), true);

    ASSERT_NE(tab->m_closeTabAction, nullptr);
    ASSERT_NE(tab->m_closeOtherTabAction, nullptr);
    ASSERT_NE(tab->m_closeLeftTabAction, nullptr);
    ASSERT_NE(tab->m_closeRightTabAction, nullptr);
    ASSERT_NE(tab->m_closeAllunModifiedTabAction, nullptr);

    // Trigger each connected lambda
    EXPECT_NO_FATAL_FAILURE(tab->m_closeTabAction->trigger());
    EXPECT_NO_FATAL_FAILURE(tab->m_closeOtherTabAction->trigger());
    EXPECT_NO_FATAL_FAILURE(tab->m_closeLeftTabAction->trigger());
    EXPECT_NO_FATAL_FAILURE(tab->m_closeRightTabAction->trigger());
    EXPECT_NO_FATAL_FAILURE(tab->m_closeAllunModifiedTabAction->trigger());

    s.reset(ADDR(DTabBar, tabAt));
    s.reset(static_cast<QAction *(QMenu::*)(const QPoint &, QAction *)>(&QMenu::exec));

    EXPECT_NE(tab, nullptr);
    delete e;
    tab->deleteLater();
}

// Tabbar constructor lambda connected to DGuiApplicationHelper::sizeModeChanged
TEST(UT_Tabbar_ConstructorSizeModeLambda, UT_Tabbar_ConstructorSizeModeLambda)
{
    Tabbar *tab = new Tabbar();
    auto helper = DGuiApplicationHelper::instance();
    auto origMode = helper->sizeMode();

    EXPECT_NO_FATAL_FAILURE(helper->setSizeMode(origMode == DGuiApplicationHelper::NormalMode
                                                    ? DGuiApplicationHelper::CompactMode
                                                    : DGuiApplicationHelper::NormalMode));
    helper->setSizeMode(origMode); // restore

    EXPECT_NE(tab, nullptr);
    tab->deleteLater();
}

// Tabbar::tr - Q_OBJECT generated translator, exercised explicitly
TEST(UT_Tabbar_tr, UT_Tabbar_tr)
{
    Tabbar *tab = new Tabbar();
    EXPECT_FALSE(tab->tr("Close tab").isEmpty());
    EXPECT_FALSE(tab->tr("Close other tabs").isEmpty());
    tab->deleteLater();
}
