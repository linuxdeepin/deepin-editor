#ifndef CONVERTCASECOMMOND_H
#define CONVERTCASECOMMOND_H

#include "QUndoCommand"
class InsertTextUndoCommand;
class DeleteTextUndoCommand;

class ConvertCaseCommond:public QUndoCommand
{
public:
    ConvertCaseCommond(DeleteTextUndoCommand* deleteCommond,InsertTextUndoCommand* insertCommond);
    virtual ~ConvertCaseCommond();
    virtual void undo();
    virtual void redo();

private:
    DeleteTextUndoCommand* m_delete;
    InsertTextUndoCommand* m_insert;
};


#endif // CONVERTCASECOMMOND_H
