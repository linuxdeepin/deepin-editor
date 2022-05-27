#ifndef EndlineFormartCommand_H
#define EndlineFormartCommand_H
#include <QUndoCommand>
#include "../widgets/bottombar.h"


class EndlineFormartCommand:public QUndoCommand
{
public:
    EndlineFormartCommand(BottomBar* bar,BottomBar::EndlineFormat from,BottomBar::EndlineFormat to);
    virtual ~EndlineFormartCommand();

protected:
    virtual void undo();
    virtual void redo();

private:
    BottomBar* m_bar=nullptr;
    BottomBar::EndlineFormat m_from;
    BottomBar::EndlineFormat m_to;

};



#endif // UndoList_H
