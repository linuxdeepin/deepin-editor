#include "replaceallcommond.h"



ReplaceAllCommond::ReplaceAllCommond(QString& oldText, QString& newText, QTextCursor cursor):
    m_oldText(oldText),
    m_newText(newText),
    m_cursor(cursor)
{

}
ReplaceAllCommond::~ReplaceAllCommond()
{

}

void ReplaceAllCommond::redo()
{

    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_newText);
}

void ReplaceAllCommond::undo()
{
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_oldText);
}
