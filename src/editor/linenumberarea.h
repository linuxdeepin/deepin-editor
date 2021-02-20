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

#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>

class LeftAreaTextEdit;
class TextEdit;

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(LeftAreaTextEdit *leftAreaWidget);
    ~LineNumberArea() override;

    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override;

private:
    LeftAreaTextEdit *m_leftAreaWidget;
};

#endif
