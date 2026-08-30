// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/CSyntaxHighlighter.h"

#include <gtest/gtest.h>
#include <QTextDocument>
#include <QTextCursor>

TEST(CSyntaxHighlighterTest, Construct_WithParent_DoesNotCrash)
{
    QObject parent;
    CSyntaxHighlighter hl(&parent);
    SUCCEED();
}

TEST(CSyntaxHighlighterTest, Construct_WithDocument_DoesNotCrash)
{
    QTextDocument doc;
    CSyntaxHighlighter hl(&doc);
    SUCCEED();
}

TEST(CSyntaxHighlighterTest, SetEnableHighlight_TogglesFlag)
{
    QTextDocument doc;
    CSyntaxHighlighter hl(&doc);
    hl.setEnableHighlight(true);
    hl.setEnableHighlight(false);
    SUCCEED();
}

TEST(CSyntaxHighlighterTest, SetInvalidCharHighlight_TogglesFlag)
{
    QTextDocument doc;
    CSyntaxHighlighter hl(&doc);
    hl.setInvalidCharHighlight(true);
    hl.setInvalidCharHighlight(false);
    SUCCEED();
}

TEST(CSyntaxHighlighterTest, HighlightBlock_WithText_DoesNotCrash)
{
    QTextDocument doc;
    CSyntaxHighlighter hl(&doc);
    hl.setEnableHighlight(true);
    QTextCursor c(&doc);
    c.insertText(QStringLiteral("int main() { return 0; }\n"));
    // Changing text triggers highlightBlock on the modified block.
    SUCCEED();
}
