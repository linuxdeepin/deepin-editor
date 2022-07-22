/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     liangweidong <liangweidong@uniontech.com>
*
* Maintainer: liangweidong <liangweidong@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DELETETEXTUNDOCOMMAND_H
#define DELETETEXTUNDOCOMMAND_H

#include <QObject>
#include <QUndoCommand>
#include <QTextCursor>
#include <QTextEdit>
#include <qplaintextedit.h>

class DeleteTextUndoCommand : public QUndoCommand
{
public:
    explicit DeleteTextUndoCommand(QTextCursor textcursor, QUndoCommand *parent = nullptr);
    explicit DeleteTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections, QUndoCommand *parent = nullptr);
    virtual void undo();
    virtual void redo();


private:
    QTextCursor m_textCursor;
    QString m_sInsertText;
    QList<QString> m_selectTextList;
    QList<QTextEdit::ExtraSelection> m_ColumnEditSelections;
};

//重写ctrl + k 和Ctrl +shift +K 逻辑的删除和撤销功能 ut002764
class DeleteTextUndoCommand2 : public QUndoCommand
{
public:
    explicit DeleteTextUndoCommand2(QTextCursor textcursor,QString text,QPlainTextEdit* edit,bool currLine);
    explicit DeleteTextUndoCommand2(QList<QTextEdit::ExtraSelection> &selections,QString text,QPlainTextEdit* edit,bool m_iscurrLine);
    virtual void undo();
    virtual void redo();

private:
    QTextCursor m_textCursor;
    QString m_sInsertText;
    QList<QString> m_selectTextList;
    QList<QTextEdit::ExtraSelection> m_ColumnEditSelections;
    QPlainTextEdit* m_edit;
    int m_beginPostion {0};
    bool m_iscurrLine {false};
};

#endif // INSERTTEXTUNDOCOMMAND_H
