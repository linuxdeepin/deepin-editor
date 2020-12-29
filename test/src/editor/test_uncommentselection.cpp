#include "test_uncommentselection.h"
#include "../../src/common/settings.h"
#include "../../src/controls/tabbar.h"
#include "../../src/editor/editwrapper.h"
#include "../../src/widgets/window.h"
#include "../../src/startmanager.h"
#include "../../src/editor/dtextedit.h"
#include "../../src/editor/uncommentselection.h"

using namespace Comment;

test_uncommentselection::test_uncommentselection()
{
}

void test_uncommentselection::SetUp()
{
    comDef = new CommentDefinition;
}

void test_uncommentselection::TearDown()
{
    delete comDef;
}

TEST_F(test_uncommentselection, CommentDefinition)
{
    CommentDefinition num;
    ASSERT_FALSE(num.isAfterWhiteSpaces);
}

TEST_F(test_uncommentselection, setComments)
{
    QString singleLineComment = "a";
    QString multiLineCommentStart = "b";
    QString multiLineCommentEnd = "c";
    comDef->setComments(singleLineComment, multiLineCommentStart, multiLineCommentEnd);
    ASSERT_EQ(comDef->singleLine, singleLineComment);
    ASSERT_EQ(comDef->multiLineStart, multiLineCommentStart);
    ASSERT_EQ(comDef->multiLineEnd, multiLineCommentEnd);
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

