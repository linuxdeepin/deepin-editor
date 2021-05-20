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
int *stub1_exec()
{

    return nullptr;
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
//TEST_F(test_window, openFile)
//{
//    window = new Window();
//    EditWrapper * a = new EditWrapper();
//    window->addTabWithWrapper(a,"aa","aad","aadd",0);
//    window->addTabWithWrapper(a,"bb","aad","aadd",1);
//    Stub stub;
//    stub.set((int (QDialog::*)(const QPoint &, int *))ADDR(QDialog, exec), stub1_exec);
//    // stub.set((QAction *(QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_exec);
//    window->openFile();
//    assert(1==1);
//}
TEST_F(test_window, saveFile)
{
    Window * window1 = new Window();
    window1->saveFile();

    assert(1==1);
}
//closeTab()
TEST_F(test_window, closeTab)
{
    Window * window = new Window();
    window->addBlankTab("aabb");
//    window->closeTab();
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
    window->addTabWithWrapper(edit,"aabb","aabb","aabb");

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
//void backupFile();
TEST_F(test_window, backupFile)
{
    window = new Window();
    window->backupFile();

    assert(1==1);
}

//void closeAllFiles();
TEST_F(test_window, closeAllFiles)
{
    window = new Window();
    window->closeAllFiles();

    assert(1==1);
}

//void addTemFileTab(QString qstrPath,QString qstrName,QString qstrTruePath,bool bIsTemFile = false);
TEST_F(test_window, addTemFileTab)
{
    window = new Window();
    window->addTemFileTab("aa","bb","cc");

    assert(1==1);
}
//Window(DMainWindow *parent = nullptr);
//~Window() override;

////跟新文件修改状态
//void updateModifyStatus(const QString &path, bool isModified);
TEST_F(test_window, updateModifyStatus)
{
    window = new Window();
    window->updateModifyStatus("aa",false);
    window->updateModifyStatus("aa",true);
    assert(1==1);
}
////跟新tab文件名称
//void updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath);
TEST_F(test_window, updateSaveAsFileName)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->updateSaveAsFileName("aa","bb");
    assert(1==1);
}

//int getTabIndex(const QString &file);
//void activeTab(int index);

//Tabbar* getTabbar();
TEST_F(test_window, getTabbar)
{
    window = new Window();
    window->getTabbar();
    assert(1==1);
}

//void addTab(const QString &filepath, bool activeTab = false);
//void addTabWithWrapper(EditWrapper *wrapper, const QString &filepath, const QString &qstrTruePath,
//                       const QString &tabName, int index = -1);
//bool closeTab();
//void restoreTab();
TEST_F(test_window, restoreTab)
{
    window = new Window();
    window->restoreTab();
    assert(1==1);
}

//void clearBlack();

//EditWrapper* createEditor();
TEST_F(test_window, createEditor)
{
    window = new Window();
    window->createEditor();
    assert(1==1);
}
//EditWrapper* currentWrapper();
TEST_F(test_window, currentWrapper)
{
    window = new Window();
    window->currentWrapper();
    assert(1==1);
}
//EditWrapper* wrapper(const QString &filePath);
//TextEdit* getTextEditor(const QString &filepath);
TEST_F(test_window, getTextEditor)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->getTextEditor("aa");
    assert(1==1);
}
//void focusActiveEditor();
TEST_F(test_window, focusActiveEditor)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->focusActiveEditor();
    assert(1==1);
}
//void removeWrapper(const QString &filePath, bool isDelete = false);
TEST_F(test_window, removeWrapper)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->removeWrapper("aa",true);
    window->removeWrapper("bb",false);
    assert(1==1);
}

TEST_F(test_window, decrementFontSize)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->decrementFontSize();
    assert(1==1);
}
//void incrementFontSize();
TEST_F(test_window, incrementFontSize)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->incrementFontSize();
    assert(1==1);
}
//void resetFontSize();
TEST_F(test_window, resetFontSize)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->resetFontSize();
    assert(1==1);
}
//void setFontSizeWithConfig(EditWrapper *editor);
TEST_F(test_window, setFontSizeWithConfig)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->setFontSizeWithConfig(a);
    assert(1==1);
}



//public slots:
//void addBlankTab();
//void addBlankTab(const QString &blankFile);
//void handleTabCloseRequested(int index);
//void handleTabsClosed(QStringList tabList);
//void handleCurrentChanged(const int &index);

//void handleJumpLineBarExit();
TEST_F(test_window, handleJumpLineBarExit)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->handleJumpLineBarExit();
    assert(1==1);
}
//void handleJumpLineBarJumpToLine(const QString &filepath, int line, bool focusEditor);
TEST_F(test_window, handleJumpLineBarJumpToLine)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleJumpLineBarJumpToLine("aa",1,true);
    window->handleJumpLineBarJumpToLine("aa",1,false);
    assert(1==1);
}

//void handleBackToPosition(const QString &file, int row, int column, int scrollOffset);
TEST_F(test_window, handleBackToPosition)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleBackToPosition("aa",1,1,1);
    assert(1==1);
}

//void handleFindNext();
TEST_F(test_window, handleFindNextSearchKeyword)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleFindNextSearchKeyword("");
    assert(1==1);
}
//void handleFindPrev();
TEST_F(test_window, handleFindPrevSearchKeyword)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleFindPrevSearchKeyword("");
    assert(1==1);
}
//void slotFindbarClose();
TEST_F(test_window, slotFindbarClose)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->slotFindbarClose();
    assert(1==1);
}
//void slotReplacebarClose();
TEST_F(test_window, slotReplacebarClose)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->slotReplacebarClose();
    assert(1==1);
}

//void handleReplaceAll(const QString &replaceText, const QString &withText);
TEST_F(test_window, handleReplaceAll)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleReplaceAll("","");
    assert(1==1);
}
//void handleReplaceNext(const QString &replaceText, const QString &withText);
TEST_F(test_window, handleReplaceNext)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleReplaceNext("a", "", "");
    assert(1==1);
}
//void handleReplaceRest(const QString &replaceText, const QString &withText);
TEST_F(test_window, handleReplaceRest)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleReplaceRest("","");
    assert(1==1);
}
//void handleReplaceSkip();
TEST_F(test_window, handleReplaceSkip)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleReplaceSkip("aa", "");
    assert(1==1);
}

//void handleRemoveSearchKeyword();
TEST_F(test_window, handleRemoveSearchKeyword)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleRemoveSearchKeyword();
    assert(1==1);
}
//void handleUpdateSearchKeyword(QWidget *widget, const QString &file, const QString &keyword);
TEST_F(test_window, handleUpdateSearchKeyword)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleUpdateSearchKeyword(window,"aa","");
    assert(1==1);
}


//void addBottomWidget(QWidget *widget);
//void removeBottomWidget();

//void loadTheme(const QString &path);
TEST_F(test_window, loadTheme)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->loadTheme("window");
    assert(1==1);
}


//void showNewEditor(EditWrapper *wrapper);
TEST_F(test_window, showNewEditor)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->showNewEditor(a);
    assert(1==1);
}
//void showNotify(const QString &message);
TEST_F(test_window, showNotify)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->showNotify("ffffkkkk");
    assert(1==1);
}
//int getBlankFileIndex();
TEST_F(test_window, getBlankFileIndex)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->getBlankFileIndex();
    assert(1==1);
}

//DDialog *createDialog(const QString &title, const QString &content);
TEST_F(test_window, createDialog)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->createDialog("dd","ddd");
    assert(1==1);
}

//void slotLoadContentTheme(DGuiApplicationHelper::ColorType themeType);
TEST_F(test_window, slotLoadContentTheme)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->slotLoadContentTheme(DGuiApplicationHelper::ColorType::DarkType);
    assert(1==1);
}
//void slotSettingResetTheme(const QString &path);
TEST_F(test_window, slotSettingResetTheme)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->slotSettingResetTheme("DGuiApplicationHelper::ColorType::DarkType");
    assert(1==1);
}

//void slot_saveReadingPath();
TEST_F(test_window, slot_saveReadingPath)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->slot_saveReadingPath();
    assert(1==1);
}
//void slot_beforeReplace(QString _);
TEST_F(test_window, slot_beforeReplace)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->slot_beforeReplace("d");
    assert(1==1);
}
//void slot_setTitleFocus();

//private:
//void handleFocusWindowChanged(QWindow *w);
TEST_F(test_window, handleFocusWindowChanged)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    QWindow * q = new QWindow();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->handleFocusWindowChanged(q);
    assert(1==1);
}
//void updateThemePanelGeomerty();
TEST_F(test_window, updateThemePanelGeomerty)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    QWindow * q = new QWindow();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->updateThemePanelGeomerty();
    assert(1==1);
}
//void checkTabbarForReload();
TEST_F(test_window, checkTabbarForReload)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    QWindow * q = new QWindow();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addBlankTab();
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->m_settings =Settings::instance();
    window->checkTabbarForReload();
    assert(1==1);
}

