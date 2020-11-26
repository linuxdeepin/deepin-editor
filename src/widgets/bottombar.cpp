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

#include "bottombar.h"
#include "../common/utils.h"
#include "../editor/editwrapper.h"

#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>
#include <DMenu>
#include <DVerticalLine>

BottomBar::BottomBar(QWidget *parent)
    : QWidget(parent),
      m_pWrapper(static_cast<EditWrapper *>(parent)),
      m_pPositionLabel(new DLabel),
      m_pCharCountLabel(new DLabel),
      m_pCursorStatus(new DLabel),
      m_pEncodeMenu(DDropdownMenu::createEncodeMenu()),
      m_pHighlightMenu(new DDropdownMenu()),
      m_rowStr(tr("Row")),
      m_columnStr(tr("Column")),
      m_chrCountStr(tr("Characters %1"))
{
    QFont font;
    font.setFamily("SourceHanSansSC-Normal");
    //font.setPixelSize(11);
    m_pPositionLabel->setFont(font);
    m_pCharCountLabel->setFont(font);
    m_pCursorStatus->setFont(font);
   // m_pEncodeMenu->setFontEx(font);
   // m_pHighlightMenu->setFontEx(font);


    DFontSizeManager::instance()->bind(m_pPositionLabel, DFontSizeManager::T9);
    DFontSizeManager::instance()->bind(m_pCharCountLabel, DFontSizeManager::T9);
    DFontSizeManager::instance()->bind(m_pCursorStatus, DFontSizeManager::T9);
    //DFontSizeManager::instance()->bind(m_pEncodeMenu->getButton(), DFontSizeManager::T9);
   // DFontSizeManager::instance()->bind(m_pHighlightMenu->getButton(), DFontSizeManager::T9);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(29, 1, 10, 0);
    layout->addWidget(m_pPositionLabel);
    layout->addStretch();
    layout->addSpacerItem(new QSpacerItem(110,20,QSizePolicy::Expanding,QSizePolicy::Fixed));
    layout->addWidget(m_pCharCountLabel);

    m_pCursorStatus->setText(qApp->translate("EditWrapper", "INSERT"));
    m_pPositionLabel->setText(QString("%1 %2  %3 %4").arg(m_rowStr, "1",m_columnStr, "1"));

    m_pCharCountLabel->setText(m_chrCountStr.arg("0"));
    m_pHighlightMenu->setCurrentTextOnly(qApp->translate("TextEdit", "None"));

    DVerticalLine *pVerticalLine1 = new DVerticalLine();
    DVerticalLine *pVerticalLine2 = new DVerticalLine();
    pVerticalLine1->setFixedSize(1, 15);
    pVerticalLine2->setFixedSize(1, 15);

    layout->addStretch();
    layout->addWidget(m_pCursorStatus);
    layout->addSpacing(10);
    layout->addWidget(pVerticalLine1);
    layout->addWidget(m_pEncodeMenu);
    layout->addWidget(pVerticalLine2);
    layout->addWidget(m_pHighlightMenu);
    setFixedHeight(32);


    //切换编码
    connect(m_pEncodeMenu, &DDropdownMenu::currentActionChanged, this,[this](QAction* pAct){
        if(m_pWrapper->reloadFileEncode(pAct->text().toLocal8Bit()))
        {
            m_pEncodeMenu->setCurrentAction(pAct);
        }

    });
}

BottomBar::~BottomBar()
{
}

void BottomBar::updatePosition(int row, int column)
{
    m_pPositionLabel->setText(QString("%1 %2  %3 %4").arg(m_rowStr, QString::number(row),
                                                          m_columnStr, QString::number(column)));
}

void BottomBar::updateWordCount(int charactorCount)
{
    m_pCharCountLabel->setText(m_chrCountStr.arg(QString::number(charactorCount-1)));
}

void BottomBar::setEncodeName(const QString &name)
{
    m_pEncodeMenu->setCurrentTextOnly(name);
    //m_wrapper->textEditor()->setTextCode(name);
}

void BottomBar::setCursorStatus(const QString &text)
{
    m_pCursorStatus->setText(text);
}

void BottomBar::setHighlightMenu(DMenu *menu)
{
    m_pHighlightMenu->setMenu(menu);
}

void BottomBar::setHightlightName(const QString &name)
{
    m_pHighlightMenu->setCurrentTextOnly(name);
}

void BottomBar::setPalette(const QPalette &palette)
{
    DPalette paPositionLabel  = DApplicationHelper::instance()->palette(m_pPositionLabel);
    DPalette paCharCountLabel = DApplicationHelper::instance()->palette(m_pCharCountLabel);
    DPalette paCursorStatus = DApplicationHelper::instance()->palette(m_pCursorStatus);
    DPalette paEncodeMenu = DApplicationHelper::instance()->palette(m_pEncodeMenu->getButton());
    DPalette paHighlightMenu = DApplicationHelper::instance()->palette(m_pHighlightMenu->getButton());

    QColor colorFont = paPositionLabel.textTips().color();

    paPositionLabel.setColor(DPalette::WindowText, colorFont);
    paCharCountLabel.setColor(DPalette::WindowText, colorFont);
    paCursorStatus.setColor(DPalette::WindowText, colorFont);
    paEncodeMenu.setColor(DPalette::WindowText, colorFont);
    paHighlightMenu.setColor(DPalette::WindowText, colorFont);

    m_pPositionLabel->setPalette(paPositionLabel);
    m_pCharCountLabel->setPalette(paCharCountLabel);
    m_pCursorStatus->setPalette(paCursorStatus);
    m_pEncodeMenu->getButton()->setPalette(paEncodeMenu);
    m_pHighlightMenu->getButton()->setPalette(paHighlightMenu);

    QString theme = (palette.color(QPalette::Background).lightness() < 128) ? "dark" : "light";
    m_pEncodeMenu->setTheme(theme);
    m_pHighlightMenu->setTheme(theme);

    QWidget::setPalette(palette);
}

void BottomBar::updateSize(int size)
{
    setFixedHeight(size);
}

void BottomBar::setChildrenFocus(bool ok,QWidget* preOrderWidget)
{
    m_pEncodeMenu->setChildrenFocus(ok);
    m_pHighlightMenu->setChildrenFocus(ok);
    if(ok) {
        if(preOrderWidget) setTabOrder(preOrderWidget,m_pEncodeMenu->getButton());
        setTabOrder(m_pEncodeMenu->getButton(),m_pHighlightMenu->getButton());
    }
}


void BottomBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);

    QColor backgroundColor = palette().color(QPalette::Background);
    QColor bottombarBackgroundColor;
    if (backgroundColor.lightness() < 128) {
        bottombarBackgroundColor = palette().base().color();
        if (bottombarBackgroundColor.name() != "#202020") {
            bottombarBackgroundColor = QColor("#202020");
        }
        bottombarBackgroundColor.setAlphaF(0.7);

    } else {
        bottombarBackgroundColor = palette().base().color();
        if (bottombarBackgroundColor.name() != "#ffffff") {
            bottombarBackgroundColor = QColor("#ffffff");
        }

        bottombarBackgroundColor.setAlphaF(0.7);
    }

    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, bottombarBackgroundColor);

    QColor splitLineColor;
    if (backgroundColor.lightness() < 128) {
        splitLineColor = palette().brightText().color();
        if (splitLineColor.name() != "#ffffff") {
            splitLineColor = QColor("#ffffff");
        }
        splitLineColor.setAlphaF(0.5);
    } else {
        splitLineColor = palette().brightText().color();
        if (splitLineColor.name() != "#000000") {
            splitLineColor = QColor("#000000");
        }
        splitLineColor.setAlphaF(0.5);
    }

    QPainterPath framePath;
    framePath.addRect(QRect(rect().x(), rect().y(), rect().width(), 1));
    painter.setOpacity(0.1);
    painter.fillPath(framePath, splitLineColor);
}
