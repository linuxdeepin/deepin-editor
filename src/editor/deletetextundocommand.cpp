#include "deletetextundocommand.h"
#include <QDebug>
#include <QTextBlock>

DeleteTextUndoCommand::DeleteTextUndoCommand(QTextCursor textcursor):
    m_textCursor(textcursor)
{
    if(m_textCursor.hasSelection()){
        m_sInsertText = m_textCursor.selectedText();
        qDebug()<<m_sInsertText;
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

DeleteTextUndoCommand::DeleteTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections):
    m_ColumnEditSelections(selections)
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
        qDebug()<<"InsertTextUndoCommand[undo]:"<<m_sInsertText<<m_sInsertText.length();

    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.insertText(m_selectTextList[i]);
            m_ColumnEditSelections[i].cursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor,m_selectTextList[i].length());
        }
        qDebug()<<"InsertTextUndoCommand[undo]:"<<m_selectTextList;
    }

}


void DeleteTextUndoCommand::redo()
{

    if(m_ColumnEditSelections.isEmpty()){
        m_textCursor.deletePreviousChar();
        qDebug()<<"InsertTextUndoCommand[redo]:"<<m_sInsertText<<m_sInsertText.length();
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.deletePreviousChar();
        }
        qDebug()<<"InsertTextUndoCommand[redo]:"<<m_selectTextList;
    }
}
