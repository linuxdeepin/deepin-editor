#include "deletebackcommond.h"
#include <QTextBlock>

DeleteBackCommond::DeleteBackCommond(QTextCursor cursor, QPlainTextEdit *edit):
    m_cursor(cursor),
    m_edit(edit)
{
    m_delText = m_cursor.selectedText();
    m_delPos = std::min(m_cursor.position(), m_cursor.anchor());
    m_insertPos = m_delPos;
}

DeleteBackCommond::~DeleteBackCommond()
{

}

void DeleteBackCommond::undo()
{
    m_cursor.setPosition(m_insertPos);
    m_cursor.insertText(m_delText);
    m_edit->setTextCursor(m_cursor);
}

void DeleteBackCommond::redo()
{
    m_cursor.setPosition(m_delPos);
    m_cursor.setPosition(m_delPos+m_delText.size(), QTextCursor::KeepAnchor);
    m_cursor.deleteChar();
}

DeleteBackAltCommond::DeleteBackAltCommond(QList<QTextEdit::ExtraSelection> &selections,QPlainTextEdit* edit):
    m_ColumnEditSelections(selections),
    m_edit(edit)
{
    int size = m_ColumnEditSelections.size();
    int sum=0;
    for(int i=0;i<size;i++){
        QString text;
        auto cursor = m_ColumnEditSelections[i].cursor;

        if(!cursor.hasSelection() && !cursor.atBlockEnd()){
            cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,1);
        }

        text = cursor.selectedText();
        if(!text.isEmpty()){
            int pos = std::min(cursor.anchor(),cursor.position());
            DelNode node;
            node.m_cursor = cursor;
            node.m_insertPos = pos;
            node.m_delPos = pos - sum;
            node.m_delText = text;
            node.m_id_in_Column = i;
            m_deletions.push_back(node);

            sum += text.size();
        }
    }
}

DeleteBackAltCommond::~DeleteBackAltCommond()
{

}

void DeleteBackAltCommond::undo()
{

    int size = m_deletions.size();
    for(int i=0;i<size;i++){
        auto cursor = m_deletions[i].m_cursor;
        int pos = m_deletions[i].m_insertPos;
        QString text = m_deletions[i].m_delText;
        cursor.setPosition(pos,QTextCursor::MoveAnchor);
        cursor.insertText(text);

        cursor.setPosition(pos,QTextCursor::MoveAnchor);
        int id = m_deletions[i].m_id_in_Column;
        m_ColumnEditSelections[id].cursor = cursor;
        m_edit->setTextCursor(cursor);
    }

}

void DeleteBackAltCommond::redo()
{
    int size = m_deletions.size();

    for(int i=0;i<size;i++){
        auto cursor = m_deletions[i].m_cursor;
        int pos = m_deletions[i].m_delPos;
        QString text = m_deletions[i].m_delText;
        cursor.setPosition(pos,QTextCursor::MoveAnchor);

        for(int j=0;j<text.size();j++){
            cursor.deleteChar();
        }
    }

}
