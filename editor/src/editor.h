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
                          
signals:
    void jumpLine(QString filepath, int line, int lineCount);
                          
public slots:
    void handleTextChanged();
    void handleTextChangeTimer();
    void handleJumpLine(int line, int lineCount);
    
private:
    
    QHBoxLayout *layout;
    QString filepath;
    
    QTimer *autoSaveTimer;
    int autoSaveInternal;
    
    bool saveFinish;
    DBusDaemon::dbus *autoSaveDBus;
    
};

#endif
