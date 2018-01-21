#include "editor.h"
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
}

void Editor::loadFile(QString filepath)
{
    QFile file(filepath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        textEditor->setPlainText(file.readAll());
    }
}
