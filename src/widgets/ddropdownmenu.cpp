// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../common/utils.h"
#include "../common/settings.h"
#include "ddropdownmenu.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPaintEvent>
#include <DApplication>
#include <QPainter>
#include <DSettingsOption>
#include <QDebug>
#include <DFontSizeManager>
#include <DLabel>
#include <DGuiApplicationHelper>
#include <DPaletteHelper>
#include <QtSvg/QSvgRenderer>
#include <QActionGroup>
#include <QFile>
#include <QLoggingCategory>

// using namespace Dtk::Core;
DWIDGET_USE_NAMESPACE

// 不同布局模式(紧凑)
const int s_DDropdownMenuHeight = 28;
const int s_DDropdownMenuHeightCompact = 20;

// 原生绘制文本的工具按钮：文本直接画在控件 backing store 上，
// 不再经过中间 QPixmap/QIcon，任何缩放下都按实际物理像素栅格化，
// 避免分数缩放时位图被非 1:1 重采样导致的文字发虚（长文本尤其明显）。
class DDropdownMenu::LabelToolButton : public DToolButton
{
public:
    explicit LabelToolButton(DDropdownMenu *host, QWidget *parent = nullptr)
        : DToolButton(parent), m_host(host) {}

protected:
    void paintEvent(QPaintEvent *event) override
    {
        // 先走基类绘制，保留 DStyle 的悬停/焦点等状态层（无图标时仅绘制按钮底板）
        DToolButton::paintEvent(event);

        DPalette dpalette = DPaletteHelper::instance()->palette(this);
        // 点击时前景色变为高亮色，与原位图方案一致
        const QColor textColor = m_host->m_bPressed
                ? dpalette.color(DPalette::Highlight)
                : dpalette.color(DPalette::WindowText);

        const QFontMetrics metrics(m_host->m_font);
        const int fontHeight = metrics.size(Qt::TextSingleLine, m_host->m_text).height();
        const int arrowX = 10 + metrics.horizontalAdvance(m_host->m_text) + 10;

        QPainter painter(this);
        painter.setFont(m_host->m_font);
        painter.setPen(textColor);
        // 整数逻辑坐标绘制，保证字形不落在半像素上
        painter.drawText(QRectF(10, (height() - fontHeight) / 2, width() - 20, fontHeight),
                         m_host->m_text);

        const QPixmap &arrow = (m_host->m_bPressed && !m_host->m_pressedArrowPixmap.isNull())
                ? m_host->m_pressedArrowPixmap
                : m_host->m_arrowPixmap;
        painter.drawPixmap(QRectF(arrowX, (height() - 5) / 2, 8, 5), arrow, arrow.rect());
    }

private:
    DDropdownMenu *m_host;
};

DDropdownMenu::DDropdownMenu(QWidget *parent)
    : QFrame(parent)
    , m_pToolButton(new LabelToolButton(this, this))
    , m_menu(new DMenu)
{
    qDebug() << "DDropdownMenu constructor";
    // 更新单独添加的高亮格式文件
    m_Repository.addCustomSearchPath(KF5_HIGHLIGHT_PATH);

    //设置toobutton属性
    m_pToolButton->setFocusPolicy(Qt::StrongFocus);
    m_pToolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_pToolButton->setArrowType(Qt::NoArrow);
    m_pToolButton->installEventFilter(this);
    //this->installEventFilter(this);
    m_pToolButton->setAccessibleName("DropdownToolButton");
    m_pToolButton->setObjectName("PToolButton");

    //设置字体
    int fontsize = DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T9);
    m_font.setPixelSize(fontsize);

     //添加布局
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_pToolButton);
    layout->setContentsMargins(0,0,0,0);
    this->setLayout(layout);

    //按当前主题生成箭头，文本由按钮原生绘制（不再生成位图图标）
    setTheme((DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Window).lightness() < 128) ? "dark" : "light");
    updateButtonSize();

    connect(this, &DDropdownMenu::requestContextMenu, this, &DDropdownMenu::slotRequestMenu);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    //设置字体自适应大小
    //设置界面大小根据内容大小自适应 梁卫东 2020.7.7
    connect(qApp,&DApplication::fontChanged,this,&DDropdownMenu::OnFontChangedSlot);
#else
    qApp->installEventFilter(this);
#endif

#ifdef DTKWIDGET_CLASS_DSizeMode
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [this](){
        updateButtonSize();
    });
#endif
    qDebug() << "DDropdownMenu constructor end";
}

DDropdownMenu::~DDropdownMenu()
{
    qDebug() << "DDropdownMenu destructor";
    deleteMenuActionGroup();
    deleteMenu();
    qDebug() << "DDropdownMenu destructor end";
}

void DDropdownMenu::setFontEx(const QFont& font)
{
    qDebug() << "DDropdownMenu setFontEx";
    m_pToolButton->setFont(font);
    m_font = font;
    //字体变化影响文本宽度，需重算按钮尺寸
    updateButtonSize();
    qDebug() << "DDropdownMenu setFontEx end";
}

void DDropdownMenu::setCurrentAction(QAction *pAct)
{
    qDebug() << "DDropdownMenu setCurrentAction";
    if(pAct){
        qDebug() << "pAct is not null";
        if (m_menu == nullptr) {
            qDebug() << "DDropdownMenu setCurrentAction: menu is null";
            setText(pAct->text());
            return;
        }
        QList<QAction*> menuList = m_menu->actions();
        pAct->setChecked(true);
        for (int i = 0; i < menuList.size(); i++) {
            QMenu *subMenu = menuList[i]->menu();
            if (subMenu == nullptr) {
                continue;
            }
            QList<QAction*> acts = subMenu->actions();
            for (int j = 0; j < acts.size(); j++) {
                if(acts[j] != pAct) acts[j]->setChecked(false);
            }
        }
        setText(pAct->text());
    }
    qDebug() << "DDropdownMenu setCurrentAction end";
}

void DDropdownMenu::setCurrentTextOnly(const QString &name)
{
    qDebug() << "DDropdownMenu setCurrentTextOnly";
    // QList<QAction*> menuList = m_menu->actions();

//   for (int i = 0; i < menuList.size(); i++) {
//       if(menuList[i]->menu()){
//           QList<QAction*> acts = menuList[i]->menu()->actions();
//           if(acts.size() == 0) continue;
//           for (int j = 0; j < acts.size(); j++) {
//           if(acts[j]->text() != name){
//               acts[j]->setCheckable(false);
//               acts[j]->setChecked(false);
//           }
//           else{
//               acts[j]->setCheckable(true);
//               acts[j]->setChecked(true);
//           }
//        }
//      }
//   }
   if (m_menu != nullptr) {
       for(auto ac:m_menu->actions()){
           setCheckedExclusive(ac,name);
       }
   }

   setText(name);
   qDebug() << "DDropdownMenu setCurrentTextOnly end";
}


void DDropdownMenu::setCheckedExclusive(QAction* action,const QString& name)
{
    qDebug() << "DDropdownMenu setCheckedExclusive";
    if (nullptr == action) {
        qDebug() << "action is null, return";
        return;
    }

    if (action->menu()) {
        qDebug() << "action has menu";
        for(auto ac:action->menu()->actions()){
            setCheckedExclusive(ac,name);
        }
    } else {
        qDebug() << "action has no menu";
        if (action->text() != name) {
            qDebug() << "action text is not equal to name";
            action->setCheckable(false);
            action->setChecked(false);
        } else {
            qDebug() << "action text is equal to name";
            action->setCheckable(true);
            action->setChecked(true);
        }
    }
    qDebug() << "DDropdownMenu setCheckedExclusive end";
}

void DDropdownMenu::slotRequestMenu(bool request)
{
    qDebug() << "DDropdownMenu slotRequestMenu";
    if (request) {
        qDebug() << "request is true";
        //如果鼠标点击清除ｆｏｃｕｓ
        m_pToolButton->clearFocus();
    }
    QPoint center = this->mapToGlobal(this->rect().center());
    int menuHeight = m_menu->sizeHint().height();
    int menuWidth = m_menu->sizeHint().width();
    center.setY(center.y() - menuHeight - this->rect().height() / 2);
    center.setX(center.x() - menuWidth / 2);
    m_menu->move(center);
    m_menu->exec();
    //清除ｆｏｃｕｓ
    m_pToolButton->clearFocus();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QHoverEvent event(QEvent::HoverLeave, center, center, center);
    QApplication::sendEvent(m_pToolButton, &event);
#else
    QEvent event(QEvent::HoverLeave);
    QApplication::sendEvent(m_pToolButton, &event);
#endif
    emit sigSetTextEditFocus();
    qDebug() << "DDropdownMenu slotRequestMenu end";
}

void DDropdownMenu::setText(const QString &text)
{
    qDebug() << "DDropdownMenu setText:" << text;
    m_text = text;
    //文本宽度变化时重算按钮尺寸（文本原生绘制，无需重建位图图标）
    updateButtonSize();
    qDebug() << "DDropdownMenu setText end";
}

void DDropdownMenu::setMenu(DMenu *menu)
{
    qDebug() << "DDropdownMenu setMenu";
    deleteMenu();
    m_menu = menu;
    if (m_menu != nullptr) {
        m_menu->setObjectName("DropdownMenu");
        m_menu->setAccessibleName("DropdownMenu");
    }
}

void DDropdownMenu::deleteMenu()
{
    qDebug() << "DDropdownMenu deleteMenu";
    if (m_menu != nullptr) {
        qDebug() << "m_menu is not null, deleting";
        delete m_menu;
        m_menu = nullptr;
    }
    qDebug() << "DDropdownMenu deleteMenu end";
}

void DDropdownMenu::setMenuActionGroup(QActionGroup *actionGroup)
{
    qDebug() << "DDropdownMenu setMenuActionGroup";
    deleteMenuActionGroup();
    m_actionGroup = actionGroup;
    m_actionGroup->setObjectName("ActionGroup");
    qDebug() << "DDropdownMenu setMenuActionGroup end";
}

void DDropdownMenu::deleteMenuActionGroup()
{
    qDebug() << "DDropdownMenu deleteMenuActionGroup";
    if (m_actionGroup != nullptr) {
        qDebug() << "m_actionGroup is not null, deleting";
        delete m_actionGroup;
        m_actionGroup = nullptr;
    }
    qDebug() << "DDropdownMenu deleteMenuActionGroup end";
}

void DDropdownMenu::setTheme(const QString &theme)
{
    qDebug() << "DDropdownMenu setTheme:" << theme;
    m_theme = theme;
    QString arrowSvgPath = QString(":/images/dropdown_arrow_%1.svg").arg(theme);
    // 根据当前显示缩放转换图片
    qreal scaled = this->devicePixelRatioF();
    QSvgRenderer svg_render(arrowSvgPath);

    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);

    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));

    m_arrowPixmap = pixmap;
    //按下态箭头依赖旧调色板颜色，置空待下次按下时按当前调色板重建
    m_pressedArrowPixmap = QPixmap();
    m_pToolButton->update();
    qDebug() << "DDropdownMenu setTheme end";
}

void DDropdownMenu::setChildrenFocus(bool ok)
{
    qDebug() << "DDropdownMenu setChildrenFocus:" << ok;
    if (ok) {
        qDebug() << "setChildrenFocus is true";
        m_pToolButton->setFocusPolicy(Qt::StrongFocus);
    } else  {
        qDebug() << "setChildrenFocus is false";
        m_pToolButton->clearFocus();
        m_pToolButton->setFocusPolicy(Qt::NoFocus);
    }
    qDebug() << "DDropdownMenu setChildrenFocus end";
}

void DDropdownMenu::setRequestMenu(bool request)
{
    qDebug() << "DDropdownMenu setRequestMenu:" << request;
    isRequest = request;
    qDebug() << "DDropdownMenu setRequestMenu end";
}

DToolButton *DDropdownMenu::getButton()
{
    qDebug() << "DDropdownMenu getButton";
    return m_pToolButton;
}

QString DDropdownMenu::getCurrentText() const
{
    qDebug() << "DDropdownMenu getCurrentText";
    return m_text;
}

DDropdownMenu *DDropdownMenu::createEncodeMenu()
{
    qDebug() << "DDropdownMenu createEncodeMenu";
    DDropdownMenu *m_pEncodeMenu = new DDropdownMenu();
    DMenu* m_pMenu = new DMenu();

    auto groupEncodeVec = Utils::getSupportEncoding();
    if (!groupEncodeVec.isEmpty()) {
        qDebug() << "groupEncodeVec is not empty, creating menu";
        int cnt = groupEncodeVec.size();
        for (int i = 0; i < cnt;i++) {
            QMenu* groupMenu = new QMenu(QObject::tr(groupEncodeVec[i].first.toLocal8Bit().data()));
             foreach(QString var, groupEncodeVec[i].second)
             {
               QAction *act= groupMenu->addAction(QObject::tr(var.toLocal8Bit().data()));
               if (act->text() == "UTF-8") {
                    qDebug() << "Adding UTF-8 action";
                    m_pEncodeMenu->m_pActUtf8 = act;
                    act->setCheckable(true);
                    act->setChecked(true);
                    m_pEncodeMenu->m_pActUtf8->setObjectName("PActUtf8");
               } else {
                    qDebug() << "Adding other action";
                    act->setCheckable(false);
               }
            }

            m_pMenu->addMenu(groupMenu);
        }
    }

    connect(m_pMenu, &DMenu::triggered, m_pEncodeMenu,[m_pEncodeMenu](QAction *action) {
        qInfo() << "Encoding changed to:" << action->text();
        //编码内容改变触发内容改变和信号发射 梁卫东 2020.7.7
        if (m_pEncodeMenu->m_text != action->text()) {
            qDebug() << "Encoding changed to:" << action->text();
            emit m_pEncodeMenu->currentActionChanged(action);
        }
    });

    m_pEncodeMenu->setText("UTF-8");
    m_pEncodeMenu->setMenu(m_pMenu);

    qDebug() << "DDropdownMenu createEncodeMenu end";
    return  m_pEncodeMenu;
}

DDropdownMenu *DDropdownMenu::createHighLightMenu()
{
    qDebug() << "DDropdownMenu createHighLightMenu";
    DDropdownMenu *m_pHighLightMenu = new DDropdownMenu();
    DMenu *m_pMenu = new DMenu;
    QAction *noHlAction = m_pMenu->addAction(tr("None"));
    noHlAction->setCheckable(true);

    QActionGroup* m_pActionGroup = new QActionGroup(m_pMenu);
    m_pActionGroup->setExclusive(true);
    m_pActionGroup->addAction(noHlAction);

    DMenu *pSubMenu = nullptr;
    QString currentGroup;

    bool intel = true;
    for (KSyntaxHighlighting::Definition def : m_pHighLightMenu->m_Repository.definitions()) {

        if(def.translatedName()=="Intel x86 (NASM)"&&intel)
        {
            intel = false;
            qDebug() << "Intel x86 (NASM) is hidden, continue";
            continue;
        }
        if (def.isHidden()) {
            qDebug() << "def is hidden, continue";
            continue;
        }

        if (currentGroup != def.section()) {
            qDebug() << "currentGroup is not equal to def.section(), creating new submenu";
            currentGroup = def.section();
            pSubMenu = m_pMenu->addMenu(def.translatedSection());
        }

        if (!pSubMenu) {
            qDebug() << "pSubMenu is null, continue";
            continue;
        }

        QAction* action = pSubMenu->addAction(def.translatedName());
        action->setCheckable(true);
        action->setText(def.name());
        m_pActionGroup->addAction(action);
    }

    // 转发选中“None“无高亮选项的信号
    connect(noHlAction, &QAction::triggered, m_pHighLightMenu, [noHlAction, m_pHighLightMenu] (bool checked) {
        if (checked) {
            qInfo() << "Highlight mode changed to None";
            emit m_pHighLightMenu->currentActionChanged(noHlAction);
        }
    });

    connect(m_pActionGroup, &QActionGroup::triggered, m_pHighLightMenu, [m_pHighLightMenu] (QAction *action) {
        const auto defName = action->text();
        const auto def = m_pHighLightMenu->m_Repository.definitionForName(defName);
        if (def.isValid() && m_pHighLightMenu->m_text != action->text()) {
            qInfo() << "Highlight mode changed to:" << defName;
            emit m_pHighLightMenu->currentActionChanged(action);
        }
        else {
            qDebug() << "Invalid highlight definition, setting to None";
            m_pHighLightMenu->setText(tr("None"));
        }
        
    });

    m_pHighLightMenu->setText(tr("None"));
    m_pHighLightMenu->setMenu(m_pMenu);
    m_pHighLightMenu->setMenuActionGroup(m_pActionGroup);

    qDebug() << "DDropdownMenu createHighLightMenu end";
    return m_pHighLightMenu;
}

void DDropdownMenu::updateButtonSize()
{
    if (m_pToolButton == nullptr) {
        return;
    }
    // 根据字体大小计算尺寸，按计算的字体高度，而非从字体文件中读取的高度(部分字体中英文高度不同)
    QFontMetrics metrics(m_font);
    // 内边距与原实现一致：左 10 + 文本 + 10 + 箭头 8 + 右 10
    const int totalWidth = metrics.horizontalAdvance(m_text) + 20 + 8 + 10;
#ifdef DTKWIDGET_CLASS_DSizeMode
    const int totalHeigth = DGuiApplicationHelper::isCompactMode() ? s_DDropdownMenuHeightCompact : s_DDropdownMenuHeight;
#else
    const int totalHeigth = s_DropdownMenuHeight;
#endif
    m_pToolButton->setFixedSize(totalWidth, totalHeigth);
    m_pToolButton->update();
}

void DDropdownMenu::OnFontChangedSlot(const QFont &font)
{
    qDebug() << "DDropdownMenu OnFontChangedSlot";
    m_font = font;
    //字体族跟随系统字体，但字号保持底栏 T9（原 Qt6 分支漏设字号会导致文字随应用字号变化）
    int fontsize = DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T9);
    m_font.setPixelSize(fontsize);
    m_pToolButton->setFont(m_font);
    updateButtonSize();
    qDebug() << "DDropdownMenu OnFontChangedSlot end";
}


bool DDropdownMenu::eventFilter(QObject *object, QEvent *event)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (event->type() == QEvent::ApplicationFontChange) {
        // 处理字体变化
        QFont font = qApp->font(); // 获取当前应用程序字体
        OnFontChangedSlot(font);   // 调用槽函数更新字体
        return false;
    }

    // 窗口移到不同缩放的屏幕时，按新 DPR 重建箭头位图（文本为原生绘制，无需处理）
    if (object == this && event->type() == QEvent::DevicePixelRatioChange) {
        setTheme(m_theme);
        return false;
    }
#endif

    if(object == m_pToolButton){
        if(event->type() == QEvent::KeyPress){
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            qDebug() << "Key pressed:" << keyEvent->key();
            if(keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Space)        //按下enter展开列表
            {
                qDebug() << "Triggering context menu via keyboard";
                Q_EMIT requestContextMenu(false);
                return true;
            }
            qDebug() << "Key event not handled, return false";
            return false;
        }

        if(event->type() == QEvent::MouseButtonPress){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                qDebug() << "Left mouse button pressed on dropdown menu";
                m_bPressed = true;
                //按下时按当前调色板高亮色重建箭头，文本颜色由 paintEvent 实时取色
                DPalette dpalette = DPaletteHelper::instance()->palette(m_pToolButton);
                m_pressedArrowPixmap = setSvgColor(dpalette.color(DPalette::Highlight).name(QColor::HexRgb));
                m_pToolButton->update();
                return true;
            }

            if(mouseEvent->button() == Qt::RightButton){
                qDebug() << "Right mouse button pressed on dropdown menu, return true";
                return true;
            }
        }

        if(event->type() == QEvent::MouseButtonRelease){
            qDebug() << "Left mouse button released on dropdown menu";
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                m_bPressed = false;
                m_pToolButton->update();
                if (isEnabled()) {
                    qDebug() << "Left mouse button released, showing context menu";
                    Q_EMIT requestContextMenu(true);
                }
                m_pToolButton->clearFocus();
            }
            qDebug() << "Left mouse button released, return true";
            return true;
        }
    }

    return QFrame::eventFilter(object,event);
}

QPixmap DDropdownMenu::setSvgColor(QString color)
{
    qDebug() << "DDropdownMenu setSvgColor";
    //设置图标颜色
    QString path = QString(":/images/arrow_dark.svg");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "DDropdownMenu setSvgColor: failed to open" << path << file.errorString();
        return QPixmap();
    }
    QByteArray data = file.readAll();
    QDomDocument doc;
    doc.setContent(data);
    file.close();
    QDomElement elem = doc.documentElement();
    SetSVGBackColor(elem, "fill", color);

    //装换图片
    //int scaled =qApp->devicePixelRatio() == 1.25 ? 2 : 1;
    qreal scaled = this->devicePixelRatioF();
    QSvgRenderer svg_render(doc.toByteArray());

    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);

    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));

    qDebug() << "DDropdownMenu setSvgColor end";
    return pixmap;
}

void DDropdownMenu::SetSVGBackColor(QDomElement &elem, QString strattr, QString strattrval)
{
    qDebug() << "DDropdownMenu SetSVGBackColor";
    if (elem.tagName().compare("g") == 0 && elem.attribute("id").compare("color") == 0)
    {
        qDebug() << "DDropdownMenu SetSVGBackColor, found color group";
        elem.setAttribute(strattr, strattrval);
    }
    for (int i = 0; i < elem.childNodes().count(); i++)
    {
        if (!elem.childNodes().at(i).isElement()) continue;
        QDomElement element = elem.childNodes().at(i).toElement();
        SetSVGBackColor(element, strattr, strattrval);
    }
    qDebug() << "DDropdownMenu SetSVGBackColor end";
}
