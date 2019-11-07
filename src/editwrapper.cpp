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

#include "widgets/toast.h"
#include "fileloadthread.h"
#include "editwrapper.h"
#include "utils.h"
#include <unistd.h>

#include <QCoreApplication>
#include <QApplication>
#include <QSaveFile>
#include <QScrollBar>
#include <QScroller>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include "drecentmanager.h"

DCORE_USE_NAMESPACE

EditWrapper::EditWrapper(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout),
      m_textEdit(new TextEdit),
      m_bottomBar(new BottomBar(this)),
      m_textCodec(QTextCodec::codecForName("UTF-8")),
      m_endOfLineMode(eolUnix),
      m_isLoadFinished(true),
      m_isRefreshing(false),
      m_waringNotices(new WarningNotices)
{
    // Init layout and widgets.
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_textEdit->lineNumberArea);
    m_layout->addWidget(m_textEdit);

    m_bottomBar->setHighlightMenu(m_textEdit->getHighlightMenu());
    m_textEdit->setWrapper(this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(m_layout);
    mainLayout->addWidget(m_bottomBar);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);

    connect(m_textEdit, &TextEdit::cursorModeChanged, this, &EditWrapper::handleCursorModeChanged);
    connect(m_textEdit, &TextEdit::hightlightChanged, this, &EditWrapper::handleHightlightChanged);
    connect(m_textEdit, &TextEdit::textChanged, this, &EditWrapper::slotTextChange);

    connect(m_waringNotices, &WarningNotices::closeButtonClicked, m_waringNotices, &WarningNotices::closeBtnClicked);
    connect(m_waringNotices, &WarningNotices::reloadBtnClicked, this, &EditWrapper::refresh);
    connect(m_waringNotices, &WarningNotices::closeBtnClicked, this, [=] {
        QFileInfo fi(filePath());
        m_modified = fi.lastModified();
    });

    connect(m_waringNotices, &WarningNotices::saveAsBtnClicked, this, &EditWrapper::requestSaveAs);
}

EditWrapper::~EditWrapper()
{
    delete m_textEdit;
    delete m_waringNotices;
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

    QFileInfo fi(filePath());
    m_modified = fi.lastModified();

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
    QFileInfo fi(file);
    m_modified = fi.lastModified();

    m_textEdit->filepath = file;
    detectEndOfLine();
}

void EditWrapper::refresh()
{
    if (filePath().isEmpty() || Utils::isDraftFile(filePath()) || m_isRefreshing) {
        return;
    }

    QFile file(filePath());
    int curPos = m_textEdit->textCursor().position();
    int yoffset = m_textEdit->verticalScrollBar()->value();
    int xoffset = m_textEdit->horizontalScrollBar()->value();

    if (file.open(QIODevice::ReadOnly)) {
        m_isRefreshing = true;

        QTextStream out(&file);
        out.setCodec(m_textCodec);
        QString content = out.readAll();

        m_textEdit->setPlainText(QString());
        m_textEdit->setPlainText(content);
        m_textEdit->setModified(false);

        QTextCursor textcur = m_textEdit->textCursor();
        textcur.setPosition(curPos);
        m_textEdit->setTextCursor(textcur);
        m_textEdit->verticalScrollBar()->setValue(yoffset);
        m_textEdit->horizontalScrollBar()->setValue(xoffset);

        QFileInfo fi(filePath());
        m_modified = fi.lastModified();

        file.close();
        m_waringNotices->hide();

        m_textEdit->setUpdatesEnabled(false);

        QTimer::singleShot(10, this, [=] {
            m_textEdit->setUpdatesEnabled(true);
            m_isRefreshing = false;
        });
    } else {
        m_isRefreshing = false;
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

void EditWrapper::setTextCodec(QTextCodec *codec, bool reload)
{
    m_textCodec = codec;

    if (reload == false)
        return;

    refresh();

    // TODO: enforce bom for some encodings
}

void EditWrapper::setTextCodec(QByteArray encodeName, bool reload)
{
    QTextCodec* codec = QTextCodec::codecForName(encodeName);

    if (!codec) {
        qWarning() << "Codec for" << encodeName << "not found! Fallback to UTF-8";
        codec = QTextCodec::codecForName("UTF-8");
    }

    setTextCodec(codec, reload);
}

void EditWrapper::hideWarningNotices()
{

    if (m_waringNotices->isVisible()) {
        m_waringNotices->hide();
    }
}

void EditWrapper::checkForReload()
{
    if (Utils::isDraftFile(m_textEdit->filepath))
        return;

    QFileInfo fi(filePath());

    if (fi.lastModified() == m_modified || m_waringNotices->isVisible())
        return;

    if (fi.exists() && fi.lastModified() != m_modified) {
        m_waringNotices->setMessage(tr("File has changed on disk. Reload?"));
        m_waringNotices->setReloadBtn();
    } else {
        m_waringNotices->setMessage(tr("File removed on the disk. Save it now?"));
        m_waringNotices->setSaveAsBtn();
    }

    DMessageManager::instance()->sendMessage(m_textEdit, m_waringNotices);
}

void EditWrapper::showNotify(const QString &message)
{
    //DFloatingMessage
    //DMainWindow::sendMessage(QIcon(":/images/ok.svg"), message);
    //DMessageManager::sendMessage(QIcon(":/images/ok.svg"), message);

    DMessageManager::instance()->sendMessage(m_textEdit, QIcon(":/images/ok.svg"), message);
}

bool EditWrapper::getTextChangeFlag()
{
    return m_bTextChange;
}

void EditWrapper::setTextChangeFlag(bool bFlag)
{
    m_bTextChange = bFlag;
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

void EditWrapper::handleCursorModeChanged(TextEdit::CursorMode mode)
{
    switch (mode) {
    case TextEdit::Insert:
        m_bottomBar->setCursorStatus(tr("INSERT"));
        break;
    case TextEdit::Overwrite:
        m_bottomBar->setCursorStatus(tr("OVERWRITE"));
        break;
    case TextEdit::Readonly:
        m_bottomBar->setCursorStatus(tr("R/O"));
        break;
    default:
        break;
    }
}

void EditWrapper::handleHightlightChanged(const QString &name)
{
    m_bottomBar->setHightlightName(name);
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
    setTextCodec(encode);

    // set text.
    m_textEdit->loadHighlighter();
    m_textEdit->setPlainText(content);

    // update status.
    m_textEdit->setModified(false);
    m_textEdit->moveToStart();

    m_bottomBar->setEncodeName(m_textCodec->name());

    // load highlight.
    //QTimer::singleShot(100, this, [=] { m_textEdit->loadHighlighter(); });
}

void EditWrapper::resizeEvent(QResizeEvent *e)
{

    QWidget::resizeEvent(e);
}

void EditWrapper::slotTextChange()
{
    m_bTextChange = true;
}
