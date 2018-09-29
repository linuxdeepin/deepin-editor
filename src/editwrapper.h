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

#include <QVBoxLayout>
#include <QWidget>

class EditWrapper : public QWidget
{
    Q_OBJECT

public:
    EditWrapper(QWidget *parent = 0);
    ~EditWrapper();

    void openFile(const QString &filepath);
    bool saveFile(const QString &encode, const QString &newline);
    bool saveFile();
    void updatePath(const QString &file);
    QByteArray fileEncode() { return m_fileEncode; }
    bool isLoadFinished() { return m_isLoadFinished; }
    bool isWritable() { return m_isWritable; }

    DTextEdit *textEditor() { return m_textEdit; }

private:
    void detectNewline();
    void handleFileLoadFinished(const QByteArray &encode, const QString &content);

private:
    QHBoxLayout *m_layout;
    DTextEdit *m_textEdit;
    QByteArray m_fileEncode;

    bool m_saveFinish;
    int m_autoSaveInternal;
    bool m_hasLoadFile = false;
    bool m_isWritable = false;
    bool m_isLoadFinished = true;
    QString m_newline;
};

#endif
