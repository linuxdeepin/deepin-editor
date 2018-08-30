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
#include "fileloadthread.h"

#include <QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>

Editor::Editor(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout(this)),
      textEditor(new TextEditor)
{
    // Init.
    m_autoSaveInternal = 1000;
    m_saveFinish = true;
    m_newline = "Linux";

    // Init layout and widgets.
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(textEditor->lineNumberArea);
    m_layout->addWidget(textEditor);
}

Editor::~Editor()
{
    delete textEditor;
}

void Editor::loadFile(const QString &filepath)
{
    // set mouse status to wait.
    // QApplication::setOverrideCursor(Qt::WaitCursor);

    // update file path.
    updatePath(filepath);

    // begin to load the file.
    FileLoadThread *thread = new FileLoadThread(filepath);
    connect(thread, &FileLoadThread::loadFinished, this, &Editor::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);

    // start the thread.
    thread->start();
}

bool Editor::saveFile(const QString &encode, const QString &newline)
{
    bool fileCreateFailed = false;

    QApplication::setOverrideCursor(Qt::WaitCursor);

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
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "Can't write file: " << textEditor->filepath;
            Utils::toast(tr("Can't write file: %1").arg(textEditor->filepath), this->topLevelWidget());
            return false;
        }

        m_newline = newline;

        QRegularExpression newlineRegex("\r?\n|\r");
        QString fileNewline;
        if (newline == "Windows") {
            fileNewline = "\r\n";
        } else if (newline == "Mac OS") {
            fileNewline = "\r";
        } else {
            fileNewline = "\n";
        }

        QTextStream out(&file);

        out.setCodec(encode.toUtf8().data());
        // NOTE: Muse call 'setGenerateByteOrderMark' to insert the BOM (Byte Order Mark) before any data has been written to file.
        // Otherwise, can't save file with given encoding.
        // out.setGenerateByteOrderMark(true);
        out << textEditor->toPlainText().replace(newlineRegex, fileNewline);

        qDebug() << "saved: " << textEditor->filepath << encode << newline;

        file.close();
    }

    if (fileCreateFailed) {
        // blumia: WARNING! Toast is NOT the correct way to tell user something goes wrong!
        // FIXME: Tell user file no longer exist and create file at original path failed,
        //        Let user select a new path to save the file if user want.
        Utils::toast(tr("File %1 create failed.").arg(textEditor->filepath), this->topLevelWidget());
        return false;
    }

    // update status.
    textEditor->setModified(false);

    QTimer::singleShot(100, [=] { QApplication::restoreOverrideCursor(); });

    return true;
}

bool Editor::saveFile()
{
    return saveFile(m_fileEncode, m_newline);
}

void Editor::updatePath(QString file)
{
    textEditor->filepath = file;
    detectNewline();
}

void Editor::detectNewline()
{
    QFile file(textEditor->filepath);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QString line = file.readLine();

    if (line.indexOf("\r\n") != -1) {
        m_newline = "Windows";
    } else if (line.indexOf("\r") != -1) {
        m_newline = "Mac OS";
    } else {
        m_newline = "Linux";
    }

    file.close();
}

void Editor::handleFileLoadFinished(const QString &encode, const QString &content)
{
    // restore mouse style.
    // QApplication::restoreOverrideCursor();

    // set text.
    textEditor->document()->moveToThread(QCoreApplication::instance()->thread());
    textEditor->document()->setPlainText(content);

    // update status.
    textEditor->setModified(false);
    textEditor->moveFirstLine();

    // load highlight.
    textEditor->loadHighlighter();
}
