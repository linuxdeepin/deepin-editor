#ifndef WARNINGNOTICES_H
#define WARNINGNOTICES_H

#include <QApplication>
#include <DFloatingMessage>
#include <QPushButton>
#include <QHBoxLayout>

DWIDGET_USE_NAMESPACE

class WarningNotices : public DFloatingMessage
{
    Q_OBJECT
public:
    explicit WarningNotices(MessageType notifyType = MessageType::ResidentType);
    ~WarningNotices();

    void setReloadBtn();
    void setSaveAsBtn();

signals:
    void reloadBtnClicked();
    void saveAsBtnClicked();
    void closeBtnClicked();

public slots:

private:
    QPushButton *m_reloadBtn;
    QPushButton *m_saveAsBtn;
    QHBoxLayout *m_pLayout;
};

#endif // WARNINGNOTICES_H
