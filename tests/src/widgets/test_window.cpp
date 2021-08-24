#include "test_window.h"

test_window::test_window()
{

}

TEST_F(test_window, Window)
{
    Window window1;
    
}

TEST_F(test_window, showCenterWindow)
{
    Window * window1 = new Window();
    window1->showCenterWindow(true);

    
}
//initTitlebar
TEST_F(test_window, initTitlebar)
{
    Window * window1 = new Window();
    window1->initTitlebar();

    
}
TEST_F(test_window, checkBlockShutdown)
{
    Window * window1 = new Window();
    window1->checkBlockShutdown();

    
}
//getTabIndex
TEST_F(test_window, getTabIndex)
{
    Window * window1 = new Window();
    window1->getTabIndex("aadd");

    
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

    
}
//addTab
TEST_F(test_window, addTab)
{
    Window * window1 = new Window();
    window1->addTab("aabb",true);

    
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
//    
//}
TEST_F(test_window, saveFile)
{
    Window * window1 = new Window();
    window1->saveFile();

    
}
//closeTab()
TEST_F(test_window, closeTab)
{
    Window * window = new Window();
    window->addBlankTab("aabb");
//    window->closeTab();
//    delete window;
//    window = nullptr;

    
}
//saveAsFileToDisk
TEST_F(test_window, saveAsFileToDisk)
{
    Window * window1 = new Window();
    window1->saveAsFileToDisk();

    
}
//saveBlankFileToDisk
TEST_F(test_window, saveBlankFileToDisk)
{
    Window * window1 = new Window();
    window1->saveBlankFileToDisk();

    
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

    
}
//changeSettingDialogComboxFontNumber
TEST_F(test_window, changeSettingDialogComboxFontNumber)
{
    EditWrapper * eeee = new EditWrapper();
    Window * window1 = new Window();
    window1->changeSettingDialogComboxFontNumber(3);

    
}
//popupFindBar
TEST_F(test_window, popupFindBar)
{
    Window * window1 = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window1->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window1->addBlankTab();
    window1->currentWrapper()->textEditor()->setPlainText(QString("12345"));
    window1->popupFindBar();

    window1->currentWrapper()->textEditor()->document()->clear();
    window1->popupFindBar();

    
}

//popupReplaceBar
TEST_F(test_window, popupReplaceBar)
{
    Window * window1 = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window1->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window1->addBlankTab();
    window1->currentWrapper()->textEditor()->setPlainText(QString("12345"));
    window1->popupReplaceBar();

    window1->currentWrapper()->textEditor()->document()->clear();
    window1->popupReplaceBar();

    
}

//popupJumpLineBar
TEST_F(test_window, popupJumpLineBar)
{
    Window * window1 = new Window();
    window1->popupJumpLineBar();

    EditWrapper *pEditWrapper = new EditWrapper();
    window1->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window1->addBlankTab();
    window1->currentWrapper()->textEditor()->setPlainText(QString("12345"));
    window1->popupJumpLineBar();

    window1->currentWrapper()->textEditor()->document()->clear();
    window1->popupJumpLineBar();

    
}

//updateJumpLineBar
TEST_F(test_window, updateJumpLineBar)
{
//    Window * window = new Window();
//    window->updateJumpLineBar();


    
}

//popupSettingsDialog
TEST_F(test_window, popupSettingsDialog)
{
//    Window * window = new Window();
//    window->popupSettingsDialog();
//    delete window;
//    window = nullptr;

    
}
//popupPrintDialog

TEST_F(test_window, popupPrintDialog)
{
//    Window * window = new Window();
//    window->popupPrintDialog();
//    delete window;
//    window = nullptr;

    
}
TEST_F(test_window, popupThemePanel)
{
    Window * window1 = new Window();
    window1->popupThemePanel();

    
}
//toggleFullscreen
TEST_F(test_window, toggleFullscreen)
{
    Window * window1 = new Window();
    window1->toggleFullscreen();

    
}
TEST_F(test_window, remberPositionSave)
{
//    Window * window = new Window();
//    window->remberPositionSave();
//    delete window;
//    window = nullptr;

    
}
//remberPositionRestore
TEST_F(test_window, remberPositionRestore)
{
    Window * window1 = new Window();
    window1->remberPositionRestore();

    
}
//displayShortcuts
TEST_F(test_window, displayShortcuts)
{
    Window * window1 = new Window();
    window1->displayShortcuts();

    
}
//setChildrenFocus
TEST_F(test_window, setChildrenFocus)
{
    Window * window1 = new Window();
    window1->setChildrenFocus(false);

    
}
//addBlankTab
TEST_F(test_window, addBlankTab)
{
    Window * window1 = new Window();
    window1->addBlankTab("aabb");

    
}
//handleTabCloseRequested
TEST_F(test_window, addTabWithWrapper)
{
    edit = new EditWrapper();
    window = new Window();
    window->addTabWithWrapper(edit,"aabb","aabb","aabb");

    
}
TEST_F(test_window, handleTabCloseRequested)
{
//    StartManager::instance()->createWindow()->addTabWithWrapper(edit,"aabb","aabb",0);
//    StartManager::instance()->createWindow()->handleTabCloseRequested(0);

    
}
//handleTabsClosed
TEST_F(test_window, handleTabsClosed)
{
    QStringList aa;
    window = new Window();
    window->handleTabsClosed(aa);

    
}
//handleCurrentChanged
TEST_F(test_window, handleCurrentChanged)
{
    QStringList aa;
    window = new Window();
    window->handleCurrentChanged(0);

    
}
//slot_setTitleFocus
TEST_F(test_window, slot_setTitleFocus)
{
//    QStringList aa;
    //window = new Window();
//    StartManager::instance()->createWindow()->slot_setTitleFocus();
    //window->slot_setTitleFocus();

    
}
//resizeEvent
TEST_F(test_window, resizeEvent)
{
    QStringList aa;
    window = new Window();
    QResizeEvent * eve;
    window->resizeEvent(eve);

    
}
//closeEvent
TEST_F(test_window, keyPressEvent)
{
    QStringList aa;
    window = new Window();
    QKeyEvent * eve;
    //window->keyPressEvent(eve);

    
}
//hideEvent
TEST_F(test_window, hideEvent)
{
    QStringList aa;
    window = new Window();
    QHideEvent * eve;
    window->hideEvent(eve);

    
}

//TextEdit
//void backupFile();
TEST_F(test_window, backupFile)
{
    window = new Window();
    window->backupFile();

    
}

//void closeAllFiles();
TEST_F(test_window, closeAllFiles)
{
    window = new Window();
    window->closeAllFiles();

    
}

//void addTemFileTab(QString qstrPath,QString qstrName,QString qstrTruePath,bool bIsTemFile = false);
TEST_F(test_window, addTemFileTab)
{
    window = new Window();
    window->addTemFileTab("aa","bb","cc");

    
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
    
}

//int getTabIndex(const QString &file);
//void activeTab(int index);

//Tabbar* getTabbar();
TEST_F(test_window, getTabbar)
{
    window = new Window();
    window->getTabbar();
    
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
    
}

//void clearBlack();

//EditWrapper* createEditor();
TEST_F(test_window, createEditor)
{
    window = new Window();
    window->createEditor();
    
}
//EditWrapper* currentWrapper();
TEST_F(test_window, currentWrapper)
{
    window = new Window();
    window->currentWrapper();
    
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
    
}
//void focusActiveEditor();
TEST_F(test_window, focusActiveEditor)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->addTabWithWrapper(a,"aa","aad","aadd",0);
    window->addTabWithWrapper(a,"bb","aad","aadd",1);
    window->focusActiveEditor();
    
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
    
}

TEST_F(test_window, decrementFontSize)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->decrementFontSize();
    
}
//void incrementFontSize();
TEST_F(test_window, incrementFontSize)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->incrementFontSize();
    
}
//void resetFontSize();
TEST_F(test_window, resetFontSize)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->resetFontSize();
    
}
//void setFontSizeWithConfig(EditWrapper *editor);
TEST_F(test_window, setFontSizeWithConfig)
{
    window = new Window();
    EditWrapper * a = new EditWrapper();
    window->m_settings =Settings::instance();
    window->setFontSizeWithConfig(a);
    
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
    
}

//void slotSigThemeChanged(const QString &path);
TEST_F(test_window, slotSigThemeChanged)
{
    Window *pWindow = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    pWindow->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    pWindow->addBlankTab();
    pWindow->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);
    pWindow->m_settings = Settings::instance();
    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::LightType);
    //pWindow->slotSigThemeChanged(DEEPIN_THEME);
    //pWindow->slotSigThemeChanged(DEEPIN_DARK_THEME);

    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::DarkType);
    //pWindow->slotSigThemeChanged(DEEPIN_THEME);
    //pWindow->slotSigThemeChanged(DEEPIN_DARK_THEME);

    
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

    
}

//void slotSigAdjustFont();
TEST_F(test_window, slotSigAdjustFont)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper,"bb", "aad", "aadd", 1);
    window->slotSigAdjustFont(QString());

    
}

//void slotSigAdjustFontSize();
TEST_F(test_window, slotSigAdjustFontSize)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);
    window->slotSigAdjustFontSize(14);

    
}

//void slotSigAdjustTabSpaceNumber();
TEST_F(test_window, slotSigAdjustTabSpaceNumber)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);

    window->slotSigAdjustTabSpaceNumber(14);
    
}

//void slotSigAdjustWordWrap();
TEST_F(test_window, slotSigAdjustWordWrap)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);

    window->slotSigAdjustWordWrap(true);
    
}

//void slotSigAdjustBookmark();
TEST_F(test_window, slotSigAdjustBookmark)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);

    window->slotSigAdjustBookmark(true);
    
}

//void slotSigShowBlankCharacter();
TEST_F(test_window, slotSigShowBlankCharacter)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);

    window->slotSigShowBlankCharacter(true);
    
}

//void slotSigHightLightCurrentLine();
TEST_F(test_window, slotSigHightLightCurrentLine)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);

    window->slotSigHightLightCurrentLine(true);
    
}

//void slotSigShowCodeFlodFlag();
TEST_F(test_window, slotSigShowCodeFlodFlag)
{
    window = new Window();
    EditWrapper *pEditWrapper = new EditWrapper();
    window->addTabWithWrapper(pEditWrapper, "aa", "aad", "aadd", 0);
    window->addBlankTab();
    window->addTabWithWrapper(pEditWrapper, "bb", "aad", "aadd", 1);

    window->slotSigShowCodeFlodFlag(true);
    
}

//void slotSigShowCodeFlodFlag();
TEST_F(test_window, slotSigChangeWindowSize)
{
    window = new Window();
    window->slotSigChangeWindowSize(QString());
    window->slotSigChangeWindowSize(QString("fullscreen"));
    window->slotSigChangeWindowSize(QString("window_maximum"));

    
}
