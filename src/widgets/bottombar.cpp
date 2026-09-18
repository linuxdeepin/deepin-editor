// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>
#include <QActionGroup>
#include <QDebug>
#include <DMenu>
#include <DVerticalLine>
#include "bottombar.h"
#include "../common/utils.h"
#include "../editor/editwrapper.h"
#include "../editor/markdown/viewmode.h"
#include "../widgets/window.h"
#include "../editor/replaceallcommond.h"

// 不同布局模式(紧凑)
const int s_BottomBarHeight = 32;
const int s_BottomBarHeightCompact = 26;

BottomBar::BottomBar(QWidget *parent)
    : QWidget(parent),
      m_pWrapper(static_cast<EditWrapper *>(parent)),
      m_pPositionLabel(new DLabel),
      m_pCharCountLabel(new DLabel),
      m_pEncodeMenu(DDropdownMenu::createEncodeMenu()),
      m_pHighlightMenu(DDropdownMenu::createHighLightMenu()),
      m_rowStr(tr("Row")),
      m_columnStr(tr("Column")),
      m_chrCountStr(tr("Characters %1")),
      m_scaleLabel(new DLabel),
      m_progressLabel(new DLabel),
      m_progressBar(new DProgressBar)
{
    qDebug() << "BottomBar constructor";
    QFont font;
    m_pPositionLabel->setFont(font);
    m_pPositionLabel->setAccessibleName("PositionLabel");
    m_pCharCountLabel->setFont(font);
    m_pCharCountLabel->setAccessibleName("CharCountLabel");
    m_scaleLabel->setFont(font);
    m_scaleLabel->setAccessibleName("ScaleLabel");
    m_progressLabel->setFont(font);
    m_progressLabel->setAccessibleName("ProgressLabel");
    m_progressLabel->setText(tr("Loading:"));
    m_progressBar->setRange(0,100);
    m_progressBar->setAccessibleName("ProgressBar");
    m_progressBar->setTextVisible(false);
    m_progressBar->setMinimumWidth(80);
    QHBoxLayout* progressLayout = new QHBoxLayout;
    progressLayout->addWidget(m_progressLabel);
    progressLayout->addWidget(m_progressBar);
   // progressLayout->addStretch();

    DFontSizeManager::instance()->bind(m_pPositionLabel, DFontSizeManager::T9);
    DFontSizeManager::instance()->bind(m_pCharCountLabel, DFontSizeManager::T9);
    DFontSizeManager::instance()->bind(m_scaleLabel, DFontSizeManager::T9);
    DFontSizeManager::instance()->bind(m_progressLabel, DFontSizeManager::T9);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    qApp->installEventFilter(this);
#endif

    initFormatMenu();

    // —— Markdown 视图模式 combobox（§8.2，与编码格式菜单同型）——
    m_pViewModeMenu = new DDropdownMenu();
    m_pViewModeMenu->setAccessibleName("ViewModeMenu");
    DMenu *pViewModeDMenu = new DMenu(m_pViewModeMenu);
    QActionGroup *pViewModeGroup = new QActionGroup(m_pViewModeMenu);
    m_actEditView = pViewModeDMenu->addAction(tr("Edit Mode"));
    m_actReadView = pViewModeDMenu->addAction(tr("Read View"));
    m_actLivePreview = pViewModeDMenu->addAction(tr("Live Preview"));
    m_actEditView->setCheckable(true);
    m_actReadView->setCheckable(true);
    m_actLivePreview->setCheckable(true);
    m_actEditView->setData(static_cast<int>(ViewMode::Edit));
    m_actReadView->setData(static_cast<int>(ViewMode::ReadView));
    m_actLivePreview->setData(static_cast<int>(ViewMode::LivePreview));
    m_actEditView->setChecked(true);
    pViewModeGroup->addAction(m_actEditView);
    pViewModeGroup->addAction(m_actReadView);
    pViewModeGroup->addAction(m_actLivePreview);
    // 默认非 md（含新建标签）：仅「实时预览」置灰（§4.4 置灰规则）
    m_actLivePreview->setEnabled(false);
    connect(pViewModeDMenu, &DMenu::triggered, this, [this](QAction *action) {
        emit viewModeRequested(static_cast<ViewMode>(action->data().toInt()));
    });
    m_pViewModeMenu->setCurrentTextOnly(tr("Edit Mode"));
    m_pViewModeMenu->setMenu(pViewModeDMenu);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(29, 1, 10, 0);
    layout->setAlignment(Qt::AlignVCenter);
    layout->addLayout(progressLayout);
   // layout->addStretch();
    layout->addWidget(m_pPositionLabel);
    layout->addStretch();
    layout->addSpacerItem(new QSpacerItem(110,20,QSizePolicy::Expanding,QSizePolicy::Fixed));
    layout->addWidget(m_pCharCountLabel);

    m_progressBar->hide();
    m_progressLabel->hide();

    m_pPositionLabel->setText(QString("%1 %2  %3 %4").arg(m_rowStr, "1",m_columnStr, "1"));

    m_pCharCountLabel->setText(m_chrCountStr.arg("0"));
    m_pHighlightMenu->setCurrentTextOnly(qApp->translate("TextEdit", "None"));
    m_pEncodeMenu->setCurrentTextOnly(QString("UTF-8"));

    DVerticalLine *pVerticalLine1 = new DVerticalLine();
    DVerticalLine *pVerticalLine2 = new DVerticalLine();
    pVerticalLine1->setFixedSize(1, 15);
    pVerticalLine2->setFixedSize(1, 15);
    DVerticalLine *pVerticalLine3 = new DVerticalLine();
    pVerticalLine3->setFixedSize(1, 15);

    layout->addStretch();
    layout->addWidget(m_scaleLabel);
    layout->addStretch();
    layout->addWidget(m_pViewModeMenu);
    layout->addSpacing(10);
    layout->addWidget(pVerticalLine1);
    layout->addWidget(m_pEncodeMenu);
    layout->addWidget(pVerticalLine2);
    layout->addWidget(m_formatMenu);
    layout->addWidget(pVerticalLine3);
    layout->addWidget(m_pHighlightMenu);

    //切换编码
    connect(m_pEncodeMenu, &DDropdownMenu::currentActionChanged, this,[this](QAction* pAct){
        qInfo() << "Encoding changed to:" << pAct->text();
        // 保持界面统一
        QString previousText = m_pEncodeMenu->getCurrentText();

        // 处于文件加载状态或转换失败则恢复默认编码格式
        if (m_pWrapper->getFileLoading() || !m_pWrapper->reloadFileEncode(pAct->text().toLocal8Bit())) {
            qWarning() << "Failed to change encoding, reverting to:" << previousText;
            m_pEncodeMenu->setCurrentTextOnly(previousText);
        } else {
            // 存储完成后更新底栏显示文本，重载时可能需要保存旧文本
            m_pEncodeMenu->setCurrentTextOnly(pAct->text());
        }
    });

    //切换文件类型
    connect(m_pHighlightMenu, &DDropdownMenu::currentActionChanged, this,[this](QAction* pAct) {
        qInfo() << "Highlight mode changed to:" << pAct->text();
        m_pHighlightMenu->setCurrentTextOnly(pAct->text());

        // 更新使用格式高亮类型
        m_pWrapper->reloadFileHighlight(pAct->text());
    });

    //编码按钮/文本类型按钮失去焦点后，设置光标回到文本框里
    connect(m_pEncodeMenu, &DDropdownMenu::sigSetTextEditFocus, this, &BottomBar::slotSetTextEditFocus);
    connect(m_pHighlightMenu, &DDropdownMenu::sigSetTextEditFocus, this, &BottomBar::slotSetTextEditFocus);
    connect(m_formatMenu,&DDropdownMenu::sigSetTextEditFocus, this, &BottomBar::slotSetTextEditFocus);

#ifdef DTKWIDGET_CLASS_DSizeMode
    updateSizeMode();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, &BottomBar::updateSizeMode);
#else
    setFixedHeight(s_BottomBarHeight);
#endif
}

BottomBar::~BottomBar()
{
    qDebug() << "BottomBar destructor";
    if (m_pEncodeMenu != nullptr) {
        delete m_pEncodeMenu;
        m_pEncodeMenu = nullptr;
    }

    if (m_pHighlightMenu != nullptr) {
        delete m_pHighlightMenu;
        m_pHighlightMenu = nullptr;
    }
}

void BottomBar::updatePosition(int row, int column)
{
    m_pPositionLabel->setText(QString("%1 %2  %3 %4").arg(m_rowStr, QString::number(row),
                                                          m_columnStr, QString::number(column)));
}

void BottomBar::updateWordCount(int charactorCount)
{
    m_pCharCountLabel->setText(m_chrCountStr.arg(QString::number(charactorCount-1)));
}

void BottomBar::setEncodeName(const QString &name)
{
    m_pEncodeMenu->setCurrentTextOnly(name);
}

void BottomBar::setPalette(const QPalette &palette)
{
    DPalette paPositionLabel  = DGuiApplicationHelper::instance()->applicationPalette();
    DPalette paCharCountLabel = DGuiApplicationHelper::instance()->applicationPalette();
    DPalette paEncodeMenu = DGuiApplicationHelper::instance()->applicationPalette();
    DPalette paHighlightMenu = DGuiApplicationHelper::instance()->applicationPalette();
    DPalette paViewModeMenu = DGuiApplicationHelper::instance()->applicationPalette();

    QColor colorFont = paPositionLabel.textTips().color();

    paPositionLabel.setColor(DPalette::WindowText, colorFont);
    paCharCountLabel.setColor(DPalette::WindowText, colorFont);
    paEncodeMenu.setColor(DPalette::WindowText, colorFont);
    paHighlightMenu.setColor(DPalette::WindowText, colorFont);
    paViewModeMenu.setColor(DPalette::WindowText, colorFont);


    m_pPositionLabel->setPalette(paPositionLabel);
    m_pCharCountLabel->setPalette(paCharCountLabel);
    m_pEncodeMenu->getButton()->setPalette(paEncodeMenu);
    m_pHighlightMenu->getButton()->setPalette(paHighlightMenu);
    m_pViewModeMenu->getButton()->setPalette(paViewModeMenu);
    m_scaleLabel->setPalette(paPositionLabel);
    m_formatMenu->getButton()->setPalette(paEncodeMenu);

    QString theme = (palette.color(QPalette::Window).lightness() < 128) ? "dark" : "light";
    m_pEncodeMenu->setTheme(theme);
    m_pHighlightMenu->setTheme(theme);
    m_formatMenu->setTheme(theme);
    m_pViewModeMenu->setTheme(theme);

    QWidget::setPalette(palette);
}

void BottomBar::updateSize(int size, bool bIsFindOrReplace)
{
    setFixedHeight(size);
    m_bIsFindOrReplace = bIsFindOrReplace;
}

void BottomBar::setChildEnabled(bool enabled)
{
    m_pEncodeMenu->setEnabled(enabled);
    m_pHighlightMenu->setEnabled(enabled);
    m_pEncodeMenu->setRequestMenu(enabled);
    m_pHighlightMenu->setRequestMenu(enabled);
    m_formatMenu->setEnabled(enabled);
}

void BottomBar::setChildrenFocus(bool ok,QWidget* preOrderWidget)
{
    m_pEncodeMenu->setChildrenFocus(ok);
    m_pHighlightMenu->setChildrenFocus(ok);
    m_formatMenu->setChildrenFocus(ok);
    if(ok) {
        if(preOrderWidget) setTabOrder(preOrderWidget,m_pEncodeMenu->getButton());

        setTabOrder(m_pEncodeMenu->getButton(),m_formatMenu->getButton());
        setTabOrder(m_formatMenu->getButton(),m_pHighlightMenu->getButton());
    }
}

void BottomBar::setScaleLabelText(qreal fontSize)
{
    int maxFont = 50;
    int minFont = 8;
    int midFont = 12;
    QString text;
    if (qFuzzyCompare(fontSize, midFont)) {
        text = "100%";
    }
    else if(fontSize > midFont){
        float delta = (500-100)*1.0/(maxFont - midFont);
        int target = 100 + qRound(delta * (fontSize-midFont));
        text = QString("%1%").arg(target);
    }
    else {
        float delta = (100-10)*1.0/(midFont - minFont);
        int target = 100 + qRound(delta * (fontSize-midFont));
        target = std::max(10,target);
        text = QString("%1%").arg(target);
    }

    m_scaleLabel->setText(text);
}

void BottomBar::setProgress(int progress)
{
    if(progress<0){
        return;
    }
    qDebug() << "Loading progress:" << progress << "%";
    m_progressBar->show();
    m_progressLabel->show();
    m_progressBar->setValue(progress);
    if(progress >= 100){
        qDebug() << "Loading completed";
        m_progressBar->hide();
        m_progressLabel->hide();
    }
}

DDropdownMenu *BottomBar::getEncodeMenu()
{
    return m_pEncodeMenu;
}

DDropdownMenu *BottomBar::getHighlightMenu()
{
    return m_pHighlightMenu;
}

bool BottomBar::eventFilter(QObject *watched, QEvent *event)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (event->type() == QEvent::ApplicationFontChange) {
        QFont font = qApp->font();
        m_pPositionLabel->setFont(font);
        m_pCharCountLabel->setFont(font);
        m_scaleLabel->setFont(font);
        m_progressLabel->setFont(font);
        return false;
    }
#endif
    return QWidget::eventFilter(watched, event);
}

void BottomBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);

    QColor backgroundColor = palette().color(QPalette::Window);
    QColor bottombarBackgroundColor;
    if (backgroundColor.lightness() < 128) {
        bottombarBackgroundColor = palette().base().color();
        if (bottombarBackgroundColor.name() != "#202020") {
            bottombarBackgroundColor = QColor("#202020");
        }
        bottombarBackgroundColor.setAlphaF(0.7);

    } else {
        bottombarBackgroundColor = palette().base().color();
        if (bottombarBackgroundColor.name() != "#ffffff") {
            bottombarBackgroundColor = QColor("#ffffff");
        }

        bottombarBackgroundColor.setAlphaF(0.7);
    }

    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, bottombarBackgroundColor);

    QColor splitLineColor;
    if (backgroundColor.lightness() < 128) {
        splitLineColor = palette().brightText().color();
        if (splitLineColor.name() != "#ffffff") {
            splitLineColor = QColor("#ffffff");
        }
        splitLineColor.setAlphaF(0.5);
    } else {
        splitLineColor = palette().brightText().color();
        if (splitLineColor.name() != "#000000") {
            splitLineColor = QColor("#000000");
        }
        splitLineColor.setAlphaF(0.5);
    }

    if (!m_bIsFindOrReplace) {
	    QPainterPath framePath;
	    framePath.addRect(QRect(rect().x(), rect().y(), rect().width(), 1));
	    painter.setOpacity(0.1);
	    painter.fillPath(framePath, splitLineColor);
    }
}

void BottomBar::slotSetTextEditFocus()
{
    Window *pWindow = static_cast<Window *>(m_pWrapper->window());
    emit pWindow->pressEsc();
}

BottomBar::EndlineFormat BottomBar::getEndlineFormat(const QByteArray& text)
{
    for(int i=0;i<text.size();i++){
        if(text[i]=='\n'){
            return EndlineFormat::Unix;
        }
        if(text[i]=='\r' && i+1<text.size() && text[i+1]=='\n'){
            return EndlineFormat::Windows;
        }
    }

    return EndlineFormat::Unknow;
}

BottomBar::EndlineFormat BottomBar:: getEndlineFormat()
{
    return m_endlineFormat;
}

//初始化行尾格式相关
void BottomBar::initFormatMenu()
{
    m_formatMenu = new DDropdownMenu(this);
    DMenu *menu = new DMenu(this);
    QActionGroup* actionGroup = new QActionGroup(menu);
    actionGroup->setExclusive(true);
    m_formatMenu->setMenuActionGroup(actionGroup);

    m_unixAction = menu->addAction("Unix");
    m_unixAction->setObjectName("UnixAction");
    m_windowsAction = menu->addAction("Windows");
    m_windowsAction->setObjectName("WindowsAction");
    m_unixAction->setProperty(FormatActionType,EndlineFormat::Unix);
    m_windowsAction->setProperty(FormatActionType,EndlineFormat::Windows);
    actionGroup->addAction(m_unixAction);
    actionGroup->addAction(m_windowsAction);
    connect(actionGroup, &QActionGroup::triggered, this,&BottomBar::onFormatMenuTrigged);

    m_formatMenu->setMenu(menu);
    m_formatMenu->setCurrentTextOnly("Unix");
}

//行尾格式action槽函数
void BottomBar::onFormatMenuTrigged(QAction* action)
{
    if(!action){
        return;
    }
    int type = action->property(FormatActionType).toInt();
    if(m_endlineFormat == type){
        return;
    }

    qInfo() << "Endline format changed from" << m_endlineFormat << "to" << type;
    m_pWrapper->textEditor()->onEndlineFormatChanged(m_endlineFormat,(EndlineFormat)type);
    m_endlineFormat = (EndlineFormat)type;

}

/**
   @brief 根据界面布局模式 `DGuiApplicationHelper::isCompactMode()` 切换当前界面布局参数。
        需要注意，界面参数同设计图参数并非完全一致，而是按照实际的显示像素值进行比对。
 */
void BottomBar::updateSizeMode()
{
#ifdef DTKWIDGET_CLASS_DSizeMode
    if (DGuiApplicationHelper::isCompactMode()) {
        setFixedHeight(s_BottomBarHeightCompact);
    } else {
        setFixedHeight(s_BottomBarHeight);
    }
#endif
}

//设置行尾menu text
void BottomBar::setEndlineMenuText(EndlineFormat format)
{
    if(format == EndlineFormat::Unix || format == EndlineFormat::Unknow){
        m_formatMenu->setCurrentTextOnly("Unix");
        m_endlineFormat = EndlineFormat::Unix;

    }
    else {
        m_formatMenu->setCurrentTextOnly("Windows");
        m_endlineFormat = EndlineFormat::Windows;
    }
}

/**
   @return 返回默认的底部栏高度，此高度在紧凑模式下有不同值
 */
int BottomBar::defaultHeight()
{
#ifdef DTKWIDGET_CLASS_DSizeMode
    return DGuiApplicationHelper::isCompactMode() ? s_BottomBarHeightCompact : s_BottomBarHeight;
#else
    return s_BottomBarHeight;
#endif
}

// —— Markdown 视图模式入口（§8.2）——
void BottomBar::setViewMode(ViewMode mode)
{
    QAction *action = nullptr;
    QString name;
    switch (mode) {
    case ViewMode::ReadView:
        action = m_actReadView;
        name = tr("Read View");
        break;
    case ViewMode::LivePreview:
        action = m_actLivePreview;
        name = tr("Live Preview");
        break;
    case ViewMode::Edit:
    case ViewMode::Wysiwyg:
    default:
        action = m_actEditView;
        name = tr("Edit Mode");
        break;
    }
    if (action && !action->isChecked())
        action->setChecked(true);
    m_pViewModeMenu->setCurrentTextOnly(name);
}

void BottomBar::setMarkdownAvailable(bool ok)
{
    // 非 md 文件：仅「实时预览」置灰（§4.4 置灰规则）
    m_actLivePreview->setEnabled(ok);
}
