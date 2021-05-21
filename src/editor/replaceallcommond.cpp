#include "replaceallcommond.h"

ReplaceAllCommond::ReplaceAllCommond(QString replace, QString replaceWith, QList<QTextCursor>& cursors):
    m_replace(replace),
    m_replaceWith(replaceWith),
    m_cursors(cursors)
{

    for(int i=0;i<cursors.size();i++){
        int pos = std::min(cursors[i].position(),cursors[i].anchor());
        int delPos = pos - i * m_replace.size();
        int insertPos = delPos + i * m_replaceWith.size();
        m_delPos.push_back(delPos);
        m_insertPos.push_back(insertPos);
    }

}

ReplaceAllCommond::~ReplaceAllCommond()
{

}

void ReplaceAllCommond::redo()
{
    for(int i=0;i<m_cursors.size();i++){
        int delPos = m_delPos.at(i);
        auto cursor = m_cursors.at(i);
        cursor.setPosition(delPos);
        cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,m_replace.size());
        cursor.deleteChar();

    }

    for(int i = 0;i<m_cursors.size();i++){
        int insertPos = m_insertPos.at(i);
        auto cursor = m_cursors.at(i);
        cursor.setPosition(insertPos);
        cursor.insertText(m_replaceWith);
    }
}

void ReplaceAllCommond::undo()
{

    for(int i=0;i<m_cursors.size();i++){
        auto cursor = m_cursors.at(i);
        int delPos = m_insertPos.at(i);
        delPos -= m_replaceWith.size() * i;
        cursor.setPosition(delPos);

        cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,m_replaceWith.size());
        cursor.deleteChar();
    }


    for(int i=0;i<m_cursors.size();i++){
        auto cursor = m_cursors.at(i);
        int insertPos = m_delPos.at(i);
        insertPos += m_replace.size() * i;
        cursor.setPosition(insertPos);
        cursor.insertText(m_replace);
    }

}
