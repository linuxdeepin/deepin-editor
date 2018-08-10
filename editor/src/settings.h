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
#include <QPointer>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DTK_USE_NAMESPACE

class Settings : public QObject
{
    Q_OBJECT

public:
    Settings(QWidget *parent = 0);
    ~Settings();

    void dtkThemeWorkaround(QWidget *parent, const QString &theme);

    int defaultFontSize = 12;
    int maxFontSize = 50;
    int minFontSize = 8;

    QPointer<DSettings> settings;

signals:
    void adjustFont(QString name);
    void adjustFontSize(int fontSize);
    void adjustTabSpaceNumber(int number);

private:
    void updateAllKeysWithKeymap(QString keymap);
    void copyCustomizeKeysFromKeymap(QString keymap);

private:
    Dtk::Core::QSettingBackend *m_backend;
    DSettingsDialog *m_settingsDialog;

    QString m_configPath;
    bool m_userChangeKey = false;
};

#endif // SETTINGS_H
