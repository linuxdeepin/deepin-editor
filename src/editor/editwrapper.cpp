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
#include <QFileInfo>

DCORE_USE_NAMESPACE

EditWrapper::EditWrapper(Window* window,QWidget *parent)
    : QWidget(parent),
      m_pWindow(window),
      m_pTextEdit(new TextEdit(this)),
      m_pBottomBar(new BottomBar(this)),
      m_pWaringNotices(new WarningNotices(WarningNotices::ResidentType,this))

{
    m_pWaringNotices->hide();
    // Init layout and widgets.
    QHBoxLayout* m_layout = new QHBoxLayout;
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
    connect(m_pTextEdit->verticalScrollBar(),&QScrollBar::valueChanged,this,[this](int){
        OnUpdateHighlighter();
    });
}

EditWrapper::~EditWrapper()
{
    disconnect(m_pTextEdit);
    disconnect(m_pWaringNotices);
}

void EditWrapper::setQuitFlag()
{
    m_bQuit = true;
}

bool EditWrapper::getFileLoading()
{
    return (m_bQuit || m_bFileLoading);
}

void EditWrapper::openFile(const QString &filepath,QString qstrTruePath,bool bIsTemFile)
{

    m_bIsTemFile = bIsTemFile;
    // update file path.
    updatePath(filepath,qstrTruePath);
    m_pTextEdit->setIsFileOpen();

    FileLoadThread *thread = new FileLoadThread(filepath);
    // begin to load the file.
    connect(thread, &FileLoadThread::sigLoadFinished, this, &EditWrapper::handleFileLoadFinished);
    connect(thread, &FileLoadThread::finished, thread, &FileLoadThread::deleteLater);
    thread->start();
}

bool EditWrapper::readFile(QByteArray encode)
{
   QByteArray newEncode = encode;
   if(newEncode.isEmpty()){
      newEncode = DetectCode::GetFileEncodingFormat(m_pTextEdit->getFilePath());
      m_sFirstEncode = newEncode;
   }

    QFile file(m_pTextEdit->getFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileContent = file.readAll();
        QByteArray Outdata;
        DetectCode::ChangeFileEncodingFormat(fileContent,Outdata,newEncode,QString("UTF-8"));
        loadContent(Outdata);
        file.close();
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
    if(Utils::isDraftFile(m_pTextEdit->getFilePath()) &&  m_pTextEdit->toPlainText().isEmpty()) {
        m_sCurEncode = encode;
        m_sFirstEncode = encode;
        return true;
    }


    //1.如果修改切换编码提示用户是否保存,不保存重新打开文件读取.2.没有修改是否另存为
    if(m_pTextEdit->getModified())
    {
        DDialog *dialog = new DDialog(tr("Encoding changed. Do you want to save the file now?"), "", this);
        dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
        dialog->setIcon(QIcon::fromTheme("deepin-editor"));
        dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);//取消
 //       dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);//不保存
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
            if(Utils::isDraftFile(m_pTextEdit->getFilePath())){
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
    if (m_pTextEdit->getModified()) {
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
           if(Utils::isDraftFile(m_pTextEdit->getFilePath())){
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

    QFileInfo fi(m_pTextEdit->getFilePath());
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

            QFileInfo fi(qstrFilePath);
            m_tModifiedDateTime = fi.lastModified();

            // did save work?
            // only finalize if stream status == OK
            bool ok = (error == QFileDevice::NoError);

            // update status.
            if (ok)  updateModifyStatus(false);
            m_bIsTemFile = false;
            return ok;

        }else {
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
    }else {      
        return false;
    }

}

bool EditWrapper::saveTemFile(QString qstrDir)
{
    QFile file(qstrDir);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray fileContent = m_pTextEdit->toPlainText().toLocal8Bit();
//        if(!fileContent.isEmpty())
//        {
            QByteArray Outdata;
            DetectCode::ChangeFileEncodingFormat(fileContent,Outdata,QString("UTF-8"),m_sCurEncode);
            file.write(Outdata);
            QFileDevice::FileError error = file.error();
            file.close();
            m_sFirstEncode = m_sCurEncode;

            // did save work?
            // only finalize if stream status == OK
            bool ok = (error == QFileDevice::NoError);

            // update status.
            if (ok)  updateModifyStatus(isModified());
            return ok;

//        }else {
//            file.write(fileContent);
//            QFileDevice::FileError error = file.error();
//            file.close();
//            m_sFirstEncode = m_sCurEncode;

//            // did save work?
//            // only finalize if stream status == OK
//            bool ok = (error == QFileDevice::NoError);

//            // update status.
//            if (ok)  updateModifyStatus(true);
//            return ok;
//        }
    }else {
        return false;
    }
}

void EditWrapper::updatePath(const QString &file,QString qstrTruePath)
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
        QFile(m_pTextEdit->getFilePath()).remove();
        updateSaveAsFileName(m_pTextEdit->getFilePath(),newFilePath);
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
    if (Utils::isDraftFile(m_pTextEdit->getTruePath())) return;

    QFileInfo fi(m_pTextEdit->getTruePath());

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


void EditWrapper::handleFileLoadFinished(const QByteArray &encode,const QByteArray &content)
{   

    qint64 time1 = QDateTime::currentMSecsSinceEpoch();
    m_Definition = m_Repository.definitionForFileName(m_pTextEdit->getFilePath());
    qDebug()<<"===========begin load file:"<<time1;
    qDebug()<<m_Definition.isValid()<<m_Definition.filePath()<<m_Definition.translatedName();
    if(m_Definition.isValid() && !m_Definition.filePath().isEmpty()){
        if(!m_pSyntaxHighlighter) m_pSyntaxHighlighter = new CSyntaxHighlighter(m_pTextEdit->document());
        QString m_themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
        if(m_themePath.contains("dark")){
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
        }else {
            m_pSyntaxHighlighter->setTheme(m_Repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
        }

        if(m_pSyntaxHighlighter) m_pSyntaxHighlighter->setDefinition(m_Definition);;
        m_pTextEdit->setSyntaxDefinition(m_Definition);
        m_pBottomBar->getHighlightMenu()->setCurrentTextOnly(m_Definition.translatedName());

    }

    qint64 time2 = QDateTime::currentMSecsSinceEpoch();
    qDebug()<<"===========load SyntaxHighter:"<<time2 - time1;



    if (!Utils::isDraftFile(m_pTextEdit->getFilePath())) {
        DRecentData data;
        data.appName = "Deepin Editor";
        data.appExec = "deepin-editor";
        DRecentManager::addItem(m_pTextEdit->getFilePath(), data);
    }

    bool flag = m_pTextEdit->getReadOnlyPermission();
    if(flag == true) m_pTextEdit->setReadOnlyPermission(false);

    m_bFileLoading = true;
    m_sCurEncode = encode;
    m_sFirstEncode = encode;
  
    //备份显示修改状态
    if (m_bIsTemFile) {
       // m_bIsTemFile = false;
        updateModifyStatus(true);
    }

    loadContent(content);

    qint64 time3 = QDateTime::currentMSecsSinceEpoch();
    qDebug()<<"===========end load file:"<<time3 - time1;

    PerformanceMonitor::openFileFinish(filePath(), QFileInfo(filePath()).size());

    m_bFileLoading = false;
    if(flag == true) m_pTextEdit->setReadOnlyPermission(true);
    if(m_bQuit) return;
    m_pTextEdit->setTextFinished();

    QStringList temFileList = Settings::instance()->settings->option("advance.editor.browsing_history_temfile")->value().toStringList();

    for (int var = 0; var < temFileList.count(); ++var) {
        QJsonParseError jsonError;
        // 转化为 JSON 文档
        QJsonDocument doucment = QJsonDocument::fromJson(temFileList.value(var).toUtf8(), &jsonError);
        // 解析未发生错误
        if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError))
        {
            if (doucment.isObject())
            {
                QString temFilePath;
                QString localPath;
                // JSON 文档为对象
                QJsonObject object = doucment.object();  // 转化为对象

                if (object.contains("localPath") || object.contains("temFilePath"))
                {  // 包含指定的 key
                    QJsonValue localPathValue = object.value("localPath");  // 获取指定 key 对应的 value
                    QJsonValue temFilePathValue = object.value("temFilePath");  // 获取指定 key 对应的 value

                    if (localPathValue.toString() == m_pTextEdit->getFilePath())
                    {
                        QJsonValue value = object.value("cursorPosition");  // 获取指定 key 对应的 value

                        if (value.isString())
                        {
                            QTextCursor cursor = m_pTextEdit->textCursor();
                            cursor.setPosition(value.toString().toInt());
                            m_pTextEdit->setTextCursor(cursor);
                            OnUpdateHighlighter();
                            break;
                        }
                    } else if (temFilePathValue.toString() == m_pTextEdit->getFilePath()) {
                        QJsonValue value = object.value("cursorPosition");  // 获取指定 key 对应的 value

                        if (value.isString())
                        {
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
        //m_bIsTemFile = false;
        updateModifyStatus(true);
    }

    if(m_pSyntaxHighlighter) m_pSyntaxHighlighter->setEnableHighlight(true);

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
    if(m_pSyntaxHighlighter){
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
    if(m_pSyntaxHighlighter  && !m_bQuit){
      QScrollBar* pScrollBar = m_pTextEdit->verticalScrollBar();
      //QTextBlock textBlock = m_pTextEdit->document()->findBlockByNumber(value);

      QTextBlock beginBlock = m_pTextEdit->document()->findBlockByNumber(m_pTextEdit->getFirstVisibleBlockId());

      QTextBlock endBlock;
      if (pScrollBar->maximum() > 0){
          QPoint endPoint = QPointF(0,m_pTextEdit->height() + (m_pTextEdit->height()/pScrollBar->maximum())*pScrollBar->value()).toPoint();
         endBlock = m_pTextEdit->cursorForPosition(endPoint).block();

      }else {
         endBlock = m_pTextEdit->document()->lastBlock();
      }

    if(!beginBlock.isValid() || !endBlock.isValid()) return;

     for (QTextBlock var = beginBlock; var != endBlock; var= var.next()) {
          m_pSyntaxHighlighter->setEnableHighlight(true);
          m_pSyntaxHighlighter->rehighlightBlock(var);
          m_pSyntaxHighlighter->setEnableHighlight(false);
     }

     qDebug()<<"OnUpdateHighlighter:"<<beginBlock.text()<<endBlock.text();
    }
}

void EditWrapper::updateModifyStatus(bool bModified)
{
    if(getFileLoading()) return;
    m_pTextEdit->document()->setModified(bModified);
    Window *pWindow = static_cast<Window*>(QWidget::window());
    pWindow->updateModifyStatus(m_pTextEdit->getFilePath(),bModified);
}

void EditWrapper::updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath)
{
    m_pWindow->updateSaveAsFileName(strOldFilePath,strNewFilePath);
}

//yanyuhan
void EditWrapper::setLineNumberShow(bool bIsShow ,bool bIsFirstShow)
{
    if(bIsShow && !bIsFirstShow) {
        int lineNumberAreaWidth = m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->width();
        int leftAreaWidth = m_pTextEdit->getLeftAreaWidget()->width();
        m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->show();
        m_pTextEdit->getLeftAreaWidget()->setFixedWidth(leftAreaWidth + lineNumberAreaWidth);

    } else if(!bIsShow) {
        int lineNumberAreaWidth = m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->width();
        int leftAreaWidth = m_pTextEdit->getLeftAreaWidget()->width();
        m_pTextEdit->getLeftAreaWidget()->m_pLineNumberArea->hide();
        m_pTextEdit->getLeftAreaWidget()->setFixedWidth(leftAreaWidth - lineNumberAreaWidth);
    }
    m_pTextEdit->bIsSetLineNumberWidth = bIsShow;
    m_pTextEdit->updateLeftAreaWidget();
}

//显示空白符
void EditWrapper::setShowBlankCharacter(bool ok)
{
    if(ok){
        QTextOption opts = m_pTextEdit->document()->defaultTextOption();
        QTextOption::Flags flag = opts.flags();
        flag |= QTextOption::ShowTabsAndSpaces;
       // flag |= QTextOption::ShowLineAndParagraphSeparators;
        opts.setFlags(flag);
        m_pTextEdit->document()->setDefaultTextOption(opts);
    }else {
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
    return m_pWindow;
}

//支持大文本加载 界面不卡顿 秒关闭
void EditWrapper::loadContent(const QByteArray &content)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_pTextEdit->clear();
    m_bQuit = false;
    //QTextDocument *doc = m_pTextEdit->document();
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
              QTextCursor firstLineCursor = m_pTextEdit->textCursor();
              firstLineCursor.movePosition(QTextCursor::Start,QTextCursor::MoveAnchor);
              m_pTextEdit->setTextCursor(firstLineCursor);
              //秒开界面语法高亮
              OnUpdateHighlighter();
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
            QTextCursor firstLineCursor = m_pTextEdit->textCursor();
            firstLineCursor.movePosition(QTextCursor::Start,QTextCursor::MoveAnchor);
            m_pTextEdit->setTextCursor(firstLineCursor);
            //秒开界面语法高亮
            OnUpdateHighlighter();
            QApplication::processEvents();
            if(!m_bQuit){
                data = content.mid(InitContentPos,len-InitContentPos);
                cursor.insertText(data);
            }
        }else {
           if(!m_bQuit) {
              cursor.insertText(content);
              QTextCursor firstLineCursor = m_pTextEdit->textCursor();
              firstLineCursor.movePosition(QTextCursor::Start,QTextCursor::MoveAnchor);
              m_pTextEdit->setTextCursor(firstLineCursor);
               //秒开界面语法高亮
              OnUpdateHighlighter();
           }
        }
    }

    QApplication::restoreOverrideCursor();
}
