// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDROPDOWNMENU_H
#define DDROPDOWNMENU_H

#include <QFrame>
#include <DMenu>
#include <QLabel>
#include <QEvent>
#include <DToolButton>
#include <QPalette>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>
#include <QtXml/QDomNodeList>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
DWIDGET_USE_NAMESPACE

class DDropdownMenu : public QFrame
{
    Q_OBJECT

public:
    explicit DDropdownMenu(QWidget *parent = nullptr);
    ~DDropdownMenu();
    void setFontEx(const QFont& font);

    void setMenu(DMenu *menu);
    void deleteMenu();
    void setMenuActionGroup(QActionGroup *actionGroup);
    void deleteMenuActionGroup();
    void setTheme(const QString &theme);

    void setChildrenFocus(bool ok);
    void setRequestMenu(bool request);
    DToolButton* getButton();
    QString getCurrentText() const;

public slots:
    void setCurrentAction(QAction*);
    void setCurrentTextOnly(const QString& name);
    void slotRequestMenu(bool request);
    void setCheckedExclusive(QAction* action,const QString& name);

public:
    //创建编码菜单
    static DDropdownMenu* createEncodeMenu();
    //创建文件类型菜单
    static DDropdownMenu* createHighLightMenu();

signals:
    void requestContextMenu(bool bClicked = false);
    void currentTextChanged(const QString &text);
    void currentActionChanged(QAction*);
    void sigSetTextEditFocus();

private:
    //原生绘制文本的工具按钮（嵌套类可直接访问宿主私有成员，定义在 cpp）
    class LabelToolButton;
    void setText(const QString &text);
    //按当前文本与字体计算并应用按钮尺寸
    void updateButtonSize();
private slots:
    //字体大小跟随系统变化
    void OnFontChangedSlot(const QFont &font);
protected:
    //按键事件　鼠标释放弹出菜单
    bool eventFilter(QObject *object, QEvent *event);
private:
    QPixmap setSvgColor(QString color);
    void SetSVGBackColor(QDomElement &elem, QString strattr, QString strattrval);
private:
    DToolButton *m_pToolButton = nullptr;   //实际类型为 LabelToolButton（见 cpp）
    DMenu *m_menu = nullptr;
    QActionGroup *m_actionGroup = nullptr;
    QPixmap m_arrowPixmap;
    QPixmap m_pressedArrowPixmap;  //按下态高亮色箭头，首次按下时生成
    QString m_theme;               //当前箭头主题（dark/light）
    QString m_text = "UTF-8";
    QAction* m_pActUtf8 = nullptr;
    QFont m_font;
    bool m_bPressed =false;
    bool isRequest = false;
    KSyntaxHighlighting::Repository m_Repository;
};

#endif
