#include "editor.h"
#include "utils.h"
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QApplication>

Editor::Editor(QWidget *parent) : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    textEditor = new TextEditor;

    autoSaveDBus = new DBusDaemon::dbus("com.deepin.editor.daemon", "/", QDBusConnection::systemBus(), this);

    layout->addWidget(textEditor->lineNumberArea);
    layout->addWidget(textEditor);

    autoSaveInternal = 1000;
    saveFinish = true;

    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setSingleShot(true);
    connect(autoSaveTimer, &QTimer::timeout, this, &Editor::handleTextChangeTimer);

    connect(textEditor, &TextEditor::textChanged, this, &Editor::handleTextChanged, Qt::QueuedConnection);
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
    if (Utils::fileIsWritable(filepath)) {
        QFile file(filepath);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug() << "Can't write file: " << filepath;

            return;
        }

        QTextStream out(&file);
        out << textEditor->toPlainText();
        file.close();
    } else {
        bool result = autoSaveDBus->saveFile(filepath, textEditor->toPlainText());
        if (!result) {
            qDebug() << QString("Save root file %1 failed").arg(filepath);
        }
    }
}

void Editor::updatePath(QString file)
{
    filepath = file;
}

void Editor::handleTextChanged()
{
    if (Utils::fileExists(filepath)) {
        saveFinish = false;

        if (autoSaveTimer->isActive()) {
            autoSaveTimer->stop();
        }
        autoSaveTimer->start(autoSaveInternal);
    }
}

void Editor::handleTextChangeTimer()
{
    if (Utils::fileExists(filepath)) {
        saveFinish = true;

        saveFile();
    }
}
