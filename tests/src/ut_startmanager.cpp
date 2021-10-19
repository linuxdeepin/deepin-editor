#include "ut_startmanager.h"
#include "src/stub.h"
#include "qdir.h"
#include "../../src/widgets/window.h"
namespace startmanagerstub {

EditWrapper *wrapperstub(const QString &filePath)
{
    return new EditWrapper;
}

StartManager::FileTabInfo getFileTabInfostub()
{
    return StartManager::FileTabInfo{1,1};
}

TextEdit *textEditorstub()
{
    return new TextEdit;
}
void returnstub()
{
    return;
}


bool returntruestub()
{
    return true;
}

bool returnfalsestub()
{
    return false;
}

QStringList entryliststub()
{
    return QStringList{"123","456"};
}

}

using namespace startmanagerstub;

UT_StartManager::UT_StartManager()
{

}

TEST(UT_StartManager_StartManager, StartManager)
{
    //StartManager startManager(nullptr);
    StartManager *pStartManager = new StartManager(nullptr);
    ASSERT_TRUE(pStartManager != nullptr);

    pStartManager->deleteLater();
}

TEST(UT_StartManager_instance, instance)
{
    StartManager *pStartManager = StartManager::instance();
    ASSERT_TRUE(pStartManager != nullptr);

    pStartManager->deleteLater();
}

TEST(UT_StartManager_openFilesInWindow, openFilesInWindow)
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
TEST(UT_StartManager_initWindowPosition, initWindowPosition)
{
    StartManager *startManager = StartManager::instance();
    startManager->openFilesInTab(QStringList());
    ASSERT_TRUE(startManager->m_windows.at(0) != nullptr);

    startManager->deleteLater();
}

//getFileTabInfo
TEST(UT_StartManager_getFileTabInfo, getFileTabInfo)
{
    StartManager *startManager = StartManager::instance();
    StartManager::FileTabInfo fileTabInfo = startManager->getFileTabInfo(".cache/deepin/deepin-editor");

    ASSERT_TRUE(fileTabInfo.windowIndex != 0);
    startManager->deleteLater();
}

//slotCheckUnsaveTab
TEST(UT_StartManager_slotCheckUnsaveTab, slotCheckUnsaveTab)
{
    StartManager *startManager = StartManager::instance();
    startManager->openFilesInWindow(QStringList());
    startManager->slotCheckUnsaveTab();

    bool bRet = startManager->m_windows.at(0)->checkBlockShutdown();
    ASSERT_TRUE(bRet == false);

    startManager->deleteLater();
}

//checkPath
TEST(UT_StartManager_checkPath, checkPath)
{
    StartManager *startManager = StartManager::instance();
    startManager->m_windows.clear();
    bool bRet = startManager->checkPath(".cache/deepin/deepin-editor");
    ASSERT_TRUE(bRet == true);
    
    startManager->deleteLater();
}
TEST(UT_StartManager_ifKlu,ifKlu )
{
    StartManager *startManager = StartManager::instance();
    bool bRet = startManager->ifKlu();
    ASSERT_TRUE(bRet == false);

    startManager->deleteLater();
}

//loadTheme
TEST(UT_StartManager_loadThem,loadTheme)
{
    StartManager *startManager = StartManager::instance();
    startManager->loadTheme("Dark");
    ASSERT_TRUE(startManager != nullptr);

    startManager->deleteLater();
}

// bool isMultiWindow();
TEST(UT_StartManager_isMultiWindow,isMultiWindow)
{
    StartManager *startManager = StartManager::instance();
    bool bRet = startManager->isMultiWindow();
    ASSERT_TRUE(bRet == false);
    
    startManager->deleteLater();
}

TEST(UT_StartManager_isTemFilesEmpty,isTemFilesEmpty)
{
//    StartManager *startManager = StartManager::instance();
//    bool bRet = startManager->isTemFilesEmpty();
//    ASSERT_TRUE(bRet == true);
    
//    startManager->deleteLater();
}

TEST(UT_StartManager_autoBackupFile,autoBackupFile)
{
    StartManager *startManager = StartManager::instance();
    Window* w1 = new Window;

    EditWrapper* wr1 = new EditWrapper;

    TextEdit* e1 = new TextEdit;
    e1->m_wrapper = wr1;
    e1->m_listBookmark = {1,2};
    wr1->m_bIsTemFile = true;
    wr1->m_pTextEdit = e1;

    QList<Window*> windows = {w1};
    QMap<QString, EditWrapper *> wrappers{ {"wr1",wr1}};
    startManager->m_windows = windows;
    w1->m_wrappers = wrappers;

    Stub s1;
    s1.set(ADDR(QStringList,replace),returnstub);
    Stub s2;
    s2.set(ADDR(StartManager,getFileTabInfo),getFileTabInfostub);
    s2.set(ADDR(EditWrapper,saveTemFile),returnstub);

    startManager->autoBackupFile();
    ASSERT_TRUE(startManager != nullptr);

    startManager->deleteLater();
    w1->deleteLater();
    wr1->deleteLater();
    e1->deleteLater();
}

TEST(UT_StartManager_recoverFile,recoverFile_001)
{
    StartManager *startManager = StartManager::instance();
    QString c1 = "{\"bookMark\":\"7,7,8,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\",\"cursorPosition\":\"7\",\"focus\":true,\"localPath\":\"/home/uos/.local/share/deepin/deepin-editor/blank-files/blank_file_2021-09-22_09-43-10-824\",\"modify\":true}";
    QString c2 = "{\"bookMark\":\"1,0,1,0,0\",\"cursorPosition\":\"23\",\"localPath\":\"/home/uos/Desktop/563/526.txt\",\"modify\":false}";
    startManager->m_qlistTemFile.push_back(c1);
    startManager->m_qlistTemFile.push_back(c2);

    Stub s1;
    s1.set(ADDR(Window,wrapper),wrapperstub);
    Stub s2;
    s2.set(ADDR(StartManager,popupExistTabs),returnstub);

    Window w;
    int iRet = startManager->recoverFile(&w);
    ASSERT_TRUE(iRet == 0);
    
    startManager->deleteLater();
}

TEST(UT_StartManager_recoverFile,recoverFile_002)
{
    StartManager *startManager = StartManager::instance();
    QString c1 = "{\"bookMark\":\"7,7,8,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\",\"cursorPosition\":\"7\",\"focus\":true,\"localPath\":\"/home/uos/.local/share/deepin/deepin-editor/blank-files/blank_file_2021-09-22_09-43-10-824\",\"modify\":true}";
    QString c2 = "{\"bookMark\":\"1,0,1,0,0\",\"cursorPosition\":\"23\",\"localPath\":\"/home/uos/Desktop/563/526.txt\",\"modify\":false}";
    startManager->m_qlistTemFile.push_back(c1);
    startManager->m_qlistTemFile.push_back(c2);

    Stub s1;
    s1.set(ADDR(Window,wrapper),wrapperstub);
    Stub s2;
    s2.set(ADDR(StartManager,popupExistTabs),returnstub);
    Stub s3;
    s3.set((bool(QJsonObject::*)(const QString &) const)ADDR(QJsonObject,contains), returntruestub);
    Stub s4;
    s4.set(ADDR(QString,isEmpty),returnfalsestub);
    Stub s5;
    s5.set(ADDR(Utils,fileExists),returntruestub);
    Stub s6;
    s6.set(ADDR(QJsonValue,isDouble),returntruestub);


    Window w;
    int iRet = startManager->recoverFile(&w);
    ASSERT_TRUE(iRet == 0);

    startManager->deleteLater();
}

TEST(UT_StartManager_openFilesInTab,openFilesInTab_001)
{
    StartManager *startManager = StartManager::instance();
    startManager->m_windows.clear();

    Stub s1;
    s1.set(ADDR(Window,wrapper),wrapperstub);
    Stub s2;
    s2.set(ADDR(StartManager,checkPath),returntruestub);


    startManager->openFilesInTab(QStringList());
    startManager->openFilesInTab(QStringList(".cache/deepin/deepin-editor"));

    startManager->slotCloseWindow();
    startManager->slotCreatNewwindow();

    ASSERT_TRUE(startManager->m_windows.count() > 0);
    startManager->deleteLater();
}

TEST(UT_StartManager_openFilesInTab,openFilesInTab_002)
{
    StartManager *startManager = StartManager::instance();
    startManager->m_windows.clear();

    Stub s1;
    s1.set(ADDR(Window,wrapper),wrapperstub);
    Stub s2;
    s2.set(ADDR(StartManager,checkPath),returntruestub);
    Stub s3;
    s3.set(ADDR(StartManager,isTemFilesEmpty),returntruestub);

    startManager->openFilesInTab(QStringList());
    startManager->openFilesInTab(QStringList(".cache/deepin/deepin-editor"));

    startManager->slotCloseWindow();
    startManager->slotCreatNewwindow();

    ASSERT_TRUE(startManager->m_windows.count() > 0);
    startManager->deleteLater();
}

TEST(UT_StartManager_openFilesInTab,openFilesInTab_003)
{
    StartManager *startManager = StartManager::instance();
    startManager->m_windows.clear();

    Stub s1;
    s1.set(ADDR(Window,wrapper),wrapperstub);
    Stub s2;
    s2.set(ADDR(StartManager,checkPath),returntruestub);
    Stub s3;
    s3.set(ADDR(StartManager,isTemFilesEmpty),returnfalsestub);

    startManager->openFilesInTab(QStringList());
    startManager->openFilesInTab(QStringList(".cache/deepin/deepin-editor"));

    startManager->slotCloseWindow();
    startManager->slotCreatNewwindow();

    ASSERT_TRUE(startManager->m_windows.count() > 0);
    startManager->deleteLater();
}

TEST(UT_StartManager_analyzeBookmakeInfo,analyzeBookmakeInfo)
{
    StartManager *startManager = StartManager::instance();
    QList<int> list = startManager->analyzeBookmakeInfo(QString());
    ASSERT_TRUE(!list.isEmpty());
    
    startManager->deleteLater();
}

TEST(UT_StartManager_slotCloseWindow,slotCloseWindow)
{
    StartManager *startManager = StartManager::instance();
    startManager->m_windows.clear();

    Stub s1;
    s1.set((QStringList(QDir::*)(QDir::Filters ,QDir::SortFlags) const)ADDR(QDir,entryList), entryliststub);
    Stub s2;
    s2.set((bool(QString::*)(const QString &,Qt::CaseSensitivity cs) const)ADDR(QString,contains), returntruestub);
    Stub s3;
    s3.set(ADDR(QList<Window*>,isEmpty), returntruestub);

    startManager->slotCloseWindow();

    startManager->deleteLater();
}

