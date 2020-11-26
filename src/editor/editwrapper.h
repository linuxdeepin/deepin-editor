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

#ifndef EDITORBUFFER_H
#define EDITORBUFFER_H

#include "../editor/dtextedit.h"
#include "../widgets/bottombar.h"
#include "../controls/warningnotices.h"
#include <QVBoxLayout>
#include <QWidget>
#include <DMessageManager>
#include <DFloatingMessage>
#include <QByteArray>
#include <QTextCodec>
#include <DDialog>
#include <DMessageBox>
#include <DFileDialog>

class Window;
class EditWrapper : public QWidget
{
    Q_OBJECT

public:
    struct FileStateItem {
        QDateTime modified;
        QFile::Permissions permissions;
    };

    EditWrapper(Window* window=nullptr,QWidget *parent = nullptr);
    ~EditWrapper();

    //清除焦点　梁卫东　２０２０－０９－１４　１１：００：５０
    void clearAllFocus();
    void setQuitFlag();
    bool getFileLoading();

    //打开文件
    void openFile(const QString &filepath);
    //以默认编码encode重写读取去文件
    bool readFile(QByteArray encode="");
    //保存文件
    bool saveFile();
    //重新加载文件编码
    bool saveAsFile(const QString &newFilePath, QByteArray encodeName);
    //保存草稿文件
    bool saveDraftFile();
    //另存为第一次打开文件编码文件
    bool saveAsFile();
    //重新加载文件编码 1.文件修改 2.文件未修改处理逻辑一样 切换编码重新加载和另存为 梁卫东
    bool reloadFileEncode(QByteArray encode);
    //重写加载修改文件
    void reloadModifyFile();
    //获取文件编码
    QString getTextEncode();
    //跟新路径
    void updatePath(const QString &file);
    //判断是否修改
    bool isModified();
    //判断是否草稿文件
    bool isDraftFile();
    //判断内容是否为空
    bool isPlainTextEmpty();

    void hideWarningNotices();
    void checkForReload();
    void initToastPosition();
    void showNotify(const QString &message);
    bool getTextChangeFlag();
    void setTextChangeFlag(bool bFlag);
    void setLineNumberShow(bool bIsShow,bool bIsFirstShow = false);
    void setShowBlankCharacter(bool ok);
    //
    BottomBar *bottomBar();
    QString filePath();
    TextEdit *textEditor();
private:
    // 类似setPlainText(QString) 接口支持大文本加载 不卡顿 秒退出 梁卫东 2020年11月11日16:56:27
    void loadContent(const QByteArray&);
private:
    void handleCursorModeChanged(TextEdit::CursorMode mode);
    void handleHightlightChanged(const QString &name);

    int GetCorrectUnicode1(const QByteArray &ba);

public slots:
    void slotTextChange();
    void handleFileLoadFinished(const QByteArray &encode,const QByteArray &content);
signals:
    void requestSaveAs();
    void sigCodecSaveFile(const QString &strOldFilePath, const QString &strNewFilePath);
private:
    //第一次打开文件编码
    QString m_sFirstEncode= QString("UTF-8");
    //当前切换文件编码
    QString  m_sCurEncode =QString("UTF-8");


    TextEdit *m_pTextEdit = nullptr;
    BottomBar *m_pBottomBar = nullptr;
    WarningNotices *m_pWaringNotices = nullptr;

    QDateTime m_tModifiedDateTime;


    bool m_bTextChange = true;
    bool m_bIsContinue;
    //退出
    bool m_bQuit = false;
    //文件是否加载
    bool m_bFileLoading = false;

    Window* m_pWindow= nullptr;
};

#endif
