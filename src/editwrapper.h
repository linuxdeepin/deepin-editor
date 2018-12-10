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
#include "widgets/toast.h"

#include <QVBoxLayout>
#include <QWidget>

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

    void openFile(const QString &filepath);
    bool saveFile();
    void updatePath(const QString &file);
    void refresh();
    bool isLoadFinished() { return m_isLoadFinished; }

    EndOfLineMode endOfLineMode();
    void setEndOfLineMode(EndOfLineMode eol);
    void setTextCodec(QTextCodec *codec);

    BottomBar *bottomBar() { return m_bottomBar; }
    QString filePath() { return m_textEdit->filepath; }
    DTextEdit *textEditor() { return m_textEdit; }
    bool toastVisible() { return m_toast->isVisible(); }
    void hideToast();

    void checkForReload();
    void initToastPosition();

private:
    void detectEndOfLine();
    void handleCursorModeChanged(DTextEdit::CursorMode mode);
    void handleHightlightChanged(const QString &name);
    void handleFileLoadFinished(const QByteArray &encode, const QString &content);

protected:
    void resizeEvent(QResizeEvent *);

private:
    QHBoxLayout *m_layout;
    DTextEdit *m_textEdit;
    BottomBar *m_bottomBar;
    QTextCodec *m_textCodec;

    EndOfLineMode m_endOfLineMode;
    bool m_isLoadFinished;
    QDateTime m_modified;
    Toast *m_toast;
};

#endif
