#ifndef FINDBAR_H
#define FINDBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include "dtextbutton.h"
#include "linebar.h"

DWIDGET_USE_NAMESPACE

class FindBar : public QWidget
{
    Q_OBJECT
    
public:
    FindBar(QWidget *parent = 0);
    
    void activeInput(QString text, QString file, int row, int column, int scrollOffset);
    
    void focus();
    
    bool isFocus();
    
signals:
    void backToPosition(QString file, int row, int column, int scrollOffset);
    void updateSearchKeyword(QString file, QString keyword);
    void findNext();
    void findPrev();
    void cleanMatchKeyword();
    
public slots:
    void back();
    void handleContentChanged();
    
protected:
    void paintEvent(QPaintEvent *event);
    void hideEvent(QHideEvent *event);
    
private:
    QHBoxLayout *layout;
    QLabel *findLabel;
    LineBar *editLine;
    
    DTextButton *findNextButton;
    DTextButton *findPrevButton;
    
    QString findFile;
    int findFileRow;
    int findFileColumn;
    int findFileSrollOffset;
};

#endif
