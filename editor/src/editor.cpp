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

    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    textEditor = new TextEditor;
    textEditor->setFont(font);
    
    highlightCurrentLine();

    autoSaveDBus = new DBusDaemon::dbus("com.deepin.editor.daemon", "/", QDBusConnection::systemBus(), this);

    highlighter = new Highlighter(textEditor->document());

    layout->addWidget(textEditor->lineNumberArea);
    layout->addWidget(textEditor);

    QTimer::singleShot(0, textEditor, SLOT(setFocus()));

    autoSaveInternal = 1000;
    saveFinish = true;

    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setSingleShot(true);
    connect(autoSaveTimer, &QTimer::timeout, this, &Editor::handleTextChangeTimer);

    connect(textEditor, &TextEditor::textChanged, this, &Editor::handleTextChanged, Qt::QueuedConnection);
    connect(textEditor, &TextEditor::cursorPositionChanged, this, &Editor::highlightCurrentLine, Qt::QueuedConnection);
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

        if (Utils::fileIsWritable(filepath)) {
            saveFile();
        } else {
            bool result = autoSaveDBus->saveFile(filepath, textEditor->toPlainText());
            if (!result) {
                qDebug() << QString("Save root file %1 failed").arg(filepath);
            }
        }

        qDebug() << "Save " << filepath;
    }
}

void Editor::trySaveFile()
{
    if (Utils::fileExists(filepath)) {
        if (Utils::fileIsWritable(filepath)) {
            qDebug() << QString("Don't need file %1 handly").arg(filepath);
        }
    }
}

void Editor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor("#333333");

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textEditor->textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    textEditor->setExtraSelections(extraSelections);
}
