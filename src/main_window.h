/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dmainwindow.h"
#include <dimagebutton.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QWidget>
#include "editor.h"
#include "tabbar.h"

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(DMainWindow *parent = 0);
    ~MainWindow();
    
    void keyPressEvent(QKeyEvent *keyEvent);
    void resizeEvent(QResizeEvent* event);
    
private:
    QWidget *layoutWidget;
    QVBoxLayout *layout;
    
    QWidget *tabbarWidget;
    QHBoxLayout *tabbarLayout;
    
    Editor *editor;
    
    Tabbar *tabbar;
    
    DImageButton *tabAddButton;
};

#endif
