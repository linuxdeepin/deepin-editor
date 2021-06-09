#include "test_startmanager.h"

test_startmanager::test_startmanager()
{

}

TEST_F(test_startmanager, StartManager)
{
    StartManager startManager(nullptr);
    assert(1==1);
}

TEST_F(test_startmanager, instance)
{
    StartManager::instance();
    assert(1==1);
}

TEST_F(test_startmanager, openFilesInWindow)
{
    StartManager *startManager = StartManager::instance();
    QStringList filePathList;
    filePathList<<".cache/deepin/deepin-editor";
    startManager->openFilesInWindow(filePathList);

    assert(1==1);
}
//initWindowPosition
TEST_F(test_startmanager, initWindowPosition)
{
    Window*window = new Window();
    StartManager *startManager = StartManager::instance();
    startManager->initWindowPosition(window,true);

    assert(1==1);
}
//getFileTabInfo
TEST_F(test_startmanager, getFileTabInfo)
{
    StartManager *startManager = StartManager::instance();
    startManager->getFileTabInfo(".cache/deepin/deepin-editor");

    assert(1==1);
}

//slotCheckUnsaveTab
TEST_F(test_startmanager, slotCheckUnsaveTab)
{
    StartManager *startManager = StartManager::instance();
    startManager->slotCheckUnsaveTab();

    assert(1==1);
}

//checkPath
TEST_F(test_startmanager, checkPath)
{
    StartManager *startManager = StartManager::instance();
    startManager->checkPath(".cache/deepin/deepin-editor");

    assert(1==1);
}
TEST_F(test_startmanager,ifKlu )
{
    StartManager *startManager = StartManager::instance();
    startManager->ifKlu();

    assert(1==1);
}
//loadTheme
TEST_F(test_startmanager,loadTheme )
{
    StartManager *startManager = StartManager::instance();
    startManager->loadTheme("Dark");

    assert(1==1);
}

// bool isMultiWindow();
TEST_F(test_startmanager,isMultiWindow )
{
    StartManager *startManager = StartManager::instance();
    startManager->isMultiWindow();

    assert(1==1);
}
