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

#ifndef STARTMANAGER_H
#define STARTMANAGER_H

#include <QObject>
#include "window.h"

class StartManager : public QObject
{
    Q_OBJECT
    
    Q_CLASSINFO("D-Bus Interface", "com.deepin.Editor")
    
public:
    StartManager(QObject *parent = 0);
                                     
    QList<int> fileIsOpened(QString file);
    
public slots:
    Q_SCRIPTABLE void openFilesInWindow(QStringList files);
    Q_SCRIPTABLE void openFilesInTab(QStringList files);
    
private:
    QList<Window*> windows;
    
    void initWindowPosition(Window *window, bool alwaysCenter=false);
    Window* createWindow(bool alwaysCenter=false);
    void popupExitTab(QList<int> fileIndexes);
};

#endif
