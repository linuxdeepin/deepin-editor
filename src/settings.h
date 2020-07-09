/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
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
#include <DKeySequenceEdit>
#include <DDialog>
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
    //static QWidget *createFontComBoBoxHandle(QObject *obj);
    static QPair<QWidget*, QWidget*> createFontComBoBoxHandle(QObject *obj);
    static QPair<QWidget*, QWidget*> createKeySequenceEditHandle(QObject *obj);
    static Settings* instance();

    void setSettingDialog(DSettingsDialog *settingsDialog);

    int defaultFontSize = 12;
    int maxFontSize = 50;
    int minFontSize = 8;

    DSettings *settings;

signals:
    void adjustFont(QString name);
    void adjustFontSize(int fontSize);
    void adjustTabSpaceNumber(int number);
    void adjustWordWrap(bool enable);
    void showCodeFlodFlag(bool enable);
    void themeChanged(const QString &theme);
    void setLineNumberShow(bool bIsShow);

private:
    void updateAllKeysWithKeymap(QString keymap);
    void copyCustomizeKeysFromKeymap(QString keymap);
    bool checkShortcutValid(const QString &Name, const QString &Key, QString &Reason, bool &bIsConflicts);
    bool isShortcutConflict(const QString &Name, const QString &Key);
    DDialog *createDialog(const QString &title, const QString &content, const bool &bIsConflicts);

private:
    Dtk::Core::QSettingBackend *m_backend;

    QString m_configPath;
    bool m_userChangeKey = false;
    DSettingsDialog *m_pSettingsDialog;
    static Settings* m_setting;
    DKeySequenceEdit *m_shortCutLineEdit;
    DDialog *m_pDialog;
};

class KeySequenceEdit : public DKeySequenceEdit
{
public:
    KeySequenceEdit(DTK_CORE_NAMESPACE::DSettingsOption *opt, QWidget *parent = nullptr): DKeySequenceEdit(parent)
    {
        m_poption = opt;
    }
    DTK_CORE_NAMESPACE::DSettingsOption *option()
    {
        return m_poption;
    }
private:
    DTK_CORE_NAMESPACE::DSettingsOption *m_poption = nullptr;
};

#endif // SETTINGS_H
