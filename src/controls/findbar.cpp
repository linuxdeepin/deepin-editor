// SPDX-FileCopyrightText: 2011-2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "findbar.h"
#include "../common/utils.h"

#include <QDebug>

FindBar::FindBar(QWidget *parent)
    : DFloatingWidget(parent)
{
    // Init.
    //setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    hide();
    setFixedHeight(60);

    // Init layout and widgets.

    m_layout = new QHBoxLayout();
    m_layout->setSpacing(10);
    m_findLabel = new QLabel(tr("Find"));
    m_findLabel->setMinimumHeight(36);
    m_editLine = new LineBar();
    m_editLine->lineEdit()->setMinimumHeight(36);
    m_findPrevButton = new QPushButton(tr("Previous"));
    //m_findPrevButton->setFixedSize(80, 36);
    m_findNextButton = new QPushButton(tr("Next"));
    //m_findNextButton->setFixedSize(80, 36);
    m_closeButton = new DIconButton(DStyle::SP_CloseButton);
    m_closeButton->setIconSize(QSize(30, 30));
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setEnabledCircle(true);
    m_closeButton->setFlat(true);
    m_layout->setContentsMargins(16, 6, 10, 6);

    m_layout->addWidget(m_findLabel);
    m_layout->addWidget(m_editLine);
    m_layout->addWidget(m_findPrevButton);
    m_layout->addWidget(m_findNextButton);
    m_layout->addWidget(m_closeButton);
    this->setLayout(m_layout);

    // Make button don't grab keyboard focus after click it.
    // m_findNextButton->setFocusPolicy(Qt::NoFocus);
    // m_findPrevButton->setFocusPolicy(Qt::NoFocus);
    // m_closeButton->setFocusPolicy(Qt::NoFocus);

    connect(this, &FindBar::pressEsc, this, &FindBar::findCancel, Qt::QueuedConnection);
    // connect(m_editLine, &LineBar::pressEnter, this, &FindBar::findNext, Qt::QueuedConnection);            //Shielded by Hengbo ,for new demand. 20200220
    //connect(m_editLine, &LineBar::pressCtrlEnter, this, &FindBar::findPrev, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::returnPressed, this, &FindBar::handleContentChanged, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::signal_sentText, this, &FindBar::receiveText, Qt::QueuedConnection);
    //connect(m_editLine, &LineBar::contentChanged, this, &FindBar::slot_ifClearSearchWord, Qt::QueuedConnection);

    connect(m_findNextButton, &QPushButton::clicked,  this, &FindBar::handleFindNext, Qt::QueuedConnection);
    connect(m_findPrevButton, &QPushButton::clicked, this, &FindBar::handleFindPrev, Qt::QueuedConnection);
    //connect(m_findPrevButton, &QPushButton::clicked, this, &FindBar::findPrev, Qt::QueuedConnection);

    connect(m_closeButton, &DIconButton::clicked, this, &FindBar::findCancel, Qt::QueuedConnection);
}

bool FindBar::isFocus()
{
    return m_editLine->lineEdit()->hasFocus();
}

void FindBar::focus()
{
    m_editLine->lineEdit()->setFocus();
    m_editLine->lineEdit()->selectAll();
}

void FindBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    // Try fill keyword with select text.
    m_editLine->lineEdit()->clear();
    m_editLine->lineEdit()->insert(text);
    m_editLine->lineEdit()->selectAll();

    // Show.
    QWidget::show();

    // Save file info for back to position.
    m_findFile = file;
    m_findFileRow = row;
    m_findFileColumn = column;
    m_findFileSrollOffset = scrollOffset;

    // Focus.
    focus();
}

void FindBar::findCancel()
{
    QWidget::hide();
    emit sigFindbarClose();
}

void FindBar::handleContentChanged()
{
    updateSearchKeyword(m_findFile, m_editLine->lineEdit()->text());
}

void FindBar::handleFindPrev()
{
    findPrev(m_editLine->lineEdit()->text());
}

void FindBar::handleFindNext()
{
    findNext(m_editLine->lineEdit()->text());
}

void FindBar::hideEvent(QHideEvent *)
{
    //保留查询标记
    //removeSearchKeyword();
}

bool FindBar::focusNextPrevChild(bool next)
{
    return false;
}

void FindBar::keyPressEvent(QKeyEvent *e)
{
    const QString &key = Utils::getKeyshortcut(e);
    if (key == "Esc") {
        QWidget::hide();
        emit sigFindbarClose();
    }
    if (m_closeButton->hasFocus() && key == "Tab") {
        m_editLine->lineEdit()->setFocus();
    } else {
        DFloatingWidget::keyPressEvent(e);
    }
    if (key == "Enter") {
        if (m_findPrevButton->hasFocus()) {
            m_findPrevButton->click();
        }
        if (m_findNextButton->hasFocus()) {
            m_findNextButton->click();
        }
    }
}

void FindBar::setMismatchAlert(bool isAlert)
{
    m_editLine->setAlert(isAlert);
}

void FindBar::receiveText(QString t)
{
    searched = false;
    if (t != "") {
        m_receivedText = t;
    }
}

void FindBar::setSearched(bool _)
{
    searched = _;
}

void FindBar::findPreClicked()
{
    if (!searched) {
        updateSearchKeyword(m_findFile, m_editLine->lineEdit()->text());
        emit findPrev(m_editLine->lineEdit()->text());
        searched = true;
    } else {
        emit findPrev(m_editLine->lineEdit()->text());
    }
}
