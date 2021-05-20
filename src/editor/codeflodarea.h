/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
*
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
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

#ifndef CODEFLODAREA_H
#define CODEFLODAREA_H
#include <QWidget>
class LeftAreaTextEdit;


class CodeFlodArea: public QWidget
{
    Q_OBJECT
public:
    explicit CodeFlodArea(LeftAreaTextEdit *leftAreaWidget);
    ~CodeFlodArea() override;
    void paintEvent(QPaintEvent *e) override;

private:
    LeftAreaTextEdit* m_pLeftAreaWidget = nullptr;
};

#endif // CODEFLODAREA_H
