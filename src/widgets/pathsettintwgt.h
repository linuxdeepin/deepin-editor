#ifndef PATHSETTINTWGT_H
#define PATHSETTINTWGT_H
#include <DWidget>
#include <DCheckBox>
#include <DPushButton>
#include <DLineEdit>
#include <QButtonGroup>

DWIDGET_USE_NAMESPACE
class PathSettingWgt : public DWidget
{
    Q_OBJECT
public:
    enum CheckBoxType{
        LastOptBox,
        CurFileBox,
        CustomBox
    };

public:
    PathSettingWgt(QWidget* parent = nullptr);
    virtual ~PathSettingWgt();

public slots:
    void onSaveIdChanged(int id);
    void setEditText(const QString& text);

private:
    void init();
    void connections();
    // 判断自定义路径是否合法，不合法返回 文档 路径
    QString checkCustomPath(const QString &path) const;

signals:
    void savingPathChanged(CheckBoxType id,const QString& path);

private slots:
    void onBoxClicked(int);
    void onBtnClicked();

private:
    DCheckBox* m_curFileBox = nullptr;
    DCheckBox* m_lastOptBox = nullptr;
    DCheckBox* m_customBox = nullptr;
    DLineEdit* m_customEdit = nullptr;
    DPushButton* m_customBtn = nullptr;
    QButtonGroup* m_group = nullptr;
};

#endif
