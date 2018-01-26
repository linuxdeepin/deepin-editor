#include "tabwidget.h"
#include "window.h"
#include "texteditor.h"
#include <QDebug>

TabWidget::TabWidget()
{
    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);
    
    QColor dropColor("#333333");
    dropColor.setAlpha(128);
    setMaskColor(dropColor);
    
    setStartDragDistance(20);
}

QPixmap TabWidget::createDragPixmapFramTab(int index, const QStyleOptionTab &, QPoint *) const
{
    TextEditor *textEditor = static_cast<Window*>(this->window())->getTextEditor(tabFiles[index]);
    
    int width = textEditor->width();
    int height = textEditor->height();
    QPixmap pixmap(width, height);
    textEditor->render(&pixmap, QPoint(), QRegion(0, 0, width, height));
    
    return pixmap.scaled(width / 5, height / 5);
}

QMimeData* TabWidget::createMimeDataFromTab(int index, const QStyleOptionTab &) const
{
    QString tabPath = tabFiles[index];
    QString tabName = tabText(index);
    TextEditor *textEditor = static_cast<Window*>(this->window())->getTextEditor(tabFiles[index]);
    QString tabContent = textEditor->toPlainText();
    
    QMimeData *mimeData = new QMimeData;
    mimeData->setText(QString("%1\n%2\n%3").arg(tabName, tabPath, tabContent));
    
    return mimeData;
}

bool TabWidget::canInsertFromMimeData(int, const QMimeData *) const
{
    return true;
}

void TabWidget::insertFromMimeData(int, const QMimeData *source) 
{
    // Create new tab create drop from other deepin-editor.
    QStringList dropContent = source->text().split("\n");
    QString tabName = dropContent[0];
    QString tabPath = dropContent[1];
    QString tabContent = source->text().remove(0, tabName.size() + tabPath.size() + 2); // 2 mean two \n char
    
    static_cast<Window*>(this->window())->addTabWithContent(tabName, tabPath, tabContent);
}
