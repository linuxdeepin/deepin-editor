// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "indenttextcommond.h"
#include "dtextedit.h"
#include <QDebug>

IndentTextCommand::IndentTextCommand(TextEdit* edit,int startpos,int endpos,int startline,int endline):
    m_edit(edit),
    m_startpos(startpos),
    m_endpos(endpos),
    m_startline(startline),
    m_endline(endline)
{
    qDebug() << "IndentTextCommand created - startpos:" << startpos
             << "endpos:" << endpos << "lines:" << startline << "-" << endline
             << "hasSelection:" << m_edit->textCursor().hasSelection();
    m_hasselected = m_edit->textCursor().hasSelection();
}
IndentTextCommand::~IndentTextCommand()
{
    qDebug() << "IndentTextCommand destroyed";
}

void IndentTextCommand::redo()
{
    qInfo() << "IndentTextCommand redo - adding indents to lines:"
            << m_startline << "-" << m_endline;
    auto cursor = m_edit->textCursor();

    //insert "\t" in front of multiple lines.
    cursor.setPosition(m_startpos);
    for(int i=m_startline;i<=m_endline;i++){
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.insertText("\t");

        cursor.movePosition(QTextCursor::NextBlock);
    }

    //reset selection.
    if(m_hasselected){
        qDebug() << "IndentTextCommand redo, m_hasselected";
        if(m_startline == m_endline){
            qDebug() << "IndentTextCommand redo, m_startline == m_endline";
            cursor.setPosition(m_startpos);
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
            m_edit->setTextCursor(cursor);
        }
        else{
            qDebug() << "IndentTextCommand redo, m_startline != m_endline";
            cursor.setPosition(m_startpos+1);
            cursor.setPosition(m_endpos + m_endline - m_startline +1,QTextCursor::KeepAnchor);
            m_edit->setTextCursor(cursor);
        }

    }
    qDebug() << "IndentTextCommand redo exit";
}



void IndentTextCommand::undo()
{
    qInfo() << "IndentTextCommand undo - removing indents from lines:"
            << m_startline << "-" << m_endline;
    auto cursor = m_edit->textCursor();

    //delete "\t" in front of multiple lines.
    cursor.setPosition(m_startpos);
    for(int i=m_startline;i<=m_endline;i++){
        qDebug() << "IndentTextCommand undo, i:" << i;
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.deleteChar();

        cursor.movePosition(QTextCursor::NextBlock);
    }

    //reset selection
    if(m_hasselected){
        qDebug() << "IndentTextCommand undo, m_hasselected";
        cursor.setPosition(m_startpos);
        cursor.setPosition(m_endpos,QTextCursor::KeepAnchor);
        m_edit->setTextCursor(cursor);
    }
    qDebug() << "IndentTextCommand undo exit";
}

UnindentTextCommand::UnindentTextCommand(TextEdit* edit,int startpos,int endpos,int startline,int endline,int tabSpaceNumber):
    m_edit(edit),
    m_startpos(startpos),
    m_endpos(endpos),
    m_startline(startline),
    m_endline(endline),
    m_tabSpaceNumber(tabSpaceNumber)
{
    qDebug() << "UnindentTextCommand created - startpos:" << startpos
             << "endpos:" << endpos << "lines:" << startline << "-" << endline
             << "hasSelection:" << m_edit->textCursor().hasSelection();
    m_hasselected = m_edit->textCursor().hasSelection();
    m_removedChars.resize(endline - startline + 1);
}

UnindentTextCommand::~UnindentTextCommand()
{
    qDebug() << "UnindentTextCommand destroyed";
}

void UnindentTextCommand::redo()
{
    qInfo() << "UnindentTextCommand redo - removing indents from lines:"
            << m_startline << "-" << m_endline;
    auto cursor = m_edit->textCursor();

    //remove indentation from multiple lines.
    cursor.setPosition(m_startpos);
    int totalRemoved = 0;

    for(int i=m_startline;i<=m_endline;i++){
        cursor.movePosition(QTextCursor::StartOfBlock);
        int lineStart = cursor.position();
        int removed = 0;

        //check if line starts with tab
        if(m_edit->document()->characterAt(lineStart) == '\t'){
            cursor.deleteChar();
            removed = 1;
        }
        //check if line starts with spaces
        else if(m_edit->document()->characterAt(lineStart) == ' '){
            int cnt = 0;
            int pos = lineStart;
            while(m_edit->document()->characterAt(pos) == ' ' && cnt < m_tabSpaceNumber){
                pos++;
                cnt++;
            }
            if(cnt > 0){
                cursor.setPosition(lineStart);
                cursor.setPosition(pos, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                removed = cnt;
            }
        }

        m_removedChars[i - m_startline] = removed;
        totalRemoved += removed;
        cursor.movePosition(QTextCursor::NextBlock);
    }

    //reset selection.
    if(m_hasselected){
        qDebug() << "UnindentTextCommand redo, m_hasselected, totalRemoved:" << totalRemoved;
        if(m_startline == m_endline){
            qDebug() << "UnindentTextCommand redo, m_startline == m_endline";
            int newStartPos = m_startpos - m_removedChars[0];
            if(newStartPos < 0) newStartPos = 0;
            cursor.setPosition(newStartPos);
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
            m_edit->setTextCursor(cursor);
        }
        else{
            qDebug() << "UnindentTextCommand redo, m_startline != m_endline";
            int firstLineRemoved = m_removedChars[0];
            int newStartPos = m_startpos - firstLineRemoved;
            int newEndPos = m_endpos - totalRemoved;

            //ensure positions are valid
            if(newStartPos < 0) newStartPos = 0;
            if(newEndPos < newStartPos) newEndPos = newStartPos;

            cursor.setPosition(newStartPos);
            cursor.setPosition(newEndPos, QTextCursor::KeepAnchor);
            m_edit->setTextCursor(cursor);
        }
    }
    qDebug() << "UnindentTextCommand redo exit";
}

void UnindentTextCommand::undo()
{
    qInfo() << "UnindentTextCommand undo - restoring indents to lines:"
            << m_startline << "-" << m_endline;
    auto cursor = m_edit->textCursor();

    //restore indentation to multiple lines.
    cursor.setPosition(m_startpos);
    for(int i=m_startline;i<=m_endline;i++){
        qDebug() << "UnindentTextCommand undo, i:" << i;
        cursor.movePosition(QTextCursor::StartOfBlock);
        int removed = m_removedChars[i - m_startline];

        if(removed == 1){
            //restore tab
            cursor.insertText("\t");
        }
        else if(removed > 1){
            //restore spaces
            cursor.insertText(QString(removed, ' '));
        }

        cursor.movePosition(QTextCursor::NextBlock);
    }

    //reset selection
    if(m_hasselected){
        qDebug() << "UnindentTextCommand undo, m_hasselected";
        cursor.setPosition(m_startpos);
        cursor.setPosition(m_endpos, QTextCursor::KeepAnchor);
        m_edit->setTextCursor(cursor);
    }
    qDebug() << "UnindentTextCommand undo exit";
}
