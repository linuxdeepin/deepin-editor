// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>
#include <QLabel>
#include <DLabel>
#include "ddropdownmenu.h"
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <QPainterPath>

class EditWrapper;
class BottomBar : public QWidget
{
    Q_OBJECT

public:
    explicit BottomBar(QWidget *parent = nullptr);
    ~BottomBar();

    void updatePosition(int row, int column);
    void updateWordCount(int charactorCount);
    void setEncodeName(const QString &name);
    void setCursorStatus(const QString &text);
    void setPalette(const QPalette &palette);
    void updateSize(int size, bool bIsFindOrReplace);
    void setChildEnabled(bool enabled);
    //设置所有焦点　梁卫东　２０２０－０９－１４　１０：５５：２２
    void setChildrenFocus(bool ok,QWidget* preOrderWidget = nullptr);

    DDropdownMenu* getEncodeMenu();
    DDropdownMenu* getHighlightMenu();

protected:
    void paintEvent(QPaintEvent *);

private:
    EditWrapper *m_pWrapper {nullptr};
    DLabel *m_pPositionLabel {nullptr};
    DLabel *m_pCharCountLabel {nullptr};
    DLabel *m_pCursorStatus {nullptr};
    DDropdownMenu *m_pEncodeMenu {nullptr};
    DDropdownMenu *m_pHighlightMenu {nullptr};
    QString m_rowStr {QString()};
    QString m_columnStr {QString()};
    QString m_chrCountStr {QString()};
    bool m_bIsFindOrReplace {false};

public slots:
	//编码按钮/文本类型按钮失去焦点后，设置光标回到文本框里
    void slotSetTextEditFocus();
};

#endif
