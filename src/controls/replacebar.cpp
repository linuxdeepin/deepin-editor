// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "replacebar.h"
#include "../common/utils.h"

#include <QDebug>

// 不同布局模式下界面参数，不完全对应设计图固定值，调整后实际像素值和设计图对应
const int s_RBHeight = 60;
const QMargins s_RBContetsMargins = {16, 0, 10, 0};
const int s_RBCloseBtnSize = 30;
const int s_RBHeight_Compact = 40;
const QMargins s_RBContetsMarginsCompact = {16, 0, 6, 0};
const int s_RBCloseBtnSizeCompact = 26;
const int s_RBCloseIconSizeCompact = 27;

ReplaceBar::ReplaceBar(QWidget *parent)
    :  DFloatingWidget(parent)
{
    qCDebug(appLog) << "ReplaceBar constructor start";
    // Init.
    hide();
    setFixedHeight(60);
    qCDebug(appLog) << "ReplaceBar initialized with height 60";
    // Init layout and widgets.
    m_layout = new QHBoxLayout();
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(16, 0, 10, 0);
    m_layout->setAlignment(Qt::AlignVCenter);
    m_replaceLabel = new QLabel(tr("Find"));
    m_replaceLabel->setAccessibleName("ReplaceFindLabel");
    m_replaceLine = new LineBar();
    m_replaceLine->setObjectName("ReplaceLine");
    m_replaceLine->setAccessibleName("ReplaceFindInput");
    m_withLabel = new QLabel(tr("Replace With"));
    m_withLabel->setAccessibleName("ReplaceWithLabel");
    m_withLine = new LineBar();
    m_withLine->setObjectName("WithLine");
    m_withLine->setAccessibleName("ReplaceWithInput");
    m_replaceButton = new QPushButton(tr("Replace"));
    m_replaceButton->setObjectName("ReplaceButton_2");
    m_replaceButton->setAccessibleName("ReplaceNext");
    m_replaceSkipButton = new QPushButton(tr("Skip"));
    m_replaceSkipButton->setObjectName("ReplaceSkipButton");
    m_replaceSkipButton->setAccessibleName("ReplaceSkip");
    m_replaceRestButton = new QPushButton(tr("Replace Rest"));
    m_replaceRestButton->setObjectName("ReplaceRestButton");
    m_replaceRestButton->setAccessibleName("ReplaceRest");
    m_replaceAllButton = new QPushButton(tr("Replace All"));
    m_replaceAllButton->setObjectName("ReplaceAllButton");
    m_replaceAllButton->setAccessibleName("ReplaceAll");
    m_closeButton = new DIconButton(DStyle::SP_CloseButton);
    m_closeButton->setObjectName("ReplaceCloseButton");
    m_closeButton->setAccessibleName("CloseReplace");
    m_closeButton->setFlat(true);
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setEnabledCircle(true);
    m_closeButton->setIconSize(QSize(30, 30));

    m_layout->addWidget(m_replaceLabel);
    m_layout->addLayout(createVerticalLine(m_replaceLine));
    m_layout->addWidget(m_withLabel);
    m_layout->addLayout(createVerticalLine(m_withLine));
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

#ifdef DTKWIDGET_CLASS_DSizeMode
    updateSizeMode();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, &ReplaceBar::updateSizeMode);
#endif
    qCDebug(appLog) << "ReplaceBar constructor end";
}

bool ReplaceBar::isFocus()
{
    qCDebug(appLog) << "isFocus";
    return m_replaceLine->hasFocus();
}

void ReplaceBar::focus()
{
    qCDebug(appLog) << "focus";
    m_replaceLine->lineEdit()->setFocus();
    qCDebug(appLog) << "focus success";
}

void ReplaceBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    qCDebug(appLog) << "Activating replace bar for file:" << file
                      << "at row:" << row << "column:" << column;
    // Try fill keyword with select text.
    m_withLine->lineEdit()->clear();
    m_replaceLine->lineEdit()->clear();
    m_replaceLine->lineEdit()->insert(text);
    m_replaceLine->lineEdit()->selectAll();
    qCDebug(appLog) << "Initial search text set, length:" << text.length();

    // Show.
    show();
    qCDebug(appLog) << "Replace bar shown";

    // Save file info for back to position.
    m_replaceFile = file;
    m_replaceFileRow = row;
    m_replaceFileColumn = column;
    m_replaceFileSrollOffset = scrollOffset;
    qCDebug(appLog) << "Saved position info - row:" << row
                       << "column:" << column << "scrollOffset:" << scrollOffset;

    // Focus.
    focus();
    qCDebug(appLog) << "Focus set to replace input";
}

void ReplaceBar::handleSkip()
{
    qCDebug(appLog) << "handleSkip";
    emit replaceSkip(m_replaceFile, m_replaceLine->lineEdit()->text());
}

void ReplaceBar::replaceClose()
{
    qCDebug(appLog) << "replaceClose";
    searched = false;
    m_replaceLine->setMatchCount(0, 0);
    hide();
    emit sigReplacebarClose();
    qCDebug(appLog) << "replaceClose end";
}

void ReplaceBar::handleContentChanged()
{
    qCDebug(appLog) << "handleContentChanged";
    updateSearchKeyword(m_replaceFile, m_replaceLine->lineEdit()->text());
}

void ReplaceBar::slotUpdateMatchCount(int current, int total)
{
    qCDebug(appLog) << "ReplaceBar slotUpdateMatchCount current:" << current << "total:" << total;
    m_replaceLine->setMatchCount(current, total);
}

void ReplaceBar::handleReplaceNext()
{
    qCDebug(appLog) << "handleReplaceNext";
    QString searchText = m_replaceLine->lineEdit()->text();
    QString replaceText = m_withLine->lineEdit()->text();
    qCDebug(appLog) << "Replacing next occurrence, search length:"
                      << searchText.length() << "replace length:" << replaceText.length();
                      
    if (!searched) {
        qCDebug(appLog) << "First search, removing previous keywords";
        emit removeSearchKeyword();
        emit beforeReplace(searchText);
    }
    replaceNext(m_replaceFile, searchText, replaceText);
    searched = true;
    qCDebug(appLog) << "Replace next completed";
}

void ReplaceBar::handleReplaceRest()
{
    qCDebug(appLog) << "handleReplaceRest";
    replaceRest(m_replaceLine->lineEdit()->text(), m_withLine->lineEdit()->text());
}

void ReplaceBar::handleReplaceAll()
{
    qCDebug(appLog) << "handleReplaceAll";
    replaceAll(m_replaceLine->lineEdit()->text(), m_withLine->lineEdit()->text());
}

void ReplaceBar::hideEvent(QHideEvent *)
{
    qCDebug(appLog) << "hideEvent";
    searched = false;
    removeSearchKeyword();
}

bool ReplaceBar::focusNextPrevChild(bool)
{
    qCDebug(appLog) << "focusNextPrevChild";
    // Make keyword jump between two EditLine widgets.
    auto *editWidget = qobject_cast<LineBar *>(focusWidget());
    if (editWidget != nullptr) {
        qCDebug(appLog) << "editWidget" << editWidget;
        if (editWidget == m_replaceLine) {
            m_withLine->lineEdit()->setFocus();
            qCDebug(appLog) << "focus withLine";
            return true;
        } else if (editWidget == m_withLine) {
            m_replaceLine->lineEdit()->setFocus();
            qCDebug(appLog) << "focus replaceLine";
            return true;
        }
    }
    qCDebug(appLog) << "focusNextPrevChild end";
    return false;
}

void ReplaceBar::keyPressEvent(QKeyEvent *e)
{
    qCDebug(appLog) << "keyPressEvent";
    const QString &key = Utils::getKeyshortcut(e);
    if (key == "Esc") {
        qCDebug(appLog) << "Esc";
        QWidget::hide();
        emit sigReplacebarClose();
        qCDebug(appLog) << "sigReplacebarClose";
    }
    if (m_closeButton->hasFocus() && key == "Tab") {
        qCDebug(appLog) << "Tab";
        m_replaceLine->lineEdit()->setFocus();
    } else {
        DFloatingWidget::keyPressEvent(e);
    }
    if (key == "Enter") {
        qCDebug(appLog) << "Enter";
        if (m_replaceAllButton->hasFocus()) {
            qCDebug(appLog) << "replaceAllButton";
            m_replaceAllButton->click();
        }
        if (m_replaceButton->hasFocus()) {
            qCDebug(appLog) << "replaceButton";
            m_replaceButton->click();
        }
        if (m_replaceRestButton->hasFocus()) {
            qCDebug(appLog) << "replaceRestButton";
            m_replaceRestButton->click();
        }
        if (m_replaceSkipButton->hasFocus()) {
            qCDebug(appLog) << "replaceSkipButton";
            m_replaceSkipButton->click();
        }
    }
    qCDebug(appLog) << "keyPressEvent end";
}

/**
   @brief 根据界面布局模式 `DGuiApplicationHelper::isCompactMode()` 切换当前界面布局参数。
        需要注意，界面参数同设计图参数并非完全一致，而是按照实际的显示像素值进行比对。
 */
void ReplaceBar::updateSizeMode()
{
    qCDebug(appLog) << "updateSizeMode";
#ifdef DTKWIDGET_CLASS_DSizeMode
    bool isCompact = DGuiApplicationHelper::isCompactMode();
    qCDebug(appLog) << "Updating size mode, compact:" << isCompact;
    
    if (isCompact) {
        qCDebug(appLog) << "isCompact";
        setFixedHeight(s_RBHeight_Compact);
        m_closeButton->setFixedSize(s_RBCloseBtnSizeCompact, s_RBCloseBtnSizeCompact);
        m_closeButton->setIconSize(QSize(s_RBCloseIconSizeCompact, s_RBCloseIconSizeCompact));

        m_layout->setContentsMargins(s_RBContetsMarginsCompact);
        m_layout->invalidate();
    } else {
        qCDebug(appLog) << "is not compact";
        setFixedHeight(s_RBHeight);
        m_closeButton->setFixedSize(s_RBCloseBtnSize, s_RBCloseBtnSize);
        m_closeButton->setIconSize(QSize(s_RBCloseBtnSize, s_RBCloseBtnSize));

        m_layout->setContentsMargins(s_RBContetsMargins);
        m_layout->invalidate();
    }
#endif
    qCDebug(appLog) << "updateSizeMode end";
}

/**
   @brief 单独处理输入框 \a bar，由于Qt坐标机制，奇数处理存在累计误差，
    导致显示效果和预期存在差异，添加 spacer 强制输入框居中，组合完成返回布局。
 */
QVBoxLayout *ReplaceBar::createVerticalLine(QWidget *content) const
{
    qCDebug(appLog) << "createVerticalLine";
    QVBoxLayout *lineBarLayout = new QVBoxLayout();
    lineBarLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
    lineBarLayout->addWidget(content);
    lineBarLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
    qCDebug(appLog) << "createVerticalLine end";
    return lineBarLayout;
}

void ReplaceBar::setMismatchAlert(bool isAlert)
{
    qCDebug(appLog) << "setMismatchAlert";
    m_replaceLine->setAlert(isAlert);
}

void ReplaceBar::setsearched(bool _)
{
    qCDebug(appLog) << "setsearched";
    searched = _;
}

void ReplaceBar::change()
{
    qCDebug(appLog) << "change";
    searched = false;
}
