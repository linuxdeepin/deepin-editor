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
    // 回退默认值按深浅色区分：浅色黑字，深色白字（黑字深底不可读）
    static QString serialize(const QVariantMap &themeMap)
    {
        QColor bg = backgroundColor(themeMap);
        QColor fg = foregroundColor(themeMap);
        const bool dark = isDark(themeMap);

        QJsonObject obj;
        obj["--bg"] = bg.isValid() ? bg.name()
            : (dark ? QStringLiteral("#1f1f1f") : QStringLiteral("#ffffff"));
        obj["--fg"] = fg.isValid() ? fg.name()
            : (dark ? QStringLiteral("#ffffff") : QStringLiteral("#1f1f1f"));
        obj["--fg-secondary"] = fg.isValid()
            ? (dark ? fg.darker(150).name() : fg.darker(180).name())
            : (dark ? QStringLiteral("#a6a6a6") : QStringLiteral("#595959"));
        // 代码块前景：跟随正文前景色（缺省时 theme.css 的浅色默认值会在深色主题下不可读）
        obj["--code-fg"] = fg.isValid() ? fg.name()
            : (dark ? QStringLiteral("#ffffff") : QStringLiteral("#1f1f1f"));
        // 代码块背景：浅色略深于正文背景，深色略浅（darker 会让深底上的色块/边框几乎不可见）
        obj["--code-bg"] = bg.isValid()
            ? (dark ? bg.lighter(130).name() : bg.darker(110).name())
            : (dark ? QStringLiteral("#2b2b2b") : QStringLiteral("#f5f5f5"));
        obj["--border"] = bg.isValid()
            ? (dark ? bg.lighter(150).name() : bg.darker(130).name())
            : (dark ? QStringLiteral("#3a3a3a") : QStringLiteral("#e0e0e0"));

        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

private:
    static QColor backgroundColor(const QVariantMap &themeMap)
    {
        auto ec = themeMap.value(QStringLiteral("editor-colors")).toMap();
        return QColor(ec.value(QStringLiteral("background-color")).toString());
    }
    // 前景色：优先 editor-colors.text-color；主题文件中该键不存在时，
    // 回退 text-styles.Normal.text-color（deepin/deepin_dark 等主题的文本色实际存放位置）
    static QColor foregroundColor(const QVariantMap &themeMap)
    {
        auto ec = themeMap.value(QStringLiteral("editor-colors")).toMap();
        QColor fg = QColor(ec.value(QStringLiteral("text-color")).toString());
        if (fg.isValid())
            return fg;

        auto ts = themeMap.value(QStringLiteral("text-styles")).toMap();
        auto normal = ts.value(QStringLiteral("Normal")).toMap();
        return QColor(normal.value(QStringLiteral("text-color")).toString());
    }
};

#endif // THEMESERIALIZER_H
