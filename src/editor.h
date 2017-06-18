#ifndef EDITOR_H
#define EDITOR_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include "highlighter.h"

class Editor : public QWidget
{
    Q_OBJECT
    
public:
    Editor(QWidget *parent = 0);
    
private:
    QTextEdit *textEditor;
    Highlighter *highlighter;
    
    QVBoxLayout *layout;
};

#endif
