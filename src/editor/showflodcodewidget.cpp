// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "showflodcodewidget.h"
#include <QFileInfo>
#include <QDebug>
#include <QPalette>
#include <QVBoxLayout>
#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>
#include <QGraphicsDropShadowEffect>
#include <QXmlStreamReader>

namespace KSyntaxHighlighting {
    class SyntaxHighlighter;
}

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

ShowFlodCodeWidget::ShowFlodCodeWidget(DWidget *parent)
    : DFrame(parent)
{
    qCDebug(appLog) << "ShowFlodCodeWidget created with parent:" << parent;
    //setFrameRounded(false);
    QGraphicsDropShadowEffect *effert = new QGraphicsDropShadowEffect(this);
    effert->setOffset(0,6);
    effert->setBlurRadius(20);
    QColor color(0,0,0);
    color.setAlphaF(0.2);
    effert->setColor(color);
    this->setGraphicsEffect(effert);
    QVBoxLayout *pSubLayout = new QVBoxLayout();
    m_pContentEdit = new DPlainTextEdit(this);
    m_pContentEdit->setObjectName("PContentEdit");
    m_pContentEdit->setAccessibleName("PContentEdit");
    m_pContentEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    m_pContentEdit->setFrameStyle(0);
    m_pContentEdit->setReadOnly(true);
    m_pContentEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pContentEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pSubLayout->addWidget(m_pContentEdit);
    this->setLayout(pSubLayout);
    m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(m_pContentEdit->document());

    // 更新单独添加的高亮格式文件
    m_repository.addCustomSearchPath(KF5_HIGHLIGHT_PATH);
}

ShowFlodCodeWidget::~ShowFlodCodeWidget()
{
    qCDebug(appLog) << "ShowFlodCodeWidget destroyed";
    m_highlighter->deleteLater();
    m_highlighter = nullptr;
}

void ShowFlodCodeWidget::clear()
{
    qCDebug(appLog) << "ShowFlodCodeWidget clear content";
    m_pContentEdit->document()->clear();
    m_nTextWidth = 0;
    adjustSize();
    qCDebug(appLog) << "ShowFlodCodeWidget::clear() exit";
}

void ShowFlodCodeWidget::initHighLight(QString filepath, bool bIsLight)
{
    qCInfo(appLog) << "ShowFlodCodeWidget initHighLight - file:" << filepath << "light theme:" << bIsLight;
    if (m_highlighter != nullptr) {
        if (!bIsLight) {
            m_highlighter->setTheme(m_repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
        } else {
            m_highlighter->setTheme(m_repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
        }
    }
   // m_highlighter->rehighlight();
    const auto def = m_repository.definitionForFileName(QFileInfo(filepath).fileName().toLower());
    m_highlighter->setDefinition(def);
    qCDebug(appLog) << "Syntax definition set to:" << def.name();
}

void ShowFlodCodeWidget::setStyle(bool bIsLineWrap)
{
    qCDebug(appLog) << "ShowFlodCodeWidget setStyle - lineWrap:" << bIsLineWrap;
    QPalette pa = palette();
    QColor color(25,25,25);

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        qCDebug(appLog) << "Setting dark theme style";
        color.setAlphaF(0.8);
        pa.setColor(QPalette::Base,color);
        m_pContentEdit->setPalette(pa);
        pa.setColor(QPalette::Base,QColor(25,25,25));
        setPalette(pa);
    } else {
        qCDebug(appLog) << "Setting light theme style";
        color = QColor(247,247,247);
        color.setAlphaF(0.6);
        pa.setColor(QPalette::Base,color);
        m_pContentEdit->setPalette(pa);
        pa.setColor(QPalette::Base,QColor(247,247,247));
        setPalette(pa);
    }

    if (bIsLineWrap) {
        qCDebug(appLog) << "Setting line wrap mode to WidgetWidth";
        m_pContentEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    } else {
        qCDebug(appLog) << "Setting line wrap mode to NoWrap";
        m_pContentEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    qCDebug(appLog) << "ShowFlodCodeWidget::setStyle() exit";
}

void ShowFlodCodeWidget::hideFirstBlock()
{
    qCDebug(appLog) << "ShowFlodCodeWidget hideFirstBlock";
    m_pContentEdit->document()->firstBlock().setVisible(false);
    m_pContentEdit->document()->lastBlock().setVisible(false);
    int editHight = 0;

    for (auto block = m_pContentEdit->document()->firstBlock(); block.isValid(); block = block.next()) {

        if (block == m_pContentEdit->document()->lastBlock()) {
            qCDebug(appLog) << "Reached last block, breaking loop";
            break;
        }

        editHight += m_pContentEdit->document()->documentLayout()->blockBoundingRect(block).toRect().height();
    }

    m_pContentEdit->setFixedHeight(editHight + 10);
    qCDebug(appLog) << "Adjusted height to:" << editHight + 10;
    qCDebug(appLog) << "ShowFlodCodeWidget::hideFirstBlock() exit";
}

void ShowFlodCodeWidget::appendText(QString strText, int maxWidth)
{
    qCDebug(appLog) << "ShowFlodCodeWidget appendText - text length:" << strText.length() << "maxWidth:" << maxWidth;
    int textWidth = m_pContentEdit->fontMetrics().horizontalAdvance(strText) + 10;

    if (m_nTextWidth < textWidth) {
        qCDebug(appLog) << "Updating text width, old:" << m_nTextWidth << ", new:" << textWidth;
        m_nTextWidth = textWidth;
        if (m_nTextWidth > maxWidth - 50) {
            qCDebug(appLog) << "Adjusting text width to fit maxWidth - 50";
            m_nTextWidth = maxWidth - 50;
        }
    }

    m_pContentEdit->setFixedWidth(m_nTextWidth);
    qCDebug(appLog) << "Text width set to:" << m_nTextWidth;

    if (m_pContentEdit->document()->isEmpty()) {
        qCDebug(appLog) << "Document is empty, setting plain text";
        m_pContentEdit->setPlainText(strText);
    } else {
        qCDebug(appLog) << "Document not empty, appending plain text";
        m_pContentEdit->appendPlainText(strText);
    }

    QTextCursor cursor = m_pContentEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_pContentEdit->setTextCursor(cursor);
    qCDebug(appLog) << "ShowFlodCodeWidget::appendText() exit";
}

