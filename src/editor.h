#ifndef EDITOR_H
#define EDITOR_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include "highlighter.h"

class Editor : public QWidget
{
    Q_OBJECT
    
public:
    Editor(QWidget *parent = 0);
    
private:
    QPlainTextEdit *textEditor;
    Highlighter *highlighter;
    
    QVBoxLayout *layout;
};

#endif
