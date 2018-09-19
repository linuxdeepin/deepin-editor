/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#include "editorbuffer.h"
#include "utils.h"
#include "window.h"
#include "fileloadthread.h"

#include <QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>

EditorBuffer::EditorBuffer(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout(this)),
      m_textEditor(new TextEditor)
{
    // Init.
    m_autoSaveInternal = 1000;
    m_saveFinish = true;
    m_newline = "Linux";

    // Init layout and widgets.
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_textEditor->lineNumberArea);
    m_layout->addWidget(m_textEditor);
}

EditorBuffer::~EditorBuffer()
{
    delete m_textEditor;
}

void EditorBuffer::openFile(const QString &filepath)
{
    // update file path.
    updatePath(filepath);

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QTimer::singleShot(100, this, [=] {
            Utils::toast(QString(tr("You do not have permission to open %1")).arg(filepath),
                         this->topLevelWidget());
        });

        return;
    }

    m_isWritable = QFileInfo(filepath).isWritable();
    m_isLoadFinished = false;

    // begin to load the file.
    FileLoadThread *thread = new FileLoadThread(filepath);
    connect(thread, &FileLoadThread::loadFinished, this, &EditorBuffer::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);

    // start the thread.
    thread->start();
}

bool EditorBuffer::saveFile(const QString &encode, const QString &newline)
{
    bool fileCreateFailed = false;
    m_fileEncode = encode.toUtf8();

    if (!Utils::fileExists(m_textEditor->filepath)) {
        QString directory = QFileInfo(m_textEditor->filepath).dir().absolutePath();

        // Create file if filepath is not exists.
        if (Utils::fileIsWritable(directory)) {
            QDir().mkpath(directory);
            if (QFile(m_textEditor->filepath).open(QIODevice::ReadWrite)) {
                qDebug() << QString("File %1 not exists, create one.").arg(m_textEditor->filepath);
            }
        } else {
            // Make flag fileCreateFailed with 'true' if no permission to create.
            fileCreateFailed = true;
        }
    }

    // Try save file if file has permission.
    if (Utils::fileIsWritable(m_textEditor->filepath) && !fileCreateFailed) {
        QFile file(m_textEditor->filepath);
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "Can't write file: " << m_textEditor->filepath;
            // Utils::toast(tr("Can't write file: %1").arg(m_textEditor->filepath), this->topLevelWidget());
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
        out << m_textEditor->toPlainText().replace(newlineRegex, fileNewline);

        qDebug() << "saved: " << m_textEditor->filepath << encode << newline;

        file.close();
    }

    if (fileCreateFailed) {
        // blumia: WARNING! Toast is NOT the correct way to tell user something goes wrong!
        // FIXME: Tell user file no longer exist and create file at original path failed,
        //        Let user select a new path to save the file if user want.
        // Utils::toast(tr("File %1 create failed.").arg(m_textEditor->filepath), this->topLevelWidget());
        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QTimer::singleShot(100, [=] { QApplication::restoreOverrideCursor(); });

    // update status.
    m_textEditor->setModified(false);
    m_isLoadFinished = true;
    m_isWritable = true;

    return true;
}

bool EditorBuffer::saveFile()
{
    return saveFile(m_fileEncode, m_newline);
}

void EditorBuffer::updatePath(const QString &file)
{
    m_textEditor->filepath = file;
    detectNewline();
}

void EditorBuffer::detectNewline()
{
    QFile file(m_textEditor->filepath);

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

void EditorBuffer::handleFileLoadFinished(const QByteArray &encode, const QString &content)
{
    // restore mouse style.
    // QApplication::restoreOverrideCursor();

    m_isLoadFinished = true;
    m_fileEncode = encode;

    // set text.
    m_textEditor->document()->moveToThread(QCoreApplication::instance()->thread());
    m_textEditor->document()->setPlainText(content);

    // update status.
    m_textEditor->setModified(false);
    m_textEditor->moveToStart();

    // load highlight.
    QTimer::singleShot(100, this, [=] { m_textEditor->loadHighlighter(); });
}
