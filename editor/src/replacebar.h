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
    void replaceNext(QString replaceText, QString withText);
    void replaceSkip();
    void replaceRest(QString replaceText, QString withText);
    void replaceAll(QString replaceText, QString withText);
    void cleanMatchKeyword();
    
public slots:
    void back();
    void handleContentChanged();
    void handleReplaceNext();
    void handleReplaceRest();
    void handleReplaceAll();
    
protected:
    void paintEvent(QPaintEvent *event);
    bool focusNextPrevChild(bool next);
    void hideEvent(QHideEvent *event);
    
private:
    QHBoxLayout *layout;
    
    QLabel *replaceLabel;
    LineBar *replaceLine;
    
    QLabel *withLabel;
    LineBar *withLine;
    
    DTextButton *replaceButton;
    DTextButton *replaceSkipButton;
    DTextButton *replaceRestButton;
    DTextButton *replaceAllButton;
    
    QString replaceFile;
    int replaceFileRow;
    int replaceFileColumn;
    int replaceFileSrollOffset;
};

#endif
