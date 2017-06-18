#include "editor.h"
#include <QDebug>
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
    
    textEditor = new QPlainTextEdit;
    textEditor->setFont(font);

    highlighter = new Highlighter(textEditor->document());
    
    layout->addWidget(textEditor);
    
    QDir projectDir = QDir(qApp->applicationDirPath());
    projectDir.cdUp();
    QFile file(QString("%1/src/%2").arg(projectDir.absolutePath()).arg("editor.cpp"));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        textEditor->setPlainText(file.readAll());
    }
}
