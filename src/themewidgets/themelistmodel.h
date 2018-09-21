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

#ifndef THEMELISTMODEL_H
#define THEMELISTMODEL_H

#include <QAbstractListModel>

class ThemeListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum ThemeRole {
        ThemeName = Qt::DisplayRole,
        ThemePath = Qt::UserRole,
        ThemeMap,
        FrameNormalColor,
        FrameSelectedColor
    };

    ThemeListModel(QObject *parent = nullptr);
    ~ThemeListModel();

    void setFrameColor(const QString &selectedColor, const QString &normalColor);
    void setSelection(const QString &path);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

signals:
    void requestCurrentIndex(const QModelIndex &idx);

private:
    void initThemes();

private:
    // first is theme name.
    // second is theme file path.
    QList<QPair<QString, QString>> m_themes;

    QString m_frameNormalColor;
    QString m_frameSelectedColor;
};

#endif
