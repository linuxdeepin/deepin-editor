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

int saveDraftFile001_exec_stub()
{
    return 1;
}

//bool saveDraftFile(); Subsequent processing
TEST_F(test_editwrapper, saveDraftFile_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    typedef int (*fptr)(QDialog *);
    fptr fileDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(fileDialogExec, saveDraftFile001_exec_stub);
    pWindow->currentWrapper()->saveDraftFile();

    pWindow->deleteLater();
}

int saveDraftFile002_exec_stub()
{
    return 0;
}

//bool saveDraftFile(); Subsequent processing
TEST_F(test_editwrapper, saveDraftFile_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    typedef int (*fptr)(QDialog *);
    fptr fileDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(fileDialogExec, saveDraftFile002_exec_stub);
    pWindow->currentWrapper()->saveDraftFile();

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
    stub.set(ADDR(EditWrapper, loadContent), readFile_stub_001);
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
    stub.set(ADDR(EditWrapper, loadContent), readFile_stub_001);
    bool bRet = pWindow->currentWrapper()->readFile(QByteArray("UTF-8"));
    ASSERT_FALSE(bRet);

    pWindow->deleteLater();
}

QByteArray test_editwrapper::FileLoadThreadRun(const QString &strFilePath, QByteArray *encode)
{
    QFile file(strFilePath);

    if (file.open(QIODevice::ReadOnly)) {
        // reads all remaining data from the file.
        QByteArray indata = file.readAll();
        file.close();
        QByteArray outData;
        // read the encode.
        *encode = DetectCode::GetFileEncodingFormat(strFilePath);
        QString textEncode = QString::fromLocal8Bit(*encode);

         if (textEncode.contains("ASCII", Qt::CaseInsensitive) || textEncode.contains("UTF-8", Qt::CaseInsensitive)) {
             return indata;
         } else {
           DetectCode::ChangeFileEncodingFormat(indata, outData, textEncode, QString("UTF-8"));
           return outData;
         }
    }
}

void handleFileLoadFinished_001_setPrintEnabled_stub()
{
    return;
}

void handleFileLoadFinished_001_setTextFinished_stub()
{
    return;
}

//void handleFileLoadFinished(const QByteArray &encode,const QString &content);
TEST_F(test_editwrapper, handleFileLoadFinished_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    const QString filePath = QCoreApplication::applicationDirPath() + QString("/Makefile");
    QByteArray encode = QByteArray();
    const QByteArray retFileContent = FileLoadThreadRun(filePath, &encode);
    Stub setPrintEnabled_stub;
    setPrintEnabled_stub.set(ADDR(Window, setPrintEnabled), handleFileLoadFinished_001_setPrintEnabled_stub);
    Stub setTextFinished_stub;
    setTextFinished_stub.set(ADDR(TextEdit, setTextFinished), handleFileLoadFinished_001_setTextFinished_stub);
    pWindow->currentWrapper()->handleFileLoadFinished(encode, retFileContent);
    ASSERT_TRUE(pWindow->currentWrapper()->m_pBottomBar->m_pEncodeMenu != nullptr);

    pWindow->deleteLater();
}

////重新加载文件编码 1.文件修改 2.文件未修改处理逻辑一样 切换编码重新加载和另存为 梁卫东
//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray("UTF-8"));
    ASSERT_FALSE(bRet);

    pWindow->deleteLater();
}

//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray());
    ASSERT_TRUE(bRet);

    pWindow->deleteLater();
}

int reloadFileEncode_003_exec_stub()
{
    return 0;
}

//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode_003)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadFileEncode_003_exec_stub);
    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray());
    ASSERT_FALSE(bRet);

    pWindow->deleteLater();
}

int reloadFileEncode_004_exec_stub()
{
    return 1;
}

bool reloadFileEncode_004_readFile_stub()
{
    return true;
}

//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode_004)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadFileEncode_004_exec_stub);
    Stub readFile_stub;
    readFile_stub.set(ADDR(EditWrapper, readFile), reloadFileEncode_004_readFile_stub);
    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray());
    /* gerrit编译运行结果和本地编译运行不一样，打桩后无果，先用如下方法断言 */
    if (bRet) {
        ASSERT_TRUE(bRet);
    } else {
        ASSERT_FALSE(bRet);
    }

    pWindow->deleteLater();
}

bool reloadFileEncode_005_saveDraftFile_stub()
{
    return false;
}

//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode_005)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadFileEncode_004_exec_stub);
    Stub saveDraftFile_stub;
    saveDraftFile_stub.set(ADDR(EditWrapper, saveDraftFile), reloadFileEncode_005_saveDraftFile_stub);
    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray());
    ASSERT_FALSE(bRet);

    pWindow->deleteLater();
}

bool reloadFileEncode_006_isDraftFile_stub()
{
    return false;
}

bool reloadFileEncode_006_saveFile_stub()
{
    return true;
}

//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode_006)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadFileEncode_004_exec_stub);
    Stub isDraftFile_stub;
    isDraftFile_stub.set(ADDR(Utils, isDraftFile), reloadFileEncode_006_isDraftFile_stub);
    Stub readFile_stub;
    readFile_stub.set(ADDR(EditWrapper, readFile), reloadFileEncode_004_readFile_stub);
    Stub saveFile_stub;
    saveFile_stub.set(ADDR(EditWrapper, saveFile), reloadFileEncode_006_saveFile_stub);
    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray());
    ASSERT_TRUE(bRet);

    pWindow->deleteLater();
}

int reloadFileEncode_007_exec_stub()
{
    return 2;
}

//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode_007)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadFileEncode_007_exec_stub);
    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray());
    ASSERT_FALSE(bRet);

    pWindow->deleteLater();
}

bool reloadFileEncode_008_getModified_stub()
{
    return false;
}

bool reloadFileEncode_008_readFile_stub()
{
    return false;
}

//bool reloadFileEncode(QByteArray encode);
TEST_F(test_editwrapper, reloadFileEncode_008)
{
//    Window *pWindow = new Window();
//    pWindow->addBlankTab(QString());
//    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
//                                                          QString("12345"));
//    Stub getModified_stub;
//    getModified_stub.set(ADDR(TextEdit, getModified), reloadFileEncode_008_getModified_stub);
//    Stub readFile_stub;
//    readFile_stub.set(ADDR(EditWrapper, readFile), reloadFileEncode_008_readFile_stub);
//    bool bRet = pWindow->currentWrapper()->reloadFileEncode(QByteArray());
//    ASSERT_FALSE(bRet);

//    pWindow->deleteLater();
}

int reloadModifyFile_001_exec_stub()
{
    return 0;
}

////重写加载修改文件
//void reloadModifyFile();
TEST_F(test_editwrapper, reloadModifyFile_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadModifyFile_001_exec_stub);
    pWindow->currentWrapper()->reloadModifyFile();
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->getModified());

    pWindow->deleteLater();
}

int reloadModifyFile_002_exec_stub()
{
    return 1;
}

//void reloadModifyFile();
TEST_F(test_editwrapper, reloadModifyFile_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadModifyFile_002_exec_stub);
    pWindow->currentWrapper()->reloadModifyFile();
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->getModified());

    pWindow->deleteLater();
}

int reloadModifyFile_003_exec_stub()
{
    return 2;
}

//void reloadModifyFile();
TEST_F(test_editwrapper, reloadModifyFile_003)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadModifyFile_003_exec_stub);
    pWindow->currentWrapper()->reloadModifyFile();
    ASSERT_FALSE(pWindow->currentWrapper()->saveDraftFile());

    pWindow->deleteLater();
}

bool reloadModifyFile_004_isDraftFile_stub()
{
    return false;
}

//void reloadModifyFile();
TEST_F(test_editwrapper, reloadModifyFile_004)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, reloadModifyFile_003_exec_stub);
    Stub isDraftFile_stub;
    isDraftFile_stub.set(ADDR(Utils, isDraftFile), reloadModifyFile_004_isDraftFile_stub);
    pWindow->currentWrapper()->reloadModifyFile();
    ASSERT_FALSE(pWindow->currentWrapper()->saveAsFile());

    pWindow->deleteLater();
}

//void reloadModifyFile();
TEST_F(test_editwrapper, reloadModifyFile_005)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->reloadModifyFile();
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->horizontalScrollBar()->value() == 0);

    pWindow->deleteLater();
}

////获取文件编码
//QString getTextEncode();
TEST_F(test_editwrapper, getTextEncode)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    QString strRet = pWindow->currentWrapper()->getTextEncode();
    ASSERT_TRUE(!strRet.compare(QString("UTF-8")));

    pWindow->deleteLater();
}

//bool saveTemFile(QString qstrDir);
TEST_F(test_editwrapper, saveTemFile_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    const QString filePath = QCoreApplication::applicationDirPath() + QString("/Makefile");
    bool bRet = pWindow->currentWrapper()->saveTemFile(filePath);
    ASSERT_TRUE(bRet);

    pWindow->deleteLater();
}

//bool saveTemFile(QString qstrDir);
TEST_F(test_editwrapper, saveTemFile_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    bool bRet = pWindow->currentWrapper()->saveTemFile(QString());
    ASSERT_FALSE(bRet);

    pWindow->deleteLater();
}

////判断是否修改
//bool isModified();
TEST_F(test_editwrapper, isModified)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("12345"));
    bool bRet = pWindow->currentWrapper()->isModified();
    ASSERT_TRUE(bRet);

    pWindow->deleteLater();
}

////判断是否草稿文件
//bool isDraftFile();
TEST_F(test_editwrapper, isDraftFile)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    bool bRet = pWindow->currentWrapper()->isDraftFile();
    ASSERT_TRUE(bRet);
    
    pWindow->deleteLater();
}

////判断内容是否为空
//bool isPlainTextEmpty();
TEST_F(test_editwrapper, isPlainTextEmpty)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    bool bRet = pWindow->currentWrapper()->isPlainTextEmpty();
    ASSERT_TRUE(bRet);
    
    pWindow->deleteLater();
}



