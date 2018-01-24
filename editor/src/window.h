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

#ifndef WINDOW_H
#define WINDOW_H

#include "dmainwindow.h"
#include <dimagebutton.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QWidget>
#include "editor.h"
#include "tabbar.h"
#include <QStackedLayout>

DWIDGET_USE_NAMESPACE

class Window : public DMainWindow
{
    Q_OBJECT
    
public:
    Window(DMainWindow *parent = 0);
    ~Window();
    
    void keyPressEvent(QKeyEvent *keyEvent);
    
    void addTab(QString file);
    int isFileInTabs(QString file);
    void activeTab(int index);
    void openFile();
    void saveFile();
    void saveAsFile();
                      
    void toggleFullscreen();
                           
    Editor* getActiveEditor();
                             
    void saveFileAsAnotherPath(QString fromPath, QString toPath);
    
    void addTabWithContent(QString tabName, QString filepath, QString content);
    
    static QPixmap getFileScreenshot(int index);
                                                                
signals:
    void popTab(QString tabName, QString filepath, QString content);
    
public slots:
    void handleSwitchToFile(QString filepath);
    void handleCloseFile(QString filepath);
    void handleTabReleaseRequested(QString tabName, QString filepath, int index);

    void addBlankTab();
    
private:
    QWidget *layoutWidget;
    QStackedLayout *layout;
    
    Tabbar *tabbar;
    QMap<QString, Editor*> editorMap;
    
    int notifyPadding = 20;
};

#endif
