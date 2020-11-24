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
#include "../settings.h"
#include <DSettingsOption>
#include <QDebug>
#include <DFontSizeManager>
#include <DLabel>
#include <DApplicationHelper>
using namespace Dtk::Core;

QVector<QPair<QString,QStringList>> DDropdownMenu::sm_groupEncodeVec;

DDropdownMenu::DDropdownMenu(QWidget *parent)
    : QFrame(parent)
    , m_pToolButton(new DToolButton(this))
    , m_menu(new DMenu)
{
    //设置toobutton属性
    m_pToolButton->setFocusPolicy(Qt::StrongFocus);
    m_pToolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_pToolButton->setArrowType(Qt::NoArrow);
    m_pToolButton->setFixedHeight(30);
    m_pToolButton->installEventFilter(this);

    //设置图标
    QPixmap arrowPixmap = Utils::renderSVG(":/images/dropdown_arrow_light.svg", QSize(9, 5));
    arrowPixmap.setDevicePixelRatio(devicePixelRatioF());
    m_arrowPixmap = arrowPixmap;
    m_pToolButton->setIcon(createIcon());

    //获取文本颜色
    QString themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
    QVariantMap jsonMap = Utils::getThemeMapFromPath(themePath);
    m_textColor = jsonMap["text-styles"].toMap()["Normal"].toMap()["text-color"].toString();
    m_backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();

    //设置字体
    int fontsize =DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T9);
    m_font.setPixelSize(fontsize);
    m_font.setFamily("SourceHanSansSC-Normal");


     //添加布局
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_pToolButton);
    layout->setContentsMargins(0,0,0,0);
    this->setLayout(layout);

    connect(this, &DDropdownMenu::requestContextMenu, this, [=] (bool bClicked){
        QPoint center = this->mapToGlobal(this->rect().center());
        int menuHeight = m_menu->sizeHint().height();
        int menuWidth = m_menu->sizeHint().width();
        center.setY(center.y() - menuHeight - this->rect().height() / 2);
        center.setX(center.x() - menuWidth / 2);
        m_menu->move(center);
        m_menu->exec();
        if(bClicked){
            //如果鼠标点击清除ｆｏｃｕｓ
            m_pToolButton->clearFocus();
        }
    });

    //设置字体自适应大小
    //设置界面大小根据内容大小自适应 梁卫东 2020.7.7
    connect(qApp,&DApplication::fontChanged,this,&DDropdownMenu::OnFontChangedSlot);
}

DDropdownMenu::~DDropdownMenu() {}



void DDropdownMenu::setCurrentTextOnly(const QString &text)
{
    setText(text);
}

void DDropdownMenu::setCurrentAction(QAction *pAct)
{
    if(pAct){
        QList<QAction*> menuList = m_menu->actions();
        pAct->setChecked(true);
        for (int i = 0; i < menuList.size(); i++) {
            QList<QAction*> acts = menuList[i]->menu()->actions();
            for (int j = 0; j < acts.size(); j++) {
                if(acts[j] != pAct) acts[j]->setChecked(false);
            }

        }
    }
    setText(pAct->text());
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
}

void DDropdownMenu::setTheme(const QString &theme)
{
    QString arrowSvgPath = QString(":/images/dropdown_arrow_%1.svg").arg(theme);
    QPixmap arrowPixmap = Utils::renderSVG(arrowSvgPath, QSize(9, 5));
    arrowPixmap.setDevicePixelRatio(devicePixelRatioF());

    //获取文本颜色
    QString themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
    QVariantMap jsonMap = Utils::getThemeMapFromPath(themePath);
    m_textColor = jsonMap["text-styles"].toMap()["Normal"].toMap()["text-color"].toString();
    m_backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();

    m_arrowPixmap = arrowPixmap;
    m_pToolButton->setIcon(createIcon());
}

void DDropdownMenu::setChildrenFocus(bool ok)
{
    if(ok)  m_pToolButton->setFocusPolicy(Qt::StrongFocus);
    else   m_pToolButton->setFocusPolicy(Qt::NoFocus);
}

DToolButton *DDropdownMenu::getButton()
{
    return m_pToolButton;
}

DDropdownMenu *DDropdownMenu::createEncodeMenu()
{
    DDropdownMenu *m_pEncodeMenu = new DDropdownMenu();
    DMenu* m_pMenu = new DMenu();
    if(sm_groupEncodeVec.isEmpty()){
        QFile file(":/encodes/encodes.ini");
        QString data;
        if(file.open(QIODevice::ReadOnly))
        {
           data = QString::fromUtf8(file.readAll());
           file.close();
        }

        QTextStream readStream(&data,QIODevice::ReadOnly);
        while (!readStream.atEnd()) {
            QString group = readStream.readLine();
            QString key = group.mid(1,group.length()-2);
            QString encodes = readStream.readLine();
            QString value = encodes.mid(8,encodes.length()-2);
            sm_groupEncodeVec.append(QPair<QString,QStringList>(key,value.split(",")));

            QStringList list = value.split(",");
            QMenu* groupMenu = new QMenu(QObject::tr(key.toLocal8Bit().data()));
             foreach(QString var,list)
             {
               QAction *act= groupMenu->addAction(QObject::tr(var.toLocal8Bit().data()));
               act->setCheckable(true);
             }

            m_pMenu->addMenu(groupMenu);
        }
    }else {

        int cnt = sm_groupEncodeVec.size();
        for (int i = 0;i < cnt;i++) {
            QMenu* groupMenu = new QMenu(QObject::tr(sm_groupEncodeVec[i].first.toLocal8Bit().data()));
             foreach(QString var,sm_groupEncodeVec[i].second)
             {
               QAction *act= groupMenu->addAction(QObject::tr(var.toLocal8Bit().data()));
               act->setCheckable(true);
             }

            m_pMenu->addMenu(groupMenu);
        }
    }

    connect(m_pMenu, &DMenu::triggered, m_pEncodeMenu,[m_pEncodeMenu](QAction *action) {
        //编码内容改变触发内容改变和信号发射 梁卫东 2020.7.7
        if (m_pEncodeMenu->m_text != action->text()) {
            //emit m_pEncodeMenu->currentTextChanged(action->text());
            emit m_pEncodeMenu->currentActionChanged(action);
        }
    });

    m_pEncodeMenu->setText("UTF-8");
    m_pEncodeMenu->setMenu(m_pMenu);

    return  m_pEncodeMenu;
}

DDropdownMenu *DDropdownMenu::createHighLightMenu()
{
    return nullptr;
}

QIcon DDropdownMenu::createIcon()
{
    //根据字体大小设置icon大小
    //height 30    width QFontMetrics fm(font()) fm.width(text)+40;
    int fontWidth = QFontMetrics(m_font).width(m_text)+20;
    int fontHeight = QFontMetrics(m_font).height();
    int iconW = fontWidth+20;
    int iconH = 30;
    setFixedWidth(iconW);
    m_pToolButton->setIconSize(QSize(iconW,iconH));

    QPixmap icon(QSize(iconW,iconH) * devicePixelRatioF());
    icon.setDevicePixelRatio(devicePixelRatioF());
    icon.fill(Qt::transparent);

    //获取文本字体颜色
    QColor textColor;
    DLabel label;
    label.setFont(m_font);
    DPalette dpalette  = DApplicationHelper::instance()->palette(&label);
    textColor = dpalette.textTips().color();

    QPainter painter(&icon);
    painter.setFont(m_font);
    painter.setPen(textColor);
    painter.drawText(QRect(10,(iconH-fontHeight)/2,fontWidth,fontHeight),m_text);
    painter.drawPixmap(QRect(fontWidth,(iconH-5)/2,9,5),m_arrowPixmap,m_arrowPixmap.rect());

    painter.end();
    return icon;
}

void DDropdownMenu::OnFontChangedSlot(const QFont &font)
{
    m_font = font;
    int fontsize =DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T9);
    m_font.setPixelSize(fontsize);
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
                Q_EMIT requestContextMenu(false);
                return true;
            }
            return false;
        }

        if(event->type() == QEvent::MouseButtonRelease){
             QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                Q_EMIT requestContextMenu(true);
                return true;
            }
             return false;
        }

    }

    return QFrame::eventFilter(object,event);
}
