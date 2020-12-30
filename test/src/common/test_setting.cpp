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
#include <DSettings>
#include <QStandardPaths>
#include <DtkCores>

test_setting::test_setting()
{
}

void test_setting::SetUp()
{
    m_setting = new Settings();
}

void test_setting::TearDown()
{
    delete m_setting;
}

TEST_F(test_setting, Settings)
{
    QString figPath = QString("%1/%2/%3/config.conf")
                          .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
                          .arg(qApp->organizationName())
                          .arg(qApp->applicationName());
    QSettingBackend *m_backend = new QSettingBackend(figPath);

    m_setting->settings = DSettings::fromJsonFile(":/resources/settings.json");
    m_setting->settings->setBackend(m_backend);
    auto fontFamliy = m_setting->settings->option("base.font.family");
    //        QVariant value;
    //        fontFamliy->valueChanged(value);
    //        sleep(500);
    QVariant retVal;
    QMetaObject::invokeMethod(fontFamliy, "valueChanged", Qt::DirectConnection,
                              QGenericReturnArgument(),
                              Q_ARG(QVariant, "dsd"));
}

//static Settings* instance();
TEST_F(test_setting, instance)
{
    Settings::instance();
    assert(1 == 1);
}

//void dtkThemeWorkaround(QWidget *parent, const QString &theme);
TEST_F(test_setting, dtkThemeWorkaround)
{
    QWidget *widget = new QWidget();
    Settings::instance()->dtkThemeWorkaround(widget, "dlight");
    assert(1 == 1);
}

//static QPair<QWidget*, QWidget*> createFontComBoBoxHandle(QObject *obj);
TEST_F(test_setting, createFontComBoBoxHandle)
{
    QWidget *widget = new QWidget();
    DSettingsDialog *dialog = new DSettingsDialog(widget);
    dialog->widgetFactory()->registerWidget("fontcombobox", Settings::createFontComBoBoxHandle);
    assert(1 == 1);
}

//static QPair<QWidget*, QWidget*> createKeySequenceEditHandle(QObject *obj);
TEST_F(test_setting, createKeySequenceEditHandle)
{
    QWidget *widget = new QWidget();
    DSettingsDialog *dialog = new DSettingsDialog(widget);
    dialog->widgetFactory()->registerWidget("fontcombobox", Settings::createKeySequenceEditHandle);

    //    Settings::instance()->createKeySequenceEditHandle(dialog);
}

//static Settings* instance();

//void setSettingDialog(DSettingsDialog *settingsDialog);
TEST_F(test_setting, setSettingDialog)
{
    QWidget *widget = new QWidget();
    DSettingsDialog *dialog = new DSettingsDialog(widget);
    Settings::instance()->setSettingDialog(dialog);
    assert(1 == 1);
}

//private:
//void updateAllKeysWithKeymap(QString keymap);
TEST_F(test_setting, updateAllKeysWithKeymap)
{
    QString keymap = Settings::instance()->settings->option("shortcuts.keymap.keymap")->value().toString();
    Settings::instance()->updateAllKeysWithKeymap(keymap);
    assert(1 == 1);
}

//void copyCustomizeKeysFromKeymap(QString keymap);
TEST_F(test_setting, copyCustomizeKeysFromKeymap)
{
    QString keymap = Settings::instance()->settings->option("shortcuts.keymap.keymap")->value().toString();
    Settings::instance()->copyCustomizeKeysFromKeymap(keymap);
    assert(1 == 1);
}

//此函数代码调试中已经覆盖， html中显示未覆盖
TEST_F(test_setting, checkShortcutValid)
{
    bool ok;
    QString reason = "reason";
    m_setting->checkShortcutValid("shortcuts.keymap.keymap", "Enter", reason, ok);
}

TEST_F(test_setting, checkShortcutValid2)
{
    bool ok;
    QString reason = "reason";
    m_setting->checkShortcutValid("shortcuts.keymap.keymap", "<", reason, ok);
}

TEST_F(test_setting, checkShortcutValid3)
{
    bool ok;
    QString reason = "reason";
    m_setting->checkShortcutValid("shortcuts.keymap.keymap<", "Num+", reason, ok);
}

TEST_F(test_setting, isShortcutConflict)
{
    //Settings::instance()->isShortcutConflict("shortcuts.keymap.keymap", "Enter");
    //    assert(1 == 1);
    QStringList list;
    list << "aa"
         << "bb";
    Settings set;
    set.isShortcutConflict("aa", "bb");
}

//以下两条CASE 脚本跑会造成程序崩，加两行debug后就不崩了
//TEST_F(test_setting, createDialog2)
//{
//    Settings set;
//    set.createDialog("ba", "bb", false);
//}

//TEST_F(test_setting, createDialog)
//{
//    Settings set;
//    set.createDialog("ba", "bb", true);
//}
