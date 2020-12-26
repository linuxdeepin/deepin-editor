/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
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

#include "CSyntaxHighlighter.h"
#include <QDebug>

CSyntaxHighlighter::CSyntaxHighlighter(QObject *parent):
    SyntaxHighlighter (parent),
    m_bHighlight(false)
{

}

CSyntaxHighlighter::CSyntaxHighlighter(QTextDocument *pDocument):
    SyntaxHighlighter (pDocument),m_bHighlight(false)
{

}

void CSyntaxHighlighter::setEnableHighlight(bool isEnable)
{
    m_bHighlight = isEnable;
}

void CSyntaxHighlighter::highlightBlock(const QString &text)
{
    if (!m_bHighlight) {
        return;
    }

    KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);
}
