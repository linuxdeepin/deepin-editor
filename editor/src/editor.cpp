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
    // Init.
    autoSaveInternal = 1000;
    saveFinish = true;

    // Init layout and widgets.
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    textEditor = new TextEditor;

    layout->addWidget(textEditor->lineNumberArea);
    layout->addWidget(textEditor);

    // Init auto save timer.
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
        
        textEditor->loadHighlighter();
    }
    
    file.close();
}

void Editor::saveFile()
{
    bool fileCreateFailed = false;
    if (!Utils::fileExists(textEditor->filepath)) {
        QString directory = QFileInfo(textEditor->filepath).dir().absolutePath();
        
        // Create file if filepath is not exists.
        if (Utils::fileIsWritable(directory)) {
            QDir().mkpath(directory);
            if (QFile(textEditor->filepath).open(QIODevice::ReadWrite)) {
                qDebug() << QString("File %1 not exists, create one.").arg(textEditor->filepath);
            }
        } else {
            // Make flag fileCreateFailed with 'true' if no permission to create.
            fileCreateFailed = true;
        }
    }

    // Try save file if file has permission.
    if (Utils::fileIsWritable(textEditor->filepath) && !fileCreateFailed) {
        QFile file(textEditor->filepath);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug() << "Can't write file: " << textEditor->filepath;

            return;
        }

        QTextStream out(&file);
        out << textEditor->toPlainText();
        file.close();
    }
}

void Editor::updatePath(QString file)
{
    textEditor->filepath = file;
}

void Editor::handleTextChangeTimer()
{
    if (Utils::fileExists(textEditor->filepath)) {
        saveFinish = true;

        saveFile();
    }
}

void Editor::handleTextChanged()
{
    if (Utils::fileExists(textEditor->filepath)) {
        saveFinish = false;

        if (autoSaveTimer->isActive()) {
            autoSaveTimer->stop();
        }
        autoSaveTimer->start(autoSaveInternal);
    }
}

