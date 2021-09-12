#include "ut_startmanager.h"

test_startmanager::test_startmanager()
{

}

TEST_F(test_startmanager, StartManager)
{
    //StartManager startManager(nullptr);
    StartManager *pStartManager = new StartManager(nullptr);
    ASSERT_TRUE(pStartManager != nullptr);

    pStartManager->deleteLater();
}

TEST_F(test_startmanager, instance)
{
    StartManager *pStartManager = StartManager::instance();
    ASSERT_TRUE(pStartManager != nullptr);

    pStartManager->deleteLater();
}

TEST_F(test_startmanager, openFilesInWindow)
{
    StartManager *startManager = StartManager::instance();
    QStringList filePathList;
    filePathList<<".cache/deepin/deepin-editor";
    startManager->openFilesInWindow(filePathList);

    filePathList<<".cache/deepin/";
    startManager->openFilesInWindow(filePathList);
    ASSERT_TRUE(startManager->m_windows.at(0) != nullptr);

    startManager->deleteLater();
}
//initWindowPosition
TEST_F(test_startmanager, initWindowPosition)
{
    StartManager *startManager = StartManager::instance();
    startManager->openFilesInTab(QStringList());
    ASSERT_TRUE(startManager->m_windows.at(0) != nullptr);

    startManager->deleteLater();
}

//getFileTabInfo
TEST_F(test_startmanager, getFileTabInfo)
{
    StartManager *startManager = StartManager::instance();
    StartManager::FileTabInfo fileTabInfo = startManager->getFileTabInfo(".cache/deepin/deepin-editor");

    ASSERT_TRUE(fileTabInfo.windowIndex != 0);
    startManager->deleteLater();
}

//slotCheckUnsaveTab
TEST_F(test_startmanager, slotCheckUnsaveTab)
{
    StartManager *startManager = StartManager::instance();
    startManager->openFilesInWindow(QStringList());
    startManager->slotCheckUnsaveTab();

    bool bRet = startManager->m_windows.at(0)->checkBlockShutdown();
    ASSERT_TRUE(bRet == false);

    startManager->deleteLater();
}

//checkPath
TEST_F(test_startmanager, checkPath)
{
    StartManager *startManager = StartManager::instance();
    startManager->m_windows.clear();
    bool bRet = startManager->checkPath(".cache/deepin/deepin-editor");
    ASSERT_TRUE(bRet == true);
    
    startManager->deleteLater();
}
TEST_F(test_startmanager,ifKlu )
{
    StartManager *startManager = StartManager::instance();
    bool bRet = startManager->ifKlu();
    ASSERT_TRUE(bRet == false);

    startManager->deleteLater();
}

//loadTheme
TEST_F(test_startmanager,loadTheme )
{
    StartManager *startManager = StartManager::instance();
    startManager->loadTheme("Dark");
    ASSERT_TRUE(startManager != nullptr);

    startManager->deleteLater();
}

// bool isMultiWindow();
TEST_F(test_startmanager,isMultiWindow )
{
    StartManager *startManager = StartManager::instance();
    bool bRet = startManager->isMultiWindow();
    ASSERT_TRUE(bRet == false);
    
    startManager->deleteLater();
}

TEST_F(test_startmanager,isTemFilesEmpty )
{
    StartManager *startManager = StartManager::instance();
    bool bRet = startManager->isTemFilesEmpty();
    ASSERT_TRUE(bRet == true);
    
    startManager->deleteLater();
}

TEST_F(test_startmanager,autoBackupFile)
{
    StartManager *startManager = StartManager::instance();
    startManager->autoBackupFile();
    ASSERT_TRUE(startManager != nullptr);

    startManager->deleteLater();
}

TEST_F(test_startmanager,recoverFile)
{
    StartManager *startManager = StartManager::instance();
    Window w;
    int iRet = startManager->recoverFile(&w);
    ASSERT_TRUE(iRet == 0);
    
    startManager->deleteLater();
}

TEST_F(test_startmanager,openFilesInTab)
{
    StartManager *startManager = StartManager::instance();
    startManager->openFilesInTab(QStringList());

    startManager->openFilesInTab(QStringList(".cache/deepin/deepin-editor"));

    startManager->slotCloseWindow();
    startManager->slotCreatNewwindow();

    ASSERT_TRUE(startManager->m_windows.count() > 0);
    startManager->deleteLater();
}

TEST_F(test_startmanager,analyzeBookmakeInfo)
{
    StartManager *startManager = StartManager::instance();
    QList<int> list = startManager->analyzeBookmakeInfo(QString());
    ASSERT_TRUE(!list.isEmpty());
    
    startManager->deleteLater();
}

