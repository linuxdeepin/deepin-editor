// SPDX-FileCopyrightText: 2011-2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "replacebar.h"
#include "../common/utils.h"

#include <QDebug>

ReplaceBar::ReplaceBar(QWidget *parent)
    :  DFloatingWidget(parent)
{
    // Init.
    hide();
    setFixedHeight(60);

    // Init layout and widgets.
    m_layout = new QHBoxLayout();
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(16, 6, 10, 6);
    m_replaceLabel = new QLabel(tr("Find"));
    //m_replaceLabel->setMinimumHeight(36);
    m_replaceLine = new LineBar();
    //m_replaceLine->lineEdit()->setMinimumHeight(36);
    m_withLabel = new QLabel(tr("Replace With"));
    //m_withLabel->setMinimumHeight(36);
    m_withLine = new LineBar();
    //m_withLine->lineEdit()->setMinimumHeight(36);
    m_replaceButton = new QPushButton(tr("Replace"));
    //m_replaceButton->setMinimumWidth(66);
    //m_replaceButton->setMinimumHeight(36);
    m_replaceSkipButton = new QPushButton(tr("Skip"));
    //m_replaceSkipButton->setMinimumWidth(66);
    //m_replaceSkipButton->setMinimumHeight(36);
    m_replaceRestButton = new QPushButton(tr("Replace Rest"));
    //m_replaceRestButton->setMinimumWidth(80);
    //m_replaceRestButton->setMinimumHeight(36);
    m_replaceAllButton = new QPushButton(tr("Replace All"));
    //m_replaceAllButton->setMinimumWidth(80);
    //m_replaceAllButton->setMinimumHeight(36);
    m_closeButton = new DIconButton(DStyle::SP_CloseButton);
    m_closeButton->setFlat(true);
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setEnabledCircle(true);
    m_closeButton->setIconSize(QSize(30, 30));

    m_layout->addWidget(m_replaceLabel);
    m_layout->addWidget(m_replaceLine);
    m_layout->addWidget(m_withLabel);
    m_layout->addWidget(m_withLine);
    m_layout->addWidget(m_replaceButton);
    m_layout->addWidget(m_replaceSkipButton);
    m_layout->addWidget(m_replaceRestButton);
    m_layout->addWidget(m_replaceAllButton);
    m_layout->addWidget(m_closeButton);
    this->setLayout(m_layout);

    // Make button don't grab keyboard focus after click it.
    #if 0
    m_replaceButton->setFocusPolicy(Qt::NoFocus);
    m_replaceSkipButton->setFocusPolicy(Qt::NoFocus);
    m_replaceRestButton->setFocusPolicy(Qt::NoFocus);
    m_replaceAllButton->setFocusPolicy(Qt::NoFocus);
    m_closeButton->setFocusPolicy(Qt::NoFocus);
    #endif
    connect(m_replaceLine, &LineBar::signal_sentText, this, &ReplaceBar::change, Qt::QueuedConnection);

    connect(this, &ReplaceBar::pressEsc, this, &ReplaceBar::replaceClose, Qt::QueuedConnection);

    //connect(m_replaceLine, &LineBar::pressEnter, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);         //Shielded by Hengbo for new demand.
    connect(m_withLine, &LineBar::returnPressed, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(m_replaceLine, &LineBar::pressCtrlEnter, this, &ReplaceBar::handleSkip,Qt::QueuedConnection);
    connect(m_withLine, &LineBar::pressCtrlEnter, this, &ReplaceBar::handleSkip,Qt::QueuedConnection);
    connect(m_replaceLine, &LineBar::pressAltEnter, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);
    connect(m_withLine, &LineBar::pressAltEnter, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);
    connect(m_replaceLine, &LineBar::pressMetaEnter, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
    connect(m_withLine, &LineBar::pressMetaEnter, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
    connect(m_replaceLine, &LineBar::returnPressed, this, &ReplaceBar::handleContentChanged, Qt::QueuedConnection);
    connect(m_replaceButton, &QPushButton::clicked, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(m_replaceSkipButton, &QPushButton::clicked, this, &ReplaceBar::handleSkip,Qt::QueuedConnection);
    connect(m_replaceRestButton, &QPushButton::clicked, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);

    connect(m_closeButton, &DIconButton::clicked, this, &ReplaceBar::replaceClose, Qt::QueuedConnection);
}

bool ReplaceBar::isFocus()
{
    return m_replaceLine->hasFocus();
}

void ReplaceBar::focus()
{
    m_replaceLine->lineEdit()->setFocus();
}

void ReplaceBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    // Try fill keyword with select text.
    m_withLine->lineEdit()->clear();
    m_replaceLine->lineEdit()->clear();
    m_replaceLine->lineEdit()->insert(text);
    m_replaceLine->lineEdit()->selectAll();

    // Show.
    show();

    // Save file info for back to position.
    m_replaceFile = file;
    m_replaceFileRow = row;
    m_replaceFileColumn = column;
    m_replaceFileSrollOffset = scrollOffset;

    // Focus.
    focus();
}

void ReplaceBar::handleSkip()
{
    emit replaceSkip(m_replaceFile, m_replaceLine->lineEdit()->text());
}

void ReplaceBar::replaceClose()
{
    searched = false;
    hide();
    emit sigReplacebarClose();
}

void ReplaceBar::handleContentChanged()
{
    updateSearchKeyword(m_replaceFile, m_replaceLine->lineEdit()->text());
}

void ReplaceBar::handleReplaceNext()
{
    if (!searched) {
        emit removeSearchKeyword();
        emit beforeReplace(m_replaceLine->lineEdit()->text());
    }
    replaceNext(m_replaceFile, m_replaceLine->lineEdit()->text(), m_withLine->lineEdit()->text());
    searched = true;
}

void ReplaceBar::handleReplaceRest()
{
    replaceRest(m_replaceLine->lineEdit()->text(), m_withLine->lineEdit()->text());
}

void ReplaceBar::handleReplaceAll()
{
    replaceAll(m_replaceLine->lineEdit()->text(), m_withLine->lineEdit()->text());
}

void ReplaceBar::hideEvent(QHideEvent *)
{
    searched = false;
    removeSearchKeyword();
}

bool ReplaceBar::focusNextPrevChild(bool)
{
    // Make keyword jump between two EditLine widgets.
    auto *editWidget = qobject_cast<LineBar *>(focusWidget());
    if (editWidget != nullptr) {
        if (editWidget == m_replaceLine) {
            m_withLine->lineEdit()->setFocus();

            return true;
        } else if (editWidget == m_withLine) {
            m_replaceLine->lineEdit()->setFocus();

            return true;
        }
    }

    return false;
}

void ReplaceBar::keyPressEvent(QKeyEvent *e)
{
    const QString &key = Utils::getKeyshortcut(e);
    if (key == "Esc") {
        QWidget::hide();
        emit sigReplacebarClose();
    }
    if (m_closeButton->hasFocus() && key == "Tab") {
        m_replaceLine->lineEdit()->setFocus();
    } else {
        DFloatingWidget::keyPressEvent(e);
    }
    if (key == "Enter") {
        if (m_replaceAllButton->hasFocus()) {
            m_replaceAllButton->click();
        }
        if (m_replaceButton->hasFocus()) {
            m_replaceButton->click();
        }
        if (m_replaceRestButton->hasFocus()) {
            m_replaceRestButton->click();
        }
        if (m_replaceSkipButton->hasFocus()) {
            m_replaceSkipButton->click();
        }
    }
}

void ReplaceBar::setMismatchAlert(bool isAlert)
{
    m_replaceLine->setAlert(isAlert);
}

void ReplaceBar::setsearched(bool _)
{
    searched = _;
}

void ReplaceBar::change()
{
    searched = false;
}
