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

#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>
#include <QLabel>
#include <DLabel>
#include "ddropdownmenu.h"
#include <DApplicationHelper>
#include <DFontSizeManager>

class EditWrapper;
class BottomBar : public QWidget
{
    Q_OBJECT

public:
    BottomBar(QWidget *parent = nullptr);
    ~BottomBar();

    void updatePosition(int row, int column);
    void updateWordCount(int charactorCount);
    void setEncodeName(const QString &name);
    void setCursorStatus(const QString &text);
    void setHighlightMenu(DMenu *menu);
    void setHightlightName(const QString &name);
    void setPalette(const QPalette &palette);
    void updateSize(int size);
    //清除所有焦点　梁卫东　２０２０－０９－１４　１０：５５：２２
    void chearAllFocus();
private:
    void handleEncodeChanged(const QString &name);

protected:
    void paintEvent(QPaintEvent *);

private:
    EditWrapper *m_wrapper;
    DLabel *m_positionLabel;
    DLabel *m_charCountLabel;
    DLabel *m_cursorStatus;
    DDropdownMenu *m_encodeMenu;
    DDropdownMenu *m_highlightMenu;
    QString m_rowStr;
    QString m_columnStr;
    QString m_chrCountStr;
};

#endif
