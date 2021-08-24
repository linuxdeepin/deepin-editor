#include "test_startmanager.h"

test_startmanager::test_startmanager()
{

}

TEST_F(test_startmanager, StartManager)
{
    StartManager startManager(nullptr);
    
}

TEST_F(test_startmanager, instance)
{
    StartManager::instance();
    
}

TEST_F(test_startmanager, openFilesInWindow)
{
    StartManager *startManager = StartManager::instance();
    QStringList filePathList;
    filePathList<<".cache/deepin/deepin-editor";
    startManager->openFilesInWindow(filePathList);

    filePathList<<".cache/deepin/";
    startManager->openFilesInWindow(filePathList);

    
}
//initWindowPosition
TEST_F(test_startmanager, initWindowPosition)
{
    Window*window = new Window();
    StartManager *startManager = StartManager::instance();
    startManager->initWindowPosition(window,true);

    
}
//getFileTabInfo
TEST_F(test_startmanager, getFileTabInfo)
{
    StartManager *startManager = StartManager::instance();
    startManager->getFileTabInfo(".cache/deepin/deepin-editor");

    
}

//slotCheckUnsaveTab
TEST_F(test_startmanager, slotCheckUnsaveTab)
{
    StartManager *startManager = StartManager::instance();
    startManager->slotCheckUnsaveTab();

    
}

//checkPath
TEST_F(test_startmanager, checkPath)
{
    StartManager *startManager = StartManager::instance();
    startManager->checkPath(".cache/deepin/deepin-editor");

    
}
TEST_F(test_startmanager,ifKlu )
{
    StartManager *startManager = StartManager::instance();
    startManager->ifKlu();

    
}
//loadTheme
TEST_F(test_startmanager,loadTheme )
{
    StartManager *startManager = StartManager::instance();
    startManager->loadTheme("Dark");

    
}

// bool isMultiWindow();
TEST_F(test_startmanager,isMultiWindow )
{
    StartManager *startManager = StartManager::instance();
    startManager->isMultiWindow();

    
}

TEST_F(test_startmanager,isTemFilesEmpty )
{
    StartManager *startManager = StartManager::instance();
    startManager->isTemFilesEmpty();

    
}

TEST_F(test_startmanager,autoBackupFile)
{
    StartManager *startManager = StartManager::instance();
    startManager->autoBackupFile();

    
}

TEST_F(test_startmanager,recoverFile)
{
    StartManager *startManager = StartManager::instance();
    Window w;
    startManager->recoverFile(&w);

    
}

TEST_F(test_startmanager,openFilesInTab)
{
    StartManager *startManager = StartManager::instance();
    startManager->openFilesInTab(QStringList());

    startManager->openFilesInTab(QStringList(".cache/deepin/deepin-editor"));

    startManager->slotCloseWindow();
    startManager->slotCreatNewwindow();



    
}

TEST_F(test_startmanager,analyzeBookmakeInfo)
{
    StartManager *startManager = StartManager::instance();
    startManager->analyzeBookmakeInfo(QString());

    
}

