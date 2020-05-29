#ifndef SHOWFLODCODEWIDGET_H
#define SHOWFLODCODEWIDGET_H
#include <DWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <DFrame>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE

class ShowFlodCodeWidget: public DWidget
{
    Q_OBJECT
public:
    explicit ShowFlodCodeWidget(DWidget *parent = nullptr);
    ~ShowFlodCodeWidget();
    void appendText(QString strText);
    void clear();
public slots:
    void textAreaChanged();

private:
    DTextEdit *m_pContentEdit;
    DFrame *m_pDArrowRectangle;

};

#endif // SHOWFLODCODEWIDGET_H
