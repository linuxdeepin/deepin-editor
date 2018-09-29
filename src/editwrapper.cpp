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

#include "editwrapper.h"
#include "utils.h"
#include "fileloadthread.h"
#include <unistd.h>

#include <QCoreApplication>
#include <QApplication>
#include <QSaveFile>
#include <QDebug>
#include <QTimer>
#include <QDir>

EditWrapper::EditWrapper(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout(this)),
      m_textEdit(new DTextEdit)
{
    // Init.
    m_autoSaveInternal = 1000;
    m_saveFinish = true;
    m_newline = "Linux";

    // Init layout and widgets.
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_textEdit->lineNumberArea);
    m_layout->addWidget(m_textEdit);
}

EditWrapper::~EditWrapper()
{
    delete m_textEdit;
}

void EditWrapper::openFile(const QString &filepath)
{
    // update file path.
    updatePath(filepath);

    m_isWritable = QFileInfo(filepath).isWritable();
    m_isLoadFinished = false;

    // begin to load the file.
    FileLoadThread *thread = new FileLoadThread(filepath);
    connect(thread, &FileLoadThread::loadFinished, this, &EditWrapper::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);

    // start the thread.
    thread->start();
}

bool EditWrapper::saveFile(const QString &encode, const QString &newline)
{
    m_fileEncode = encode.toUtf8();
    m_newline = newline;

    // use QSaveFile for safely save files.
    QSaveFile saveFile(m_textEdit->filepath);
    saveFile.setDirectWriteFallback(true);

    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QFile file(m_textEdit->filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QRegularExpression newlineRegex("\r?\n|\r");
    QString fileNewline;
    if (newline == "Windows") {
        fileNewline = "\r\n";
    } else if (newline == "Mac OS") {
        fileNewline = "\r";
    } else {
        fileNewline = "\n";
    }

    QTextStream stream(&file);
    stream.setCodec(encode.toUtf8().data());
    stream << m_textEdit->toPlainText().replace(newlineRegex, fileNewline);

    // flush stream.
    stream.flush();

    // close and delete file.
    file.close();

    // flush file.
    if (!saveFile.flush()) {
        return false;
    }

    // ensure that the file is written to disk
    fsync(saveFile.handle());

    // did save work?
    // only finalize if stream status == OK
    bool ok = (stream.status() == QTextStream::Ok);

    // update status.
    if (ok) {
        m_textEdit->setModified(false);
        m_isLoadFinished = true;
        m_isWritable = true;
    }

    qDebug() << "Saved file:" << m_textEdit->filepath
             << "with codec:" << m_fileEncode
             << "Line Endings:" << m_newline
             << "State:" << ok;

    return ok;
}

bool EditWrapper::saveFile()
{
    return saveFile(m_fileEncode, m_newline);
}

void EditWrapper::updatePath(const QString &file)
{
    m_textEdit->filepath = file;
    detectNewline();
}

void EditWrapper::detectNewline()
{
    QFile file(m_textEdit->filepath);

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

void EditWrapper::handleFileLoadFinished(const QByteArray &encode, const QString &content)
{
    // restore mouse style.
    // QApplication::restoreOverrideCursor();

    qDebug() << "load finished: " << m_textEdit->filepath << ", " << encode;

    m_isLoadFinished = true;
    m_fileEncode = encode;

    // set text.
    m_textEdit->blockSignals(true);
    m_textEdit->setPlainText(content);
    m_textEdit->blockSignals(false);

    // update status.
    m_textEdit->setModified(false);
    m_textEdit->updateLineNumber();
    m_textEdit->moveToStart();

    // load highlight.
    QTimer::singleShot(100, this, [=] { m_textEdit->loadHighlighter(); });
}
