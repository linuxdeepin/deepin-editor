/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ut_editwrapper.h"

test_editwrapper::test_editwrapper()
{
    QString text = QString("#include \"window.h\"\n"
                           "#include \"urlinfo.h\"\n"
                           "int main(int argc, char *argv[])\n"
                           "{\n"
                           "using namespace Dtk::Core;\n"
                           "PerformanceMonitor::initializeAppStart();\n"
                           "return 0;\n"
                           "}");
    QFile f("1.cpp");
    if(f.open(QFile::WriteOnly)){
        f.write(text.toUtf8());
        f.close();
    }
}

TEST_F(test_editwrapper, EditWrapper)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    ASSERT_TRUE(pWindow->currentWrapper()->m_pBottomBar != nullptr);
    ASSERT_TRUE(pWindow->currentWrapper()->m_pTextEdit != nullptr);
    
    delete pWindow;
    pWindow = nullptr;
}

//void setQuitFlag();
TEST_F(test_editwrapper, setQuitFlag)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->setQuitFlag();
    ASSERT_TRUE(pWindow->currentWrapper()->m_bQuit == true);

    delete pWindow;
    pWindow = nullptr;
}

//bool getFileLoading();
TEST_F(test_editwrapper, getFileLoading)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->setQuitFlag();
    ASSERT_TRUE(pWindow->currentWrapper()->getFileLoading() == true);

    delete pWindow;
    pWindow = nullptr;
}

//void openFile(const QString &filepath,QString qstrTruePath,bool bIsTemFile = false);
TEST_F(test_editwrapper, openFile)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->openFile(QString("a.cpp"), QString("b.cpp"));
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->m_bIsFileOpen == true);

    delete pWindow;
    pWindow = nullptr;
}

bool saveFile_001_stub()
{
    return true;
}

//bool saveFile_001();
TEST_F(test_editwrapper, saveFile_001)
{
//    Window *pWindow = new Window();
//    pWindow->addBlankTab(QString());
//    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
//                                                          QString("12345"));
//    bool bRet = pWindow->currentWrapper()->saveFile();
//    if (bRet == false) {
//        Stub stub;
//        stub.set(pWindow->currentWrapper()->saveFile(), saveFile_001_stub());
//        bRet = pWindow->currentWrapper()->saveFile();
//    }
//    ASSERT_TRUE(bRet == true);

//    pWindow->deleteLater();
}

bool saveFile_002_stub()
{
    return true;
}

//bool saveFile_002();
TEST_F(test_editwrapper, saveFile_002)
{
//    Window *pWindow = new Window();
//    pWindow->addBlankTab(QString());
//    bool bRet = pWindow->currentWrapper()->saveFile();
//    if (bRet == false) {
//        Stub stub;
//        stub.set(pWindow->currentWrapper()->saveFile(), saveFile_002_stub());
//        bRet = pWindow->currentWrapper()->saveFile();
//    }
//    ASSERT_TRUE(bRet == true);

//    delete pWindow;
//    pWindow = nullptr;
}

//bool saveFile_003();
TEST_F(test_editwrapper, saveFile_003)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->m_sFilePath = QString("");
    pWindow->currentWrapper()->textEditor()->m_qstrTruePath = QString("");
    bool bRet = pWindow->currentWrapper()->saveFile();
    ASSERT_TRUE(bRet == false);

    delete pWindow;
    pWindow = nullptr;
}

//bool saveAsFile_001(const QString &newFilePath, QByteArray encodeName);
TEST_F(test_editwrapper, saveAsFile_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    bool bRet = pWindow->currentWrapper()->saveAsFile(QString(), QByteArray("UTF-8"));
    ASSERT_TRUE(bRet == false);

    pWindow->deleteLater();
}

bool saveAsFile_002_stub()
{
    return true;
}

//bool saveAsFile_001(const QString &newFilePath, QByteArray encodeName);
TEST_F(test_editwrapper, saveAsFile_002)
{
//    Window *pWindow = new Window();
//    pWindow->addBlankTab(QString());
//    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
//                                                          QString("12345"));
//    QString newFilePaht(pWindow->currentWrapper()->textEditor()->getTruePath());
//    bool bRet = pWindow->currentWrapper()->saveAsFile(newFilePaht, QByteArray("UTF-8"));
//    if (bRet == false) {
//        Stub stub;
//        stub.set(pWindow->currentWrapper()->saveAsFile(), saveAsFile_002_stub());
//    }
//    ASSERT_TRUE(bRet == true);

//    pWindow->deleteLater();
}

//void updatePath(const QString &file);
TEST_F(test_editwrapper, updatePath)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    QString strFilePath(pWindow->currentWrapper()->textEditor()->m_sFilePath);
    QString strTruePath(pWindow->currentWrapper()->textEditor()->m_qstrTruePath);
    ASSERT_TRUE(!strFilePath.compare(strTruePath));

    delete pWindow;
    pWindow = nullptr;
}

//void hideWarningNotices();
TEST_F(test_editwrapper, hideWarningNotices)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    if (pWindow->currentWrapper()->m_pWaringNotices != nullptr) {
        pWindow->currentWrapper()->m_pWaringNotices->show();
    }
    pWindow->currentWrapper()->hideWarningNotices();
    ASSERT_TRUE(pWindow->currentWrapper()->m_pWaringNotices->isHidden());
    
    delete pWindow;
    pWindow = nullptr;
}

//void checkForReload();
TEST_F(test_editwrapper, checkForReload_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->checkForReload();
    QString strFilePath(pWindow->currentWrapper()->textEditor()->m_sFilePath);
    QString strTruePath(pWindow->currentWrapper()->textEditor()->m_qstrTruePath);
    ASSERT_TRUE(!strFilePath.compare(strTruePath));

    pWindow->deleteLater();
}

bool checkForReload_002_stub()
{
    return false;
}

//void checkForReload();
TEST_F(test_editwrapper, checkForReload_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    Stub stub;
    stub.set(ADDR(Utils,isDraftFile), checkForReload_002_stub);
    pWindow->currentWrapper()->checkForReload();
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->getTruePath() != nullptr);

    pWindow->deleteLater();
}

//void initToastPosition() 无实现;

//void showNotify(const QString &message);
TEST_F(test_editwrapper, showNotify_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->setReadOnlyPermission(true);
    pWindow->currentWrapper()->textEditor()->m_readOnlyMode = true;
    pWindow->currentWrapper()->showNotify(QString("read-only"));
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->getReadOnlyPermission() == true);

    pWindow->deleteLater();
}

//void showNotify(const QString &message);
TEST_F(test_editwrapper, showNotify_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->setReadOnlyPermission(false);
    pWindow->currentWrapper()->textEditor()->m_readOnlyMode = false;
    pWindow->currentWrapper()->showNotify(QString("Not read-only"));
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->getReadOnlyPermission() == false);

    pWindow->deleteLater();
}

//void setLineNumberShow(bool bIsShow,bool bIsFirstShow = false);
TEST_F(test_editwrapper, setLineNumberShow_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->setLineNumberShow(true, false);
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->getLeftAreaWidget()->m_pLineNumberArea->isVisible());

    pWindow->deleteLater();
}

//void setLineNumberShow(bool bIsShow,bool bIsFirstShow = false);
TEST_F(test_editwrapper, setLineNumberShow_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->setLineNumberShow(false);
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->getLeftAreaWidget()->m_pLineNumberArea->isHidden());

    pWindow->deleteLater();
}

//void setShowBlankCharacter(bool ok);
TEST_F(test_editwrapper, setShowBlankCharacter_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->setShowBlankCharacter(true);
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->document()->defaultTextOption().flags() == QTextOption::ShowTabsAndSpaces);

    pWindow->deleteLater();
}

//void setShowBlankCharacter(bool ok);
TEST_F(test_editwrapper, setShowBlankCharacter_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->setShowBlankCharacter(false);
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->document()->defaultTextOption().flags() != QTextOption::ShowTabsAndSpaces);

    pWindow->deleteLater();
}

//BottomBar *bottomBar();
TEST_F(test_editwrapper, bottomBar)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    ASSERT_TRUE(pWindow->currentWrapper()->bottomBar());
    
    pWindow->deleteLater();
}

//QString filePath();
TEST_F(test_editwrapper, filePath)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    ASSERT_TRUE(!pWindow->currentWrapper()->filePath().isEmpty());
    
    pWindow->deleteLater();
}

//TextEdit *textEditor() { return m_textEdit; }
TEST_F(test_editwrapper, textEditor)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor());

    pWindow->deleteLater();
}

//int GetCorrectUnicode1(const QByteArray &ba) 未实现;

int saveDraftFile_stub()
{
    return 1;
}

//bool saveDraftFile(); Subsequent processing
TEST_F(test_editwrapper, saveDraftFile)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    //Stub stub;
    //stub.set(ADDR(DFileDialog, exec), saveDraftFile_stub);
    //pWindow->currentWrapper()->saveDraftFile();

    pWindow->deleteLater();
}

void readFile_stub_001()
{
    return;
}

//void readFile(const QString &filePath);
TEST_F(test_editwrapper, readFile_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    QString filePath = QCoreApplication::applicationDirPath() + QString("/Makefile");
    pWindow->currentWrapper()->textEditor()->setTruePath(filePath);
    pWindow->currentWrapper()->textEditor()->m_sFilePath = filePath;
    Stub stub;
    stub.set(ADDR(EditWrapper,loadContent), readFile_stub_001);
    bool bRet = pWindow->currentWrapper()->readFile(QByteArray());
    ASSERT_TRUE(bRet);
    
    pWindow->deleteLater();
}

//void readFile(const QString &filePath);
TEST_F(test_editwrapper, readFile_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    QString filePath = QCoreApplication::applicationDirPath() + QString("/Makefile001");
    pWindow->currentWrapper()->textEditor()->setTruePath(filePath);
    pWindow->currentWrapper()->textEditor()->m_sFilePath = filePath;
    Stub stub;
    stub.set(ADDR(EditWrapper,loadContent), readFile_stub_001);
    bool bRet = pWindow->currentWrapper()->readFile(QByteArray("UTF-8"));
    ASSERT_FALSE(bRet);

    pWindow->deleteLater();
}

//void handleFileLoadFinished(const QByteArray &encode,const QString &content);
TEST_F(test_editwrapper, handleFileLoadFinished)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    const QString filePath = QCoreApplication::applicationDirPath() + QString("/Makefile");
    pWindow->currentWrapper()->openFile(filePath, filePath, false);

    pWindow->deleteLater();
}

////重新加载文件编码 1.文件修改 2.文件未修改处理逻辑一样 切换编码重新加载和另存为 梁卫东
//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode)
{
    Window* pWindow = new Window;
    char c = 'd';
    QByteArray d(5,c);
    EditWrapper *wrapper = pWindow->createEditor();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->reloadFileEncode(d);
    
}

////重写加载修改文件
//void reloadModifyFile();
TEST_F(test_editwrapper, reloadModifyFile)
{
    Window* pWindow = new Window;
    char c = 'd';
    QByteArray d(5,c);
    EditWrapper *wrapper = pWindow->createEditor();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->reloadModifyFile();
    
}
////获取文件编码
//QString getTextEncode();
TEST_F(test_editwrapper, getTextEncode)
{
    Window* pWindow = new Window;
    char c = 'd';
    QByteArray d(5,c);
    EditWrapper *wrapper = pWindow->createEditor();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->getTextEncode();
    
}
//bool saveTemFile(QString qstrDir);
//TEST_F(test_editwrapper, saveTemFile)
//{
//    Window* pWindow = new Window;
//    char c = 'd';
//    QByteArray d(5,c);
//    EditWrapper *wrapper = pWindow->createEditor();
//    Settings *s = new Settings();
//    wrapper->textEditor()->setSettings(s);
//    wrapper->textEditor()->setWrapper(wrapper);
//    wrapper->openFile("1.cpp","1.cpp");
//    wrapper->saveTemFile("ddd");
//    
//}
////跟新路径
//void updatePath(const QString &file,QString qstrTruePath = QString());
////判断是否修改
//bool isModified();
TEST_F(test_editwrapper, isModified)
{
    Window* pWindow = new Window;
    char c = 'd';
    QByteArray d(5,c);
    EditWrapper *wrapper = pWindow->createEditor();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->isModified();
    
}
////判断是否草稿文件
//bool isDraftFile();
TEST_F(test_editwrapper, isDraftFile)
{
    Window* pWindow = new Window;
    char c = 'd';
    QByteArray d(5,c);
    EditWrapper *wrapper = pWindow->createEditor();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->isDraftFile();
    
}
////判断内容是否为空
//bool isPlainTextEmpty();
TEST_F(test_editwrapper, isPlainTextEmpty)
{
    Window* pWindow = new Window;
    char c = 'd';
    QByteArray d(5,c);
    EditWrapper *wrapper = pWindow->createEditor();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->isPlainTextEmpty();
    
}



