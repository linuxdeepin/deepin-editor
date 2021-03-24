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
#ifndef COLORSELECTWDG_H
#define COLORSELECTWDG_H

#include <DWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWidgetAction>
#include <QEvent>
#include <DPushButton>
#include <DLabel>
#include <QPainterPath>

using namespace Dtk::Widget;

//单个颜色显示控件
/*
 * 　编辑区域右键菜单　标记颜色action添加显示标记颜色控件　梁卫东　２０２０－０８－１３　０９：４０：４９
 *
 *   ColorLabel 颜色显示控件
*/
class ColorLabel : public DWidget
{
    Q_OBJECT
public:
    explicit ColorLabel(QColor color,QWidget *parent = nullptr);
    void setColorSelected(bool bSelect);
    bool isSelected();
    QColor getColor();
signals:
    void sigColorClicked(bool bSelect,QColor color);
protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *e);
private:
    //颜色选择标记
    bool m_bSelected = false;
    QColor m_color;
};

/*
*       编辑区域右键菜单　标记颜色action添加显示标记颜色控件　梁卫东　２０２０－０８－１３　０９：４０：４９
*
*       ColorSelectWdg 颜色选择控件
*/
//自定义ColorSelectWdg
class ColorSelectWdg: public DWidget
{
    Q_OBJECT
public:
    explicit ColorSelectWdg(QString text,QWidget *parent = nullptr);
    void setTheme(const QString &theme);
    QColor getDefaultColor();
signals:
    void sigColorSelected(bool bSelect,QColor color);
private:
    void initWidget();
protected:
    bool eventFilter(QObject *object, QEvent *event);
private:
    QList<ColorLabel*> m_colorLabels;
    int m_labelWidth = 23;
    int m_labelHeight =23;
    DPushButton* m_pButton = nullptr;
    DLabel* m_pLabel = nullptr;
    QString m_text;
    QColor m_defaultColor;
    QString m_textColor;
};


#endif // COLORSELECTWDG_H
