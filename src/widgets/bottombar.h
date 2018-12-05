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

#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>
#include <QLabel>
#include <QComboBox>

class EditWrapper;
class BottomBar : public QWidget
{
    Q_OBJECT

public:
    BottomBar(QWidget *parent = nullptr);
    ~BottomBar();

    void updatePosition(int row, int column);
    void setEncodeName(const QString &name);
    void setPalette(const QPalette &palette);

private:
    void handleEncodeChanged(const QString &name);

protected:
    void paintEvent(QPaintEvent *);

private:
    EditWrapper *m_wrapper;
    QLabel *m_positionLabel;
    QComboBox *m_encodeComboBox;
    QString m_rowStr;
    QString m_columnStr;
};

#endif
