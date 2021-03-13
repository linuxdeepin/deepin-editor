/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     guoshaoyu <guoshaoyu@uniontech.com>
*
* Maintainer: guoshaoyu <guoshaoyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "../common/comment_global.h"
#include <QString>
#include <QDebug>

namespace Comment {
    class COMMENT_EXPORT CommentDefinition
    {
      public:
      CommentDefinition();

      enum Style { NoStyle, CppStyle, HashStyle };
      void setComments(QString singleLineComment, QString multiLineCommentStart, QString multiLineCommentEnd);

      bool isValid() const;
      bool hasSingleLineStyle() const;
      bool hasMultiLineStyle() const;

      public:
      bool isAfterWhiteSpaces;
      QString singleLine;
      QString multiLineStart;
      QString multiLineEnd;
    };
}
