// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "themelistmodel.h"
#include "../common/utils.h"
#include <QFileInfoList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>

ThemeListModel::ThemeListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qCDebug(appLog) << "ThemeListModel constructor";
    initThemes();
}

ThemeListModel::~ThemeListModel()
{
    qCDebug(appLog) << "ThemeListModel destructor";
}

void ThemeListModel::setFrameColor(const QString &selectedColor, const QString &normalColor)
{
    qCInfo(appLog) << "Setting frame colors - selected:" << selectedColor << "normal:" << normalColor;
    m_frameSelectedColor = selectedColor;
    m_frameNormalColor = normalColor;
}

void ThemeListModel::setSelection(const QString &path)
{
    qCDebug(appLog) << "Setting selection for theme path:" << path;
    for (auto pair : m_themes) {
        if (path == pair.second) {
            const int row = m_themes.indexOf(pair);
            const QModelIndex &idx = QAbstractListModel::index(row, 0);
            emit requestCurrentIndex(idx);
            qCDebug(appLog) << "Break, Selection set for theme path:" << path;
            break;
        }
    }
}

int ThemeListModel::rowCount(const QModelIndex &parent) const
{
    return m_themes.size();
}

QVariant ThemeListModel::data(const QModelIndex &index, int role) const
{
    // 按 QAbstractItemModel 契约校验索引有效性，无效索引返回 QVariant()
    if (!index.isValid() || index.row() < 0 || index.row() >= m_themes.size()) {
        qCWarning(appLog) << "Invalid index for row:" << index.row();
        return QVariant();
    }
    qCDebug(appLog) << "Getting data for row:" << index.row() << "role:" << role;
    const int r = index.row();

    const QString &name = m_themes.at(r).first;
    const QString &path = m_themes.at(r).second;

    switch (role) {
    case ThemeName:
        qCDebug(appLog) << "Theme name:" << name;
        return name;
    case ThemePath:
        qCDebug(appLog) << "Theme path:" << path;
        return path;
    case FrameNormalColor:
        qCDebug(appLog) << "Frame normal color:" << m_frameNormalColor;
        return m_frameNormalColor;
    case FrameSelectedColor:
        qCDebug(appLog) << "Frame selected color:" << m_frameSelectedColor;
        return m_frameSelectedColor;
    }

    qCDebug(appLog) << "No data for role:" << role;
    return QVariant();
}

void ThemeListModel::initThemes()
{
    qCDebug(appLog) << "Initializing theme list";
    QFileInfoList infoList = QDir(QString("%1share/deepin-editor/themes").arg(LINGLONG_PREFIX)).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    for (QFileInfo info : infoList) {
        QVariantMap jsonMap = Utils::getThemeMapFromPath(info.filePath());
        QPair<QString, QString> pair;
        pair.first = jsonMap["metadata"].toMap()["name"].toString();
        pair.second = info.filePath();

        qCDebug(appLog) << "Found theme:" << pair.first << "at path:" << pair.second;
        m_themes << pair;
    }

    qCDebug(appLog) << "Sorting themes by background lightness";
    std::sort(m_themes.begin(), m_themes.end(),
              [=] (QPair<QString, QString> &a, QPair<QString, QString> &b) {
                  QVariantMap firstMap = Utils::getThemeMapFromPath(a.second);
                  QVariantMap secondMap = Utils::getThemeMapFromPath(b.second);

                  const QString &firstColor = firstMap["editor-colors"].toMap()["background-color"].toString();
                  const QString &secondColor = secondMap["editor-colors"].toMap()["background-color"].toString();

                  return QColor(firstColor).lightness() < QColor(secondColor).lightness();
              });

    qCDebug(appLog) << "Themes:" << m_themes;
}
