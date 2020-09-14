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

#include "dbusinterface.h"
#include "dtextedit.h"
#include "widgets/bottombar.h"
#include "warningnotices.h"

#include <QVBoxLayout>
#include <QWidget>
#include <DMessageManager>
#include <DFloatingMessage>
#include <QByteArray>
#include <QTextCodec>
#include <DDialog>
#include <DMessageBox>
#include <DFileDialog>

class EditWrapper : public QWidget
{
    Q_OBJECT

public:
    // end of line mode.
    enum EndOfLineMode {
        eolUnknown = -1,
        eolUnix = 0,
        eolDos = 1,
        eolMac = 2
    };

    struct FileStateItem {
        QDateTime modified;
        QFile::Permissions permissions;
    };

    EditWrapper(QWidget *parent = 0);
    ~EditWrapper();

    //清除焦点　梁卫东　２０２０－０９－１４　１１：００：５０
    void clearAllFocus();

    void openFile(const QString &filepath);
    bool saveFile();
    bool saveAsFile(const QString &newFilePath, QByteArray encodeName);
    void updatePath(const QString &file);
    void refresh();
    bool isLoadFinished() { return m_isLoadFinished; }

    EndOfLineMode endOfLineMode();
    void setEndOfLineMode(EndOfLineMode eol);
    void setTextCodec(QByteArray encodeName, bool reload = false);

    void hideWarningNotices();
    void checkForReload();
    void initToastPosition();
    void showNotify(const QString &message);
    bool getTextChangeFlag();
    void setTextChangeFlag(bool bFlag);
    void setLineNumberShow(bool bIsShow,bool bIsFirstShow = false);
    void setShowBlankCharacter(bool ok);

    BottomBar *bottomBar() { return m_bottomBar; }
    QString filePath() { return m_textEdit->filepath; }
    TextEdit *textEditor() { return m_textEdit; }
private:
    void detectEndOfLine();
    void handleCursorModeChanged(TextEdit::CursorMode mode);
    void handleHightlightChanged(const QString &name);
    void handleFileLoadFinished(const QByteArray &encode,const QString &content);
    void setTextCodec(QTextCodec *codec, bool reload = false);

    int GetCorrectUnicode1(const QByteArray &ba);
    bool saveDraftFile();
    void readFile(const QString &filePath);
public slots:
    void onFileClosed();
    void slotTextChange();

signals:
    void requestSaveAs();
    void sigCodecSaveFile(const QString &strOldFilePath, const QString &strNewFilePath);

protected:
    void resizeEvent(QResizeEvent *);
private:
    QHBoxLayout *m_layout;
    TextEdit *m_textEdit;
    QTextCodec *m_textCodec;
    BottomBar *m_bottomBar;
    EndOfLineMode m_endOfLineMode;
    bool m_isLoadFinished;
    QDateTime m_modified;
    bool m_isRefreshing;
    WarningNotices *m_waringNotices;
    bool m_bTextChange = true;
    QByteArray m_BeforeEncodeName {"UTF-8"};
    bool m_bIsContinue;
};

#endif
