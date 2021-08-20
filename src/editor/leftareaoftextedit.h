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
#ifndef LEFTAREAOFTEXTEDIT_H
#define LEFTAREAOFTEXTEDIT_H

#include <QWidget>

class CodeFlodArea;
class BookMarkWidget;
class LineNumberArea;
class TextEdit;

class LeftAreaTextEdit : public QWidget
{
    Q_OBJECT
public:
    explicit LeftAreaTextEdit(TextEdit *textEdit);
    ~LeftAreaTextEdit() override;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void bookMarkAreaPaintEvent(QPaintEvent *event);
    void codeFlodAreaPaintEvent(QPaintEvent *event);
    void updateLineNumber();
    void updateBookMark();
    void updateCodeFlod();
    void updateAll();
protected:
    void paintEvent(QPaintEvent *event);
public:
    LineNumberArea *m_pLineNumberArea = nullptr;
    BookMarkWidget *m_pBookMarkArea = nullptr;
    CodeFlodArea *m_pFlodArea = nullptr;

private:
    TextEdit *m_pTextEdit = nullptr;
};

#endif // LEFTAREAOFTEXTEDIT_H
