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

#ifndef THEMEPANEL_H
#define THEMEPANEL_H

#include <QWidget>
#include "themelistview.h"
#include "themelistmodel.h"
#include "themeitemdelegate.h"

class ThemePanel : public QWidget
{
    Q_OBJECT

public:
    ThemePanel(QWidget *parent = nullptr);
    ~ThemePanel();

    void setBackground(const QString &color);
    void popup();
    void hide();

    void setFrameColor(const QString &selectedColor, const QString &normalColor);
    void setSelectionTheme(const QString &path);

signals:
    void themeChanged(const QString &path);
    void popupFinished();

protected:
    void paintEvent(QPaintEvent *);

private:
    ThemeListView *m_themeView;
    ThemeListModel *m_themeModel;

    QColor m_frameLightColor = QColor("#000000");
    QColor m_frameDarkColor = QColor("#FFFFFF");
    QColor m_backgroundColor;
    QColor m_frameColor;
};

#endif
