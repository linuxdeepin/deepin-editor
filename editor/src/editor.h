#ifndef EDITOR_H
#define EDITOR_H

#include "dbusinterface.h"
#include <QWidget>
#include "texteditor.h"
#include <QVBoxLayout>
#include "highlighter.h"

class Editor : public QWidget
{
    Q_OBJECT
    
public:
    Editor(QWidget *parent = 0);
    void loadFile(QString filepath);
    void saveFile();
    void updatePath(QString file);
    
public slots:
    void handleTextChanged();
    void handleTextChangeTimer();
    void highlightCurrentLine();
    
private:
    TextEditor *textEditor;
    Highlighter *highlighter;
    
    QHBoxLayout *layout;
    QString filepath;
    
    QTimer *autoSaveTimer;
    int autoSaveInternal;
    
    bool saveFinish;
    DBusDaemon::dbus *autoSaveDBus;
    
};

#endif
