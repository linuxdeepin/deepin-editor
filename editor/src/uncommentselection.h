#pragma once

#include "comment_global.h"

#include <QString>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Comment {
    class COMMENT_EXPORT CommentDefinition
    {
    public:
    CommentDefinition();

    enum Style { NoStyle, CppStyle, HashStyle };
    void setStyle(Style style);

    bool isValid() const;
    bool hasSingleLineStyle() const;
    bool hasMultiLineStyle() const;

    public:
    bool isAfterWhiteSpaces;
    QString singleLine;
    QString multiLineStart;
    QString multiLineEnd;
    };

    COMMENT_EXPORT
    void unCommentSelection(QPlainTextEdit *edit,
                        const CommentDefinition &definiton = CommentDefinition());

}
