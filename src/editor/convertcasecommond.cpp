#include "convertcasecommond.h"
#include "assert.h"
#include "deletetextundocommand.h"
#include "inserttextundocommand.h"
#include "../common/utils.h"
ConvertCaseCommond::ConvertCaseCommond(DeleteTextUndoCommand* deleteCommond,InsertTextUndoCommand* insertCommond):
    m_delete(deleteCommond),
    m_insert(insertCommond)
{
    assert(m_delete != nullptr);
    assert(m_insert != nullptr);
}

ConvertCaseCommond::~ConvertCaseCommond()
{
    SAFE_DELETE(m_delete);
    SAFE_DELETE(m_insert);
}

void ConvertCaseCommond::undo()
{
    m_insert->undo();
    m_delete->undo();
}

void ConvertCaseCommond::redo()
{
    m_delete->redo();
    m_insert->redo();
}
