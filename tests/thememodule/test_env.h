// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// thememodule 测试共享环境：
// - 一个测试二进制承载 4 个套件，QApplication(offscreen) 全进程仅构造一次
// - 主题目录扫描隔离：拦截 QDir::entryInfoList + Utils::getThemeMapFromPath，
//   以受控假数据驱动 ThemeListModel::initThemes，不读写真实文件系统

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QPair>
#include <QString>
#include <QVariantMap>

#include "stubext.h"
#include "utils.h"

// 共享 QApplication（offscreen）：4 个套件复用同一实例，进程退出时统一释放
inline QApplication *thememoduleEnsureApp()
{
    if (QApplication::instance() == nullptr) {
        static int argc = 1;
        static char appName[] = "test_thememodule";
        static char *argv[] = { appName, nullptr };
        (void)new QApplication(argc, argv);
    }
    return static_cast<QApplication *>(QApplication::instance());
}

// 一份受控主题数据：first = 主题文件路径，second = getThemeMapFromPath 应返回的 JSON 映射
using FakeThemeSource = QList<QPair<QString, QVariantMap>>;

// 构造标准主题映射（name / background-color / text-styles 全套颜色）
inline QVariantMap makeThemeMap(const QString &name, const QString &bgColorHex,
                                const QString &textColorHex = "#101010")
{
    QVariantMap textStyles;
    for (const char *key : { "Import", "String", "BuiltIn", "Keyword",
                             "Comment", "Function", "Normal", "Others" }) {
        QVariantMap style;
        style.insert("text-color", textColorHex);
        textStyles.insert(QLatin1String(key), style);
    }

    QVariantMap editorColors;
    editorColors.insert("background-color", bgColorHex);

    QVariantMap metadata;
    metadata.insert("name", name);

    QVariantMap map;
    map.insert("text-styles", textStyles);
    map.insert("editor-colors", editorColors);
    map.insert("metadata", metadata);
    return map;
}

// 拦截主题数据源（必须在 ThemeListModel 构造前安装）：
// 1) QDir::entryInfoList(QDir::Filters, QDir::SortFlags) const → 返回假主题文件清单
// 2) Utils::getThemeMapFromPath → 按路径查表返回假 JSON 映射（未登记路径返回空映射）
inline void installThemeSourceStubs(stub_ext::StubExt &stub, const FakeThemeSource &themes)
{
    stub.set_lamda(
        static_cast<QFileInfoList (QDir::*)(QDir::Filters, QDir::SortFlags) const>(
            &QDir::entryInfoList),
        [themes](QDir *, QDir::Filters, QDir::SortFlags) -> QFileInfoList {
            QFileInfoList list;
            for (const auto &theme : themes)
                list << QFileInfo(theme.first);
            return list;
        });

    stub.set_lamda(&Utils::getThemeMapFromPath,
                   [themes](const QString &filepath) -> QVariantMap {
                       for (const auto &theme : themes)
                           if (theme.first == filepath)
                               return theme.second;
                       return QVariantMap();
                   });
}
