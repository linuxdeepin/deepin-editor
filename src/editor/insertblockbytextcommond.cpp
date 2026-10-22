// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
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
    qCDebug(appLog) << "InsertBlockByTextCommand created - text size:" << text.size()
             << "edit:" << edit << "wrapper:" << wrapper;
    if(nullptr == m_edit || m_text.isEmpty() || nullptr == m_wrapper)
        return;

    auto cursor = m_edit->textCursor();
    if(cursor.hasSelection()){
        m_selected = cursor.selectedText();
        m_insertPos = std::min(cursor.anchor(),cursor.position());
        qCDebug(appLog) << "Has selection - text:" << m_selected << "pos:" << m_insertPos;
    }
}

InsertBlockByTextCommand::~InsertBlockByTextCommand()
{
    qCDebug(appLog) << "InsertBlockByTextCommand destroyed";
}

void InsertBlockByTextCommand::redo()
{
    qCInfo(appLog) << "InsertBlockByTextCommand redo - inserting text block";
    treat(true);
    insertByBlock();
    treat(false);
    qCDebug(appLog) << "InsertBlockByTextCommand redo exit";
}

void InsertBlockByTextCommand::undo()
{
    qCInfo(appLog) << "InsertBlockByTextCommand undo - removing inserted text block";
    treat(true);

    auto cursor = m_edit->textCursor();
    cursor.setPosition(m_delPos);
    cursor.setPosition(m_delPos - m_text.size(),QTextCursor::KeepAnchor);
    cursor.deleteChar();

    if(!m_selected.isEmpty()){
        qCDebug(appLog) << "Restoring original selection - text:" << m_selected << "pos:" << m_insertPos;
        cursor.setPosition(m_insertPos);
        cursor.insertText(m_selected);
    }

    treat(false);
    qCDebug(appLog) << "InsertBlockByTextCommand undo exit";
}

void InsertBlockByTextCommand::treat(bool isStart)
{
    qCDebug(appLog) << "InsertBlockByTextCommand treat, isStart:" << isStart;
    if(m_wrapper!=nullptr){
        qCDebug(appLog) << "InsertBlockByTextCommand treat, m_wrapper not nullptr";
        Window* window = m_wrapper->window();
        BottomBar* bar = m_wrapper->bottomBar();
        if(window){
            qCDebug(appLog) << "InsertBlockByTextCommand treat, window not nullptr";
            window->setPrintEnabled(!isStart);
        }
        if(bar){
            qCDebug(appLog) << "InsertBlockByTextCommand treat, bar not nullptr";
            bar->setChildEnabled(!isStart);
        }
    }

    if(!isStart) {
        qCDebug(appLog) << "InsertBlockByTextCommand treat, connect cursorPositionChanged";
        QObject::connect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
    } else {
        qCDebug(appLog) << "InsertBlockByTextCommand treat, disconnect cursorPositionChanged";
        QObject::disconnect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
    }
    qCDebug(appLog) << "InsertBlockByTextCommand treat exit";
}

void InsertBlockByTextCommand::insertByBlock()
{
    qCDebug(appLog) << "InsertBlockByTextCommand insertByBlock";
    auto cursor = m_edit->textCursor();
    int block = 1 * 1024 * 1024 ;
    int size = m_text.size();
    qCDebug(appLog) << "Inserting text by blocks - total size:" << size << "block size:" << block;
    if(size > block){
        qCDebug(appLog) << "InsertBlockByTextCommand insertByBlock, size > block";
        int n = size / (block);
        int y = size % (block);
        int k=0;
        for(;k<n;k++){
            if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
                QString insertText = m_text.mid(k*block,block);
                qCDebug(appLog) << "Inserting block" << k+1 << "/" << n << "size:" << insertText.size();
                cursor.insertText(insertText);
                QApplication::processEvents();
            }
        }
        if(y){
            qCDebug(appLog) << "InsertBlockByTextCommand insertByBlock, y:" << y;
            if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
                QString insertText = m_text.mid(k*block,y);
                qCDebug(appLog) << "Inserting final block size:" << y;
                cursor.insertText(insertText);
                QApplication::processEvents();
            }
        }
    }
    m_delPos = cursor.position();
    qCDebug(appLog) << "Insert completed - delete position:" << m_delPos;
}
