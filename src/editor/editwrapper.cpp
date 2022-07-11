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
#include "../common/fileloadthread.h"
#include "../widgets/pathsettintwgt.h"
#include "editwrapper.h"
#include "../common/utils.h"
#include "leftareaoftextedit.h"
#include "drecentmanager.h"
#include "../common/settings.h"
#include <DSettingsOption>
#include <DSettings>
#include <unistd.h>
#include <QCoreApplication>
#include <QApplication>
#include <QSaveFile>
#include <QScrollBar>
#include <QScroller>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <DSettingsOption>
#include <DMenuBar>
#include <QFileInfo>
#include <QEvent>

DCORE_USE_NAMESPACE

enum ReadLengthType {
    EReadStepSize = 1 * 1024 * 1024,    // 单次读取文件最大长度
};

/**
 * @brief 处理文件时使用的事件类型，处理解析文件数据时，
 *      将此事件抛给事件队列，使用事件队列分发解析任务
 */
class ParseFileEvent : public QEvent
{
public:
    enum Type {
        EParseFile = QEvent::User + 1024,
    };

    ParseFileEvent();
    virtual ~ParseFileEvent();

    // 返回此事件的克隆对象，用于下次任务。
    ParseFileEvent *clone();

    // 内部公开数据
    int             m_alreadyReadOffset = 0;    // 当前已读取文本大小
    QByteArray      m_contentData;              // 文本内容
    QTextCursor     m_cursor;                   // 插入光标
    QTextCodec      *m_codec = nullptr;         // 编码
};

ParseFileEvent::ParseFileEvent()
    : QEvent(static_cast<QEvent::Type>(EParseFile))
{
}

ParseFileEvent::~ParseFileEvent()
{
}

ParseFileEvent *ParseFileEvent::clone()
{
    // 创建克隆对象，复制数据(浅拷贝)
    ParseFileEvent *cloneEvent = new ParseFileEvent;
    cloneEvent->m_contentData = this->m_contentData;
    cloneEvent->m_alreadyReadOffset = this->m_alreadyReadOffset;
    cloneEvent->m_cursor = this->m_cursor;
    cloneEvent->m_codec = this->m_codec;
    return cloneEvent;
}


EditWrapper::EditWrapper(Window *window, QWidget *parent)
    : QWidget(parent),
      m_pWindow(window),
      m_pTextEdit(new TextEdit(this)),
      m_pBottomBar(new BottomBar(this)),
      m_pWaringNotices(new WarningNotices(WarningNotices::ResidentType, this))

{
    m_bQuit = false;
    m_pWaringNotices->hide();
    // Init layout and widgets.
    QHBoxLayout *m_layout = new QHBoxLayout;
    m_pLeftAreaTextEdit = m_pTextEdit->getLeftAreaWidget();
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_pLeftAreaTextEdit);
    m_layout->addWidget(m_pTextEdit);
    m_pTextEdit->setWrapper(this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(m_layout);
    mainLayout->addWidget(m_pBottomBar);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);

    connect(m_pTextEdit, &TextEdit::cursorModeChanged, this, &EditWrapper::handleCursorModeChanged);
    connect(m_pWaringNotices, &WarningNotices::reloadBtnClicked, this, &EditWrapper::reloadModifyFile);
    connect(m_pWaringNotices, &WarningNotices::saveAsBtnClicked, m_pWindow, &Window::saveAsFile);
    connect(m_pTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        OnUpdateHighlighter();
        if ((m_pWindow->findBarIsVisiable() || m_pWindow->replaceBarIsVisiable()) &&
                (QString::compare(m_pWindow->getKeywordForSearchAll(), m_pWindow->getKeywordForSearch(), Qt::CaseInsensitive) == 0)) {
            m_pTextEdit->highlightKeywordInView(m_pWindow->getKeywordForSearchAll());
        }

        m_pTextEdit->markAllKeywordInView();
    });
}

EditWrapper::~EditWrapper()
{
    if (m_pTextEdit != nullptr) {
        disconnect(m_pTextEdit);
        delete m_pTextEdit;
        m_pTextEdit = nullptr;
    }
    if (m_pBottomBar != nullptr) {
        disconnect(m_pBottomBar);
        delete m_pBottomBar;
        m_pBottomBar = nullptr;
    }
    //delete 之后，如果出现文件被修改，需要重新加载弹框，之后，点击标签关闭，闪退问题　78042 ut002764
//    if (m_pWaringNotices != nullptr) {
//    disconnect(m_pWaringNotices);
//        delete m_pWaringNotices;
//        m_pWaringNotices = nullptr;
//    }
}

void EditWrapper::setQuitFlag()
{
    m_bQuit = true;
}

bool EditWrapper::isQuit()
{
    return m_bQuit;
}

bool EditWrapper::getFileLoading()
{
    return (m_bQuit || m_bFileLoading);
}

void EditWrapper::openFile(const QString &filepath, QString qstrTruePath, bool bIsTemFile)
{
    m_bIsTemFile = bIsTemFile;
    // update file path.
    updatePath(filepath, qstrTruePath);
    m_pTextEdit->setIsFileOpen();

   if (!bIsTemFile && !isDraftFile()) {
       Settings::instance()->setSavePath(PathSettingWgt::CurFileBox, QFileInfo(qstrTruePath).absolutePath());
   }

    FileLoadThread *thread = new FileLoadThread(filepath);
    // begin to load the file.
    connect(thread, &FileLoadThread::sigLoadFinished, this, &EditWrapper::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);
    thread->start();
}

bool EditWrapper::readFile(QByteArray encode)
{
    QByteArray newEncode = encode;
    if (newEncode.isEmpty()) {
        newEncode = DetectCode::GetFileEncodingFormat(m_pTextEdit->getFilePath());
        m_sFirstEncode = newEncode;
    }

    //QFile file(m_pTextEdit->getFilePath());
    QFile file2(m_pTextEdit->getTruePath());

    if (file2.open(QIODevice::ReadOnly)) {
        QByteArray fileContent = file2.readAll();
        QByteArray Outdata;
        DetectCode::ChangeFileEncodingFormat(fileContent, Outdata, newEncode, QString("UTF-8"));
        loadContent(Outdata);
        file2.close();
        m_sCurEncode = newEncode;
        updateModifyStatus(false);
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
    DetectCode::ChangeFileEncodingFormat(fileContent, Outdata, QString("UTF-8"), encodeName);
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

    //this->setUpdatesEnabled(false);
    int mode =  dialog.exec();
    //this->setUpdatesEnabled(true);
    hideWarningNotices();

    if (QDialog::Accepted == mode) {
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
        DetectCode::ChangeFileEncodingFormat(inputData, outData, QString("UTF-8"), m_sFirstEncode);
        qfile.write(outData);
        qfile.close();

        return true;
    }

    return false;
}

bool EditWrapper::reloadFileEncode(QByteArray encode)
{
    //切换编码相同不重写加载
    if (m_sCurEncode == encode) {
        return false;
    }

    //草稿文件 空白文件不保存
    if (Utils::isDraftFile(m_pTextEdit->getFilePath()) &&  m_pTextEdit->toPlainText().isEmpty()) {
        m_sCurEncode = encode;
        m_sFirstEncode = encode;
        return true;
    }

    //temporarily use '*' to determine whether the text has been modified.
    auto tabname = this->window()->getTabbar()->currentName();
    bool hasFlag = false;
    if (tabname.size() > 0 && "*" == tabname[0]) {
        hasFlag = true;
    }

    //1.如果修改切换编码提示用户是否保存,不保存重新打开文件读取.2.没有修改是否另存为
    if (m_pTextEdit->getModified() || hasFlag) {
        DDialog *dialog = new DDialog(tr("Encoding changed. Do you want to save the file now?"), "", this);
        //dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnBottomHint);
        dialog->setIcon(QIcon::fromTheme("deepin-editor"));
        dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);   //取消
        //dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);//不保存
        dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);   //保存
        int res = dialog->exec();//0  1

        //关闭对话框
        if (res == 0) {
            return false;
        }

        //不保存,重写载入
#if 0
        if (res == 1) {
            bool ok = readFile(encode);
            //if(ok && m_sCurEncode != m_sFirstEncode) m_pTextEdit->setTabbarModified(true);
            return ok;
        }
#endif

        //保存
        if (res == 1) {
            //草稿文件
            if (Utils::isDraftFile(m_pTextEdit->getFilePath())) {
                if (saveDraftFile()) {
                    return readFile(encode);
                } else {
                    return false;
                }
            } else {
                return (saveFile() && readFile(encode));
            }
        }

        return false;
    } else {
        return readFile(encode);
    }
}

void EditWrapper::reloadModifyFile()
{
    hideWarningNotices();

    int curPos = m_pTextEdit->textCursor().position();
    int yoffset = m_pTextEdit->verticalScrollBar()->value();
    int xoffset = m_pTextEdit->horizontalScrollBar()->value();

    bool bIsModified = m_pTextEdit->getModified();

    if (m_pWindow->getTabbar()->textAt(m_pWindow->getTabbar()->currentIndex()).front() == "*") {
        bIsModified = true;
    }
    //如果文件修改提示用户是否保存  如果临时文件保存就是另存为
    if (bIsModified) {
        DDialog *dialog = new DDialog(tr("Do you want to save this file?"), "", this);
        dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnBottomHint);
        dialog->setIcon(QIcon::fromTheme("deepin-editor"));
        dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);//不保存
        dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);//取消
        dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);//保存
        dialog->setCloseButtonVisible(false);
        int res = dialog->exec();//0  1

        //点击关闭
        if (res == 0) {
            return;
        }

        //不保存
        if (res == 1) {
            //重写加载文件
            readFile();
            m_bIsTemFile = false;
        }
        //另存
        if (res == 2) {
            //临时文件保存另存为 需要删除源草稿文件文件
            if (Utils::isDraftFile(m_pTextEdit->getFilePath())) {
                if (!saveDraftFile()) {
                    return;
                }
            } else {
                if (!saveAsFile()) {
                    return;
                }
            }
            //重写加载文件
            readFile();
            m_bIsTemFile = false;
        }

    } else {
        //重写加载文件
        readFile();
    }
    m_pTextEdit->setBookMarkList(QList<int>());
    QFileInfo fi(m_pTextEdit->getTruePath());
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
    QString qstrFilePath = m_pTextEdit->getTruePath();
    QFile file(qstrFilePath);
    hideWarningNotices();

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray fileContent;
        getPlainTextContent(fileContent);
        if (!fileContent.isEmpty()) {
            QByteArray Outdata;
            DetectCode::ChangeFileEncodingFormat(fileContent, Outdata, QString("UTF-8"), m_sCurEncode);
            // 如果 iconv 转换错误
            if (Outdata.size() == 0) {
                qWarning() << QString("iconv Encode Transformat from '%1' to '%2' Fail!")
                           .arg(QString("UTF-8")).arg(m_sCurEncode)
                           << ", start QTextCodec Encode Transformat.";
                // 使用 QTextCodec 进行转换尝试
                QTextCodec *codec = QTextCodec::codecForName(m_sCurEncode.toUtf8());
                QByteArray encodedString = codec->fromUnicode(fileContent);

                if (encodedString.isEmpty()) {
                    qWarning() << "Both iconv and QTextCodec Encode Transformat Fail!";
                } else {
                    qWarning() << QString("QTextCodec Encode Transformat from '%1' to '%2' Success!")
                               .arg(QString("UTF-8")).arg(m_sCurEncode);
                    Outdata = encodedString;
                }
            }

            if (Outdata.isEmpty() == false) {
                // 如果新数据为空，不进行文件写入，以降低文件内容损失
                // 此时如果写入，整个文件将被清空
                file.write(Outdata);
            }

            QFileDevice::FileError error = file.error();
            file.close();
            m_sFirstEncode = m_sCurEncode;

            QFileInfo fi(qstrFilePath);
            m_tModifiedDateTime = fi.lastModified();

            // did save work?
            // only finalize if stream status == OK
            // 增加对于转换失败的判断，新数据为空，ok返回false
            bool ok = (Outdata.isEmpty() == false && error == QFileDevice::NoError);

            // update status.
            if (ok)  updateModifyStatus(false);
            m_bIsTemFile = false;
            return ok;
        } else {
            file.write(fileContent);
            QFileDevice::FileError error = file.error();
            file.close();
            m_sFirstEncode = m_sCurEncode;

            QFileInfo fi(qstrFilePath);
            m_tModifiedDateTime = fi.lastModified();

            // did save work?
            // only finalize if stream status == OK
            bool ok = (error == QFileDevice::NoError);

            // update status.
            if (ok)  updateModifyStatus(false);
            m_bIsTemFile = false;
            return ok;
        }
    } else {
        DMessageManager::instance()->sendMessage(this->window()->getStackedWgt()->currentWidget(), QIcon(":/images/warning.svg")
                                                 , QString(tr("You do not have permission to save %1")).arg(file.fileName()));
        return false;
    }
}

void EditWrapper::getPlainTextContent(QByteArray &plainTextContent)
{
    QString strPlainText = m_pTextEdit->toPlainText();
    if (BottomBar::EndlineFormat::Windows == m_pBottomBar->getEndlineFormat()) {
        strPlainText.replace("\n", "\r\n");
    }

    qint64 iPlainTextAllLen = strPlainText.length();
    qint64 iStep = 300 * DATA_SIZE_1024 * DATA_SIZE_1024;
    /* qt开发分析结论：大文本情况下toLocal8Bit()转换会引起应用闪退，此闪退问题qt方无法处理 */
    /* 如果文本框里的文本内容小于等于300MB，无需分段转换 */
    if (iPlainTextAllLen <= iStep) {
        plainTextContent = QByteArray(strPlainText.toLocal8Bit());
        return;
    }

    /*
     * 如果文本框里的文本内容大于300MB，则采用分段转换，每段以300MB为单位
     * 规避大文toLocal8Bit()转换时导致应用闪退的qt缺陷问题
     */
    qint64 iSections = iPlainTextAllLen / (300 * DATA_SIZE_1024 * DATA_SIZE_1024);
    qint64 iResidue = iPlainTextAllLen % iStep;

    for (int i = 0; i < iSections; i++) {
        plainTextContent += strPlainText.mid(i * iStep, iStep).toLocal8Bit();
        QApplication::processEvents();

        if (i == iSections - 1 && iResidue > 0) {
            plainTextContent += strPlainText.mid(iSections * iStep, iResidue).toLocal8Bit();
        }
    }
}

bool EditWrapper::saveTemFile(QString qstrDir)
{
    QFile file(qstrDir);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray fileContent;
        getPlainTextContent(fileContent);
        QByteArray Outdata;
        DetectCode::ChangeFileEncodingFormat(fileContent, Outdata, QString("UTF-8"), m_sCurEncode);
        file.write(Outdata);
        QFileDevice::FileError error = file.error();
        file.close();
        m_sFirstEncode = m_sCurEncode;

        // did save work?
        // only finalize if stream status == OK
        bool ok = (error == QFileDevice::NoError);

        // update status.
        if (ok) {
            updateModifyStatus(isModified());
        }
        return ok;

#if 0
    } else {
        file.write(fileContent);
        QFileDevice::FileError error = file.error();
        file.close();
        m_sFirstEncode = m_sCurEncode;

        did save work ?
        only finalize if stream status == OK
        bool ok = (error == QFileDevice::NoError);

        // update status.
        if (ok)  updateModifyStatus(true);
        return ok;
    }
#endif
} else
{
    return false;
}
}

void EditWrapper::updatePath(const QString &file, QString qstrTruePath)
{
    if (qstrTruePath.isEmpty()) {
        qstrTruePath = file;
    }

    QFileInfo fi(qstrTruePath);
    m_tModifiedDateTime = fi.lastModified();

    m_pTextEdit->setFilePath(file);
    m_pTextEdit->setTruePath(qstrTruePath);
}

bool EditWrapper::isModified()
{
    //编码改变内容没有修改也算是文件修改
    // bool modified = (m_sFirstEncode != m_sCurEncode || m_pTextEdit->document()->isModified());
    bool modified =  m_pTextEdit->getModified();

    return  modified | m_bIsTemFile;
}

bool EditWrapper::isDraftFile()
{
    return Utils::isDraftFile(m_pTextEdit->getFilePath());
}

/**
 * @return 返回当前编辑文件是否为备份文件
 */
bool EditWrapper::isBackupFile()
{
    return Utils::isBackupFile(m_pTextEdit->getFilePath());
}

bool EditWrapper::isPlainTextEmpty()
{
    return m_pTextEdit->document()->isEmpty();
}

bool EditWrapper::isTemFile()
{
    return m_bIsTemFile;
}

bool EditWrapper::saveDraftFile()
{
    DFileDialog dialog(this, tr("Save"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(QObject::tr("Encoding"),  QStringList() << m_sCurEncode);
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("*.txt");

    if (m_pWindow) {
        m_pWindow = this->window();
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(m_pWindow->getTabbar()->currentName());
        dialog.selectFile(match.captured(0) + ".txt");
    }

    //this->setUpdatesEnabled(false);
    int mode =  dialog.exec(); // 0表示取消 1保存
    // this->setUpdatesEnabled(true);
    hideWarningNotices();

    if (mode == 1) {
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
        DetectCode::ChangeFileEncodingFormat(inputData, outData, QString("UTF-8"), m_sCurEncode);
        qfile.write(outData);
        qfile.close();

        //草稿文件保存 等同于重写打开
        m_sFirstEncode = m_sCurEncode;
        QFile(m_pTextEdit->getFilePath()).remove();
        updateSaveAsFileName(m_pTextEdit->getFilePath(), newFilePath);
        m_pTextEdit->document()->setModified(false);
        m_bIsTemFile = false;
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
    if (Utils::isDraftFile(m_pTextEdit->getTruePath())) {
        return;
    }

    QFileInfo fi(m_pTextEdit->getTruePath());

    QTimer::singleShot(50, [ = ]() {
        if (fi.lastModified() == m_tModifiedDateTime || m_pWaringNotices->isVisible()) {
            return;
        }

        QFileInfo finfo(m_pTextEdit->getTruePath());

        if (!finfo.exists()) {
            m_pWaringNotices->setMessage(tr("File removed on the disk. Save it now?"));
            m_pWaringNotices->setSaveAsBtn();
            m_pWaringNotices->show();
            DMessageManager::instance()->sendMessage(m_pTextEdit, m_pWaringNotices);
        } else if (!m_tModifiedDateTime.toString().isEmpty() && finfo.lastModified().toString() != m_tModifiedDateTime.toString()) {
            m_pWaringNotices->setMessage(tr("File has changed on disk. Reload?"));
            m_pWaringNotices->setReloadBtn();
            m_pWaringNotices->show();
            DMessageManager::instance()->sendMessage(m_pTextEdit, m_pWaringNotices);
        }
    });
}

void EditWrapper::showNotify(const QString &message)
{
    if (m_pTextEdit->getReadOnlyPermission() || m_pTextEdit->getReadOnlyMode()) {
        DMessageManager::instance()->sendMessage(m_pTextEdit, QIcon(":/images/warning.svg"), message);
    } else {
        DMessageManager::instance()->sendMessage(m_pTextEdit, QIcon(":/images/ok.svg"), message);
    }
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

void EditWrapper::handleFileLoadFinished(const QByteArray &encode, const QByteArray &content)
{
    m_Definition = m_Repository.definitionForFileName(m_pTextEdit->getFilePath());
    if (m_Definition.isValid() && !m_Definition.filePath().isEmpty()) {
        if (!m_pSyntaxHighlighter) m_pSyntaxHighlighter = new CSyntaxHighlighter(m_pTextEdit->document());
        QString m_themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
        if (m_themePath.contains("dark")) {
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
        } else {
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
        }

        if (m_pSyntaxHighlighter) m_pSyntaxHighlighter->setDefinition(m_Definition);;
        m_pTextEdit->setSyntaxDefinition(m_Definition);
        m_pBottomBar->getHighlightMenu()->setCurrentTextOnly(m_Definition.translatedName());
    }

    if (!Utils::isDraftFile(m_pTextEdit->getFilePath())) {
        DRecentData data;
        data.appName = "Deepin Editor";
        data.appExec = "deepin-editor";
        DRecentManager::addItem(m_pTextEdit->getFilePath(), data);
    }

    bool flag = m_pTextEdit->getReadOnlyPermission();
    if (flag == true) {
        // note: 特殊处理，由于需要TextEdit处于可编辑状态追加文件数据，临时设置非只读状态
        m_pTextEdit->setReadOnly(false);
    }

    m_bFileLoading = true;
    m_sCurEncode = encode;
    m_sFirstEncode = encode;

    //备份显示修改状态
    if (m_bIsTemFile) {
        updateModifyStatus(true);
    }

    // 判断处理前后对象状态
    QPointer<QObject> checkPtr(this);
    // 加载数据
    loadContent(content);
    if (checkPtr.isNull()) {
        return;
    }

    //先屏蔽，双字节空字符先按照显示字符编码号处理
    //clearDoubleCharaterEncode();
    //PerformanceMonitor::openFileFinish(filePath(), QFileInfo(filePath()).size());

    m_bFileLoading = false;
    if (flag == true) {
        m_pTextEdit->setReadOnly(true);
    }
    if (m_bQuit) {
        return;
    }
    m_pTextEdit->setTextFinished();

    QStringList temFileList = Settings::instance()->settings->option("advance.editor.browsing_history_temfile")->value().toStringList();

    for (int var = 0; var < temFileList.count(); ++var) {
        QJsonParseError jsonError;
        // 转化为 JSON 文档
        QJsonDocument doucment = QJsonDocument::fromJson(temFileList.value(var).toUtf8(), &jsonError);
        // 解析未发生错误
        if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
            if (doucment.isObject()) {
                // JSON 文档为对象
                QJsonObject object = doucment.object();  // 转化为对象

                if (object.contains("localPath") || object.contains("temFilePath")) {
                    // 包含指定的 key
                    QJsonValue localPathValue = object.value("localPath");  // 获取指定 key 对应的 value
                    QJsonValue temFilePathValue = object.value("temFilePath");  // 获取指定 key 对应的 value

                    if (localPathValue.toString() == m_pTextEdit->getFilePath()) {
                        QJsonValue value = object.value("cursorPosition");  // 获取指定 key 对应的 value

                        if (value.isString()) {
                            QTextCursor cursor = m_pTextEdit->textCursor();
                            cursor.setPosition(value.toString().toInt());
                            m_pTextEdit->setTextCursor(cursor);
                            OnUpdateHighlighter();
                            break;
                        }
                    } else if (temFilePathValue.toString() == m_pTextEdit->getFilePath()) {
                        QJsonValue value = object.value("cursorPosition");  // 获取指定 key 对应的 value

                        if (value.isString()) {
                            QTextCursor cursor = m_pTextEdit->textCursor();
                            cursor.setPosition(value.toString().toInt());
                            m_pTextEdit->setTextCursor(cursor);
                            OnUpdateHighlighter();
                            break;
                        }
                    }
                }
            }
        }
    }
    //备份显示修改状态
    if (m_bIsTemFile) {
        updateModifyStatus(true);
    }

    if (m_pSyntaxHighlighter) {
        m_pSyntaxHighlighter->setEnableHighlight(true);
        OnUpdateHighlighter();
    }

    m_pBottomBar->setEncodeName(m_sCurEncode);
}


void EditWrapper::OnThemeChangeSlot(QString theme)
{
    QVariantMap jsonMap = Utils::getThemeMapFromPath(theme);
    QString backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
    QString textColor = jsonMap["Normal"].toMap()["text-color"].toString();

    //设置底部栏
    QPalette palette = m_pBottomBar->palette();
    palette.setColor(QPalette::Background, backgroundColor);
    palette.setColor(QPalette::Text, textColor);
    m_pBottomBar->setPalette(palette);

    //设置编辑器
    if (m_pSyntaxHighlighter) {
        if (QColor(backgroundColor).lightness() < 128) {
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
        } else {
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
        }
        m_pSyntaxHighlighter->rehighlight();
    }

    m_pTextEdit->setTheme(theme);
}

void EditWrapper::UpdateBottomBarWordCnt(int cnt)
{
    m_pBottomBar->updateWordCount(cnt);
}

void EditWrapper::OnUpdateHighlighter()
{
    if (m_pSyntaxHighlighter  && !m_bQuit && !m_bHighlighterAll) {
        QScrollBar *pScrollBar = m_pTextEdit->verticalScrollBar();
        QPoint startPoint = QPointF(0, 0).toPoint();
        QTextBlock beginBlock = m_pTextEdit->cursorForPosition(startPoint).block();
        QTextBlock endBlock;

        if (pScrollBar->maximum() > 0) {
            QPoint endPoint = QPointF(0, 1.5 * m_pTextEdit->height()).toPoint();
            endBlock = m_pTextEdit->cursorForPosition(endPoint).block();
        } else {
            endBlock = m_pTextEdit->document()->lastBlock();
        }

        if (!beginBlock.isValid() || !endBlock.isValid()) {
            return;
        }

        for (QTextBlock var = beginBlock; var != endBlock; var = var.next()) {
            m_pSyntaxHighlighter->setEnableHighlight(true);
            m_pSyntaxHighlighter->rehighlightBlock(var);
            m_pSyntaxHighlighter->setEnableHighlight(false);
        }

        m_pSyntaxHighlighter->setEnableHighlight(true);
        m_pSyntaxHighlighter->rehighlightBlock(endBlock);
        m_pSyntaxHighlighter->setEnableHighlight(false);
    }
}

void EditWrapper::setTemFile(bool value)
{
    m_bIsTemFile = value;
}

void EditWrapper::updateHighlighterAll()
{
    if (m_pSyntaxHighlighter  && !m_bQuit && !m_bHighlighterAll) {
        QTextBlock beginBlock = m_pTextEdit->document()->firstBlock();
        QTextBlock endBlock = m_pTextEdit->document()->lastBlock();

        if (!beginBlock.isValid() || !endBlock.isValid()) {
            return;
        }

        for (QTextBlock var = beginBlock; var != endBlock; var = var.next()) {
            m_pSyntaxHighlighter->setEnableHighlight(true);
            m_pSyntaxHighlighter->rehighlightBlock(var);
            m_pSyntaxHighlighter->setEnableHighlight(false);
        }

        m_pSyntaxHighlighter->setEnableHighlight(true);
        m_pSyntaxHighlighter->rehighlightBlock(endBlock);
        m_pSyntaxHighlighter->setEnableHighlight(false);

        m_bHighlighterAll = true;
    }
}

QDateTime EditWrapper::getLastModifiedTime() const
{
    return m_tModifiedDateTime;
}

void EditWrapper::setLastModifiedTime(const QString &time)
{
    m_tModifiedDateTime = QDateTime::fromString(time);
}
void EditWrapper::updateModifyStatus(bool bModified)
{
    if (getFileLoading()) return;
    if (!bModified)
        m_pTextEdit->updateSaveIndex();
    m_pTextEdit->document()->setModified(bModified);
    Window *pWindow = static_cast<Window *>(QWidget::window());
    pWindow->updateModifyStatus(m_pTextEdit->getFilePath(), bModified);
}

void EditWrapper::updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath)
{
    m_pWindow->updateSaveAsFileName(strOldFilePath, strNewFilePath);
}

//yanyuhan
void EditWrapper::setLineNumberShow(bool bIsShow, bool bIsFirstShow)
{
    if (bIsShow && !bIsFirstShow) {
        //int lineNumberAreaWidth = m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->width();
        //int leftAreaWidth = m_pTextEdit->getLeftAreaWidget()->width();
        m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->show();
        //m_pTextEdit->getLeftAreaWidget()->setFixedWidth(leftAreaWidth + lineNumberAreaWidth);

    } else if (!bIsShow) {
        // int lineNumberAreaWidth = m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->width();
        //int leftAreaWidth = m_pTextEdit->getLeftAreaWidget()->width();
        m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->hide();
        //m_pTextEdit->getLeftAreaWidget()->setFixedWidth(leftAreaWidth - lineNumberAreaWidth);
    }
    m_pTextEdit->bIsSetLineNumberWidth = bIsShow;
    m_pTextEdit->updateLeftAreaWidget();
}

//显示空白符
void EditWrapper::setShowBlankCharacter(bool ok)
{
    if (ok) {
        QTextOption opts = m_pTextEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag |= QTextOption::ShowTabsAndSpaces;
        // flag |= QTextOption::ShowLineAndParagraphSeparators;
        opts.setFlags(flag);
        m_pTextEdit->document()->setDefaultTextOption(opts);
    } else {
        QTextOption opts = m_pTextEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag &= ~QTextOption::ShowTabsAndSpaces;
        // flag &= ~QTextOption::ShowLineAndParagraphSeparators;
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
    return  m_pTextEdit->getFilePath();
}

TextEdit *EditWrapper::textEditor()
{
    return m_pTextEdit;
}

Window *EditWrapper::window()
{
    Window *window = static_cast<Window *>(QWidget::window());

    if (m_pWindow != window) {
        m_pWindow = window;
    }

    return m_pWindow;
}

void EditWrapper::customEvent(QEvent *e)
{
    // 处理解析文件任务，大文件不会在单次事件任务中处理，而是每次读取一部分并将下次任务抛出
    if (static_cast<QEvent::Type>(ParseFileEvent::EParseFile) == e->type()) {
        // 中途退出则不继续处理
        if (m_bQuit) {
            return;
        }

        ParseFileEvent *parseEvent = static_cast<ParseFileEvent *>(e);
        int needReadLen = parseEvent->m_contentData.length() - parseEvent->m_alreadyReadOffset;

        // 调整最大读取长度(单次读取最大长度)
        if (needReadLen > EReadStepSize) {
            needReadLen = EReadStepSize;
        }

        // 转码数据并插入光标位置
        QByteArray text = parseEvent->m_contentData.mid(parseEvent->m_alreadyReadOffset, needReadLen);
        QTextCodec::ConverterState state;
        QString data = parseEvent->m_codec->toUnicode(text.constData(), text.size(), &state);
        parseEvent->m_cursor.insertText(data);

        // 当前为首次读取
        if (0 == parseEvent->m_alreadyReadOffset) {
            QTextCursor firstLineCursor = m_pTextEdit->textCursor();
            firstLineCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
            m_pTextEdit->setTextCursor(firstLineCursor);
            //秒开界面语法高亮
            OnUpdateHighlighter();
        }

        // 此次读取后的偏移量
        int curReadOffset = parseEvent->m_alreadyReadOffset + needReadLen;
        // 设置当前的进度条
        float progress = (1.0f * curReadOffset) / parseEvent->m_contentData.length() * 100;
        m_pBottomBar->setProgress(static_cast<int>(progress));

        // 是否已读取完成
        if (parseEvent->m_contentData.length() == curReadOffset) {
            // 异步读取结束
            m_bAsyncReadFileFinished = true;
        } else {
            ParseFileEvent *nextEvent = parseEvent->clone();
            // 调整已读取偏移位置
            nextEvent->m_alreadyReadOffset = curReadOffset;
            // 抛出下一次处理的事件，根据当前是否显示界面调整优先级
            qApp->postEvent(this, nextEvent, isVisible() ? Qt::NormalEventPriority : (Qt::LowEventPriority - 1));
        }
    }
}

//支持大文本加载 界面不卡顿 秒关闭
void EditWrapper::loadContent(const QByteArray &strContent)
{
    if (m_pBottomBar != nullptr) {
        m_pBottomBar->setChildEnabled(false);
    }
    if (m_pWindow != nullptr) {
        m_pWindow->setPrintEnabled(false);
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_pTextEdit->clear();
    m_pTextEdit->setReadOnly(true);
    m_bQuit = false;
    //QTextDocument *doc = m_pTextEdit->document();
    QTextCursor cursor = m_pTextEdit->textCursor();

    /* 如此转换后打开1G以上的文本会闪退，Qt QString类的缺陷 */
    //QString strContent = content.data();


    QTextCodec::ConverterState state;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    if (nullptr == codec) {
        qInfo() << "QTextCodec::codecForName return nullptr";
        return;
    }

    int len = strContent.length();
    //初始化显示文本大小
    int InitContentPos = 5 * 1024;
    int max = 40 * 1024 * 1024;

    QString data;
    int inserted = 0;

    if (len > max) {
        // 当读取大文件时，采用事件队列方式处理
        ParseFileEvent *parseEvent = new ParseFileEvent;
        parseEvent->m_contentData = strContent;
        parseEvent->m_cursor = cursor;
        parseEvent->m_codec = codec;

        // 将处理事件追加到事件队列
        qApp->postEvent(this, parseEvent, Qt::HighEventPriority);

        // 使用QPointer判断对象状态
        QPointer<QObject> checkPtr(this);

        // 程序未退出且读取未完成的情况，持续处理事件
        m_bAsyncReadFileFinished = false;
        while (!checkPtr.isNull() && !m_bQuit && !m_bAsyncReadFileFinished) {
            QApplication::processEvents();
        }

        // 判断当前对象是否已析构，使用processEvents()可能导致在处理事件时，当前编辑控件关闭，
        // 且在事件循环中进行了析构，导致processEvents()退出时，this对象已析构，无法访问对象成员
        if (checkPtr.isNull()) {
            QApplication::restoreOverrideCursor();
            return;
        }
    } else if (len > 0) {
        //初始化秒开
        if (!m_bQuit && len > InitContentPos) {
            //data = strContent.mid(0, InitContentPos);
            QByteArray text = strContent.mid(0, InitContentPos);
            data = codec->toUnicode(text.constData(), text.size(), &state);
            cursor.insertText(data);
            QTextCursor firstLineCursor = m_pTextEdit->textCursor();
            firstLineCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
            m_pTextEdit->setTextCursor(firstLineCursor);
            //秒开界面语法高亮
            OnUpdateHighlighter();
            QApplication::processEvents();
            inserted += InitContentPos;
            float progress = (inserted * 1.0) / len * 100;
            m_pBottomBar->setProgress(progress);
            if (!m_bQuit) {
                //data = strContent.mid(InitContentPos, len - InitContentPos);
                QByteArray text = strContent.mid(InitContentPos, len - InitContentPos);
                data = codec->toUnicode(text.constData(), text.size(), &state);
                cursor.insertText(data);
                inserted += (len - InitContentPos);
                float progress = (inserted * 1.0) / len * 100;
                m_pBottomBar->setProgress(progress);
            }
        } else {
            if (!m_bQuit) {
                //cursor.insertText(strContent);
                data = codec->toUnicode(strContent.constData(), strContent.size(), &state);
                cursor.insertText(data);
                QTextCursor firstLineCursor = m_pTextEdit->textCursor();
                firstLineCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
                m_pTextEdit->setTextCursor(firstLineCursor);
                //秒开界面语法高亮
                OnUpdateHighlighter();
                inserted += len;
                float progress = (inserted * 1.0) / len * 100;
                m_pBottomBar->setProgress(progress);
            }
        }
    }
    if (m_pWindow != nullptr) {
        m_pWindow->setPrintEnabled(true);
    }
    if (m_pBottomBar != nullptr) {
        m_pBottomBar->setChildEnabled(true);
        auto format = BottomBar::getEndlineFormat(strContent);
        m_pBottomBar->setEndlineMenuText(format);
    }
    m_pTextEdit->setReadOnly(false);

    QApplication::restoreOverrideCursor();


}

void EditWrapper::clearDoubleCharaterEncode()
{
    if (QFileInfo(filePath()).baseName().contains("double")
            || QFileInfo(filePath()).baseName().contains("user")
            || QFileInfo(filePath()).baseName().contains("four")) {
        if (QFileInfo(filePath()).size() > 500 * 1024) {
            return;
        }
        emit sigClearDoubleCharaterEncode();
    }
}


