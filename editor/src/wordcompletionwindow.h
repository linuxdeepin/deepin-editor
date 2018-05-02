#ifndef WORD_COMPLETION_WINDOW_H
#define WORD_COMPLETION_WINDOW_H

#include <QWidget>
#include <DSimpleListView>

DWIDGET_USE_NAMESPACE

class WordCompletionWindow : public QWidget
{
    Q_OBJECT
    
public:
    WordCompletionWindow(QWidget *parent=0);
    ~WordCompletionWindow(); 
    
    void addWords(QStringList words);
    
    DSimpleListView *listview;
    QList<DSimpleListItem*> items;
    
    int windowWidth;
    int windowHeight;
    int lineHeight;
};	

#endif
