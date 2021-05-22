#include "replaceallcommond.h"


ReplaceAllCommond::ReplaceAllCommond(QString& oldText, QString& newText, QPlainTextEdit *edit):
    m_oldText(oldText),
    m_newText(newText),
    m_edit(edit)
{

}
ReplaceAllCommond::~ReplaceAllCommond()
{

}

void ReplaceAllCommond::redo()
{
    m_edit->clear();
    m_edit->setPlainText(m_newText);
}

void ReplaceAllCommond::undo()
{
    m_edit->clear();
    m_edit->setPlainText(m_oldText);
}
