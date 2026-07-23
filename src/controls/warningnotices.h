// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    void clearBtn();
    void setEditAnywayBtn();

signals:
    void reloadBtnClicked();
    void saveAsBtnClicked();
    void closeBtnClicked();
    void editAnywayBtnClicked();

public slots:
    void slotreloadBtnClicked();
    void slotsaveAsBtnClicked();
    void slotEditAnywayBtnClicked();

private:
    QPushButton *m_reloadBtn;
    QPushButton *m_saveAsBtn;
    QPushButton *m_editAnywayBtn;
    QHBoxLayout *m_pLayout;
};

#endif // WARNINGNOTICES_H
