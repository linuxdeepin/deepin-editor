#include "insertblockbytextcommond.h"
#include "inserttextundocommand.h"
#include <QApplication>

InsertBlockByTextCommond::InsertBlockByTextCommond(const QString &text,QPlainTextEdit *edit):
    m_text(text),
    m_edit(edit)
{
    if(nullptr == m_edit || m_text.isEmpty())
        return;

    auto cursor = m_edit->textCursor();
    int block = 1 * 1024 * 1024 ;
    int size = m_text.size();
    if(size > block){
        int n = size / (block);
        int y = size % (block);
        int k=0;
        for(;k<n;k++){
            QString insertText = m_text.mid(k*block,block);
            InsertTextUndoCommand* commond = new InsertTextUndoCommand(cursor,insertText);
            m_commondList.push_back(commond);
            cursor.setPosition(cursor.position() + block);
        }
        if(y){
            QString insertText = m_text.mid(k*block,y);
            InsertTextUndoCommand* commond = new InsertTextUndoCommand(cursor,insertText);
            m_commondList.push_back(commond);
            cursor.setPosition(cursor.position() + y);
        }
    }
}

InsertBlockByTextCommond::~InsertBlockByTextCommond()
{

    for(auto it = m_commondList.begin();it != m_commondList.end(); it++){
        if(*it){
            delete (*it);
            (*it) = nullptr;
        }
    }

    m_commondList.clear();
}

void InsertBlockByTextCommond::redo()
{
    for(auto it = m_commondList.begin();it != m_commondList.end(); it++){
        if(m_edit->isVisible() && (*it) != nullptr){
            (*it)->redo();
            QApplication::processEvents();
        }
    }
}

void InsertBlockByTextCommond::undo()
{
    for(auto it = m_commondList.rbegin();it != m_commondList.rend(); it++){
        if(m_edit->isVisible() && (*it) != nullptr){
            (*it)->undo();
            QApplication::processEvents();
        }
    }
}
