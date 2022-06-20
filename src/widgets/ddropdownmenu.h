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
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>
#include <QtXml/QDomNodeList>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
DWIDGET_USE_NAMESPACE

class DDropdownMenu : public QFrame
{
    Q_OBJECT

public:
    explicit DDropdownMenu(QWidget *parent = nullptr);
    ~DDropdownMenu();
    void setFontEx(const QFont& font);

    void setMenu(DMenu *menu);
    void deleteMenu();
    void setMenuActionGroup(QActionGroup *actionGroup);
    void deleteMenuActionGroup();
    void setTheme(const QString &theme);

    void setChildrenFocus(bool ok);
    void setRequestMenu(bool request);
    DToolButton* getButton();

public slots:
    void setCurrentAction(QAction*);
    void setCurrentTextOnly(const QString& name);
    void slotRequestMenu(bool request);
    void setCheckedExclusive(QAction* action,const QString& name);

public:
    //创建编码菜单
    static DDropdownMenu* createEncodeMenu();
    //创建文件类型菜单
    static DDropdownMenu* createHighLightMenu();

signals:
    void requestContextMenu(bool bClicked = false);
    void currentTextChanged(const QString &text);
    void currentActionChanged(QAction*);
    void sigSetTextEditFocus();

private:
    //创建文字ICON
    QIcon createIcon();
    void setText(const QString &text);
private slots:
    //字体大小跟随系统变化
    void OnFontChangedSlot(const QFont &font);
protected:
    //按键事件　鼠标释放弹出菜单
    bool eventFilter(QObject *object, QEvent *event);
private:
    QPixmap setSvgColor(QString color);
    void SetSVGBackColor(QDomElement &elem, QString strattr, QString strattrval);
private:
    DToolButton *m_pToolButton = nullptr;
    DMenu *m_menu = nullptr;
    QActionGroup *m_actionGroup = nullptr;
    QPixmap m_arrowPixmap;
    QString m_text = "UTF-8";
    QAction* m_pActUtf8 = nullptr;
    QFont m_font;
    bool m_bPressed =false;
    bool isRequest = false;
    KSyntaxHighlighting::Repository m_Repository;
private:
    static QVector<QPair<QString,QStringList>> sm_groupEncodeVec;
};

#endif
