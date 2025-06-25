// SPDX-FileCopyrightText: 2011-2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "jumplinebar.h"

// #include <DThemeManager>
#include <DGuiApplicationHelper>

#include <QDebug>

// 各项组件的默认大小
const int s_nJumpLineBarWidth = 232;
const int s_nJumpLineBarHeight = 60;
const QMargins s_JLBDefaultMarigins = {10, 0, 10, 0};
const int s_nJumpLineBarSpinBoxWidth = 124;
const int s_nJumpLineBarSPinBoxHeight = 36;
const int s_nCloseButtonSize = 30;
// 水平方向与边界间距
const int s_nJumpLineBarHorizenMargin = 10;
// 紧凑模式下的控件高度调整(相较设计图调整，以使得实际像素与设计图一致)
const int s_nJumpLineBarWidthCompact = 212;
const int s_nJumpLineBarHeightCompact = 40;
const QMargins s_JLBCompactMarigins = {10, 0, 0, 0};
const int s_nJumpLineBarSPinBoxHeightCompact = 28;
const int s_nCloseButtonSizeCompact = 27;

JumpLineBar::JumpLineBar(DFloatingWidget *parent)
    : DFloatingWidget(parent)
{
    qDebug() << "JumpLineBar constructor start";
    // Init layout and widgets.
    m_layout = new QHBoxLayout();
    m_layout->setContentsMargins(10, 0, 10, 0);
    m_layout->setAlignment(Qt::AlignVCenter);
    m_layout->setSpacing(5);
    qDebug() << "Base layout initialized";
    m_closeButton = new DIconButton(DStyle::SP_CloseButton);
    m_closeButton->setIconSize(QSize(30, 30));
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setEnabledCircle(true);
    m_closeButton->setFlat(true);

    m_label = new QLabel();
    m_label->setText(tr("Go to Line: "));
    // 按文本长度计算显示宽度，不同语言下翻译文本长度不一，需完整显示
    m_label->setFixedWidth(fontMetrics().horizontalAdvance(m_label->text()));
    m_pSpinBoxInput = new DSpinBox;
    m_pSpinBoxInput->setFixedSize(s_nJumpLineBarSpinBoxWidth, s_nJumpLineBarSPinBoxHeight);
    m_pSpinBoxInput->lineEdit()->clear();
    m_pSpinBoxInput->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pSpinBoxInput->installEventFilter(this);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_pSpinBoxInput);
    m_layout->addWidget(m_closeButton);
    this->setLayout(m_layout);

    // 初始化浮动条宽度，根据文本长度计算
    setFixedHeight(s_nJumpLineBarHeight);
    setFixedWidth(m_layout->sizeHint().width() + s_nJumpLineBarHorizenMargin);

    connect(this, &JumpLineBar::pressEsc, this, &JumpLineBar::jumpCancel, Qt::QueuedConnection);
    connect(m_pSpinBoxInput->lineEdit(), &QLineEdit::returnPressed, this, &JumpLineBar::jumpConfirm, Qt::QueuedConnection);
    connect(m_pSpinBoxInput->lineEdit(), &QLineEdit::textChanged, this, &JumpLineBar::handleLineChanged, Qt::QueuedConnection);
    connect(m_closeButton, &DIconButton::clicked, this, &JumpLineBar::close, Qt::QueuedConnection);

#ifdef DTKWIDGET_CLASS_DSizeMode
    // 初始化布局模式
    updateSizeMode();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, &JumpLineBar::updateSizeMode);
#endif
    qDebug() << "JumpLineBar constructor end";
}

JumpLineBar::~JumpLineBar()
{
    qDebug() << "JumpLineBar destructor";
}

void JumpLineBar::focus()
{
    qDebug() << "focus";
    m_pSpinBoxInput->lineEdit()->setFocus();
    qDebug() << "focus success";
}

bool JumpLineBar::isFocus()
{
    qDebug() << "isFocus";
    return m_pSpinBoxInput->lineEdit()->hasFocus();
}

void JumpLineBar::activeInput(QString file, int row, int column, int lineCount, int scrollOffset)
{
    qDebug() << "Activating input for file:" << file
                   << "row:" << row << "column:" << column
                   << "lineCount:" << lineCount;
    // Save file info for back to line.
    m_jumpFile = file;
    m_rowBeforeJump = row;
    m_columnBeforeJump = column;
    m_jumpFileScrollOffset = scrollOffset;
    m_lineCount = lineCount;
    // 调整为 0~lineCount ，0已被处理不允许首位输入，不影响仅单行的情况
    // 设置 range 后会自动调整输入范围，不使用 clear() 防止在读取文件时已输入的行号被清空
    m_pSpinBoxInput->setRange(0, lineCount);
    int lineWidth = QString::number(lineCount).size() * fontMetrics().horizontalAdvance('9');
    if (m_pSpinBoxInput->minimumWidth() < lineWidth) {
        qDebug() << "lineWidth" << lineWidth;
        m_pSpinBoxInput->setFixedWidth(lineWidth);
    } else {
        qDebug() << "lineWidth" << lineWidth;
        m_pSpinBoxInput->setFixedWidth(s_nJumpLineBarSpinBoxWidth);
    }
    qDebug() << "setFixedWidth";
    setFixedWidth(m_layout->sizeHint().width() + s_nJumpLineBarHorizenMargin);

    // Clear line number.
    if (m_pSpinBoxInput->lineEdit()->text().toInt() > lineCount) {
        qDebug() << "Clearing invalid line number input";
        m_pSpinBoxInput->lineEdit()->setText("");
    }
    qDebug() << "activeInput end";
}

void JumpLineBar::handleFocusOut()
{
    qDebug() << "handleFocusOut";
    //hide();
    lostFocusExit();
    qDebug() << "handleFocusOut end";
}

void JumpLineBar::handleLineChanged()
{
    qDebug() << "handleLineChanged";
    QString content = m_pSpinBoxInput->lineEdit()->text();
    if (content != "") {
        qDebug() << "content" << content;
        if (content.toInt() == 0) {
            m_pSpinBoxInput->clear();
            qDebug() << "clear";
            return;
        }
        jumpToLine(m_jumpFile, content.toInt(), false);
    }
    qDebug() << "handleLineChanged end";
}

void JumpLineBar::jumpCancel()
{
    qDebug() << "jumpCancel";
    hide();
    // esc键不跳转　返回当前
    // backToPosition(m_jumpFile, m_rowBeforeJump, m_columnBeforeJump, m_jumpFileScrollOffset);
}

void JumpLineBar::jumpConfirm()
{
    qDebug() << "jumpConfirm";
    QString content = m_pSpinBoxInput->lineEdit()->text();
    if (content != "") {
        qDebug() << "content" << content;
        jumpToLine(m_jumpFile, content.toInt(), true);
    }
    qDebug() << "jumpConfirm end";
}

void JumpLineBar::slotFocusChanged(bool bFocus)
{
    qDebug() << "slotFocusChanged";
    if (bFocus == false) {
        lostFocusExit();
        qDebug() << "slotFocusChanged end";
    }
}

// Hide 跳转到行窗口时，需要清空编辑框中的内容
void JumpLineBar::hide()
{
    qDebug() << "hide";
    m_pSpinBoxInput->clear();
    DFloatingWidget::hide();
    qDebug() << "hide end";
}

int JumpLineBar::getLineCount()
{
    qDebug() << "getLineCount";
    return m_lineCount;
}

bool JumpLineBar::eventFilter(QObject *pObject, QEvent *pEvent)
{
    qDebug() << "eventFilter";
    if (pObject == m_pSpinBoxInput) {
        if (pEvent->type() == QEvent::FocusOut) {
            qDebug() << "FocusOut";
            handleFocusOut();
            /**
             * 规避当DSpinBox输入框里为空且失去focus焦点时会显示上一次输入的数值内容的问题
             */
            if (m_pSpinBoxInput->lineEdit()->text() == "") {
                m_pSpinBoxInput->lineEdit()->clear();
            }
        }
    }
    qDebug() << "eventFilter end";
    return DFloatingWidget::eventFilter(pObject, pEvent);
}

/**
   @brief 根据界面布局模式 `DGuiApplicationHelper::isCompactMode()` 切换当前界面布局参数。
        需要注意，界面参数同设计图参数并非完全一致，而是按照实际的显示像素值进行比对。
 */
void JumpLineBar::updateSizeMode()
{
    qDebug() << "updateSizeMode";
#ifdef DTKWIDGET_CLASS_DSizeMode
    bool isCompact = DGuiApplicationHelper::isCompactMode();
    qDebug() << "Updating size mode, compact:" << isCompact;
    
    if (isCompact) {
        qDebug() << "isCompact";
        m_layout->setContentsMargins(s_JLBCompactMarigins);
        m_closeButton->setIconSize(QSize(s_nCloseButtonSizeCompact, s_nCloseButtonSizeCompact));
        m_closeButton->setFixedSize(QSize(s_nCloseButtonSizeCompact, s_nCloseButtonSizeCompact));
        m_pSpinBoxInput->setFixedHeight(s_nJumpLineBarSPinBoxHeightCompact);
        setMinimumWidth(s_nJumpLineBarWidthCompact);
        setFixedHeight(s_nJumpLineBarHeightCompact);
    } else {
        qDebug() << "is not compact";
        m_layout->setContentsMargins(s_JLBDefaultMarigins);
        m_closeButton->setIconSize(QSize(s_nCloseButtonSize, s_nCloseButtonSize));
        m_closeButton->setFixedSize(QSize(s_nCloseButtonSize, s_nCloseButtonSize));
        m_pSpinBoxInput->setFixedHeight(s_nJumpLineBarSPinBoxHeight);
        setMinimumWidth(s_nJumpLineBarWidth);
        setFixedHeight(s_nJumpLineBarHeight);
    }
#endif
    qDebug() << "updateSizeMode end";
}

