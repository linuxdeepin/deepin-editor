#include "ut_tabbar.h"
#include "QStyleOptionTab"
#include "../src/widgets/window.h"
#include "../src/editor/editwrapper.h"
#include <QMouseEvent>

test_tabbar::test_tabbar()
{

}

TEST_F(test_tabbar, Tabbar)
{
    Tabbar* tab = new Tabbar;
    tab->m_moreWaysCloseMenu = new QMenu(tab);
    tab->m_rightMenu = new QMenu(tab);

    EXPECT_NE(tab,nullptr);
    EXPECT_NE(tab->m_moreWaysCloseMenu,nullptr);
    EXPECT_NE(tab->m_rightMenu,nullptr);

    tab->deleteLater();

    
}

TEST_F(test_tabbar, openFilesInWindow)
{
    Tabbar * tab = new Tabbar();
    tab->addTab(".cache/deepin/deepin-editor","aabb");

    EXPECT_NE(tab,nullptr);
    EXPECT_EQ(tab->currentIndex(),0);


    tab->deleteLater();
    
}
//addTabWithIndex
TEST_F(test_tabbar, addTabWithIndex)
{
    Tabbar * tab = new Tabbar();
    tab->addTabWithIndex(0,".cache/deepin/deepin-editor","aabb");

    EXPECT_NE(tab,nullptr);
    EXPECT_NE(tab->tabToolTip(0),"aabb");

    tab->deleteLater();
    
}
//closeTab
TEST_F(test_tabbar, closeTab)
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
TEST_F(test_tabbar, closeCurrentTab)
{
    Tabbar * tab = new Tabbar();
    tab->closeCurrentTab();

    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}
//void closeOtherTabs();
TEST_F(test_tabbar, closeOtherTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeOtherTabs();

    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);


    tab->deleteLater();
    
}

//void closeLeftTabs(const QString &filePath);
TEST_F(test_tabbar, closeLeftTabs)
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
TEST_F(test_tabbar, closeRightTabs)
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
TEST_F(test_tabbar, closeOtherTabsExceptFile)
{
    Tabbar * tab = new Tabbar();
    tab->m_tabPaths.push_back("bb");
    tab->closeOtherTabsExceptFile("aa");

    EXPECT_EQ(tab->count(),0);
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}
//void updateTab(int index, const QString &filePath, const QString &tabName);
TEST_F(test_tabbar, updateTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    tab->updateTab(0,"/.cache/deepin/deepin-editor","aa");

    EXPECT_NE(tab,nullptr);
    tab->deleteLater();
    
}
//void previousTab();
TEST_F(test_tabbar, previousTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);
    tab->previousTab();

    tab->addTab("/.cache/deepin/deepin-editor","bb");
    tab->previousTab();

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}
//void nextTab();
TEST_F(test_tabbar, nextTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    tab->nextTab();

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}

//int indexOf(const QString &filePath);
TEST_F(test_tabbar, indexOf)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);
    EXPECT_NE(tab->indexOf("/.cache/deepin/deepin-editor"),2);

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}

//QString currentName() const;
TEST_F(test_tabbar, currentName)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_NE(tab->currentName(),"");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}
//QString currentPath() const;
TEST_F(test_tabbar, currentPath)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_NE(tab->currentPath(),"");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}
//QString fileAt(int index) const;
TEST_F(test_tabbar, fileAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_NE(tab->fileAt(0),"");
    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}
//QString textAt(int index) const;
TEST_F(test_tabbar, textAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);
    EXPECT_NE(tab->textAt(0),"");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}

//void setTabPalette(const QString &activeColor, const QString &inactiveColor);
TEST_F(test_tabbar, setTabPalette)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);
    tab->setTabPalette("red","red");

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}
//void setBackground(const QString &startColor, const QString &endColor);
TEST_F(test_tabbar, setBackground)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    tab->setBackground("red","red");

    EXPECT_EQ(tab->m_backgroundStartColor,"red");

    tab->deleteLater();
    
}
//void setDNDColor(const QString &startColor, const QString &endColor);
TEST_F(test_tabbar, setDNDColor)
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
TEST_F(test_tabbar, eventFilter)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    QMouseEvent *e = new QMouseEvent(QEvent::MouseButtonPress,QPointF(76,29),Qt::RightButton,Qt::RightButton,Qt::NoModifier);

    //eventFilter: m_rightMenu->exec(mapToGlobal(position));会导致运行停止
    //tab->eventFilter(tab,e);

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

    
}

//QSize minimumTabSizeHint(int index) const;
TEST_F(test_tabbar, minimumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    EXPECT_EQ(tab->m_tabPaths.contains("/.cache/deepin/deepin-editor"),true);

    EXPECT_EQ(tab->minimumTabSizeHint(0),QSize(110, 40));


    EXPECT_NE(tab,nullptr);

    tab->deleteLater();

    
}
//QSize maximumTabSizeHint(int index) const;
TEST_F(test_tabbar, maximumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");

    EXPECT_EQ(tab->maximumTabSizeHint(0),QSize(160, 40));

    EXPECT_NE(tab,nullptr);

    tab->deleteLater();
    
}

TEST_F(test_tabbar, createDragPixmapFromTab)
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


TEST_F(test_tabbar, createMimeDataFromTab)
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

TEST_F(test_tabbar, insertFromMimeDataOnDragEnter)
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

TEST_F(test_tabbar, insertFromMimeData)
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


TEST_F(test_tabbar, canInsertFromMimeData)
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


TEST_F(test_tabbar, handleDragActionChanged)
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

TEST_F(test_tabbar, mousePressEvent)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    QMouseEvent* event = new QMouseEvent(QEvent::None,QPoint(),Qt::MidButton,Qt::MidButton,Qt::NoModifier);


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

TEST_F(test_tabbar, dropEvent)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("dedit/tabbar","test");
    QDropEvent* event = new QDropEvent(QPointF(100,100),Qt::CopyAction,mimeData,Qt::LeftButton,Qt::NoModifier);


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

TEST_F(test_tabbar, tabSizeHint)
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

TEST_F(test_tabbar, handleTabMoved)
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

TEST_F(test_tabbar, handleTabReleased)
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

TEST_F(test_tabbar, handleTabIsRemoved)
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

TEST_F(test_tabbar, handleTabDroped)
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

TEST_F(test_tabbar, onTabDrapStart)
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

TEST_F(test_tabbar, resizeEvent)
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


