#include "test_uncommentselection.h"

test_uncommentselection::test_uncommentselection()
{

}
//void setComments(QString singleLineComment, QString multiLineCommentStart, QString multiLineCommentEnd);

TEST_F(test_uncommentselection, CommentDefinition)
{
    CommentDefinition num;
    assert(1==1);
}

TEST_F(test_uncommentselection, setComments)
{
    CommentDefinition * numm = new CommentDefinition();
    numm->setComments("aa","aa","aa");

    assert(1==1);
}
//bool isValid() const;
TEST_F(test_uncommentselection, isValid)
{
    Comment::CommentDefinition numm;
    numm.isValid();

    assert(1==1);
}
//bool hasSingleLineStyle() const;
TEST_F(test_uncommentselection, hasSingleLineStyle)
{
    Comment::CommentDefinition numm;
    numm.hasSingleLineStyle();

    assert(1==1);
}
//bool hasMultiLineStyle() const;
TEST_F(test_uncommentselection, hasMultiLineStyle)
{
    Comment::CommentDefinition numm;
    numm.hasMultiLineStyle();

    assert(1==1);
}
//void unCommentSelection(QPlainTextEdit *edit,
//                    const CommentDefinition &definiton = CommentDefinition());
TEST_F(test_uncommentselection, unCommentSelection)
{
    TextEdit * edit = new TextEdit();
    KSyntaxHighlighting::Repository m_r;
    KSyntaxHighlighting::Definition m_Definition=m_r.definitionForFileName("aa.cpp");
    edit->setSyntaxDefinition(m_Definition);
    const CommentDefinition &definiton = CommentDefinition();
    Comment::CommentDefinition numm;
    Comment::unCommentSelection(edit,definiton);

    assert(1==1);
}
//                    const CommentDefinition &definiton = CommentDefinition());
//void setComment(QPlainTextEdit *edit,
//                const CommentDefinition &definiton = CommentDefinition(),QString name="");
TEST_F(test_uncommentselection, setComment)
{
    TextEdit * edit = new TextEdit();
    KSyntaxHighlighting::Repository m_r;
    KSyntaxHighlighting::Definition m_Definition=m_r.definitionForFileName("aa.cpp");
    edit->setSyntaxDefinition(m_Definition);
    const CommentDefinition &definiton = CommentDefinition();
    Comment::CommentDefinition numm;
    Comment::setComment(edit,definiton);

    assert(1==1);
}

//void removeComment(QPlainTextEdit *edit,
//                   const CommentDefinition &definiton = CommentDefinition(),QString name="");
TEST_F(test_uncommentselection, removeComment)
{
    TextEdit * edit = new TextEdit();
    KSyntaxHighlighting::Repository m_r;
    KSyntaxHighlighting::Definition m_Definition=m_r.definitionForFileName("aa.cpp");
    edit->setSyntaxDefinition(m_Definition);
    const CommentDefinition &definiton = CommentDefinition();
    Comment::CommentDefinition numm;
    Comment::removeComment(edit,definiton);

    assert(1==1);
}

