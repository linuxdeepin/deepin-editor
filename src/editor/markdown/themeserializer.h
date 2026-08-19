// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef THEMESERIALIZER_H
#define THEMESERIALIZER_H

#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>

//
// ThemeSerializer —— 主题序列化（纯函数，§4.7 / §5'.3）
//
// 把 Utils::getThemeMapFromName 返回的 QVariantMap themeMap 派生为：
//   1. 深浅色判定（背景色 lightness < 128 → 深色）
//   2. CSS 变量 JSON（注入到渲染区 :root，供 theme.css 引用）
//
class ThemeSerializer
{
public:
    // 深浅色判定：themeMap["editor-colors"]["background-color"] 的 lightness < 128 → true
    static bool isDark(const QVariantMap &themeMap)
    {
        QColor bg = backgroundColor(themeMap);
        if (!bg.isValid()) return false;
        return bg.lightness() < 128;
    }

    // 序列化为 CSS 变量 JSON，供 JS applyTheme 注入到 document.documentElement.style
    static QString serialize(const QVariantMap &themeMap)
    {
        QColor bg = backgroundColor(themeMap);
        QColor fg = foregroundColor(themeMap);

        QJsonObject obj;
        obj["--bg"] = bg.isValid() ? bg.name() : QStringLiteral("#ffffff");
        obj["--fg"] = fg.isValid() ? fg.name() : QStringLiteral("#1f1f1f");
        obj["--fg-secondary"] = fg.isValid() ? fg.darker(180).name() : QStringLiteral("#595959");
        // 代码块前景：跟随正文前景色（缺省时 theme.css 的浅色默认值会在深色主题下不可读）
        obj["--code-fg"] = fg.isValid() ? fg.name() : QStringLiteral("#1f1f1f");
        // 代码块背景：略深于正文背景
        obj["--code-bg"] = bg.isValid() ? bg.darker(110).name() : QStringLiteral("#f5f5f5");
        obj["--border"] = bg.isValid() ? bg.darker(130).name() : QStringLiteral("#e0e0e0");

        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

private:
    static QColor backgroundColor(const QVariantMap &themeMap)
    {
        auto ec = themeMap.value(QStringLiteral("editor-colors")).toMap();
        return QColor(ec.value(QStringLiteral("background-color")).toString());
    }
    static QColor foregroundColor(const QVariantMap &themeMap)
    {
        auto ec = themeMap.value(QStringLiteral("editor-colors")).toMap();
        return QColor(ec.value(QStringLiteral("text-color")).toString());
    }
};

#endif // THEMESERIALIZER_H
