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

class bookmarkwidget;
class LineNumberArea;
class TextEdit;
class leftareaoftextedit : public QWidget
{
    Q_OBJECT
public:
    leftareaoftextedit(TextEdit *textEdit);
    ~leftareaoftextedit() override;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
//    LineNumberArea *getLineNumberArea();
//    bookmarkwidget *getBookMarkWidget();

    void bookMarkAreaPaintEvent(QPaintEvent *event);
public:
    LineNumberArea *m_linenumberarea;
    bookmarkwidget *m_bookMarkArea;
    QWidget *m_flodArea;

protected:
//    void paintEvent(QPaintEvent *e) override;
//    void mousePressEvent(QMouseEvent *event) override;
//    QSize sizeHint() const override;
private:
    TextEdit *m_textEdit;
};

#endif // LEFTAREAOFTEXTEDIT_H
