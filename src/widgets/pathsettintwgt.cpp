#include "pathsettintwgt.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QFileDialog>
#include "../common/settings.h"
PathSettingWgt::PathSettingWgt(QWidget* parent):DWidget(parent)
{
    init();
    onSaveIdChanged(Settings::instance()->getSavePathId());
}
PathSettingWgt::~PathSettingWgt()
{

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
    m_customBtn = new DPushButton(this);
    m_group->addButton(m_lastOptBox,LastOptBox);
    m_group->addButton(m_curFileBox,CurFileBox);
    m_group->addButton(m_customBox,CustomBox);

    m_lastOptBox->setText("remember the last opt path");
    m_curFileBox->setText("be consistent with it cur file path");
    m_customBox->setText("custom the file path");
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

}

void PathSettingWgt::connections()
{
    connect(m_group,static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),this,&PathSettingWgt::onBoxClicked);
    connect(m_customBtn,&DPushButton::clicked,this,&PathSettingWgt::onBtnClicked);
}

void PathSettingWgt::setEditText(const QString& text)
{
    QFontMetrics metrics(m_customEdit->font());
    Qt::TextElideMode em = Qt::TextElideMode::ElideMiddle;
    m_customEdit->setText(metrics.elidedText(text, em, 175));
}

void PathSettingWgt::onBoxClicked(int id)
{
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
//        auto path = Settings::instance()->getSavePath(CustomBox);
//        if(path.isEmpty()){
//            path = QDir::homePath() + "/Documents";
//            Settings::instance()->setSavePath(CustomBox,path);
//            Settings::instance()->setSavePath(LastOptBox,path);
//            Settings::instance()->setSavePath(CurFileBox,path);
//        }
        setEditText(Settings::instance()->getSavePath(CustomBox));
        m_customBtn->setEnabled(true);
        break;
    }
    default:
        break;
    }
}

void PathSettingWgt::onBtnClicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString path = Settings::instance()->getSavePath(PathSettingWgt::CustomBox);
    if(!QDir(path).exists() || path.isEmpty()){
        path = QDir::homePath() + "/Documents";
    }
    dialog.setDirectory(path);
    const int mode = dialog.exec();
    if (mode != QDialog::Accepted) {
        return;
    }

    path = dialog.selectedFiles().at(0);
    setEditText(path);
    Settings::instance()->setSavePath(PathSettingWgt::CustomBox,path);
}
