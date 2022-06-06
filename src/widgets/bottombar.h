/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>
#include <QLabel>
#include <DLabel>
#include "ddropdownmenu.h"
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <QPainterPath>
#include <DProgressBar>

#define FormatActionType "format-action-type"

class EditWrapper;
class BottomBar : public QWidget
{
    Q_OBJECT

public:
    enum EndlineFormat{
        Unknow = -1,
        Unix,
        Windows
    };

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
    void setScaleLabelText(int fontSize);
    void setProgress(int progress);

    DDropdownMenu* getEncodeMenu();
    DDropdownMenu* getHighlightMenu();
    static EndlineFormat getEndlineFormat(const QString& text);
    EndlineFormat getEndlineFormat();
    void setEndlineMenuText(EndlineFormat format);

protected:
    void paintEvent(QPaintEvent *);

private:
    void initFormatMenu();
private slots:
    void onFormatMenuTrigged(QAction* action);

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
    DLabel* m_scaleLabel = nullptr;
    DLabel* m_progressLabel = nullptr;
    DProgressBar* m_progressBar = nullptr;
    DDropdownMenu *m_formatMenu = nullptr;
    EndlineFormat m_endlineFormat = EndlineFormat::Unix;
    QAction* m_unixAction = nullptr;
    QAction* m_windowsAction = nullptr;


public slots:
	//编码按钮/文本类型按钮失去焦点后，设置光标回到文本框里
    void slotSetTextEditFocus();
};

#endif
