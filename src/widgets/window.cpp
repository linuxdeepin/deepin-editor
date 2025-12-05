// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "window.h"
#include "pathsettintwgt.h"
#include <DTitlebar>
#include <DAnchors>
#include <DSettingsWidgetFactory>
#include <DSettingsGroup>
#include <DSettings>
#include <DSettingsOption>
#include <QApplication>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QScreen>
#include <QStyleFactory>
#include <QEvent>
#include <QDebug>
#include <DDialog>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <DGuiApplicationHelper>
#include <DMessageManager>
#include <DGuiApplicationHelper>
#include <DPrintPreviewDialog>
#include <QGuiApplication>
#include <QWindow>
#include <DWidgetUtil>
#include <QTextDocumentFragment>
#include <dprintpreviewwidget.h>

#ifdef DTKWIDGET_CLASS_DFileDialog

#include <DFileDialog>
#else
#include <QFileDialog>
#endif

#define PRINT_FLAG 2
#define PRINT_ACTION 8
#define PRINT_FORMAT_MARGIN 10
#define FLOATTIP_MARGIN 95

/**
 * @brief 根据传入的源文档 \a doc 创建新的文档
 * @param doc 源文档指针
 * @return 创建的新文档，用于拷贝数据
 */
static QTextDocument *createNewDocument(QTextDocument *doc)
{
    qDebug() << "createNewDocument called";
    QTextDocument *createDoc = new QTextDocument(doc);
    // 不记录撤销信息
    createDoc->setUndoRedoEnabled(false);
    createDoc->setMetaInformation(QTextDocument::DocumentTitle,
                                  doc->metaInformation(QTextDocument::DocumentTitle));
    createDoc->setMetaInformation(QTextDocument::DocumentUrl,
                                  doc->metaInformation(QTextDocument::DocumentUrl));
    createDoc->setDefaultFont(doc->defaultFont());
    createDoc->setDefaultStyleSheet(doc->defaultStyleSheet());
    createDoc->setIndentWidth(doc->indentWidth());

    // 设置固定为从左向右布局，降低后续高亮等操作导致更新计算布局方向
    auto textOption = createDoc->defaultTextOption();
    textOption.setTextDirection(Qt::LeftToRight);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    createDoc->setDefaultTextOption(textOption);

    qDebug() << "createNewDocument end";
    return createDoc;
}

/*!
 * \~chinese \brief printPage 绘制每一页文本纸张到打印机
 * \~chinese \param index 纸张索引
 * \~chinese \param doc 文本文件
 * \~chinese \param body 范围大小
 * \~chinese \param pageCountBox 绘制页码的范围
 */
void Window::printPage(int index, QPainter *painter, const QTextDocument *doc,
                       const QRectF &body, const QRectF &pageCountBox)
{
    qDebug() << "printPage called";
    painter->save();

    painter->translate(body.left(), body.top() - (index - 1) * body.height());
    const QRectF view(0, (index - 1) * body.height(), body.width(), body.height());

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QAbstractTextDocumentLayout::PaintContext ctx;

    painter->setFont(QFont(doc->defaultFont()));
    const QRectF box = pageCountBox.translated(0, view.top());
    const QString pageString = QString::number(index);
    painter->drawText(box, Qt::AlignRight, pageString);

    painter->setClipRect(view);
    ctx.clip = view;
    ctx.palette.setColor(QPalette::Text, Qt::black);

    layout->draw(painter, ctx);

    painter->restore();
    qDebug() << "printPage end";
}

/**
 * @brief 使用多组文档打印，用于超大文档的情况
 * @param index         纸张索引
 * @param painter       打印指针
 * @param printInfo     多组文档信息
 * @param body          范围大小
 * @param pageCountBox  绘制页码的范围
 */
void Window::printPageWithMultiDoc(
    int index, QPainter *painter, const QVector<Window::PrintInfo> &printInfo, const QRectF &body, const QRectF &pageCountBox)
{
    painter->save();

    int docIndex = index;
    for (auto info : printInfo) {
        if (docIndex <= info.doc->pageCount()) {
            // 调整绘制工具坐标到当前页面顶部
            painter->translate(body.left(), body.top() - (docIndex - 1) * body.height());

            // 绘制页码
            painter->setFont(QFont(info.doc->defaultFont()));
            const QRectF box = pageCountBox.translated(0, (docIndex - 1) * body.height());
            const QString pageString = QString::number(index);
            painter->drawText(box, Qt::AlignRight, pageString);

            // 设置文档裁剪区域
            const QRectF docView(0, (docIndex - 1) * body.height(), body.width(), body.height());
            QAbstractTextDocumentLayout *layout = info.doc->documentLayout();

            // 大文本的高亮单独处理
            if (info.highlighter) {
                // 提前两页拷贝数据，用于处理高亮计算时连续处理
                qreal offsetHeight = qMax<qreal>(0, (qMin(2, docIndex - 1) * body.height()));
                QPointF point = docView.topLeft() - QPointF(0, offsetHeight);
                int pos = layout->hitTest(point, Qt::FuzzyHit);
                QTextCursor cursor(info.doc);
                cursor.setPosition(pos);
                // 选取后续内容
                int endPos = layout->hitTest(docView.bottomLeft(), Qt::FuzzyHit);
                endPos = qMin(endPos + 1000, info.doc->characterCount() - 1);
                cursor.setPosition(endPos, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

                // 创建临时文档
                QTextDocument *tmpDoc = createNewDocument(info.doc);
                QTextCursor insertCursor(tmpDoc);
                insertCursor.beginEditBlock();
                insertCursor.insertFragment(QTextDocumentFragment(cursor));
                insertCursor.endEditBlock();

                auto margin = info.doc->rootFrame()->frameFormat().margin();
                auto fmt = tmpDoc->rootFrame()->frameFormat();
                fmt.setMargin(margin);
                tmpDoc->rootFrame()->setFrameFormat(fmt);

                tmpDoc->setPageSize(body.size());
                // 重新高亮文本
                auto newHighlighter = new CSyntaxHighlighter(tmpDoc);
                newHighlighter->setDefinition(info.highlighter->definition());
                newHighlighter->setTheme(info.highlighter->theme());
                rehighlightPrintDoc(tmpDoc, newHighlighter);

                // 确保布局完成
                tmpDoc->pageCount();

                // 调整显示位置
                painter->resetTransform();
                painter->translate(body.left(), body.top() - offsetHeight);
                QAbstractTextDocumentLayout::PaintContext ctx;
                ctx.clip = QRectF(0, offsetHeight, body.width(), body.height());
                ctx.palette.setColor(QPalette::Text, Qt::black);
                tmpDoc->documentLayout()->setPaintDevice(painter->device());
                tmpDoc->documentLayout()->draw(painter, ctx);

                delete newHighlighter;
                delete tmpDoc;
            } else {
                QAbstractTextDocumentLayout::PaintContext ctx;
                ctx.clip = docView;
                ctx.palette.setColor(QPalette::Text, Qt::black);
                // 绘制文档
                layout->draw(painter, ctx);
            }

            break;
        }
        docIndex -= info.doc->pageCount();
    }

    painter->restore();

}

Window::Window(DMainWindow *parent)
    : DMainWindow(parent),
      m_centralWidget(new QWidget),
      m_editorWidget(new QStackedWidget),
      m_centralLayout(new QVBoxLayout(m_centralWidget)),
      m_tabbar(new Tabbar),
      m_jumpLineBar(new JumpLineBar()),
      m_replaceBar(new ReplaceBar(this)),
      m_themePanel(new ThemePanel(this)),
      m_findBar(new FindBar(this)),
      m_menu(new DMenu),
      m_blankFileDir(QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first()).filePath("blank-files")),
      m_backupDir(QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first()).filePath("backup-files")),
      m_autoBackupDir(QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first()).filePath("autoBackup-files")),
      m_titlebarStyleSheet(titlebar()->styleSheet()),
      m_themePath(Settings::instance()->settings->option("advance.editor.theme")->value().toString())
{
    qDebug() << "Window constructor called";

    qRegisterMetaType<TextEdit *>("TextEdit");
    m_rootSaveDBus = new DBusDaemon::dbus("com.deepin.editor.daemon", "/", QDBusConnection::systemBus(), this);
    m_settings = Settings::instance();

    // Init.
    setAcceptDrops(true);
    loadTheme(m_themePath);
    DGuiApplicationHelper *guiAppHelp = DGuiApplicationHelper::instance();
    slotLoadContentTheme(guiAppHelp->themeType());

    //关闭　替换　查找 跳行bar
    connect(this, &Window::pressEsc, m_replaceBar, &ReplaceBar::pressEsc, Qt::QueuedConnection);
    connect(this, &Window::pressEsc, m_findBar, &FindBar::pressEsc, Qt::QueuedConnection);
    connect(this, &Window::pressEsc, m_jumpLineBar, &JumpLineBar::pressEsc, Qt::QueuedConnection);

    // Init settings.
    connect(m_settings, &Settings::sigAdjustFont, this, &Window::slotSigAdjustFont);
    connect(m_settings, &Settings::sigAdjustFontSize, this, &Window::slotSigAdjustFontSize);
    connect(m_settings, &Settings::sigAdjustTabSpaceNumber, this, &Window::slotSigAdjustTabSpaceNumber);
    connect(m_settings, &Settings::sigThemeChanged, this, &Window::slotSigThemeChanged);
    connect(m_settings, &Settings::sigAdjustWordWrap, this, &Window::slotSigAdjustWordWrap);
    connect(m_settings, &Settings::sigSetLineNumberShow, this, &Window::slotSigSetLineNumberShow);
    connect(m_settings, &Settings::sigAdjustBookmark, this, &Window::slotSigAdjustBookmark);
    connect(m_settings, &Settings::sigShowBlankCharacter, this, &Window::slotSigShowBlankCharacter);
    connect(m_settings, &Settings::sigHightLightCurrentLine, this, &Window::slotSigHightLightCurrentLine);
    connect(m_settings, &Settings::sigShowCodeFlodFlag, this, &Window::slotSigShowCodeFlodFlag);
    /* 设置页面里窗口模式的设置是针对新创建窗口的设置，本窗口不需要实时响应，暂且屏蔽此信号的绑定 */
    //connect(m_settings, &Settings::sigChangeWindowSize, this, &Window::slotSigChangeWindowSize);

    // Init layout and editor.
    m_centralLayout->setContentsMargins(0, 0, 0, 0);
    m_centralLayout->setSpacing(0);

    m_centralLayout->addWidget(m_editorWidget);
    setWindowIcon(QIcon::fromTheme("deepin-editor"));
    setCentralWidget(m_centralWidget);

    // Init titlebar.
    if (titlebar()) {
        qDebug() << "init titlebar";
        initTitlebar();
    }

    // window minimum size.
    setMinimumSize(680, 300);
    // resize window size.
    int window_width = Settings::instance()->settings->option("advance.window.window_width")->value().toInt();
    int window_height = Settings::instance()->settings->option("advance.window.window_height")->value().toInt();
    window_height == 1 ? window_height = 600 : window_height;
    window_width == 1 ? window_width = 1000 : window_width;
    resize(window_width, window_height);

    //设置函数最大化或者正常窗口的初始化　2021.4.26 ut002764 lxp   fix bug:74774
    showCenterWindow(true);

    // Init find bar.
    connect(m_findBar, &FindBar::findNext, this, &Window::handleFindNextSearchKeyword, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::findPrev, this, &Window::handleFindPrevSearchKeyword, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::updateSearchKeyword, this, [ = ](QString file, QString keyword) {
        handleUpdateSearchKeyword(m_findBar, file, keyword);
    });
    connect(m_findBar, &FindBar::sigFindbarClose, this, &Window::slotFindbarClose, Qt::QueuedConnection);
    
    connect(m_findBar, &FindBar::sigSwitchToReplaceBar, this, &Window::slotSwitchToReplaceBar, Qt::QueuedConnection);

    // Init replace bar.
    //connect(m_replaceBar, &ReplaceBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);

    connect(m_replaceBar, &ReplaceBar::beforeReplace, this, &Window::slot_beforeReplace, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceAll, this, &Window::handleReplaceAll, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceNext, this, &Window::handleReplaceNext, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceRest, this, &Window::handleReplaceRest, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceSkip, this, &Window::handleReplaceSkip, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::updateSearchKeyword, this, [ = ](QString file, QString keyword) {
        handleUpdateSearchKeyword(m_replaceBar, file, keyword);
    });
    connect(m_replaceBar, &ReplaceBar::sigReplacebarClose, this, &Window::slotReplacebarClose, Qt::QueuedConnection);

    // Init jump line bar.
    //QTimer::singleShot(0, m_jumpLineBar, SLOT(hide()));
    m_jumpLineBar->hide();
    m_jumpLineBar->setParent(this);

    connect(m_jumpLineBar, &JumpLineBar::jumpToLine, this, &Window::handleJumpLineBarJumpToLine, Qt::QueuedConnection);
    connect(m_jumpLineBar, &JumpLineBar::backToPosition, this, &Window::handleBackToPosition, Qt::QueuedConnection);
    connect(m_jumpLineBar, &JumpLineBar::lostFocusExit, this, &Window::handleJumpLineBarExit, Qt::QueuedConnection);

    // Make jump line bar pop at top-right of editor.
    //DAnchorsBase::setAnchor(m_jumpLineBar, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    //DAnchorsBase::setAnchor(m_jumpLineBar, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);

    DAnchors<JumpLineBar> anchors_jumpLineBar(m_jumpLineBar);
    anchors_jumpLineBar.setAnchor(Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    anchors_jumpLineBar.setAnchor(Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);
    anchors_jumpLineBar.setTopMargin(5);
    anchors_jumpLineBar.setRightMargin(5);
    m_jumpLineBar->raise();

    // Init findbar panel.
    static DAnchors<FindBar> anchors_findbar(m_findBar);
    anchors_findbar.setAnchor(Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    anchors_findbar.setAnchor(Qt::AnchorHorizontalCenter, m_centralWidget, Qt::AnchorHorizontalCenter);
    anchors_findbar.setBottomMargin(1);
    //m_findBar->move(QPoint(10, height() - 5));
    m_findBar->raise();

    // Init replaceBar panel.
    DAnchors<ReplaceBar> anchors_replaceBar(m_replaceBar);
    anchors_replaceBar.setAnchor(Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    anchors_replaceBar.setAnchor(Qt::AnchorHorizontalCenter, m_centralWidget, Qt::AnchorHorizontalCenter);
    anchors_replaceBar.setBottomMargin(1);
    //m_replaceBar->move(QPoint(10, height() - 57));
    m_replaceBar->raise();

    // Init theme panel.
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);

    // for the first time open the need be init.
    m_themePanel->setSelectionTheme(m_themePath);

    connect(m_themePanel, &ThemePanel::themeChanged, this, &Window::themeChanged);
    connect(this, &Window::requestDragEnterEvent, this, &Window::dragEnterEvent);
    connect(this, &Window::requestDropEvent, this, &Window::dropEvent);

    connect(qApp, &QGuiApplication::focusWindowChanged, this, &Window::handleFocusWindowChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &Window::slotLoadContentTheme);

    //setChildrenFocus(false);
    Utils::clearChildrenFocus(m_tabbar);//使用此函数把tabbar的组件焦点去掉(左右箭头不能focus)

#ifdef DTKWIDGET_CLASS_DSizeMode
    // 适配紧凑模式更新，注意 Qt::QueuedConnection 需要在其他子组件更新后触发
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, &Window::updateSizeMode, Qt::QueuedConnection);
#endif
    qDebug() << "Window constructor end";
}

Window::~Window()
{
    qDebug() << "Window destructor called";

    // We don't need clean pointers because application has exit here.
    if (nullptr != m_shortcutViewProcess) {
        qDebug() << "delete m_shortcutViewProcess";
        delete (m_shortcutViewProcess);
        m_shortcutViewProcess = nullptr;
    }

    clearPrintTextDocument();
    qDebug() << "Window destructor end";
}

void Window::loadIflytekaiassistantConfig()
{
    qDebug() << "loadIflytekaiassistantConfig";
    QString configPath = QString("%1/%2")
                         .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
                         .arg("iflytek");

    QDir dir(configPath);
    if (!dir.exists()) {
        qDebug() << "iflytekaiassistant config dir not exist, return";
        return;
    }
    QString key = "base/enable";
    dir.setFilter(QDir::Files);
    QStringList nameList = dir.entryList();
    for (auto name : nameList) {
        if (name.contains("-iat") || name.contains("-tts") || name.contains("-trans")) {
            QString filename = configPath + "/" + name;
            QSettings file(filename, QSettings::IniFormat);
            m_IflytekAiassistantState[name.split(".").first()] = file.value(key).toBool();
        }
    }
    qDebug() << "loadIflytekaiassistantConfig end";
}

bool Window::getIflytekaiassistantConfig(const QString &mode_name)
{
    qDebug() << "getIflytekaiassistantConfig";
    if (m_IflytekAiassistantState.contains(mode_name)) {
        qDebug() << "getIflytekaiassistantConfig end";
        return m_IflytekAiassistantState[mode_name];
    } else {
        qWarning() << "mode_name is not valid";
        return false;
    }
}

void Window::updateModifyStatus(const QString &path, bool isModified)
{
    qDebug() << "updateModifyStatus";
    int tabIndex = m_tabbar->indexOf(path);
    if (tabIndex == -1) {
        qWarning() << "Failed to get tab index for path:" << path;
        return;
    }

    QString tabName;
    QString filePath = m_tabbar->truePathAt(tabIndex);
    if (filePath.isNull() || filePath.length() <= 0 || Utils::isDraftFile(filePath)) {
        qDebug() << "Failed to get true path for tab index:" << tabIndex;
        tabName = m_tabbar->textAt(tabIndex);
        if (isModified) {
            qDebug() << "Modified but not saved, add * to tab name";
            if (!tabName.contains('*')) {
                tabName.prepend('*');
            }
        } else {
            QRegularExpression reg("[^*](.+)");
            QRegularExpressionMatch match = reg.match(tabName);
            if (match.hasMatch()) {
                qDebug() << "Tab name does not contain *, remove * from tab name";
                tabName = match.captured(0);
            }
        }
    } else {
        qDebug() << "File path is valid, updating tab name";
        QFileInfo fileInfo(filePath);
        tabName = fileInfo.fileName();
        if (isModified) {
            qDebug() << "Modified but not saved, add * to tab name";
            tabName.prepend('*');
        }
    }

    m_tabbar->setTabText(tabIndex, tabName);
    //判断是否需要阻塞系统关机
    emit sigJudgeBlockShutdown();
    qDebug() << "updateModifyStatus end";
}

void Window::updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath)
{
    qDebug() << "updateSaveAsFileName called with old path:" << strOldFilePath << "new path:" << strNewFilePath;
    int tabIndex = m_tabbar->indexOf(strOldFilePath);
    EditWrapper *wrapper = m_wrappers.value(strOldFilePath);
    if (!wrapper) {
        qWarning() << "Failed to get wrapper for old path";
        return;
    }
    m_tabbar->updateTab(tabIndex, strNewFilePath, QFileInfo(strNewFilePath).fileName());
    wrapper->updatePath(strNewFilePath);

    //tabbar中存在和strNewFilePath同名的tab
    if (m_wrappers.contains(strNewFilePath)) {
        qDebug() << "Tab with new file path already exists, closing old tab.";
        closeTab(strNewFilePath);
    }

    m_wrappers.remove(strOldFilePath);
    m_wrappers.insert(strNewFilePath, wrapper);
    qDebug() << "updateSaveAsFileName end";
}

void Window::updateSabeAsFileNameTemp(QString strOldFilePath, QString strNewFilePath)
{
    qDebug() << "updateSabeAsFileNameTemp called with old path:" << strOldFilePath << "new path:" << strNewFilePath;
    int tabIndex = m_tabbar->indexOf(strOldFilePath);
    EditWrapper *wrapper = m_wrappers.value(strOldFilePath);
    if (!wrapper) {
        qWarning() << "Failed to get wrapper for old path";
        return;
    }
    m_tabbar->updateTab(tabIndex, strNewFilePath, QFileInfo(strNewFilePath).fileName());
    wrapper->updatePath(strNewFilePath);
    m_wrappers.remove(strOldFilePath);
    m_wrappers.insert(strNewFilePath, wrapper);
    qDebug() << "updateSabeAsFileNameTemp end";
}

void Window::showCenterWindow(bool bIsCenter)
{
    qDebug() << "Window::showCenterWindow() - bIsCenter:" << bIsCenter;

    // Init window state with config.
    // Below code must before this->titlebar()->setMenu, otherwise main menu can't display pre-build-in menu items by dtk.
    QString windowState = Settings::instance()->settings->option("advance.window.windowstate")->value().toString();

    if (bIsCenter) {
        qDebug() << "Window::showCenterWindow() - bIsCenter is true";
        Dtk::Widget::moveToCenter(this);
    }
    // init window state.
    if (windowState == "window_maximum") {
        qDebug() << "Window::showCenterWindow() - setting window to maximized state";
        showMaximized();
        m_needMoveToCenter = true;
    } else if (windowState == "fullscreen") {
        qDebug() << "Window::showCenterWindow() - setting window to fullscreen state";
        showFullScreen();
        m_needMoveToCenter = true;
    } else {
        qDebug() << "Window::showCenterWindow() - setting window to normal state";
        showNormal();
    }
    qDebug() << "Window::showCenterWindow() - end";
}

void Window::initTitlebar()
{
    qDebug() << "initTitlebar called";
    QAction *newWindowAction(new QAction(tr("New window"), this));
    QAction *newTabAction(new QAction(tr("New tab"), this));
    QAction *openFileAction(new QAction(tr("Open file"), this));
    QAction *saveAction(new QAction(tr("Save"), this));
    QAction *saveAsAction(new QAction(tr("Save as"), this));
    QAction *printAction(new QAction(tr("Print"), this));
    QAction *switchThemeAction(new QAction(tr("Switch theme"), this));
    QAction *settingAction(new QAction(tr("Settings"), this));
    QAction *findAction(new QAction(QApplication::translate("TextEdit", "Find"), this));
    QAction *replaceAction(new QAction(QApplication::translate("TextEdit", "Replace"), this));

    m_menu->addAction(newWindowAction);
    m_menu->addAction(newTabAction);
    m_menu->addAction(openFileAction);
    m_menu->addSeparator();
    m_menu->addAction(findAction);
    m_menu->addAction(replaceAction);
    m_menu->addAction(saveAction);
    m_menu->addAction(saveAsAction);
    m_menu->addAction(printAction);
    //此接口不可删除，预留的编辑器内部主题选择接口
    //m_menu->addAction(switchThemeAction);
    m_menu->addSeparator();
    m_menu->addAction(settingAction);

    m_menu->setMinimumWidth(150);

    titlebar()->addWidget(m_tabbar);

    titlebar()->setCustomWidget(m_tabbar, false);
    titlebar()->setSeparatorVisible(false);
    titlebar()->setMenu(m_menu);
    titlebar()->setIcon(QIcon::fromTheme("deepin-editor"));
    titlebar()->setFocusPolicy(Qt::NoFocus);         //设置titlebar无焦点，点击titlebar时光标不移动

    DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
    addButton->setFocusPolicy(Qt::NoFocus);
    DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
    optionBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
    minBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
    quitFullBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
    maxBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");
    closeBtn->setFocusPolicy(Qt::NoFocus);

    connect(m_tabbar, &DTabBar::tabBarDoubleClicked, titlebar(), &DTitlebar::doubleClicked, Qt::QueuedConnection);

    connect(m_tabbar, &Tabbar::closeTabs, this, &Window::handleTabsClosed, Qt::QueuedConnection);
    connect(m_tabbar, &Tabbar::requestHistorySaved, this, [ = ](const QString & filePath) {
        // 单个Tab页关闭文件时记录文件信息
        if (StartManager::instance()->checkPath(filePath)) {
            qDebug() << "Recording close file history for file:" << filePath;
            Utils::recordCloseFile(filePath);
        }

        if (QFileInfo(filePath).dir().absolutePath() == m_blankFileDir) {
            qDebug() << "File is a blank file, not recording in history:" << filePath;
            return;
        }

        if (!m_closeFileHistory.contains(filePath)) {
            qDebug() << "Adding file to close file history:" << filePath;
            m_closeFileHistory << filePath;
        }
    });

    connect(m_tabbar, &DTabBar::tabCloseRequested, this, &Window::handleTabCloseRequested, Qt::QueuedConnection);
    connect(m_tabbar, &DTabBar::tabAddRequested, this, static_cast<void (Window::*)()>(&Window::addBlankTab), Qt::QueuedConnection);
    connect(m_tabbar, &DTabBar::currentChanged, this, &Window::handleCurrentChanged, Qt::QueuedConnection);
    // 当标签页新增、删除时触发变更信号(用于备份当前打开文件信息)
    connect(m_tabbar, &DTabBar::tabIsInserted, this, &Window::tabChanged, Qt::QueuedConnection);
    connect(m_tabbar, &DTabBar::tabIsRemoved, this, &Window::tabChanged, Qt::QueuedConnection);

    connect(newWindowAction, &QAction::triggered, this, &Window::newWindow);
    connect(newTabAction, &QAction::triggered, this, static_cast<void (Window::*)()>(&Window::addBlankTab));
    connect(openFileAction, &QAction::triggered, this, &Window::openFile);
    connect(findAction, &QAction::triggered, this, &Window::popupFindBar);
    connect(replaceAction, &QAction::triggered, this, &Window::popupReplaceBar);
    connect(saveAction, &QAction::triggered, this, &Window::saveFile);
    connect(saveAsAction, &QAction::triggered, this, &Window::saveAsFile);
    connect(printAction, &QAction::triggered, this, &Window::popupPrintDialog);
    connect(settingAction, &QAction::triggered, this, &Window::popupSettingsDialog);
    connect(switchThemeAction, &QAction::triggered, this, &Window::popupThemePanel);
    qDebug() << "initTitlebar end";
}

bool Window::checkBlockShutdown()
{
    qDebug() << "checkBlockShutdown called";
    //判断是否有未保存的tab项
    for (int i = 0; i < m_tabbar->count(); i++) {
        if (m_tabbar->textAt(i).isNull()) {
            qDebug() << "Tab name is null, return false";
            return false;
        }
        //如果有未保存的tab项，return true阻塞系统关机
        if (m_tabbar->textAt(i).at(0) == '*') {
            qDebug() << "There are unsaved tabs, return true";
            return true;
        }
    }

    qDebug() << "There are no unsaved tabs, return false";
    return false;
}

int Window::getTabIndex(const QString &file)
{
    qDebug() << "getTabIndex called with file:" << file;
    return m_tabbar->indexOf(file);
}

void Window::activeTab(int index)
{
    qDebug() << "activeTab called with index:" << index;
    DMainWindow::activateWindow();
    m_tabbar->setCurrentIndex(index);
    qDebug() << "activeTab end";
}

Tabbar *Window::getTabbar()
{
    return m_tabbar;
}

void Window::addTab(const QString &filepath, bool activeTab)
{
    qDebug() << "Enter addTab, filepath:" << filepath << "activeTab:" << activeTab;
    // check whether it is an editable file thround mimeType.
    if (Utils::isMimeTypeSupport(filepath)) {
        const QFileInfo fileInfo(filepath);
        QString tabName = fileInfo.fileName();
        qDebug() << "File is supported, name:" << tabName << "writable:" << fileInfo.isWritable() << "readable:" << fileInfo.isReadable();
        EditWrapper *curWrapper = currentWrapper();

        if (!fileInfo.isWritable() && fileInfo.isReadable()) {
            tabName += QString(" (%1)").arg(tr("Read-Only"));
            qDebug() << "File is read-only, updated tab name:" << tabName;
        }

        if (curWrapper) {
            // if the current page is a draft file and is empty
            if (m_tabbar->indexOf(filepath) != -1) {
                qDebug() << "File already open in tab index:" << m_tabbar->indexOf(filepath) << ", activating tab";
                m_tabbar->setCurrentIndex(m_tabbar->indexOf(filepath));
            }
        }

        // check if have permission to read the file.
        QFile file(filepath);
        QFile::Permissions permissions = file.permissions();
        bool bIsRead = (permissions & QFile::ReadUser || permissions & QFile::ReadOwner || permissions & QFile::ReadOther);
        qDebug() << "File permissions:" << permissions << "readable:" << bIsRead;

        if (fileInfo.exists() && !bIsRead) {
            qWarning() << "No permission to read file:" << filepath;
            DMessageManager::instance()->sendMessage(m_editorWidget->currentWidget(), QIcon(":/images/warning.svg")
                                                     , QString(tr("You do not have permission to open %1")).arg(filepath));
            return;
        }

        if (StartManager::instance()->checkPath(filepath)) {
            qDebug() << "Path check passed, adding tab for file";
            m_tabbar->addTab(filepath, tabName, filepath);

            if (!m_wrappers.contains(filepath)) {
                qDebug() << "Creating new editor wrapper for file";
                EditWrapper *wrapper = createEditor();
                showNewEditor(wrapper);
                m_wrappers[filepath] = wrapper;

                if (!fileInfo.isWritable() && fileInfo.isReadable()) {
                    qDebug() << "Setting read-only permission for editor";
                    wrapper->textEditor()->setReadOnlyPermission(true);
                }

                qDebug() << "Opening file in editor";
                wrapper->openFile(filepath, filepath);

                // 查找文件是否存在书签
                auto bookmarkInfo = StartManager::instance()->findBookmark(filepath);
                wrapper->textEditor()->setBookMarkList(bookmarkInfo);
            }
            // Activate window.
            activateWindow();
            qDebug() << "Window activated";
        } else {
            qDebug() << "Path check failed, file may already be open in another window";
        }

        // Active tab if activeTab is true.
        if (activeTab) {
            int tabIndex = m_tabbar->indexOf(filepath);
            if (tabIndex != -1) {
                qDebug() << "Activating tab at index:" << tabIndex;
                m_tabbar->setCurrentIndex(tabIndex);
            } else {
                qWarning() << "Failed to find tab for filepath:" << filepath;
            }
        }
    } else {
        qWarning() << "File is not supported by MIME type:" << filepath;
        if (currentWrapper() == nullptr) {
            qDebug() << "No current wrapper, adding blank tab";
            this->addBlankTab();
        }

        QString strFileName = QFileInfo(filepath).fileName();
        strFileName = tr("Invalid file: %1").arg(strFileName);
        strFileName = Utils::lineFeed(strFileName, m_editorWidget->currentWidget()->width() - FLOATTIP_MARGIN, m_editorWidget->currentWidget()->font(), 2);
        qDebug() << "Showing invalid file notification";
        DMessageManager::instance()->sendMessage(m_editorWidget->currentWidget(), QIcon(":/images/warning.svg"), strFileName);
    }
    qDebug() << "Exit addTab";
}

void Window::addTabWithWrapper(EditWrapper *wrapper, const QString &filepath, const QString &qstrTruePath, const QString &tabName, int index)
{
    qDebug() << "Enter addTabWithWrapper, filepath:" << filepath << "truePath:" << qstrTruePath << "tabName:" << tabName << "index:" << index;

    if (index == -1) {
        index = m_tabbar->count();
        qDebug() << "Index not specified, using tab count:" << index;
    }

    //这里会重复连接信号和槽，先全部取消
    QDBusConnection dbus = QDBusConnection::sessionBus();
    qDebug() << "Disconnecting previous DBus signals for gesture events";
    switch (Utils::getSystemVersion()) {
    case Utils::V23:
        qDebug() << "Using V23 DBus path for gesture events";
        dbus.systemBus().disconnect("org.deepin.dde.Gesture1",
                                    "/org/deepin/dde/Gesture1", "org.deepin.dde.Gesture1",
                                    "Event",
                                    wrapper->textEditor(), SLOT(fingerZoom(QString, QString, int)));
        break;
    default:
        qDebug() << "Using default DBus path for gesture events";
        dbus.systemBus().disconnect("com.deepin.daemon.Gesture",
                                    "/com/deepin/daemon/Gesture", "com.deepin.daemon.Gesture",
                                    "Event",
                                    wrapper->textEditor(), SLOT(fingerZoom(QString, QString, int)));
        break;
    }

    qDebug() << "Disconnecting all signals from text editor";
    wrapper->textEditor()->disconnect();

    qDebug() << "Connecting signals to text editor";
    connect(wrapper->textEditor(), &TextEdit::cursorModeChanged, wrapper, &EditWrapper::handleCursorModeChanged);
    connect(wrapper->textEditor(), &TextEdit::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::signal_setTitleFocus, this, &Window::slot_setTitleFocus, Qt::QueuedConnection);

    qDebug() << "Reconnecting DBus signals for gesture events";
    switch (Utils::getSystemVersion()) {
    case Utils::V23:
        qDebug() << "Connecting to V23 DBus path for gesture events";
        dbus.systemBus().connect("org.deepin.dde.Gesture1",
                                 "/org/deepin/dde/Gesture1", "org.deepin.dde.Gesture1",
                                 "Event",
                                 wrapper->textEditor(), SLOT(fingerZoom(QString, QString, int)));
        break;
    default:
        qDebug() << "Connecting to default DBus path for gesture events";
        dbus.systemBus().connect("com.deepin.daemon.Gesture",
                                 "/com/deepin/daemon/Gesture", "com.deepin.daemon.Gesture",
                                 "Event",
                                 wrapper->textEditor(), SLOT(fingerZoom(QString, QString, int)));
        break;
    }

    connect(wrapper->textEditor(), &QPlainTextEdit::cursorPositionChanged, wrapper->textEditor(), &TextEdit::cursorPositionChanged);

    connect(wrapper->textEditor(), &QPlainTextEdit::textChanged, wrapper->textEditor(), [ = ]() {
        wrapper->UpdateBottomBarWordCnt(wrapper->textEditor()->characterCount());
    });

    qDebug() << "Adding tab to tabbar at index:" << index << "with filepath:" << filepath << "tabName:" << tabName;
    // add wrapper to this window.
    m_tabbar->addTabWithIndex(index, filepath, tabName, qstrTruePath);
    m_wrappers[filepath] = wrapper;
    wrapper->updatePath(filepath, qstrTruePath);

    qDebug() << "Showing new editor and applying theme";
    showNewEditor(wrapper);
    wrapper->OnThemeChangeSlot(m_themePath);
    qDebug() << "Exit addTabWithWrapper";
}

/**
 * @return 关闭当前焦点的标签页并返回是否成功关闭。
 */
bool Window::closeTab()
{
    qDebug() << "Enter closeTab (no arguments)";
    const QString &filePath = m_tabbar->currentPath();
    // 避免异常情况重入时当前已无标签页的情况
    if (filePath.isEmpty()) {
        qWarning() << "Current tab path is empty, cannot close tab";
        return false;
    }
    qDebug() << "Closing current tab with path:" << filePath;
    return closeTab(filePath);
}

bool Window::closeTab(const QString &filePath)
{
    qDebug() << "Enter closeTab with path:" << filePath;
    EditWrapper *wrapper = m_wrappers.value(filePath);
    if (!wrapper) {
        qWarning() << "No wrapper found for path:" << filePath;
        return false;
    }

    if (!currentWrapper()) {
        qWarning() << "No current wrapper, cannot close tab";
        return false;
    }

    if (m_reading_list.contains(currentWrapper()->textEditor())) {
        qDebug() << "Current editor is in reading list, syncing settings";
        if (m_settings->settings) {
            m_settings->settings->sync();
        }

        if (IflytekAiAssistant::instance()->valid()) {
            qDebug() << "Stopping TTS playback";
            IflytekAiAssistant::instance()->stopTtsDirectly();
        }
    }

    // 当前窗口为打印文本窗口
    if (wrapper == m_printWrapper) {
        qDebug() << "Closing print wrapper, exiting print mode";
        // 退出打印
        m_bPrintProcessing = false;
    }

    qDebug() << "Disconnecting all signals from wrapper";
    disconnect(wrapper, nullptr);
    disconnect(wrapper->textEditor(), &TextEdit::textChanged, nullptr, nullptr);

    // this property holds whether the document has been modified by the user
    bool isModified = wrapper->isModified();
    bool isDraftFile = wrapper->isDraftFile();
    bool bIsBackupFile = false;
    qDebug() << "Tab state - modified:" << isModified << "draft file:" << isDraftFile;

    if (wrapper->isTemFile()) {
        bIsBackupFile = true;
        qDebug() << "Tab is a temporary backup file";
    }

    if (wrapper->getFileLoading()) {
        qDebug() << "File is still loading, setting modified to false";
        isModified = false;
    }

    // 关闭标签页前，记录全局的书签信息
    QList<int> bookmarkInfo = wrapper->textEditor()->getBookmarkInfo();
    if (!bookmarkInfo.isEmpty()) {
        qDebug() << "Saving bookmarks for file:" << bookmarkInfo;
        QString localPath = wrapper->textEditor()->getTruePath();
        StartManager::instance()->recordBookmark(localPath, bookmarkInfo);
    }

    if (isDraftFile) {
        if (isModified) {
            qDebug() << "Draft file is modified, prompting to save";
            DDialog *dialog = createDialog(tr("Do you want to save this file?"), "");
            int res = dialog->exec();

            //取消或关闭弹窗不做任务操作
            if (res == 0 || res == -1) {
                qDebug() << "Dialog cancelled or closed, not performing any actions";
                return false;
            }

            //不保存
            if (res == 1) {
                qDebug() << "User chose not to save, removing wrapper and closing tab";
                removeWrapper(filePath, true);
                m_tabbar->closeCurrentTab(filePath);
                QFile(filePath).remove();
                return true;
            }

            //保存
            if (res == 2) {
                QFileInfo fileInfo(filePath);
                QString newFilePath;

                if (wrapper->saveDraftFile(newFilePath)) {
                    qDebug() << "Draft file saved, removing wrapper and closing tab";
                    removeWrapper(filePath, true);
                    // 保存临时文件后已更新tab页的文件路径，使用新的文件路径删除窗口
                    m_tabbar->closeCurrentTab(newFilePath);
                    QFile(filePath).remove();
                } else {
                    // 保存不成功时不关闭窗口
                    qDebug() << "Draft file save failed, not closing tab";
                    return false;
                }
            }
        } else {
            qDebug() << "Draft file is not modified, removing wrapper and closing tab";
            removeWrapper(filePath, true);
            m_tabbar->closeCurrentTab(filePath);
            QFile(filePath).remove();
        }
    }
    // document has been modified or unsaved draft document.
    // need to prompt whether to save.
    else {
        QFileInfo fileInfo(filePath);
        isModified = false;
        if (m_tabbar->textAt(m_tabbar->currentIndex()).front() == "*") {
            qDebug() << "Tab name starts with '*', indicating modified state";
            isModified = true;
        }
        if (isModified) {
            qDebug() << "File is modified, prompting to save";
            DDialog *dialog = createDialog(tr("Do you want to save this file?"), "");
            int res = dialog->exec();

            //取消或关闭弹窗不做任务操作
            if (res == 0 || res == -1) {
                qDebug() << "Dialog cancelled or closed, not performing any actions";
                return false;
            }

            //不保存
            if (res == 1) {
                qDebug() << "User chose not to save, removing wrapper and closing tab";
                removeWrapper(filePath, true);
                m_tabbar->closeCurrentTab(filePath);

                //删除备份文件
                if (bIsBackupFile) {
                    qDebug() << "Deleting backup file";
                    QFile(filePath).remove();
                }

                //删除自动备份文件
                if (QFileInfo(m_autoBackupDir).exists()) {
                    qDebug() << "Deleting auto backup file";
                    fileInfo.setFile(wrapper->textEditor()->getTruePath());
                    QString name = fileInfo.absolutePath().replace("/", "_");
                    QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
                }

                qDebug() << "Exit closeTab, return true";
                return true;
            }

            //保存
            if (res == 2) {
                if (bIsBackupFile) {
                    if (wrapper->saveFile()) {
                        qDebug() << "File saved, removing wrapper and closing tab";
                        removeWrapper(filePath, true);
                        m_tabbar->closeCurrentTab(filePath);
                        QFile(filePath).remove();
                    } else {
                        qDebug() << "File save failed, not closing tab";
                        saveAsFile();
                    }
                } else {
                    qDebug() << "File is not a backup file, saving normally";
                    if (wrapper->saveFile()) {
                        qDebug() << "File saved, removing wrapper and closing tab";
                        removeWrapper(filePath, true);
                        m_tabbar->closeCurrentTab(filePath);
                    } else {
                        qDebug() << "File save failed, not closing tab";
                        saveAsFile();
                    }
                }
            }
        } else {
            qDebug() << "File is not modified, removing wrapper and closing tab";
            removeWrapper(filePath, true);
            m_tabbar->closeCurrentTab(filePath);
        }

        //删除自动备份文件
        if (QFileInfo(m_autoBackupDir).exists()) {
            qDebug() << "Deleting auto backup file";
            fileInfo.setFile(wrapper->textEditor()->getTruePath());
            QString name = fileInfo.absolutePath().replace("/", "_");
            QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
        }
    }

    qDebug() << "Exit closeTab, return true";
    return true;
}

void Window::restoreTab()
{
    qDebug() << "Enter restoreTab";
    if (m_closeFileHistory.size() > 0) {
        qDebug() << "Restoring tab from history";
        addTab(m_closeFileHistory.takeLast());
    }
    qDebug() << "Exit restoreTab";
}

EditWrapper *Window::createEditor()
{
    qDebug() << "Creating new EditWrapper instance";
    EditWrapper *wrapper = new EditWrapper(this);
    connect(wrapper, &EditWrapper::sigClearDoubleCharaterEncode, this, &Window::slotClearDoubleCharaterEncode);
    connect(wrapper->textEditor(), &TextEdit::signal_readingPath, this, &Window::slot_saveReadingPath, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::signal_setTitleFocus, this, &Window::slot_setTitleFocus, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::textChanged, this, [ = ]() {
        updateJumpLineBar(wrapper->textEditor());
    }, Qt::QueuedConnection);

    bool wordWrap = m_settings->settings->option("base.font.wordwrap")->value().toBool();
    wrapper->textEditor()->m_pIsShowCodeFoldArea = m_settings->settings->option("base.font.codeflod")->value().toBool();
    wrapper->OnThemeChangeSlot(m_themePath);
    wrapper->textEditor()->setSettings(m_settings);
    wrapper->textEditor()->setTabSpaceNumber(m_settings->settings->option("advance.editor.tabspacenumber")->value().toInt());
    wrapper->textEditor()->setFontFamily(m_settings->settings->option("base.font.family")->value().toString());
    wrapper->textEditor()->setLineWrapMode(wordWrap);
    setFontSizeWithConfig(wrapper);

    //设置显示空白符　梁卫东
    wrapper->setShowBlankCharacter(m_settings->settings->option("base.font.showblankcharacter")->value().toBool());
    //yanyuhan 设置行号显示
    wrapper->setLineNumberShow(m_settings->settings->option("base.font.showlinenumber")->value().toBool(), true);
    wrapper->textEditor()->setHighLineCurrentLine(m_settings->settings->option("base.font.hightlightcurrentline")->value().toBool());
    wrapper->textEditor()->setBookmarkFlagVisable(m_settings->settings->option("base.font.showbookmark")->value().toBool(), true);
    wrapper->textEditor()->setCodeFlodFlagVisable(m_settings->settings->option("base.font.codeflod")->value().toBool(), true);
    wrapper->textEditor()->updateLeftAreaWidget();

    qDebug() << "Creating new TextEdit instance";
    return wrapper;
}

EditWrapper *Window::currentWrapper()
{
    qDebug() << "Enter currentWrapper";
    if (m_wrappers.contains(m_tabbar->currentPath())) {
        qDebug() << "Found wrapper for current path:" << m_tabbar->currentPath();
        return m_wrappers.value(m_tabbar->currentPath());
    } else {
        qDebug() << "No wrapper found for current path:" << m_tabbar->currentPath();
        return nullptr;
    }
}

EditWrapper *Window::wrapper(const QString &filePath)
{
    qDebug() << "Enter wrapper with path:" << filePath;
    if (m_wrappers.contains(filePath)) {
        EditWrapper* wrapper = m_wrappers.value(filePath);
        // Although contains check passed, add a safety check
        if (!wrapper) {
             qWarning() << "Wrapper is null even after contains check";
             return nullptr;
        }
        qDebug() << "Found wrapper for path:" << filePath;
        return wrapper;
    } else {
        for (auto iterWrapper : m_wrappers) {
            // Check if iterWrapper itself is valid before accessing members
            if (!iterWrapper || !iterWrapper->textEditor()) continue;

            QString truePath = iterWrapper->textEditor()->getTruePath();
            if (truePath == filePath) {
                QString originalWrapperPath = iterWrapper->textEditor()->getFilePath();
                EditWrapper* foundWrapper = m_wrappers.value(originalWrapperPath);
                if (!foundWrapper) {
                    qWarning() << "Failed to get wrapper for original path when searching";
                    // Continue searching instead of returning nullptr immediately
                    continue;
                }
                qDebug() << "Found wrapper for original path:" << originalWrapperPath;
                return foundWrapper;
            }
        }

        qDebug() << "No wrapper found for path:" << filePath;
        return nullptr;
    }
}

TextEdit *Window::getTextEditor(const QString &filepath)
{
    qDebug() << "Enter getTextEditor with path:" << filepath;
    if (m_wrappers.contains(filepath)) {
        EditWrapper* wrapper = m_wrappers.value(filepath);
        if (!wrapper) {
             qWarning() << "Wrapper is null even after contains check";
             return nullptr;
        }
        qDebug() << "Found text editor for path:" << filepath;
        return wrapper->textEditor();
    } else {
        qDebug() << "No text editor found for path:" << filepath;
        return nullptr;
    }
}

void Window::focusActiveEditor()
{
    qDebug() << "Enter focusActiveEditor";
    if (m_tabbar->count() > 0) {
        if (currentWrapper() == nullptr) {
            qDebug() << "No current wrapper, adding blank tab";
            return;
        }
        currentWrapper()->textEditor()->setFocus();
    }
    qDebug() << "Exit focusActiveEditor";
}

void Window::removeWrapper(const QString &filePath, bool isDelete)
{
    qDebug() << "Enter removeWrapper with path:" << filePath << "isDelete:" << isDelete;
    EditWrapper *wrapper = m_wrappers.value(filePath);

    if (wrapper) {
        qInfo() << "begin removeWrapper";
        if (nullptr == m_editorWidget) {
            qWarning() << "m_editorWidget is null";
            return;
        }
        m_editorWidget->removeWidget(wrapper);
        m_wrappers.remove(filePath);
        if (isDelete) {
            disconnect(wrapper->textEditor(), nullptr);
            disconnect(wrapper, nullptr);
            wrapper->setQuitFlag();
            wrapper->deleteLater();
            qInfo() << "after delete wrapper";
        }
    }

    // Exit window after close all tabs.
    if (m_wrappers.isEmpty()) {
        close();
        qInfo() << "after close";
    }

    qInfo() << "end removeWrapper";
    StartManager::instance()->delayMallocTrim();
}

void Window::openFile()
{
    qDebug() << "Opening file dialog";

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    // read history directory.
    QString historyDirStr = m_settings->settings->option("advance.editor.file_dialog_dir")->value().toString();
    if (historyDirStr.isEmpty() || !QDir(historyDirStr).exists() || !QFileInfo(historyDirStr).isWritable() || !QDir(historyDirStr).isReadable()) {
        historyDirStr = QDir::homePath() + "/Documents";
        qDebug() << "History directory is empty or invalid, using default path:" << historyDirStr;
    }
    dialog.setDirectory(historyDirStr);

    QDir historyDir(historyDirStr);

    if (historyDir.exists()) {
        qDebug() << "Setting dialog directory to history directory:" << historyDir.absolutePath();
        dialog.setDirectory(historyDir);
    } else {
        qDebug() << "historyDir or default path not existed:" << historyDir;
    }

    QString path = m_settings->getSavePath(m_settings->getSavePathId());
    // 使用当前文件路径时，打开当前文件的目录，新建文档为系统-文档目录
    if (PathSettingWgt::CurFileBox == m_settings->getSavePathId()) {
        path = getCurrentOpenFilePath();
        qDebug() << "Setting dialog directory to current file path:" << path;
    }
    if (path.isEmpty() || !QDir(path).exists() || !QFileInfo(path).isWritable() || !QDir(path).isReadable()) {
        path = QDir::homePath() + "/Documents";
        qDebug() << "Save path is empty or invalid, using default path:" << path;
    }
    dialog.setDirectory(path);

    const int mode = dialog.exec();

    PerformanceMonitor::openFileStart();
    // save the directory string.
    m_settings->settings->option("advance.editor.file_dialog_dir")->setValue(dialog.directoryUrl().toLocalFile());

    if (mode != QDialog::Accepted) {
        qDebug() << "File dialog not accepted, returning";
        return;
    }

    QStringList supportfileNames;
    QStringList otherfiles;
    for (const QString &file : dialog.selectedFiles()) {
        QString canonicalFile = QFileInfo(file).canonicalFilePath();
        if (Utils::isMimeTypeSupport(canonicalFile)) {
            supportfileNames.append(canonicalFile);
        } else {
            otherfiles.append(canonicalFile);
        }

        //先添加支持的文件
    }
    foreach (QString var, supportfileNames) {
        addTab(var, true);
    }

    //后添加不支持文件　在最后编辑页面显示
    foreach (QString var, otherfiles) {
        addTab(var, true);
    }
    qDebug() << "Exit openFile";
}

bool Window::saveFile()
{
    qDebug() << "Saving current file";

    EditWrapper *wrapperEdit = currentWrapper();

    //大文本加载过程不允许保存
    if (!wrapperEdit || wrapperEdit->getFileLoading()) {
        qDebug() << "Wrapper is null or file is loading, returning";
        return false;
    }

    bool isDraftFile = wrapperEdit->isDraftFile();
    //bool isEmpty = wrapperEdit->isPlainTextEmpty();
    QString filePath = wrapperEdit->textEditor()->getTruePath();

    // save blank file.
    if (isDraftFile) {
        qDebug() << "Saving draft file";
        return saveAsFile();
    }

    QFileInfo info(filePath);
    if (info.exists()) {
        //判断文件是否有写的权限
        QFile temporaryBuffer(filePath);
        QFile::Permissions pers = temporaryBuffer.permissions();
        bool isWrite = ((pers & QFile::WriteUser) || (pers & QFile::WriteOwner) || (pers & QFile::WriteOther));
        if (!isWrite) {
            DMessageManager::instance()->sendMessage(m_editorWidget->currentWidget(), QIcon(":/images/warning.svg")
                                                     , QString(tr("You do not have permission to save %1")).arg(info.fileName()));
            qDebug() << "File does not have write permission, returning";
            return false;
        }
    }

    // save normal file.
    QString temPath = "";

    if (wrapperEdit->isTemFile()) {
        temPath = wrapperEdit->textEditor()->getFilePath();
    } else {
        temPath = filePath;
    }
    bool success = wrapperEdit->saveFile();

    if (success) {
        updateSabeAsFileNameTemp(temPath, filePath);
        if (currentWrapper()) {
            qDebug() << "Saved successfully, updating save as file name";
            currentWrapper()->hideWarningNotices();
        }
        showNotify(tr("Saved successfully"));

        //删除备份文件
        if (temPath != filePath) {
            QFile(temPath).remove();
        }

        //删除自动备份文件
        if (QFileInfo(m_autoBackupDir).exists()) {
            qDebug() << "Deleting auto backup file";
            QFileInfo fileInfo(filePath);
            QString name = fileInfo.absolutePath().replace("/", "_");
            QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
        }

        qDebug() << "Exit saveFile, return true";
        return true;
    }

    qDebug() << "Failed to save file, returning";
    return false;
}

bool Window::saveAsFile()
{
    qDebug() << "Enter saveAsFile";
    return !saveAsFileToDisk().isEmpty();
}

QString Window::saveAsFileToDisk()
{
    qDebug() << "Enter saveAsFileToDisk";
    EditWrapper *wrapper = currentWrapper();
    //大文本加载过程不允许保存　梁卫东
    if (!wrapper || wrapper->getFileLoading()) {
        qDebug() << "Wrapper is null or file is loading, returning";
        return QString();
    }

    bool isDraft = wrapper->isDraftFile();
    QFileInfo fileInfo(wrapper->textEditor()->getTruePath());

    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDirectory(QDir::homePath());
    // 允许选取保存的编码格式
    DFileDialog::DComboBoxOptions encodingOptions;
    encodingOptions.editable = false;
    encodingOptions.defaultValue = wrapper->getTextEncode();
    encodingOptions.data = Utils::getSupportEncodingList();
    dialog.addComboBox(QObject::tr("Encoding"), encodingOptions);

    QString path = m_settings->getSavePath(m_settings->getSavePathId());
    // 使用当前文件路径时，打开当前文件的目录，新建文档为系统-文档目录
    if (PathSettingWgt::CurFileBox == m_settings->getSavePathId()) {
        path = getCurrentOpenFilePath();
        qDebug() << "Setting dialog directory to current file path:" << path;
    }
    if (path.isEmpty() || !QDir(path).exists() || !QFileInfo(path).isWritable() || !QDir(path).isReadable()) {
        path = QDir::homePath() + "/Documents";
        qDebug() << "Save path is empty or invalid, using default path:" << path;
    }
    dialog.setDirectory(path);

    if (isDraft) {
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(m_tabbar->currentName());
        dialog.selectFile(match.captured(0) + ".txt");
        qDebug() << "Setting dialog file name to draft file name:" << match.captured(0) + ".txt";
    } else {
        dialog.setDirectory(fileInfo.dir());
        dialog.selectFile(fileInfo.fileName());
        qDebug() << "Setting dialog file name to current file name:" << fileInfo.fileName();
    }

    int mode = dialog.exec();
    if (mode == QDialog::Accepted) {
        qDebug() << "File dialog accepted, proceeding with save as";
        const QByteArray encode = dialog.getComboBoxValue(QObject::tr("Encoding")).toUtf8();
        const QString newFilePath = dialog.selectedFiles().value(0);
        Settings::instance()->setSavePath(PathSettingWgt::LastOptBox, QFileInfo(newFilePath).absolutePath());
        Settings::instance()->setSavePath(PathSettingWgt::CurFileBox, QFileInfo(newFilePath).absolutePath());

        // 保存原始文件路径，用于后续更新
        QString oldFilePath = wrapper->filePath();
        
        // 先保存文件，不更新路径
        bool needChangeEncode = (encode != wrapper->getTextEncode().toUtf8());
        bool saveSucc = false;
        if (QFileInfo(wrapper->filePath()).absoluteFilePath()
                == QFileInfo(newFilePath).absoluteFilePath()) {
            // 相同路径，直接保存文件
            saveSucc = wrapper->saveFile(encode);
            qDebug() << "Same file path, directly saving file";
        } else {
            saveSucc = wrapper->saveAsFile(newFilePath, encode);
            qDebug() << "Saving file as new path:" << newFilePath;
        }

        if (!saveSucc) {
            /* 如果保存未成功，无需记录更新新文件路径 */
            wrapper->updatePath(wrapper->filePath(), QString());
            qDebug() << "Save failed, returning empty string";
            return QString();
        } else {
            qDebug() << "Save successful, updating file path to:" << newFilePath;
            
            // 保存成功后再更新路径，防止文件还未保存完成就触发文件是否存在的检测
            wrapper->updatePath(oldFilePath, newFilePath);
            
            // 更新文件编码
            wrapper->bottomBar()->setEncodeName(encode);

            // 若编码变更，保存完成后，重新加载文件
            if (needChangeEncode) {
                wrapper->readFile(encode);
                qDebug() << "Encoding changed, reloading file with new encoding";
            }
        }

        if (wrapper->filePath().contains(m_backupDir) || wrapper->filePath().contains(m_blankFileDir)) {
            QFile(wrapper->filePath()).remove();
            qDebug() << "Removed backup or blank file at path:" << wrapper->filePath();
        }

        //删除自动备份文件
        if (QFileInfo(m_autoBackupDir).exists()) {
            qDebug() << "Deleting auto backup file";
            QString truePath = wrapper->textEditor()->getTruePath();
            fileInfo.setFile(truePath);
            QString name = fileInfo.absolutePath().replace("/", "_");
            QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
        }

        /* 如果另存为的文件名+路径与当前tab项对应的文件名+路径是一致，则直接做保存操作即可 */
        if (!wrapper->filePath().compare(newFilePath)) {
            qDebug() << "File path matches new file path, updating tab name";
            if (saveFile()) {
                qDebug() << "Save successful, returning new file path";
                return newFilePath;
            }
        }

        updateSaveAsFileName(oldFilePath, newFilePath);
        qDebug() << "Exit saveAsFileToDisk, return new file path";
        return newFilePath;
    }

    qDebug() << "Exit saveAsFileToDisk, return empty string";
    return QString();
}

QString Window::saveBlankFileToDisk()
{
    qDebug() << "Enter saveBlankFileToDisk";
    QString filePath = m_tabbar->currentPath();
    EditWrapper *wrapper = m_wrappers.value(filePath);
    if (!wrapper) {
        qWarning() << "Failed to get wrapper for current path";
        return QString();
    }
    bool isDraft = Utils::isDraftFile(filePath);
    QFileInfo fileInfo(filePath);

    if (!wrapper) {
        qWarning() << "Failed to get wrapper for current path";
        return QString();
    }

    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(tr("Encoding"), QStringList() << wrapper->getTextEncode());
    dialog.setDirectory(QDir::homePath());

    if (isDraft) {
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(m_tabbar->currentName());
        dialog.selectFile(match.captured(0) + ".txt");
        qDebug() << "Setting dialog file name to draft file name:" << match.captured(0) + ".txt";
    } else {
        dialog.setDirectory(fileInfo.dir());
        dialog.selectFile(fileInfo.fileName());
        qDebug() << "Setting dialog file name to current file name:" << fileInfo.fileName();
    }

    int mode = dialog.exec();
    if (mode == QDialog::Accepted) {
        const QString newFilePath = dialog.selectedFiles().value(0);
        wrapper->updatePath(newFilePath);
        wrapper->saveFile();

        m_wrappers.remove(filePath);
        m_wrappers.insert(newFilePath, wrapper);

        // wrapper->textEditor()->loadHighlighter();
        qDebug() << "Saved blank file to disk, returning new file path:" << newFilePath;
        return newFilePath;
    }

    qDebug() << "Exit saveBlankFileToDisk, returning empty string";
    return QString();
}

bool Window::saveAsOtherTabFile(EditWrapper *wrapper)
{
    qDebug() << "Enter saveAsOtherTabFile";
    QString filePath = wrapper->textEditor()->getFilePath();
    bool isDraft = Utils::isDraftFile(filePath);
    QFileInfo fileInfo(filePath);
    int index = m_tabbar->indexOf(filePath);
    QString strTabText = m_tabbar->tabText(index);

    if (!wrapper) {
        qWarning() << "Wrapper is null, cannot save as other tab file";
        return false;
    }

    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(QObject::tr("Encoding"), Utils::getEncodeList());
    dialog.addComboBox(QObject::tr("Line Endings"), QStringList() << "Linux" << "Windows" << "Mac OS");
    dialog.setDirectory(QDir::homePath());

    if (isDraft) {
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(strTabText);
        dialog.selectFile(match.captured(0) + ".txt");
        qDebug() << "Setting dialog file name to draft file name:" << match.captured(0) + ".txt";
    } else {
        dialog.setDirectory(fileInfo.dir());
        dialog.selectFile(fileInfo.fileName());
        qDebug() << "Setting dialog file name to current file name:" << fileInfo.fileName();
    }

    int mode = dialog.exec();
    if (mode == QDialog::Accepted) {
        const QByteArray encode = dialog.getComboBoxValue(QObject::tr("Encoding")).toUtf8();
        const QString endOfLine = dialog.getComboBoxValue(QObject::tr("Line Endings"));
        const QString newFilePath = dialog.selectedFiles().value(0);
        const QFileInfo newFileInfo(newFilePath);
//        EditWrapper::EndOfLineMode eol = EditWrapper::eolUnix;

//        if (endOfLine == "Windows") {
//            eol = EditWrapper::eolDos;
//        } else if (endOfLine == "Mac OS") {
//            eol = EditWrapper::eolMac;
//        }

        if (isDraft) {
            QFile(filePath).remove();
            qDebug() << "Removing draft file at path:" << filePath;
        }

        //m_tabbar->updateTab(m_tabbar->currentIndex(), newFilePath, newFileInfo.fileName());

        //  wrapper->setTextCodec(encode);
        wrapper->updatePath(newFilePath);
        //wrapper->setEndOfLineMode(eol);
        wrapper->saveFile();

        // wrapper->textEditor()->loadHighlighter();
    } else {
        qWarning() << "Failed to get wrapper for current path";
        return false;
    }

    qDebug() << "Exit saveAsOtherTabFile, returning true";
    return true;
}

void Window::changeSettingDialogComboxFontNumber(int fontNumber)
{
    m_settings->settings->option("base.font.size")->setValue(fontNumber);
}

/**
 * @brief 根据传入的字体大小计算字体的比例，字体大小范围在 8 ~ 500，比例范围在 10% ~ 500%,
 *      默认字体大小为12。因此在 8~12 和 12~50 两组范围字体的缩放间隔不同。
 * @param fontSize 字体大小
 * @return 字体缩放比例，范围 10 ~ 500
 */
qreal Window::calcFontScale(qreal fontSize)
{
    qDebug() << "Calculating font scale for font size:" << fontSize;
    if (qFuzzyCompare(fontSize, m_settings->m_iDefaultFontSize)) {
        qDebug() << "Font size is equal to default font size, returning 100% scale";
        return 100.0;
    } else if (fontSize > m_settings->m_iDefaultFontSize) {
        static const qreal delta = (500 - 100) * 1.0 / (m_settings->m_iMaxFontSize - m_settings->m_iDefaultFontSize);
        qreal fontScale = 100 + delta * (fontSize - m_settings->m_iDefaultFontSize);
        qDebug() << "Font size is greater than default, calculated font scale:" << fontScale;
        return qMin(fontScale, 500.0);
    } else {
        static const qreal delta = (100 - 10) * 1.0 / (m_settings->m_iDefaultFontSize - m_settings->m_iMinFontSize);
        qreal fontScale = 100 + delta * (fontSize - m_settings->m_iDefaultFontSize);
        qDebug() << "Font size is less than default, calculated font scale:" << fontScale;
        return qMax(fontScale, 10.0);
    }
}

/**
 * @brief 根据字体缩放比例返回字体大小
 * @param fontScale 字体缩放比例
 * @return 字体大小，范围 8~50
 */
qreal Window::calcFontSizeFromScale(qreal fontScale)
{
    qDebug() << "Calculating font size from scale:" << fontScale;
    if (qFuzzyCompare(fontScale, 100.0)) {
        qDebug() << "Font scale is equal to 100%, returning default font size";
        return m_settings->m_iDefaultFontSize;
    } else if (fontScale > 100.0) {
        static const qreal delta = (m_settings->m_iMaxFontSize - m_settings->m_iDefaultFontSize) * 1.0 / (500 - 100);
        qreal fontSize = m_settings->m_iDefaultFontSize + delta * ((fontScale) - 100);
        qDebug() << "Font scale is greater than 100%, calculated font size:" << fontSize;
        return qMin<qreal>(fontSize, m_settings->m_iMaxFontSize);
    } else {
        static const qreal delta = (m_settings->m_iDefaultFontSize - m_settings->m_iMinFontSize) * 1.0 / (100 - 10) ;
        qreal fontSize = m_settings->m_iMinFontSize + delta * (fontScale - 10);
        qDebug() << "Font scale is less than 100%, calculated font size:" << fontSize;
        return qMax<qreal>(fontSize, m_settings->m_iMinFontSize);
    }
}

void Window::decrementFontSize()
{
    qDebug() << "Decrementing font size";
    qreal fontScale = calcFontScale(m_fontSize);
    // 减少10%
    fontScale -= 10;
    m_fontSize = calcFontSizeFromScale(fontScale);

    qreal size = qMax<qreal>(m_fontSize, m_settings->m_iMinFontSize);
    m_settings->settings->option("base.font.size")->setValue(size);
    qDebug() << "New font size:" << m_fontSize;
}

void Window::incrementFontSize()
{
    qDebug() << "Incrementing font size";
    qreal fontScale = calcFontScale(m_fontSize);
    // 增加10%
    fontScale += 10;
    m_fontSize = calcFontSizeFromScale(fontScale);

    qreal size = qMin<qreal>(m_fontSize, m_settings->m_iMaxFontSize);
    m_settings->settings->option("base.font.size")->setValue(size);
    qDebug() << "New font size:" << m_fontSize;
}

void Window::resetFontSize()
{
    qDebug() << "Resetting font size to default";
    m_settings->settings->option("base.font.size")->setValue(m_settings->m_iDefaultFontSize);
}

void Window::setFontSizeWithConfig(EditWrapper *wrapper)
{
    qDebug() << "Setting font size with config";
    qreal size = m_settings->settings->option("base.font.size")->value().toReal();
    wrapper->textEditor()->setFontSize(size);
    wrapper->bottomBar()->setScaleLabelText(size);

    m_fontSize = size;
    qDebug() << "Font size set to:" << m_fontSize;
}

void Window::popupFindBar()
{
    qDebug() << "Enter popupFindBar";
#if 0
    // This block seems related to UI focus management and might be outdated or incorrect.
    // The original code directly calls textEditor()->setFocus() without checking if the wrapper or textEditor is null.
    // Adding a null check here.
    if (m_findBar->isVisible()) {
        m_findBar->move(QPoint(10, height() - 59));
        if (m_findBar->isFocus()) {
            EditWrapper* wrapper = m_wrappers.value(m_tabbar->currentPath());
            if (wrapper && wrapper->textEditor()) {
                wrapper->textEditor()->setFocus();
            } else {
                 qWarning() << "Failed to get wrapper or text editor to set focus in popupFindBar";
            }
        } else {
            m_findBar->focus();
        }
    } else {
        addBottomWidget(m_findBar);
    }
#endif

    m_findBar->setSearched(false);
    QString tabPath = m_tabbar->currentPath();
    EditWrapper *wrapper = currentWrapper();

    if (currentWrapper() == nullptr) {
        qDebug() << "No current wrapper, cannot popup find bar";
        return;
    }

    if (wrapper->textEditor()->document()->isEmpty()) {
        qDebug() << "Current document is empty, cannot popup find bar";
        return;
    }

    currentWrapper()->bottomBar()->updateSize(m_findBar->height() + 8, true);

    if (m_replaceBar->isVisible()) {
        qDebug() << "Hiding replace bar before showing find bar";
        m_replaceBar->hide();
    }
    m_findBar->raise();
    m_findBar->show();
    m_findBar->move(QPoint(4, height() - m_findBar->height() - 4));

    //QString text = wrapper->textEditor()->textCursor().selectedText();
    QString text = wrapper->textEditor()->selectedText();
    int row = wrapper->textEditor()->getCurrentLine();
    int column = wrapper->textEditor()->getCurrentColumn();
    int scrollOffset = wrapper->textEditor()->getScrollOffset();

    m_findBar->activeInput(text, tabPath, row, column, scrollOffset);
    // highlight keyword when findbar show
    wrapper->textEditor()->highlightKeywordInView(text);
    // set keywords
    m_keywordForSearchAll = m_keywordForSearch = text;

    QTimer::singleShot(10, this, [ = ] { m_findBar->focus(); });
    qDebug() << "Popup find bar completed";
}

void Window::popupReplaceBar()
{
    qDebug() << "Enter popupReplaceBar";
    if (currentWrapper() == nullptr) {
        qDebug() << "No current wrapper, cannot popup replace bar";
        return;
    }

    if (currentWrapper()->textEditor()->document()->isEmpty()) {
        qDebug() << "Current document is empty, cannot popup replace bar";
        return;
    }

    if (currentWrapper() && currentWrapper()->getFileLoading()) {
        qDebug() << "File is loading, cannot popup replace bar";
        return;
    }

    QTextCursor cursor = currentWrapper()->textEditor()->textCursor();

    m_replaceBar->setsearched(false);
    EditWrapper *curWrapper = currentWrapper();
    bool bIsReadOnly = curWrapper->textEditor()->getReadOnlyMode();

    if (cursor.hasSelection()) {
        currentWrapper()->textEditor()->setCursorStart(cursor.selectionStart());
        qDebug() << "Replace bar: selected";
    }

    if (bIsReadOnly) {
        showNotify(tr("Read-Only mode is on"));
        qDebug() << "Replace bar: Read-Only mode is on, cannot popup replace bar";
        return;
    }

    currentWrapper()->bottomBar()->updateSize(m_replaceBar->height() + 8, true);

    EditWrapper *wrapper = currentWrapper();
    if (m_findBar->isVisible()) {
        m_findBar->hide();
        qDebug() << "Hiding find bar before showing replace bar";
    }
    m_replaceBar->raise();
    m_replaceBar->show();
    m_replaceBar->move(QPoint(4, height() - m_replaceBar->height() - 4));
    //addBottomWidget(m_replaceBar);

    QString tabPath = m_tabbar->currentPath();
    // QString text = wrapper->textEditor()->textCursor().selectedText();
    QString text = wrapper->textEditor()->selectedText();
    int row = wrapper->textEditor()->getCurrentLine();
    int column = wrapper->textEditor()->getCurrentColumn();
    int scrollOffset = wrapper->textEditor()->getScrollOffset();

    m_replaceBar->activeInput(text, tabPath, row, column, scrollOffset);

    QTimer::singleShot(10, this, [ = ] { m_replaceBar->focus(); });
    qDebug() << "Popup replace bar completed";
}

void Window::popupJumpLineBar()
{
    qDebug() << "Enter popupJumpLineBar";
    EditWrapper *curWrapper = currentWrapper();

    if (curWrapper == nullptr) {
        qDebug() << "No current wrapper, cannot popup jump line bar";
        return;
    }

    if (curWrapper->textEditor()->document()->isEmpty()) {
        qDebug() << "Current document is empty, cannot popup jump line bar";
        return;
    }

    if (m_jumpLineBar->isVisible()) {
        m_jumpLineBar->hide();
        qDebug() << "Hiding jump line bar before showing find bar";
        return;
    }

    if (m_jumpLineBar->isVisible()) {
        qDebug() << "Jump line bar is visible, cannot popup jump line bar";
        if (m_jumpLineBar->isFocus()) {
            //QTimer::singleShot(0, m_wrappers.value(m_tabbar->currentPath())->textEditor(), SLOT(setFocus()));
            qDebug() << "Jump line bar is focused, set focus to text editor";
        } else {
            qDebug() << "Jump line bar is not focused, focusing it";
            m_jumpLineBar->focus();
        }
    } else {
        QString tabPath = m_tabbar->currentPath();
        EditWrapper *wrapper = currentWrapper();
        QString text = wrapper->textEditor()->textCursor().selectedText();
        int row = wrapper->textEditor()->getCurrentLine();
        int column = wrapper->textEditor()->getCurrentColumn();
        int count = wrapper->textEditor()->blockCount();
        int scrollOffset = wrapper->textEditor()->getScrollOffset();

        m_jumpLineBar->activeInput(tabPath, row, column, count, scrollOffset);
        m_jumpLineBar->show();
        m_jumpLineBar->focus();
        qDebug() << "Popup jump line bar completed";
    }
}

void Window::updateJumpLineBar(TextEdit *editor)
{
    qDebug() << "Enter updateJumpLineBar";
    // 文本块内容未新增行不更新跳转行号
    if (m_jumpLineBar->isVisible() && editor->blockCount() != m_jumpLineBar->getLineCount()) {
        QString tabPath = m_tabbar->currentPath();
        QString text = editor->textCursor().selectedText();
        int row = editor->getCurrentLine();
        int column = editor->getCurrentColumn();
        int count = editor->blockCount();
        int scrollOffset = editor->getScrollOffset();
        m_jumpLineBar->activeInput(tabPath, row, column, count, scrollOffset);
    }
    if (editor && !editor->ifHasHighlight()) {
        qDebug() << "No highlight, updating jump line bar";
        m_findBar->setSearched(false);
        m_replaceBar->setsearched(false);
    }
    qDebug() << "Update jump line bar completed";
}

void Window::popupSettingsDialog()
{
    qDebug() << "Enter popupSettingsDialog";
    DSettingsDialog *dialog = new DSettingsDialog(this);
    dialog->widgetFactory()->registerWidget("fontcombobox", Settings::createFontComBoBoxHandle);
    dialog->widgetFactory()->registerWidget("keySequenceEdit", Settings::createKeySequenceEditHandle);
    dialog->widgetFactory()->registerWidget("savingpathwgt", Settings::createSavingPathWgt);
    dialog->resize(680, 300);
    dialog->setMinimumSize(680, 300);
    m_settings->setSettingDialog(dialog);

    dialog->updateSettings(m_settings->settings);

    dialog->exec();

    delete dialog;
    m_settings->settings->sync();
    qDebug() << "Popup settings dialog completed";
}

/**
 * @brief 清空打印文档信息，用于关闭打印对话框或中止打印时释放构造的临时文档
 */
void Window::clearPrintTextDocument()
{
    qDebug() << "Clearing print text document";
    if (m_printDoc != nullptr) {
        qDebug() << "Print document is not null, clearing it";
        if (!m_printDoc->isEmpty()) {
            m_printDoc->clear();
            qDebug() << "Print document cleared";
        }
        m_printDoc = nullptr;
    }

    // 清空打印对象列表
    if (!m_printDocList.isEmpty()) {
        for (auto &info : m_printDocList) {
            if (info.highlighter) {
                delete info.highlighter;
                qDebug() << "Print document highlighter deleted";
            }

            if (info.doc) {
                info.doc->clear();
                delete info.doc;
                qDebug() << "Print document deleted";
            }
        }

        m_printDocList.clear();
    }
    qDebug() << "Print text document cleared";
}

void Window::setWindowTitleInfo()
{
    qDebug() << "Setting window title info";
    if (m_tabbar) {
        if (m_tabbar->truePathAt(m_tabbar->currentIndex()) != "") {
            qDebug() << "Current tab has true path, setting window title to it";
            setWindowTitle(m_tabbar->truePathAt(m_tabbar->currentIndex()));
        } else {
            qDebug() << "Current tab has no true path, setting window title to tab name";
            setWindowTitle(m_tabbar->currentName());
        }
    }
    qDebug() << "Window title info set";
}

/**
 * @brief 取得当前文件打开文档目录，新建文档为"系统-文档"目录(~/Documents)
 * @return 当前文件打开文档目录
 */
QString Window::getCurrentOpenFilePath()
{
    qDebug() << "Getting current open file path";
    QString path;
    EditWrapper *wrapper = currentWrapper();
    if (wrapper) {
        qDebug() << "Current wrapper is not null, getting current file path";
        QString curFilePath = wrapper->textEditor() ? wrapper->textEditor()->getTruePath()
                              : wrapper->filePath();
        // 临时文件或备份文件，均返回"文档"目录
        if (Utils::isDraftFile(curFilePath) || Utils::isBackupFile(curFilePath)) {
            path = QDir::homePath() + "/Documents";
            qDebug() << "Current file is draft or backup, setting path to home documents";
        } else {
            path = QFileInfo(curFilePath).absolutePath();
            qDebug() << "Current file is not draft or backup, setting path to file path";
        }
    }

    qDebug() << "Current open file path:" << path;
    return path;
}

/**
 * @brief 克隆文本数据，不同于 QTextDocument::clone(), 主要用于大文本的拷贝，拷贝过程通过
 *      QApplication::processEvents() 执行其它事件
 * @param editWrapper 文本编辑处理，提供文本编辑器和高亮信息
 * @return 是否克隆数据成功
 *
 * @note Qt自带的布局算法在超长文本时存在计算越界的问题，计算长度将返回溢出值，导致显示异常，
        调整为拆分的文档结构
 */
bool Window::cloneLargeDocument(EditWrapper *editWrapper)
{
    qDebug() << "Cloning large document";

    if (!editWrapper) {
        qWarning() << "Invalid edit wrapper, cannot clone document";
        return false;
    }

    // 获取源文本
    QTextDocument *srcDoc = editWrapper->textEditor()->document();
    // 是否需要高亮
    bool needHighlighter = editWrapper->getSyntaxHighlighter()
                           && editWrapper->getSyntaxHighlighter()->definition().isValid();
    // 创建新的打印信息
    auto createPrintInfo = [ = ]()->PrintInfo {
        PrintInfo info;
        info.doc = createNewDocument(srcDoc);
        if (needHighlighter)
        {
            // 构造高亮处理
            info.highlighter = new CSyntaxHighlighter(info.doc);
            info.highlighter->setDefinition(editWrapper->getSyntaxHighlighter()->definition());
            info.highlighter->setTheme(editWrapper->getSyntaxHighlighter()->theme());
            qDebug() << "Print document highlighter created";
        }
        qDebug() << "Print document created";
        return info;
    };

    // 每次拷贝约100kb数据
    static const int s_StepCopyCharCount = 1024 * 100;
    // 单个文档字符不超过50MB, 防止布局失败
    static const int s_MaxDocCharCount = 1024 * 1024 * 50;
    // 单个文档文本块数不超过10万行
    static const int s_MaxDocBlockCount = 100000;

    // 创建首个拷贝文本对象
    PrintInfo info = createPrintInfo();
    QTextDocument *copyDoc = info.doc;
    m_printDocList.append(info);

    try {
        // 此次拷贝到字符数统计
        int currentCopyCharCount = 0;
        // 此次拷贝勾选的文本块数
        int currentSelectBlock = 0;
        // 已读取的源文件偏移量
        int srcOffset = 0;

        // 标记开始处理
        m_bPrintProcessing = true;
        QPointer<QObject> checkPtr(this);

        QTextCursor copyCursor(copyDoc);
        QTextCursor srcCursor(srcDoc);

        for (QTextBlock srcBlock = srcDoc->firstBlock(); srcBlock.isValid(); srcBlock = srcBlock.next()) {
            currentSelectBlock++;
            currentCopyCharCount += srcBlock.length();

            if (currentCopyCharCount >= s_StepCopyCharCount) {
                qDebug() << "Copying " << s_StepCopyCharCount << " characters";
                // 判断是否超过单个文档允许的最大范围
                if ((copyDoc->characterCount() + currentCopyCharCount) > s_MaxDocCharCount ||
                    (copyDoc->blockCount() + currentSelectBlock) > s_MaxDocBlockCount) {
                    // 创建新的打印文档
                    m_printDocList.append(createPrintInfo());
                    copyDoc = m_printDocList.last().doc;
                    // 创建新的插入光标
                    copyCursor = QTextCursor(copyDoc);
                    qDebug() << "Creating new print document for large print";
                }

                srcCursor.setPosition(srcOffset);
                srcCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, currentSelectBlock);
                copyCursor.insertFragment(QTextDocumentFragment(srcCursor));

                srcOffset += currentCopyCharCount;
                currentSelectBlock = 0;
                currentCopyCharCount = 0;
            }

            // 处理其它事件
            qApp->processEvents();
            if (!checkPtr || !m_bPrintProcessing) {
                qWarning() << "Abort print copy!";
                clearPrintTextDocument();
                return false;
            }
        }

        // 补充保存最后的信息
        srcCursor.setPosition(srcOffset);
        srcCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, currentSelectBlock);
        // 保证拷贝到文档末尾
        srcCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor, 1);
        copyCursor.insertFragment(QTextDocumentFragment(srcCursor));

        m_bPrintProcessing = false;
    } catch (const std::exception &e) {
        qWarning() << "Print copy doc error!" << e.what();
        // 清理数据
        m_bPrintProcessing = false;
        clearPrintTextDocument();
        return false;
    }

    qDebug() << "Large document cloned";
    return true;
}

/**
   @brief 使用 \a highlighter 重新高亮传入的文本 \a doc ,此函数用于大文本打印时对临时文本的处理
 */
void Window::rehighlightPrintDoc(QTextDocument *doc, CSyntaxHighlighter *highlighter)
{
    qDebug() << "Rehighlighting print document";
    if (!doc || !highlighter) {
        qWarning() << "Invalid document or highlighter, cannot rehighlight";
        return;
    }

    QColor background = m_printWrapper->textEditor()->palette().color(QPalette::Base);
    bool backgroundIsDark = background.value() < 128;

    highlighter->setEnableHighlight(true);
    if (backgroundIsDark) {
        qDebug() << "Background is dark, rehighlighting each block";
        for (QTextBlock block = doc->firstBlock(); block.isValid(); block = block.next()) {
            highlighter->rehighlightBlock(block);

            QVector<QTextLayout::FormatRange> formatList = block.layout()->formats();
            // adjust syntax highlighting colors for better contrast
            for (int i = formatList.count() - 1; i >= 0; --i) {
                QTextCharFormat &format = formatList[i].format;
                if (format.background().color() == background) {
                    QBrush brush = format.foreground();
                    QColor color = brush.color();
                    int h, s, v, a;
                    color.getHsv(&h, &s, &v, &a);
                    color.setHsv(h, s, qMin(128, v), a);
                    brush.setColor(color);
                    format.setForeground(brush);
                    qDebug() << "Adjusted syntax highlighting color for block";
                }
                format.setBackground(Qt::white);
            }

            block.layout()->setFormats(formatList);
        }
    } else {
        qDebug() << "Background is light, rehighlighting all text";
        highlighter->rehighlight();
    }
    highlighter->setEnableHighlight(false);
    qDebug() << "Print document rehighlighted";
}

/**
   @brief 接收布局模式变更信号 DGuiApplicationHelper::sizeModeChanged() ，更新界面布局
        Window 接收布局模式变更，调整 findBar 和 replaceBar 的坐标位置。
   @note 需要在 findBar / replaceBar / bottomBar 更新后触发更新
 */
void Window::updateSizeMode()
{
    qDebug() << "Update size mode";
    if (m_findBar && m_findBar->isVisible()) {
        qDebug() << "Find bar is visible, updating its position";
        currentWrapper()->bottomBar()->updateSize(m_findBar->height() + 8, true);
        m_findBar->move(QPoint(4, height() - m_findBar->height() - 4));
    }

    if (m_replaceBar && m_replaceBar->isVisible()) {
        qDebug() << "Replace bar is visible, updating its position";
        currentWrapper()->bottomBar()->updateSize(m_replaceBar->height() + 8, true);
        m_replaceBar->move(QPoint(4, height() - m_replaceBar->height() - 4));
    }

    qDebug() << "Size mode updated";
}

void Window::popupPrintDialog()
{
    qDebug() << "Enter popupPrintDialog";
    //大文本加载过程不允许打印操作
    if (currentWrapper() && currentWrapper()->getFileLoading()) {
        qDebug() << "File is loading, cannot print";
        return;
    }

    // 已有处理的打印事件，不继续进入
    if (m_bPrintProcessing) {
        qDebug() << "Print processing, cannot print";
        return;
    }

    const QString &filePath = currentWrapper()->textEditor()->getFilePath();
    const QString &fileDir = QFileInfo(filePath).dir().absolutePath();

    qInfo() << qPrintable("Start print doc");

    //适配打印接口2.0，dtk版本大于或等于5.4.10才放开最新的2.0打印预览接口
#if (DTK_VERSION_MAJOR > 5 \
    || (DTK_VERSION_MAJOR == 5 && DTK_VERSION_MINOR > 4) \
    || (DTK_VERSION_MAJOR == 5 && DTK_VERSION_MINOR == 4 && DTK_VERSION_PATCH >= 10))

    QTextDocument *doc = currentWrapper()->textEditor()->document();
    m_bLargePrint = false;
    m_bPrintProcessing = false;
    m_printWrapper = currentWrapper();
    m_multiDocPageCount = 0;

    if (doc != nullptr && !doc->isEmpty()) {
        qDebug() << "Document is not empty, checking if it needs highlighting";
        static const int s_maxDirectReadLen = 1024 * 1024 * 10;
        static const int s_maxHighlighterDirectReadLen = 1024 * 1024 * 5;
        // 判断是否需要文本高亮，文本数据量大小，不同数据量使用不同分支。
        bool needHighlighter = currentWrapper()->getSyntaxHighlighter()
                               && currentWrapper()->getSyntaxHighlighter()->definition().isValid();

        if (needHighlighter
                && doc->characterCount() > s_maxHighlighterDirectReadLen) {
            // 需要高亮且数据量超过 5MB
            m_bLargePrint = true;
            qInfo() << qPrintable("Clone large print document with highlighting");
        } else if (!needHighlighter
                   && doc->characterCount() > s_maxDirectReadLen) {
            // 无需高亮且数据量超过 10MB
            m_bLargePrint = true;
            qInfo() << qPrintable("Clone large print document without highlighting");
        } else {
            currentWrapper()->updateHighlighterAll();
            m_printDoc = doc->clone(doc);
            qInfo() << qPrintable("Clone small print document");
        }

        // 大文件处理
        if (m_bLargePrint) {
            qInfo() << qPrintable("Clone large print document");
            // 克隆大文本数据
            if (!cloneLargeDocument(currentWrapper())) {
                qWarning() << "Failed to clone large document for printing!";
                return;
            }
        }
    }

    if ((!m_bLargePrint && nullptr == m_printDoc)
            || (m_bLargePrint && m_printDocList.isEmpty())) {
        qWarning() << "The print document is not valid!";
        return;
    }

    DPrinter printer(QPrinter::HighResolution);
    m_isNewPrint = true;
    m_pPreview = new DPrintPreviewDialog(this);
    m_pPreview->setAttribute(Qt::WA_DeleteOnClose);

    // 设置 QPrinter 的文档名称，保留绝对路径和文件后缀(在cups的page_log中保留完整的job-name)
    // 注意和文件的输出文件路径进行区分
    if (fileDir == m_blankFileDir) {
        qInfo() << qPrintable("Print blank file");
        m_pPreview->setDocName(filePath);
    } else {
        qInfo() << qPrintable("Print file: ") << filePath;
        QString path = currentWrapper()->textEditor()->getTruePath();
        m_pPreview->setDocName(path);
    }

    // 后续布局计算后再更新打印页数
    m_pPreview->setAsynPreview(PRINT_FLAG);

    connect(m_pPreview, &DPrintPreviewDialog::finished, this, [ = ] {
        m_bPrintProcessing = false;
        clearPrintTextDocument();
    });
    connect(m_pPreview, &DPrintPreviewDialog::rejected, this, [ = ] {
        m_bPrintProcessing = false;
        clearPrintTextDocument();
    });

    connect(m_pPreview, QOverload<DPrinter *, const QVector<int> &>::of(&DPrintPreviewDialog::paintRequested),
    this, [ = ](DPrinter * _printer, const QVector<int> &pageRange) {
        if (m_bLargePrint) {
            this->doPrintWithLargeDoc(_printer, pageRange);
            qInfo() << qPrintable("Print large document");
        } else {
            this->doPrint(_printer, pageRange);
            qInfo() << qPrintable("Print small document");
        }
    });

    m_pPreview->exec();

#else
    DPrintPreviewDialog preview(this);

    connect(&preview, QOverload<DPrinter *>::of(&DPrintPreviewDialog::paintRequested),
    this, [ & ](DPrinter * printer) {
        if (fileDir == m_blankFileDir) {
            printer->setDocName(QString(m_tabbar->currentName()));
        } else {
            printer->setDocName(QString(QFileInfo(filePath).baseName()));
        }
        currentWrapper()->textEditor()->print(printer);
    });

    currentWrapper()->updateHighlighterAll();

    preview.exec();
#endif
}

void Window::popupThemePanel()
{
    qDebug() << "Enter popupThemePanel";
    updateThemePanelGeomerty();
    m_themePanel->setSelectionTheme(m_themePath);
    m_themePanel->popup();
    qDebug() << "Exit popupThemePanel";
}

void Window::toggleFullscreen()
{
    qDebug() << "Enter toggleFullscreen";
    if (!window()->windowState().testFlag(Qt::WindowFullScreen)) {
        qDebug() << "Enter fullscreen";
        window()->setWindowState(windowState() | Qt::WindowFullScreen);
    } else {
        qDebug() << "Exit fullscreen";
        window()->setWindowState(windowState() & ~Qt::WindowFullScreen);
    }
    qDebug() << "Exit toggleFullscreen";
}

void Window::remberPositionSave()
{
    qDebug() << "Enter remberPositionSave";
    EditWrapper *wrapper = currentWrapper();

    m_remberPositionFilePath = m_tabbar->currentPath();
    m_remberPositionRow = wrapper->textEditor()->getCurrentLine();
    m_remberPositionColumn = wrapper->textEditor()->getCurrentColumn();
    m_remberPositionScrollOffset = wrapper->textEditor()->getScrollOffset();

    currentWrapper()->showNotify(tr("Current location remembered"));
    qDebug() << "Exit remberPositionSave";
}

void Window::remberPositionRestore()
{
    qDebug() << "Enter remberPositionRestore";
    if (m_remberPositionFilePath.isEmpty()) {
        qDebug() << "No remembered position, cannot restore";
        return;
    }

    if (m_wrappers.contains(m_remberPositionFilePath)) {
        const QString &filePath = m_remberPositionFilePath;
        const int &scrollOffset = m_remberPositionScrollOffset;
        const int &row = m_remberPositionRow;
        const int &column = m_remberPositionColumn;

        activeTab(m_tabbar->indexOf(m_remberPositionFilePath));
        EditWrapper* wrapper = m_wrappers.value(filePath);
         if (!wrapper || !wrapper->textEditor()) {
             qWarning() << "Failed to get wrapper or text editor in remberPositionRestore";
             return;
         }
        wrapper->textEditor()->scrollToLine(scrollOffset, row, column);
    }
    qDebug() << "Exit remberPositionRestore";
}


void Window::displayShortcuts()
{
    qDebug() << "Enter displayShortcuts";
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);
    // 获取当前焦点位置（光标所在屏幕中心）
    QScreen *screen = nullptr;
#if DTK_VERSION > DTK_VERSION_CHECK(5, 5, 2, 0)
    if (DGuiApplicationHelper::isTabletEnvironment()) {
        // bug 88079 避免屏幕旋转弹出位置错误
        screen = qApp->primaryScreen();
        qDebug() << "Tablet environment, use primary screen";
    } else {
        screen = QGuiApplication::screenAt(QCursor::pos());
        qDebug() << "Use screen at cursor position";
    }
#else
    screen = QGuiApplication::screenAt(QCursor::pos());
#endif

    if (screen) {
        pos = screen->geometry().center();
        qDebug() << "Screen center position: " << pos;
    }

    //窗体快捷键
    QStringList windowKeymaps;
    windowKeymaps << "addblanktab" << "newwindow" << "savefile"
                  << "saveasfile" << "selectnexttab" << "selectprevtab"
                  << "closetab" << "closeothertabs" << "restoretab"
                  << "openfile" << "incrementfontsize" << "decrementfontsize"
                  << "resetfontsize" << "togglefullscreen" << "find" << "replace"
                  << "jumptoline" << "saveposition" << "restoreposition"
                  << "escape" << "print";

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    QJsonObject windowJsonGroup;
    windowJsonGroup.insert("groupName", QObject::tr("Window"));
    QJsonArray windowJsonItems;

    for (const QString &keymap : windowKeymaps) {
        auto option = m_settings->settings->group("shortcuts.window")->option(QString("shortcuts.window.%1").arg(keymap));
        QJsonObject jsonItem;
        jsonItem.insert("name", QObject::tr(option->name().toUtf8().data()));
        if (keymap != "incrementfontsize" && keymap != "decrementfontsize") {
            jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        } else if (keymap == "incrementfontsize") {
            QString strIncrementfontValue = QString(tr("Ctrl+'='"));
            jsonItem.insert("value", strIncrementfontValue.replace("Meta", "Super"));
        } else if (keymap == "decrementfontsize" && option->value().toString() == "Ctrl+-") {
            QString strDecrementfontValue = QString(tr("Ctrl+'-'"));
            jsonItem.insert("value", strDecrementfontValue.replace("Meta", "Super"));
        }

        windowJsonItems.append(jsonItem);
    }

    windowJsonGroup.insert("groupItems", windowJsonItems);
    jsonGroups.append(windowJsonGroup);

    //编辑快捷键
    QStringList editorKeymaps;
    editorKeymaps << "indentline" << "backindentline" << "forwardchar"
                  << "backwardchar" << "forwardword" << "backwardword"
                  << "nextline" << "prevline" << "newline" << "opennewlineabove"
                  << "opennewlinebelow" << "duplicateline" << "killline"
                  << "killcurrentline" << "swaplineup" << "swaplinedown"
                  << "scrolllineup" << "scrolllinedown" << "scrollup"
                  << "scrolldown" << "movetoendofline" << "movetostartofline"
                  << "movetoend" << "movetostart" << "movetolineindentation"
                  << "upcaseword" << "downcaseword" << "capitalizeword"
                  << "killbackwardword" << "killforwardword" << "forwardpair"
                  << "backwardpair" << "selectall" << "copy" << "cut"
                  << "paste" << "transposechar" << "setmark" << "exchangemark"
                  << "copylines" << "cutlines" << "joinlines" << "togglereadonlymode"
                  << "togglecomment" << "removecomment" << "undo" << "redo" << "switchbookmark" << "movetoprebookmark"
                  << "movetonextbookmark" << "mark";

    QJsonObject editorJsonGroup;
    editorJsonGroup.insert("groupName", tr("Editor"));
    QJsonArray editorJsonItems;

    for (const QString &keymap : editorKeymaps) {
        auto option = m_settings->settings->group("shortcuts.editor")->option(QString("shortcuts.editor.%1").arg(keymap));
        QJsonObject jsonItem;
        jsonItem.insert("name", QObject::tr(option->name().toUtf8().data()));
        jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        editorJsonItems.append(jsonItem);
    }
    editorJsonGroup.insert("groupItems", editorJsonItems);
    jsonGroups.append(editorJsonGroup);

    //设置快捷键
    QStringList setupKeymaps;
    setupKeymaps << "help" << "displayshortcuts";

    QJsonObject setupJsonGroup;
    setupJsonGroup.insert("groupName", tr("Settings"));
    QJsonArray setupJsonItems;

    for (const QString &keymap : setupKeymaps) {
        auto option = m_settings->settings->group("shortcuts.window")->option(QString("shortcuts.window.%1").arg(keymap));
        QJsonObject jsonItem;
        jsonItem.insert("name", QObject::tr(option->name().toUtf8().data()));
        jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        setupJsonItems.append(jsonItem);
    }
    setupJsonGroup.insert("groupItems", setupJsonItems);
    jsonGroups.append(setupJsonGroup);

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    m_shortcutViewProcess = new QProcess();
    m_shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(m_shortcutViewProcess, SIGNAL(finished(int)), m_shortcutViewProcess, SLOT(deleteLater()));
    qDebug() << "Exit displayShortcuts";
}

/*!
 * \~chinese \brief Window::doPrint 调用打印预览，将文本分块输出给打印机
 * \~chinese \param pageRange 打印预览请求的页码列表
 */
#if (DTK_VERSION_MAJOR > 5 \
 || (DTK_VERSION_MAJOR == 5 && DTK_VERSION_MINOR > 4) \
 || (DTK_VERSION_MAJOR == 5 && DTK_VERSION_MINOR == 4 && DTK_VERSION_PATCH >= 10))
void Window::doPrint(DPrinter *printer, const QVector<int> &pageRange)
{
    qDebug() << "Enter doPrint";
    if (nullptr == m_printDoc) {
        qDebug() << "Print document is null";
        return;
    }

    //如果打印预览请求的页码为空，则直接返回
    if (pageRange.isEmpty()) {
        qDebug() << "Page range is empty";
        return;
    }

    QPainter p(printer);
    if (!p.isActive()) {
        qDebug() << "Painter is not active";
        return;
    }

    if (m_lastLayout.isValid() && !m_isNewPrint) {
        if (m_lastLayout == printer->pageLayout()) {
            // 如果打印属性没发生变化，直接加载已生成的资源，提高切换速度
            asynPrint(p, printer, pageRange);
            qDebug() << "Exit doPrint";
            return;
        }
    }

    //保留print的打印布局
    m_lastLayout = printer->pageLayout();
    m_isNewPrint = false;

    QTextOption opt = m_printDoc->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_printDoc->setDefaultTextOption(opt);

    (void)m_printDoc->documentLayout(); // make sure that there is a layout

    if (currentWrapper() == nullptr) {
        qDebug() << "Current wrapper is null";
        return;
    }
    QColor background = currentWrapper()->textEditor()->palette().color(QPalette::Base);
    bool backgroundIsDark = background.value() < 128;
    //对文本进行分页处理
    for (QTextBlock srcBlock = currentWrapper()->textEditor()->document()->firstBlock(), dstBlock = m_printDoc->firstBlock();
            srcBlock.isValid() && dstBlock.isValid();
            srcBlock = srcBlock.next(), dstBlock = dstBlock.next()) {
        QVector<QTextLayout::FormatRange> formatList = srcBlock.layout()->formats();
        if (backgroundIsDark) {
            qDebug() << "Background is dark";
            // adjust syntax highlighting colors for better contrast
            for (int i = formatList.count() - 1; i >= 0; --i) {
                QTextCharFormat &format = formatList[i].format;
                if (format.background().color() == background) {
                    QBrush brush = format.foreground();
                    QColor color = brush.color();
                    int h, s, v, a;
                    color.getHsv(&h, &s, &v, &a);
                    color.setHsv(h, s, qMin(128, v), a);
                    brush.setColor(color);
                    format.setForeground(brush);
                }
                format.setBackground(Qt::white);
            }
        }

        dstBlock.layout()->setFormats(formatList);
    }

    QAbstractTextDocumentLayout *layout = m_printDoc->documentLayout();
    layout->setPaintDevice(p.device());

    int dpiy = p.device()->logicalDpiY();
    int margin = static_cast<int>((2 / 2.54) * dpiy); // 2 cm margins

    auto fmt = m_printDoc->rootFrame()->frameFormat();
    fmt.setMargin(margin);
    m_printDoc->rootFrame()->setFrameFormat(fmt);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRectF pageRect(printer->pageRect());
#else
    QRectF pageRect(printer->pageRect(QPrinter::Point));
#endif

    QRectF body = QRectF(0, 0, pageRect.width(), pageRect.height());
    QFontMetrics fontMetrics(m_printDoc->defaultFont(), p.device());
    QRectF titleBox(margin,
                    body.bottom() - margin
                    + fontMetrics.height()
                    - 6 * dpiy / 72.0,
                    body.width() - 2 * margin,
                    fontMetrics.height());
    m_printDoc->setPageSize(body.size());
    //输出总页码给到打印预览
    m_pPreview->setAsynPreview(m_printDoc->pageCount());

    //渲染第一页文本
    for (int i = 0; i < pageRange.count(); ++i) {
        if (pageRange[i] > m_printDoc->pageCount())
            continue;
        Window::printPage(pageRange[i], &p, m_printDoc, body, titleBox);
        if (i != pageRange.count() - 1)
            printer->newPage();
    }
    qDebug() << "Exit doPrint";
}

/**
 * @brief 用于较大的文本文件进行打印计算和绘制，使用 processEvents 规避打印输入
 * @param printer       打印指针
 * @param pageRange     打印范围
 */
void Window::doPrintWithLargeDoc(DPrinter *printer, const QVector<int> &pageRange)
{
    qDebug() << "Enter doPrintWithLargeDoc";
    if (m_bPrintProcessing) {
        qDebug() << "Print processing";
        return;
    }

    if (m_printDocList.isEmpty()) {
        qDebug() << "Print doc list is empty";
        return;
    }

    //如果打印预览请求的页码为空，则直接返回
    if (pageRange.isEmpty()) {
        qDebug() << "Page range is empty";
        return;
    }

    QPainter p(printer);
    if (!p.isActive()) {
        qDebug() << "Painter is not active";
        return;
    }

    if (m_lastLayout.isValid() && !m_isNewPrint) {
        if (m_lastLayout == printer->pageLayout()) {
            // 如果打印属性没发生变化，直接加载已生成的资源，提高切换速度
            asynPrint(p, printer, pageRange);
            qDebug() << "Exit doPrintWithLargeDoc";
            return;
        }
    }

    //保留print的打印布局
    m_lastLayout = printer->pageLayout();
    m_isNewPrint = false;

    if (currentWrapper() == nullptr) {
        qDebug() << "Current wrapper is null";
        return;
    }

    qInfo() << qPrintable("Calc print large doc!");

    // 执行处理时屏蔽其它输入
    DPrintPreviewWidget *prieviewWidget = m_pPreview->findChild<DPrintPreviewWidget *>();
    QStringList findWidgetList {"leftWidget", "rightWidget"};
    QWidgetList widList;
    if (prieviewWidget) {
        // 通过设置刷新标识，防止频繁的触发刷新
        prieviewWidget->refreshBegin();

        for (QString widStr : findWidgetList) {
            QWidget *wid = m_pPreview->findChild<QWidget *>(widStr);
            if (wid) {
                wid->setEnabled(false);
                widList.append(wid);
            } else {
                qWarning() << "not find widget";
            }
        }

        QApplication::processEvents();
    }

    m_bPrintProcessing = true;
    QPointer<QObject> checkPtr(this);

    int dpiy = p.device()->logicalDpiY();
    int margin = static_cast<int>((2 / 2.54) * dpiy); // 2 cm margins

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRectF pageRect(printer->pageRect());
#else
    QRectF pageRect(printer->pageRect(QPrinter::Point));
#endif
    QRectF body = QRectF(0, 0, pageRect.width(), pageRect.height());
    // 使用首个文档信息即可
    QFontMetrics fontMetrics(m_printDocList.first().doc->defaultFont(), p.device());
    QRectF titleBox(margin,
                    body.bottom() - margin
                    + fontMetrics.height()
                    - 6 * dpiy / 72.0,
                    body.width() - 2 * margin,
                    fontMetrics.height());

    // Note:大文本打印的高亮处理被延迟到实际打印时计算
    m_multiDocPageCount = 0;
    for (auto &info : m_printDocList) {
        QTextDocument *printDoc = info.doc;

        QAbstractTextDocumentLayout *lay = info.doc->documentLayout();
        lay->setPaintDevice(p.device());

        auto fmt = printDoc->rootFrame()->frameFormat();
        fmt.setMargin(margin);
        printDoc->rootFrame()->setFrameFormat(fmt);

        // 设置打印大小
        printDoc->setPageSize(body.size());

        for (QTextBlock block = printDoc->firstBlock(); block.isValid(); block = block.next()) {
            // 通过对单个文本块进行布局，拆分布局总时间，防止布局时间过长
            lay->blockBoundingRect(block);
            // 处理其它事件
            qApp->processEvents(QEventLoop::AllEvents | QEventLoop::DialogExec);
            // 判断当前窗口是否关闭，退出打印状态
            if (!checkPtr || !m_bPrintProcessing) {
                qWarning() << "Abort print layout!";

                // 手动清理数据
                QObject::disconnect(m_pPreview, nullptr, this, nullptr);
                clearPrintTextDocument();

                // 异常退出、中断打印时需要关闭窗口
                QMetaObject::invokeMethod(m_pPreview, "reject", Qt::QueuedConnection);
                qDebug() << "Exit doPrintWithLargeDoc";
                return;
            }
        }

        // 更新文件总打印页数
        m_multiDocPageCount += printDoc->pageCount();
    }

    if (prieviewWidget) {
        qDebug() << "Print preview widget is valid";
        prieviewWidget->refreshEnd();
    }

    m_bPrintProcessing = false;
    for (auto wid : widList) {
        wid->setEnabled(true);
    }

    //输出总页码给到打印预览
    m_pPreview->setAsynPreview(m_multiDocPageCount);

    if (!m_printDocList.isEmpty()) {
        qDebug() << "Print doc list is not empty";
        //渲染第一页文本
        for (int i = 0; i < pageRange.count(); ++i) {
            if (pageRange[i] > m_multiDocPageCount)
                continue;
            printPageWithMultiDoc(pageRange[i], &p, m_printDocList, body, titleBox);
            if (i != pageRange.count() - 1)
                printer->newPage();
        }
    }

    qInfo() << qPrintable("Calc print large doc finised!");
}
#endif

/*!
 * \~chinese \brief Window::asynPrint 进行翻页预览打印
 * \~chinese \param pageRange 打印预览请求的页码列表
 */
void Window::asynPrint(QPainter &p, DPrinter *printer, const QVector<int> &pageRange)
{
    qDebug() << "Enter asynPrint";
    if ((!m_bLargePrint && nullptr == m_printDoc)
            || (m_bLargePrint && m_printDocList.isEmpty())) {
        qWarning() << "The print document is not valid!";
        return;
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRectF pageRect(printer->pageRect());
#else
    QRectF pageRect(printer->pageRect(QPrinter::Point));
#endif
    int dpiy = p.device()->logicalDpiY();
    int margin = static_cast<int>((2 / 2.54) * dpiy); // 2 cm margins
    QRectF body = QRectF(0, 0, pageRect.width(), pageRect.height());

    QTextDocument *curDoc = m_bLargePrint ? m_printDocList.first().doc : m_printDoc;
    QFontMetrics fontMetrics(curDoc->defaultFont(), p.device());
    QRectF titleBox(margin,
                    body.bottom() - margin
                    + fontMetrics.height()
                    - 6 * dpiy / 72.0,
                    body.width() - 2 * margin,
                    fontMetrics.height());

    if (m_bLargePrint
            && !m_printDocList.isEmpty()) {
        qDebug() << "Large print and print doc list is not empty";
        // 大文本打印
        for (int i = 0; i < pageRange.count(); ++i) {
            if (pageRange[i] > m_multiDocPageCount)
                continue;
            printPageWithMultiDoc(pageRange[i], &p, m_printDocList, body, titleBox);
            if (i != pageRange.count() - 1)
                printer->newPage();
        }
    } else {
        qDebug() << "Small print or print doc list is empty";
        for (int i = 0; i < pageRange.count(); ++i) {
            if (pageRange[i] > m_printDoc->pageCount())
                continue;
            Window::printPage(pageRange[i], &p, m_printDoc, body, titleBox);
            if (i != pageRange.count() - 1)
                printer->newPage();
        }
    }

    qDebug() << "Exit asynPrint";
}

void Window::backupFile()
{
    qInfo() << "begin backupFile()";
    if (!QFileInfo(m_backupDir).exists()) {
        qInfo() << "backupDir not exist, create it";
        QDir().mkpath(m_backupDir);
    }

    QMap<QString, EditWrapper *> wrappers = m_wrappers;
    QStringList listBackupInfo;
    QString filePath, localPath, curPos;
    QFileInfo fileInfo;
    m_qlistTemFile.clear();
    m_qlistTemFile = wrappers.keys();

    for (EditWrapper *wrapper : wrappers) {
        if (nullptr == wrapper) {
            qWarning() << "wrapper is null, continue";
            continue;
        }
        if (wrapper->getFileLoading()) continue;

        if (nullptr == wrapper->textEditor()) {
            qWarning() << "textEditor is null, continue";
            continue;
        }

        filePath = wrapper->textEditor()->getFilePath();
        localPath = wrapper->textEditor()->getTruePath();

        if (localPath.isEmpty()) {
            qWarning() << "localPath is empty, use filePath instead";
            localPath = filePath;
        }

        qInfo() << "begin backupFile()";
        StartManager::FileTabInfo tabInfo = StartManager::instance()->getFileTabInfo(filePath);
        curPos = QString::number(wrapper->textEditor()->textCursor().position());
        fileInfo.setFile(localPath);

        QJsonObject jsonObject;
        QJsonDocument document;
        jsonObject.insert("localPath", localPath);
        jsonObject.insert("cursorPosition", QString::number(wrapper->textEditor()->textCursor().position()));
        jsonObject.insert("modify", wrapper->isModified());
        jsonObject.insert("lastModifiedTime", wrapper->getLastModifiedTime().toString());
        QList<int> bookmarkList = wrapper->textEditor()->getBookmarkInfo();
        if (!bookmarkList.isEmpty()) {
            qInfo() << "bookmarkList is not empty";
            QString bookmarkInfo;

            //记录书签
            for (int i = 0; i < bookmarkList.count(); i++) {
                if (i == bookmarkList.count() - 1) {
                    bookmarkInfo.append(QString::number(bookmarkList.value(i)));
                } else {
                    bookmarkInfo.append(QString::number(bookmarkList.value(i)) + ",");
                }
            }

            jsonObject.insert("bookMark", bookmarkInfo);
        }

        //记录活动页
        if (filePath == m_tabbar->currentPath()) {
            qInfo() << "current tab is this file, set focus to true";
            jsonObject.insert("focus", true);
        }

        //保存备份文件
        if (Utils::isDraftFile(filePath)) {
            qInfo() << "is draft file, save to draft dir";
            wrapper->saveTemFile(filePath);
        } else {
            if (wrapper->isModified()) {
                qInfo() << "is modified, save to backup dir";
                QString name = fileInfo.absolutePath().replace("/", "_");
                QString qstrFilePath = m_backupDir + "/" + Utils::getStringMD5Hash(fileInfo.baseName()) + "." + name + "." + fileInfo.suffix();
                jsonObject.insert("temFilePath", qstrFilePath);
                wrapper->saveTemFile(qstrFilePath);
            }
        }

        qInfo() << "after call wrapper->saveTemFile()";

        //使用json串形式保存
        document.setObject(jsonObject);
        QByteArray byteArray = document.toJson(QJsonDocument::Compact);
        m_qlistTemFile.replace(tabInfo.tabIndex, byteArray);
    }

    //将json串列表写入配置文件
    m_settings->settings->option("advance.editor.browsing_history_temfile")->setValue(m_qlistTemFile);

    //删除自动备份文件
    if (QFileInfo(m_autoBackupDir).exists()) {
        qInfo() << "autoBackupDir exist, remove it";
        QDir(m_autoBackupDir).removeRecursively();
    }

    qInfo() << "end backupFile()";
}

bool Window::closeAllFiles()
{
    qInfo() << "begin closeAllFiles()";
    QMap<QString, EditWrapper *> wrappers = m_wrappers;

    // 被删除的窗口索引已变更，需要计算其范围
    int closedTabCount = 0;
    //关闭所有文件
    for (int i = 0; i < wrappers.count(); i++) {
        // 窗口索引 - 已删除窗口索引
        m_tabbar->setCurrentIndex(i - closedTabCount);

        if (!closeTab()) {
            qDebug() << "closeTab() failed, return false";
            // 取消操作时，停留在当前标签页
            return false;
        } else {
            // 窗口已删除，调整索引
            ++closedTabCount;
        }
    }
    qInfo() << "end closeAllFiles()";
    return true;
}

bool Window::saveAllFloatingFiles()
{
    qInfo() << "begin saveAllFloatingFiles()";
    QMap<QString, EditWrapper *> wrappers = m_wrappers;
    for (int i = wrappers.count() - 1; i >= 0; i--) {
        const QString &filePath = m_tabbar->truePathAt(i);
        // 避免异常情况重入时当前已无标签页的情况
        if (filePath.isEmpty()) {
            return false;
        }

        QFileInfo finfo(filePath);
        if (!finfo.exists()) {
            qWarning() << "File not exist:" << filePath;
            m_tabbar->setCurrentIndex(i);
            DDialog *dialog = createDialog(tr("Do you want to save this file?"), "");
            int res = dialog->exec();

            //取消或关闭弹窗不做任务操作
            if (res == 0 || res == -1) {
                return false;
            }

            //不保存
            if (res == 1) {
                continue;
            }

            //保存
            if (res == 2) {
                saveAsFile();
            }
        }
    }
    qInfo() << "end saveAllFloatingFiles()";
    return true;
}

/**
 * @brief addTemFileTab　恢复备份文件标签页
 * @param qstrPath　打开文件路径
 * @param qstrName　真实文件名
 * @param qstrTruePath　真实文件路径
 * @param qstrTruePath　最后一次修改时间
 * @param bIsTemFile　是否修改
 */
void Window::addTemFileTab(const QString &qstrPath, const QString &qstrName, const QString &qstrTruePath, const QString &lastModifiedTime, bool bIsTemFile)
{
    qInfo() << "begin addTemFileTab()";
    if (qstrPath.isEmpty() || !Utils::fileExists(qstrPath)) {
        qWarning() << "qstrPath is empty or file not exists";
        return;
    }

    // 若为MIMETYPE不支持的文件，给出无效文件提示 修复bug 132653
    if (!Utils::isMimeTypeSupport(qstrPath)) {
        qDebug() << "file is not support, show invalid file tip";
        if (currentWrapper() == nullptr) {
            qDebug() << "current wrapper is null, create a new one";
            this->addBlankTab();
        }

        QString strFileName = QFileInfo(qstrPath).fileName();
        strFileName = tr("Invalid file: %1").arg(strFileName);
        strFileName = Utils::lineFeed(strFileName, m_editorWidget->currentWidget()->width() - FLOATTIP_MARGIN, m_editorWidget->currentWidget()->font(), 2);
        DMessageManager::instance()->sendMessage(m_editorWidget->currentWidget(), QIcon(":/images/warning.svg"), strFileName);

        qDebug() << "return from addTemFileTab()";
        return;
    }

    EditWrapper *wrapper = createEditor();
    m_tabbar->addTab(qstrPath, qstrName, qstrTruePath);
    wrapper->openFile(qstrPath, qstrTruePath, bIsTemFile);

    // 查找文件是否存在书签，临时文件同样可标记书签
    auto bookmarkInfo = StartManager::instance()->findBookmark(qstrTruePath);
    wrapper->textEditor()->setBookMarkList(bookmarkInfo);

    //set m_tModifiedDateTime in wrapper again.
    if (bIsTemFile && !lastModifiedTime.isEmpty()) {
        wrapper->setLastModifiedTime(lastModifiedTime);
        qDebug() << "set last modified time in wrapper";
    }
    m_wrappers[qstrPath] = wrapper;
    showNewEditor(wrapper);
    qInfo() << "end addTemFileTab()";
}

QMap<QString, EditWrapper *> Window::getWrappers()
{
    return m_wrappers;
}

void Window::setChildrenFocus(bool ok)
{
    qDebug() << "setChildrenFocus" << ok;
    if (ok) {
        DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
        DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
        DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
        DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
        DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
        DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");

        titlebar()->setFocusPolicy(Qt::TabFocus);
        if (addButton) addButton->setFocusPolicy(Qt::TabFocus);
        if (optionBtn) optionBtn->setFocusPolicy(Qt::TabFocus);
        if (minBtn) minBtn->setFocusPolicy(Qt::TabFocus);
        if (quitFullBtn) quitFullBtn->setFocusPolicy(Qt::TabFocus);
        if (maxBtn) maxBtn->setFocusPolicy(Qt::TabFocus);
        if (closeBtn) closeBtn->setFocusPolicy(Qt::TabFocus);

        setTabOrder(addButton, optionBtn);
        setTabOrder(optionBtn, minBtn);
        setTabOrder(minBtn, quitFullBtn);
        setTabOrder(quitFullBtn, maxBtn);
        setTabOrder(maxBtn, closeBtn);
    } else {
        DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
        DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
        DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
        DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
        DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
        DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");

        titlebar()->setFocusPolicy(Qt::NoFocus);
        if (addButton) addButton->setFocusPolicy(Qt::NoFocus);
        if (optionBtn) optionBtn->setFocusPolicy(Qt::NoFocus);
        if (minBtn) minBtn->setFocusPolicy(Qt::NoFocus);
        if (quitFullBtn) quitFullBtn->setFocusPolicy(Qt::NoFocus);
        if (maxBtn) maxBtn->setFocusPolicy(Qt::NoFocus);
        if (closeBtn) closeBtn->setFocusPolicy(Qt::NoFocus);
    }
    qDebug() << "setChildrenFocus end";
}

void Window::addBlankTab()
{
    addBlankTab("");
}

void Window::addBlankTab(const QString &blankFile)
{
    qInfo() << "begin addBlankTab()";
    QString blankTabPath;

    if (blankFile.isEmpty()) {
        qInfo() << "blankFile is empty, create a new blank file";
        const QString &fileName = QString("blank_file_%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz"));
        blankTabPath = QDir(m_blankFileDir).filePath(fileName);

        if (!Utils::fileExists(blankTabPath)) {
            QDir().mkpath(m_blankFileDir);
        }
    } else {
        qInfo() << "blankFile is not empty, use it";
        blankTabPath = blankFile;
    }

    int blankFileIndex = getBlankFileIndex();
    m_tabbar->addTab(blankTabPath, tr("Untitled %1").arg(blankFileIndex));
    EditWrapper *wrapper = createEditor();
    wrapper->updatePath(blankTabPath);

    if (!blankFile.isEmpty() && Utils::fileExists(blankFile)) {
        qInfo() << "blankFile exists, copy it to blankTabPath";
        wrapper->openFile(blankTabPath, blankTabPath);
    }

    m_wrappers[blankTabPath] = wrapper;
    showNewEditor(wrapper);
    qInfo() << "end addBlankTab()";
}

/**
 * @brief 标签页关闭请求处理
 * @param index 标签页索引
 *
 * @note 由于使用 Qt::QueuedConnection 模式，在界面被其它任务阻塞时，可能触发多次关闭事件,
 *      导致队列中存在多个请求关闭任务，阻塞结束后，重复触发，删除所有的标签页，导致窗口关闭。
 */
void Window::handleTabCloseRequested(int index)
{
    qDebug() << "handleTabCloseRequested" << index;
    // 延迟执行关闭标签页
    if (!m_delayCloseTabTimer.isActive()) {
        qDebug() << "m_delayCloseTabTimer is not active";
        // 记录关闭标签索引
        m_requestCloseTabIndex = index;

        // 延迟10ms
        static const int s_nDelayTime = 10;
        m_delayCloseTabTimer.start(s_nDelayTime, this);
    }
    qDebug() << "handleTabCloseRequested end";
}

void Window::handleTabsClosed(QStringList tabList)
{
    qDebug() << "handleTabsClosed, size=" << tabList.size();
    if (tabList.isEmpty()) {
        qDebug() << "tabList is empty, return";
        return;
    }

    for (const QString &path : tabList) {
        int index = m_tabbar->indexOf(path);
        m_tabbar->setCurrentIndex(index);
        closeTab();
    }
    qDebug() << "handleTabsClosed end";
}

void Window::handleCurrentChanged(const int &index)
{
    qDebug() << "handleCurrentChanged" << index;

    if (m_findBar->isVisible()) {
        qDebug() << "m_findBar is visible";
        m_findBar->hide();
    }

    if (m_replaceBar->isVisible()) {
        qDebug() << "m_replaceBar is visible";
        m_replaceBar->hide();
    }

    if (m_jumpLineBar->isVisible()) {
        qDebug() << "m_jumpLineBar is visible";
        m_jumpLineBar->hide();
    }

    for (auto wrapper : m_wrappers.values()) {
        wrapper->textEditor()->removeKeywords();
    }

    if (currentWrapper()) {
        qDebug() << "current wrapper is not null";
        currentWrapper()->checkForReload();
        checkTabbarForReload();
    }

    const QString &filepath = m_tabbar->fileAt(index);

    if (m_wrappers.contains(filepath)) {
        bool bIsContains = false;
        EditWrapper *wrapper = m_wrappers.value(filepath);
        if (!wrapper || !wrapper->textEditor()) {
             qWarning() << "Failed to get wrapper or text editor in handleCurrentChanged";
             // Attempt to remove potentially problematic widget if wrapper is null but was in m_wrappers
             if (!wrapper && m_editorWidget) {
                 // Find widget by path if possible, or handle error
             }
             return; // Exit early if wrapper or editor is invalid
         }
        wrapper->textEditor()->setFocus();
        for (int i = 0; i < m_editorWidget->count(); i++) {
            if (m_editorWidget->widget(i) == wrapper) {
                qDebug() << "wrapper is already in m_editorWidget";
                bIsContains = true;
            }
        }

        if (!bIsContains) {
            qDebug() << "wrapper is not in m_editorWidget, add it";
            m_editorWidget->addWidget(wrapper);
        }

        m_editorWidget->setCurrentWidget(wrapper);
    }

    if (currentWrapper() != nullptr) {
        qDebug() << "current wrapper is not null, show bottom bar";
        currentWrapper()->bottomBar()->show();
        currentWrapper()->bottomBar()->updateSize(BottomBar::defaultHeight(), false);
    }
    qDebug() << "handleCurrentChanged end";
}

void Window::handleJumpLineBarExit()
{
    qDebug() << "handleJumpLineBarExit";
    if (currentWrapper() != nullptr) {
        qDebug() << "current wrapper is not null";
        QTimer::singleShot(0, currentWrapper()->textEditor(), SLOT(setFocus()));
    }
    qDebug() << "handleJumpLineBarExit end";
}

void Window::handleJumpLineBarJumpToLine(const QString &filepath, int line, bool focusEditor)
{
    qDebug() << "handleJumpLineBarJumpToLine" << filepath << line << focusEditor;
    if (m_wrappers.contains(filepath)) {
        getTextEditor(filepath)->jumpToLine(line, true);

        if (focusEditor) {
            qDebug() << "focus editor";
            QTimer::singleShot(0, getTextEditor(filepath), SLOT(setFocus()));
        }
    }
    qDebug() << "handleJumpLineBarJumpToLine end";
}

void Window::handleBackToPosition(const QString &file, int row, int column, int scrollOffset)
{
    qDebug() << "handleBackToPosition" << file << row << column << scrollOffset;
    if (m_wrappers.contains(file)) {
        EditWrapper* wrapper = m_wrappers.value(file);
        if (!wrapper || !wrapper->textEditor()) {
             qWarning() << "Failed to get wrapper or text editor in handleBackToPosition";
             return;
         }
        wrapper->textEditor()->scrollToLine(scrollOffset, row, column);
    }
    qDebug() << "handleBackToPosition end";
}

void Window::handleFindNextSearchKeyword(const QString &keyword)
{
    qDebug() << "handleFindNextSearchKeyword" << keyword;
    handleFindKeyword(keyword, true);
    qDebug() << "handleFindNextSearchKeyword end";
}

void Window::handleFindPrevSearchKeyword(const QString &keyword)
{
    qDebug() << "handleFindPrevSearchKeyword" << keyword;
    handleFindKeyword(keyword, false);
    qDebug() << "handleFindPrevSearchKeyword end";
}

void Window::handleFindKeyword(const QString &keyword, bool state)
{
    qDebug() << "handleFindKeyword" << keyword << state;
    EditWrapper *wrapper = currentWrapper();
    m_keywordForSearch = keyword;
    wrapper->textEditor()->saveMarkStatus();
    wrapper->textEditor()->updateCursorKeywordSelection(m_keywordForSearch, state);
    if (QString::compare(m_keywordForSearch, m_keywordForSearchAll, Qt::CaseInsensitive) != 0) {
        qDebug() << "m_keywordForSearchAll is not equal to m_keywordForSearch, clear it";
        m_keywordForSearchAll.clear();
        wrapper->textEditor()->clearFindMatchSelections();
    } else {
        qDebug() << "m_keywordForSearchAll is equal to m_keywordForSearch, highlight it";
        wrapper->textEditor()->highlightKeywordInView(m_keywordForSearchAll);
    }

    wrapper->textEditor()->markAllKeywordInView();

    wrapper->textEditor()->renderAllSelections();
    wrapper->textEditor()->restoreMarkStatus();
    wrapper->textEditor()->updateLeftAreaWidget();

    // 变更查询字符串位置后(可能滚屏)，刷新当前界面的代码高亮效果
    wrapper->OnUpdateHighlighter();
    qDebug() << "handleFindKeyword end";
}

void Window::slotFindbarClose()
{
    qDebug() << "slotFindbarClose";
    EditWrapper *wrapper = currentWrapper();

    if (wrapper->bottomBar()->isHidden()) {
        qDebug() << "bottom bar is hidden, show it";
        wrapper->bottomBar()->show();
    }

    wrapper->bottomBar()->updateSize(BottomBar::defaultHeight(), false);
    currentWrapper()->textEditor()->setFocus();
    currentWrapper()->textEditor()->tellFindBarClose();
    qDebug() << "slotFindbarClose end";
}

void Window::slotReplacebarClose()
{
    qDebug() << "slotReplacebarClose";
    EditWrapper *wrapper = currentWrapper();

    if (wrapper->bottomBar()->isHidden()) {
        qDebug() << "bottom bar is hidden, show it";
        wrapper->bottomBar()->show();
    }

    wrapper->bottomBar()->updateSize(BottomBar::defaultHeight(), false);
    currentWrapper()->textEditor()->setFocus();
    currentWrapper()->textEditor()->tellFindBarClose();
    qDebug() << "slotReplacebarClose end";
}

void Window::slotSwitchToReplaceBar()
{
    qDebug() << "slotSwitchToReplaceBar - switching from find bar to replace bar";
    
    // 获取当前查找栏中的搜索文本和位置信息
    QString searchText;
    QString currentFile;
    int row = 0, column = 0, scrollOffset = 0;
    
    if (m_findBar && m_findBar->isVisible()) {
        searchText = m_findBar->getCurrentSearchText();
        qDebug() << "Got search text from find bar:" << searchText;
        
        // 获取当前编辑器的位置信息
        EditWrapper *wrapper = currentWrapper();
        if (wrapper && wrapper->textEditor()) {
            currentFile = wrapper->textEditor()->getFilePath();
            QTextCursor cursor = wrapper->textEditor()->textCursor();
            row = cursor.blockNumber();
            column = cursor.columnNumber();
            scrollOffset = wrapper->textEditor()->verticalScrollBar()->value();
            qDebug() << "Current position - file:" << currentFile << "row:" << row << "column:" << column;
            if (!searchText.isEmpty()) {
                // 检查当前光标是否有选中文本，且选中文本与搜索文本匹配
                QTextCursor currentCursor = wrapper->textEditor()->textCursor();
                if (currentCursor.hasSelection() && currentCursor.selectedText() == searchText) {
                    qDebug() << "Current selection matches search text, using it as search pointer";
                    // 直接将当前选中文本设置为搜索指针
                    wrapper->textEditor()->setFindHighlightSelection(currentCursor);
                } else {
                    qDebug() << "No matching selection, setting search pointer from current position";
                    // 没有匹配的选中文本，从当前位置搜索
                    wrapper->textEditor()->updateCursorKeywordSelection(searchText, true);
                }
            }
        }
        
        // 隐藏查找栏
        m_findBar->hide();
        qDebug() << "Find bar hidden";
    }
    
    // 弹出替换栏并传递搜索文本
    if (currentWrapper() == nullptr) {
        qDebug() << "No current wrapper, cannot popup replace bar";
        return;
    }

    if (currentWrapper()->textEditor()->document()->isEmpty()) {
        qDebug() << "Current document is empty, cannot popup replace bar";
        return;
    }
    
    // 激活替换栏并传递搜索文本
    m_replaceBar->activeInput(searchText, currentFile, row, column, scrollOffset);
    qDebug() << "Replace bar activated with search text:" << searchText;
}

void Window::handleReplaceAll(const QString &replaceText, const QString &withText)
{
    qDebug() << "handleReplaceAll" << replaceText << withText;
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->replaceAll(replaceText, withText);
    qDebug() << "handleReplaceAll end";
}

void Window::handleReplaceNext(const QString &file, const QString &replaceText, const QString &withText)
{
    qDebug() << "handleReplaceNext" << file << replaceText << withText;
    Q_UNUSED(file);
    m_keywordForSearch = replaceText;
    m_keywordForSearchAll = replaceText;
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->replaceNext(replaceText, withText);
    qDebug() << "handleReplaceNext end";
}

void Window::handleReplaceRest(const QString &replaceText, const QString &withText)
{
    qDebug() << "handleReplaceRest" << replaceText << withText;
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->replaceRest(replaceText, withText);
    qDebug() << "handleReplaceRest end";
}

void Window::handleReplaceSkip(QString file, QString keyword)
{
    qDebug() << "handleReplaceSkip" << file << keyword;
    EditWrapper *wrapper = currentWrapper();
    handleUpdateSearchKeyword(m_replaceBar, file, keyword);
    if (QString::compare(m_keywordForSearch, m_keywordForSearchAll, Qt::CaseInsensitive) != 0) {
        m_keywordForSearchAll.clear();
        wrapper->textEditor()->clearFindMatchSelections();
        qDebug() << "m_keywordForSearchAll is not equal to m_keywordForSearch, clear it";
    } else {
        wrapper->textEditor()->highlightKeywordInView(m_keywordForSearchAll);
        qDebug() << "m_keywordForSearchAll is equal to m_keywordForSearch, highlight it";
    }
#if 0
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->updateCursorKeywordSelection(m_keywordForSearchAll, true);
    wrapper->textEditor()->renderAllSelections();
#endif
    qDebug() << "handleReplaceSkip end";
}

void Window::handleRemoveSearchKeyword()
{
    qDebug() << "handleRemoveSearchKeyword";
    if (currentWrapper() != nullptr) {
        qDebug() << "remove keywords";
        currentWrapper()->textEditor()->removeKeywords();
    }
    qDebug() << "handleRemoveSearchKeyword end";
}

void Window::handleUpdateSearchKeyword(QWidget *widget, const QString &file, const QString &keyword)
{
    qDebug() << "handleUpdateSearchKeyword" << file << keyword;
    if (file == m_tabbar->currentPath() && m_wrappers.contains(file)) {
        qDebug() << "update search keyword";
        EditWrapper* wrapper = m_wrappers.value(file);
        if (!wrapper || !wrapper->textEditor()) {
             qWarning() << "Failed to get wrapper or text editor in handleUpdateSearchKeyword";
             return;
         }

        // Update input widget warning status along with keyword match situation.
        bool findKeyword = wrapper->textEditor()->highlightKeyword(keyword, wrapper->textEditor()->getPosition());
        m_keywordForSearchAll = keyword;
        m_keywordForSearch = keyword;
        bool emptyKeyword = keyword.trimmed().isEmpty();

        auto *findBarWidget = qobject_cast<FindBar *>(widget);
        if (findBarWidget != nullptr) {
            qDebug() << "find bar widget is not null";
            if (emptyKeyword) {
                findBarWidget->setMismatchAlert(false);
                qDebug() << "empty keyword, set mismatch alert false";
            } else {
                findBarWidget->setMismatchAlert(!findKeyword);
                qDebug() << "not empty keyword, set mismatch alert " << !findKeyword;
            }
        } else {
            qDebug() << "find bar widget is null";
            auto *replaceBarWidget = qobject_cast<ReplaceBar *>(widget);
            if (replaceBarWidget != nullptr) {
                if (emptyKeyword) {
                    replaceBarWidget->setMismatchAlert(false);
                    qDebug() << "empty keyword, set mismatch alert false";
                } else {
                    replaceBarWidget->setMismatchAlert(false);
                    qDebug() << "not empty keyword, set mismatch alert false";
                }
            }
        }
    }
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->updateLeftAreaWidget();
    // 在设置查询字符串并跳转后，及时刷新代码高亮效果
    wrapper->OnUpdateHighlighter();
    qDebug() << "handleUpdateSearchKeyword end";
}

void Window::loadTheme(const QString &path)
{
    qInfo() << "Loading theme:" << path;

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        qWarning() << "Theme file not exists!" << path;
        return;
    }

    m_themePath = path;

    QVariantMap jsonMap = Utils::getThemeMapFromPath(path);
    const QString &backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
    const QString &tabbarStartColor = jsonMap["app-colors"].toMap()["tab-background-start-color"].toString();
    const QString &tabbarEndColor = jsonMap["app-colors"].toMap()["tab-background-end-color"].toString();


    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->OnThemeChangeSlot(m_themePath);
    }

    m_themePanel->setBackground(backgroundColor);
    m_tabbar->setBackground(tabbarStartColor, tabbarEndColor);
    m_tabbar->setDNDColor(jsonMap["app-colors"].toMap()["tab-dnd-start"].toString(), jsonMap["app-colors"].toMap()["tab-dnd-end"].toString());

    const QString &frameSelectedColor = jsonMap["app-colors"].toMap()["themebar-frame-selected"].toString();
    const QString &frameNormalColor = jsonMap["app-colors"].toMap()["themebar-frame-normal"].toString();
    m_themePanel->setFrameColor(frameSelectedColor, frameNormalColor);
    m_settings->settings->option("advance.editor.theme")->setValue(path);
    qDebug() << "load theme end";
}

void Window::showNewEditor(EditWrapper *wrapper)
{
    qDebug() << "show new editor";
    m_editorWidget->addWidget(wrapper);
    m_editorWidget->setCurrentWidget(wrapper);
    qDebug() << "show new editor end";
}

void Window::showNotify(const QString &message, bool warning)
{
    qDebug() << "show notify" << message << warning;
    //DMainWindow::sendMessage(QIcon(":/images/ok.svg"), message);
    //如果是第一次打开编辑器，需要创建一个空白编辑显示框窗体
    if (currentWrapper() == nullptr) {
        qDebug() << "current wrapper is null";
        this->addBlankTab();
    }
    currentWrapper()->showNotify(message, warning);
    qDebug() << "show notify end";
}

int Window::getBlankFileIndex()
{
    qDebug() << "get blank file index";
    // get blank tab index list.
    QList<int> tabIndexes;

    // tabFiles.size()
    for (int i = 0; i < m_tabbar->count(); ++i) {
        // find all the blank tab index number.
        QFileInfo fileInfo(m_tabbar->fileAt(i));
        if (fileInfo.dir().absolutePath() == m_blankFileDir) {
            const QString tabText = m_tabbar->textAt(i);
            QRegularExpression reg("(\\d+)");
            QRegularExpressionMatch match = reg.match(tabText);

            tabIndexes << match.captured(1).toInt();
        }
    }
    std::sort(tabIndexes.begin(), tabIndexes.end());

    // Return 1 if no blank file exists.
    if (tabIndexes.size() == 0) {
        qDebug() << "no blank file, return 1";
        return 1;
    }

    // Return first mismatch index as new blank file index.
    for (int j = 0; j < tabIndexes.size(); j++) {
        if (tabIndexes[j] != j + 1) {
            return j + 1;
        }
    }

    // Last, return biggest index as blank file index.
    int lastIndex = tabIndexes.size() + 1;
    qDebug() << "last blank file index is " << lastIndex;
    return lastIndex;
}

DDialog *Window::createDialog(const QString &title, const QString &content)
{
    qDebug() << "create dialog";
    DDialog *dialog = new DDialog(title, content, this);
    //dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnBottomHint);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setIcon(QIcon::fromTheme("deepin-editor"));
    dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);

    qDebug() << "create dialog end";
    return dialog;
}

void Window::slotLoadContentTheme(DGuiApplicationHelper::ColorType themeType)
{
    qDebug() << "slot load content theme";
    if (themeType == DGuiApplicationHelper::ColorType::LightType) {
        qDebug() << "load light theme";
        loadTheme(DEEPIN_THEME);
        if (DGuiApplicationHelper::instance()->paletteType() == DGuiApplicationHelper::ColorType::UnknownType) {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::ColorType::UnknownType);
        }
    } else if (themeType == DGuiApplicationHelper::ColorType::DarkType) {
        qDebug() << "load dark theme";
        loadTheme(DEEPIN_DARK_THEME);
        if (DGuiApplicationHelper::instance()->paletteType() == DGuiApplicationHelper::ColorType::UnknownType) {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::ColorType::UnknownType);
        }
    }

    QString qstrColor = palette().color(QPalette::Active, QPalette::Text).name();

    for (auto wrapper : m_wrappers) {
        wrapper->textEditor()->setEditPalette(qstrColor, qstrColor);
        wrapper->OnUpdateHighlighter();
    }

    qstrColor = palette().color(QPalette::Active, QPalette::ButtonText).name();
    QString qstrHighlightColor = palette().color(QPalette::Active, QPalette::HighlightedText).name();

    m_tabbar->setTabPalette(qstrColor, qstrHighlightColor);
    qDebug() << "slot setting reset theme end";
}

void Window::slotSettingResetTheme(const QString &path)
{
    qDebug() << "slot setting reset theme";
    if (path == DEEPIN_THEME) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            qDebug() << "light theme, return";
            return;
        } else {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
            qDebug() << "set light theme";
        }
    } else if (path == DEEPIN_DARK_THEME) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
            qDebug() << "dark theme, return";
            return;
        } else {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::DarkType);
            qDebug() << "set dark theme";
        }
    }
    qDebug() << "slot setting reset theme end";
}

void Window::slot_saveReadingPath()
{
    qDebug() << "slot save reading path";
    m_reading_list.clear();
    m_reading_list.append(currentWrapper()->textEditor());
    qDebug() << "slot save reading path end";
}

void Window::slot_beforeReplace(QString _)
{
    qDebug() << "slot before replace";
    currentWrapper()->textEditor()->beforeReplace(_);
}

void Window::slot_setTitleFocus()
{
    qDebug() << "slot set title focus";
    QMap<QString, EditWrapper *>::Iterator it = m_wrappers.begin();
    for (; it != m_wrappers.end(); it++) {
        it.value()->bottomBar()->setChildrenFocus(true);
    }

    titlebar()->setFocusPolicy(Qt::TabFocus);
    titlebar()->setFocus(Qt::MouseFocusReason);
    DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
    addButton->setFocusPolicy(Qt::TabFocus);
    DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
    optionBtn->setFocusPolicy(Qt::TabFocus);
    DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
    minBtn->setFocusPolicy(Qt::TabFocus);

    DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
    quitFullBtn->setFocusPolicy(Qt::TabFocus);
    DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
    maxBtn->setFocusPolicy(Qt::TabFocus);
    DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");
    closeBtn->setFocusPolicy(Qt::TabFocus);
    QWidget::setTabOrder(addButton, optionBtn);
    QWidget::setTabOrder(optionBtn, minBtn);
    QWidget::setTabOrder(minBtn, quitFullBtn);
    QWidget::setTabOrder(quitFullBtn, maxBtn);
    QWidget::setTabOrder(maxBtn, closeBtn);
    currentWrapper()->bottomBar()->setChildrenFocus(true, closeBtn);
    qDebug() << "slot set title focus end";
}

void Window::slotClearDoubleCharaterEncode()
{
    qDebug() << "slot clear double charater encode";
    //赛迪方要求不能出现以下字符，但是编码库中存在，所以手动去掉
    QStringList shouldBeEmpty;
    shouldBeEmpty << "\uE768" << "\uE769" << "\uE76A" << "\uE76B" << "\uE76D" << "\uE76E" << "\uE76F" << "\uE766" << "\uE767" << "\uE770"
                  << "\uE771" << "\uE777" << "\uE778" << "\uE779" << "\uE77A" << "\uE77B" << "\uE77C" << "\uE77D" << "\uE77E" << "\uE77F" << "\uE7FE" << "\uE7FF"
                  << "\uE801" << "\uE802" << "\uE803" << "\uE804" << "\uE805" << "\uE806" << "\uE807" << "\uE808" << "\uE809" << "\uE80A" << "\uE80B" << "\uE80C" << "\uE80D" << "\uE80E"
                  << "\uE80F" << "\uE800" << "\uE7D3" << "\uE7D4" << "\uE7D5" << "\uE7D6" << "\uE7D7" << "\uE7D8" << "\uE7D9" << "\uE7DA" << "\uE7DB" << "\uE7DC" << "\uE7DD"
                  << "\uE7DE" << "\uE7DF" << "\uE7E0" << "\uE7E1" << "\uE7CD" << "\uE7CE" << "\uE7CF" << "\uE7D0" << "\uE7D1" << "\uE7D2" << "\uE7AF" << "\uE7B0" << "\uE7B1" << "\uE7B2"
                  << "\uE7B3" << "\uE7B4" << "\uE7B5" << "\uE7B6" << "\uE7B7" << "\uE7B8" << "\uE7B9" << "\uE7BA" << "\uE7BB" << "\uE7A0" << "\uE7A1" << "\uE7A2" << "\uE7A3" << "\uE7A4"
                  << "\uE7A5" << "\uE7A6" << "\uE7A7" << "\uE7A8" << "\uE7A9" << "\uE7AA" << "\uE7AB" << "\uE7AC" << "\uE7AD" << "\uE7AE" << "\uE797" << "\uE798" << "\uE799" << "\uE79A"
                  << "\uE79B" << "\uE79C" << "\uE79D" << "\uE79E" << "\uE79F" << "\uE780" << "\uE781" << "\uE782" << "\uE783" << "\uE784" << "\uE772" << "\uE773" << "\uE774" << "\uE775"
                  << "\uE776" << "\uE78D" << "\uE78E" << "\uE78F" << "\uE790" << "\uE791" << "\uE792" << "\uE793" << "\uE796"
                  << "\uE7BC" << "\uE7BD" << "\uE7BE" << "\uE7BF" << "\uE7C0" << "\uE7C1" << "\uE7C2" << "\uE7C3" << "\uE7C4"
                  << "\uE7C5" << "\uE7C6" << "\uE7E3" << "\uE7E4" << "\uE7E5" << "\uE7E6" << "〾⿰⿱⿲⿳⿴⿵" << "\uE7F4" << "\uE7F5" << "\uE7F6"
                  << "\uE7F7" << "\uE7F8" << "\uE7F9" << "\uE7FA" << "\uE7FB" << "\uE7FC" << "⿶⿷⿸⿹⿺⿻" << "\uE7FD"
                  << "\u4DB6" << "\uE26C" << "\uE28F" << "\uE290" << "\uE291" << "\uE292" << "\uE293" << "\uE294" << "\uE295" << "\uE296" << "\uE297" << "\uE298" << "\uE299"
                  << "\uE29A" << "\uE29B" << "\uE29C" << "\uE29D" << "\uE29E" << "\uE29F" << "\uE26D"
                  << "\uE644" << "\uE645" << "\uE645" << "\uE646" << "\uE647" << "\uE648" << "\uE649" << "\uE64A" << "\uE64B" << "\uE64C" << "\uE64D" << "\uE64E" << "\uE64F"
                  << "\uE680" << "\uE681" << "\uE682" << "\uE683" << "\uE686" << "\uE688" << "\uE689" << "\uE68A" << "\uE68B" << "\uE68C" << "\uE68D" << "\uE6AC" << "\uE6AD" << "\uE6AE";

    for (const QString &strTemp : shouldBeEmpty) {
        handleReplaceAll(strTemp, " ");
    }
    qDebug() << "slot clear double charater encode end";
}

void Window::slotSigThemeChanged(const QString &path)
{
    qDebug() << "slot sig theme changed";
    if (path == DEEPIN_THEME) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            qDebug() << "light theme, return";
            return;
        } else {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
        }
    } else if (path == DEEPIN_DARK_THEME) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
            qDebug() << "dark theme, return";
            return;
        } else {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::DarkType);
        }
    }
    qDebug() << "slot sig theme changed end";
}

void Window::slotSigAdjustFont(QString fontName)
{
    qDebug() << "slot sig adjust font" << fontName;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setFontFamily(fontName);
    }
    qDebug() << "slot sig adjust font end";
}

void Window::slotSigAdjustFontSize(qreal fontSize)
{
    qDebug() << "slot sig adjust font size" << fontSize;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setFontSize(fontSize);
        wrapper->bottomBar()->setScaleLabelText(fontSize);
        wrapper->OnUpdateHighlighter();
    }

    m_fontSize = fontSize;
    qDebug() << "slot sig adjust font size end";
}

void Window::slotSigAdjustTabSpaceNumber(int number)
{
    qDebug() << "slot sig adjust tab space number" << number;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setTabSpaceNumber(number);
    }
    qDebug() << "slot sig adjust tab space number end";
}

void Window::slotSigAdjustWordWrap(bool enable)
{
    qDebug() << "slot sig adjust word wrap" << enable;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        TextEdit *textedit = wrapper->textEditor();
        textedit->setLineWrapMode(enable);
    }
    qDebug() << "slot sig adjust word wrap end";
}

void Window::slotSigSetLineNumberShow(bool bIsShow)
{
    qDebug() << "slot sig set line number show" << bIsShow;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->setLineNumberShow(bIsShow);
    }
    qDebug() << "slot sig set line number show end";
}

void Window::slotSigAdjustBookmark(bool bIsShow)
{
    qDebug() << "slot sig adjust bookmark" << bIsShow;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        TextEdit *textedit = wrapper->textEditor();
        textedit->setBookmarkFlagVisable(bIsShow);
    }
    qDebug() << "slot sig adjust bookmark end";
}

void Window::slotSigShowBlankCharacter(bool bIsShow)
{
    qDebug() << "slot sig show blank character" << bIsShow;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->setShowBlankCharacter(bIsShow);
    }
    qDebug() << "slot sig show blank character end";
}

void Window::slotSigHightLightCurrentLine(bool bIsShow)
{
    qDebug() << "slot sig hight light current line" << bIsShow;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setHighLineCurrentLine(bIsShow);
    }
    qDebug() << "slot sig hight light current line end";
}

void Window::slotSigShowCodeFlodFlag(bool bIsShow)
{
    qDebug() << "slot sig show code flod flag" << bIsShow;
    for (EditWrapper *wrapper : m_wrappers.values()) {
        TextEdit *textedit = wrapper->textEditor();
        textedit->setCodeFlodFlagVisable(bIsShow);
    }
    qDebug() << "slot sig show code flod flag end";
}

void Window::slotSigChangeWindowSize(QString mode)
{
    qDebug() << "slot sig change window size" << mode;
    if (mode == "fullscreen") {
        qDebug() << "slot sig change window size fullscreen";
        this->showFullScreen();
    } else if (mode == "window_maximum") {
        qDebug() << "slot sig change window size window_maximum";
        this->showNormal();
        this->showMaximized();
    } else {
        qDebug() << "slot sig change window size window_minimum";
        this->showNormal();
    }
    qDebug() << "slot sig change window size end";
}

void Window::handleFocusWindowChanged(QWindow *w)
{
    qDebug() << "handle focus window changed";
    if (windowHandle() != w || !currentWrapper() || !isActiveWindow()) {
        qDebug() << "handle focus window changed return";
        return;
    }

    currentWrapper()->checkForReload();
    checkTabbarForReload();
    qDebug() << "handle focus window changed end";
}

void Window::updateThemePanelGeomerty()
{
    qDebug() << "update theme panel geomerty";
    int yOffset = isFullScreen() ? 0 : titlebar()->height();
    QRect themePanelRect(0, yOffset, 250, height() - yOffset);
    themePanelRect.moveRight(rect().right());
    m_themePanel->setGeometry(themePanelRect);
    qDebug() << "update theme panel geomerty end";
}

void Window::checkTabbarForReload()
{
    qDebug() << "check tabbar for reload";
    /* 修复99423 bug暂且屏蔽；拖拽出只读tab文件项，只读字样消失
    int cur = m_tabbar->currentIndex();
    QFileInfo fi(m_tabbar->truePathAt(cur));
    */
    QFileInfo fi;
    if (m_tabbar->currentPath().contains("backup-files")) {
        fi.setFile(m_tabbar->truePathAt(m_tabbar->currentIndex()));
        qDebug() << "check tabbar for reload backup-files" << fi.filePath();
    } else {
        fi.setFile(m_tabbar->currentPath());
        qDebug() << "check tabbar for reload normal" << fi.filePath();
    }

    QString tabName = m_tabbar->currentName();
    QString readOnlyStr = QString(" (%1)").arg(tr("Read-Only"));
    tabName.remove(readOnlyStr);

    EditWrapper *wrapper = m_wrappers.value(m_tabbar->currentPath());
    if (!wrapper || !wrapper->textEditor()) {
        qWarning() << "Failed to get wrapper or text editor in checkTabbarForReload";
        return;
    }

    if (fi.exists() && !fi.isWritable()) {
        qDebug() << "check tabbar for reload backup-files not writable";
        tabName.append(readOnlyStr);
        m_tabbar->setTabText(m_tabbar->currentIndex(), tabName);
        wrapper->textEditor()->setReadOnlyPermission(true);
    } else {
        qDebug() << "check tabbar for reload backup-files writable";
        tabName.remove(readOnlyStr);
        m_tabbar->setTabText(m_tabbar->currentIndex(), tabName);
        wrapper->textEditor()->setReadOnlyPermission(false);
    }

    setWindowTitleInfo();
    //m_tabbar->setTabText(m_tabbar->currentIndex(), tabName);
    //判断是否需要阻塞系统关机
    emit sigJudgeBlockShutdown();
    qDebug() << "check tabbar for reload end";
    StartManager::instance()->delayMallocTrim();
}

void Window::resizeEvent(QResizeEvent *e)
{
    if (m_themePanel->isVisible()) {
        qDebug() << "resize event update theme panel geomerty";
        updateThemePanelGeomerty();
    }

    if (!isMaximized() && !isFullScreen()) {
        qDebug() << "resize event update window size";
        m_settings->settings->option("advance.window.window_width")->setValue(rect().width());
        m_settings->settings->option("advance.window.window_height")->setValue(rect().height());
        if (m_needMoveToCenter) {
            Dtk::Widget::moveToCenter(this);
            m_needMoveToCenter = false;
            qDebug() << "resize event update window size move to center";
        }
    }

    m_findBar->setGeometry(4, height() - m_findBar->height() - 4, width() - 8, m_findBar->height());
    m_replaceBar->setGeometry(4, height() - m_replaceBar->height() - 4, width() - 8, m_replaceBar->height());

    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->OnUpdateHighlighter();
    }

#if 0
    if (!(m_tabbar->currentPath() == "")) {
        EditWrapper *wrapper = m_wrappers.value(m_tabbar->currentPath());
        wrapper->textEditor()->hideRightMenu();
    }
#endif

    DMainWindow::resizeEvent(e);
}

void Window::closeEvent(QCloseEvent *e)
{
    qDebug() << "close event";
    PerformanceMonitor::closeAppStart();

    if (StartManager::instance()->isMultiWindow()) {
        qDebug() << "close event multi window";
        if (!closeAllFiles()) {
            e->ignore();
            qDebug() << "close event ignore";
            return;
        }
    } else {
        // 是否记录上次打开的文件
        qDebug() << "close event single window";
        bool save_tab_before_close = m_settings->settings->option("advance.startup.save_tab_before_close")->value().toBool();
        if (!save_tab_before_close) {
            // 不记录，关闭所有标签页，没保存的文件提示保存
            qDebug() << "close event save tab before close false";
            if (!closeAllFiles()) {
                e->ignore();
                qDebug() << "close event ignore";
                return;
            }
        } else {
            qDebug() << "close event save tab before close true";
            // 记录，没保存的文件不提示保存，除非磁盘上文件已删除
            // 单个窗口时，没有记录单独关闭窗口，需要记录窗口信息。
            for (auto itr = m_wrappers.begin(); itr != m_wrappers.end(); ++itr) {
                QString filePath = itr.value()->textEditor()->getFilePath();
                Utils::recordCloseFile(filePath);
            }
            // 检查是否存在磁盘上已删除的文件，若存在则提示保存
            if (!saveAllFloatingFiles()) {
                backupFile();
                e->ignore();
                return;
            }
            backupFile();
        }
    }

    // WARNING: 调用 QProcess::startDetached() 前同步配置，防止多线程对 qgetenv() 中的 environmentMutex 加锁
    // 导致创建进程 fork() 时保留了 environmentMutex 锁状态，使得父子进程陷入资源竞态，阻塞状态。
    if (m_settings->settings) {
        qDebug() << "close event sync settings";
        m_settings->settings->sync();
    }

    // 退出打印状态
    m_bPrintProcessing = false;

    if (IflytekAiAssistant::instance()->valid()) {
        qDebug() << "close event iflytek ai assistant valid";
        IflytekAiAssistant::instance()->stopTtsDirectly();
    }

    QList<EditWrapper *> needSaveList;
    QMap<QString, EditWrapper *> wrappers = m_wrappers;

    for (EditWrapper *wrapper : wrappers) {
        if (nullptr == wrapper) {
            continue;
        }
        m_wrappers.remove(wrapper->filePath());
        disconnect(wrapper->textEditor());
        wrapper->setQuitFlag();
        wrapper->deleteLater();
    }

    disconnect(m_settings, nullptr, this, nullptr);
    //this->close();


    StartManager::instance()->closeAboutForWindow(this);

    emit closeWindow();

    return DMainWindow::closeEvent(e);
}

void Window::hideEvent(QHideEvent *event)
{
    qDebug() << "hide event";
    if (this->isVisible()) {
        qDebug() << "hide event is visible";
        if (currentWrapper() != nullptr) {
            currentWrapper()->textEditor()->setFocus();
            qDebug() << "hide event set focus";
        }
    }
    //如果查找浮窗正显示着，则隐藏
    if (m_findBar->isVisible()) {
        qDebug() << "hide event hide find bar";
        // m_findBar->hide();
        if (currentWrapper() != nullptr) {
            currentWrapper()->bottomBar()->show();
            qDebug() << "hide event show bottom bar";
        }
    }

    //如果替换浮窗正显示着，则隐藏
#if 0
    if (m_replaceBar->isVisible()) {
        qDebug() << "hide event hide replace bar";
        //m_replaceBar->hide();
        if (currentWrapper() != nullptr) {
            currentWrapper()->m_bottomBar->show();
        }
    }
#endif
    DMainWindow::hideEvent(event);
}

void Window::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeyshortcut(e);
    qDebug() << "key press event" << key;

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "decrementfontsize") ||
            key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "incrementfontsize") ||
            key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "togglefullscreen")) {
        currentWrapper()->textEditor()->setCodeFoldWidgetHide(true);
        qDebug() << "key press event toggle full screen";
    }

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "addblanktab")) {
        qDebug() << "key press event add blank tab";
        addBlankTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "newwindow")) {
        qDebug() << "key press event new window";
        emit newWindow();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "savefile")) {
        qDebug() << "key press event save file";
        saveFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "saveasfile")) {
        qDebug() << "key press event save as file";
        saveAsFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectnexttab")) {
        qDebug() << "key press event select next tab";
        m_tabbar->nextTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectprevtab")) {
        qDebug() << "key press event select previous tab";
        m_tabbar->previousTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "closetab")) {
        qDebug() << "key press event close tab";
        closeTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "restoretab")) {
        qDebug() << "key press event restore tab";
        restoreTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "closeothertabs")) {
        qDebug() << "key press event close other tabs";
        m_tabbar->closeOtherTabs();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "openfile")) {
        qDebug() << "key press event open file";
        openFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "incrementfontsize")) {
        qDebug() << "key press event increment font size";
        incrementFontSize();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "decrementfontsize")) {
        qDebug() << "key press event decrement font size";
        decrementFontSize();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "resetfontsize")) {
        qDebug() << "key press event reset font size";
        resetFontSize();
    }  else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "togglefullscreen")) {
        qDebug() << "key press event toggle full screen";
        DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
        DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
        quitFullBtn->setFocusPolicy(Qt::TabFocus);
        DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
        if (minBtn->hasFocus() || maxBtn->hasFocus()) {
            qDebug() << "min or max button has focus";
            toggleFullscreen();
            quitFullBtn->setFocus();
        } else {
            qDebug() << "quit full screen button has focus";
            toggleFullscreen();
        }
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "find")) {
        qDebug() << "key press event popup find bar";
        popupFindBar();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "findNext")) {
        qDebug() << "key press event handle find next search keyword";
        handleFindNextSearchKeyword(m_keywordForSearch);
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "findPrev")) {
        qDebug() << "key press event handle find prev search keyword";
        handleFindPrevSearchKeyword(m_keywordForSearch);
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "replace")) {
        qDebug() << "key press event popup replace bar";
        popupReplaceBar();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "jumptoline")) {
        qDebug() << "key press event popup jump line bar";
        popupJumpLineBar();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "saveposition")) {
        qDebug() << "key press event save position";
        remberPositionSave();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "restoreposition")) {
        qDebug() << "key press event restore position";
        remberPositionRestore();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "escape")) {
        qDebug() << "key press event press escape";
        emit pressEsc();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "displayshortcuts")) {
        qDebug() << "key press event display shortcuts";
        displayShortcuts();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "print")) {
        qDebug() << "key press event popup print dialog";
        popupPrintDialog();
    } else {
        qDebug() << "key press event post event to window widget";
        // Post event to window widget if match Alt+0 ~ Alt+9
        QRegularExpression re("^Alt\\+\\d");
        QRegularExpressionMatch match = re.match(key);
        if (match.hasMatch()) {
            auto tabIndex = key.replace("Alt+", "").toInt();
            if (tabIndex == 9) {
                qDebug() << "tab index is 9";
                if (m_tabbar->count() > 1) {
                    activeTab(m_tabbar->count() - 1);
                }
            } else {
                qDebug() << "tab index is " << tabIndex;
                if (tabIndex <= m_tabbar->count()) {
                    activeTab(tabIndex - 1);
                }
            }
        }
    }
}

void Window::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if ((keyEvent->modifiers() | Qt::ShiftModifier) || (keyEvent->modifiers() | Qt::ControlModifier)) {
        qDebug() << "key release event ignore";
//        if (nullptr != m_shortcutViewProcess) {
//            int count = Utils::getProcessCountByName("deepin-shortcut-viewer");
//            if (count > 0) {
//                Utils::killProcessByName("deepin-shortcut-viewer");
//            }
//            delete (m_shortcutViewProcess);
//            m_shortcutViewProcess = nullptr;
//        }
    }
}


void Window::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "drag enter event";
    // Accept drag event if mime type is url.
    event->accept();
}

void Window::dropEvent(QDropEvent *event)
{
    qDebug() << "drop event";
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        qDebug() << "drop event has urls";
        QStringList supportfileNames;
        QStringList otherfiles;
        for (auto url : mimeData->urls()) {
            QString file = QFileInfo(url.toLocalFile()).canonicalFilePath();
            if (Utils::isMimeTypeSupport(file)) {
                supportfileNames.append(file);
            } else {
                otherfiles.append(file);
            }
        }

        //先添加支持的文件
        foreach (QString var, supportfileNames) {
            addTab(var, true);
        }

        //后添加不支持文件　在最后编辑页面显示
        foreach (QString var, otherfiles) {
            addTab(var, true);
        }
    }
    qDebug() << "drop event end";
}

/**
 * @brief 处理定时事件 \a e , 主要处理延迟关闭标签事件，会在定时器结束后关闭标签页
 */
void Window::timerEvent(QTimerEvent *e)
{
    qDebug() << "timer event";
    // 处理延迟关闭标签事件
    if (e->timerId() == m_delayCloseTabTimer.timerId()) {
        qDebug() << "timer event delay close tab";
        m_delayCloseTabTimer.stop();

        // 判断当前处理的标签页是否合法
        if (0 <= m_requestCloseTabIndex && m_requestCloseTabIndex < m_tabbar->count()) {
            activeTab(m_requestCloseTabIndex);
            closeTab();
            qDebug() << "timer event close tab";
        }
    }
    qDebug() << "timer event end";
}

bool Window::findBarIsVisiable()
{
    qDebug() << "find bar is visiable";
    if (m_findBar->isVisible()) {
        qDebug() << "find bar is visiable true";
        return true;
    } else {
        qDebug() << "find bar is visiable false";
        return false;
    }
}

bool Window::replaceBarIsVisiable()
{
    qDebug() << "replace bar is visiable";
    return m_replaceBar == nullptr ? false : m_replaceBar->isVisible();
}

QString Window::getKeywordForSearchAll()
{
    qDebug() << "get keyword for search all";
    return m_keywordForSearchAll;
}

QString Window::getKeywordForSearch()
{
    qDebug() << "get keyword for search";
    return m_keywordForSearch;
}

void Window::setPrintEnabled(bool enabled)
{
    qDebug() << "set print enabled";
    for (int i = 0; i < m_menu->actions().count(); i++) {
        if (!m_menu->actions().at(i)->text().compare(QString("Print"))) {
            m_menu->actions().at(i)->setEnabled(enabled);
            qDebug() << "has Print, return";
            return;
        }
    }
}

QStackedWidget *Window::getStackedWgt()
{
    qDebug() << "get stacked wgt";
    return m_editorWidget;
}
