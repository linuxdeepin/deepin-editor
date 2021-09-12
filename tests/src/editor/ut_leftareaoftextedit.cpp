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
#include "ut_leftareaoftextedit.h"
#include "../../src/editor/leftareaoftextedit.h"
#include "../../src/editor/dtextedit.h"

test_leftareaoftextedit::test_leftareaoftextedit()
{

}

//LeftAreaTextEdit(TextEdit *textEdit);
TEST_F(test_leftareaoftextedit, LeftAreaTextEdit)
{
    TextEdit *textEdit = new TextEdit;
    LeftAreaTextEdit *leftArea = new LeftAreaTextEdit(textEdit);
    
    ASSERT_TRUE(leftArea->m_pFlodArea != nullptr);
    leftArea->deleteLater();
    textEdit->deleteLater();
}

//void lineNumberAreaPaintEvent(QPaintEvent *event);
TEST_F(test_leftareaoftextedit, lineNumberAreaPaintEvent)
{
    TextEdit *textEdit = new TextEdit;
    LeftAreaTextEdit *leftArea = new LeftAreaTextEdit(textEdit);
    QPaintEvent *event = new QPaintEvent(QRegion());
    leftArea->lineNumberAreaPaintEvent(event);
    
    ASSERT_TRUE(leftArea->m_pFlodArea != nullptr);
    delete event;
    event = nullptr;
    leftArea->deleteLater();
    textEdit->deleteLater();
}

//int lineNumberAreaWidth();
TEST_F(test_leftareaoftextedit, lineNumberAreaWidth)
{
    TextEdit *textEdit = new TextEdit;
    LeftAreaTextEdit *leftArea = new LeftAreaTextEdit(textEdit);
    leftArea->lineNumberAreaWidth();
    
    ASSERT_TRUE(leftArea->m_pFlodArea != nullptr);
    leftArea->deleteLater();
    textEdit->deleteLater();
}

//void bookMarkAreaPaintEvent(QPaintEvent *event);
TEST_F(test_leftareaoftextedit, bookMarkAreaPaintEvent)
{
    TextEdit *textEdit = new TextEdit;
    LeftAreaTextEdit *leftArea = new LeftAreaTextEdit(textEdit);
    QPaintEvent *event = new QPaintEvent(QRegion());
    leftArea->bookMarkAreaPaintEvent(event);
    
    ASSERT_TRUE(leftArea->m_pFlodArea != nullptr);
    delete event;
    event = nullptr;
    leftArea->deleteLater();
    textEdit->deleteLater();
}

//void codeFlodAreaPaintEvent(QPaintEvent *event);
TEST_F(test_leftareaoftextedit, codeFlodAreaPaintEvent)
{
    TextEdit *textEdit = new TextEdit;
    LeftAreaTextEdit *leftArea = new LeftAreaTextEdit(textEdit);
    QPaintEvent *event = new QPaintEvent(QRegion());
    leftArea->codeFlodAreaPaintEvent(event);
    
    ASSERT_TRUE(leftArea->m_pFlodArea != nullptr);
    delete event;
    event = nullptr;
    leftArea->deleteLater();
    textEdit->deleteLater();
}
