// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "insertblockbytextcommond.h"
#include "inserttextundocommand.h"
#include <QApplication>
#include "dtextedit.h"
#include "editwrapper.h"
#include "../widgets/window.h"
#include "../widgets/bottombar.h"
#include <QDebug>

InsertBlockByTextCommand::InsertBlockByTextCommand(const QString &text,TextEdit *edit,EditWrapper* wrapper):
    m_text(text),
    m_edit(edit),
    m_wrapper(wrapper)
{
    qDebug() << "InsertBlockByTextCommand created - text size:" << text.size()
             << "edit:" << edit << "wrapper:" << wrapper;
    if(nullptr == m_edit || m_text.isEmpty() || nullptr == m_wrapper)
        return;

    auto cursor = m_edit->textCursor();
    if(cursor.hasSelection()){
        m_selected = cursor.selectedText();
        m_insertPos = std::min(cursor.anchor(),cursor.position());
        qDebug() << "Has selection - text:" << m_selected << "pos:" << m_insertPos;
    }
}

InsertBlockByTextCommand::~InsertBlockByTextCommand()
{
    qDebug() << "InsertBlockByTextCommand destroyed";
}

void InsertBlockByTextCommand::redo()
{
    qInfo() << "InsertBlockByTextCommand redo - inserting text block";
    treat(true);
    insertByBlock();
    treat(false);
}

void InsertBlockByTextCommand::undo()
{
    qInfo() << "InsertBlockByTextCommand undo - removing inserted text block";
    treat(true);

    auto cursor = m_edit->textCursor();
    cursor.setPosition(m_delPos);
    cursor.setPosition(m_delPos - m_text.size(),QTextCursor::KeepAnchor);
    cursor.deleteChar();

    if(!m_selected.isEmpty()){
        qDebug() << "Restoring original selection - text:" << m_selected << "pos:" << m_insertPos;
        cursor.setPosition(m_insertPos);
        cursor.insertText(m_selected);
    }

    treat(false);
}

void InsertBlockByTextCommand::treat(bool isStart)
{
    if(m_wrapper!=nullptr){

        Window* window = m_wrapper->window();
        BottomBar* bar = m_wrapper->bottomBar();
        if(window){
            window->setPrintEnabled(!isStart);
        }
        if(bar){
            bar->setChildEnabled(!isStart);
        }
    }

    if(!isStart)
        QObject::connect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
    else
        QObject::disconnect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
}

void InsertBlockByTextCommand::insertByBlock()
{
    auto cursor = m_edit->textCursor();
    int block = 1 * 1024 * 1024 ;
    int size = m_text.size();
    qDebug() << "Inserting text by blocks - total size:" << size << "block size:" << block;
    if(size > block){
        int n = size / (block);
        int y = size % (block);
        int k=0;
        for(;k<n;k++){
            if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
                QString insertText = m_text.mid(k*block,block);
                qDebug() << "Inserting block" << k+1 << "/" << n << "size:" << insertText.size();
                cursor.insertText(insertText);
                QApplication::processEvents();
            }
        }
        if(y){
            if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
                QString insertText = m_text.mid(k*block,y);
                qDebug() << "Inserting final block size:" << y;
                cursor.insertText(insertText);
                QApplication::processEvents();
            }
        }
    }
    m_delPos = cursor.position();
    qDebug() << "Insert completed - delete position:" << m_delPos;
}
