// SPDX-FileCopyrightText: 2017 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "editwrapper.h"
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
#include <QFileInfo>
#include <QEvent>

#include <DSettings>
#include <DSettingsOption>
#include <DMenuBar>

#include "../common/fileloadthread.h"
#include "../common/utils.h"
#include "../common/settings.h"
#include "../common/text_file_saver.h"
#include "../encodes/detectcode.h"
#include "../widgets/window.h"
#include "../widgets/pathsettintwgt.h"

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
    qDebug() << "ParseFileEvent constructor";
}

ParseFileEvent::~ParseFileEvent()
{
    qDebug() << "ParseFileEvent destructor";
}

ParseFileEvent *ParseFileEvent::clone()
{
    qDebug() << "ParseFileEvent clone";
    // 创建克隆对象，复制数据(浅拷贝)
    ParseFileEvent *cloneEvent = new ParseFileEvent;
    cloneEvent->m_contentData = this->m_contentData;
    cloneEvent->m_alreadyReadOffset = this->m_alreadyReadOffset;
    cloneEvent->m_cursor = this->m_cursor;
    cloneEvent->m_codec = this->m_codec;
    qDebug() << "ParseFileEvent clone exit";
    return cloneEvent;
}


EditWrapper::EditWrapper(Window *window, QWidget *parent)
    : QWidget(parent),
      m_pWindow(window),
      m_pTextEdit(new TextEdit(this)),
      m_pBottomBar(new BottomBar(this)),
      m_pWaringNotices(new WarningNotices(WarningNotices::ResidentType, this))

{
    qDebug() << "EditWrapper constructor";
    // 更新单独添加的高亮格式文件
    m_Repository.addCustomSearchPath(KF5_HIGHLIGHT_PATH);

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
    // NOTE: 文本高亮会触发重新布局，与界面布局(拖拽、放大窗口)变更时的布局操作冲突，因此调整更新顺序，在布局后刷新高亮
    connect(m_pTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        qDebug() << "EditWrapper connect verticalScrollBar valueChanged";
        OnUpdateHighlighter();
        if ((m_pWindow->findBarIsVisiable() || m_pWindow->replaceBarIsVisiable()) &&
                (QString::compare(m_pWindow->getKeywordForSearchAll(), m_pWindow->getKeywordForSearch(), Qt::CaseInsensitive) == 0)) {
            qDebug() << "EditWrapper connect verticalScrollBar valueChanged, highlightKeywordInView";
            m_pTextEdit->highlightKeywordInView(m_pWindow->getKeywordForSearchAll());
        }

        m_pTextEdit->markAllKeywordInView();
    }, Qt::QueuedConnection);
    qDebug() << "EditWrapper constructor exit";
}

EditWrapper::~EditWrapper()
{
    qDebug() << "EditWrapper destructor";
    if (m_pTextEdit != nullptr) {
        qDebug() << "EditWrapper destructor, m_pTextEdit not nullptr";
        disconnect(m_pTextEdit);
        delete m_pTextEdit;
        m_pTextEdit = nullptr;
    }
    if (m_pBottomBar != nullptr) {
        qDebug() << "EditWrapper destructor, m_pBottomBar not nullptr";
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
    qDebug() << "EditWrapper destructor exit";
}

void EditWrapper::setQuitFlag()
{
    qDebug() << "EditWrapper setQuitFlag";
    m_bQuit = true;
    qDebug() << "EditWrapper setQuitFlag exit";
}

bool EditWrapper::isQuit()
{
    qDebug() << "EditWrapper isQuit";
    return m_bQuit;
}

bool EditWrapper::getFileLoading()
{
    qDebug() << "EditWrapper getFileLoading";
    return (m_bQuit || m_bFileLoading);
}

void EditWrapper::openFile(const QString &filepath, QString qstrTruePath, bool bIsTemFile)
{
    qDebug() << "Opening file:" << filepath << "Real path:" << qstrTruePath << "Is temp:" << bIsTemFile;
    
    m_bIsTemFile = bIsTemFile;
    // update file path.
    updatePath(filepath, qstrTruePath);
    m_pTextEdit->setIsFileOpen();
    // 设置预处理标识位
    m_bHasPreProcess = false;

    if (!bIsTemFile && !isDraftFile()) {
        QString absPath = QFileInfo(qstrTruePath).absolutePath();
        qDebug() << "EditWrapper openFile, Setting save paths to:" << absPath;
        Settings::instance()->setSavePath(PathSettingWgt::LastOptBox, absPath);
        Settings::instance()->setSavePath(PathSettingWgt::CurFileBox, absPath);
    }

    FileLoadThread *thread = new FileLoadThread(filepath);
    qDebug() << "Created file load thread for:" << filepath;
    // begin to load the file.
    connect(thread, &FileLoadThread::sigPreProcess, this, &EditWrapper::handleFilePreProcess);
    connect(thread, &FileLoadThread::sigLoadFinished, this, &EditWrapper::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);
    thread->start();
}

/**
 * @brief 按文件编码 \a encode 读取当前文件数据，若文件存在异常，则返回读取失败。
 *      \a encode 可缺省，按文件默认编码读取。
 * @param encode 文件编码
 * @return 是否成功读取文件数据
 */
bool EditWrapper::readFile(QByteArray encode)
{
    qDebug() << "Reading file with encoding:" << encode;
    QString filePath = m_pTextEdit->getTruePath();
    qDebug() << "Reading file:" << filePath << "with encode:" << encode;
    
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        qDebug() << "File opened successfully";
        QByteArray fileContent = file.readAll();
        QByteArray newEncode = encode;
        if (newEncode.isEmpty()) {
            qDebug() << "Auto detecting file encoding";
            // 接口修改，补充文件头内容，最多读取1MB文件头数据
            newEncode = DetectCode::GetFileEncodingFormat(
                            m_pTextEdit->getFilePath(), fileContent.left(DATA_SIZE_1024 * DATA_SIZE_1024));
            qDebug() << "Detected file encoding:" << newEncode;
            m_sFirstEncode = newEncode;
        }

        QByteArray Outdata;
        bool encodeSuccess = DetectCode::ChangeFileEncodingFormat(fileContent, Outdata, newEncode, QString("UTF-8"));
        if (!encodeSuccess) {
            qWarning() << "EditWrapper readFile, Failed to convert file encoding";
            return false;
        }
        qDebug() << "File encoding converted successfully";
        loadContent(Outdata);
        file.close();
        m_sCurEncode = newEncode;
        updateModifyStatus(false);
        qDebug() << "EditWrapper readFile exit, return true";
        return true;
    }
    qDebug() << "EditWrapper readFile exit, return false";
    return false;
}

/**
 * @brief 将当前文件内容按编码格式 \a encodeName 保存到指定路径 \a newFilePath
 * @param newFilePath 另存的文件路径
 * @param encodeName 文件保存时指定的文件格式
 * @return 是否成功另存文件
 */
bool EditWrapper::saveAsFile(const QString &newFilePath, const QByteArray &encodeName)
{
    qDebug() << "Attempting to save file to:" << newFilePath << "with encoding:" << encodeName;
    
    // Use TextFileSaver for safe file saving
    TextFileSaver saver(m_pTextEdit->document());
    saver.setFilePath(newFilePath);
    saver.setEncoding(encodeName);
    
    bool saveSuccess = saver.save();
    if (!saveSuccess) {
        qWarning() << "Failed to save file:" << newFilePath << "Error:" << saver.errorString();
        QWidget *curWidget = this->window()->getStackedWgt()->currentWidget();
        if (curWidget) {
            DMessageManager::instance()->sendMessage(
                curWidget,
                QIcon(":/images/warning.svg"),
                QString(tr("You do not have permission to save %1")).arg(QFileInfo(newFilePath).fileName())
            );
        }
        qDebug() << "EditWrapper saveAsFile, Failed to save file:" << newFilePath << "Error:" << saver.errorString();
        return false;
    }

    QFileInfo fi(filePath());
    m_tModifiedDateTime = fi.lastModified();

    qDebug() << "EditWrapper saveAsFile, save file success, return true";
    return true;
}

bool EditWrapper::saveAsFile()
{
    qDebug() << "EditWrapper saveAsFile";
    // Create save file dialog
    DFileDialog dialog(this, tr("Save"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(QObject::tr("Encoding"), QStringList() << m_sFirstEncode);
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("*.txt");

    // Execute dialog and check result
    int mode = dialog.exec();
    hideWarningNotices();

    if (QDialog::Accepted == mode) {
        const QString newFilePath = dialog.selectedFiles().value(0);
        if (newFilePath.isEmpty()) {
            qDebug() << "EditWrapper saveAsFile, newFilePath is empty, return false";
            return false;
        }

        // Use TextFileSaver for safe file saving
        TextFileSaver saver(m_pTextEdit->document());
        saver.setFilePath(newFilePath);
        saver.setEncoding(m_sFirstEncode.toUtf8());
        
        if (!saver.save()) {
            qWarning() << "EditWrapper saveAsFile, Failed to save file:" << saver.errorString();
            return false;
        }
        qDebug() << "EditWrapper saveAsFile, save file success, return true";
        return true;
    }

    qDebug() << "EditWrapper saveAsFile, save file failed, return false";
    return false;
}

/**
 * @brief 按文件编码 \a encode 重新加载文件，并返回是否加载成功。文件有变更会提示是否保存。
 * @param encode 文件编码
 * @return 返回是否加载成功
 */
bool EditWrapper::reloadFileEncode(QByteArray encode)
{
    qDebug() << "Reloading file with new encoding:" << encode.size();
    
    //切换编码相同不重写加载
    if (m_sCurEncode == encode) {
        qDebug() << "Encoding unchanged, skipping reload";
        return false;
    }

    //草稿文件 空白文件不保存
    if (Utils::isDraftFile(m_pTextEdit->getFilePath()) &&  m_pTextEdit->toPlainText().isEmpty()) {
        qDebug() << "EditWrapper reloadFileEncode, isDraftFile and toPlainText is empty, return true";
        m_sCurEncode = encode;
        m_sFirstEncode = encode;
        return true;
    }

    //temporarily use '*' to determine whether the text has been modified.
    auto tabname = this->window()->getTabbar()->currentName();
    bool hasFlag = false;
    if (tabname.size() > 0 && "*" == tabname[0]) {
        qDebug() << "EditWrapper reloadFileEncode, tabname is not empty and tabname[0] is *, hasFlag is true";
        hasFlag = true;
    }

    //1.如果修改切换编码提示用户是否保存,不保存重新打开文件读取.2.没有修改是否另存为
    if (m_pTextEdit->getModified() || hasFlag) {
        qDebug() << "EditWrapper reloadFileEncode, m_pTextEdit->getModified() or hasFlag is true";
        DDialog *dialog = new DDialog(tr("Encoding changed. Do you want to save the file now?"), "", this);
        //dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnBottomHint);
        dialog->setIcon(QIcon::fromTheme("deepin-editor"));
        dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);   //取消
        //dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);//不保存
        dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);   //保存
        int res = dialog->exec();//0  1

        //关闭对话框
        if (res == 0) {
            qDebug() << "EditWrapper reloadFileEncode, res is 0, return false";
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
            qDebug() << "EditWrapper reloadFileEncode, res is 1, save file";
            // 保存文件前缓存当前文件的编码格式，文件将按之前的编码格式保存
            QString tempEncode = m_sCurEncode;

            //草稿文件
            bool reloadSucc = false;
            if (Utils::isDraftFile(m_pTextEdit->getFilePath())) {
                qDebug() << "EditWrapper reloadFileEncode, Utils::isDraftFile(m_pTextEdit->getFilePath())";
                QString newFilePath;
                if (saveDraftFile(newFilePath)) {
                    qDebug() << "EditWrapper reloadFileEncode, saveDraftFile(newFilePath) success";
                    reloadSucc = readFile(encode);
                }
            } else {
                qDebug() << "EditWrapper reloadFileEncode, !Utils::isDraftFile(m_pTextEdit->getFilePath())";
                if (this->window()->saveAsFile()) {
                    qDebug() << "EditWrapper reloadFileEncode, this->window()->saveAsFile() success";
                    reloadSucc = readFile(encode);
                }
            }

            if (reloadSucc) {
                qDebug() << "EditWrapper reloadFileEncode, reloadSucc is true";
                // 保存成功，更新编码格式
                m_pBottomBar->setEncodeName(encode);
            } else {
                qDebug() << "EditWrapper reloadFileEncode, reloadSucc is false";
                // 未成功保存，复位编码格式
                m_sCurEncode = tempEncode;
            }
            qDebug() << "EditWrapper reloadFileEncode, reloadSucc:" << reloadSucc;
            return reloadSucc;
        }

        qDebug() << "EditWrapper reloadFileEncode, return false";
        return false;
    } else {
        qDebug() << "EditWrapper reloadFileEncode, readFile(encode)";
        return readFile(encode);
    }
}

/**
 * @brief 根据高亮格式文件的名称 \a definitionName 查询高亮格式配置，并格式化文档。
 *      若未查找到格式，例如“None”，则移除高亮效果。
 * @param definitionName 高亮格式文件名称
 */
void EditWrapper::reloadFileHighlight(QString definitionName)
{
    qDebug() << "Reloading syntax highlight with definition:" << definitionName;
    m_Definition = m_Repository.definitionForName(definitionName);
    if (m_Definition.isValid() && !m_Definition.filePath().isEmpty()) {
        qDebug() << "EditWrapper reloadFileHighlight, m_Definition.isValid() && !m_Definition.filePath().isEmpty()";
        if (!m_pSyntaxHighlighter) m_pSyntaxHighlighter = new CSyntaxHighlighter(m_pTextEdit->document());
        QString m_themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
        qDebug() << "EditWrapper reloadFileHighlight, m_themePath:" << m_themePath;
        if (m_themePath.contains("dark")) {
            qDebug() << "EditWrapper reloadFileHighlight, m_themePath contains dark";
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
        } else {
            qDebug() << "EditWrapper reloadFileHighlight, m_themePath not contains dark";
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
        }
        if (m_pSyntaxHighlighter) m_pSyntaxHighlighter->setDefinition(m_Definition);
        m_pTextEdit->setSyntaxDefinition(m_Definition);

        // 获取当前展示区域文本块
        QPoint startPoint = QPoint(0, 0);
        QTextBlock beginBlock = m_pTextEdit->cursorForPosition(startPoint).block();
        QTextBlock endBlock;

        QScrollBar *pScrollBar = m_pTextEdit->verticalScrollBar();
        if (pScrollBar->maximum() > 0) {
            qDebug() << "EditWrapper reloadFileHighlight, pScrollBar->maximum() > 0";
            QPoint endPoint = QPointF(0, 1.5 * m_pTextEdit->height()).toPoint();
            endBlock = m_pTextEdit->cursorForPosition(endPoint).block();
        } else {
            qDebug() << "EditWrapper reloadFileHighlight, pScrollBar->maximum() <= 0";
            endBlock = m_pTextEdit->document()->lastBlock();
        }

        // 移除当前页面旧的高亮内容
        QTextCursor cursor(beginBlock);
        cursor.beginEditBlock();
        for (QTextBlock var = beginBlock; var.isValid(); var = var.next()) {
            var.layout()->clearFormats();
            if (var == endBlock) {
                qDebug() << "EditWrapper reloadFileHighlight, var == endBlock, break!";
                break;
            }
        }
        cursor.endEditBlock();
        // 复位标识位
        m_bHighlighterAll = false;

        // 重新高亮当前界面
        OnUpdateHighlighter();
    } else {
        qDebug() << "EditWrapper reloadFileHighlight, m_Definition.isValid() && !m_Definition.filePath().isEmpty() is false";
        // 不允许的高亮格式或无对应的高亮格式文件，例如“None”，移除高亮效果
        m_bHighlighterAll = false;
        m_Definition = KSyntaxHighlighting::Definition();
        if (m_pSyntaxHighlighter) {
            qDebug() << "EditWrapper reloadFileHighlight, m_pSyntaxHighlighter is not nullptr";
            m_pSyntaxHighlighter->deleteLater();
            m_pSyntaxHighlighter = nullptr;
        }
        qDebug() << "EditWrapper reloadFileHighlight, m_pTextEdit->setSyntaxDefinition(m_Definition)";
        m_pTextEdit->setSyntaxDefinition(m_Definition);
    }
    qDebug() << "EditWrapper reloadFileHighlight, exit";
}

void EditWrapper::reloadModifyFile()
{
    qDebug() << "EditWrapper reloadModifyFile";
    hideWarningNotices();

    int curPos = m_pTextEdit->textCursor().position();
    int yoffset = m_pTextEdit->verticalScrollBar()->value();
    int xoffset = m_pTextEdit->horizontalScrollBar()->value();

    bool bIsModified = m_pTextEdit->getModified();
    qDebug() << "EditWrapper reloadModifyFile, bIsModified:" << bIsModified;
    if (m_pWindow->getTabbar()->textAt(m_pWindow->getTabbar()->currentIndex()).front() == "*") {
        qDebug() << "EditWrapper reloadModifyFile, m_pWindow->getTabbar()->textAt(m_pWindow->getTabbar()->currentIndex()).front() == *";
        bIsModified = true;
    }
    //如果文件修改提示用户是否保存  如果临时文件保存就是另存为
    if (bIsModified) {
        qDebug() << "EditWrapper reloadModifyFile, bIsModified is true";
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
            qDebug() << "EditWrapper reloadModifyFile, res is 0, return";
            return;
        }

        //不保存
        if (res == 1) {
            qDebug() << "EditWrapper reloadModifyFile, res is 1, readFile";
            //重写加载文件
            readFile();
            m_bIsTemFile = false;
        }
        //另存
        if (res == 2) {
            qDebug() << "EditWrapper reloadModifyFile, res is 2, saveAsFile";
            //临时文件保存另存为 需要删除源草稿文件文件
            if (Utils::isDraftFile(m_pTextEdit->getFilePath())) {
                QString newFilePath;
                if (!saveDraftFile(newFilePath)) {
                    qDebug() << "EditWrapper reloadModifyFile, saveDraftFile(newFilePath) failed, return";
                    return;
                }
            } else {
                if (!this->window()->saveAsFile()) {
                    qDebug() << "EditWrapper reloadModifyFile, this->window()->saveAsFile() failed, return";
                    return;
                }
            }
            //重写加载文件
            readFile();
            m_bIsTemFile = false;
        }

    } else {
        qDebug() << "EditWrapper reloadModifyFile, bIsModified is false, readFile";
        //重写加载文件
        readFile();
    }
    qDebug() << "EditWrapper reloadModifyFile, setBookMarkList";
    m_pTextEdit->setBookMarkList(QList<int>());
    QFileInfo fi(m_pTextEdit->getTruePath());
    m_tModifiedDateTime = fi.lastModified();

    QTextCursor textcur = m_pTextEdit->textCursor();
    textcur.setPosition(curPos);
    m_pTextEdit->setTextCursor(textcur);
    m_pTextEdit->verticalScrollBar()->setValue(yoffset);
    m_pTextEdit->horizontalScrollBar()->setValue(xoffset);
    qDebug() << "EditWrapper reloadModifyFile, exit";
}

QString EditWrapper::getTextEncode()
{
    qDebug() << "EditWrapper getTextEncode, m_sCurEncode:" << m_sCurEncode;
    return m_sCurEncode;
}

/**
 * @brief 保存当前文件数据，若设置了特定的编码格式 \a encode , 则按此编码格式存储，
 *      否则使用当前显示的文本编码。
 * @param encode 文件编码
 * @return 是否成功保存文件数据
 */
bool EditWrapper::saveFile(QByteArray encode)
{
    qDebug() << "EditWrapper saveFile, encode:" << encode;
    QString qstrFilePath = m_pTextEdit->getTruePath();
    hideWarningNotices();

    // 更新编码设置
    if (!encode.isEmpty()) {
        qDebug() << "EditWrapper saveFile, encode is not empty";
        m_sCurEncode = encode;
        m_pBottomBar->setEncodeName(encode);
    }

    // 使用TextFileSaver进行文件保存
    TextFileSaver saver(m_pTextEdit->document());
    saver.setFilePath(qstrFilePath);
    saver.setEncoding(m_sCurEncode.toUtf8());
    
    bool ok = saver.save();
    if (ok) {
        qDebug() << "EditWrapper saveFile, ok is true";
        m_sFirstEncode = m_sCurEncode;
        QFileInfo fi(qstrFilePath);
        m_tModifiedDateTime = fi.lastModified();
        updateModifyStatus(false);
        m_bIsTemFile = false;
    } else {
        qDebug() << "EditWrapper saveFile, ok is false";
        DMessageManager::instance()->sendMessage(
            this->window()->getStackedWgt()->currentWidget(),
            QIcon(":/images/warning.svg"),
            QString(tr("You do not have permission to save %1")).arg(QFileInfo(qstrFilePath).fileName())
        );
    }
    qDebug() << "EditWrapper saveFile, ok:" << ok;
    return ok;
}

void EditWrapper::getPlainTextContent(QByteArray &plainTextContent)
{
    qDebug() << "EditWrapper getPlainTextContent";
    QString strPlainText = m_pTextEdit->toPlainText();
    qDebug() << "EditWrapper getPlainTextContent, strPlainText:" << strPlainText;
    if (BottomBar::EndlineFormat::Windows == m_pBottomBar->getEndlineFormat()) {
        qDebug() << "EditWrapper getPlainTextContent, BottomBar::EndlineFormat::Windows";
        strPlainText.replace("\n", "\r\n");
    }

    qint64 iPlainTextAllLen = strPlainText.length();
    qint64 iStep = 300 * DATA_SIZE_1024 * DATA_SIZE_1024;
    /* qt开发分析结论：大文本情况下toLocal8Bit()转换会引起应用闪退，此闪退问题qt方无法处理 */
    /* 如果文本框里的文本内容小于等于300MB，无需分段转换 */
    if (iPlainTextAllLen <= iStep) {
        plainTextContent = QByteArray(strPlainText.toLocal8Bit());
        qDebug() << "EditWrapper getPlainTextContent, iPlainTextAllLen <= iStep, plainTextContent:" << plainTextContent;
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
    qDebug() << "EditWrapper getPlainTextContent, exit";
}

/**
 * @brief saveTemFile 保存备份文件
 * @param qstrDir　备份文件路径
 * @return true or false
 */
bool EditWrapper::saveTemFile(QString qstrDir)
{
    qDebug() << "EditWrapper saveTemFile, qstrDir:" << qstrDir;
    // Use TextFileSaver for safe file saving
    TextFileSaver saver(m_pTextEdit->document());
    saver.setFilePath(qstrDir);
    saver.setEncoding(m_sCurEncode.toUtf8());
    
    bool ok = saver.save();
    if (ok) {
        qDebug() << "EditWrapper saveTemFile, ok is true";
        m_sFirstEncode = m_sCurEncode;
        updateModifyStatus(isModified());
    }
    qDebug() << "EditWrapper saveTemFile, ok:" << ok;
    return ok;
}

void EditWrapper::updatePath(const QString &file, QString qstrTruePath)
{
    qDebug() << "EditWrapper updatePath, file:" << file << "qstrTruePath:" << qstrTruePath;
    if (qstrTruePath.isEmpty()) {
        qDebug() << "EditWrapper updatePath, qstrTruePath is empty";
        qstrTruePath = file;
    }
    qDebug() << "EditWrapper updatePath, qstrTruePath:" << qstrTruePath;
    QFileInfo fi(qstrTruePath);
    m_tModifiedDateTime = fi.lastModified();
    qDebug() << "EditWrapper updatePath, m_tModifiedDateTime:" << m_tModifiedDateTime;
    m_pTextEdit->setFilePath(file);
    qDebug() << "EditWrapper updatePath, m_pTextEdit->setFilePath(file)";
    m_pTextEdit->setTruePath(qstrTruePath);
    qDebug() << "EditWrapper updatePath, m_pTextEdit->setTruePath(qstrTruePath)";
}

bool EditWrapper::isModified()
{
    qDebug() << "EditWrapper isModified";
    //编码改变内容没有修改也算是文件修改
    // bool modified = (m_sFirstEncode != m_sCurEncode || m_pTextEdit->document()->isModified());
    bool modified =  m_pTextEdit->getModified();
    qDebug() << "EditWrapper isModified, modified:" << modified << "m_bIsTemFile:" << m_bIsTemFile;
    return  modified | m_bIsTemFile;
}

bool EditWrapper::isDraftFile()
{
    qDebug() << "EditWrapper isDraftFile";
    return Utils::isDraftFile(m_pTextEdit->getFilePath());
}

/**
 * @return 返回当前编辑文件是否为备份文件
 */
bool EditWrapper::isBackupFile()
{
    qDebug() << "EditWrapper isBackupFile";
    return Utils::isBackupFile(m_pTextEdit->getFilePath());
}

bool EditWrapper::isPlainTextEmpty()
{
    qDebug() << "EditWrapper isPlainTextEmpty";
    return m_pTextEdit->document()->isEmpty();
}

bool EditWrapper::isTemFile()
{
    qDebug() << "EditWrapper isTemFile";
    return m_bIsTemFile;
}

/**
 * @brief 保存草稿文件，若保存成功，新文件路径通过 \a newFilePath 返回，
 *      且对应标签页已更新为新的文件信息。
 * @param newFilePath 保存的新文件路径
 * @return 是否保存成功
 */
bool EditWrapper::saveDraftFile(QString &newFilePath)
{
    qDebug() << "EditWrapper saveDraftFile, newFilePath:" << newFilePath;
    DFileDialog dialog(this, tr("Save"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("*.txt");

    // 允许选取保存的编码格式
    DFileDialog::DComboBoxOptions encodingOptions;
    encodingOptions.editable = false;
    encodingOptions.defaultValue = getTextEncode();
    encodingOptions.data = Utils::getSupportEncodingList();
    dialog.addComboBox(QObject::tr("Encoding"), encodingOptions);

    if (m_pWindow) {
        m_pWindow = this->window();
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(m_pWindow->getTabbar()->currentName());
        dialog.selectFile(match.captured(0) + ".txt");
    }

    int mode =  dialog.exec(); // 0表示取消 1保存
    hideWarningNotices();

    if (mode == 1) {
        qDebug() << "EditWrapper saveDraftFile, mode is 1";
        // Get selected encoding and file path
        const QByteArray encode = dialog.getComboBoxValue(QObject::tr("Encoding")).toUtf8();
        newFilePath = dialog.selectedFiles().value(0);
        if (newFilePath.isEmpty())
            return false;

        // Use TextFileSaver to save the file
        TextFileSaver saver(m_pTextEdit->document());
        saver.setFilePath(newFilePath);
        saver.setEncoding(encode);
        
        if (!saver.save()) {
            qDebug() << "EditWrapper saveDraftFile, saver.save() failed";
            return false;
        }

        // For draft files, remove the old file and update paths
        m_sFirstEncode = m_sCurEncode;
        QFile(m_pTextEdit->getFilePath()).remove();
        updateSaveAsFileName(m_pTextEdit->getFilePath(), newFilePath);
        m_pTextEdit->document()->setModified(false);
        m_bIsTemFile = false;
        qDebug() << "EditWrapper saveDraftFile, saver.save() success";
        return true;
    }

    qDebug() << "EditWrapper saveDraftFile, mode is not 1, return false";
    return false;
}

void EditWrapper::hideWarningNotices()
{
    qDebug() << "EditWrapper hideWarningNotices";
    if (m_pWaringNotices->isVisible()) {
        qDebug() << "EditWrapper hideWarningNotices, m_pWaringNotices->isVisible()";
        m_pWaringNotices->hide();
    }
    qDebug() << "EditWrapper hideWarningNotices, exit";
}

//除草稿文件 检查文件是否被删除,是否被修复
void EditWrapper::checkForReload()
{
    qDebug() << "EditWrapper checkForReload";
    if (Utils::isDraftFile(m_pTextEdit->getTruePath())) {
        qDebug() << "EditWrapper checkForReload, Utils::isDraftFile(m_pTextEdit->getTruePath())";
        return;
    }

    QFileInfo fi(m_pTextEdit->getTruePath());
    qDebug() << "EditWrapper checkForReload, fi:" << fi;
    QTimer::singleShot(50, [ = ]() {
        if (fi.lastModified() == m_tModifiedDateTime || m_pWaringNotices->isVisible()) {
            qDebug() << "EditWrapper checkForReload, fi.lastModified() == m_tModifiedDateTime || m_pWaringNotices->isVisible()";
            return;
        }
        qDebug() << "EditWrapper checkForReload, fi.lastModified() != m_tModifiedDateTime && !m_pWaringNotices->isVisible()";
        QFileInfo finfo(m_pTextEdit->getTruePath());
        qDebug() << "EditWrapper checkForReload, finfo:" << finfo;
        if (!finfo.exists()) {
            qDebug() << "EditWrapper checkForReload, finfo.exists() is false";
            m_pWaringNotices->setMessage(tr("File removed on the disk. Save it now?"));
            m_pWaringNotices->setSaveAsBtn();
            m_pWaringNotices->show();
            DMessageManager::instance()->sendMessage(m_pTextEdit, m_pWaringNotices);
        } else if (!m_tModifiedDateTime.toString().isEmpty() && finfo.lastModified().toString() != m_tModifiedDateTime.toString()) {
            qDebug() << "EditWrapper checkForReload, finfo.lastModified().toString() != m_tModifiedDateTime.toString()";
            m_pWaringNotices->setMessage(tr("File has changed on disk. Reload?"));
            m_pWaringNotices->setReloadBtn();
            m_pWaringNotices->show();
            DMessageManager::instance()->sendMessage(m_pTextEdit, m_pWaringNotices);
        }
    });
    qDebug() << "EditWrapper checkForReload, exit";
}

void EditWrapper::showNotify(const QString &message, bool warning)
{
    qDebug() << "EditWrapper showNotify, message:" << message << "warning:" << warning;
    if (message.isEmpty()) {
        qDebug() << "EditWrapper showNotify, message is empty, return";
        return;
    }

    if (warning || m_pTextEdit->getReadOnlyPermission() || m_pTextEdit->getReadOnlyMode()) {
        qDebug() << "EditWrapper showNotify, warning || m_pTextEdit->getReadOnlyPermission() || m_pTextEdit->getReadOnlyMode()";
#ifdef DTKWIDGET_CLASS_DSizeMode
        Utils::sendFloatMessageFixedFont(m_pTextEdit, QIcon(":/images/warning.svg"), message);
#else
        DMessageManager::instance()->sendMessage(m_pTextEdit, QIcon(":/images/warning.svg"), message);
#endif
    } else {
        qDebug() << "EditWrapper showNotify, warning is false";
#ifdef DTKWIDGET_CLASS_DSizeMode
        Utils::sendFloatMessageFixedFont(m_pTextEdit, QIcon(":/images/ok.svg"), message);
#else
        DMessageManager::instance()->sendMessage(m_pTextEdit, QIcon(":/images/ok.svg"), message);
#endif
    }
    qDebug() << "EditWrapper showNotify, exit";
}


void EditWrapper::handleCursorModeChanged(TextEdit::CursorMode mode)
{
    qDebug() << "EditWrapper handleCursorModeChanged, mode:" << mode;
    switch (mode) {
    case TextEdit::Insert:
        qDebug() << "EditWrapper handleCursorModeChanged, mode is Insert";
        m_pBottomBar->setCursorStatus(tr("INSERT"));
        break;
    case TextEdit::Overwrite:
        qDebug() << "EditWrapper handleCursorModeChanged, mode is Overwrite";
        m_pBottomBar->setCursorStatus(tr("OVERWRITE"));
        break;
    case TextEdit::Readonly:
        qDebug() << "EditWrapper handleCursorModeChanged, mode is Readonly";
        m_pBottomBar->setCursorStatus(tr("R/O"));
        break;
    }
    qDebug() << "EditWrapper handleCursorModeChanged, exit";
}

/**
 * @brief 处理文件预加载服务
 * @param encode    文件编码
 * @param content   文件内容，不超过1MB
 *
 * @note 预加载的数据量较小(1MB)，且可能分割字符，在文件完成读取后，预加载的数据将被清空并重新加载完整文档。
 */
void EditWrapper::handleFilePreProcess(const QByteArray &encode, const QByteArray &content)
{
    qDebug() << "EditWrapper handleFilePreProcess";
    // 重新加载处理
    reinitOnFileLoad(encode);
    // 已进行预处理标识
    m_bHasPreProcess = true;

    // 设置界面交互效果
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_pTextEdit->clear();
    m_pTextEdit->setLeftAreaUpdateState(TextEdit::FileOpenBegin);
    QTextCursor cursor = m_pTextEdit->textCursor();

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    if (!codec) {
        qInfo() << "QTextCodec::codecForName \"UTF-8\" return nullptr";
        return;
    }

    // 直接加载数据到文档页面
    QString data = codec->toUnicode(content.constData(), content.size());
    cursor.insertText(data);
    // 界面语法高亮
    OnUpdateHighlighter();

    m_pTextEdit->setLeftAreaUpdateState(TextEdit::FileOpenEnd);
    QApplication::restoreOverrideCursor();
    qDebug() << "EditWrapper handleFilePreProcess, exit";
}

/**
 * @brief 处理文件加载完成服务，取得加载完成的所有文件数据，重新初始化界面
 * @param encode    文件编码
 * @param content   完整文件内容
 */
void EditWrapper::handleFileLoadFinished(const QByteArray &encode, const QByteArray &content, bool error)
{
    qDebug() << "File load finished. Encoding:" << encode << "Error:" << error << "Preprocessed:" << m_bHasPreProcess;
    
    // 判断是否预加载，若已预加载，则无需重新初始化
    if (!m_bHasPreProcess) {
        qDebug() << "Performing initial file load initialization";
        reinitOnFileLoad(encode);
    }

    if (!error) {
        qDebug() << "File content loaded successfully. Size:" << content.size() << "bytes";
        bool flag = m_pTextEdit->getReadOnlyPermission();
        if (flag == true) {
            qDebug() << "EditWrapper handleFileLoadFinished, flag is true";
            // note: 特殊处理，由于需要TextEdit处于可编辑状态追加文件数据，临时设置非只读状态
            m_pTextEdit->setReadOnly(false);
        }

        m_bFileLoading = true;

        //备份显示修改状态
        if (m_bIsTemFile) {
            qDebug() << "EditWrapper handleFileLoadFinished, m_bIsTemFile is true";
            updateModifyStatus(true);
        }

        // 判断处理前后对象状态
        QPointer<QObject> checkPtr(this);
        // 加载数据
        loadContent(content);
        if (checkPtr.isNull()) {
            qDebug() << "EditWrapper handleFileLoadFinished, checkPtr is null, return";
            return;
        }

        m_bFileLoading = false;
        if (flag == true) {
            qDebug() << "EditWrapper handleFileLoadFinished, flag is true, setReadOnly(true)";
            m_pTextEdit->setReadOnly(true);
        }

        if (m_bQuit) {
            qDebug() << "EditWrapper handleFileLoadFinished, m_bQuit is true, return";
            return;
        }
    } else {
        // 清除之前读取的数据
        qDebug() << "EditWrapper handleFileLoadFinished, flag is false, clear";
        m_pTextEdit->clear();
    }

    m_pTextEdit->setTextFinished();

    QStringList temFileList = Settings::instance()->settings->option("advance.editor.browsing_history_temfile")->value().toStringList();

    for (int var = 0; var < temFileList.count(); ++var) {
        QJsonParseError jsonError;
        // 转化为 JSON 文档
        QJsonDocument doucment = QJsonDocument::fromJson(temFileList.value(var).toUtf8(), &jsonError);
        // 解析未发生错误
        if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
            qDebug() << "EditWrapper handleFileLoadFinished, doucment is not null";
            if (doucment.isObject()) {
                qDebug() << "EditWrapper handleFileLoadFinished, doucment is object";
                // JSON 文档为对象
                QJsonObject object = doucment.object();  // 转化为对象

                if (object.contains("localPath") || object.contains("temFilePath")) {
                    qDebug() << "EditWrapper handleFileLoadFinished, doucment contains localPath or temFilePath";
                    // 包含指定的 key
                    QJsonValue localPathValue = object.value("localPath");  // 获取指定 key 对应的 value
                    QJsonValue temFilePathValue = object.value("temFilePath");  // 获取指定 key 对应的 value

                    if (localPathValue.toString() == m_pTextEdit->getFilePath()
                            || temFilePathValue.toString() == m_pTextEdit->getFilePath()) {
                        qDebug() << "EditWrapper handleFileLoadFinished, localPathValue or temFilePathValue is equal to m_pTextEdit->getFilePath()";
                        QJsonValue value = object.value("cursorPosition");  // 获取指定 key 对应的 value

                        if (value.isString()) {
                            qDebug() << "EditWrapper handleFileLoadFinished, value is string";
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
        qDebug() << "EditWrapper handleFileLoadFinished, m_bIsTemFile is true, updateModifyStatus(true)";
        updateModifyStatus(true);
    }

    if (m_pSyntaxHighlighter) {
        qDebug() << "EditWrapper handleFileLoadFinished, m_pSyntaxHighlighter is not nullptr";
        m_pSyntaxHighlighter->setEnableHighlight(true);
        OnUpdateHighlighter();
    }

    m_pBottomBar->setEncodeName(m_sCurEncode);

    // 提示读取错误信息
    if (error) {
        qDebug() << "EditWrapper handleFileLoadFinished, error is true, onReadAllocError()";
        onReadAllocError();
    }
    qDebug() << "EditWrapper handleFileLoadFinished, exit";
}


void EditWrapper::OnThemeChangeSlot(QString theme)
{
    qDebug() << "EditWrapper OnThemeChangeSlot, theme:" << theme;
    QVariantMap jsonMap = Utils::getThemeMapFromPath(theme);
    QString backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
    QString textColor = jsonMap["Normal"].toMap()["text-color"].toString();

    //设置底部栏
    QPalette palette = m_pBottomBar->palette();
    palette.setColor(QPalette::Window, backgroundColor);
    palette.setColor(QPalette::Text, textColor);
    m_pBottomBar->setPalette(palette);

    //设置编辑器
    if (m_pSyntaxHighlighter) {
        qDebug() << "EditWrapper OnThemeChangeSlot, m_pSyntaxHighlighter is not nullptr";
        if (QColor(backgroundColor).lightness() < 128) {
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
            qDebug() << "EditWrapper OnThemeChangeSlot, QColor(backgroundColor).lightness() < 128, setTheme(DarkTheme)";
        } else {
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
            qDebug() << "EditWrapper OnThemeChangeSlot, QColor(backgroundColor).lightness() >= 128, setTheme(LightTheme)";
        }
        m_pSyntaxHighlighter->rehighlight();
        qDebug() << "EditWrapper OnThemeChangeSlot, m_pSyntaxHighlighter->rehighlight()";
    }

    m_pTextEdit->setTheme(theme);
    qDebug() << "EditWrapper OnThemeChangeSlot, m_pTextEdit->setTheme(theme)";
}

void EditWrapper::UpdateBottomBarWordCnt(int cnt)
{
    qDebug() << "EditWrapper UpdateBottomBarWordCnt, cnt:" << cnt;
    m_pBottomBar->updateWordCount(cnt);
}

/**
 * @brief 界面显示内容变更时触发，将查询当前显示的内容
 * @param forceUpdate 是否强制重设高亮处理，部分高亮无需重复设置
 */
void EditWrapper::OnUpdateHighlighter()
{
    qDebug() << "EditWrapper OnUpdateHighlighter";
    if (m_pSyntaxHighlighter  && !m_bQuit && !m_bHighlighterAll) {
        qDebug() << "EditWrapper OnUpdateHighlighter, m_pSyntaxHighlighter is not nullptr";
        QScrollBar *pScrollBar = m_pTextEdit->verticalScrollBar();
        QPoint startPoint = QPoint(0, 0);
        QTextBlock beginBlock = m_pTextEdit->cursorForPosition(startPoint).block();
        QTextBlock endBlock;
        QTextBlock foundBlock;

        if (pScrollBar->maximum() > 0) {
            qDebug() << "EditWrapper OnUpdateHighlighter, pScrollBar->maximum() > 0";
            QPoint endPoint = QPointF(0, 1.5 * m_pTextEdit->height()).toPoint();
            endBlock = m_pTextEdit->cursorForPosition(endPoint).block();
        } else {
            qDebug() << "EditWrapper OnUpdateHighlighter, pScrollBar->maximum() <= 0";
            endBlock = m_pTextEdit->document()->lastBlock();
        }

        // 判断当前文件是否支持高亮处理
        if (m_pSyntaxHighlighter->definition().isValid()) {
            qDebug() << "EditWrapper OnUpdateHighlighter, m_pSyntaxHighlighter->definition().isValid()";
            // NOTE: 同样需要考虑极限条件下遍历全部文本的情况
            // 限制最多向上查找512个文本块
            static int s_MaxFindCount = 512;
            int maxFindBlockNumber = qMax(0, (beginBlock.blockNumber() - s_MaxFindCount));

            QChar begin = '{', end = '}';
            int braceDepth = 0;
            bool foundBrace = false;
            // 判断是否此文本块为折叠区域起始的文本块，判断'{''}'处理
            QTextBlock prevBlock = endBlock;
            while (prevBlock.isValid()
                    && !foundBrace
                    && maxFindBlockNumber < prevBlock.blockNumber()) {
                // 逆序查询文本块数据，查找起始文本块
                QString text = prevBlock.text();
                qDebug() << "EditWrapper OnUpdateHighlighter, text:" << text;
                for (int i = text.size() - 1; i >= 0; i--) {
                    if (text.at(i) == end) {
                        qDebug() << "EditWrapper OnUpdateHighlighter, text.at(i) == end";
                        braceDepth++;
                    } else if (text.at(i) == begin) {
                        qDebug() << "EditWrapper OnUpdateHighlighter, text.at(i) == begin";
                        braceDepth--;

                        // 防止'{'处于展示文本块中间的情况，查询不准确
                        if (braceDepth < 0) {
                            qDebug() << "EditWrapper OnUpdateHighlighter, braceDepth < 0";
                            braceDepth = 0;
                        }

                        // 判断是否查找到完整折叠区域
                        if (0 == braceDepth
                                && prevBlock.blockNumber() <= beginBlock.blockNumber()) {
                            qDebug() << "EditWrapper OnUpdateHighlighter, foundBrace";
                            foundBrace = true;
                            break;
                        }
                    }
                }

                prevBlock = prevBlock.previous();
            }

            // 仅在查询到折叠区域更新文本块
            if (foundBrace) {
                qDebug() << "EditWrapper OnUpdateHighlighter, foundBrace";
                foundBlock = prevBlock;
            }
        }

        if (!beginBlock.isValid() || !endBlock.isValid()) {
            qDebug() << "EditWrapper OnUpdateHighlighter, !beginBlock.isValid() || !endBlock.isValid()";
            return;
        }

        auto rehighlightBlock = [this](const QTextBlock &block) {
            qDebug() << "EditWrapper OnUpdateHighlighter, rehighlightBlock";
            m_pSyntaxHighlighter->setEnableHighlight(true);
            m_pSyntaxHighlighter->rehighlightBlock(block);
            m_pSyntaxHighlighter->setEnableHighlight(false);
        };

        if (foundBlock.isValid() && foundBlock < beginBlock) {
            qDebug() << "EditWrapper OnUpdateHighlighter, foundBlock.isValid() && foundBlock < beginBlock";
            for (QTextBlock var = foundBlock; var.isValid() && var != beginBlock; var = var.next())
            {
                // Kate syntax highlighter 在高亮文本后会设置数据，通过此数据判断是否需要重复设置高亮
                if (var.userData()) {
                    qDebug() << "EditWrapper OnUpdateHighlighter, var.userData()";
                    continue;
                }

                rehighlightBlock(var);
            }
        }

        for (QTextBlock var = beginBlock; var != endBlock; var = var.next()) {
            qDebug() << "EditWrapper OnUpdateHighlighter, rehighlightBlock";
            rehighlightBlock(var);
        }

        qDebug() << "EditWrapper OnUpdateHighlighter, rehighlightBlock";
        rehighlightBlock(endBlock);
    }
    qDebug() << "EditWrapper OnUpdateHighlighter, exit";
}

void EditWrapper::setTemFile(bool value)
{
    qDebug() << "EditWrapper setTemFile, value:" << value;
    m_bIsTemFile = value;
    qDebug() << "EditWrapper setTemFile, exit";
}

void EditWrapper::updateHighlighterAll()
{
    qDebug() << "EditWrapper updateHighlighterAll";
    if (m_pSyntaxHighlighter  && !m_bQuit && !m_bHighlighterAll) {
        qDebug() << "EditWrapper updateHighlighterAll, m_pSyntaxHighlighter is not nullptr";
        QTextBlock beginBlock = m_pTextEdit->document()->firstBlock();
        QTextBlock endBlock = m_pTextEdit->document()->lastBlock();

        if (!beginBlock.isValid() || !endBlock.isValid()) {
            qDebug() << "EditWrapper updateHighlighterAll, !beginBlock.isValid() || !endBlock.isValid()";
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
    qDebug() << "EditWrapper updateHighlighterAll, exit";
}

QDateTime EditWrapper::getLastModifiedTime() const
{
    qDebug() << "EditWrapper getLastModifiedTime";
    return m_tModifiedDateTime;
}

void EditWrapper::setLastModifiedTime(const QString &time)
{
    qDebug() << "EditWrapper setLastModifiedTime, time:" << time;
    m_tModifiedDateTime = QDateTime::fromString(time);
}
void EditWrapper::updateModifyStatus(bool bModified)
{
    qDebug() << "EditWrapper updateModifyStatus, bModified:" << bModified;
    if (getFileLoading()) {
        qDebug() << "Skipping modify status update during file loading";
        return;
    }
    
    QString filePath = m_pTextEdit->getFilePath();
    qDebug() << "Updating modify status for:" << filePath << "New status:" << bModified;
    
    if (!bModified) {
        qDebug() << "Updating save index for:" << filePath;
        m_pTextEdit->updateSaveIndex();
    }
    
    m_pTextEdit->document()->setModified(bModified);
    Window *pWindow = static_cast<Window *>(QWidget::window());
    pWindow->updateModifyStatus(filePath, bModified);
    qDebug() << "EditWrapper updateModifyStatus, exit";
}

void EditWrapper::updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath)
{
    qDebug() << "EditWrapper updateSaveAsFileName, strOldFilePath:" << strOldFilePath << "strNewFilePath:" << strNewFilePath;
    m_pWindow->updateSaveAsFileName(strOldFilePath, strNewFilePath);
}

//yanyuhan
void EditWrapper::setLineNumberShow(bool bIsShow, bool bIsFirstShow)
{
    qDebug() << "EditWrapper setLineNumberShow, bIsShow:" << bIsShow << "bIsFirstShow:" << bIsFirstShow;
    if (bIsShow && !bIsFirstShow) {
        qDebug() << "EditWrapper setLineNumberShow, bIsShow && !bIsFirstShow";
        //int lineNumberAreaWidth = m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->width();
        //int leftAreaWidth = m_pTextEdit->getLeftAreaWidget()->width();
        m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->show();
        //m_pTextEdit->getLeftAreaWidget()->setFixedWidth(leftAreaWidth + lineNumberAreaWidth);

    } else if (!bIsShow) {
        qDebug() << "EditWrapper setLineNumberShow, !bIsShow";
        // int lineNumberAreaWidth = m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->width();
        //int leftAreaWidth = m_pTextEdit->getLeftAreaWidget()->width();
        m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->hide();
        //m_pTextEdit->getLeftAreaWidget()->setFixedWidth(leftAreaWidth - lineNumberAreaWidth);
    }
    m_pTextEdit->bIsSetLineNumberWidth = bIsShow;
    m_pTextEdit->updateLeftAreaWidget();
    qDebug() << "EditWrapper setLineNumberShow, exit";
}

//显示空白符
void EditWrapper::setShowBlankCharacter(bool ok)
{
    qDebug() << "EditWrapper setShowBlankCharacter, ok:" << ok;
    if (ok) {
        qDebug() << "EditWrapper setShowBlankCharacter, ok is true";
        QTextOption opts = m_pTextEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag |= QTextOption::ShowTabsAndSpaces;
        // flag |= QTextOption::ShowLineAndParagraphSeparators;
        opts.setFlags(flag);
        m_pTextEdit->document()->setDefaultTextOption(opts);
    } else {
        qDebug() << "EditWrapper setShowBlankCharacter, ok is false";
        QTextOption opts = m_pTextEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag &= ~QTextOption::ShowTabsAndSpaces;
        // flag &= ~QTextOption::ShowLineAndParagraphSeparators;
        opts.setFlags(flag);
        m_pTextEdit->document()->setDefaultTextOption(opts);
    }
    qDebug() << "EditWrapper setShowBlankCharacter, exit";
}

BottomBar *EditWrapper::bottomBar()
{
    qDebug() << "EditWrapper bottomBar";
    return m_pBottomBar;
}

QString EditWrapper::filePath()
{
    qDebug() << "EditWrapper filePath";
    return  m_pTextEdit->getFilePath();
}

TextEdit *EditWrapper::textEditor()
{
    qDebug() << "EditWrapper textEditor";
    return m_pTextEdit;
}

Window *EditWrapper::window()
{
    qDebug() << "EditWrapper window";
    Window *window = static_cast<Window *>(QWidget::window());

    if (m_pWindow != window) {
        qDebug() << "EditWrapper window, m_pWindow != window";
        m_pWindow = window;
    }

    qDebug() << "EditWrapper window, return m_pWindow";
    return m_pWindow;
}

void EditWrapper::customEvent(QEvent *e)
{
    qDebug() << "EditWrapper customEvent";
    // 处理解析文件任务，大文件不会在单次事件任务中处理，而是每次读取一部分并将下次任务抛出
    if (static_cast<QEvent::Type>(ParseFileEvent::EParseFile) == e->type()) {
        // 中途退出则不继续处理
        if (m_bQuit) {
            qDebug() << "EditWrapper customEvent, m_bQuit is true";
            return;
        }

        bool errorOccurred = false;
        try {

            ParseFileEvent *parseEvent = static_cast<ParseFileEvent *>(e);
            int needReadLen = parseEvent->m_contentData.length() - parseEvent->m_alreadyReadOffset;

            // 调整最大读取长度(单次读取最大长度)
            if (needReadLen > EReadStepSize) {
                qDebug() << "EditWrapper customEvent, needReadLen > EReadStepSize";
                needReadLen = EReadStepSize;
            }

            // 转码数据并插入光标位置
            QByteArray text = parseEvent->m_contentData.mid(parseEvent->m_alreadyReadOffset, needReadLen);
            qDebug() << "EditWrapper customEvent, text:" << text;
            QTextCodec::ConverterState state;
            QString data = parseEvent->m_codec->toUnicode(text.constData(), text.size(), &state);

            // Note: Qt5 just under 2^30 characters (almost 1GB) in one QString.
            // In Qt6.8, the value up to almost 2^63, release on Qt6.
            parseEvent->m_cursor.insertText(data);

            // 当前为首次读取
            if (0 == parseEvent->m_alreadyReadOffset) {
                qDebug() << "EditWrapper customEvent, 0 == parseEvent->m_alreadyReadOffset";
                QTextCursor firstLineCursor = m_pTextEdit->textCursor();
                firstLineCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
                m_pTextEdit->setTextCursor(firstLineCursor);
                //秒开界面语法高亮
                OnUpdateHighlighter();
                qDebug() << "EditWrapper customEvent, OnUpdateHighlighter()";
            }

            // 此次读取后的偏移量
            int curReadOffset = parseEvent->m_alreadyReadOffset + needReadLen;

            // 是否已读取完成
            if (parseEvent->m_contentData.length() == curReadOffset) {
                qDebug() << "EditWrapper customEvent, parseEvent->m_contentData.length() == curReadOffset";
                // 异步读取结束
                m_bAsyncReadFileFinished = true;
            } else {
                qDebug() << "EditWrapper customEvent, parseEvent->m_contentData.length() != curReadOffset";
                ParseFileEvent *nextEvent = parseEvent->clone();
                // 调整已读取偏移位置
                nextEvent->m_alreadyReadOffset = curReadOffset;
                // 抛出下一次处理的事件，根据当前是否显示界面调整优先级
                qApp->postEvent(this, nextEvent, isVisible() ? Qt::NormalEventPriority : (Qt::LowEventPriority - 1));
            }

        } catch (const std::bad_alloc &) {
            errorOccurred = true;
            qWarning() << "Memory allocation failed";
        } catch (const std::exception &e) {
            errorOccurred = true;
            qWarning() << QString("Error occurred: %1").arg(e.what());
        }

        if (errorOccurred) {
            qDebug() << "EditWrapper customEvent, errorOccurred is true";
            m_bAsyncReadFileFinished = true;
            QMetaObject::invokeMethod(this, &EditWrapper::onReadAllocError, Qt::QueuedConnection);
        }
    }
    qDebug() << "EditWrapper customEvent, exit";
}

//支持大文本加载 界面不卡顿 秒关闭
void EditWrapper::loadContent(const QByteArray &strContent)
{
    qDebug() << "EditWrapper loadContent, strContent:" << strContent;
    if (m_pBottomBar != nullptr) {
        qDebug() << "EditWrapper loadContent, m_pBottomBar is not nullptr";
        m_pBottomBar->setChildEnabled(false);
    }
    if (m_pWindow != nullptr) {
        qDebug() << "EditWrapper loadContent, m_pWindow is not nullptr";
        m_pWindow->setPrintEnabled(false);
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_pTextEdit->clear();
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->setLeftAreaUpdateState(TextEdit::FileOpenBegin);
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
        qDebug() << "EditWrapper loadContent, len > max";
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
            qDebug() << "EditWrapper loadContent, len > InitContentPos";
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
            double progress = (inserted * 1.0) / len * 100;
            m_pBottomBar->setProgress(static_cast<int>(progress));
            if (!m_bQuit) {
                qDebug() << "EditWrapper loadContent, !m_bQuit";
                //data = strContent.mid(InitContentPos, len - InitContentPos);
                QByteArray text = strContent.mid(InitContentPos, len - InitContentPos);
                data = codec->toUnicode(text.constData(), text.size(), &state);
                cursor.insertText(data);
                inserted += (len - InitContentPos);
                progress = (inserted * 1.0) / len * 100;
                m_pBottomBar->setProgress(static_cast<int>(progress));
            }
        } else {
            qDebug() << "EditWrapper loadContent, m_bQuit";
            if (!m_bQuit) {
                qDebug() << "EditWrapper loadContent, !m_bQuit";
                //cursor.insertText(strContent);
                data = codec->toUnicode(strContent.constData(), strContent.size(), &state);
                cursor.insertText(data);
                QTextCursor firstLineCursor = m_pTextEdit->textCursor();
                firstLineCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
                m_pTextEdit->setTextCursor(firstLineCursor);
                //秒开界面语法高亮
                OnUpdateHighlighter();
                inserted += len;
                double progress = (inserted * 1.0) / len * 100;
                m_pBottomBar->setProgress(static_cast<int>(progress));
            }
        }
    }
    if (m_pWindow != nullptr) {
        qDebug() << "EditWrapper loadContent, m_pWindow is not nullptr";
        m_pWindow->setPrintEnabled(true);
    }
    if (m_pBottomBar != nullptr) {
        qDebug() << "EditWrapper loadContent, m_pBottomBar is not nullptr";
        m_pBottomBar->setChildEnabled(true);
        auto format = BottomBar::getEndlineFormat(strContent);
        m_pBottomBar->setEndlineMenuText(format);
    }
    m_pTextEdit->setReadOnly(false);
    m_pTextEdit->setLeftAreaUpdateState(TextEdit::FileOpenEnd);
    QApplication::restoreOverrideCursor();
    qDebug() << "EditWrapper loadContent, exit";
}

/**
 * @brief 文件加载时重新执行初始化，包括重置文件高亮配置、记录文件信息。
 * @param encode 当前文件解析的编码
 */
void EditWrapper::reinitOnFileLoad(const QByteArray &encode)
{
    qDebug() << "EditWrapper reinitOnFileLoad";
    m_Definition = m_Repository.definitionForFileName(m_pTextEdit->getFilePath());
    if (m_Definition.isValid() && !m_Definition.filePath().isEmpty()) {
        qDebug() << "EditWrapper reinitOnFileLoad, m_Definition.isValid() && !m_Definition.filePath().isEmpty()";
        if (!m_pSyntaxHighlighter) m_pSyntaxHighlighter = new CSyntaxHighlighter(m_pTextEdit->document());
        QString m_themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
        if (m_themePath.contains("dark")) {
            qDebug() << "EditWrapper reinitOnFileLoad, m_themePath contains dark";
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
        } else {
            qDebug() << "EditWrapper reinitOnFileLoad, m_themePath does not contain dark";
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
        }

        if (m_pSyntaxHighlighter) {
            qDebug() << "EditWrapper reinitOnFileLoad, m_pSyntaxHighlighter is not nullptr";
            m_pSyntaxHighlighter->setDefinition(m_Definition);
        }
        m_pTextEdit->setSyntaxDefinition(m_Definition);
        m_pBottomBar->getHighlightMenu()->setCurrentTextOnly(m_Definition.translatedName());
        qDebug() << "EditWrapper reinitOnFileLoad, exit";
    }

    if (!Utils::isDraftFile(m_pTextEdit->getFilePath())) {
        qDebug() << "EditWrapper reinitOnFileLoad, !Utils::isDraftFile(m_pTextEdit->getFilePath())";
        DRecentData data;
        data.appName = "Deepin Editor";
        data.appExec = "deepin-editor";
        DRecentManager::addItem(m_pTextEdit->getFilePath(), data);
    }

    // 初始化设置编码
    m_sCurEncode = encode;
    m_sFirstEncode = encode;
    qDebug() << "EditWrapper reinitOnFileLoad, exit";
}

void EditWrapper::onReadAllocError()
{
    qDebug() << "EditWrapper onReadAllocError";
    // 设置文本为只读模式，且不显示通知
    if (!m_pTextEdit->getReadOnlyMode()) {
        qDebug() << "EditWrapper onReadAllocError, !m_pTextEdit->getReadOnlyMode()";
        m_pTextEdit->toggleReadOnlyMode(true);
    }

    m_pWaringNotices->setMessage(tr("The file cannot be read, which may be too large or has been damaged!"));
    m_pWaringNotices->clearBtn();
    m_pWaringNotices->show();
    DMessageManager::instance()->sendMessage(m_pTextEdit, m_pWaringNotices);
    qDebug() << "EditWrapper onReadAllocError, exit";
}

void EditWrapper::clearDoubleCharaterEncode()
{
    qDebug() << "EditWrapper clearDoubleCharaterEncode";
    if (QFileInfo(filePath()).baseName().contains("double")
            || QFileInfo(filePath()).baseName().contains("user")
            || QFileInfo(filePath()).baseName().contains("four")) {
        if (QFileInfo(filePath()).size() > 500 * 1024) {
            qDebug() << "EditWrapper size > 500 * 1024, return";
            return;
        }
        emit sigClearDoubleCharaterEncode();
    }
    qDebug() << "EditWrapper clearDoubleCharaterEncode, exit";
}
