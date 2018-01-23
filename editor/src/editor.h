#ifndef EDITOR_H
#define EDITOR_H

#include "dbusinterface.h"
#include <QWidget>
#include "texteditor.h"
#include <QVBoxLayout>

class Editor : public QWidget
{
    Q_OBJECT
    
public:
    Editor(QWidget *parent = 0);
    void loadFile(QString filepath);
    void saveFile();
    void updatePath(QString file);
                                 
    TextEditor *textEditor;
                          
public slots:
    void handleTextChanged();
    void handleTextChangeTimer();
    
private:
    
    QHBoxLayout *layout;
    QString filepath;
    
    QTimer *autoSaveTimer;
    int autoSaveInternal;
    
    bool saveFinish;
    DBusDaemon::dbus *autoSaveDBus;
    
};

#endif
