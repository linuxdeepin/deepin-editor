// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "themepanel.h"
#include "../widgets/window.h"

#include <QPropertyAnimation>
#include <QScroller>
#include <QVBoxLayout>
#include <QPainter>
#include <QTimer>
#include <QDebug>

ThemePanel::ThemePanel(QWidget *parent)
    : QWidget(parent),
      m_themeView(new ThemeListView),
      m_themeModel(new ThemeListModel),
      m_window(static_cast<Window *>(parent))
{
    qCDebug(appLog) << "ThemePanel constructor";
    // init view.
    m_themeView->setAccessibleName("ThemeListView");
    m_themeView->setObjectName("ThemeView");
    m_themeView->setModel(m_themeModel);
    m_themeView->setItemDelegate(new ThemeItemDelegate(this));

    QScroller::grabGesture(m_themeView, QScroller::TouchGesture);

    // init layout.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_themeView);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setFixedWidth(250);

    QWidget::hide();
    connect(m_themeView, &ThemeListView::focusOut, this, &ThemePanel::hide);
    connect(m_themeView, &ThemeListView::themeChanged, this, &ThemePanel::themeChanged);
    connect(m_themeModel, &ThemeListModel::requestCurrentIndex, this, [=] (const QModelIndex &idx) {
        m_themeView->setCurrentIndex(idx);
        m_themeView->scrollTo(idx);
    });
    qCDebug(appLog) << "ThemePanel initialized";
}

ThemePanel::~ThemePanel()
{
    qCDebug(appLog) << "ThemePanel destructor";
}

void ThemePanel::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);

    QPainterPath backgroundPath;
    backgroundPath.addRect(QRect(rect().x() + 1, rect().y(), rect().width() - 1, rect().height()));
    painter.setOpacity(0.9);
    painter.fillPath(backgroundPath, m_backgroundColor);

    QPainterPath separatorPath;
    separatorPath.addRect(QRect(rect().x(), rect().y(), 1, rect().height()));
    painter.setOpacity(0.1);
    painter.fillPath(separatorPath, m_frameColor);
}
void ThemePanel::setBackground(const QString &color)
{
    qCDebug(appLog) << "Setting panel background color:" << color;
    m_backgroundColor = QColor(color);

    if (m_backgroundColor.lightness() < 128) {
        qCDebug(appLog) << "Background color is dark";
        m_frameColor = m_frameDarkColor;
    } else {
        qCDebug(appLog) << "Background color is light";
        m_frameColor = m_frameLightColor;
    }

    update();
    qCDebug(appLog) << "Panel background color set";
}

void ThemePanel::popup()
{
    qCDebug(appLog) << "Showing theme panel with animation";
    // parent 非 Window（或为空）时 m_window 无效，直接早退避免空指针解引用
    if (!m_window) {
        qCWarning(appLog) << "Theme panel has no host window, skip popup";
        QWidget::show();
        return;
    }
    QWidget::show();
    QWidget::raise();

    m_themeView->setFocus();

    QRect rect = geometry();
    QRect windowRect = m_window->geometry();
    QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(250);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setStartValue(QRect(windowRect.width(), rect.y(), rect.width(), rect.height()));
    animation->setEndValue(QRect(windowRect.width() - rect.width(), rect.y(), rect.width(), rect.height()));
    animation->start();

    connect(animation, &QPropertyAnimation::valueChanged, this, [=] { m_themeView->adjustScrollbarMargins(); });
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);
    qCDebug(appLog) << "popup end";
}

void ThemePanel::hide()
{
    qCDebug(appLog) << "Hiding theme panel with animation";
    // parent 非 Window（或为空）时 m_window 无效，直接早退避免空指针解引用
    if (!m_window) {
        qCWarning(appLog) << "Theme panel has no host window, hide directly";
        QWidget::hide();
        return;
    }
    QRect rect = geometry();
    QRect windowRect = m_window->geometry();
    QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(250);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setStartValue(QRect(windowRect.width() - rect.width(), rect.y(), rect.width(), rect.height()));
    animation->setEndValue(QRect(windowRect.width(), rect.y(), rect.width(), rect.height()));

    animation->start();

    connect(animation, &QPropertyAnimation::finished, this, &QWidget::hide);
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);
    qCDebug(appLog) << "hide end";
}

void ThemePanel::setFrameColor(const QString &selectedColor, const QString &normalColor)
{
    qCDebug(appLog) << "Setting frame color:" << selectedColor << normalColor;
    m_themeModel->setFrameColor(selectedColor, normalColor);
}

void ThemePanel::setSelectionTheme(const QString &path)
{
    qCDebug(appLog) << "Setting selected theme path:" << path;
    m_themeModel->setSelection(path);
}
