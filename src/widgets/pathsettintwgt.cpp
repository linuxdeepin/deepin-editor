// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pathsettintwgt.h"
#include "../common/settings.h"

#include <DStyleHelper>
#include <DGuiApplicationHelper>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QFileDialog>

// 不同布局模式参数
const int s_PSWSuggestBtnSize = 36;
const int s_PSWSuggestIconSize = 24;
const int s_PSWSuggestBtnSizeCompact = 24;
const int s_PSWSuggestIconSizeCompact = 16;

PathSettingWgt::PathSettingWgt(QWidget* parent):DWidget(parent)
{
    qDebug() << "PathSettingWgt constructor";
    init();
    onSaveIdChanged(Settings::instance()->getSavePathId());
}

PathSettingWgt::~PathSettingWgt()
{
    qDebug() << "PathSettingWgt destructor";
}

void PathSettingWgt::onSaveIdChanged(int id)
{
    switch (id) {
    case CurFileBox:{
        m_curFileBox->setChecked(true);
        m_customBtn->setEnabled(false);
        break;
    }
    case LastOptBox:{
        m_lastOptBox->setChecked(true);
        m_customBtn->setEnabled(false);
        break;
    }
    case CustomBox:{
        m_customBox->setChecked(true);
        m_customBtn->setEnabled(true);
        setEditText(Settings::instance()->getSavePath(CustomBox));
        break;
    }
    default:
        break;
    }
}

void PathSettingWgt::init()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_group = new QButtonGroup(this);
    layout->setContentsMargins(0,0,0,0);
    m_curFileBox = new DCheckBox(this);
    m_lastOptBox = new DCheckBox(this);
    m_customBox = new DCheckBox(this);
    m_customEdit = new DLineEdit(this);
    m_customBtn = new DSuggestButton(this);
    // 获取DTK提供的选取按钮，保持和 DFileChooserEdit 相同的设置
    m_customBtn->setFixedSize(36, 36);
    m_customBtn->setIconSize(QSize(24, 24));
    m_customBtn->setIcon(DStyleHelper(style()).standardIcon(DStyle::SP_SelectElement, nullptr));

    m_group->addButton(m_lastOptBox,LastOptBox);
    m_group->addButton(m_curFileBox,CurFileBox);
    m_group->addButton(m_customBox,CustomBox);

    m_lastOptBox->setText(tr("Remember the last used path"));
    m_curFileBox->setText(tr("Same path as the current file"));
    m_customBox->setText(tr("Customize the default path"));
    m_customBtn->setEnabled(false);
    m_customEdit->setDisabled(true);
    m_customEdit->setClearButtonEnabled(false);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(m_customBox);
    hlayout->addWidget(m_customEdit);
    hlayout->addWidget(m_customBtn);

    layout->addWidget(m_lastOptBox);
    layout->addWidget(m_curFileBox);
    layout->addLayout(hlayout);

    connections();

#ifdef DTKWIDGET_CLASS_DSizeMode
    updateSizeMode();
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, &PathSettingWgt::updateSizeMode);
#endif
}

void PathSettingWgt::connections()
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    connect(m_group, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),this,&PathSettingWgt::onBoxClicked);
#else
    connect(m_group, static_cast<void(QButtonGroup::*)(QAbstractButton*)>(&QButtonGroup::buttonClicked), this, [this](QAbstractButton* button) {
        int id = m_group->id(button);
        onBoxClicked(id);
    });
#endif
    connect(m_customBtn, &QPushButton::clicked, this, &PathSettingWgt::onBtnClicked);
}

void PathSettingWgt::setEditText(const QString& text)
{
    QFontMetrics metrics(m_customEdit->font());
    Qt::TextElideMode em = Qt::TextElideMode::ElideMiddle;
    m_customEdit->setText(metrics.elidedText(text, em, 175));
}

void PathSettingWgt::onBoxClicked(int id)
{
    qInfo() << "Save path option changed to:" << id;
    switch (id) {
    case CurFileBox:{
        Settings::instance()->setSavePathId(CurFileBox);
        m_customBtn->setEnabled(false);
        break;
    }
    case LastOptBox:{
        Settings::instance()->setSavePathId(LastOptBox);
        m_customBtn->setEnabled(false);
        break;
    }
    case CustomBox:{
        Settings::instance()->setSavePathId(CustomBox);
        QString path = Settings::instance()->getSavePath(CustomBox);
        qDebug() << "Custom path set to:" << path;
        setEditText(path);
        m_customBtn->setEnabled(true);
        break;
    }
    default:
        break;
    }
}

void PathSettingWgt::onBtnClicked()
{
    qDebug() << "Custom path button clicked";
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString path = Settings::instance()->getSavePath(PathSettingWgt::CustomBox);
    if(!QDir(path).exists() || path.isEmpty()){
        path = QDir::homePath() + "/Documents";
        qDebug() << "Using default documents path:" << path;
    }
    dialog.setDirectory(path);
    const int mode = dialog.exec();
    if (mode != QDialog::Accepted) {
        qDebug() << "Path selection dialog canceled";
        return;
    }

    path = dialog.selectedFiles().at(0);
    qInfo() << "Selected new custom path:" << path;
    setEditText(path);
    Settings::instance()->setSavePath(PathSettingWgt::CustomBox,path);
}

/**
   @brief 根据界面布局模式 `DGuiApplicationHelper::isCompactMode()` 切换当前界面布局参数。
        需要注意，界面参数同设计图参数并非完全一致，而是按照实际的显示像素值进行比对。
 */
void PathSettingWgt::updateSizeMode()
{
#ifdef DTKWIDGET_CLASS_DSizeMode
    if (DGuiApplicationHelper::isCompactMode()) {
        m_customBtn->setFixedSize(s_PSWSuggestBtnSizeCompact, s_PSWSuggestBtnSizeCompact);
        m_customBtn->setIconSize(QSize(s_PSWSuggestIconSizeCompact, s_PSWSuggestIconSizeCompact));
    } else {
        m_customBtn->setFixedSize(s_PSWSuggestBtnSize, s_PSWSuggestBtnSize);
        m_customBtn->setIconSize(QSize(s_PSWSuggestIconSize, s_PSWSuggestIconSize));
    }
#endif
}
