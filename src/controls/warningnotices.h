/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     guoshaoyu <guoshaoyu@uniontech.com>
*
* Maintainer: guoshaoyu <guoshaoyu@uniontech.com>
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

#ifndef WARNINGNOTICES_H
#define WARNINGNOTICES_H

#include <QApplication>
#include <DFloatingMessage>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFont>

DWIDGET_USE_NAMESPACE

class WarningNotices : public DFloatingMessage
{
    Q_OBJECT
public:
    explicit WarningNotices(MessageType notifyType = MessageType::ResidentType ,QWidget *parent = nullptr);
    ~WarningNotices();

    void setReloadBtn();
    void setSaveAsBtn();

signals:
    void reloadBtnClicked();
    void saveAsBtnClicked();
    void closeBtnClicked();

public slots:
    void slotreloadBtnClicked();
    void slotsaveAsBtnClicked();

private:
    QPushButton *m_reloadBtn;
    QPushButton *m_saveAsBtn;
    QHBoxLayout *m_pLayout;
};

#endif // WARNINGNOTICES_H
