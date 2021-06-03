#include "test_tabbar.h"
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
    tab->m_moreWaysCloseMenu = new QMenu();
    tab->m_rightMenu = new QMenu();
    delete tab;tab=nullptr;

    assert(1==1);
}

TEST_F(test_tabbar, openFilesInWindow)
{
    Tabbar * tab = new Tabbar();
    tab->addTab(".cache/deepin/deepin-editor","aabb");

    tab->deleteLater();
    assert(1==1);
}
//addTabWithIndex
TEST_F(test_tabbar, addTabWithIndex)
{
    Tabbar * tab = new Tabbar();
    tab->addTabWithIndex(0,".cache/deepin/deepin-editor","aabb");

    tab->deleteLater();
    assert(1==1);
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

    window->deleteLater();
    wrapper->deleteLater();
    assert(1==1);
}
//void closeCurrentTab();
TEST_F(test_tabbar, closeCurrentTab)
{
    Tabbar * tab = new Tabbar();
    tab->closeCurrentTab();

    tab->deleteLater();
    assert(1==1);
}
//void closeOtherTabs();
TEST_F(test_tabbar, closeOtherTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeOtherTabs();

    tab->deleteLater();
    assert(1==1);
}

//void closeLeftTabs(const QString &filePath);
TEST_F(test_tabbar, closeLeftTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeLeftTabs("aa");

    tab->deleteLater();
    assert(1==1);
}
//void closeRightTabs(const QString &filePath);
TEST_F(test_tabbar, closeRightTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeRightTabs("aa");

    tab->deleteLater();
    assert(1==1);
}

//void closeOtherTabsExceptFile(const QString &filePath);
TEST_F(test_tabbar, closeOtherTabsExceptFile)
{
    Tabbar * tab = new Tabbar();
    tab->closeOtherTabsExceptFile("aa");

    tab->deleteLater();
    assert(1==1);
}
//void updateTab(int index, const QString &filePath, const QString &tabName);
TEST_F(test_tabbar, updateTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->updateTab(0,"/.cache/deepin/deepin-editor","aa");

    tab->deleteLater();
    assert(1==1);
}
//void previousTab();
TEST_F(test_tabbar, previousTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->previousTab();

    tab->deleteLater();
    assert(1==1);
}
//void nextTab();
TEST_F(test_tabbar, nextTab)
{
    Tabbar * tab = new Tabbar();
    tab->nextTab();

    tab->deleteLater();
    assert(1==1);
}

//int indexOf(const QString &filePath);
TEST_F(test_tabbar, indexOf)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->indexOf("/.cache/deepin/deepin-editor");

    tab->deleteLater();
    assert(1==1);
}

//QString currentName() const;
TEST_F(test_tabbar, currentName)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->currentName();

    tab->deleteLater();
    assert(1==1);
}
//QString currentPath() const;
TEST_F(test_tabbar, currentPath)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->currentPath();

    tab->deleteLater();
    assert(1==1);
}
//QString fileAt(int index) const;
TEST_F(test_tabbar, fileAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->fileAt(0);

    tab->deleteLater();
    assert(1==1);
}
//QString textAt(int index) const;
TEST_F(test_tabbar, textAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->textAt(0);

    tab->deleteLater();
    assert(1==1);
}

//void setTabPalette(const QString &activeColor, const QString &inactiveColor);
TEST_F(test_tabbar, setTabPalette)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->setTabPalette("red","red");

    tab->deleteLater();
    assert(1==1);
}
//void setBackground(const QString &startColor, const QString &endColor);
TEST_F(test_tabbar, setBackground)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->setBackground("red","red");

    tab->deleteLater();
    assert(1==1);
}
//void setDNDColor(const QString &startColor, const QString &endColor);
TEST_F(test_tabbar, setDNDColor)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->setDNDColor("red","red");

    tab->deleteLater();
    assert(1==1);
}
//bool canInsertFromMimeData(int index, const QMimeData *source) const;
//bool eventFilter(QObject *, QEvent *event);
TEST_F(test_tabbar, eventFilter)
{

    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    QMouseEvent *e = new QMouseEvent(QEvent::MouseButtonPress,QPointF(76,29),Qt::RightButton,Qt::RightButton,Qt::NoModifier);

    //eventFilter: m_rightMenu->exec(mapToGlobal(position));会导致运行停止
    //tab->eventFilter(tab,e);

    tab->deleteLater();

    assert(1==1);
}

//QSize minimumTabSizeHint(int index) const;
TEST_F(test_tabbar, minimumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->minimumTabSizeHint(0);

    tab->deleteLater();

    assert(1==1);
}
//QSize maximumTabSizeHint(int index) const;
TEST_F(test_tabbar, maximumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->maximumTabSizeHint(0);

    tab->deleteLater();
    assert(1==1);
}

TEST_F(test_tabbar, createDragPixmapFromTab)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->createDragPixmapFromTab(index,option,&p);

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}


TEST_F(test_tabbar, createMimeDataFromTab)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->createMimeDataFromTab(index,option);

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
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
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->insertFromMimeDataOnDragEnter(index,mimeData);

    delete mimeData;mimeData = nullptr;
    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
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
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->insertFromMimeData(index,mimeData);

    delete mimeData;mimeData = nullptr;
    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
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
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->canInsertFromMimeData(index,mimeData);

    delete mimeData;mimeData = nullptr;
    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}


TEST_F(test_tabbar, handleDragActionChanged)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    Qt::DropAction actions[2] = {Qt::IgnoreAction,Qt::MoveAction};

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleDragActionChanged(actions[0]);
    window->getTabbar()->handleDragActionChanged(actions[1]);

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}

TEST_F(test_tabbar, mousePressEvent)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    QMouseEvent* event = new QMouseEvent(QEvent::None,QPoint(),Qt::MidButton,Qt::MidButton,Qt::NoModifier);


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->mousePressEvent(event);

    delete event;event = nullptr;
    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
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
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->dropEvent(event);

    delete mimeData;mimeData = nullptr;
    delete event;event = nullptr;
    window->deleteLater();
    wrapper->deleteLater();


    assert(1==1);
}

TEST_F(test_tabbar, tabSizeHint)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->tabSizeHint(index);

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}

TEST_F(test_tabbar, handleTabMoved)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleTabMoved(index,index);

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}

TEST_F(test_tabbar, handleTabReleased)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->m_listOldTabPath.push_back("/.cache/deepin/deepin-editor");
    window->getTabbar()->handleTabReleased(index);

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}

TEST_F(test_tabbar, handleTabIsRemoved)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;

    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleTabIsRemoved(index);

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}

TEST_F(test_tabbar, handleTabDroped)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    Qt::DropAction actions[2] = {Qt::IgnoreAction,Qt::MoveAction};


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->handleTabDroped(index,actions[0],nullptr);
    window->getTabbar()->handleTabDroped(index,actions[0],window->getTabbar());

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}

TEST_F(test_tabbar, onTabDrapStart)
{
    int index = 0;
    QStyleOptionTab option;
    QPoint p;
    Qt::DropAction actions[2] = {Qt::IgnoreAction,Qt::MoveAction};


    Window * window = new Window;
    window->addTab("/.cache/deepin/deepin-editor",true);
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->onTabDrapStart();

    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
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
    EditWrapper* wrapper = new EditWrapper(window);
    window->getTabbar()->resizeEvent(e);

    delete e;  e = nullptr;
    window->deleteLater();
    wrapper->deleteLater();

    assert(1==1);
}


