#include "tab_close_button.h"
#include "utils.h"
#include <QDebug>

TabCloseButton::TabCloseButton(QPushButton *parent) : QPushButton(parent)
{
    installEventFilter(this);  // add event filter
    setMouseTracking(true);

    isFocus = false;
    isPress = false;
    isVisible = false;
    
    normalImg = QImage(Utils::getQrcPath("tab_close_normal.png"));
    hoverImg = QImage(Utils::getQrcPath("tab_close_hover.png"));
    pressImg = QImage(Utils::getQrcPath("tab_close_press.png"));
    
    setFixedSize(normalImg.width(), normalImg.height());
}

TabCloseButton::~TabCloseButton()
{
}

void TabCloseButton::paintEvent(QPaintEvent *)
{
    if (isVisible) {
        // Init.
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QString status = "NORMAL";
        if (isFocus) {
            if (isPress) {
                status = "PRESS";
            } else {
                status = "HOVER";
            }
        }
    
        // Draw.
        painter.setOpacity(1);
        int iconX = rect().x() + (rect().width() - normalImg.width()) / 2;
        int iconY = rect().y() + (rect().height() - normalImg.height()) / 2;
        if (status == "NORMAL") {
            painter.drawImage(QPoint(iconX, iconY), normalImg);
        } else if (status == "PRESS") {
            painter.drawImage(QPoint(iconX, iconY), pressImg);
        } else if (status == "HOVER") {
            painter.drawImage(QPoint(iconX, iconY), hoverImg);
        }
    }
    
}

void TabCloseButton::setButtonVisible(bool visible)
{
    isVisible = visible;
    
    repaint();
}

bool TabCloseButton::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        isPress = true;
        repaint();
    } else if (event->type() == QEvent::MouseButtonRelease) {
        isPress = false;
        repaint();
    } else if (event->type() == QEvent::Enter) {
        isFocus = true;
        repaint();
    } else if (event->type() == QEvent::Leave) {
        isFocus = false;
        repaint();
    }

    return false;
}
