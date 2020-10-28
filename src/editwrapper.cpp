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
#include "leftareaoftextedit.h"

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
      m_textEdit(new TextEdit(this)),
      m_textCodec(QTextCodec::codecForName("UTF-8")),
      m_bottomBar(new BottomBar(this)),
      m_endOfLineMode(eolUnix),
      m_isLoadFinished(true),
      m_isRefreshing(false),
      m_waringNotices(new WarningNotices)
{
    // Init layout and widgets.
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_textEdit->m_pLeftAreaWidget);
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

    //connect(m_waringNotices, &WarningNotices::closeButtonClicked, m_waringNotices, &WarningNotices::closeBtnClicked);
    connect(m_waringNotices, &WarningNotices::reloadBtnClicked, this, &EditWrapper::refresh);
    connect(m_waringNotices, &WarningNotices::closeButtonClicked, this, [=] {
        QFileInfo fi(filePath());
        m_modified = fi.lastModified();
    });

    connect(m_waringNotices, &WarningNotices::saveAsBtnClicked, this, &EditWrapper::requestSaveAs);
}

EditWrapper::~EditWrapper()
{
  // if(m_textEdit)
   //    m_textEdit->deleteLater();
    //delete m_waringNotices;
}

void EditWrapper::setQuitFlag()
{
    m_bQuit = true;
    //m_bFileLoading = true;
    //QApplication::processEvents();
}

bool EditWrapper::getFileLoading()
{
    return (m_bQuit || m_bFileLoading);
}


void EditWrapper::openFile(const QString &filepath)
{
    // update file path.
    updatePath(filepath);
    //detectEndOfLine();

    m_textEdit->setIsFileOpen();
    m_isLoadFinished = false;

    FileLoadThread *thread = new FileLoadThread(filepath);

    // begin to load the file.
    connect(thread, &FileLoadThread::loadFinished, this, &EditWrapper::handleFileLoadFinished);
    connect(thread, &FileLoadThread::toTellFileClosed, this, &EditWrapper::onFileClosed);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);

    QStringList encodeList = textEditor()->readEncodeHistoryRecord();
    QStringList filepathList = textEditor()->readHistoryRecordofFilePath("advance.editor.browsing_encode_history");
    thread->setEncodeInfo(filepathList,encodeList);
//    // start the thread.
    QApplication::setOverrideCursor(Qt::WaitCursor);
    thread->start();
}

bool EditWrapper::saveAsFile(const QString &newFilePath, QByteArray encodeName)
{
    QTextCodec *pSaveAsFileCodec = QTextCodec::codecForName(encodeName);
    // use QSaveFile for safely save files.
    QSaveFile saveFile(newFilePath);
    saveFile.setDirectWriteFallback(true);

    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QFile file(newFilePath);
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

    //auto append new line char to end of file when file's format is Linux/MacOS
    QString fileContent = m_textEdit->toPlainText();

    if (m_endOfLineMode == eolUnix) {
        if (!fileContent.endsWith("\n"))
        {
            fileContent = fileContent.append(QChar('\n'));
        }
    }
    else if (m_endOfLineMode == eolDos)
    {
        if (!fileContent.endsWith("\r\n"))
        {
            fileContent = fileContent.append(QChar('\r')).append(QChar('\n'));
        }
    }
    else if (m_endOfLineMode == eolMac) {
        if (!fileContent.endsWith("\r"))
        {
            fileContent = fileContent.append(QChar('\r'));
        }
    }

    QTextStream stream(&file);
    stream.setCodec(pSaveAsFileCodec);
    stream << fileContent.replace(eolRegex, eol);

    //flush stream.
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
        //m_textEdit->setModified(false);
        //m_isLoadFinished = true;
    }

//    qDebug() << "Saved file:" << m_textEdit->filepath
//             << "with codec:" << pSaveAsFileCodec->name()
//             << "Line Endings:" << m_endOfLineMode
//             << "State:" << ok;

    return ok;
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

    //auto append new line char to end of file when file's format is Linux/MacOS
    QString fileContent = m_textEdit->toPlainText();

    if (m_endOfLineMode == eolUnix) {
        if (!fileContent.endsWith("\n"))
        {
            //fileContent = fileContent.append(QChar('\n'));
        }
    }
    else if (m_endOfLineMode == eolDos)
    {
        if (!fileContent.endsWith("\r\n"))
        {
            //fileContent = fileContent.append(QChar('\r')).append(QChar('\n'));
        }
    }
    else if (m_endOfLineMode == eolMac) {
        if (!fileContent.endsWith("\r"))
        {
            //fileContent = fileContent.append(QChar('\r'));
        }
    }

    QTextStream stream(&file);
    stream.setCodec(m_textCodec);
    stream << fileContent.replace(eolRegex, eol);

//    qDebug() << "saveFile stream.codec()->name():" << stream.codec()->name();

    //flush stream.
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

//    qDebug() << "Saved file:" << m_textEdit->filepath
//             << "with codec:" << m_textCodec->name()
//             << "Line Endings:" << m_endOfLineMode
//             << "State:" << ok;

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
//    if (filePath().isEmpty() || Utils::isDraftFile(filePath()) || m_isRefreshing) {
//        return;
//    }
    if (filePath().isEmpty() || m_isRefreshing) {
        return;
    }

    QFile file(filePath());
    int curPos = m_textEdit->textCursor().position();
    int yoffset = m_textEdit->verticalScrollBar()->value();
    int xoffset = m_textEdit->horizontalScrollBar()->value();

    //如果文件有被修改了
    if (m_textEdit->document()->isModified()) {
        DDialog *dialog = new DDialog(tr("Encoding changed. Do you want to save the file now?"), "", this);
        dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
        dialog->setIcon(QIcon::fromTheme("deepin-editor"));
        dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
        dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);

        //如果用户直接按关闭按钮
        connect(dialog, &DDialog::closed, this, [=] {
            m_bottomBar->setEncodeName(QString(m_BeforeEncodeName));
            m_textCodec = QTextCodec::codecForName(m_BeforeEncodeName);
            return;
        });

        connect(dialog, &DDialog::buttonClicked, this, [=] (int index) {
            dialog->hide();

            // 如果用户放弃了这次操作
            if (index == 0) {
                m_bottomBar->setEncodeName(QString(m_BeforeEncodeName));
                m_textCodec = QTextCodec::codecForName(m_BeforeEncodeName);
                return;
            }
            else if (index == 1) {

                bool isDraft = Utils::isDraftFile(m_textEdit->filepath);
                //如果是临时标签文件
                if (isDraft == true) {
                    //如果临时文件内容为空则不做操作
                    if (m_textEdit->toPlainText() == "") {
                        return;
                    }

                    const QString &new_file = DFileDialog::getSaveFileName(this, tr("Save"), QDir::homePath(), "*.txt");
                    if (new_file.isEmpty())
                        return;

                    QFile qfile(new_file);

                    if (!qfile.open(QFile::WriteOnly)) {
                        return;
                    }

                    // 以切换前的编码保存内容到文件
                    qDebug() << "write m_BeforeEncodeName:" << m_BeforeEncodeName;
                    if (m_BeforeEncodeName.isNull()) {
                        QString str = "UTF-8";
                        m_BeforeEncodeName = str.toLocal8Bit();
                    }
                    qfile.write(QTextCodec::codecForName(m_BeforeEncodeName)->fromUnicode(m_textEdit->toPlainText()));
                    qfile.close();

                    QString strOldFilePath = m_textEdit->filepath;
                    if (isDraft) {
                        QFile(m_textEdit->filepath).remove();
                        this->updatePath(new_file);
                    }

                    // 重新读取文件
                    readFile(new_file);
                    m_bTextChange = false;

                    emit sigCodecSaveFile(strOldFilePath, new_file);
                    //m_textEdit->setModified(false);
                    //m_textEdit->document()->setModified(false);
                    //m_isLoadFinished = true;
                }
                //如果是已经存在的文件
                else {
                    QFile qfile(m_textEdit->filepath);
                    if (!qfile.open(QFile::WriteOnly)) {
                        return;
                    }

                    // 以切换前的编码保存内容到文件
                    qfile.write(QTextCodec::codecForName(m_BeforeEncodeName)->fromUnicode(m_textEdit->toPlainText()));
                    qfile.close();

                    // 重新读取文件
                    readFile(m_textEdit->filepath);
                    m_bTextChange = false;
                }
            }
        });

        dialog->exec();
    }
    else {
        readFile(m_textEdit->filepath);
        m_bTextChange = false;
    }

    if (file.open(QIODevice::ReadOnly)) {
        m_isRefreshing = true;

        //QTextStream out(&file);
        //out.setCodec(m_textCodec);
        //QString content = out.readAll();

        //设置编码方法1
        //const QString strContent = m_textEdit->toPlainText();
        //QTextStream streamContent(strContent.toLocal8Bit());
        //streamContent.setCodec(m_textCodec);
        //QString content = streamContent.readAll();
        //in >> content;

        //设置编码 方法2
//        const QString strContent = m_textEdit->toPlainText();
//        qDebug() << "get strContent:" << strContent;
//        QByteArray byteContent = m_textCodec->fromUnicode(strContent);
//        QTextStream streamContent(byteContent);
//        streamContent.setCodec(m_textCodec);
//        QString content = streamContent.readAll();

//        m_textEdit->setPlainText(QString());
//        qDebug() << "set content:" << content;
//        m_textEdit->setPlainText(content);
        //m_textEdit->setModified(false);

        QTextCursor textcur = m_textEdit->textCursor();
        textcur.setPosition(curPos);
        m_textEdit->setTextCursor(textcur);
        m_textEdit->verticalScrollBar()->setValue(yoffset);
        m_textEdit->horizontalScrollBar()->setValue(xoffset);

        QFileInfo fi(filePath());
        m_modified = fi.lastModified();

        file.close();
        m_waringNotices->hide();

        fsync(file.handle());

        m_textEdit->setUpdatesEnabled(false);

        QTimer::singleShot(10, this, [=] {
            m_textEdit->setUpdatesEnabled(true);
            m_isRefreshing = false;
        });
    } else {
        m_isRefreshing = false;
    }
}

bool EditWrapper::saveDraftFile()
{
    const QString &new_file = DFileDialog::getSaveFileName(this, tr("Save"), QDir::homePath(), "*.txt");
    if (new_file.isEmpty())
        return false;

    QFile qfile(new_file);

    if (!qfile.open(QFile::WriteOnly)) {
        return false;
    }

    // 以新的编码保存内容到文件
    qfile.write(QTextCodec::codecForName(m_textCodec->name())->fromUnicode(m_textEdit->toPlainText()));
    qfile.close();

    QFile(m_textEdit->filepath).remove();
    this->updatePath(new_file);

    return true;
}

void EditWrapper::readFile(const QString &filePath)
{
    QByteArray data;
    QFile qfile(filePath);
    m_bIsContinue = true;

    // 重新读取文件
    if (data.isEmpty()) {
        if (!qfile.open(QFile::ReadOnly)) {
            return;
        }

        data = qfile.readAll();
        qfile.close();
    }

    if  (data.isEmpty()) {
        return;
    }

    // 使用新的编码重新加载文件
    // 获取当前用户选择使用的编码
    QTextCodec *codec = QTextCodec::codecForName(m_textCodec->name());
    QTextDecoder *decoder = codec->makeDecoder();
    const QString &text = decoder->toUnicode(data);

    // 判断是否出错
    if (decoder->hasFailure()) {
        DDialog *dialogWarning = new DDialog(tr("There are errors when using this encoding. If continue, the file contents may be changed"), "", this);
        dialogWarning->setWindowFlags(dialogWarning->windowFlags() | Qt::WindowStaysOnTopHint);
        dialogWarning->setIcon(QIcon::fromTheme("deepin-editor"));
        dialogWarning->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
        dialogWarning->addButton(QString(tr("Continue")), true, DDialog::ButtonRecommend);

        // 如果用户按关闭按钮放弃了这次操作
        connect(dialogWarning, &DDialog::closed, this, [=] {
            // 恢复到旧的编码
            //QSignalBlocker blocker(ui->comboBox); // 禁用信号通知
            //Q_UNUSED(blocker)
            //ui->comboBox->setCurrentText(m_currentCodec);
            m_bottomBar->setEncodeName(m_BeforeEncodeName);
            m_bIsContinue = false;
            return;
        });

        connect(dialogWarning, &DDialog::buttonClicked, this, [=] (int index) {
            qDebug() << "index:" << index;
            // 如果用户放弃了这次操作
            if (index == 0) {
                // 恢复到旧的编码
                //QSignalBlocker blocker(ui->comboBox); // 禁用信号通知
                //Q_UNUSED(blocker)
                //ui->comboBox->setCurrentText(m_currentCodec);
                m_bottomBar->setEncodeName(m_BeforeEncodeName);
                m_bIsContinue = false;
                return;
            }
        });

        dialogWarning->exec();
    }

    if (m_bIsContinue == false) {
        return;
    }
    m_BeforeEncodeName = m_textCodec->name();
    m_textEdit->setPlainText(text);
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

    m_waringNotices->show();
    DMessageManager::instance()->sendMessage(m_textEdit, m_waringNotices);
}

void EditWrapper::showNotify(const QString &message)
{
    //DFloatingMessage
    //DMainWindow::sendMessage(QIcon(":/images/ok.svg"), message);
    //DMessageManager::sendMessage(QIcon(":/images/ok.svg"), message);
    if (m_textEdit->getReadOnlyPermission() == true || m_textEdit->getReadOnlyMode() == true) {
        DMessageManager::instance()->sendMessage(m_textEdit, QIcon(":/images/warning.svg"), message);
    } else {
        DMessageManager::instance()->sendMessage(m_textEdit, QIcon(":/images/ok.svg"), message);
    }
}

bool EditWrapper::getTextChangeFlag()
{
    return m_bTextChange;
}

void EditWrapper::setTextChangeFlag(bool bFlag)
{
    m_bTextChange = bFlag;
}

void EditWrapper::onFileClosed()
{
    m_textEdit->clearBlack();
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

void EditWrapper::handleFileLoadFinished(const QByteArray &encode,const QString &content)
{
    // restore mouse style.
    // QApplication::restoreOverrideCursor();

    if (!Utils::isDraftFile(m_textEdit->filepath)) {
        DRecentData data;
        data.appName = "Deepin Editor";
        data.appExec = "deepin-editor";
        DRecentManager::addItem(m_textEdit->filepath, data);
    }

    bool flag = m_textEdit->getReadOnlyPermission();
    if(flag == true) m_textEdit->setReadOnlyPermission(false);

    m_isLoadFinished = true;

    m_bFileLoading = true;

    m_BeforeEncodeName = encode;
    if (m_BeforeEncodeName.isEmpty()) {
        QString str = "UTF-8";
        m_BeforeEncodeName = str.toLocal8Bit();
    }

    setTextCodec(encode);

    // set text.
    m_textEdit->loadHighlighter();
    m_textEdit->clear();

    //tab点击关闭
    m_bQuit = false;
    QTextDocument *doc = m_textEdit->document();
    QTextCursor cursor = m_textEdit->textCursor();
    int len = content.length();
    //初始化显示文本大小
    int InitContentPos = 5*1024;
    //每次读取文件步长
    int step = 1*1024*1024;
    //循环读取次数
    int cnt = len / step;
    //文件末尾余数
    int mod = len % step;

    int max = 40*1024*1024;
    QString data;
    if(len > max){
        for (int i = 0; i < cnt; i++) {
            //初始化秒开
            if(i == 0 && !m_bQuit){
              data = content.mid(i*step,InitContentPos);
              cursor.insertText(data);
              m_textEdit->setModified(false);
              QApplication::processEvents();
              continue;
            }
            if(!m_bQuit){
                data= content.mid(i*step,step);
                cursor.insertText(data);
                m_textEdit->setModified(false);
                QApplication::processEvents();
                if(!m_bQuit && i == cnt -1 && mod > 0){
                    data = content.mid(cnt*step,mod);
                    cursor.insertText(data);
                    m_textEdit->setModified(false);
                    QApplication::processEvents();
                }
            }
        }

    }else {
        //初始化秒开
        if(!m_bQuit && len > InitContentPos){
            data = content.mid(0,InitContentPos);
            cursor.insertText(data);
            QApplication::processEvents();

            if(!m_bQuit){
                data = content.mid(InitContentPos,len-InitContentPos);
                cursor.insertText(data);
            }
        }else {
           if(!m_bQuit) cursor.insertText(content);
        }
    }


    PerformanceMonitor::openFileFinish(filePath(), QFileInfo(filePath()).size());

    m_bFileLoading = false;
    if(flag == true) m_textEdit->setReadOnlyPermission(true);

    if(m_bQuit) return;

    m_textEdit->setTextFinished();

//    m_textEdit->clearBlack();
    QApplication::restoreOverrideCursor();
    // update status.
    m_textEdit->setModified(false);
//    m_textEdit->moveToStart();

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

//yanyuhan
void EditWrapper::setLineNumberShow(bool bIsShow ,bool bIsFirstShow)
{
    if(bIsShow && !bIsFirstShow) {
        int lineNumberAreaWidth = m_textEdit->m_pLeftAreaWidget->m_linenumberarea->width();
        int leftAreaWidth = m_textEdit->m_pLeftAreaWidget->width();
        m_textEdit->m_pLeftAreaWidget->m_linenumberarea->show();
        m_textEdit->m_pLeftAreaWidget->setFixedWidth(leftAreaWidth + lineNumberAreaWidth);

    } else if(!bIsShow) {
        int lineNumberAreaWidth = m_textEdit->m_pLeftAreaWidget->m_linenumberarea->width();
        int leftAreaWidth = m_textEdit->m_pLeftAreaWidget->width();
        m_textEdit->m_pLeftAreaWidget->m_linenumberarea->hide();
        m_textEdit->m_pLeftAreaWidget->setFixedWidth(leftAreaWidth - lineNumberAreaWidth);
    }
    m_textEdit->bIsSetLineNumberWidth = bIsShow;
    m_textEdit->updateLineNumber();
}

//显示空白符
void EditWrapper::setShowBlankCharacter(bool ok)
{
    if(ok){
        QTextOption opts = m_textEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag |= QTextOption::ShowTabsAndSpaces;
//        flag |= QTextOption::ShowLineAndParagraphSeparators;
//        flag |= QTextOption::AddSpaceForLineAndParagraphSeparators;
        opts.setFlags(flag);
        m_textEdit->document()->setDefaultTextOption(opts);
    }else {
        QTextOption opts = m_textEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag &= ~QTextOption::ShowTabsAndSpaces;
//        flag &= ~QTextOption::ShowLineAndParagraphSeparators;
//        flag &= ~QTextOption::AddSpaceForLineAndParagraphSeparators;
        opts.setFlags(flag);
        m_textEdit->document()->setDefaultTextOption(opts);
    }
}
