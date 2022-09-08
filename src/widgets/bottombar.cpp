// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bottombar.h"
#include "../common/utils.h"
#include "../editor/editwrapper.h"
#include "../widgets/window.h"

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
      m_pHighlightMenu(DDropdownMenu::createHighLightMenu()),
      m_rowStr(tr("Row")),
      m_columnStr(tr("Column")),
      m_chrCountStr(tr("Characters %1"))
{
    QFont font;
    font.setFamily("SourceHanSansSC-Normal");
    m_pPositionLabel->setFont(font);
    m_pCharCountLabel->setFont(font);
    m_pCursorStatus->setFont(font);

    DFontSizeManager::instance()->bind(m_pPositionLabel, DFontSizeManager::T9);
    DFontSizeManager::instance()->bind(m_pCharCountLabel, DFontSizeManager::T9);
    DFontSizeManager::instance()->bind(m_pCursorStatus, DFontSizeManager::T9);

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
    m_pEncodeMenu->setCurrentTextOnly(QString("UTF-8"));

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
        if(!m_pWrapper->getFileLoading() && m_pWrapper->reloadFileEncode(pAct->text().toLocal8Bit())) {
            m_pEncodeMenu->setCurrentTextOnly(pAct->text());
        }
        //先屏蔽，双字节空字符先按照显示字符编码号处理
        //m_pWrapper->clearDoubleCharaterEncode();
    });

    //切换文件类型
    connect(m_pHighlightMenu, &DDropdownMenu::currentActionChanged, this,[this](QAction* pAct) {
        m_pHighlightMenu->setCurrentTextOnly(pAct->text());
    });

    //编码按钮/文本类型按钮失去焦点后，设置光标回到文本框里
    connect(m_pEncodeMenu, &DDropdownMenu::sigSetTextEditFocus, this, &BottomBar::slotSetTextEditFocus);
    connect(m_pHighlightMenu, &DDropdownMenu::sigSetTextEditFocus, this, &BottomBar::slotSetTextEditFocus);
}

BottomBar::~BottomBar()
{
    if (m_pEncodeMenu != nullptr) {
        delete m_pEncodeMenu;
        m_pEncodeMenu = nullptr;
    }

    if (m_pHighlightMenu != nullptr) {
        delete m_pHighlightMenu;
        m_pHighlightMenu = nullptr;
    }
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
}

void BottomBar::setCursorStatus(const QString &text)
{
    m_pCursorStatus->setText(text);
}

void BottomBar::setPalette(const QPalette &palette)
{
    DPalette paPositionLabel  = DApplicationHelper::instance()->applicationPalette();
    DPalette paCharCountLabel = DApplicationHelper::instance()->applicationPalette();
    DPalette paCursorStatus = DApplicationHelper::instance()->applicationPalette();
    DPalette paEncodeMenu = DApplicationHelper::instance()->applicationPalette();
    DPalette paHighlightMenu = DApplicationHelper::instance()->applicationPalette();

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

void BottomBar::updateSize(int size, bool bIsFindOrReplace)
{
    setFixedHeight(size);
    m_bIsFindOrReplace = bIsFindOrReplace;
}

void BottomBar::setChildEnabled(bool enabled)
{
    m_pEncodeMenu->setEnabled(enabled);
    m_pHighlightMenu->setEnabled(enabled);
    m_pEncodeMenu->setRequestMenu(enabled);
    m_pHighlightMenu->setRequestMenu(enabled);
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

DDropdownMenu *BottomBar::getEncodeMenu()
{
    return m_pEncodeMenu;
}

DDropdownMenu *BottomBar::getHighlightMenu()
{
    return m_pHighlightMenu;
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

    if (!m_bIsFindOrReplace) {
	    QPainterPath framePath;
	    framePath.addRect(QRect(rect().x(), rect().y(), rect().width(), 1));
	    painter.setOpacity(0.1);
	    painter.fillPath(framePath, splitLineColor);
    }
}

void BottomBar::slotSetTextEditFocus()
{
    Window *pWindow = static_cast<Window *>(m_pWrapper->window());
    emit pWindow->pressEsc();
}
