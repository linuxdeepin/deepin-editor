#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <DTabBar>

DWIDGET_USE_NAMESPACE

typedef QPixmap (* FileScreenshotFunc) (int index);

class TabWidget : public DTabBar
{
    Q_OBJECT
    
public:
    TabWidget();
    
    QPixmap createDragPixmapFramTab(int index, const QStyleOptionTab &option, QPoint *hotspot) const;
    QMimeData *createMimeDataFromTab(int index, const QStyleOptionTab &option) const;
    bool canInsertFromMimeData(int index, const QMimeData *source) const;
    void insertFromMimeData(int index, const QMimeData *source);    
    
    QList<QString> tabFiles;
};

#endif
