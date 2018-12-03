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

#include "drecentmanager.h"

DCORE_USE_NAMESPACE

EditWrapper::EditWrapper(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout(this)),
      m_textEdit(new DTextEdit),
      m_textCodec(QTextCodec::codecForName("UTF-8")),
      m_endOfLineMode(eolUnix),
      m_isLoadFinished(true)
{
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
    detectEndOfLine();

    m_isLoadFinished = false;

    // begin to load the file.
    FileLoadThread *thread = new FileLoadThread(filepath);
    connect(thread, &FileLoadThread::loadFinished, this, &EditWrapper::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);

    // start the thread.
    thread->start();
}

bool EditWrapper::saveFile()
{
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

    QRegularExpression eolRegex("\r?\n|\r");
    QString eol = QStringLiteral("\n");
    if (m_endOfLineMode == eolDos) {
        eol = QStringLiteral("\r\n");
    } else if (m_endOfLineMode == eolMac) {
        eol = QStringLiteral("\r");
    }

    QTextStream stream(&file);
    stream.setCodec(m_textCodec);
    stream << m_textEdit->toPlainText().replace(eolRegex, eol);

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
    }

    qDebug() << "Saved file:" << m_textEdit->filepath
             << "with codec:" << m_textCodec->name()
             << "Line Endings:" << m_endOfLineMode
             << "State:" << ok;

    return ok;
}

void EditWrapper::updatePath(const QString &file)
{
    m_textEdit->filepath = file;
    // detectEndOfLine();
}

void EditWrapper::refresh()
{
    if (filePath().isEmpty() ||
        Utils::isDraftFile(filePath())) {
        return;
    }

    QFile file(filePath());
    int curPos = m_textEdit->textCursor().position();
    m_textEdit->setPlainText(QString());

    if (file.open(QIODevice::ReadOnly)) {
        QTextStream out(&file);
        out.setCodec(m_textCodec);
        QString content = out.readAll();

        m_textEdit->setUpdatesEnabled(false);
        m_textEdit->setPlainText(content);
        m_textEdit->setModified(false);

        QTextCursor textcur = m_textEdit->textCursor();
        textcur.setPosition(curPos);
        m_textEdit->setTextCursor(textcur);

        QTimer::singleShot(50, this, [=] {
            m_textEdit->setUpdatesEnabled(true);
        });

        file.close();
    }
}

EditWrapper::EndOfLineMode EditWrapper::endOfLineMode()
{
    return m_endOfLineMode;
}

void EditWrapper::setEndOfLineMode(EndOfLineMode eol)
{
    m_endOfLineMode = eol;
}

void EditWrapper::setTextCodec(QTextCodec *codec)
{
    m_textCodec = codec;

    // TODO: enforce bom for some encodings
}

void EditWrapper::detectEndOfLine()
{
    QFile file(m_textEdit->filepath);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray line = file.readLine();
    if (line.indexOf("\r\n") != -1) {
        m_endOfLineMode = eolDos;
    } else if (line.indexOf("\r") != -1) {
        m_endOfLineMode = eolMac;
    } else {
        m_endOfLineMode = eolUnix;
    }

    file.close();
}

void EditWrapper::handleFileLoadFinished(const QByteArray &encode, const QString &content)
{
    // restore mouse style.
    // QApplication::restoreOverrideCursor();

    qDebug() << "load finished: " << m_textEdit->filepath << ", " << encode << "endOfLine: " << m_endOfLineMode;

    if (!Utils::isDraftFile(m_textEdit->filepath)) {
        DRecentData data;
        data.appName = "Deepin Editor";
        data.appExec = "deepin-editor";
        DRecentManager::addItem(m_textEdit->filepath, data);
    }

    m_isLoadFinished = true;
    m_textCodec = QTextCodec::codecForName(encode);

    // set text.
    m_textEdit->setPlainText(content);

    // update status.
    m_textEdit->setModified(false);
    m_textEdit->moveToStart();

    // load highlight.
    QTimer::singleShot(100, this, [=] { m_textEdit->loadHighlighter(); });
}
