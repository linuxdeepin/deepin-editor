/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#ifndef EDITOR_H
#define EDITOR_H

#include "dbusinterface.h"
#include "texteditor.h"

#include <QVBoxLayout>
#include <QWidget>

class Editor : public QWidget
{
    Q_OBJECT

public:
    Editor(QWidget *parent = 0);

    void loadFile(const QString &filepath);
    bool saveFile(const QString &encode, const QString &newline);
    bool saveFile();

    void updatePath(QString file);

    TextEditor *textEditor;

private:
    void detectNewline();

private:
    QHBoxLayout *m_layout;
    QByteArray m_fileEncode;

    bool m_saveFinish;
    int m_autoSaveInternal;
    bool m_hasLoadFile = false;
    QString m_newline;
};

#endif
