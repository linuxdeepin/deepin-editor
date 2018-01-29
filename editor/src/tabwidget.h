#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <DTabBar>
#include <QMenu>

DWIDGET_USE_NAMESPACE

typedef QPixmap (* FileScreenshotFunc) (int index);

class TabWidget : public DTabBar
{
    Q_OBJECT
    
    bool eventFilter(QObject *, QEvent *event);
    
public:
    TabWidget();
    
    QPixmap createDragPixmapFromTab(int index, const QStyleOptionTab &option, QPoint *hotspot) const;
    QMimeData *createMimeDataFromTab(int index, const QStyleOptionTab &option) const;
    bool canInsertFromMimeData(int index, const QMimeData *source) const;
    void insertFromMimeData(int index, const QMimeData *source);    
    
    QList<QString> tabFiles;
                           
signals:
    void closeTab(int index);
    void closeOtherTabs(int index);
    
public slots:
    void handleCloseTab();
    void handleCloseOtherTabs();
    
private:
    QMenu *menu;
    QAction *closeTabAction;
    QAction *closeOtherTabAction;
    
    int rightClickTab;
};

#endif
