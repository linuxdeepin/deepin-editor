#include "test_csyntaxhighlighter.h"

test_CSyntaxHighlighter::test_CSyntaxHighlighter()
{

}

TEST_F(test_CSyntaxHighlighter, setEnableHighlight)
{
    QTextDocument *document;
    TextEdit *m_pTextEdit = new TextEdit();
    bool bRet = true;
    CSyntaxHighlighter *pCSyntaxHighlighter = new CSyntaxHighlighter(m_pTextEdit->document());
    pCSyntaxHighlighter->setEnableHighlight(bRet);

    
}

TEST_F(test_CSyntaxHighlighter, setEnableHighlight2)
{
    QTextDocument *document;
    TextEdit *m_pTextEdit = new TextEdit();
    bool bRet = true;
    CSyntaxHighlighter *pCSyntaxHighlighter = new CSyntaxHighlighter(this);
    pCSyntaxHighlighter->setEnableHighlight(bRet);

    
}

TEST_F(test_CSyntaxHighlighter, highlightBlock)
{
    QTextDocument *document;
    TextEdit *m_pTextEdit = new TextEdit();
    bool bRet = true;
    const QString strTest = "12345";
    CSyntaxHighlighter *pCSyntaxHighlighter = new CSyntaxHighlighter(m_pTextEdit->document());
    pCSyntaxHighlighter->m_bHighlight = false;
    pCSyntaxHighlighter->highlightBlock(strTest);

    
}

TEST_F(test_CSyntaxHighlighter, highlightBlock1)
{
    QTextDocument *document;
    TextEdit *m_pTextEdit = new TextEdit();
    bool bRet = true;
    const QString strTest = "12345";
    CSyntaxHighlighter *pCSyntaxHighlighter = new CSyntaxHighlighter(m_pTextEdit->document());
    pCSyntaxHighlighter->m_bHighlight = true;
    pCSyntaxHighlighter->highlightBlock(strTest);

    
}

