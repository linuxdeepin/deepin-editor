// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COLORSELECTWDG_H
#define COLORSELECTWDG_H

#include <DWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWidgetAction>
#include <QEvent>
#include <DPushButton>
#include <DLabel>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPointer>

using namespace Dtk::Widget;

//单个颜色显示控件
/*
 * 　编辑区域右键菜单　标记颜色action添加显示标记颜色控件　梁卫东　２０２０－０８－１３　０９：４０：４９
 *
 *   ColorLabel 颜色显示控件
*/
class ColorLabel : public DWidget
{
    Q_OBJECT
public:
    explicit ColorLabel(QColor color,QWidget *parent = nullptr);

    void setColorSelected(bool bSelect);
    bool isSelected();
    QColor getColor();
signals:
    void sigColorClicked(bool bSelect,QColor color);
protected:
    void paintEvent(QPaintEvent *event) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent *event) override;
#else
    void enterEvent(QEnterEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
private:
    //颜色选择标记

    bool m_bSelected = false;
    bool m_bHover = false;
    bool m_bPressed = false;
    QColor m_color;
};

/*
*       编辑区域右键菜单　标记颜色action添加显示标记颜色控件　梁卫东　２０２０－０８－１３　０９：４０：４９
*
*       ColorSelectWdg 颜色选择控件
*/
//自定义ColorSelectWdg
class ColorSelectWdg: public DWidget
{
    Q_OBJECT
public:
    explicit ColorSelectWdg(QString text,QWidget *parent = nullptr);
    ~ColorSelectWdg();

    void setTheme(const QString &theme);
    QColor getDefaultColor();

signals:
    void sigColorSelected(bool bSelect,QColor color);

private:
    void initWidget();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    QPointer<QVBoxLayout> m_pMainLayout {nullptr};
    QPointer<QHBoxLayout> m_pHLayout1 {nullptr};
    QPointer<QHBoxLayout> m_pHLayout2 {nullptr};
    QList<ColorLabel*> m_colorLabels;
    int m_labelWidth = 23;
    int m_labelHeight = 23;
    DPushButton* m_pButton = nullptr;
    DLabel* m_pLabel = nullptr;
    QString m_text;
    QColor m_defaultColor;
    QString m_textColor;
};


#endif // COLORSELECTWDG_H
