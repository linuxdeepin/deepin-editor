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

#ifndef DDROPDOWNMENU_H
#define DDROPDOWNMENU_H

#include <QFrame>
#include <DMenu>
#include <QLabel>
#include <QEvent>
#include <DToolButton>
#include <QPalette>

DWIDGET_USE_NAMESPACE

class DDropdownMenu : public QFrame
{
    Q_OBJECT

public:
    DDropdownMenu(QWidget *parent = nullptr);
    ~DDropdownMenu();

    QList<QAction *> actions() const;

    QAction *addAction(const QString &text);
    void addActions(QStringList list);
    void setCurrentAction(QAction *action);
    void setCurrentText(const QString &text);
    void setCurrentTextOnly(const QString &text);
    void setText(const QString &text);

    void setMenu(DMenu *menu);
    void setTheme(const QString &theme);


private:
    //创建文字ICON
    QIcon createIcon();
public slots:
    //字体大小跟随系统变化
    void OnFontChangedSlot(const QFont &font);

signals:
    void requestContextMenu();
    void triggered(QAction *action);
    void currentTextChanged(const QString &text);

protected:
    //按键事件　鼠标释放弹出菜单
    bool eventFilter(QObject *object, QEvent *event);
private:
    DToolButton *m_pToolButton = nullptr;
    DMenu *m_menu = nullptr;
    QPixmap m_arrowPixmap;
    QString m_text;
    QFont m_font;
    QString m_textColor;
    QString m_backgroundColor;
};

#endif
