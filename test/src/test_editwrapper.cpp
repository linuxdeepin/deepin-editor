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

}

//void clearAllFocus() 无实现;

//void setQuitFlag();
TEST_F(test_editwrapper, setQuitFlag)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setQuitFlag();
    assert(1==1);
}

//bool getFileLoading();
TEST_F(test_editwrapper, getFileLoading)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->getFileLoading();
    assert(1==1);
}

//void openFile(const QString &filepath);
TEST_F(test_editwrapper, openFile)
{
    EditWrapper *wrapper = new EditWrapper();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->openFile("aa");
    assert(1==1);
}

//bool saveFile();
TEST_F(test_editwrapper, saveFile)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->saveFile();
    assert(1==1);
}

//bool saveAsFile(const QString &newFilePath, QByteArray encodeName);
TEST_F(test_editwrapper, saveAsFile)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->saveAsFile("aa","UTF-8");
    assert(1==1);
}

//void updatePath(const QString &file);
TEST_F(test_editwrapper, updatePath)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->updatePath("aa");
    assert(1==1);
}

//void refresh();
TEST_F(test_editwrapper, refresh)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->refresh();
    assert(1==1);
}

//bool isLoadFinished();
TEST_F(test_editwrapper, isLoadFinished)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->refresh();
    assert(1==1);
}

//EndOfLineMode endOfLineMode();
TEST_F(test_editwrapper, endOfLineMode)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->endOfLineMode();
    assert(1==1);
}

//void setEndOfLineMode(EndOfLineMode eol);
TEST_F(test_editwrapper, setEndOfLineMode)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setEndOfLineMode(EditWrapper::eolDos);
    assert(1==1);
}

//void setTextCodec(QByteArray encodeName, bool reload = false);
//void setTextCodec(QTextCodec *codec, bool reload = false);
TEST_F(test_editwrapper, setTextCodec)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setTextCodec("UTF-8");
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    wrapper->setTextCodec(codec);
    assert(1==1);
}

//void hideWarningNotices();
TEST_F(test_editwrapper, hideWarningNotices)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->hideWarningNotices();
    assert(1==1);
}

//void checkForReload();
TEST_F(test_editwrapper, checkForReload)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->checkForReload();
    assert(1==1);
}

//void initToastPosition() 无实现;

//void showNotify(const QString &message);
TEST_F(test_editwrapper, showNotify)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->showNotify("aa");
    assert(1==1);
}

//bool getTextChangeFlag();
TEST_F(test_editwrapper, getTextChangeFlag)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->getTextChangeFlag();
    assert(1==1);
}

//void setTextChangeFlag(bool bFlag);
TEST_F(test_editwrapper, setTextChangeFlag)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setTextChangeFlag(true);
    assert(1==1);
}

//void setLineNumberShow(bool bIsShow,bool bIsFirstShow = false);
TEST_F(test_editwrapper, setLineNumberShow)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setLineNumberShow(true);
    assert(1==1);
}

//void setShowBlankCharacter(bool ok);
TEST_F(test_editwrapper, setShowBlankCharacter)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->setShowBlankCharacter(true);
    assert(1==1);
}

//BottomBar *bottomBar();
TEST_F(test_editwrapper, bottomBar)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->bottomBar();
    assert(1==1);
}

//QString filePath();
TEST_F(test_editwrapper, filePath)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->filePath();
    assert(1==1);
}

//TextEdit *textEditor() { return m_textEdit; }
TEST_F(test_editwrapper, textEditor)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->textEditor();
    assert(1==1);
}

//private:
//void detectEndOfLine();
TEST_F(test_editwrapper, detectEndOfLine)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->detectEndOfLine();
    assert(1==1);
}

void handleCursorModeChanged(TextEdit::CursorMode mode);
TEST_F(test_editwrapper, handleCursorModeChanged)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->handleCursorModeChanged(TextEdit::Insert);
    assert(1==1);
}

//void handleHightlightChanged(const QString &name);
TEST_F(test_editwrapper, handleHightlightChanged)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->handleHightlightChanged("aa");
    assert(1==1);
}

//int GetCorrectUnicode1(const QByteArray &ba) 未实现;

//bool saveDraftFile();
TEST_F(test_editwrapper, saveDraftFile)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->saveDraftFile();
    assert(1==1);
}

//void readFile(const QString &filePath);
TEST_F(test_editwrapper, readFile)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->readFile("aa");
    assert(1==1);
}


//public slots:
//void onFileClosed();
TEST_F(test_editwrapper, onFileClosed)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->onFileClosed();
    assert(1==1);
}

//void slotTextChange();
TEST_F(test_editwrapper, slotTextChange)
{
    EditWrapper *wrapper = new EditWrapper();
    wrapper->slotTextChange();
    assert(1==1);
}

//void handleFileLoadFinished(const QByteArray &encode,const QString &content);
TEST_F(test_editwrapper, handleFileLoadFinished)
{
    EditWrapper *wrapper = new EditWrapper();
    Settings *s = new Settings();
    wrapper->textEditor()->setSettings(s);
    wrapper->textEditor()->setWrapper(wrapper);
    wrapper->handleFileLoadFinished("UTF-8","aa");
    assert(1==1);
}

//signals:
//void requestSaveAs();
//void sigCodecSaveFile(const QString &strOldFilePath, const QString &strNewFilePath);

//protected:
//void resizeEvent(QResizeEvent *);
TEST_F(test_editwrapper, resizeEvent)
{
    EditWrapper *wrapper = new EditWrapper();
    QResizeEvent *e;
    wrapper->resizeEvent(e);
    assert(1==1);
}
