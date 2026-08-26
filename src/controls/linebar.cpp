// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "linebar.h"
#include "../common/utils.h"

#include <DGuiApplicationHelper>
#include <DFontSizeManager>

#include <QDebug>
#include <QGraphicsOpacityEffect>

// 不同布局模式(紧凑)
const int s_nLineBarHeight = 36;
const int s_nLineBarHeightCompact = 24;

// 计数 label 与自绘清除按钮的布局参数（像素）
const int s_nClearButtonSize = 22;     // 自绘清除按钮尺寸
const int s_nLabelButtonSpacing = 6;   // label 与清除按钮之间的间距
const int s_nRightMargin = 6;          // 清除按钮右边缘距输入框右边缘的间距

LineBar::LineBar(DLineEdit *parent)
    : DLineEdit(parent)
{
    qDebug() << "LineBar constructor start";

    // ★ 关键：禁用 DLineEdit 内置清除按钮。
    // 原因：Qt6 下 QLineEdit::addAction(TrailingPosition) 和 DLineEdit::setRightWidgets
    // 都把自定义 widget 放在内置清除按钮的右侧，无法让计数 label 处于清除按钮左侧，
    // 且 setRightWidgets 会压缩文本区域。
    setClearButtonEnabled(false);

    // label 和清除按钮直接 parent 到内嵌 lineEdit，覆盖在输入框内部
    m_matchCountLabel = new QLabel(lineEdit());
    int fontsize = DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T9);
    QFont labelFont = m_matchCountLabel->font();
    labelFont.setPixelSize(fontsize);
    m_matchCountLabel->setFont(labelFont);
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(m_matchCountLabel);
    opacityEffect->setOpacity(0.7);
    m_matchCountLabel->setGraphicsEffect(opacityEffect);
    m_matchCountLabel->hide();

    m_clearButton = new DIconButton(QStyle::SP_LineEditClearButton, lineEdit());
    m_clearButton->setObjectName("ClearButton");
    m_clearButton->setAccessibleName("ClearButton");
    m_clearButton->setFixedSize(s_nClearButtonSize, s_nClearButtonSize);
    m_clearButton->setIconSize(QSize(s_nClearButtonSize, s_nClearButtonSize));
    m_clearButton->setFocusPolicy(Qt::NoFocus);
    m_clearButton->hide();

    m_autoSaveInternal = 50;
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    qDebug() << "Auto-save timer initialized with interval:" << m_autoSaveInternal << "ms";

    connect(m_autoSaveTimer, &QTimer::timeout, this, &LineBar::handleTextChangeTimer);
    connect(this, &DLineEdit::textEdited, this, &LineBar::sendText, Qt::QueuedConnection);
    connect(this, &DLineEdit::textChanged, this, &LineBar::handleTextChanged, Qt::QueuedConnection);
    qDebug() << "Signal connections established";

    // 自绘清除按钮：点击清空文本；可见性跟随文本是否为空
    connect(m_clearButton, &DIconButton::clicked, this, [this]() {
        lineEdit()->clear();
    });

#ifdef DTKWIDGET_CLASS_DSizeMode
    setFixedHeight(DGuiApplicationHelper::isCompactMode() ? s_nLineBarHeightCompact : s_nLineBarHeight);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [this](){
        setFixedHeight(DGuiApplicationHelper::isCompactMode() ? s_nLineBarHeightCompact : s_nLineBarHeight);
    });
#endif
}

void LineBar::handleTextChangeTimer()
{
    qDebug() << "Text change timer triggered, emitting contentChanged";
    // Emit contentChanged signal.
    contentChanged();
}

void LineBar::handleTextChanged(const QString &str)
{
    // Stop timer if new character is typed, avoid unused timer run.
    if (m_autoSaveTimer->isActive()) {
        qDebug() << "Restarting text change timer";
        m_autoSaveTimer->stop();
    }
    if(str.isEmpty()) {
        qDebug() << "Text cleared, disabling alert";
        setAlert(false);
        m_clearButton->hide();          // 文本为空时隐藏清除按钮
    } else {
        m_clearButton->show();          // 有文本时显示清除按钮
    }
    // Start new timer.
    m_autoSaveTimer->start(m_autoSaveInternal);
    qDebug() << "Text changed, length:" << str.length();
}

void LineBar::sendText(QString t)
{
    emit signal_sentText(t);
}

void LineBar::focusOutEvent(QFocusEvent *e)
{
    qDebug() << "Focus lost";
    // Emit focus out signal.
    focusOut();

    // Throw event out avoid DLineEdit can't hide cursor after lost focus.
    DLineEdit::focusOutEvent(e);
}

void LineBar::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeyshortcut(e);
    Qt::KeyboardModifiers modifiers = e->modifiers();
    qDebug() << "Key pressed:" << key << "modifiers:" << modifiers;

    if(modifiers == Qt::ControlModifier && e->text() == "\r"){
       qDebug() << "Ctrl+Enter pressed";
       pressCtrlEnter();
    }else if(modifiers == Qt::AltModifier && e->text() == "\r"){
       qDebug() << "Alt+Enter pressed";
       pressAltEnter();
    }else if(modifiers == Qt::MetaModifier && e->text() == "\r"){
       qDebug() << "Meta+Enter pressed";
       pressMetaEnter();
    }else if(modifiers == Qt::NoModifier && e->text() == "\r"){
       qDebug() << "Enter pressed";
       pressEnter();
    }else {
      // Pass event to DLineEdit continue, otherwise you can't type anything after here. ;)
       DLineEdit::keyPressEvent(e);
    }
}

void LineBar::setMatchCount(int current, int total)
{
    if (total == 0) {
        m_matchCountLabel->hide();
    } else {
        m_matchCountLabel->setText(tr("第%1/%2项").arg(current).arg(total));
        m_matchCountLabel->show();
    }
    // label 文本变化导致宽度变化，重新计算坐标
    updateRightWidgetsGeometry();
}

void LineBar::resizeEvent(QResizeEvent *event)
{
    DLineEdit::resizeEvent(event);
    updateRightWidgetsGeometry();
}

void LineBar::updateRightWidgetsGeometry()
{
    QWidget *le = lineEdit();
    if (!le) {
        return;
    }

    const int editWidth  = le->width();
    const int editHeight = le->height();
    const int buttonWidth  = m_clearButton->width();
    const int buttonHeight = m_clearButton->height();
    const int labelWidth   = m_matchCountLabel->sizeHint().width();
    const int labelHeight  = m_matchCountLabel->sizeHint().height();

    // button 垂直居中，右边缘距 lineEdit 右边缘 s_nRightMargin
    const int buttonX = editWidth - s_nRightMargin - buttonWidth;
    const int buttonY = (editHeight - buttonHeight) / 2;
    m_clearButton->setGeometry(buttonX, buttonY, buttonWidth, buttonHeight);

    // label 垂直居中，水平紧贴 button 左侧 s_nLabelButtonSpacing
    const int labelX = buttonX - s_nLabelButtonSpacing - labelWidth;
    const int labelY = (editHeight - labelHeight) / 2;
    m_matchCountLabel->setGeometry(labelX, labelY, labelWidth, labelHeight);
}
