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
#pragma once
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>

using namespace KSyntaxHighlighting;
class CSyntaxHighlighter : public SyntaxHighlighter
{
    Q_OBJECT
public:
    explicit CSyntaxHighlighter(QObject *parent = nullptr);
    explicit CSyntaxHighlighter(QTextDocument *pDocument);
    void setEnableHighlight(bool isEnable);

protected:
    virtual void highlightBlock(const QString & text) override;

private:
    bool m_bHighlight = false;
};
