#ifndef JUMPLINEBAR_H
#define JUMPLINEBAR_H

#include <QWidget>
#include <QIntValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include "linebar.h"

DWIDGET_USE_NAMESPACE

class JumpLineBar : public QWidget
{
    Q_OBJECT
    
public:
    JumpLineBar(QWidget *parent = 0);
    
    void activeInput(QString file, int line, int lineCount, int scrollOffset);
    
public slots:    
    void cancel();
    void back();
    void jump();
    void tempJump();
    
signals:
    void backToLine(QString file, int line, int scrollOffset);
    void jumpToLine(QString file, int line);
    void tempJumpToLine(QString file, int line);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QHBoxLayout *layout;
    QLabel *label;
    LineBar *editLine;
    
    QString jumpFile;
    int lineBeforeJump;
    int jumpFileScrollOffset;
    
    QIntValidator *lineValidator;
};

#endif
