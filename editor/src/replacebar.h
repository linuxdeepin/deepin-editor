#ifndef REPLACEBAR_H
#define REPLACEBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include "dtextbutton.h"
#include "linebar.h"

DWIDGET_USE_NAMESPACE

class ReplaceBar : public QWidget
{
    Q_OBJECT
    
public:
    ReplaceBar(QWidget *parent = 0);
    
    void activeInput(QString text, QString file, int row, int column, int scrollOffset);
    
    void focus();
    
    bool isFocus();
    
signals:
    void backToPosition(QString file, int row, int column, int scrollOffset);
    void updateSearchKeyword(QString file, QString keyword);
    void replaceNext();
    void replacePrev();
    
public slots:
    void back();
    void handleContentChanged();
    void handleClickedNextButton();
    void handleClickedPrevButton();
    
protected:
    void paintEvent(QPaintEvent *event);
    bool focusNextPrevChild(bool next);
    
private:
    QHBoxLayout *layout;
    
    QLabel *replaceLabel;
    LineBar *replaceLine;
    
    QLabel *withLabel;
    LineBar *withLine;
    
    DTextButton *replaceButton;
    DTextButton *skipButton;
    DTextButton *replaceRestButton;
    DTextButton *replaceAllButton;
    
    QString replaceFile;
    int replaceFileRow;
    int replaceFileColumn;
    int replaceFileSrollOffset;
};

#endif
