// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
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
    qCDebug(appLog) << "CustemBackend::CustemBackend" << filepath;
}

void CustemBackend::doSync()
{
    qCDebug(appLog) << "Entering CustemBackend::doSync";
    m_settings->sync();
    qCDebug(appLog) << "Leaving CustemBackend::doSync";
}

void CustemBackend::doSetOption(const QString &key, const QVariant &value)
{
    qCDebug(appLog) << "CustemBackend::doSetOption" << key << value;
    /*
     * 将配置值写入config配置文件
     * 为了规避数据写入重复或者错乱，配置写入之前上锁，写入结束后开锁
     */
    m_writeLock.lock();
    qCDebug(appLog) << "Write lock acquired";
    m_settings->setValue(key, value);
    m_settings->sync();
    m_writeLock.unlock();
    qCDebug(appLog) << "Write lock released. Leaving CustemBackend::doSetOption";
}

QStringList CustemBackend::keys() const
{
    qCDebug(appLog) << "Entering CustemBackend::keys";
    QStringList keyList = m_settings->allKeys();
    qCDebug(appLog) << "Leaving CustemBackend::keys, returning:" << keyList;

    return keyList;
}

QVariant CustemBackend::getOption(const QString &key) const
{
    qCDebug(appLog) << "Entering CustemBackend::getOption, key:" << key;
    QVariant value = m_settings->value(key);
    qCDebug(appLog) << "Leaving CustemBackend::getOption, returning value:" << value;
    return value;
}

CustemBackend::~CustemBackend()
{
    qCDebug(appLog) << "Entering CustemBackend::~CustemBackend";
    delete m_settings;
    qCDebug(appLog) << "Leaving CustemBackend::~CustemBackend";
}

Settings::Settings(QWidget *parent)
    : QObject(parent)
{
    qCDebug(appLog) << "Initializing Settings instance";
    
    QString strConfigPath = QString("%1/%2/%3/config.conf")
                            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
                            .arg(qApp->organizationName())
                            .arg(qApp->applicationName());
    qCDebug(appLog) << "Config file path:" << strConfigPath;

    removeLockFiles();
    m_backend = new QSettingBackend(strConfigPath);
    qCDebug(appLog) << "Created QSettingBackend for config file";

    settings = DSettings::fromJsonFile(":/resources/settings.json");
    if (!settings) {
        qCWarning(appLog) << "Failed to load settings from JSON file";
        return;
    }
    settings->setBackend(m_backend);
    qCInfo(appLog) << "Settings initialized successfully";

    qCDebug(appLog) << "Connecting settings signals to slots";
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
    qCDebug(appLog) << "Leaving Settings::Settings";
}

void Settings::setSavePath(int id,const QString& path)
{
    qCDebug(appLog) << "Entering setSavePath for id:" << id << "path:" << path;
    switch (id) {
    case PathSettingWgt::LastOptBox:{
        qCDebug(appLog) << "Setting LastOptBox path";
        auto opt = settings->option("advance.open_save_setting.open_save_lastopt_path");
        opt->setValue(path);
        break;
    }
    case PathSettingWgt::CurFileBox:{
        qCDebug(appLog) << "Setting CurFileBox path";
        auto opt = settings->option("advance.open_save_setting.open_save_curfile_path");
        opt->setValue(path);
        break;
    }
    case PathSettingWgt::CustomBox:{
        qCDebug(appLog) << "Setting CustomBox path";
        auto opt = settings->option("advance.open_save_setting.open_save_custom_path");
        opt->setValue(path);
        break;
    }
    default:
        qCWarning(appLog) << "Unknown id in setSavePath:" << id;
        break;
    }
    qCDebug(appLog) << "Leaving setSavePath";
}


QString Settings::getSavePath(int id)
{
    qCDebug(appLog) << "Entering getSavePath for id:" << id;
    QString path;
    switch (id) {
    case PathSettingWgt::LastOptBox:{
        qCDebug(appLog) << "Getting LastOptBox path";
        path = settings->option("advance.open_save_setting.open_save_lastopt_path")->value().toString();
        break;
    }
    case PathSettingWgt::CurFileBox:{
        qCDebug(appLog) << "Getting CurFileBox path";
        path = settings->option("advance.open_save_setting.open_save_curfile_path")->value().toString();
        break;
    }
    case PathSettingWgt::CustomBox:{
        qCDebug(appLog) << "Getting CustomBox path";
        path = settings->option("advance.open_save_setting.open_save_custom_path")->value().toString();
        break;
    }
    default:
        qCWarning(appLog) << "Unknown id in getSavePath:" << id;
        break;
    }

    qCDebug(appLog) << "Leaving getSavePath, returning:" << path;
    return path;
}

void Settings::setSavePathId(int id)
{
    qCDebug(appLog) << "Entering setSavePathId, id:" << id;
    auto opt = settings->option("advance.open_save_setting.savingpathwgt");
    opt->setValue(QString::number(id));
    qCDebug(appLog) << "Leaving setSavePathId. Save path ID set to:" << opt->value();
}
int Settings::getSavePathId()
{
   qCDebug(appLog) << "Entering getSavePathId";
   int id = settings->option("advance.open_save_setting.savingpathwgt")->value().toInt();
   qCDebug(appLog) << "Leaving getSavePathId, returning:" << id;
   return id;

}
Settings::~Settings()
{
    qCDebug(appLog) << "Destroying Settings instance";
    // 单例被销毁后置空，避免 instance() 返回悬空指针（测试中可能 deleteLater 单例）
    if (s_pSetting == this) {
        s_pSetting = nullptr;
    }
    // 先销毁 DSettings 再销毁后端：setBackend() 会将 backend moveToThread
    // 到专门的写线程，doSetOption/doSync 经队列在该线程异步落盘，仅
    // DSettings::destroyed 会 quit+wait 该线程。若先 delete m_backend，
    // 主线程销毁 backend（及其 QSettings）会与写线程在途的 doSetOption
    // 竞态（QSettings UAF → SIGSEGV，CI test_settings 套件复现），且
    // settings 裸指针从不释放；此处先停写线程，后端销毁即无并发风险
    if (settings != nullptr) {
        delete settings;
        settings = nullptr;
    }
    if (m_backend != nullptr) {
        qCDebug(appLog) << "Cleaning up settings backend";
        delete m_backend;
        m_backend = nullptr;
    }
    qCDebug(appLog) << "Settings instance destroyed";
}

void Settings::setSettingDialog(DSettingsDialog *settingsDialog)
{
    qCDebug(appLog) << "Setting dialog instance";
    m_pSettingsDialog = settingsDialog;
}

// This function is workaround, it will remove after DTK fixed SettingDialog theme bug.
void Settings::dtkThemeWorkaround(QWidget *parent, const QString &theme)
{
    qCDebug(appLog) << "Entering dtkThemeWorkaround, theme:" << theme;
    parent->setStyle(QStyleFactory::create(theme));
    qCDebug(appLog) << "Theme applied to parent widget:" << parent->objectName();

    for (auto obj : parent->children()) {
        auto w = qobject_cast<QWidget *>(obj);
        if (!w) {
            qCDebug(appLog) << "w is null";
            continue;
        }
        qCDebug(appLog) << "w is not null";
        dtkThemeWorkaround(w, theme);
    }
    qCDebug(appLog) << "Leaving dtkThemeWorkaround for parent:" << parent->objectName();
}

QPair<QWidget *, QWidget *> Settings::createFontComBoBoxHandle(QObject *obj)
{
    qCDebug(appLog) << "Entering createFontComBoBoxHandle";
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);

    QComboBox *comboBox = new QComboBox;
    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, comboBox);

    QFontDatabase fontDatabase;
    comboBox->addItems(fontDatabase.families());
    // 设置最小宽度，以保持和下方 OptionDSpinBox 一样的长度
    comboBox->setMinimumSize(240, 36);

    if (option->value().toString().isEmpty()) {
        qCDebug(appLog) << "option->value().toString().isEmpty()";
        option->setValue(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
    }

    // init.
    comboBox->setCurrentText(option->value().toString());

    connect(option, &DSettingsOption::valueChanged, comboBox, [ = ](QVariant var) {
        qCDebug(appLog) << "Font family value changed to:" << var.toString();
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

    qCDebug(appLog) << "Leaving createFontComBoBoxHandle";
    return optionWidget;
}

QWidget* Settings::createSavingPathWgt(QObject* obj)
{
    qCDebug(appLog) << "Creating saving path widget";
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);
    QString name = option->name();
    PathSettingWgt* pathwgt = new PathSettingWgt;
    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, pathwgt);

    connect(option, &DSettingsOption::valueChanged, pathwgt, [ = ](QVariant var) {
        int id = var.toInt();
        qCDebug(appLog) << "pathwgt->onSaveIdChanged" << id;
        pathwgt->onSaveIdChanged(id);
    });

    auto custompath = s_pSetting->settings->option("advance.open_save_setting.open_save_custom_path");
    connect(custompath, &Dtk::Core::DSettingsOption::valueChanged, [=](QVariant var){
        //pathwgt->setEditText(var.toString());
    });

    qCDebug(appLog) << "Leaving createSavingPathWgt";
    return optionWidget.second;
}

QPair<QWidget *, QWidget *> Settings::createKeySequenceEditHandle(QObject *obj)
{
    qCDebug(appLog) << "Entering createKeySequenceEditHandle";
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);
    KeySequenceEdit *shortCutLineEdit = new KeySequenceEdit(option);

    shortCutLineEdit->ShortcutDirection(Qt::AlignLeft);
    shortCutLineEdit->setFocusPolicy(Qt::StrongFocus);

    // init.
    shortCutLineEdit->setKeySequence(QKeySequence(option->value().toString()));
    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, shortCutLineEdit);
    qCDebug(appLog) << "shortCutLineEdit->editingFinished";
    option->connect(shortCutLineEdit, &DKeySequenceEdit::editingFinished, [ = ](const QKeySequence & sequence) {
        qCDebug(appLog) << "shortCutLineEdit->editingFinished" << sequence;
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
            qCDebug(appLog) << "defaultOption" << defaultOption->defaultValue().toString();
            defaultKey = defaultOption->defaultValue().toString();
        }

        // 判断新设置的组合键序列是否允许
        if (!instance()->checkShortcutValid(checkName, sequence.toString(), reason, bIsConflicts, defaultKey)) {
            qCDebug(appLog) << "checkShortcutValid is false";
            instance()->m_pDialog = instance()->createDialog(reason, "", bIsConflicts);
            instance()->m_pDialog->exec();
            // 恢复组合键序列
            QString currentValidValue = instance()->settings->value(checkName).toString();
            QStringList customizeKeySplitList = option->key().split(".");
            customizeKeySplitList[1] = QString("%1_keymap_customize").arg(customizeKeySplitList[1]);
            instance()->settings->option(customizeKeySplitList.join("."))->setValue(currentValidValue);
            keymap->setValue("emacs");
            keymap->setValue("customize");
            qCDebug(appLog) << "Leaving createKeySequenceEditHandle";
            return;
        }

        bool bIsCustomize = false;
        QString conflictsKeys;
        QString originalKeys;

        if (keymap->value().toString() != "customize") {
            qCDebug(appLog) << "keymap->value().toString() != customize";
            instance()->m_bUserChangeKey = true;

            for (auto loopOption : instance()->settings->group("shortcuts.window_keymap_customize")->options()) {
                QStringList loopKeySplitList = loopOption->key().split(".");
                loopKeySplitList[1] = QString("window_keymap_%1").arg(keymap->value().toString());

                if (loopOption->value().toString() == sequence.toString()) {
                    qCDebug(appLog) << "loopOption->value().toString() == sequence.toString()";
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
                    qCDebug(appLog) << "loopOption->value().toString() == sequence.toString()";
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
                    qCDebug(appLog) << "loopOption->value().toString() == sequence.toString()";
                    if (checkName.contains(loopKeySplitList.last())) {
                        qCDebug(appLog) << "checkName.contains(loopKeySplitList.last())";
                        //return;
                    } else {
                        qCDebug(appLog) << "loopOption->value().toString() != sequence.toString()";
                        bIsConflicts = true;
                        conflictsKeys = loopKeySplitList.join(".");
                    }
                }
            }

            for (auto loopOption : instance()->settings->group("shortcuts.editor_keymap_customize")->options()) {
                QStringList loopKeySplitList = loopOption->key().split(".");
                loopKeySplitList[1] = QString("editor_keymap_%1").arg(keymap->value().toString());

                if (loopOption->value().toString() == sequence.toString()) {
                    qCDebug(appLog) << "loopOption->value().toString() == sequence.toString()";
                    if (checkName.contains(loopKeySplitList.last())) {
                        qCDebug(appLog) << "checkName.contains(loopKeySplitList.last())";
                        //return;
                    } else {
                        qCDebug(appLog) << "loopOption->value().toString() != sequence.toString()";
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
        qCDebug(appLog) << "qstrSequence" << qstrSequence;

        if (sequence.toString().contains("<")) {
            qCDebug(appLog) << "sequence.toString().contains(<)";
            qstrSequence.replace(qstrSequence.indexOf("<"), 1, "&lt;");
        }

        if (sequence.toString().contains("Return")) {
            qCDebug(appLog) << "sequence.toString().contains(Return)";
            qstrSequence.replace(qstrSequence.indexOf("Return"), 6, "Enter");
        }

        QString style = QString("<span style=\"color: rgba(255, 87, 54, 1);\">[%1]</span>").arg(qstrSequence);

        if (bIsConflicts || sequence.toString() == "Alt+M") {
            qCDebug(appLog) << "bIsConflicts" << bIsConflicts;
            if (sequence.toString() == "Alt+M") {
                qCDebug(appLog) << "sequence.toString() == Alt+M";
                instance()->m_pDialog = instance()->createDialog(tr("This shortcut conflicts with system shortcut %1").arg(style), "", bIsConflicts);
            } else {
                qCDebug(appLog) << "else Alt+M";
                instance()->m_pDialog = instance()->createDialog(tr("This shortcut conflicts with %1, click on Replace to make this shortcut effective immediately").arg(style), "", bIsConflicts);
            }

            int mode = instance()->m_pDialog->exec();

            // click cancel button.
            if (mode == -1 || mode == 0) {
                shortCutLineEdit->setKeySequence(QKeySequence(instance()->settings->value(keySplitList.join(".")).toString()));
                keymap->setValue("emacs");
                keymap->setValue("customize");
                qCDebug(appLog) << "click cancel button.";
                return;
            } else {
                qCDebug(appLog) << "click replace button.";
                keySplitList = option->key().split(".");
                keySplitList[1] = QString("%1_keymap_customize").arg(keySplitList[1]);

                if (!bIsCustomize) {
                    qCDebug(appLog) << "!bIsCustomize";
                    instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
                    instance()->settings->option(conflictsKeys)->setValue("");
                    shortCutLineEdit->setKeySequence(QKeySequence(instance()->settings->value(checkName).toString()));
                } else {
                    qCDebug(appLog) << "bIsCustomize";
                    instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
                    instance()->settings->option(conflictsKeys)->setValue("");
                }
                keymap->setValue("emacs");
                keymap->setValue("customize");
                qCDebug(appLog) << "click replace button.";
                return;
            }
        }

        if (!bIsCustomize) {
            qCDebug(appLog) << "!bIsCustomize";
            keySplitList = option->key().split(".");
            keySplitList[1] = QString("%1_keymap_customize").arg(keySplitList[1]);
            instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
        } else {
            qCDebug(appLog) << "bIsCustomize";
            instance()->settings->option(keySplitList.join("."))->setValue(sequence.toString());
        }
        keymap->setValue("emacs");
        keymap->setValue("customize");
        qCDebug(appLog) << "click replace button.";
    });

    // 配置修改
    option->connect(option, &DTK_CORE_NAMESPACE::DSettingsOption::valueChanged, shortCutLineEdit, &KeySequenceEdit::slotDSettingsOptionvalueChanged);
    qCDebug(appLog) << "Leaving createKeySequenceEditHandle";
    return optionWidget;
}

void KeySequenceEdit::slotDSettingsOptionvalueChanged(const QVariant & value)
{
    qCDebug(appLog) << "KeySequenceEdit::slotDSettingsOptionvalueChanged" << value;
    QString keyseq = value.toString();
    qCDebug(appLog) << "keyseq" << keyseq;
    if (keyseq.isEmpty()) {
        this->clear();
        qCDebug(appLog) << "keyseq is empty";
        return;
    }
    qCDebug(appLog) << "keyseq is not empty";
    this->setKeySequence(QKeySequence(keyseq));
    qCDebug(appLog) << "Leaving KeySequenceEdit::slotDSettingsOptionvalueChanged";
}

bool KeySequenceEdit::eventFilter(QObject *object, QEvent *event)
{
    qCDebug(appLog) << "KeySequenceEdit::eventFilter";
    //设置界面　回车键和空格键　切换输入 梁卫东　２０２０－０８－２１　１６：２８：３１
    if (object == this) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            qCDebug(appLog) << "KeySequenceEdit::eventFilter KeyPress";
            //判断是否包含组合键　梁卫东　２０２０－０９－０２　１５：０３：５６
            Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
            qCDebug(appLog) << "modifiers" << modifiers;
            bool bHasModifier = (modifiers & Qt::ShiftModifier || modifiers & Qt::ControlModifier ||
                                 modifiers & Qt::AltModifier);
            qCDebug(appLog) << "bHasModifier" << bHasModifier;

            if (!bHasModifier && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Space)) {
                qCDebug(appLog) << "keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Space";
                QRect rect = this->rect();
                qCDebug(appLog) << "rect" << rect;
                QList<QLabel*> childern = findChildren<QLabel*>();
                qCDebug(appLog) << "childern" << childern;

                for (int i =0; i < childern.size(); i++) {
                    QPoint pos(25,rect.height()/2);

                    QMouseEvent event0(QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    DApplication::sendEvent(childern[i], &event0);
                }

                qCDebug(appLog) << "KeySequenceEdit::eventFilter KeyPress return true";
                return true;
            }
        }
    }

    return DKeySequenceEdit::eventFilter(object, event);
}
Settings *Settings::instance()
{
    qCDebug(appLog) << "Accessing Settings singleton instance";
    if (s_pSetting == nullptr) {
        qCDebug(appLog) << "Creating new Settings instance";
        s_pSetting = new Settings;
    }
    return s_pSetting;
}

void Settings::updateAllKeysWithKeymap(QString keymap)
{
    qCDebug(appLog) << "Updating all keys with keymap:" << keymap;
    m_bUserChangeKey = true;

    int updatedCount = 0;
    for (auto option : settings->group("shortcuts.window")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap);
        auto opt = settings->option(keySplitList.join("."));
        if (opt) {
            qCDebug(appLog) << "Updating key" << keySplitList.join(".") << "with value" << opt->value().toString();
            option->setValue(opt->value().toString());
        } else {
            qCWarning(appLog) << "Unknown shortcut key:" << keySplitList.join(".");
        }
    }

    for (auto option : settings->group("shortcuts.editor")->options()) {
        updatedCount++;
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap);
        auto opt = settings->option(keySplitList.join("."));
        if (opt) {
            qCDebug(appLog) << "Updating key" << keySplitList.join(".") << "with value" << opt->value().toString();
            option->setValue(opt->value().toString());
        } else {
            qCWarning(appLog) << "Unknown shortcut key:" << keySplitList.join(".");
        }
    }

    m_bUserChangeKey = false;
    qCInfo(appLog) << "Updated" << updatedCount << "shortcuts with keymap:" << keymap;
}

void Settings::copyCustomizeKeysFromKeymap(QString keymap)
{
    qCDebug(appLog) << "Entering copyCustomizeKeysFromKeymap, keymap:" << keymap;

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
    qCDebug(appLog) << "Leaving copyCustomizeKeysFromKeymap";
}

bool Settings::checkShortcutValid(const QString &Name, QString Key, QString &Reason, bool &bIsConflicts, QString defaultValue)
{
    Q_UNUSED(Name);
    qCDebug(appLog) << "Entering checkShortcutValid for" << Name << "with key" << Key;

    if (Key.contains("<")) {
        qCDebug(appLog) << "Key contains <";
        Key.replace(Key.indexOf("<"), 1, "&lt;");
    }

    QString style = QString("<span style=\"color: rgba(255, 87, 54, 1);\">[%1]</span>").arg(Key);
    // 单键
    if (Key.count("+") == 0) {
        qCDebug(appLog) << "Key count + is 0";
        //F1-F12是允许的，这个正则不够精确，但是没关系。
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QRegExp regexp("^F[0-9]{1,2}$");
#else
        QRegularExpression regexp("^F[0-9]{1,2}$");
#endif
        if (!Key.contains(regexp)) {
            qCDebug(appLog) << "Key does not contain regexp";
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
        qCDebug(appLog) << "Key contains regexpNum";
        Reason = tr("The shortcut %1 is invalid, please set another one.").arg(style);
        bIsConflicts = false;
        return  false;
    }

    // 屏蔽Shift组合键处理，但允许恢复为默认组合键序列
    if (Key.contains("Shift")
            && Key != defaultValue) {
        qCDebug(appLog) << "Key contains Shift and Key is not defaultValue";
        Reason = tr("The shortcut %1 is invalid, please set another one.").arg(style);
        bIsConflicts = false;
        return false;
    }

    qCDebug(appLog) << "Leaving checkShortcutValid, returning true.";
    return true;
}

bool Settings::isShortcutConflict(const QString &Name, const QString &Key)
{
    qCDebug(appLog) << "Entering isShortcutConflict for" << Name << "with key" << Key;
    for (QString tmpKey : settings->keys()) {
        if (settings->value(tmpKey).toString() == Key/* && tmpKey.contains("customize")*/) {
            if (Name != tmpKey) {
                return  true;
            }
        }
    }
    qCDebug(appLog) << "Leaving isShortcutConflict, returning false.";
    return  false;
}

DDialog *Settings::createDialog(const QString &title, const QString &content, const bool &bIsConflicts)
{
    qCDebug(appLog) << "Entering createDialog for" << title << "with content" << content << "and bIsConflicts" << bIsConflicts;
    DDialog *dialog = new DDialog(title, content, m_pSettingsDialog);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnBottomHint);
    dialog->setIcon(QIcon::fromTheme("deepin-editor"));

    if (bIsConflicts) {
        qCDebug(appLog) << "Adding Cancel button";
        dialog->addButton(QString(tr("Cancel")), true, DDialog::ButtonNormal);
        dialog->addButton(QString(tr("Replace")), false, DDialog::ButtonRecommend);
    } else {
        qCDebug(appLog) << "Adding OK button";
        dialog->addButton(QString(tr("OK")), true, DDialog::ButtonRecommend);
    }

    qCDebug(appLog) << "Leaving createDialog";
    return dialog;
}

//删除config.conf配置文件目录下的.lock文件和.rmlock文件
void Settings::removeLockFiles()
{
    qCDebug(appLog) << "Removing lock files";
    QString configPath = QString("%1/%2/%3")
            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
            .arg(qApp->organizationName())
            .arg(qApp->applicationName());
    qCDebug(appLog) << "Config directory:" << configPath;

    QDir dir(configPath);
    if (!dir.exists()) {
        qCDebug(appLog) << "Config directory does not exist";
        return;
    }

    dir.setFilter(QDir::Files);
    QStringList nameList = dir.entryList();
    int removedCount = 0;
    for (auto name: nameList) {
        if (name.contains(".lock") || name.contains(".rmlock")) {
            // entryList 返回裸文件名，必须拼绝对路径后再删除，
            // 否则删除落点依赖进程当前工作目录，正常启动场景清理必然失效
            const QString lockFile = dir.absoluteFilePath(name);
            qCDebug(appLog) << "Removing lock file:" << lockFile;
            if (QFile::remove(lockFile)) {
                qCDebug(appLog) << "Removed lock file:" << lockFile;
                removedCount++;
            } else {
                qCWarning(appLog) << "Failed to remove lock file:" << lockFile;
            }
        }
    }
    qCInfo(appLog) << "Removed" << removedCount << "lock files";
}

void Settings::slotCustomshortcut(const QString &key, const QVariant &value)
{
    qCDebug(appLog) << "Entering slotCustomshortcut for key:" << key << "value:" << value.toString();
    auto keymap = settings->option("shortcuts.keymap.keymap");
    if (!m_bUserChangeKey && key.startsWith("shortcuts.") && key != "shortcuts.keymap.keymap" && !key.contains("_keymap_")) {
        qCDebug(appLog) << "start with shortcuts.";
        m_bUserChangeKey = true;
        qCDebug(appLog) << "m_bUserChangeKey is true";
        QString currentKeymap = settings->option("shortcuts.keymap.keymap")->value().toString();
        qCDebug(appLog) << "currentKeymap" << currentKeymap;
        QStringList keySplitList = key.split(".");
        keySplitList[1] = QString("%1_keymap_customize").arg(keySplitList[1]);
        QString customizeKey = keySplitList.join(".");
        qCDebug(appLog) << "customizeKey" << customizeKey;
        // Just update customize key user input, don't change keymap.
        if (currentKeymap == "customize") {
            settings->option(customizeKey)->setValue(value);
            qCDebug(appLog) << "currentKeymap is customize";
        }
        // If current kemap is not "customize".
        // Copy all customize keys from current keymap, and then update customize key just user input.
        // Then change keymap name.
        else {
            copyCustomizeKeysFromKeymap(currentKeymap);
            settings->option(customizeKey)->setValue(value);
            keymap->setValue("customize");
            qCDebug(appLog) << "currentKeymap is not customize";
        }
        m_bUserChangeKey = false;
        qCDebug(appLog) << "m_bUserChangeKey is false";
    }
    qCDebug(appLog) << "Leaving slotCustomshortcut";
}


void Settings::slotsigAdjustFont(QVariant value)
{
    qCDebug(appLog) << "Font family changed to:" << value.toString();
    emit sigAdjustFont(value.toString());
    qCDebug(appLog) << "Leaving slotsigAdjustFont";
}

void Settings::slotsigAdjustFontSize(QVariant value)
{
    qCDebug(appLog) << "Font size changed to:" << value.toReal();
    emit sigAdjustFontSize(value.toReal());
}

void Settings::slotsigAdjustWordWrap(QVariant value)
{
    qCDebug(appLog) << "Entering slotsigAdjustWordWrap, value:" << value.toBool();
    emit sigAdjustWordWrap(value.toBool());
    qCDebug(appLog) << "Leaving slotsigAdjustWordWrap";
}

void Settings::slotsigSetLineNumberShow(QVariant value)
{
    qCDebug(appLog) << "Entering slotsigSetLineNumberShow, value:" << value.toBool();
    emit sigSetLineNumberShow(value.toBool());
    qCDebug(appLog) << "Leaving slotsigSetLineNumberShow";
}

void Settings::slotsigAdjustBookmark(QVariant value)
{
    qCDebug(appLog) << "Entering slotsigAdjustBookmark, value:" << value.toBool();
    emit sigAdjustBookmark(value.toBool());
    qCDebug(appLog) << "Leaving slotsigAdjustBookmark";
}

void Settings::slotsigShowCodeFlodFlag(QVariant value)
{
    qCDebug(appLog) << "Entering slotsigShowCodeFlodFlag, value:" << value.toBool();
    emit sigShowCodeFlodFlag(value.toBool());
    qCDebug(appLog) << "Leaving slotsigShowCodeFlodFlag";
}

void Settings::slotsigShowBlankCharacter(QVariant value)
{
    qCDebug(appLog) << "Entering slotsigShowBlankCharacter, value:" << value.toBool();
    emit sigShowBlankCharacter(value.toBool());
    qCDebug(appLog) << "Leaving slotsigShowBlankCharacter";
}

void Settings::slotsigHightLightCurrentLine(QVariant value)
{
    qCDebug(appLog) << "Entering slotsigHightLightCurrentLine, value:" << value.toBool();
    emit sigHightLightCurrentLine(value.toBool());
    qCDebug(appLog) << "Leaving slotsigHightLightCurrentLine";
}

void Settings::slotsigAdjustTabSpaceNumber(QVariant value)
{
    qCDebug(appLog) << "Entering slotsigAdjustTabSpaceNumber, value:" << value.toInt();
    emit sigAdjustTabSpaceNumber(value.toInt());
    qCDebug(appLog) << "Leaving slotsigAdjustTabSpaceNumber";
}

void Settings::slotupdateAllKeysWithKeymap(QVariant value)
{
    qCDebug(appLog) << "Entering slotupdateAllKeysWithKeymap, value:" << value.toString();
    updateAllKeysWithKeymap(value.toString());
    qCDebug(appLog) << "Leaving slotupdateAllKeysWithKeymap";
}

KeySequenceEdit::KeySequenceEdit(DTK_CORE_NAMESPACE::DSettingsOption *opt, QWidget *parent)
    : DKeySequenceEdit(parent)
{
    qCDebug(appLog) << "KeySequenceEdit constructor called";
    m_pOption = opt;
    this->installEventFilter(this);
}

DSettingsOption *KeySequenceEdit::option()
{
    return m_pOption;
}
