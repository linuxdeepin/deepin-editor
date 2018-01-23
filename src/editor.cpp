#include "editor.h"
#include "utils.h"
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QApplication>

Editor::Editor(QWidget *parent) : QWidget(parent)
{
    QFont font;
    font.setFamily("Note Mono");
    font.setFixedPitch(true);
    font.setPointSize(12);

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    textEditor = new QTextEdit;
    textEditor->setFont(font);

    highlighter = new Highlighter(textEditor->document());

    layout->addWidget(textEditor);

    QTimer::singleShot(0, textEditor, SLOT(setFocus()));
    
    autoSaveInternal = 1000;
    saveFinish = true;
    
    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setSingleShot(true);
    connect(autoSaveTimer, &QTimer::timeout, this, &Editor::handleTextChangeTimer);
    
    connect(textEditor, &QTextEdit::textChanged, this, &Editor::handleTextChanged, Qt::QueuedConnection);
}

void Editor::loadFile(QString filepath)
{
    QFile file(filepath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        textEditor->setPlainText(file.readAll());
        
        updatePath(filepath);
    }
}

void Editor::saveFile()
{
    QFile file(filepath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Can't write file: " << filepath;
        
        return;
    }
    
    QTextStream out(&file);
    out << textEditor->toPlainText();
    file.close();
}

void Editor::updatePath(QString file)
{
    filepath = file;
}

void Editor::handleTextChanged()
{
    if (Utils::fileExists(filepath)) {
        if (Utils::fileIsWritable(filepath)) {
            saveFinish = false;
    
            if (autoSaveTimer->isActive()) {
                autoSaveTimer->stop();
            }
            autoSaveTimer->start(autoSaveInternal);
        } else {
            qDebug() << QString("%1 no permission to write").arg(filepath);
        }
    }
}

void Editor::handleTextChangeTimer()
{
    if (Utils::fileExists(filepath) && Utils::fileIsWritable(filepath)) {
        saveFinish = true;
        saveFile();
        
        qDebug() << "Save " << filepath;
    }
}
