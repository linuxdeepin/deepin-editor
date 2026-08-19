// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>
#include <QLabel>
#include <DLabel>
#include "ddropdownmenu.h"
#include <DGuiApplicationHelper>
#include <DFontSizeManager>
#include <QPainterPath>
#include <DProgressBar>

#define FormatActionType "format-action-type"

class EditWrapper;
enum class ViewMode;

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
    void setPalette(const QPalette &palette);
    void updateSize(int size, bool bIsFindOrReplace);
    void setChildEnabled(bool enabled);
    //设置所有焦点　梁卫东　２０２０－０９－１４　１０：５５：２２
    void setChildrenFocus(bool ok,QWidget* preOrderWidget = nullptr);
    void setScaleLabelText(qreal fontSize);
    void setProgress(int progress);

    // —— Markdown 视图模式入口（§8.2）：折叠态显示当前视图名；非 md 仅置灰「实时预览」 ——
    void setViewMode(ViewMode mode);
    void setMarkdownAvailable(bool ok);

    DDropdownMenu* getEncodeMenu();
    DDropdownMenu* getHighlightMenu();
    static EndlineFormat getEndlineFormat(const QByteArray& text);
    EndlineFormat getEndlineFormat();
    void setEndlineMenuText(EndlineFormat format);
    static int defaultHeight();

protected:
    void paintEvent(QPaintEvent *);
    bool eventFilter(QObject *, QEvent *) override;

signals:
    // 用户经 combobox 请求切换视图模式（EditWrapper 校验后经 viewModeChanged 回写状态）
    void viewModeRequested(ViewMode mode);

private:
    void initFormatMenu();
    Q_SLOT void onFormatMenuTrigged(QAction* action);
    Q_SLOT void updateSizeMode();

private:
    EditWrapper *m_pWrapper {nullptr};
    DLabel *m_pPositionLabel {nullptr};
    DLabel *m_pCharCountLabel {nullptr};
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

    // —— Markdown 视图模式 combobox（§8.2，与编码格式菜单同型）——
    DDropdownMenu *m_pViewModeMenu {nullptr};
    QAction *m_actEditView {nullptr};
    QAction *m_actReadView {nullptr};
    QAction *m_actLivePreview {nullptr};


public slots:
	//编码按钮/文本类型按钮失去焦点后，设置光标回到文本框里
    void slotSetTextEditFocus();
};

#endif
