/*
 * Copyright (C) 2017 ~ 2019 Deepin, Inc.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#include "linenumberarea.h"
#include "dtextedit.h"

LineNumberArea::LineNumberArea(TextEdit *textEdit)
{
    m_textEdit = textEdit;
}

LineNumberArea::~LineNumberArea()
{
}

void LineNumberArea::paintEvent(QPaintEvent *e)
{
    m_textEdit->lineNumberAreaPaintEvent(e);
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_textEdit->lineNumberAreaWidth(), 0);
}
