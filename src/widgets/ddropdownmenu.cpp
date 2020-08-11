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

#include "ddropdownmenu.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <DApplication>
#include "../utils.h"
#include <QPainter>
#include "settings.h"
#include <DSettingsOption>
#include <QDebug>

#define COLOR1 "#1f1c1b"
#define COLOR2 "#cfcfc2"
using namespace Dtk::Core;

DDropdownMenu::DDropdownMenu(QWidget *parent)
    : QFrame(parent)
    , m_pToolButton(new DToolButton(parent))
    , m_menu(new DMenu)
{
    //设置toobutton属性
    m_pToolButton->setFocusPolicy(Qt::StrongFocus);
    m_pToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pToolButton->setArrowType(Qt::NoArrow);
    m_pToolButton->setFixedHeight(30);
    m_pToolButton->installEventFilter(this);

    //设置图标
    QPixmap arrowPixmap = Utils::renderSVG(":/images/dropdown_arrow_light.svg", QSize(9, 5));
    arrowPixmap.setDevicePixelRatio(devicePixelRatioF());
    m_arrowPixmap = arrowPixmap;
    m_pToolButton->setIcon(createIcon());


    /*
    "text-styles": {
        "Normal" : {
            "text-color" : "#1f1c1b",
    */
     //获取文本颜色
     QString themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
     QVariantMap jsonMap = Utils::getThemeMapFromPath(themePath);
     m_textColor = jsonMap["text-styles"].toMap()["Normal"].toMap()["text-color"].toString();


    //添加布局
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_pToolButton);
    layout->setContentsMargins(0,0,0,0);
    this->setLayout(layout);

    connect(m_menu, &DMenu::triggered, this, [=](QAction *action) {
        //编码内容改变触发内容改变和信号发射 梁卫东 2020.7.7
        if (m_text != action->text()) {
            Q_EMIT this->triggered(action);
            Q_EMIT this->currentTextChanged(action->text());
        }
        setText(action->text());
        setCurrentAction(action);
    });

    connect(this, &DDropdownMenu::requestContextMenu, this, [=] {
        QPoint center = this->mapToGlobal(this->rect().center());
        int menuHeight = m_menu->sizeHint().height();
        int menuWidth = m_menu->sizeHint().width();
        center.setY(center.y() - menuHeight - this->rect().height() / 2);
        center.setX(center.x() - menuWidth / 2);
        m_menu->move(center);
        m_menu->exec();
        //显示菜单　清除焦点
        m_pToolButton->clearFocus();
    });

    //设置字体自适应大小
    //设置界面大小根据内容大小自适应 梁卫东 2020.7.7
      connect(qApp,&DApplication::fontChanged,this,&DDropdownMenu::OnFontChangedSlot);
}

DDropdownMenu::~DDropdownMenu() {}

QList<QAction *> DDropdownMenu::actions() const
{
    return m_menu->actions();
}

QAction *DDropdownMenu::addAction(const QString &text)
{
    QAction *action = m_menu->addAction(text);
    action->setCheckable(true);
    setText(action->text());
    return action;
}

void DDropdownMenu::addActions(QStringList list)
{
    for (QString text : list) {
        QAction *action = m_menu->addAction(text);
        action->setCheckable(true);
        setText(action->text());
    }
}

void DDropdownMenu::setCurrentAction(QAction *action)
{
    if (action) {
        for (QAction *action : m_menu->actions()) {
            action->setChecked(false);
        }
        m_text = action->text();
        m_pToolButton->setIcon(createIcon());
        action->setChecked(true);
    } else {
        for (QAction *action : m_menu->actions()) {
            action->setChecked(false);
        }
    }
}

void DDropdownMenu::setCurrentText(const QString &text)
{
    QString strCodecName = text;
    strCodecName = strCodecName.toUpper();
    for (QAction *action : m_menu->actions()) {
        if (action->text() == strCodecName) {
            setCurrentAction(action);
            setText(strCodecName);
            break;
        }
    }
}

void DDropdownMenu::setCurrentTextOnly(const QString &text)
{
    setText(text);
}

void DDropdownMenu::setText(const QString &text)
{
    m_text = text;
    //重新绘制icon　设置宽度
    m_pToolButton->setIcon(createIcon());
}

void DDropdownMenu::setMenu(DMenu *menu)
{
    if (m_menu) {
        delete m_menu;
    }

    m_menu = menu;
   //m_pToolButton->setMenu(m_menu);
}

void DDropdownMenu::setTheme(const QString &theme)
{
    QString arrowSvgPath = QString(":/images/dropdown_arrow_%1.svg").arg(theme);
    QPixmap arrowPixmap = Utils::renderSVG(arrowSvgPath, QSize(9, 5));
    arrowPixmap.setDevicePixelRatio(devicePixelRatioF());

    //获取主题颜色
    if(theme == "light") {
        m_textColor = "#1f1c1b";
    }else if(theme == "dark"){
        m_textColor ="#cfcfc2";
    }

    m_arrowPixmap = arrowSvgPath;
    m_pToolButton->setIcon(createIcon());
}


QIcon DDropdownMenu::createIcon()
{
    //height 30    width QFontMetrics fm(font()) fm.width(text)+40;
    int h = 30;
    int w = QFontMetrics(m_font).width(m_text)+20;
    setFixedWidth(w+40);
    m_pToolButton->setIconSize(QSize(w,h));

    QPixmap icon(w,h);
    icon.fill(Qt::transparent);

    QPainter painter(&icon);
    painter.setFont(m_font);
    painter.setPen(QColor(m_textColor));
//    QTextOption opt;
//    opt.setAlignment(Qt::AlignCenter);
    painter.drawText(0,20,m_text);
    painter.drawPixmap(w-9,h/2,9,5,m_arrowPixmap,0,0,9,5);

    painter.end();
    return icon;
}

void DDropdownMenu::OnFontChangedSlot(const QFont &font)
{
    m_font = font;
    m_pToolButton->setIcon(createIcon());
}


bool DDropdownMenu::eventFilter(QObject *object, QEvent *event)
{

    if(object == m_pToolButton){
        if(event->type() == QEvent::KeyPress){
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            QString key = Utils::getKeyshortcut(keyEvent);
            if(key=="Enter")        //按下enter展开列表
            {
                Q_EMIT requestContextMenu();
                return true;
            }
            return false;
        }

        if(event->type() == QEvent::MouseButtonRelease){
             QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                Q_EMIT requestContextMenu();
                return true;
            }
             return false;
        }
    }

    return QFrame::eventFilter(object,event);
}
