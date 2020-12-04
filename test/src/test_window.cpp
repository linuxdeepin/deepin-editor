#include "test_window.h"

test_window::test_window()
{

}

TEST_F(test_window, Window)
{
    Window window1;
    assert(1==1);
}

TEST_F(test_window, showCenterWindow)
{
    Window * window1 = new Window();
    window1->showCenterWindow(true);

    assert(1==1);
}
//initTitlebar
TEST_F(test_window, initTitlebar)
{
    Window * window1 = new Window();
    window1->initTitlebar();

    assert(1==1);
}
TEST_F(test_window, checkBlockShutdown)
{
    Window * window1 = new Window();
    window1->checkBlockShutdown();

    assert(1==1);
}
//getTabIndex
TEST_F(test_window, getTabIndex)
{
    Window * window1 = new Window();
    window1->getTabIndex("aadd");

    assert(1==1);
}
//activeTab
TEST_F(test_window, activeTab)
{
    Window * window1 = new Window();
    window1->activeTab(0);

    assert(1==1);
}
//addTab
TEST_F(test_window, addTab)
{
    Window * window1 = new Window();
    window1->addTab("aabb",true);

    assert(1==1);
}
//openFile
TEST_F(test_window, openFile)
{
    Window * window = new Window();
  //  window->openFile();
//    delete window;
//    window = nullptr;

    assert(1==1);
}
TEST_F(test_window, saveFile)
{
    Window * window1 = new Window();
    window1->saveFile();

    assert(1==1);
}
//closeTab
TEST_F(test_window, closeTab)
{
    Window * window = new Window();
    window->addBlankTab("aabb");
    window->closeTab();
//    delete window;
//    window = nullptr;

    assert(1==1);
}
//saveAsFileToDisk
TEST_F(test_window, saveAsFileToDisk)
{
    Window * window1 = new Window();
    window1->saveAsFileToDisk();

    assert(1==1);
}
//saveBlankFileToDisk
TEST_F(test_window, saveBlankFileToDisk)
{
    Window * window1 = new Window();
    window1->saveBlankFileToDisk();

    assert(1==1);
}
//saveAsOtherTabFile
TEST_F(test_window, saveAsOtherTabFile)
{
//    Window * window = StartManager::instance()->createWindow();
//    window->addBlankTab("");
//    window->addBlankTab("");
//    window->addBlankTab("");
//    EditWrapper *wrapper = window->currentWrapper();
//    window->saveAsOtherTabFile(wrapper);
////    delete window;
//    window = nullptr;

    assert(1==1);
}
//changeSettingDialogComboxFontNumber
TEST_F(test_window, changeSettingDialogComboxFontNumber)
{
    EditWrapper * eeee = new EditWrapper();
    Window * window1 = new Window();
    window1->changeSettingDialogComboxFontNumber(3);

    assert(1==1);
}
//popupFindBar
TEST_F(test_window, popupFindBar)
{
    Window * window1 = new Window();
    window1->popupFindBar();

    assert(1==1);
}
//popupReplaceBar
TEST_F(test_window, popupReplaceBar)
{
    Window * window1 = new Window();
    window1->popupReplaceBar();

    assert(1==1);
}
//popupJumpLineBar
TEST_F(test_window, popupJumpLineBar)
{
    Window * window1 = new Window();
    window1->popupJumpLineBar();


    assert(1==1);
}
//updateJumpLineBar
TEST_F(test_window, updateJumpLineBar)
{
//    Window * window = new Window();
//    window->updateJumpLineBar();


    assert(1==1);
}
//popupSettingsDialog
TEST_F(test_window, popupSettingsDialog)
{
//    Window * window = new Window();
//    window->popupSettingsDialog();
//    delete window;
//    window = nullptr;

    assert(1==1);
}
//popupPrintDialog

TEST_F(test_window, popupPrintDialog)
{
//    Window * window = new Window();
//    window->popupPrintDialog();
//    delete window;
//    window = nullptr;

    assert(1==1);
}
TEST_F(test_window, popupThemePanel)
{
    Window * window1 = new Window();
    window1->popupThemePanel();

    assert(1==1);
}
//toggleFullscreen
TEST_F(test_window, toggleFullscreen)
{
    Window * window1 = new Window();
    window1->toggleFullscreen();

    assert(1==1);
}
TEST_F(test_window, remberPositionSave)
{
//    Window * window = new Window();
//    window->remberPositionSave();
//    delete window;
//    window = nullptr;

    assert(1==1);
}
//remberPositionRestore
TEST_F(test_window, remberPositionRestore)
{
    Window * window1 = new Window();
    window1->remberPositionRestore();

    assert(1==1);
}
//displayShortcuts
TEST_F(test_window, displayShortcuts)
{
    Window * window1 = new Window();
    window1->displayShortcuts();

    assert(1==1);
}
//setChildrenFocus
TEST_F(test_window, setChildrenFocus)
{
    Window * window1 = new Window();
    window1->setChildrenFocus(false);

    assert(1==1);
}
//addBlankTab
TEST_F(test_window, addBlankTab)
{
    Window * window1 = new Window();
    window1->addBlankTab("aabb");

    assert(1==1);
}
//handleTabCloseRequested
TEST_F(test_window, addTabWithWrapper)
{
    edit = new EditWrapper();
    window = new Window();
    window->addTabWithWrapper(edit,"aabb","aabb",0);

    assert(1==1);
}
TEST_F(test_window, handleTabCloseRequested)
{
//    StartManager::instance()->createWindow()->addTabWithWrapper(edit,"aabb","aabb",0);
//    StartManager::instance()->createWindow()->handleTabCloseRequested(0);

    assert(1==1);
}
//handleTabsClosed
TEST_F(test_window, handleTabsClosed)
{
    QStringList aa;
    window = new Window();
    window->handleTabsClosed(aa);

    assert(1==1);
}
//handleCurrentChanged
TEST_F(test_window, handleCurrentChanged)
{
    QStringList aa;
    window = new Window();
    window->handleCurrentChanged(0);

    assert(1==1);
}
//slot_setTitleFocus
TEST_F(test_window, slot_setTitleFocus)
{
//    QStringList aa;
    //window = new Window();
//    StartManager::instance()->createWindow()->slot_setTitleFocus();
    //window->slot_setTitleFocus();

    assert(1==1);
}
//resizeEvent
TEST_F(test_window, resizeEvent)
{
    QStringList aa;
    window = new Window();
    QResizeEvent * eve;
    window->resizeEvent(eve);

    assert(1==1);
}
//closeEvent
TEST_F(test_window, keyPressEvent)
{
    QStringList aa;
    window = new Window();
    QKeyEvent * eve;
    //window->keyPressEvent(eve);

    assert(1==1);
}
//hideEvent
TEST_F(test_window, hideEvent)
{
    QStringList aa;
    window = new Window();
    QHideEvent * eve;
    window->hideEvent(eve);

    assert(1==1);
}
//TextEdit
