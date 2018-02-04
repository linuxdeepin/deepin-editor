/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "editor.h"
#include "utils.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>

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
    connect(textEditor, &TextEditor::jumpLine, this, &Editor::handleJumpLine, Qt::QueuedConnection);
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
    bool fileCreateFailed = false;
    if (!Utils::fileExists(filepath)) {
        QString directory = QFileInfo(filepath).dir().absolutePath();
        
        // Create file if filepath is not exists.
        if (Utils::fileIsWritable(directory)) {
            QDir().mkpath(directory);
            if (QFile(filepath).open(QIODevice::ReadWrite)) {
                qDebug() << QString("File %1 not exists, create one.").arg(filepath);
            }
        } else {
            // Make flag fileCreateFailed with 'true' if no permission to create.
            fileCreateFailed = true;
        }
    }

    if (Utils::fileIsWritable(filepath) && !fileCreateFailed) {
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

void Editor::handleJumpLine(int line, int lineCount, int scrollOffset)
{
    jumpLine(filepath, line, lineCount, scrollOffset);
}
