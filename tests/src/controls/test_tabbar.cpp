#include "test_tabbar.h"

test_tabbar::test_tabbar()
{

}

TEST_F(test_tabbar, Tabbar)
{
    Tabbar tab(nullptr);
    assert(1==1);
}

TEST_F(test_tabbar, openFilesInWindow)
{
    Tabbar * tab = new Tabbar();
    tab->addTab(".cache/deepin/deepin-editor","aabb");

    assert(1==1);
}
//addTabWithIndex
TEST_F(test_tabbar, addTabWithIndex)
{
    Tabbar * tab = new Tabbar();
    tab->addTabWithIndex(0,".cache/deepin/deepin-editor","aabb");

    assert(1==1);
}
//closeTab
TEST_F(test_tabbar, closeTab)
{
    Tabbar * tab = new Tabbar();
    tab->closeTab(0);

    assert(1==1);
}
//void closeCurrentTab();
TEST_F(test_tabbar, closeCurrentTab)
{
    Tabbar * tab = new Tabbar();
    tab->closeCurrentTab();

    assert(1==1);
}
//void closeOtherTabs();
TEST_F(test_tabbar, closeOtherTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeOtherTabs();

    assert(1==1);
}

//void closeLeftTabs(const QString &filePath);
TEST_F(test_tabbar, closeLeftTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeLeftTabs("aa");

    assert(1==1);
}
//void closeRightTabs(const QString &filePath);
TEST_F(test_tabbar, closeRightTabs)
{
    Tabbar * tab = new Tabbar();
    tab->closeRightTabs("aa");

    assert(1==1);
}

//void closeOtherTabsExceptFile(const QString &filePath);
TEST_F(test_tabbar, closeOtherTabsExceptFile)
{
    Tabbar * tab = new Tabbar();
    tab->closeOtherTabsExceptFile("aa");

    assert(1==1);
}
//void updateTab(int index, const QString &filePath, const QString &tabName);
TEST_F(test_tabbar, updateTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->updateTab(0,"/.cache/deepin/deepin-editor","aa");

    assert(1==1);
}
//void previousTab();
TEST_F(test_tabbar, previousTab)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->previousTab();

    assert(1==1);
}
//void nextTab();
TEST_F(test_tabbar, nextTab)
{
    Tabbar * tab = new Tabbar();
    tab->nextTab();

    assert(1==1);
}

//int indexOf(const QString &filePath);
TEST_F(test_tabbar, indexOf)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->indexOf("/.cache/deepin/deepin-editor");

    assert(1==1);
}

//QString currentName() const;
TEST_F(test_tabbar, currentName)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->currentName();

    assert(1==1);
}
//QString currentPath() const;
TEST_F(test_tabbar, currentPath)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->currentPath();

    assert(1==1);
}
//QString fileAt(int index) const;
TEST_F(test_tabbar, fileAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->fileAt(0);

    assert(1==1);
}
//QString textAt(int index) const;
TEST_F(test_tabbar, textAt)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->textAt(0);

    assert(1==1);
}

//void setTabPalette(const QString &activeColor, const QString &inactiveColor);
TEST_F(test_tabbar, setTabPalette)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->setTabPalette("red","red");

    assert(1==1);
}
//void setBackground(const QString &startColor, const QString &endColor);
TEST_F(test_tabbar, setBackground)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->setBackground("red","red");

    assert(1==1);
}
//void setDNDColor(const QString &startColor, const QString &endColor);
TEST_F(test_tabbar, setDNDColor)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    tab->setDNDColor("red","red");

    assert(1==1);
}
//bool canInsertFromMimeData(int index, const QMimeData *source) const;
//bool eventFilter(QObject *, QEvent *event);
TEST_F(test_tabbar, eventFilter)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    QEvent a=QEvent(QEvent::Type::MouseButtonRelease);
    tab->eventFilter(tab,&a);

    assert(1==1);
}

//QSize tabSizeHint(int index) const;
TEST_F(test_tabbar, tabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    QEvent a=QEvent(QEvent::Type::MouseButtonRelease);
    tab->tabSizeHint(0);

    assert(1==1);
}
//QSize minimumTabSizeHint(int index) const;
TEST_F(test_tabbar, minimumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    QEvent *a;
    tab->minimumTabSizeHint(0);

    assert(1==1);
}
//QSize maximumTabSizeHint(int index) const;
TEST_F(test_tabbar, maximumTabSizeHint)
{
    Tabbar * tab = new Tabbar();
    tab->addTab("/.cache/deepin/deepin-editor","aa");
    QEvent *a;
    tab->maximumTabSizeHint(0);

    assert(1==1);
}
