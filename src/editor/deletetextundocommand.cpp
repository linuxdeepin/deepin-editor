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

#include "deletetextundocommand.h"
#include "editwrapper.h"
#include <QDebug>
#include <QTextBlock>

DeleteTextUndoCommand::DeleteTextUndoCommand(QTextCursor textcursor, QPlainTextEdit* edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_edit(edit)
    , m_textCursor(textcursor)
{
    if(m_textCursor.hasSelection()){
        m_sInsertText = m_textCursor.selectedText();
        //qDebug()<<m_sInsertText;
    }else{
        int pos = m_textCursor.positionInBlock() -1;
        if(pos >= 0){
            m_sInsertText = m_textCursor.block().text().at(pos);
        }else {
            //上一行lastQChar
            m_sInsertText = "\n";
        }
    }
}

DeleteTextUndoCommand::DeleteTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections, QPlainTextEdit *edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_edit(edit)
    , m_ColumnEditSelections(selections)
{
    int cnt = m_ColumnEditSelections.size();
    for (int i = 0; i < cnt; i++) {
        QTextCursor textCursor =m_ColumnEditSelections[i].cursor;
        if(textCursor.hasSelection())
        {
            m_selectTextList.append(textCursor.selectedText());
        }else {
            int pos = textCursor.positionInBlock()-1;
            if(pos >= 0){
                m_selectTextList.append(textCursor.block().text().at(pos));
            }else {
                //上一行lastQChar
                m_selectTextList.append("\n");
            }
        }
    }
}


void DeleteTextUndoCommand::undo()
{
    if(m_ColumnEditSelections.isEmpty()){
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor,m_sInsertText.length());

        // 进行撤销/恢复时将光标移动到撤销位置
        if (m_edit) {
            m_edit->setTextCursor(m_textCursor);
        }
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.insertText(m_selectTextList[i]);
            m_ColumnEditSelections[i].cursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor,m_selectTextList[i].length());
        }

        if (m_edit && !m_ColumnEditSelections.isEmpty()) {
            m_edit->setTextCursor(m_ColumnEditSelections.last().cursor);
        }
    }

}


void DeleteTextUndoCommand::redo()
{
    if (m_ColumnEditSelections.isEmpty()) {
        m_textCursor.deletePreviousChar();

        // 进行撤销/恢复时将光标移动到撤销位置
        if (m_edit) {
            m_edit->setTextCursor(m_textCursor);
        }
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.deletePreviousChar();
        }

        if (m_edit && !m_ColumnEditSelections.isEmpty()) {
            m_edit->setTextCursor(m_ColumnEditSelections.last().cursor);
        }
    }
}


DeleteTextUndoCommand2::DeleteTextUndoCommand2(QTextCursor textcursor,QString text,QPlainTextEdit* edit,bool currLine):
    m_textCursor(textcursor),
    m_sInsertText(text),
    m_edit(edit),
    m_iscurrLine(currLine)
{

}

DeleteTextUndoCommand2::DeleteTextUndoCommand2(QList<QTextEdit::ExtraSelection> &selections,QString text,QPlainTextEdit* edit,bool currLine):
    m_sInsertText(text),
    m_ColumnEditSelections(selections),
    m_edit(edit),
    m_iscurrLine(currLine)
{
    int cnt = m_ColumnEditSelections.size();
    for (int i = 0; i < cnt; i++) {
        QTextCursor textCursor =m_ColumnEditSelections[i].cursor;
        if(textCursor.hasSelection())
        {
            m_selectTextList.append(textCursor.selectedText());
        }else {
            int pos = textCursor.positionInBlock()-1;
            if(pos >= 0){
                m_selectTextList.append(textCursor.block().text().at(pos));
            }else {
                //上一行lastQChar
                m_selectTextList.append("\n");
            }
        }
    }
}

void DeleteTextUndoCommand2::undo()
{
    if(m_ColumnEditSelections.isEmpty()){
        m_textCursor.setPosition(m_beginPostion);
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.setPosition(m_beginPostion);
        m_edit->setTextCursor(m_textCursor);
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.setPosition(m_beginPostion);
            m_ColumnEditSelections[i].cursor.insertText(m_selectTextList[i]);
            m_ColumnEditSelections[i].cursor.setPosition(m_beginPostion);
            m_edit->setTextCursor(m_ColumnEditSelections[i].cursor);
        }
    }
}

void DeleteTextUndoCommand2::redo()
{
    if(m_ColumnEditSelections.isEmpty()){

        bool isEmptyLine = (m_sInsertText.size() == 0);
        bool isBlankLine = (m_sInsertText.trimmed().size() == 0);

        if(!m_iscurrLine) {
            //删除到行尾
            if (isEmptyLine || m_textCursor.atBlockEnd()) {
                m_textCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            }
            else if (isBlankLine && m_textCursor.atBlockStart()) {
                m_textCursor.movePosition(QTextCursor::StartOfBlock);
                m_textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            }
            else {
                m_textCursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
                m_textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            }
        }
        else {
            //删除整行
            m_textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
            m_textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            m_textCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        }

        m_sInsertText = m_textCursor.selectedText();
        m_beginPostion = m_textCursor.selectionStart();
        m_textCursor.deletePreviousChar();

        // 进行撤销/恢复时将光标移动到撤销位置
        m_edit->setTextCursor(m_textCursor);
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_beginPostion = m_ColumnEditSelections[i].cursor.selectionStart();
            m_ColumnEditSelections[i].cursor.deletePreviousChar();
            m_edit->setTextCursor(m_ColumnEditSelections[i].cursor);
        }
    }
}
