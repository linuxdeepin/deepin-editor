/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     liangweidong <liangweidong@uniontech.com>
*
* Maintainer: liangweidong <liangweidong@uniontech.com>
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
#ifndef EDITORAPPLICATION_H
#define EDITORAPPLICATION_H
#include "environments.h"

#include <DApplication>
#include <DPushButton>
#include <QEvent>
#include <QObject>
#include <DSettingsDialog>
#include <DDialog>
#include <QKeyEvent>
#include <QObject>
#include <QTimer>
#include <QSharedMemory>
#include "startmanager.h"

DWIDGET_USE_NAMESPACE

class EditorApplication:public DApplication
{
    Q_OBJECT
public:
    EditorApplication(int &argc, char *argv[]);
    ~EditorApplication() override;
protected:
    void handleQuitAction()override;
    bool notify(QObject *object, QEvent *event) override;
private:
    // 模拟键盘space键按压
    void pressSpace(DPushButton *pushButton);
};

#endif // EDITORAPPLICATION_H
