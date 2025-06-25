// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
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
