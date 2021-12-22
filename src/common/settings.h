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
#include <QKeyEvent>
#include <QDebug>
#include <DApplication>
#include <QLabel>
#include <QPushButton>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DTK_USE_NAMESPACE

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QWidget *parent = 0);
    ~Settings();

    void dtkThemeWorkaround(QWidget *parent, const QString &theme);
    //static QWidget *createFontComBoBoxHandle(QObject *obj);
    static QPair<QWidget*, QWidget*> createFontComBoBoxHandle(QObject *obj);
    static QPair<QWidget*, QWidget*> createKeySequenceEditHandle(QObject *obj);
    static Settings* instance();
    void setSettingDialog(DSettingsDialog *settingsDialog);

    int m_iDefaultFontSize = 12;
    int m_iMaxFontSize = 50;
    int m_iMinFontSize = 8;
    DSettings *settings {nullptr};

signals:
    void sigAdjustFont(QString name);
    void sigAdjustFontSize(int fontSize);
    void sigAdjustTabSpaceNumber(int number);
    void sigAdjustWordWrap(bool enable);
    void sigAdjustBookmark(bool enable);
    void sigShowCodeFlodFlag(bool enable);
    void sigShowBlankCharacter(bool enable);
    void sigHightLightCurrentLine(bool enable);
    void sigThemeChanged(const QString &theme);
    void sigSetLineNumberShow(bool bIsShow);
    void sigChangeWindowSize(QString mode);

public slots:
    //自定义快捷键 ut002764 2021.6.28
    void slotCustomshortcut(const QString &key, const QVariant &value);

    //添加信号槽 ut002764-07-05
    void slotsigAdjustFont(QVariant value);
    void slotsigAdjustFontSize(QVariant value);
    void slotsigAdjustWordWrap(QVariant value);
    void slotsigSetLineNumberShow(QVariant value);
    void slotsigAdjustBookmark(QVariant value);
    void slotsigShowCodeFlodFlag(QVariant value);
    void slotsigShowBlankCharacter(QVariant value);
    void slotsigHightLightCurrentLine(QVariant value);
    void slotsigAdjustTabSpaceNumber(QVariant value);
    void slotupdateAllKeysWithKeymap(QVariant value);

private:
    void updateAllKeysWithKeymap(QString keymap);
    void copyCustomizeKeysFromKeymap(QString keymap);
    bool checkShortcutValid(const QString &Name, QString Key, QString &Reason, bool &bIsConflicts);
    bool isShortcutConflict(const QString &Name, const QString &Key);
    DDialog *createDialog(const QString &title, const QString &content, const bool &bIsConflicts);

private:
    Dtk::Core::QSettingBackend *m_backend {nullptr};

    bool m_bUserChangeKey = false;
    DSettingsDialog *m_pSettingsDialog;
    static Settings* s_pSetting;
    DDialog *m_pDialog;
};

class KeySequenceEdit : public DKeySequenceEdit
{
public:
    explicit KeySequenceEdit(DTK_CORE_NAMESPACE::DSettingsOption *opt, QWidget *parent = nullptr);
    /**
     * @brief option 获取设置页面操作句柄
     */
    DTK_CORE_NAMESPACE::DSettingsOption *option();
    /**
     * @brief slotDSettingsOptionvalueChanged 设置页面配置修改
     */
    void slotDSettingsOptionvalueChanged(const QVariant &value);

protected:
    /**
     * @brief eventFilter 设置页面快捷键类的事件过滤器
     */
    bool eventFilter(QObject *object, QEvent *event);

private:
    DTK_CORE_NAMESPACE::DSettingsOption *m_pOption = nullptr;
};

#endif // SETTINGS_H
