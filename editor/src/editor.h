#ifndef EDITOR_H
#define EDITOR_H

#include "dbusinterface.h"
#include <QWidget>
#include <QPlainTextEdit>
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
    void trySaveFile();
    
public slots:
    void handleTextChanged();
    void handleTextChangeTimer();
    
private:
    QTextEdit *textEditor;
    Highlighter *highlighter;
    
    QVBoxLayout *layout;
    QString filepath;
    
    QTimer *autoSaveTimer;
    int autoSaveInternal;
    
    bool saveFinish;
    DBusDaemon::dbus *autoSaveDBus;
};

#endif
