// SPDX-FileCopyrightText: 2011-2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"
#include "settings.h"

#include "dthememanager.h"
#include "../controls/fontitemdelegate.h"
#include "../widgets/pathsettintwgt.h"
#include <DSettings>
#include <DSettingsGroup>
#include <DSettingsWidgetFactory>
#include <DSettingsOption>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <DGuiApplicationHelper>

Settings *Settings::s_pSetting = nullptr;

CustemBackend::CustemBackend(const QString &filepath, QObject *parent)
    : DSettingsBackend (parent),
      m_settings (new QSettings(filepath, QSettings::IniFormat))
{
    qDebug() << "CustemBackend::CustemBackend" << filepath;
}

void CustemBackend::doSync()
{
    qDebug() << "Entering CustemBackend::doSync";
    m_settings->sync();
    qDebug() << "Leaving CustemBackend::doSync";
}

void CustemBackend::doSetOption(const QString &key, const QVariant &value)
{
    qDebug() << "CustemBackend::doSetOption" << key << value;
    /*
     * 将配置值写入config配置文件
     * 为了规避数据写入重复或者错乱，配置写入之前上锁，写入结束后开锁
     */
    m_writeLock.lock();
    qDebug() << "Write lock acquired";
    m_settings->setValue(key, value);
    m_settings->sync();
    m_writeLock.unlock();
    qDebug() << "Write lock released. Leaving CustemBackend::doSetOption";
}

QStringList CustemBackend::keys() const
{
    qDebug() << "Entering CustemBackend::keys";
    QStringList keyList = m_settings->allKeys();
    qDebug() << "Leaving CustemBackend::keys, returning:" << keyList;

    return keyList;
}

QVariant CustemBackend::getOption(const QString &key) const
{
    qDebug() << "Entering CustemBackend::getOption, key:" << key;
    QVariant value = m_settings->value(key);
    qDebug() << "Leaving CustemBackend::getOption, returning value:" << value;
    return value;
}

CustemBackend::~CustemBackend()
{
    qDebug() << "Entering CustemBackend::~CustemBackend";
    delete m_settings;
    qDebug() << "Leaving CustemBackend::~CustemBackend";
}

Settings::Settings(QWidget *parent)
    : QObject(parent)
{
    qDebug() << "Initializing Settings instance";
    
    QString strConfigPath = QString("%1/%2/%3/config.conf")
                            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
                            .arg(qApp->organizationName())
                            .arg(qApp->applicationName());
    qDebug() << "Config file path:" << strConfigPath;

    removeLockFiles();
    m_backend = new QSettingBackend(strConfigPath);
    qDebug() << "Created QSettingBackend for config file";

    settings = DSettings::fromJsonFile(":/resources/settings.json");
    if (!settings) {
        qWarning() << "Failed to load settings from JSON file";
        return;
    }
    settings->setBackend(m_backend);
    qInfo() << "Settings initialized successfully";

    qDebug() << "Connecting settings signals to slots";
    auto fontFamliy = settings->option("base.font.family");
    connect(fontFamliy, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigAdjustFont);

    auto fontSize = settings->option("base.font.size");
    connect(fontSize, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigAdjustFontSize);

    auto wordWrap = settings->option("base.font.wordwrap");
    connect(wordWrap, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigAdjustWordWrap);

    auto showLineNumber = settings->option("base.font.showlinenumber");
    connect(showLineNumber, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigSetLineNumberShow);

    auto bookmark = settings->option("base.font.showbookmark");
    connect(bookmark, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigAdjustBookmark);

    auto codeFlod = settings->option("base.font.codeflod");
    connect(codeFlod, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigShowCodeFlodFlag);

    //添加显示空白符　梁卫东
    auto blankCharacter = settings->option("base.font.showblankcharacter");
    connect(blankCharacter, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigShowBlankCharacter);

    //hightlightcurrentline
    auto hightlightCurrentLine = settings->option("base.font.hightlightcurrentline");
    connect(hightlightCurrentLine, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigHightLightCurrentLine);

    /* 设置页面主题变更信息监听，当前暂时无用，暂且做屏蔽处理
    auto theme = settings->option("advance.editor.theme");
    connect(theme, &Dtk::Core::DSettingsOption::valueChanged, this, [ = ](QVariant value) {
        //emit themeChanged(value.toString());
    }); */

    auto tabSpaceNumber = settings->option("advance.editor.tabspacenumber");
    connect(tabSpaceNumber, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotsigAdjustTabSpaceNumber);

    auto keymap = settings->option("shortcuts.keymap.keymap");
    QMap<QString, QVariant> keymapMap;
    keymapMap.insert("keys", QStringList() << "standard" << "emacs" << "customize");
    keymapMap.insert("values", QStringList() << tr("Standard") << "Emacs" << tr("Customize"));
    keymap->setData("items", keymapMap);
    connect(keymap, &Dtk::Core::DSettingsOption::valueChanged, this, &Settings::slotupdateAllKeysWithKeymap);

    //only used by new window
    auto windowState = settings->option("advance.window.windowstate");
    #if 0
    connect(windowState, &Dtk::Core::DSettingsOption::valueChanged, this, [=] (QVariant value) {
        emit sigChangeWindowSize(value.toString());
    });
    #endif
    QMap<QString, QVariant> windowStateMap;
    windowStateMap.insert("keys", QStringList() << "window_normal" << "window_maximum" << "fullscreen");
    windowStateMap.insert("values", QStringList() << tr("Normal") << tr("Maximum") << tr("Fullscreen"));
    windowState->setData("items", windowStateMap);

    connect(settings, &Dtk::Core::DSettings::valueChanged, this, &Settings::slotCustomshortcut);
    qDebug() << "Leaving Settings::Settings";
}

void Settings::setSavePath(int id,const QString& path)
{
    qDebug() << "Entering setSavePath for id:" << id << "path:" << path;
    switch (id) {
    case PathSettingWgt::LastOptBox:{
        qDebug() << "Setting LastOptBox path";
        auto opt = settings->option("advance.open_save_setting.open_save_lastopt_path");
        opt->setValue(path);
        break;
    }
    case PathSettingWgt::CurFileBox:{
        qDebug() << "Setting CurFileBox path";
        auto opt = settings->option("advance.open_save_setting.open_save_curfile_path");
        opt->setValue(path);
        break;
    }
    case PathSettingWgt::CustomBox:{
        qDebug() << "Setting CustomBox path";
        auto opt = settings->option("advance.open_save_setting.open_save_custom_path");
        opt->setValue(path);
        break;
    }
    default:
        qWarning() << "Unknown id in setSavePath:" << id;
        break;
    }
    qDebug() << "Leaving setSavePath";
}


QString Settings::getSavePath(int id)
{
    qDebug() << "Entering getSavePath for id:" << id;
    QString path;
    switch (id) {
    case PathSettingWgt::LastOptBox:{
        qDebug() << "Getting LastOptBox path";
        path = settings->option("advance.open_save_setting.open_save_lastopt_path")->value().toString();
        break;
    }
    case PathSettingWgt::CurFileBox:{
        qDebug() << "Getting CurFileBox path";
        path = settings->option("advance.open_save_setting.open_save_curfile_path")->value().toString();
        break;
    }
    case PathSettingWgt::CustomBox:{
        qDebug() << "Getting CustomBox path";
        path = settings->option("advance.open_save_setting.open_save_custom_path")->value().toString();
        break;
    }
    default:
        qWarning() << "Unknown id in getSavePath:" << id;
        break;
    }

    qDebug() << "Leaving getSavePath, returning:" << path;
    return path;
}

void Settings::setSavePathId(int id)
{
    qDebug() << "Entering setSavePathId, id:" << id;
    auto opt = settings->option("advance.open_save_setting.savingpathwgt");
    opt->setValue(QString::number(id));
    qDebug() << "Leaving setSavePathId. Save path ID set to:" << opt->value();
}
int Settings::getSavePathId()
{
   qDebug() << "Entering getSavePathId";
   int id = settings->option("advance.open_save_setting.savingpathwgt")->value().toInt();
   qDebug() << "Leaving getSavePathId, returning:" << id;
   return id;

}
Settings::~Settings()
{
    qDebug() << "Destroying Settings instance";
    if (m_backend != nullptr) {
        qDebug() << "Cleaning up settings backend";
        delete m_backend;
        m_backend = nullptr;
    }
    qDebug() << "Settings instance destroyed";
}

void Settings::setSettingDialog(DSettingsDialog *settingsDialog)
{
    qDebug() << "Setting dialog instance";
    m_pSettingsDialog = settingsDialog;
}

// This function is workaround, it will remove after DTK fixed SettingDialog theme bug.
void Settings::dtkThemeWorkaround(QWidget *parent, const QString &theme)
{
    qDebug() << "Entering dtkThemeWorkaround, theme:" << theme;
    parent->setStyle(QStyleFactory::create(theme));
    qDebug() << "Theme applied to parent widget:" << parent->objectName();

    for (auto obj : parent->children()) {
        auto w = qobject_cast<QWidget *>(obj);
        if (!w) {
            qDebug() << "w is null";
            continue;
        }
        qDebug() << "w is not null";
        dtkThemeWorkaround(w, theme);
    }
    qDebug() << "Leaving dtkThemeWorkaround for parent:" << parent->objectName();
}

QPair<QWidget *, QWidget *> Settings::createFontComBoBoxHandle(QObject *obj)
{
    qDebug() << "Entering createFontComBoBoxHandle";
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);

    QComboBox *comboBox = new QComboBox;
    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, comboBox);

    QFontDatabase fontDatabase;
    comboBox->addItems(fontDatabase.families());
    // 设置最小宽度，以保持和下方 OptionDSpinBox 一样的长度
    comboBox->setMinimumSize(240, 36);

    if (option->value().toString().isEmpty()) {
        qDebug() << "option->value().toString().isEmpty()";
        option->setValue(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
    }

    // init.
    comboBox->setCurrentText(option->value().toString());

    connect(option, &DSettingsOption::valueChanged, comboBox, [ = ](QVariant var) {
        qDebug() << "Font family value changed to:" << var.toString();
        comboBox->setCurrentText(var.toString());
    });

    option->connect(comboBox, &QComboBox::currentTextChanged, option, [ = ](const QString & text) {
        option->setValue(text);
    });

#ifdef DTKWIDGET_CLASS_DSizeMode
    // 适配紧凑模式，不同布局下调整高度
    const int defaultHeight = 36;
    const int compactHeight = 24;
    comboBox->setFixedHeight(DGuiApplicationHelper::isCompactMode() ? compactHeight : defaultHeight);
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, comboBox, [ = ](){
        comboBox->setFixedHeight(DGuiApplicationHelper::isCompactMode() ? compactHeight : defaultHeight);
    });
#endif

    qDebug() << "Leaving createFontComBoBoxHandle";
    return optionWidget;
}

QWidget* Settings::createSavingPathWgt(QObject* obj)
{
    qDebug() << "Creating saving path widget";
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);
    QString name = option->name();
    PathSettingWgt* pathwgt = new PathSettingWgt;
    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, pathwgt);

    connect(option, &DSettingsOption::valueChanged, pathwgt, [ = ](QVariant var) {
        int id = var.toInt();
        qDebug() << "pathwgt->onSaveIdChanged" << id;
        pathwgt->onSaveIdChanged(id);
    });

    auto custompath = s_pSetting->settings->option("advance.open_save_setting.open_save_custom_path");
    connect(custompath, &Dtk::Core::DSettingsOption::valueChanged, [=](QVariant var){
        //pathwgt->setEditText(var.toString());
    });

    qDebug() << "Leaving createSavingPathWgt";
    return optionWidget.second;
}

QPair<QWidget *, QWidget *> Settings::createKeySequenceEditHandle(QObject *obj)
{
    qDebug() << "Entering createKeySequenceEditHandle";
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);
    KeySequenceEdit *shortCutLineEdit = new KeySequenceEdit(option);

    shortCutLineEdit->ShortcutDirection(Qt::AlignLeft);
    shortCutLineEdit->setFocusPolicy(Qt::StrongFocus);

    // init.
    shortCutLineEdit->setKeySequence(QKeySequence(option->value().toString()));
    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, shortCutLineEdit);
    qDebug() << "shortCutLineEdit->editingFinished";
    option->connect(shortCutLineEdit, &DKeySequenceEdit::editingFinished, [ = ](const QKeySequence & sequence) {
        qDebug() << "shortCutLineEdit->editingFinished" << sequence;
        QString checkName = option->key();
        QString reason;
        bool bIsConflicts = false;
        auto keymap = instance()->settings->option("shortcuts.keymap.keymap");
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap->value().toString());

        // 获取默认设置的组合键序列
        QString defaultKey;
        auto defaultOption = instance()->settings->option(keySplitList.join("."));
        if (defaultOption) {
            qDebug() << "defaultOption" << defaultOption->defaultValue().toString();
            defaultKey = defaultOption->defaultValue().toString();
        }

        // 判断新设置的组合键序列是否允许
        if (!instance()->checkShortcutValid(checkName, sequence.toString(), reason, bIsConflicts, defaultKey)) {
            qDebug() << "checkShortcutValid is false";
            instance()->m_pDialog = instance()->createDialog(reason, "", bIsConflicts);
            instance()->m_pDialog->exec();
            // 恢复组合键序列
            shortCutLineEdit->setKeySequence(instance()->settings->value(keySplitList.join(".")).toString());
            keymap->setValue("emacs");
            keymap->setValue("customize");
            qDebug() << "Leaving createKeySequenceEditHandle";
            return;
        }

        bool bIsCustomize = false;
        QString conflictsKeys;
        QString originalKeys;

        if (keymap->value().toString() != "customize") {
            qDebug() << "keymap->value().toString() != customize";
            instance()->m_bUserChangeKey = true;

            for (auto loopOption : instance()->settings->group("shortcuts.window_keymap_customize")->options()) {
                QStringList loopKeySplitList = loopOption->key().split(".");
                loopKeySplitList[1] = QString("window_keymap_%1").arg(keymap->value().toString());

                if (loopOption->value().toString() == sequence.toString()) {
                    qDebug() << "loopOption->value().toString() == sequence.toString()";
                    if (checkName.contains(loopKeySplitList.last())) {
                        keymap->setValue("customize");
                        //return;
                    } else {
                        bIsConflicts = true;
                        loopKeySplitList[1] = QString("window_keymap_%1").arg("customize");
                        conflictsKeys = loopKeySplitList.join(".");
                    }
                }
            }

            for (auto loopOption : instance()->settings->group("shortcuts.editor_keymap_customize")->options()) {
                QStringList loopKeySplitList = loopOption->key().split(".");
                loopKeySplitList[1] = QString("editor_keymap_%1").arg(keymap->value().toString());

                if (loopOption->value().toString() == sequence.toString()) {
                    qDebug() << "loopOption->value().toString() == sequence.toString()";
                    if (checkName.contains(loopKeySplitList.last())) {
                        keymap->setValue("customize");
                        //return;
                    } else {
                        bIsConflicts = true;
                        loopKeySplitList[1] = QString("editor_keymap_%1").arg("customize");
                        conflictsKeys = loopKeySplitList.join(".");
                    }
                }
            }

            instance()->m_bUserChangeKey = false;
        }  else {
            bIsCustomize = true;
            instance()->m_bUserChangeKey = true;
            for (auto loopOption : instance()->settings->group("shortcuts.window_keymap_customize")->options()) {
                QStringList loopKeySplitList = loopOption->key().split(".");
                loopKeySplitList[1] = QString("window_keymap_%1").arg(keymap->value().toString());

                if (loopOption->value().toString() == sequence.toString()) {
                    qDebug() << "loopOption->value().toString() == sequence.toString()";
                    if (checkName.contains(loopKeySplitList.last())) {
                        qDebug() << "checkName.contains(loopKeySplitList.last())";
                        //return;
                    } else {
                        qDebug() << "loopOption->value().toString() != sequence.toString()";
                        bIsConflicts = true;
                        conflictsKeys = loopKeySplitList.join(".");
                    }
                }
            }

            for (auto loopOption : instance()->settings->group("shortcuts.editor_keymap_customize")->options()) {
                QStringList loopKeySplitList = loopOption->key().split(".");
                loopKeySplitList[1] = QString("editor_keymap_%1").arg(keymap->value().toString());

                if (loopOption->value().toString() == sequence.toString()) {
                    qDebug() << "loopOption->value().toString() == sequence.toString()";
                    if (checkName.contains(loopKeySplitList.last())) {
                        qDebug() << "checkName.contains(loopKeySplitList.last())";
                        //return;
                    } else {
                        qDebug() << "loopOption->value().toString() != sequence.toString()";
                        bIsConflicts = true;
                        conflictsKeys = loopKeySplitList.join(".");
                    }
                }
            }
            instance()->m_bUserChangeKey = false;
        }

        keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap->value().toString());
        QString qstrSequence = sequence.toString();
        qDebug() << "qstrSequence" << qstrSequence;

        if (sequence.toString().contains("<")) {
            qDebug() << "sequence.toString().contains(<)";
            qstrSequence.replace(qstrSequence.indexOf("<"), 1, "&lt;");
        }

        if (sequence.toString().contains("Return")) {
            qDebug() << "sequence.toString().contains(Return)";
            qstrSequence.replace(qstrSequence.indexOf("Return"), 6, "Enter");
        }

        QString style = QString("<span style=\"color: rgba(255, 87, 54, 1);\">[%1]</span>").arg(qstrSequence);

        if (bIsConflicts || sequence.toString() == "Alt+M") {
            qDebug() << "bIsConflicts" << bIsConflicts;
            if (sequence.toString() == "Alt+M") {
                qDebug() << "sequence.toString() == Alt+M";
                instance()->m_pDialog = instance()->createDialog(tr("This shortcut conflicts with system shortcut %1").arg(style), "", bIsConflicts);
            } else {
                qDebug() << "else Alt+M";
                instance()->m_pDialog = instance()->createDialog(tr("This shortcut conflicts with %1, click on Replace to make this shortcut effective immediately").arg(style), "", bIsConflicts);
            }

            int mode = instance()->m_pDialog->exec();

            // click cancel button.
            if (mode == -1 || mode == 0) {
                shortCutLineEdit->setKeySequence(QKeySequence(instance()->settings->value(keySplitList.join(".")).toString()));
                keymap->setValue("emacs");
                keymap->setValue("customize");
                qDebug() << "click cancel button.";
                return;
            } else {
                qDebug() << "click replace button.";
                keySplitList = option->key().split(".");
                keySplitList[1] = QString("%1_keymap_customize").arg(keySplitList[1]);

                if (!bIsCustomize) {
                    qDebug() << "!bIsCustomize";
                    instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
                    instance()->settings->option(conflictsKeys)->setValue("");
                    shortCutLineEdit->setKeySequence(QKeySequence(instance()->settings->value(checkName).toString()));
                } else {
                    qDebug() << "bIsCustomize";
                    instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
                    instance()->settings->option(conflictsKeys)->setValue("");
                }
                keymap->setValue("emacs");
                keymap->setValue("customize");
                qDebug() << "click replace button.";
                return;
            }
        }

        if (!bIsCustomize) {
            qDebug() << "!bIsCustomize";
            keySplitList = option->key().split(".");
            keySplitList[1] = QString("%1_keymap_customize").arg(keySplitList[1]);
            instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
        } else {
            qDebug() << "bIsCustomize";
            instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
        }
        keymap->setValue("emacs");
        keymap->setValue("customize");
        qDebug() << "click replace button.";
    });

    // 配置修改
    option->connect(option, &DTK_CORE_NAMESPACE::DSettingsOption::valueChanged, shortCutLineEdit, &KeySequenceEdit::slotDSettingsOptionvalueChanged);
    qDebug() << "Leaving createKeySequenceEditHandle";
    return optionWidget;
}

void KeySequenceEdit::slotDSettingsOptionvalueChanged(const QVariant & value)
{
    qDebug() << "KeySequenceEdit::slotDSettingsOptionvalueChanged" << value;
    QString keyseq = value.toString();
    qDebug() << "keyseq" << keyseq;
    if (keyseq.isEmpty()) {
        this->clear();
        qDebug() << "keyseq is empty";
        return;
    }
    qDebug() << "keyseq is not empty";
    this->setKeySequence(QKeySequence(keyseq));
    qDebug() << "Leaving KeySequenceEdit::slotDSettingsOptionvalueChanged";
}

bool KeySequenceEdit::eventFilter(QObject *object, QEvent *event)
{
    qDebug() << "KeySequenceEdit::eventFilter";
    //设置界面　回车键和空格键　切换输入 梁卫东　２０２０－０８－２１　１６：２８：３１
    if (object == this) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            qDebug() << "KeySequenceEdit::eventFilter KeyPress";
            //判断是否包含组合键　梁卫东　２０２０－０９－０２　１５：０３：５６
            Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
            qDebug() << "modifiers" << modifiers;
            bool bHasModifier = (modifiers & Qt::ShiftModifier || modifiers & Qt::ControlModifier ||
                                 modifiers & Qt::AltModifier);
            qDebug() << "bHasModifier" << bHasModifier;

            if (!bHasModifier && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Space)) {
                qDebug() << "keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Space";
                QRect rect = this->rect();
                qDebug() << "rect" << rect;
                QList<QLabel*> childern = findChildren<QLabel*>();
                qDebug() << "childern" << childern;

                for (int i =0; i < childern.size(); i++) {
                    QPoint pos(25,rect.height()/2);

                    QMouseEvent event0(QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    DApplication::sendEvent(childern[i], &event0);
                }

                qDebug() << "KeySequenceEdit::eventFilter KeyPress return true";
                return true;
            }
        }
    }

    return DKeySequenceEdit::eventFilter(object, event);
}
Settings *Settings::instance()
{
    qDebug() << "Accessing Settings singleton instance";
    if (s_pSetting == nullptr) {
        qDebug() << "Creating new Settings instance";
        s_pSetting = new Settings;
    }
    return s_pSetting;
}

void Settings::updateAllKeysWithKeymap(QString keymap)
{
    qDebug() << "Updating all keys with keymap:" << keymap;
    m_bUserChangeKey = true;

    int updatedCount = 0;
    for (auto option : settings->group("shortcuts.window")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap);
        auto opt = settings->option(keySplitList.join("."));
        if (opt) {
            qDebug() << "Updating key" << keySplitList.join(".") << "with value" << opt->value().toString();
            option->setValue(opt->value().toString());
        } else {
            qWarning() << "Unknown shortcut key:" << keySplitList.join(".");
        }
    }

    for (auto option : settings->group("shortcuts.editor")->options()) {
        updatedCount++;
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap);
        auto opt = settings->option(keySplitList.join("."));
        if (opt) {
            qDebug() << "Updating key" << keySplitList.join(".") << "with value" << opt->value().toString();
            option->setValue(opt->value().toString());
        } else {
            qWarning() << "Unknown shortcut key:" << keySplitList.join(".");
        }
    }

    m_bUserChangeKey = false;
    qInfo() << "Updated" << updatedCount << "shortcuts with keymap:" << keymap;
}

void Settings::copyCustomizeKeysFromKeymap(QString keymap)
{
    qDebug() << "Entering copyCustomizeKeysFromKeymap, keymap:" << keymap;

    m_bUserChangeKey = true;

    for (auto option : settings->group("shortcuts.window_keymap_customize")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("window_keymap_%1").arg(keymap);
        option->setValue(settings->option(keySplitList.join("."))->value().toString());
    }

    for (auto option : settings->group("shortcuts.editor_keymap_customize")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("editor_keymap_%1").arg(keymap);
        option->setValue(settings->option(keySplitList.join("."))->value().toString());
    }

    m_bUserChangeKey = false;
    qDebug() << "Leaving copyCustomizeKeysFromKeymap";
}

bool Settings::checkShortcutValid(const QString &Name, QString Key, QString &Reason, bool &bIsConflicts, QString defaultValue)
{
    Q_UNUSED(Name);
    qDebug() << "Entering checkShortcutValid for" << Name << "with key" << Key;

    if (Key.contains("<")) {
        qDebug() << "Key contains <";
        Key.replace(Key.indexOf("<"), 1, "&lt;");
    }

    QString style = QString("<span style=\"color: rgba(255, 87, 54, 1);\">[%1]</span>").arg(Key);
    // 单键
    if (Key.count("+") == 0) {
        qDebug() << "Key count + is 0";
        //F1-F12是允许的，这个正则不够精确，但是没关系。
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QRegExp regexp("^F[0-9]{1,2}$");
#else
        QRegularExpression regexp("^F[0-9]{1,2}$");
#endif
        if (!Key.contains(regexp)) {
            qDebug() << "Key does not contain regexp";
            Reason = tr("The shortcut %1 is invalid, please set another one.").arg(style);
            bIsConflicts = false;
            return  false;
        }
    }
    // 小键盘单键都不允许
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRegExp regexpNum("^Num+.*");
#else
    QRegularExpression regexpNum("^Num+.*");
#endif
    if (Key.contains(regexpNum)) {
        qDebug() << "Key contains regexpNum";
        Reason = tr("The shortcut %1 is invalid, please set another one.").arg(style);
        bIsConflicts = false;
        return  false;
    }

    // 屏蔽Shift组合键处理，但允许恢复为默认组合键序列
    if (Key.contains("Shift")
            && Key != defaultValue) {
        qDebug() << "Key contains Shift and Key is not defaultValue";
        Reason = tr("The shortcut %1 is invalid, please set another one.").arg(style);
        bIsConflicts = false;
        return false;
    }

    qDebug() << "Leaving checkShortcutValid, returning true.";
    return true;
}

bool Settings::isShortcutConflict(const QString &Name, const QString &Key)
{
    qDebug() << "Entering isShortcutConflict for" << Name << "with key" << Key;
    for (QString tmpKey : settings->keys()) {
        if (settings->value(tmpKey).toString() == Key/* && tmpKey.contains("customize")*/) {
            if (Name != tmpKey) {
                return  true;
            }
        }
    }
    qDebug() << "Leaving isShortcutConflict, returning false.";
    return  false;
}

DDialog *Settings::createDialog(const QString &title, const QString &content, const bool &bIsConflicts)
{
    qDebug() << "Entering createDialog for" << title << "with content" << content << "and bIsConflicts" << bIsConflicts;
    DDialog *dialog = new DDialog(title, content, m_pSettingsDialog);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnBottomHint);
    dialog->setIcon(QIcon::fromTheme("deepin-editor"));

    if (bIsConflicts) {
        qDebug() << "Adding Cancel button";
        dialog->addButton(QString(tr("Cancel")), true, DDialog::ButtonNormal);
        dialog->addButton(QString(tr("Replace")), false, DDialog::ButtonRecommend);
    } else {
        qDebug() << "Adding OK button";
        dialog->addButton(QString(tr("OK")), true, DDialog::ButtonRecommend);
    }

    qDebug() << "Leaving createDialog";
    return dialog;
}

//删除config.conf配置文件目录下的.lock文件和.rmlock文件
void Settings::removeLockFiles()
{
    qDebug() << "Removing lock files";
    QString configPath = QString("%1/%2/%3")
            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
            .arg(qApp->organizationName())
            .arg(qApp->applicationName());
    qDebug() << "Config directory:" << configPath;

    QDir dir(configPath);
    if (!dir.exists()) {
        qDebug() << "Config directory does not exist";
        return;
    }

    dir.setFilter(QDir::Files);
    QStringList nameList = dir.entryList();
    int removedCount = 0;
    for (auto name: nameList) {
        if (name.contains(".lock") || name.contains(".rmlock")) {
            qDebug() << "Removing lock file:" << name;
            QFile file(name);
            if (file.remove()) {
                qDebug() << "Removed lock file:" << name;
                removedCount++;
            } else {
                qWarning() << "Failed to remove lock file:" << name;
            }
        }
    }
    qInfo() << "Removed" << removedCount << "lock files";
}

void Settings::slotCustomshortcut(const QString &key, const QVariant &value)
{
    qDebug() << "Entering slotCustomshortcut for key:" << key << "value:" << value.toString();
    auto keymap = settings->option("shortcuts.keymap.keymap");
    if (!m_bUserChangeKey && key.startsWith("shortcuts.") && key != "shortcuts.keymap.keymap" && !key.contains("_keymap_")) {
        qDebug() << "start with shortcuts.";
        m_bUserChangeKey = true;
        qDebug() << "m_bUserChangeKey is true";
        QString currentKeymap = settings->option("shortcuts.keymap.keymap")->value().toString();
        qDebug() << "currentKeymap" << currentKeymap;
        QStringList keySplitList = key.split(".");
        keySplitList[1] = QString("%1_keymap_customize").arg(keySplitList[1]);
        QString customizeKey = keySplitList.join(".");
        qDebug() << "customizeKey" << customizeKey;
        // Just update customize key user input, don't change keymap.
        if (currentKeymap == "customize") {
            settings->option(customizeKey)->setValue(value);
            qDebug() << "currentKeymap is customize";
        }
        // If current kemap is not "customize".
        // Copy all customize keys from current keymap, and then update customize key just user input.
        // Then change keymap name.
        else {
            copyCustomizeKeysFromKeymap(currentKeymap);
            settings->option(customizeKey)->setValue(value);
            keymap->setValue("customize");
            qDebug() << "currentKeymap is not customize";
        }
        m_bUserChangeKey = false;
        qDebug() << "m_bUserChangeKey is false";
    }
    qDebug() << "Leaving slotCustomshortcut";
}


void Settings::slotsigAdjustFont(QVariant value)
{
    qDebug() << "Font family changed to:" << value.toString();
    emit sigAdjustFont(value.toString());
    qDebug() << "Leaving slotsigAdjustFont";
}

void Settings::slotsigAdjustFontSize(QVariant value)
{
    qDebug() << "Font size changed to:" << value.toReal();
    emit sigAdjustFontSize(value.toReal());
}

void Settings::slotsigAdjustWordWrap(QVariant value)
{
    qDebug() << "Entering slotsigAdjustWordWrap, value:" << value.toBool();
    emit sigAdjustWordWrap(value.toBool());
    qDebug() << "Leaving slotsigAdjustWordWrap";
}

void Settings::slotsigSetLineNumberShow(QVariant value)
{
    qDebug() << "Entering slotsigSetLineNumberShow, value:" << value.toBool();
    emit sigSetLineNumberShow(value.toBool());
    qDebug() << "Leaving slotsigSetLineNumberShow";
}

void Settings::slotsigAdjustBookmark(QVariant value)
{
    qDebug() << "Entering slotsigAdjustBookmark, value:" << value.toBool();
    emit sigAdjustBookmark(value.toBool());
    qDebug() << "Leaving slotsigAdjustBookmark";
}

void Settings::slotsigShowCodeFlodFlag(QVariant value)
{
    qDebug() << "Entering slotsigShowCodeFlodFlag, value:" << value.toBool();
    emit sigShowCodeFlodFlag(value.toBool());
    qDebug() << "Leaving slotsigShowCodeFlodFlag";
}

void Settings::slotsigShowBlankCharacter(QVariant value)
{
    qDebug() << "Entering slotsigShowBlankCharacter, value:" << value.toBool();
    emit sigShowBlankCharacter(value.toBool());
    qDebug() << "Leaving slotsigShowBlankCharacter";
}

void Settings::slotsigHightLightCurrentLine(QVariant value)
{
    qDebug() << "Entering slotsigHightLightCurrentLine, value:" << value.toBool();
    emit sigHightLightCurrentLine(value.toBool());
    qDebug() << "Leaving slotsigHightLightCurrentLine";
}

void Settings::slotsigAdjustTabSpaceNumber(QVariant value)
{
    qDebug() << "Entering slotsigAdjustTabSpaceNumber, value:" << value.toInt();
    emit sigAdjustTabSpaceNumber(value.toInt());
    qDebug() << "Leaving slotsigAdjustTabSpaceNumber";
}

void Settings::slotupdateAllKeysWithKeymap(QVariant value)
{
    qDebug() << "Entering slotupdateAllKeysWithKeymap, value:" << value.toString();
    updateAllKeysWithKeymap(value.toString());
    qDebug() << "Leaving slotupdateAllKeysWithKeymap";
}

KeySequenceEdit::KeySequenceEdit(DTK_CORE_NAMESPACE::DSettingsOption *opt, QWidget *parent)
    : DKeySequenceEdit(parent)
{
    qDebug() << "KeySequenceEdit constructor called";
    m_pOption = opt;
    this->installEventFilter(this);
}

DSettingsOption *KeySequenceEdit::option()
{
    return m_pOption;
}
