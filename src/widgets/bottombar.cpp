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
#include "../utils.h"
#include "../editwrapper.h"

#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>
#include <QMenu>

#include <DVerticalLine>

BottomBar::BottomBar(QWidget *parent)
    : QWidget(parent),
      m_wrapper(static_cast<EditWrapper *>(parent)),
      m_positionLabel(new QLabel),
      m_charCountLabel(new QLabel),
      m_cursorStatus(new QLabel),
      m_encodeMenu(new DDropdownMenu),
      m_highlightMenu(new DDropdownMenu),
      m_rowStr(tr("Row")),
      m_columnStr(tr("Column")),
      m_chrCountStr(tr("Characters %1"))
{
    QFont font;
    font.setFamily("SourceHanSansSC-Normal");
    font.setPixelSize(11);
    m_positionLabel->setFont(font);
    m_charCountLabel->setFont(font);
    m_cursorStatus->setFont(font);
    m_encodeMenu->setFont(font);
    m_highlightMenu->setFont(font);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(29, 1, 10, 0);
    layout->addWidget(m_positionLabel);
    layout->addStretch();
    layout->addSpacing(110);
    layout->addWidget(m_charCountLabel);

    m_cursorStatus->setText(qApp->translate("EditWrapper", "INSERT"));
    m_positionLabel->setText(QString("%1 %2  %3 %4").arg(m_rowStr, "1",
                                                          m_columnStr, "1"));
    m_charCountLabel->setText(m_chrCountStr.arg("..."));
    m_encodeMenu->addActions(Utils::getEncodeList());
    m_encodeMenu->setCurrentText("UTF-8");
    m_highlightMenu->setCurrentTextOnly(qApp->translate("DTextEdit", "None"));

    DVerticalLine *pVerticalLine1 = new DVerticalLine();
    DVerticalLine *pVerticalLine2 = new DVerticalLine();
    pVerticalLine1->setFixedSize(1, 15);
    pVerticalLine2->setFixedSize(1, 15);

    layout->addStretch();
    layout->addWidget(m_cursorStatus);
    layout->addSpacing(10);
    layout->addWidget(pVerticalLine1);
    layout->addWidget(m_encodeMenu);
    layout->addWidget(pVerticalLine2);
    layout->addWidget(m_highlightMenu);

    setFixedHeight(32);

    connect(m_encodeMenu, &DDropdownMenu::currentTextChanged, this, &BottomBar::handleEncodeChanged);
}

BottomBar::~BottomBar()
{
}

void BottomBar::updatePosition(int row, int column)
{
    m_positionLabel->setText(QString("%1 %2  %3 %4").arg(m_rowStr, QString::number(row),
                                                          m_columnStr, QString::number(column)));
}

void BottomBar::updateWordCount(int charactorCount)
{
    m_charCountLabel->setText(m_chrCountStr.arg(QString::number(charactorCount)));
}

void BottomBar::setEncodeName(const QString &name)
{
    m_encodeMenu->setCurrentText(name);
}

void BottomBar::setCursorStatus(const QString &text)
{
    m_cursorStatus->setText(text);
}

void BottomBar::setHighlightMenu(QMenu *menu)
{
    m_highlightMenu->setMenu(menu);
}

void BottomBar::setHightlightName(const QString &name)
{
    m_highlightMenu->setCurrentTextOnly(name);
}

void BottomBar::setPalette(const QPalette &palette)
{
    DPalette paPositionLabel  = DApplicationHelper::instance()->palette(m_positionLabel);
    DPalette paCharCountLabel = DApplicationHelper::instance()->palette(m_charCountLabel);
    DPalette paCursorStatus = DApplicationHelper::instance()->palette(m_cursorStatus);
    DPalette paEncodeMenu = DApplicationHelper::instance()->palette(m_encodeMenu);
    DPalette paHighlightMenu = DApplicationHelper::instance()->palette(m_highlightMenu);

    QColor colorFont;
    if(palette.color((QPalette::Background)).lightness() < 128) {
        colorFont = QColor("#6D7C88");
    }
    else {
        colorFont = QColor("#526A7F");
    }

    //QColor(palette.color(QPalette::Text).name())
    paPositionLabel.setColor(DPalette::WindowText, colorFont);
    paCharCountLabel.setColor(DPalette::WindowText, colorFont);
    paCursorStatus.setColor(DPalette::WindowText, colorFont);
    paEncodeMenu.setColor(DPalette::WindowText, colorFont);
    paHighlightMenu.setColor(DPalette::WindowText, colorFont);
    m_positionLabel->setPalette(paPositionLabel);
    m_charCountLabel->setPalette(paCharCountLabel);
    m_cursorStatus->setPalette(paCursorStatus);
    m_encodeMenu->setPalette(paEncodeMenu);
    m_highlightMenu->setPalette(paHighlightMenu);

    QString theme = (palette.color(QPalette::Background).lightness() < 128) ? "dark" : "light";
    m_encodeMenu->setTheme(theme);
    m_highlightMenu->setTheme(theme);

    QWidget::setPalette(palette);
}

void BottomBar::handleEncodeChanged(const QString &name)
{
    m_wrapper->setTextCodec(name.toUtf8(), true);
}

void BottomBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);

    QColor backgroundColor = palette().color(QPalette::Background);
    QColor bottombarBackgroundColor;
    QColor fontColor;
    if (backgroundColor.lightness() < 128) {
        bottombarBackgroundColor = QColor("#202020");
        bottombarBackgroundColor.setAlphaF(0.7);
        fontColor = QColor("#6D7C88");
    } else {
        bottombarBackgroundColor = QColor("#ffffff");
        bottombarBackgroundColor.setAlphaF(0.7);
        fontColor = QColor("#526A7F");
    }
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, bottombarBackgroundColor);

    QColor splitLineColor;
    if (backgroundColor.lightness() < 128) {
        splitLineColor = QColor("#ffffff");
        splitLineColor.setAlphaF(0.5);
    } else {
        splitLineColor = QColor("#000000");
        splitLineColor.setAlphaF(0.5);
    }

    QPainterPath framePath;
    framePath.addRect(QRect(rect().x(), rect().y(), rect().width(), 1));
    painter.setOpacity(0.1);
    painter.fillPath(framePath, splitLineColor);

    painter.setPen(fontColor);
}
