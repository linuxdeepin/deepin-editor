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
#pragma once
#include "settings.h"
#include <QKeyEvent>
#include <QObject>
#include <QPainter>
#include <QString>
#include <QImage>
#include <DMainWindow>
#include <QIcon>
#include <QDBusInterface>
#include <QDBusReply>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)      if((p)) { delete (p); (p) = nullptr;}
#endif

#define DEEPIN_THEME        "/usr/share/deepin-editor/themes/deepin.theme"
#define DEEPIN_DARK_THEME   "/usr/share/deepin-editor/themes/deepin_dark.theme"
#define DATA_SIZE_1024      1024
#define TEXT_EIDT_MARK_ALL  "MARK_ALL"
#define PROC_MEMINFO_PATH   "/proc/meminfo"
#define COPY_CONSUME_MEMORY_MULTIPLE 9      //复制文本时内存占用系数
#define PASTE_CONSUME_MEMORY_MULTIPLE 7     //粘贴文本时内存占用系数

class Utils
{
public:
    static QString getQrcPath(const QString &imageName);
    static QString getQssPath(const QString &qssName);
    static QSize getRenderSize(int fontSize, const QString &string);
    static void setFontSize(QPainter &painter, int textSize);
    static void applyQss(QWidget *widget, const QString &qssName);
    static QString getKeyshortcut(QKeyEvent *keyEvent);
    static QString getKeyshortcutFromKeymap(Settings* settings, const QString &keyCategory, const QString &keyName);
    static bool fileExists(const QString &path);
    static bool fileIsWritable(const QString &path);
    static bool fileIsHome(const QString &path);
    static void passInputEvent(int wid);
    static QPixmap dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset);
    static QImage dropShadow(const QPixmap &px, qreal radius, const QColor &color);
    static QByteArray detectEncode(const QByteArray &data, const QString &fileName = QString());
    static QByteArray getEncode(const QByteArray &data);
    static qreal easeInOut(qreal x);
    static qreal easeInQuad(qreal x);
    static qreal easeInQuint(qreal x);
    static qreal easeOutQuad(qreal x);
    static qreal easeOutQuint(qreal x);
    static QVariantMap getThemeMapFromPath(const QString &filepath);
    static bool isMimeTypeSupport(const QString &filepath);
    static bool isDraftFile(const QString &filepath);
    // 返回文件是否为备份文件
    static bool isBackupFile(const QString &filepath);
    static QStringList cleanPath(const QStringList &filePaths);
    static const QStringList getEncodeList();
    static QPixmap renderSVG(const QString &filePath, const QSize &size ,bool bIsScale = true);
    static QList<QColor> getHiglightColorList();
    /*******************************************************************************
     1. @函数:    clearChildrenFocus
     2. @作者:    n014361 王培利
     3. @日期:    2020-05-08
     4. @说明:    清空控件内部所有子控件的焦点获取
            安全考虑，不要全局使用．仅在个别控件中使用
    *******************************************************************************/
    static void clearChildrenFocus(QObject *objParent);
    //清除　控件及子控件所以焦点　梁卫东　２０２０－０９－１４　１０：３４：１９
    static void clearChildrenFoucusEx(QWidget* pWidget);
    //设置所有控件焦点 梁卫东　２０２０－０９－１５　１７：５５：１８
    static void setChildrenFocus(QWidget* pWidget,Qt::FocusPolicy policy = Qt::StrongFocus);
    //根据指定名称获取进程数量 秦浩玲　2021-01-26
    static int getProcessCountByName(const char *pstrName);
    //批量结束指定名称的进程 秦浩玲　2021-01-26
    static void killProcessByName(const char *pstrName);
    //计算字符串MD5哈希值 秦浩玲　2021-01-28
    static QString getStringMD5Hash(const QString &input);
    //通过dbus接口从任务栏激活窗口 add by guoshaoyu 2021-04-07
    static bool activeWindowFromDock(quintptr winId);

    //判断是否共享文件夹且只读
    static bool isShareDirAndReadOnly(const QString& filePath);


    static float codecConfidenceForData(const QTextCodec *codec, const QByteArray &data, const QLocale::Country &country);

    //return system language
    static QString getSystemLan();

    static bool isWayland();

private:
    static QString m_systemLanguage;
};
