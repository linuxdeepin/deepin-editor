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
#include "../widgets/window.h"
#include "../encodes/detectcode.h"
#include "../controls/toast.h"
#include "../common/fileloadthread.h"
#include "editwrapper.h"
#include "../common/utils.h"
#include "leftareaoftextedit.h"
#include "drecentmanager.h"

#include <unistd.h>
#include <QCoreApplication>
#include <QApplication>
#include <QSaveFile>
#include <QScrollBar>
#include <QScroller>
#include <QDebug>
#include <QTimer>
#include <QDir>


DCORE_USE_NAMESPACE

EditWrapper::EditWrapper(Window* window,QWidget *parent)
    : QWidget(parent),
      m_pTextEdit(new TextEdit(this)),
      m_pBottomBar(new BottomBar(this)),
      m_pWaringNotices(new WarningNotices(WarningNotices::ResidentType,this)),
      m_pWindow(window)
{
    m_pWaringNotices->hide();
    // Init layout and widgets.
    QHBoxLayout* m_layout = new QHBoxLayout;
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_pTextEdit->m_pLeftAreaWidget);
    m_layout->addWidget(m_pTextEdit);

    m_pBottomBar->setHighlightMenu(m_pTextEdit->getHighlightMenu());
    m_pTextEdit->setWrapper(this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(m_layout);
    mainLayout->addWidget(m_pBottomBar);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);

    connect(m_pTextEdit, &TextEdit::cursorModeChanged, this, &EditWrapper::handleCursorModeChanged);
    connect(m_pTextEdit, &TextEdit::hightlightChanged, this, &EditWrapper::handleHightlightChanged);
    connect(m_pTextEdit, &TextEdit::textChanged, this, &EditWrapper::slotTextChange);

    connect(m_pWaringNotices, &WarningNotices::reloadBtnClicked, this, &EditWrapper::reloadModifyFile);
    connect(m_pWaringNotices, &WarningNotices::saveAsBtnClicked, this, &EditWrapper::requestSaveAs);
}

EditWrapper::~EditWrapper()
{
    if(m_pTextEdit) delete m_pTextEdit;
}

void EditWrapper::setQuitFlag()
{
    m_bQuit = true;
}

bool EditWrapper::getFileLoading()
{
    return (m_bQuit || m_bFileLoading);
}

void EditWrapper::openFile(const QString &filepath)
{
    // update file path.
    updatePath(filepath);

    m_pTextEdit->setIsFileOpen();

    FileLoadThread *thread = new FileLoadThread(filepath);

    // begin to load the file.
    connect(thread, &FileLoadThread::loadFinished, this, &EditWrapper::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);

    thread->start();
}

bool EditWrapper::readFile(QByteArray encode)
{
   QByteArray newEncode = encode;
   if(newEncode.isEmpty()){
      newEncode = DetectCode::GetFileEncodingFormat(m_pTextEdit->filepath);
      m_sFirstEncode = newEncode;
   }

    QFile file(m_pTextEdit->filepath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileContent = file.readAll();
        QByteArray Outdata;
        DetectCode::ChangeFileEncodingFormat(fileContent,Outdata,newEncode,QString("UTF-8"));
        loadContent(Outdata);
        file.close();
        m_sCurEncode = newEncode;
        m_pTextEdit->setModified(false);
        return true;
    }
    return false;
}

bool EditWrapper::saveAsFile(const QString &newFilePath, QByteArray encodeName)
{
    // use QSaveFile for safely save files.
    QSaveFile saveFile(newFilePath);
    saveFile.setDirectWriteFallback(true);

    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QFile file(newFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    //auto append new line char to end of file when file's format is Linux/MacOS
    QByteArray fileContent = m_pTextEdit->toPlainText().toUtf8();

    QByteArray Outdata;
    DetectCode::ChangeFileEncodingFormat(fileContent,Outdata,QString("UTF-8"),encodeName);
    file.write(Outdata);
    // close and delete file.
    QFileDevice::FileError error = file.error();
    file.close();

    // flush file.
    if (!saveFile.flush()) {
        return false;
    }

    // ensure that the file is written to disk
    fsync(saveFile.handle());
    QFileInfo fi(filePath());
    m_tModifiedDateTime = fi.lastModified();

    // did save work?
    // only finalize if stream status == OK
    bool ok = (error == QFileDevice::NoError);

    return ok;
}

bool EditWrapper::saveAsFile()
{
    DFileDialog dialog(this, tr("Save"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(QObject::tr("Encoding"),  QStringList() << m_sFirstEncode);
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("*.txt");

    this->setUpdatesEnabled(false);
    int mode =  dialog.exec();
    this->setUpdatesEnabled(true);
    hideWarningNotices();

    if(QDialog::Accepted == mode)
    {
        const QString newFilePath = dialog.selectedFiles().value(0);
        if (newFilePath.isEmpty())
            return false;

        QFile qfile(newFilePath);

        if (!qfile.open(QFile::WriteOnly | QIODevice::Truncate)) {
            return false;
        }

        // 以新的编码保存内容到文件
        QByteArray inputData = m_pTextEdit->toPlainText().toUtf8();
        QByteArray outData;
        DetectCode::ChangeFileEncodingFormat(inputData,outData,QString("UTF-8"),m_sFirstEncode);
        qfile.write(outData);
        qfile.close();

        return true;
    }

    return false;
}

bool EditWrapper::reloadFileEncode(QByteArray encode)
{
    //切换编码相同不重写加载
    if(m_sCurEncode == encode) return false;


    //草稿文件 空白文件不保存
    if(Utils::isDraftFile(m_pTextEdit->filepath) &&  m_pTextEdit->toPlainText().isEmpty()) {
        m_sCurEncode = encode;
        m_sFirstEncode = encode;
        return true;
    }


    //1.如果修改切换编码提示用户是否保存,不保存重新打开文件读取.2.没有修改是否另存为
    if(m_pTextEdit->document()->isModified())
    {
        DDialog *dialog = new DDialog(tr("Do you want to save this file?"), "", this);
        dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
        dialog->setIcon(QIcon::fromTheme("deepin-editor"));
        dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);//不保存
        dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);//取消
        dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);//保存
        int res = dialog->exec();//0  1

        //关闭对话框
        if(res == 0) return false;

        //不保存,重写载入
        if(res == 1)
        {
            bool ok = readFile(encode);
            //if(ok && m_sCurEncode != m_sFirstEncode) m_pTextEdit->setTabbarModified(true);
            return ok;
        }

        //保存
        if(res == 2){
            //草稿文件
            if(Utils::isDraftFile(m_pTextEdit->filepath)){
                if(saveDraftFile()) return readFile(encode);
                else return false;
            }else {
                return (saveFile() && readFile(encode));
            }
        }

        return false;
    }else {
        return readFile(encode);
    }
}

void EditWrapper::reloadModifyFile()
{
    hideWarningNotices();

    int curPos = m_pTextEdit->textCursor().position();
    int yoffset = m_pTextEdit->verticalScrollBar()->value();
    int xoffset = m_pTextEdit->horizontalScrollBar()->value();

    //如果文件修改提示用户是否保存  如果临时文件保存就是另存为
    if (m_pTextEdit->document()->isModified()) {
        DDialog *dialog = new DDialog(tr("Do you want to save this file?"), "", this);
        dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
        dialog->setIcon(QIcon::fromTheme("deepin-editor"));
        dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);//不保存
        dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);//取消
        dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);//保存
        dialog->setCloseButtonVisible(false);
        int res = dialog->exec();//0  1

        //点击关闭
        if(res == 0) return;

        //不保存
        if(res == 1) {
            //重写加载文件
            readFile();
        }
        //另存
        if(res == 2){
            //临时文件保存另存为 需要删除源草稿文件文件
           if(Utils::isDraftFile(m_pTextEdit->filepath)){
               if(!saveDraftFile()) return;
           }
           else {
              if(!saveAsFile()) return;
           }
           //重写加载文件
           readFile();
        }

    }else{
        //重写加载文件
        readFile();
    }

    QFileInfo fi(m_pTextEdit->filepath);
    m_tModifiedDateTime = fi.lastModified();

    QTextCursor textcur = m_pTextEdit->textCursor();
    textcur.setPosition(curPos);
    m_pTextEdit->setTextCursor(textcur);
    m_pTextEdit->verticalScrollBar()->setValue(yoffset);
    m_pTextEdit->horizontalScrollBar()->setValue(xoffset);
}

QString EditWrapper::getTextEncode()
{
    return m_sCurEncode;
}

bool EditWrapper::saveFile()
{
    QFile file(m_pTextEdit->filepath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray fileContent = m_pTextEdit->toPlainText().toLocal8Bit();
        if(!fileContent.isEmpty())
        {
            QByteArray Outdata;
            DetectCode::ChangeFileEncodingFormat(fileContent,Outdata,QString("UTF-8"),m_sCurEncode);
            file.write(Outdata);
            QFileDevice::FileError error = file.error();
            file.close();
            m_sFirstEncode = m_sCurEncode;

            QFileInfo fi(filePath());
            m_tModifiedDateTime = fi.lastModified();

            // did save work?
            // only finalize if stream status == OK
            bool ok = (error == QFileDevice::NoError);

            // update status.
            if (ok)  m_pTextEdit->setModified(false);
            return ok;

        }else {
            file.write(fileContent);
            QFileDevice::FileError error = file.error();
            file.close();
            m_sFirstEncode = m_sCurEncode;

            QFileInfo fi(filePath());
            m_tModifiedDateTime = fi.lastModified();

            // did save work?
            // only finalize if stream status == OK
            bool ok = (error == QFileDevice::NoError);

            // update status.
            if (ok)  m_pTextEdit->setModified(false);
            return ok;
        }
    }else {
        return false;
    }

}

void EditWrapper::updatePath(const QString &file)
{
    QFileInfo fi(file);
    m_tModifiedDateTime = fi.lastModified();
    m_pTextEdit->filepath = file;
}

bool EditWrapper::isModified()
{
    //编码改变内容没有修改也算是文件修改
   // bool modified = (m_sFirstEncode != m_sCurEncode || m_pTextEdit->document()->isModified());
    bool modified =  m_pTextEdit->document()->isModified();
    return  modified;
}

bool EditWrapper::isDraftFile()
{
    return Utils::isDraftFile(m_pTextEdit->filepath);
}

bool EditWrapper::isPlainTextEmpty()
{
    return m_pTextEdit->document()->isEmpty();
}

bool EditWrapper::saveDraftFile()
{
    DFileDialog dialog(this, tr("Save"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(QObject::tr("Encoding"),  QStringList() << m_sCurEncode);
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("*.txt");

    if(m_pWindow){
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(m_pWindow->getTabbar()->currentName());
        dialog.selectFile(match.captured(0) + ".txt");
    }



    this->setUpdatesEnabled(false);
    int mode =  dialog.exec(); // 0表示取消 1保存
    this->setUpdatesEnabled(true);
    hideWarningNotices();

    if(mode == 1)
    {
        const QString newFilePath = dialog.selectedFiles().value(0);
        if (newFilePath.isEmpty())
            return false;

        QFile qfile(newFilePath);

        if (!qfile.open(QFile::WriteOnly)) {
            return false;
        }

        // 以新的编码保存内容到文件
        QByteArray inputData = m_pTextEdit->toPlainText().toUtf8();
        QByteArray outData;
        DetectCode::ChangeFileEncodingFormat(inputData,outData,QString("UTF-8"),m_sCurEncode);
        qfile.write(outData);
        qfile.close();

        //草稿文件保存 等同于重写打开
        m_sFirstEncode = m_sCurEncode;
        QFile(m_pTextEdit->filepath).remove();
        emit sigCodecSaveFile(m_pTextEdit->filepath,newFilePath);
        //updatePath(newFilePath);
        m_pTextEdit->document()->setModified(false);
        return true;
    }

    return false;
}

void EditWrapper::hideWarningNotices()
{
    if (m_pWaringNotices->isVisible()) {
        m_pWaringNotices->hide();
    }
}

//除草稿文件 检查文件是否被删除,是否被修复
void EditWrapper::checkForReload()
{
    if (Utils::isDraftFile(m_pTextEdit->filepath)) return;

    QFileInfo fi(m_pTextEdit->filepath);

    if (fi.lastModified() == m_tModifiedDateTime || m_pWaringNotices->isVisible()) return;


    if (!fi.exists()) {
        m_pWaringNotices->setMessage(tr("File removed on the disk. Save it now?"));
        m_pWaringNotices->setSaveAsBtn();
    }else if(fi.lastModified() != m_tModifiedDateTime){
        m_pWaringNotices->setMessage(tr("File has changed on disk. Reload?"));
        m_pWaringNotices->setReloadBtn();
    }

    m_pWaringNotices->show();
    DMessageManager::instance()->sendMessage(m_pTextEdit, m_pWaringNotices);
}

void EditWrapper::showNotify(const QString &message)
{
    if (m_pTextEdit->getReadOnlyPermission() || m_pTextEdit->getReadOnlyMode()) {
        DMessageManager::instance()->sendMessage(m_pTextEdit, QIcon(":/images/warning.svg"), message);
    } else {
        DMessageManager::instance()->sendMessage(m_pTextEdit, QIcon(":/images/ok.svg"), message);
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

void EditWrapper::handleCursorModeChanged(TextEdit::CursorMode mode)
{
    switch (mode) {
    case TextEdit::Insert:
        m_pBottomBar->setCursorStatus(tr("INSERT"));
        break;
    case TextEdit::Overwrite:
        m_pBottomBar->setCursorStatus(tr("OVERWRITE"));
        break;
    case TextEdit::Readonly:
        m_pBottomBar->setCursorStatus(tr("R/O"));
        break;
    }
}

void EditWrapper::handleHightlightChanged(const QString &name)
{
    m_pBottomBar->setHightlightName(name);
}

void EditWrapper::handleFileLoadFinished(const QByteArray &encode,const QByteArray &content)
{   

    if (!Utils::isDraftFile(m_pTextEdit->filepath)) {
        DRecentData data;
        data.appName = "Deepin Editor";
        data.appExec = "deepin-editor";
        DRecentManager::addItem(m_pTextEdit->filepath, data);
    }

    bool flag = m_pTextEdit->getReadOnlyPermission();
    if(flag == true) m_pTextEdit->setReadOnlyPermission(false);

    m_bFileLoading = true;
    m_sCurEncode = encode;
    m_sFirstEncode = encode;
    //设置语法高亮
    m_pTextEdit->loadHighlighter();
    loadContent(content);

    PerformanceMonitor::openFileFinish(filePath(), QFileInfo(filePath()).size());

    m_bFileLoading = false;
    if(flag == true) m_pTextEdit->setReadOnlyPermission(true);
    if(m_bQuit) return;
    m_pTextEdit->setTextFinished();

    m_pTextEdit->setModified(false);
    m_pBottomBar->setEncodeName(m_sCurEncode);
}


void EditWrapper::slotTextChange()
{
    m_bTextChange = true;
}

//yanyuhan
void EditWrapper::setLineNumberShow(bool bIsShow ,bool bIsFirstShow)
{
    if(bIsShow && !bIsFirstShow) {
        int lineNumberAreaWidth = m_pTextEdit->m_pLeftAreaWidget->m_pLineNumberArea->width();
        int leftAreaWidth = m_pTextEdit->m_pLeftAreaWidget->width();
        m_pTextEdit->m_pLeftAreaWidget->m_pLineNumberArea->show();
        m_pTextEdit->m_pLeftAreaWidget->setFixedWidth(leftAreaWidth + lineNumberAreaWidth);

    } else if(!bIsShow) {
        int lineNumberAreaWidth = m_pTextEdit->m_pLeftAreaWidget->m_pLineNumberArea->width();
        int leftAreaWidth = m_pTextEdit->m_pLeftAreaWidget->width();
        m_pTextEdit->m_pLeftAreaWidget->m_pLineNumberArea->hide();
        m_pTextEdit->m_pLeftAreaWidget->setFixedWidth(leftAreaWidth - lineNumberAreaWidth);
    }
    m_pTextEdit->bIsSetLineNumberWidth = bIsShow;
    m_pTextEdit->updateLineNumber();
}

//显示空白符
void EditWrapper::setShowBlankCharacter(bool ok)
{
    if(ok){
        QTextOption opts = m_pTextEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag |= QTextOption::ShowTabsAndSpaces;
        opts.setFlags(flag);
        m_pTextEdit->document()->setDefaultTextOption(opts);
    }else {
        QTextOption opts = m_pTextEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag &= ~QTextOption::ShowTabsAndSpaces;
        opts.setFlags(flag);
        m_pTextEdit->document()->setDefaultTextOption(opts);
    }
}

BottomBar *EditWrapper::bottomBar()
{
    return m_pBottomBar;
}

QString EditWrapper::filePath()
{
    return  m_pTextEdit->filepath;
}

TextEdit *EditWrapper::textEditor()
{
    return m_pTextEdit;
}

//支持大文本加载 界面不卡顿 秒关闭
void EditWrapper::loadContent(const QByteArray &content)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_pTextEdit->clear();
    m_bQuit = false;
    QTextDocument *doc = m_pTextEdit->document();
    QTextCursor cursor = m_pTextEdit->textCursor();
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

    QByteArray data;
    if(len > max){
        for (int i = 0; i < cnt; i++) {
            //初始化秒开
            if(i == 0 && !m_bQuit){
              data = content.mid(i*step,InitContentPos);
              cursor.insertText(data);
              QApplication::processEvents();
              continue;
            }
            if(!m_bQuit){
                data= content.mid(i*step,step);
                cursor.insertText(data);
                QApplication::processEvents();
                if(!m_bQuit && i == cnt -1 && mod > 0){
                    data = content.mid(cnt*step,mod);
                    cursor.insertText(data);
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

    doc->setModified(false);
    QApplication::restoreOverrideCursor();
}
