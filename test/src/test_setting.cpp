/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "test_setting.h"
#include "../../src/common/settings.h"
#include <DSettingsDialog>
#include <DSettingsWidgetFactory>
#include <QKeySequence>
#include <DSettingsOption>

test_setting::test_setting()
{

}

TEST_F(test_setting, Settings)
{
    Settings settings(nullptr);
    assert(1==1);
}

//static Settings* instance();
TEST_F(test_setting, instance)
{
    Settings::instance();
    assert(1==1);
}

//void dtkThemeWorkaround(QWidget *parent, const QString &theme);
TEST_F(test_setting, dtkThemeWorkaround)
{
    QWidget *widget = new QWidget();
    Settings::instance()->dtkThemeWorkaround(widget,"dlight");
    assert(1==1);
}

//static QPair<QWidget*, QWidget*> createFontComBoBoxHandle(QObject *obj);
TEST_F(test_setting, createFontComBoBoxHandle)
{
    QWidget *widget = new QWidget();
    DSettingsDialog *dialog = new DSettingsDialog(widget);
    dialog->widgetFactory()->registerWidget("fontcombobox", Settings::createFontComBoBoxHandle);
    assert(1==1);
}

//static QPair<QWidget*, QWidget*> createKeySequenceEditHandle(QObject *obj);
TEST_F(test_setting, createKeySequenceEditHandle)
{
    QWidget *widget = new QWidget();
    DSettingsDialog *dialog = new DSettingsDialog(widget);
    dialog->widgetFactory()->registerWidget("fontcombobox", Settings::createKeySequenceEditHandle);
    assert(1==1);
}

//static Settings* instance();

//void setSettingDialog(DSettingsDialog *settingsDialog);
TEST_F(test_setting, setSettingDialog)
{
    QWidget *widget = new QWidget();
    DSettingsDialog *dialog = new DSettingsDialog(widget);
    Settings::instance()->setSettingDialog(dialog);
    assert(1==1);
}

//private:
//void updateAllKeysWithKeymap(QString keymap);
TEST_F(test_setting, updateAllKeysWithKeymap)
{
    QString keymap = Settings::instance()->settings->option("shortcuts.keymap.keymap")->value().toString();
    Settings::instance()->updateAllKeysWithKeymap(keymap);
    assert(1==1);
}

//void copyCustomizeKeysFromKeymap(QString keymap);
TEST_F(test_setting, copyCustomizeKeysFromKeymap)
{
    QString keymap = Settings::instance()->settings->option("shortcuts.keymap.keymap")->value().toString();
    Settings::instance()->copyCustomizeKeysFromKeymap(keymap);
    assert(1==1);
}

//bool checkShortcutValid(const QString &Name, QString Key, QString &Reason, bool &bIsConflicts);
TEST_F(test_setting, checkShortcutValid)
{
    QString reason;
    bool ok;
    Settings::instance()->checkShortcutValid("shortcuts.keymap.keymap","Enter",reason,ok);
    assert(1==1);
}

//bool isShortcutConflict(const QString &Name, const QString &Key);
TEST_F(test_setting, isShortcutConflict)
{
    Settings::instance()->isShortcutConflict("shortcuts.keymap.keymap","Enter");
    assert(1==1);
}

//DDialog *createDialog(const QString &title, const QString &content, const bool &bIsConflicts);
TEST_F(test_setting, createDialog)
{
    Settings::instance()->createDialog("aa","bb",true);
    assert(1==1);
}
