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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "dsettingsdialog.h"
#include <qsettingbackend.h>
#include <QSettings>

DWIDGET_USE_NAMESPACE
DTK_USE_NAMESPACE

class Settings : public QObject 
{
    Q_OBJECT
    
public:
    Settings(QObject *parent = 0);
    ~Settings();
    
    int defaultFontSize = 12;
    int maxFontSize = 50;
    int minFontSize = 8;
    
    Dtk::Core::DSettings* settings;
    
signals:
    void adjustFontSize(int fontSize);
    void adjustTabSpaceNumber(int number);
        
public slots:
    void popupSettingsDialog();
    
private:
    Dtk::Core::QSettingBackend* backend;
    DSettingsDialog settingsDialog;

    void dtkThemeWorkaround(QWidget *parent, const QString &theme);
};

#endif // SETTINGS_H
