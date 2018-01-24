#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <DTabBar>
#include "editor.h"

DWIDGET_USE_NAMESPACE

typedef QPixmap (* FileScreenshotFunc) (int index);

class TabWidget : public DTabBar
{
    Q_OBJECT
    
public:
    TabWidget(QMap<QString, Editor*> *editorMap);
    
    QPixmap createDragPixmapFramTab(int index, const QStyleOptionTab &option, QPoint *hotspot) const;
    
    QList<QString> tabFiles;
    
private:
    QMap<QString, Editor*> *editorMap;
};

#endif
