/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#include "themelistmodel.h"
#include <QFileInfoList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>

ThemeListModel::ThemeListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    initThemes();
}

ThemeListModel::~ThemeListModel()
{
}

QVariantMap ThemeListModel::getMap(const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "Failed to open " << filePath;
    }

    QTextStream in(&file);
    const QString jsonStr = in.readAll();
    file.close();

    QByteArray jsonBytes = jsonStr.toLocal8Bit();
    QJsonDocument document = QJsonDocument::fromJson(jsonBytes);
    QJsonObject object = document.object();

    return object.toVariantMap();
}

void ThemeListModel::setFrameColor(const QString &selectedColor, const QString &normalColor)
{
    m_frameSelectedColor = selectedColor;
    m_frameNormalColor = normalColor;
}

int ThemeListModel::rowCount(const QModelIndex &parent) const
{
    return m_themes.size();
}

QVariant ThemeListModel::data(const QModelIndex &index, int role) const
{
    const int r = index.row();

    const QString &name = m_themes.at(r).first;
    const QString &path = m_themes.at(r).second;

    switch (role) {
    case ThemeName:
        return name;

    case ThemePath:
        return m_themes.at(r).second;

    case FrameNormalColor:
        return m_frameNormalColor;

    case FrameSelectedColor:
        return m_frameSelectedColor;
    }

    return QVariant();
}

void ThemeListModel::initThemes()
{
    QFileInfoList infoList = QDir("/usr/share/deepin-editor/themes").entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    for (QFileInfo info : infoList) {
        QVariantMap jsonMap = getMap(info.filePath());
        QPair<QString, QString> pair;
        pair.first = jsonMap["metadata"].toMap()["name"].toString();
        pair.second = info.filePath();

        m_themes << pair;
    }
}
