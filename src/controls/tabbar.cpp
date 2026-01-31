// SPDX-FileCopyrightText: 2017 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tabbar.h"
#include "../widgets/window.h"
#include "../common/utils.h"
#include "../editor/editwrapper.h"
#include "../startmanager.h"

#include <QApplication>
#include <QStyleFactory>
#include <QGuiApplication>
#include <DPlatformWindowHandle>
#include <DWindowManagerHelper>
#include <QPixmap>
#include <DFrame>
#include <DGuiApplicationHelper>
#include <QDebug>

QPixmap *Tabbar::sm_pDragPixmap = nullptr;

// 不同模式下的界面调整
const int s_TabbarHeight = 40;
const int s_TabbarHeightCompact = 26;

/**
 * @brief ‘&’在Qt中被标记为助记符，替换 \a str 中的‘&’字符为“&&”，以正确显示文件名中的‘&’符号
 *      painter 绘制 CE_ToolButtonLabel 时设置了 Qt::TextShowMnemonic
 * @note 在插入、新建标签页时，内部已使用替换，部分外部更新标签页名称，需手动处理
 * @sa Tabbar::addTabWithIndex(), Tabbar::insertFromMimeData(), Tabbar::setTabText()
 */
QString replaceMnemonic(const QString &str)
{
    qDebug() << "Enter replaceMnemonic, str:" << str;
    QString tmp = str;
    tmp.replace(QChar('&'), QString("&&"));
    qDebug() << "Exit replaceMnemonic, tmp:" << tmp;
    return tmp;
}

/**
 * @brief ‘&’在Qt中被标记为助记符，替换 \a str 中的"&&字符为'&'，以正确取得文件名中的‘&’符号
 */
QString restoreMnemonic(const QString &str)
{
    qDebug() << "Enter restoreMnemonic, str:" << str;
    QString tmp = str;
    tmp.replace(QString("&&"), QChar('&'));
    qDebug() << "Exit restoreMnemonic, tmp:" << tmp;
    return tmp;
}

Tabbar::Tabbar(QWidget *parent)
    : DTabBar(parent)
{
    qDebug() << "Tabbar constructor start";
    m_rightClickTab = -1;

    installEventFilter(this);

    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);
    setAcceptDrops(true);
    // setStartDragDistance(40);
    setElideMode(Qt::ElideRight);
    setTabPalette(palette().buttonText().color().name(), palette().highlightedText().color().name());
    setFocusPolicy(Qt::NoFocus);
    qDebug() << "Tabbar initialized with movable and closable tabs";
    connect(this, &DTabBar::dragStarted, this, &Tabbar::onTabDrapStart);
    connect(this, &DTabBar::tabMoved, this, &Tabbar::handleTabMoved);
    connect(this, &DTabBar::tabDroped, this, &Tabbar::handleTabDroped);
    connect(this, &DTabBar::tabIsRemoved, this, &Tabbar::handleTabIsRemoved);
    connect(this, &DTabBar::tabReleaseRequested, this, &Tabbar::handleTabReleased);
    connect(this, &DTabBar::dragActionChanged, this, &Tabbar::handleDragActionChanged);

#ifdef DTKWIDGET_CLASS_DSizeMode
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [ this ](){
        // 更新当前控件，自动调用 sizeHint() 更新界面参数
        this->update();
    });
#endif
    qDebug() << "Tabbar constructor end";
}

Tabbar::~Tabbar()
{
    if (m_moreWaysCloseMenu != nullptr) {
        delete m_moreWaysCloseMenu;
        m_moreWaysCloseMenu = nullptr;
    }
    if (m_rightMenu != nullptr) {
        delete m_rightMenu;
        m_rightMenu = nullptr;
    }
}

void Tabbar::addTab(const QString &filePath, const QString &tabName, const QString &tipPath)
{
    qDebug() << "Enter addTab, filePath:" << filePath << "tabName:" << tabName;
    qDebug() << "Adding tab for file:" << QFileInfo(filePath).fileName()
                  << "with name:" << tabName;
    qDebug() << "Exit addTab";
    addTabWithIndex(count(), filePath, tabName, tipPath);
}

void Tabbar::addTabWithIndex(int index, const QString &filePath, const QString &tabName, const QString &tipPath)
{
    qDebug() << "Enter addTabWithIndex, index:" << index << "filePath:" << filePath << "tabName:" << tabName;
    // FIXME(rekols): do not insert duplicate values.
    if (m_tabPaths.contains(filePath)) {
        qDebug() << "m_tabPaths contains filePath";
        return;
    }
    qDebug() << "m_tabPaths does not contain filePath";
    m_tabPaths.insert(index, filePath);
    m_tabTruePaths.insert(index, tipPath);
    qDebug() << "Exit addTabWithIndex";
    // }

    // 除去空白符 梁卫东 ２０２０－０８－２６　１４：４９：１５ ；适配助记符
    QString trimmedName = replaceMnemonic(tabName.simplified());
    qDebug() << "trimmedName:" << trimmedName;
    DTabBar::insertTab(index, trimmedName);
    DTabBar::setCurrentIndex(index);
    qDebug() << "filePath.contains(Utils::localDataPath())";
    if (filePath.contains(Utils::localDataPath())) {
        if (Utils::isBackupFile(filePath) && !tipPath.isNull() && tipPath.length() > 0) {
            setTabToolTip(index, tipPath);
            qDebug() << "tipPath is not null";
        } else {
            setTabToolTip(index, tabName);
            qDebug() << "tipPath is null";
        }
    } else {
        QString path = filePath;
        QFontMetrics fontMetrics(font());
        int nFontWidth = fontMetrics.horizontalAdvance(path) * (qApp->devicePixelRatio() == 1.25 ? 2 : 1);
        qDebug() << "nFontWidth:" << nFontWidth;
        Window *pWindow = static_cast<Window *>(this->window());
        int w = pWindow->width() - 200;
        if (w < 800) w = 800;

        if (nFontWidth >= w) {
            int mod = nFontWidth % w;
            qDebug() << "mod:" << mod;
            int step = nFontWidth / w + (mod > 0 ? 1 : 0);
            qDebug() << "step:" << step;
            for (int i = 1; i < step; i++) {
                path.insert(i * (path.length() / step), '\n');
            }
        }
        qDebug() << "path:" << path;
        setTabToolTip(index, path);
    }
}

void Tabbar::resizeEvent(QResizeEvent *event)
{
    qDebug() << "Enter resizeEvent";
    int cnt = count();
    qDebug() << "cnt:" << cnt;
    for (int i = 0; i < cnt ; i++) {
        QString path = tabToolTip(i);
        path = path.replace("\n", "");
        QFontMetrics fontMetrics(font());
        int nFontWidth = fontMetrics.horizontalAdvance(path) * (qApp->devicePixelRatio() == 1.25 ? 2 : 1);
        qDebug() << "nFontWidth:" << nFontWidth;
        Window *pWindow = static_cast<Window *>(this->window());
        int w = pWindow->width() - 200;
        if (w < 800) w = 800;
        qDebug() << "w:" << w;
        if (nFontWidth >= w) {
            int mod = nFontWidth % w;
            qDebug() << "mod:" << mod;
            int step = nFontWidth / w + (mod > 0 ? 1 : 0);
            qDebug() << "step:" << step;
            for (int j = 1; j < step; j++) {
                path.insert(j * (path.length() / step), '\n');
            }
        }

        setTabToolTip(i, path);
    }

    // 临时修改方案：通过调用setIconSize()，更新内部的layoutDirty标识，强制重新刷新标签页布局, BUG 137607
    // TODO: 需要dtk暴露接口重新布局
    setIconSize(iconSize());

    DTabBar::resizeEvent(event);
    qDebug() << "Exit resizeEvent";
}

void Tabbar::closeTab(int index)
{
    qDebug() << "Enter closeTab, index:" << index;
    if (index < 0) {
        qWarning() << "Attempt to close tab with invalid index:" << index;
        return;
    }
    qDebug() << "index >= 0";
    QString file = fileAt(index);
    qDebug() << "Closing tab index:" << index
                  << "file:" << QFileInfo(file).fileName();
    emit requestHistorySaved(file);
    DTabBar::removeTab(index);
    qDebug() << "Exit closeTab";
}

void Tabbar::closeCurrentTab()
{
    qDebug() << "Enter closeCurrentTab";
    closeTab(currentIndex());
    qDebug() << "Exit closeCurrentTab";
}

void Tabbar::closeCurrentTab(const QString &strFilePath)
{
    qDebug() << "Enter closeCurrentTab, strFilePath:" << strFilePath;
    closeTab(this->indexOf(strFilePath));
    qDebug() << "Exit closeCurrentTab";
}

void Tabbar::closeOtherTabs()
{
    qDebug() << "Enter closeOtherTabs";
    closeOtherTabsExceptFile(currentPath());
    qDebug() << "Exit closeOtherTabs";
}

void Tabbar::closeOtherTabsExceptFile(const QString &filePath)
{
    qDebug() << "Enter closeOtherTabsExceptFile, filePath:" << filePath;
    QStringList closePathList;

    for (const QString &path : m_tabPaths) {
        if (filePath != path) {
            qDebug() << "filePath != path";
            closePathList << path;
        }
    }

    emit closeTabs(closePathList);
    qDebug() << "Exit closeOtherTabsExceptFile";
}

void Tabbar::closeLeftTabs(const QString &filePath)
{
    qDebug() << "Enter closeLeftTabs, filePath:" << filePath;
    QStringList closePathList;

    for (const QString &path : m_tabPaths) {
        if (filePath == path) {
            qDebug() << "before break filePath == path";
            break;
        }
        closePathList << path;
    }
    qDebug() << "closePathList:" << closePathList;
    emit closeTabs(closePathList);
    qDebug() << "Exit closeLeftTabs";
}

void Tabbar::closeRightTabs(const QString &filePath)
{
    qDebug() << "Enter closeRightTabs, filePath:" << filePath;
    QStringList closePathlist;


    for (int i = m_tabPaths.count() - 1; i >= 0; i--) {
        m_tabPaths.value(i);

        if (filePath == m_tabPaths.value(i)) {
            qDebug() << "before break filePath == m_tabPaths.value(i)";
            break;
        }
        closePathlist << m_tabPaths.value(i);
    }
    qDebug() << "closePathlist:" << closePathlist;
    emit closeTabs(closePathlist);
    qDebug() << "Exit closeRightTabs";
}

void Tabbar::updateTab(int index, const QString &filePath, const QString &tabName)
{
    qDebug() << "Updating tab index:" << index
                   << "file:" << QFileInfo(filePath).fileName()
                   << "new name:" << tabName;
    // 适配助记符 '&' 后设置文本
    setTabText(index, tabName);
    m_tabPaths[index] = filePath;
    m_tabTruePaths[index] = filePath;

    //show file path at tab,blank file only show it's name.
    if (filePath.contains(Utils::localDataPath())) {
        qDebug() << "filePath contains Utils::localDataPath()";
        setTabToolTip(index, tabName);
    } else {
        QString path = filePath;
        QFontMetrics fontMetrics(font());
        int nFontWidth = fontMetrics.horizontalAdvance(path) * (qApp->devicePixelRatio() == 1.25 ? 2 : 1);
        qDebug() << "nFontWidth:" << nFontWidth;
        Window *pWindow = static_cast<Window *>(this->window());
        int w = pWindow->width() - 200;
        if (w < 800) w = 800;
        qDebug() << "w:" << w;
        if (nFontWidth >= w) {
            int mod = nFontWidth % w;
            qDebug() << "mod:" << mod;
            int step = nFontWidth / w + (mod > 0 ? 1 : 0);
            qDebug() << "step:" << step;
            for (int i = 1; i < step; i++) {
                path.insert(i * (path.length() / step), '\n');
            }
        }
        qDebug() << "path:" << path;
        setTabToolTip(index, path);
    }
    qDebug() << "Exit updateTab";
}

void Tabbar::previousTab()
{
    qDebug() << "Enter previousTab";
    int currentIndex = DTabBar::currentIndex();
    qDebug() << "currentIndex:" << currentIndex;
    if (currentIndex <= 0) {
        DTabBar::setCurrentIndex(DTabBar::count() - 1);
    } else {
        DTabBar::setCurrentIndex(currentIndex - 1);
    }
    qDebug() << "Exit previousTab";
}

void Tabbar::nextTab()
{
    qDebug() << "Enter nextTab";
    int currentIndex = DTabBar::currentIndex();
    qDebug() << "currentIndex:" << currentIndex;
    if (currentIndex >= DTabBar::count() - 1) {
        DTabBar::setCurrentIndex(0);
    } else {
        DTabBar::setCurrentIndex(currentIndex + 1);
    }
    qDebug() << "Exit nextTab";
}

int Tabbar::indexOf(const QString &filePath)
{
    qDebug() << "Enter indexOf, filePath:" << filePath;
    return m_tabPaths.indexOf(filePath);
}

QString Tabbar::currentName() const
{
    qDebug() << "Enter currentName";
    return textAt(currentIndex());
}

QString Tabbar::currentPath() const
{
    qDebug() << "Enter currentPath";
    return m_tabPaths.value(currentIndex());
}

QString Tabbar::truePathAt(int index) const
{
    qDebug() << "Enter truePathAt, index:" << index;
    return m_tabTruePaths.value(index);
}

QString Tabbar::fileAt(int index) const
{
    qDebug() << "Enter fileAt, index:" << index;
    return m_tabPaths.value(index);
}

QString Tabbar::textAt(int index) const
{
    // 获取显示文本时恢复设置的助记符 '&'
    qDebug() << "Enter textAt, index:" << index;
    return restoreMnemonic(DTabBar::tabText(index));
}

/**
 * @brief 设置索引 \a index 指向标签页显示文本为 \a text
 */
void Tabbar::setTabText(int index, const QString &text)
{
    qDebug() << "Enter setTabText, index:" << index << "text:" << text;
    // 替换助记符
    QString tmp = replaceMnemonic(text);
    qDebug() << "tmp:" << tmp;
    DTabBar::setTabText(index, tmp);
    qDebug() << "Exit setTabText";
}

void Tabbar::setTabPalette(const QString &activeColor, const QString &highlightColor)
{
    qDebug() << "Enter setTabPalette, activeColor:" << activeColor << "highlightColor:" << highlightColor;
    // Not recommend manually setPalette()
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Q_UNUSED(activeColor)
    Q_UNUSED(highlightColor)
#else
    QPalette pa = this->palette();
    pa.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(highlightColor));
    pa.setColor(QPalette::Inactive, QPalette::WindowText, QColor(activeColor));
    pa.setColor(QPalette::Active, QPalette::WindowText, QColor(activeColor));
    setPalette(pa);
#endif
    qDebug() << "Exit setTabPalette";
}

void Tabbar::setBackground(const QString &startColor, const QString &endColor)
{
    qDebug() << "Enter setBackground, startColor:" << startColor << "endColor:" << endColor;
    m_backgroundStartColor = startColor;
    m_backgroundEndColor = endColor;
    qDebug() << "Exit setBackground";
}

void Tabbar::setDNDColor(const QString &startColor, const QString &endColor)
{
    qDebug() << "Enter setDNDColor, startColor:" << startColor << "endColor:" << endColor;
    m_dndStartColor = startColor;
    m_dndEndColor = endColor;
    qDebug() << "Exit setDNDColor";
}

QPixmap Tabbar::createDragPixmapFromTab(int index, const QStyleOptionTab &option, QPoint *hotspot) const
{
    qDebug() << "Enter createDragPixmapFromTab, index:" << index;

    Window *window = static_cast<Window *>(this->window());
    EditWrapper *wrapper = window->wrapper(fileAt(index));
    //加载大文本不允许拖拽
    //if(wrapper && wrapper->getFileLoading()) return QPixmap();

    TextEdit *textEdit = wrapper->textEditor();

    int width = textEdit->width();
    int height = textEdit->height();
    QImage screenshotImage(width, height, QImage::Format_ARGB32_Premultiplied);
    textEdit->render(&screenshotImage, QPoint(), QRegion(0, 0, width, height));

    // scaled image to smaller.
    int scaledWidth = width / 5;
    int scaledHeight = height / 5;
    auto scaledImage = screenshotImage.scaled(scaledWidth, scaledHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QImage backgroundImage(scaledWidth + 10, scaledHeight + 10, QImage::Format_ARGB32_Premultiplied);
    backgroundImage.fill(QColor(palette().color(QPalette::Base)));
    // clip screenshot image with window radius.
    QPainter painter(&backgroundImage);
    painter.drawImage(5, 5, scaledImage);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!Utils::isWayland() && count() == 1) {
        qDebug() << "Utils::isWayland() && count() == 1";
        this->window()->showMinimized();
    }

    // adjust offset.
    hotspot->setX(scaledWidth / 2);
    hotspot->setY(scaledHeight / 2);

    QPainterPath rectPath;

    if (DWindowManagerHelper::instance()->hasComposite()) {
        QPainterPath roundedRectPath;

        rectPath.addRect(0, 0, scaledWidth + 10, scaledHeight + 10);
        roundedRectPath.addRoundedRect(QRect(0, 0, scaledWidth + 10, scaledHeight + 10), 6, 6);

        rectPath -= roundedRectPath;

        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillPath(rectPath, Qt::transparent);

        QColor shadowColor = QColor(palette().color(QPalette::BrightText));
        shadowColor.setAlpha(80);

        painter.end();
        qDebug() << "painter.end()";
        if (sm_pDragPixmap) delete sm_pDragPixmap;
        sm_pDragPixmap = new QPixmap(Utils::dropShadow(QPixmap::fromImage(backgroundImage), 5, shadowColor, QPoint(0, 0)));
        return Utils::dropShadow(QPixmap::fromImage(backgroundImage), 5, shadowColor, QPoint(0, 0));
    } else {
        painter.end();
        if (sm_pDragPixmap) delete sm_pDragPixmap;
        sm_pDragPixmap = new QPixmap(QPixmap::fromImage(backgroundImage));
        return QPixmap::fromImage(backgroundImage);
    }
}

QMimeData *Tabbar::createMimeDataFromTab(int index, const QStyleOptionTab &option) const
{
    qDebug() << "Enter createMimeDataFromTab, index:" << index;
    const QString tabName = textAt(index);
    qDebug() << "tabName:" << tabName;
    Window *window = static_cast<Window *>(this->window());
    EditWrapper *wrapper = window->wrapper(fileAt(index));

    if (wrapper && wrapper->getFileLoading()) return nullptr;

    QMimeData *mimeData = new QMimeData;
    mimeData->setParent(window);

    if (!wrapper) {
        //m_tabbar->closeCurrentTab();
        qDebug() << "wrapper is null";
        return nullptr;
    }

    mimeData->setProperty("wrapper", QVariant::fromValue(static_cast<void *>(wrapper)));
    mimeData->setProperty("isModified", wrapper->isModified());
    mimeData->setData("dedit/tabbar", tabName.toUtf8());
    mimeData->removeFormat("text/plain");

    return mimeData;
}

void Tabbar::insertFromMimeDataOnDragEnter(int index, const QMimeData *source)
{
    qDebug() << "Enter insertFromMimeDataOnDragEnter, index:" << index;
    if (source == nullptr) {
        return;
    }
    qDebug() << "source is not null";
    const QString tabName = QString::fromUtf8(source->data("dedit/tabbar"));
    QVariant pVar = source->property("wrapper");
    EditWrapper *wrapper = static_cast<EditWrapper *>(pVar.value<void *>());

    //大文本加载不允许拖拽
    if (wrapper && (wrapper && wrapper->getFileLoading())) return;

    Window *window = static_cast<Window *>(this->window());

    if (!wrapper) {
        qDebug() << "wrapper is null";
        return;
    }

    window->addTabWithWrapper(wrapper, wrapper->textEditor()->getFilePath(), wrapper->textEditor()->getTruePath(), tabName, index);
    //window->currentWrapper()->textEditor()->setModified(source->property("isModified").toBool());
    wrapper->updateModifyStatus(source->property("isModified").toBool());
    wrapper->OnUpdateHighlighter();
    window->focusActiveEditor();
    qDebug() << "Exit insertFromMimeDataOnDragEnter";
}

void Tabbar::insertFromMimeData(int index, const QMimeData *source)
{
    qDebug() << "Enter insertFromMimeData, index:" << index;
    if (source == nullptr) {
        qDebug() << "source is null";
        return;
    }
    qDebug() << "source is not null";
    const QString tabName = QString::fromUtf8(source->data("dedit/tabbar"));
    QVariant pVar = source->property("wrapper");
    EditWrapper *wrapper = static_cast<EditWrapper *>(pVar.value<void *>());
    Window *window = static_cast<Window *>(this->window());

    if (!wrapper) {
        qDebug() << "wrapper is null";
        return;
    }

    //qDebug() << "insertFromMimeData";
    window->addTabWithWrapper(wrapper, wrapper->textEditor()->getFilePath(), wrapper->textEditor()->getTruePath(), tabName, index);
    //window->currentWrapper()->textEditor()->setModified(source->property("isModified").toBool());
    wrapper->updateModifyStatus(source->property("isModified").toBool());
    wrapper->OnUpdateHighlighter();
    window->focusActiveEditor();
    qDebug() << "Exit insertFromMimeData";
}

bool Tabbar::canInsertFromMimeData(int index, const QMimeData *source) const
{
    qDebug() << "Enter canInsertFromMimeData, index:" << index;
    return source->hasFormat("dedit/tabbar");
}

void Tabbar::handleDragActionChanged(Qt::DropAction action)
{
    qDebug() << "Enter handleDragActionChanged";
    // Reset cursor to Qt::ArrowCursor if drag tab to TextEditor widget.
    if (action == Qt::IgnoreAction) {
        qDebug() << "action == Qt::IgnoreAction";
        if (dragIconWindow()) {
            QGuiApplication::changeOverrideCursor(Qt::ArrowCursor);
            DPlatformWindowHandle::setDisableWindowOverrideCursor(dragIconWindow(), true);
        }
    } else if (dragIconWindow()) {
        qDebug() << "dragIconWindow()";
        DPlatformWindowHandle::setDisableWindowOverrideCursor(dragIconWindow(), false);
        if (QGuiApplication::overrideCursor())
            QGuiApplication::changeOverrideCursor(QGuiApplication::overrideCursor()->shape());
    }
    qDebug() << "Exit handleDragActionChanged";
}

bool Tabbar::eventFilter(QObject *, QEvent *event)
{
    qDebug() << "Enter eventFilter";
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent->button() == Qt::RightButton) {
            QPoint position = mouseEvent->pos();
            m_rightClickTab = this->tabAt(position);
            int indexCount = this->count();

//            m_rightClickTab = -1;

//            for (int i = 0; i < count(); i++) {
//                if (tabRect(i).contains(position)) {
//                    m_rightClickTab = i;
//                    break;
//                }
//            }

            // popup right menu on tab.
            if (m_rightClickTab >= 0) {
                m_rightMenu = new DMenu;
                //m_rightMenu->setStyle(QStyleFactory::create("dlight"));

                m_closeTabAction = new QAction(tr("Close tab"), this);
                m_closeOtherTabAction = new QAction(tr("Close other tabs"), this);
                m_moreWaysCloseMenu = new DMenu(tr("More options"), this);
                m_closeLeftTabAction = new QAction(tr("Close tabs to the left"), this);
                m_closeRightTabAction = new QAction(tr("Close tabs to the right"), this);
                m_closeAllunModifiedTabAction = new QAction(tr("Close unmodified tabs"), this);

                m_moreWaysCloseMenu->addAction(m_closeLeftTabAction);
                m_moreWaysCloseMenu->addAction(m_closeRightTabAction);
                m_moreWaysCloseMenu->addAction(m_closeAllunModifiedTabAction);

                if (m_tabPaths.length() < 2) {
                    qDebug() << "m_tabPaths.length() < 2";
                    m_closeOtherTabAction->setEnabled(false);
                    m_closeLeftTabAction->setEnabled(false);
                    m_closeRightTabAction->setEnabled(false);
                }

                //优化tab菜单显示　梁卫东
                if (m_rightClickTab == 0 && indexCount == 1) {
                    qDebug() << "m_rightClickTab == 0 && indexCount == 1";
                    m_closeLeftTabAction->setEnabled(false);
                    m_closeRightTabAction->setEnabled(false);
                } else if (m_rightClickTab == 0 && indexCount > 1) {
                    qDebug() << "m_rightClickTab == 0 && indexCount > 1";
                    m_closeLeftTabAction->setEnabled(false);
                    m_closeRightTabAction->setEnabled(true);
                } else if (m_rightClickTab == indexCount - 1 && indexCount == 1) {
                    qDebug() << "m_rightClickTab == indexCount - 1 && indexCount == 1";
                    m_closeLeftTabAction->setEnabled(false);
                    m_closeRightTabAction->setEnabled(false);
                } else if (m_rightClickTab == indexCount - 1 && indexCount > 1) {
                    qDebug() << "m_rightClickTab == indexCount - 1 && indexCount > 1";
                    m_closeLeftTabAction->setEnabled(true);
                    m_closeRightTabAction->setEnabled(false);
                } else {
                    qDebug() << "else";
                    m_closeLeftTabAction->setEnabled(true);
                    m_closeRightTabAction->setEnabled(true);
                }

                //showTabs();
                qDebug() << "connect(m_closeTabAction, &QAction::triggered, this, [ = ]";
                connect(m_closeTabAction, &QAction::triggered, this, [ = ] {
                    qDebug() << "m_closeTabAction triggered";
                    Q_EMIT tabCloseRequested(m_rightClickTab);
                });
                qDebug() << "connect(m_closeOtherTabAction, &QAction::triggered, this, [ = ]";
                connect(m_closeOtherTabAction, &QAction::triggered, this, [ = ] {
                    qDebug() << "m_closeOtherTabAction triggered";
                    closeOtherTabsExceptFile(fileAt(m_rightClickTab));
                });

                connect(m_closeLeftTabAction, &QAction::triggered, this, [ = ] {
                    closeLeftTabs(fileAt(m_rightClickTab));
//                    int currentIndex = DTabBar::currentIndex();
//                    if(currentIndex>=0) {
//                        for (int i=0;i<currentIndex;i++) {
//                            closeTab(0);
//                        }
//                    }
                });

                connect(m_closeRightTabAction, &QAction::triggered, this, [ = ] {
                    closeRightTabs(fileAt(m_rightClickTab));
//                    int currentIndex = DTabBar::currentIndex();
//                    int count = DTabBar::count();
//                    if(currentIndex>=0){
//                        for (int i=count;i>currentIndex;i--) {
//                            closeTab(currentIndex+1);
//                        }
//                    }
                });

                connect(m_closeAllunModifiedTabAction, &QAction::triggered, this, [ = ] {
                    Window *window = static_cast<Window *>(this->window());//确定在哪个窗口关闭
//                    for (int i=0;i<m_tabPaths.count();i++) {
//                        EditWrapper *wrapper = window->wrapper(m_tabPaths.value(i));
//                        if (!wrapper->textEditor()->document()->isModified()){
//                            closeTab(this->indexOf(m_tabPaths.value(i)));
//                        }
//                    }
                    for (auto path : m_tabPaths)
                    {
                        EditWrapper *wrapper = window->wrapper(path);//路径获取文件
                        if (!wrapper->isModified()) {
                            window->removeWrapper(path, true);
                            closeTab(this->indexOf(path));
                        }
                    }

                });

                m_rightMenu->addAction(m_closeTabAction);
                m_rightMenu->addAction(m_closeOtherTabAction);
                m_rightMenu->addMenu(m_moreWaysCloseMenu);
                //yanyuhan 只有一个标签页时不显示更多方式关闭
                if (m_tabPaths.size() > 1) {
                    qDebug() << "m_tabPaths.size() > 1";
                    m_moreWaysCloseMenu->setEnabled(true);
                } else {
                    qDebug() << "m_tabPaths.size() <= 1";
                    m_moreWaysCloseMenu->setEnabled(false);
                }

                m_rightMenu->exec(mapToGlobal(position));
                qDebug() << "m_rightMenu->exec(mapToGlobal(position))";
                return true;
            }
        }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        if (mouseEvent->button() == Qt::MidButton) {
#else
        if (mouseEvent->button() == Qt::MiddleButton) {
#endif
            emit tabCloseRequested(tabAt(QPoint(mouseEvent->x(), mouseEvent->y())));
            return true;
        }
    } else if (event->type() == QEvent::DragEnter) {
        qDebug() << "event->type() == QEvent::DragEnter";
//        if ((!e->source() || e->source()->parent() != this) &&
//            mimeData->hasFormat("dedit/tabbar")) {
//            static_cast<Window*>(this->window())->changeTitlebarBackground(m_dndStartColor, m_dndEndColor);
//        }
    } else if (event->type() == QEvent::DragLeave) {
        qDebug() << "event->type() == QEvent::DragLeave";
    } else if (event->type() == QEvent::Drop) {
        qDebug() << "event->type() == QEvent::Drop";
    } else if (event->type() == QEvent::DragMove) {
        qDebug() << "event->type() == QEvent::DragMove";
        event->accept();
    }
    qDebug() << "Exit eventFilter";
    return false;
}

void Tabbar::mousePressEvent(QMouseEvent *e)
{
    qDebug() << "Enter mousePressEvent";
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    bool midClick = (e->button() == Qt::MidButton);
#else
    bool midClick = (e->button() == Qt::MiddleButton);
#endif
    if (midClick) {
        qDebug() << "midClick";
        emit tabCloseRequested(tabAt(QPoint(e->x(), e->y())));
    }
    qDebug() << "Exit mousePressEvent";
    DTabBar::mousePressEvent(e);
}

void Tabbar::dropEvent(QDropEvent *e)
{
    qDebug() << "Enter dropEvent";
    if (e->dropAction() == Qt::CopyAction && e->mimeData()->hasFormat("dedit/tabbar")) {
        if (sm_pDragPixmap) {
            qDebug() << "sm_pDragPixmap is not null";
            QPoint cursorPos = QCursor::pos() - QPoint(sm_pDragPixmap->width() / 2, 20);
            DLabel *pLabel = new DLabel();
            pLabel->setWindowFlags(Qt::FramelessWindowHint);
            pLabel->move(cursorPos);
            pLabel->setPixmap(*sm_pDragPixmap);
            pLabel->setMaximumSize(sm_pDragPixmap->size());
            pLabel->show();

            QRect startRect = QRect(cursorPos, sm_pDragPixmap->size());
            QRect endRect =   QRect(QCursor::pos(), QSize(0, 0));
            QPropertyAnimation *geometry = new QPropertyAnimation(pLabel, "geometry");
            connect(geometry, &QPropertyAnimation::finished, pLabel, &DLabel::deleteLater);
            connect(geometry, &QPropertyAnimation::finished, geometry, &QPropertyAnimation::deleteLater);
            geometry->setDuration(100);
            geometry->setStartValue(startRect);
            geometry->setEndValue(endRect);
            geometry->setEasingCurve(QEasingCurve::InCubic);
            geometry->start();
        }
    }

    DTabBar::dropEvent(e);
    qDebug() << "Exit dropEvent";
}

QSize Tabbar::tabSizeHint(int index) const
{
    qDebug() << "Enter tabSizeHint, index:" << index;
    if (index >= 0) {
        int total = this->width();
        qDebug() << "total:" << total;
        //计算每个tab平均宽度 返回　100到160
        int aveargeWidth = 160;
        aveargeWidth = total / DTabBar::count();
        qDebug() << "aveargeWidth:" << aveargeWidth;
        if (aveargeWidth >= 160) {
            qDebug() << "aveargeWidth >= 160";
            aveargeWidth = 160;
        } else if (aveargeWidth <= 110) {
            qDebug() << "aveargeWidth <= 110";
            aveargeWidth = 110;
        }
        qDebug() << "aveargeWidth:" << aveargeWidth;
#ifdef DTKWIDGET_CLASS_DSizeMode
        return QSize(aveargeWidth, DGuiApplicationHelper::isCompactMode() ? s_TabbarHeightCompact : s_TabbarHeight);
#else
        return QSize(aveargeWidth, 40);
#endif
    }
    qDebug() << "Exit tabSizeHint";
    return DTabBar::tabSizeHint(index);
}

QSize Tabbar::minimumTabSizeHint(int index) const
{
    qDebug() << "Enter minimumTabSizeHint, index:" << index;
    Q_UNUSED(index)
#ifdef DTKWIDGET_CLASS_DSizeMode
    return QSize(110, DGuiApplicationHelper::isCompactMode() ? s_TabbarHeightCompact : s_TabbarHeight);
#else
    return QSize(110, 40);
#endif
    qDebug() << "Exit minimumTabSizeHint";
}

QSize Tabbar::maximumTabSizeHint(int index) const
{
    qDebug() << "Enter maximumTabSizeHint, index:" << index;
    Q_UNUSED(index)
#ifdef DTKWIDGET_CLASS_DSizeMode
    return QSize(160, DGuiApplicationHelper::isCompactMode() ? s_TabbarHeightCompact : s_TabbarHeight);
#else
    return QSize(160, 40);
#endif
    qDebug() << "Exit maximumTabSizeHint";
}

void Tabbar::handleTabMoved(int fromIndex, int toIndex)
{
    qDebug() << "Enter handleTabMoved, fromIndex:" << fromIndex << "toIndex:" << toIndex;
    //qDebug () << "handleTabMoved";
    if (m_tabPaths.count() > fromIndex && m_tabPaths.count() > toIndex && fromIndex >= 0 && toIndex >= 0) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        m_tabPaths.swap(fromIndex, toIndex);
        m_tabTruePaths.swap(fromIndex, toIndex);
#else
        m_tabPaths.swapItemsAt(fromIndex, toIndex);
        m_tabTruePaths.swapItemsAt(fromIndex, toIndex);
#endif
    }
    qDebug() << "Exit handleTabMoved";
}

void Tabbar::showTabs()
{
    qDebug() << "Enter showTabs";
    int currentIndex  =  DTabBar::currentIndex();
    if (currentIndex <= 0)  {
        m_closeLeftTabAction->setEnabled(false);
    }
    if (currentIndex >= DTabBar::count() - 1) {
        m_closeRightTabAction->setEnabled(false);
    }
    qDebug() << "Exit showTabs";
}

void Tabbar::handleTabReleased(int index)
{
    qDebug() << "Enter handleTabReleased, index:" << index;
    if (index == -1) index = 0;
    QString path = m_listOldTabPath.value(index);
    if (path.isEmpty()) {
        qDebug() << "path is empty";
        return;
    }
    qDebug() << "path:" << path;
    int newIndex = m_tabPaths.indexOf(path);
    const QString tabPath = fileAt(newIndex);
    const QString tabName = textAt(newIndex);

    Window *window = static_cast<Window *>(this->window());
    EditWrapper *wrapper = window->wrapper(tabPath);
    if (!wrapper) {
        qDebug() << "wrapper is null";
        return;
    }

    StartManager::instance()->createWindowFromWrapper(tabName, tabPath, wrapper->textEditor()->getTruePath(), wrapper, wrapper->isModified());

    closeTab(newIndex);
    // remove wrapper from window, not delete.
    window->removeWrapper(tabPath, false);
    qDebug() << "Exit handleTabReleased";
}

void Tabbar::handleTabIsRemoved(int index)
{
    qDebug() << "Enter handleTabIsRemoved, index:" << index;
    const QString filePath = m_tabPaths.at(index);
    Window *window = static_cast<Window *>(this->window());
    m_tabPaths.removeAt(index);
    m_tabTruePaths.removeAt(index);
    window->removeWrapper(filePath, false);
    qDebug() << "Exit handleTabIsRemoved";
}

void Tabbar::handleTabDroped(int index, Qt::DropAction action, QObject *target)
{
    qDebug() << "Enter handleTabDroped, index:" << index << "action:" << action << "target:" << target;
    Tabbar *tabbar = qobject_cast<Tabbar *>(target);
    if (tabbar == nullptr) {
        // tab页拖动到外部应用如网页电子表格或wps电子表格时,
        // DTabBar::dragActionChanged 信号收到的DropType为MoveAction,
        // 这种情况下DTabBar内容不能发出DTabBar::tabReleaseRequested来重新构建编辑窗口
        // 因此只能再次添加判断，若目标文本编辑窗口的TabBar未创建，则重新重建文本编辑窗口
        handleTabReleased(index);
        qDebug() << "Exit handleTabDroped";
    } else {
//        QString path = m_listOldTabPath.value(index);
//        int newIndex = m_tabPaths.indexOf(path);
        qDebug() << "closeTab(index)";
        closeTab(index);
//        StartManager::instance()->setDragEnter(false);
    }
    qDebug() << "Exit handleTabDroped";
}

void Tabbar::onTabDrapStart()
{
    qDebug() << "Tab drag operation started";
    Window *window = static_cast<Window *>(this->window());
    window->setChildrenFocus(false);
    m_listOldTabPath = m_tabPaths;
    qDebug() << "Saved" << m_listOldTabPath.size() << "tabs for drag operation";
}
