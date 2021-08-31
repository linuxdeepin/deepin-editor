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
#include "test_editwrapper.h"

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
    EditWrapper *wrapper = new EditWrapper();
    
}

//void clearAllFocus() 无实现;

//void setQuitFlag();
TEST_F(test_editwrapper, setQuitFlag)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setQuitFlag();
    
}

//bool getFileLoading();
TEST_F(test_editwrapper, getFileLoading)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->getFileLoading();
    
}

//void openFile(const QString &filepath,QString qstrTruePath,bool bIsTemFile = false);
TEST_F(test_editwrapper, openFile)
{
    EditWrapper *wrapper = new EditWrapper();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->openFile("1.cpp","1.cpp");
    
}

//bool saveFile();
TEST_F(test_editwrapper, saveFile)
{
    //EditWrapper *wrapper = new EditWrapper();
   // wrapper->saveFile();
    
}

//bool saveAsFile(const QString &newFilePath, QByteArray encodeName);
TEST_F(test_editwrapper, saveAsFile)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->saveAsFile("1.cpp","UTF-8");
    
}

//void updatePath(const QString &file);
TEST_F(test_editwrapper, updatePath)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->updatePath("1.cpp");
    
}




//void hideWarningNotices();
TEST_F(test_editwrapper, hideWarningNotices)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->hideWarningNotices();
    
}

//void checkForReload();
TEST_F(test_editwrapper, checkForReload)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->checkForReload();
    
}

//void initToastPosition() 无实现;

//void showNotify(const QString &message);
TEST_F(test_editwrapper, showNotify)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->showNotify("aa");
    
}


//void setLineNumberShow(bool bIsShow,bool bIsFirstShow = false);
TEST_F(test_editwrapper, setLineNumberShow)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setLineNumberShow(true);
    
}

//void setShowBlankCharacter(bool ok);
TEST_F(test_editwrapper, setShowBlankCharacter)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setShowBlankCharacter(true);
    
}

//BottomBar *bottomBar();
TEST_F(test_editwrapper, bottomBar)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->bottomBar();
    
}

//QString filePath();
TEST_F(test_editwrapper, filePath)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->filePath();
    
}

//TextEdit *textEditor() { return m_textEdit; }
TEST_F(test_editwrapper, textEditor)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->textEditor();
    
}

//private:


//int GetCorrectUnicode1(const QByteArray &ba) 未实现;

//bool saveDraftFile();
TEST_F(test_editwrapper, saveDraftFile)
{
    EditWrapper *wrapper = new EditWrapper();
    //wrapper->saveDraftFile();
    
}

//void readFile(const QString &filePath);
TEST_F(test_editwrapper, readFile)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->readFile("aa");
    
}



//void handleFileLoadFinished(const QByteArray &encode,const QString &content);
TEST_F(test_editwrapper, handleFileLoadFinished)
{
    Window* pWindow = new Window;
    EditWrapper *wrapper = pWindow->createEditor();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->handleFileLoadFinished("UTF-8","aa");
    
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



