#include "replaceallcommond.h"
#include "dtextedit.h"
#include "editwrapper.h"
#include "../widgets/window.h"

ReplaceAllCommond::ReplaceAllCommond(QString& oldText, QString& newText, QTextCursor cursor):
    m_oldText(oldText),
    m_newText(newText),
    m_cursor(cursor)
{

}

ReplaceAllCommond::ReplaceAllCommond(TextEdit *edit,EditWrapper* wrapper,QList<int> postions,const QString& oldText,const QString& newText):
    m_edit(edit),
    m_wrapper(wrapper),
    m_oldText(oldText),
    m_newText(newText)
{
    int size = postions.size();
    int redoSum=0;
    int undoSum=0;
    int offset = m_newText.size() - m_oldText.size();
    for(int i=0;i<size;i++){

        int pos = postions[i];
        Node node;
        node.redoPos = pos + redoSum;
        node.undoPos = node.redoPos + undoSum;

        redoSum += offset;
        undoSum -= offset;
        m_nodes.push_back(node);
    }

}

ReplaceAllCommond::~ReplaceAllCommond()
{

}

void ReplaceAllCommond::redo()
{
#if 0
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_newText);
#endif

    QObject::disconnect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
    m_wrapper->window()->showReplaceBar(false);

    QTextCursor cursor = m_edit->textCursor();
    int size = m_nodes.size();

    for(int i=0;i<size;i++){
        int pos = m_nodes[i].redoPos;
        cursor.setPosition(pos,QTextCursor::MoveAnchor);
        cursor.setPosition(pos + m_oldText.size(),QTextCursor::KeepAnchor);
        if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
            m_edit->setReadOnly(true);
            cursor.insertText(m_newText);
            QApplication::processEvents();
        }
        else{
            return;
        }
    }

     QObject::connect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
     m_edit->setReadOnly(false);

}


void ReplaceAllCommond::undo()
{
#if 0
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_oldText);
#endif

    QObject::disconnect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
    QTextCursor cursor = m_edit->textCursor();
    int size = m_nodes.size();

    for(int i=0;i<size;i++){
        int pos = m_nodes[i].undoPos;
        cursor.setPosition(pos,QTextCursor::MoveAnchor);
        cursor.setPosition(pos + m_newText.size(),QTextCursor::KeepAnchor);

        if(m_wrapper!=nullptr && !m_wrapper->isQuit()){
            m_edit->setReadOnly(true);
            cursor.insertText(m_oldText);
            QApplication::processEvents();
        }
        else{
            return;
        }
    }

     QObject::connect(m_edit, &QPlainTextEdit::cursorPositionChanged, m_edit, &TextEdit::cursorPositionChanged);
     m_edit->setReadOnly(false);
}

