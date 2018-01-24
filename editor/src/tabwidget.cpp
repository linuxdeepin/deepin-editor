#include "tabwidget.h"
#include "texteditor.h"
#include <QDebug>

TabWidget::TabWidget(QMap<QString, Editor*> *map)
{
    editorMap = map;

    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);
    
    setStartDragDistance(20);
}

QPixmap TabWidget::createDragPixmapFramTab(int index, const QStyleOptionTab &, QPoint *) const
{
    TextEditor *textEditor = (*editorMap)[tabFiles[index]]->textEditor;
    
    int width = textEditor->width();
    int height = textEditor->height();
    QPixmap pixmap(width, height);
    textEditor->render(&pixmap, QPoint(), QRegion(0, 0, width, height));
    
    return pixmap.scaled(width / 5, height / 5);
}
