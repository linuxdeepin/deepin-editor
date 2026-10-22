// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "editorapplication.h"
#include "common/settings.h"
#include <QDebug>

EditorApplication::EditorApplication(int &argc, char *argv[]) : DApplication(argc, argv)
{
    qCDebug(appLog) << "Enter EditorApplication constructor";
    const char *descriptionText = QT_TRANSLATE_NOOP(
        "MainWindow", "Text Editor is a powerful tool for viewing and editing text files.");
    const QString acknowledgementLink = "https://www.deepin.org/original/deepin-editor/";
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    qCDebug(appLog) << "Setting Qt::AA_UseHighDpiPixmaps attribute for Qt version < 6.0.0";
    setAttribute(Qt::AA_UseHighDpiPixmaps);
#else
    qCDebug(appLog) << "Running with Qt version >= 6.0.0, skipping AA_UseHighDpiPixmaps";
#endif
    qCDebug(appLog) << "Loading translator";
    loadTranslator();
    qCDebug(appLog) << "Setting organization name: deepin";
    setOrganizationName("deepin");
    qCDebug(appLog) << "Setting application name: deepin-editor";
    setApplicationName("deepin-editor");
    qCDebug(appLog) << "Setting application display name: Text Editor";
    setApplicationDisplayName(QObject::tr("Text Editor"));
    qCDebug(appLog) << "Setting application version:" << VERSION;
    setApplicationVersion(VERSION);
    qCDebug(appLog) << "Setting product icon from theme: deepin-editor";
    setProductIcon(QIcon::fromTheme("deepin-editor"));
    qCDebug(appLog) << "Setting product name: Text Editor";
    setProductName(DApplication::translate("MainWindow", "Text Editor"));
    qCDebug(appLog) << "Setting application description";
    setApplicationDescription(DApplication::translate("MainWindow", descriptionText) + "\n");
    qCDebug(appLog) << "Setting acknowledgement page:" << acknowledgementLink;
    setApplicationAcknowledgementPage(acknowledgementLink);
    qCDebug(appLog) << "Setting quitOnLastWindowClosed to false";
    setQuitOnLastWindowClosed(false);
    qCDebug(appLog) << "Exit EditorApplication constructor";
    qCInfo(appLog) << "Application initialized with version:" << VERSION;
}

EditorApplication::~EditorApplication()
{
    qCDebug(appLog) << "Enter EditorApplication destructor";
    // StartManager 生命周期由 main() 接管（创建于 create()，事件循环结束后显式释放），
    // 析构不得触碰，否则 D-Bus 转发进程会在退出期意外实例化单例（BUG-378901）
    qCDebug(appLog) << "Exit EditorApplication destructor";
    qCInfo(appLog) << "Application resources released";
}

void EditorApplication::handleQuitAction()
{
    qCDebug(appLog) << "Enter handleQuitAction";
    QWidget* active = activeWindow();
    if (active) {
        qCDebug(appLog) << "Active window found, closing it";
        active->close();
    } else {
        qCWarning(appLog) << "No active window found to close";
    }
    qCDebug(appLog) << "Exit handleQuitAction";
    qCInfo(appLog) << "Quit action triggered, closing active window";
}

bool EditorApplication::notify(QObject *object, QEvent *event)
{
    // ALT+M = 右键
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyevent = static_cast<QKeyEvent *>(event);
        /***add begin by ut001121 zhangmeng 20200801 截获DPushButton控件回车按键事件并模拟空格键点击事件,用以解决回车键不响应的问题***/
        // 回车键 恢复默认 添健按钮
        if ((object->metaObject()->className() == QStringLiteral("QPushButton")
                // 远程和自定义列表的返回按钮，编辑按钮
                || object->metaObject()->className() == QStringLiteral("IconButton")
                // 搜索框的上下搜索
                || object->metaObject()->className() == QStringLiteral("Dtk::Widget::DIconButton")
                // 设置里面的单选框
                || object->metaObject()->className() == QStringLiteral("QCheckBox")
                // 设置字体组合框
                || object->metaObject()->className() == QStringLiteral("QComboBox")
                // 设置窗口组合框
                || object->metaObject()->className() == QStringLiteral("ComboBox"))
                && (keyevent->key() == Qt::Key_Return || keyevent->key() == Qt::Key_Enter)) {
            DPushButton *pushButton = static_cast<DPushButton *>(object);
            // 模拟空格键按下事件
            pressSpace(pushButton);
            qCDebug(appLog) << "Return/Enter key event processed, simulated space key press";
            return true;
        }
        /***add end by ut001121***/
        // 左键
        // 远程和自定义列表的返回按钮 Key_Left
        if ((object->objectName() == QStringLiteral("CustomRebackButton")
                || object->objectName() == QStringLiteral("RemoteSearchRebackButton")
                || object->objectName() == QStringLiteral("RemoteGroupRebackButton"))
                && keyevent->key() == Qt::Key_Left) {
            DPushButton *pushButton = static_cast<DPushButton *>(object);
            // 模拟空格键按下事件
            pressSpace(pushButton);
            qCDebug(appLog) << "Left key event processed, simulated space key press";
            return true;
        }
        qCDebug(appLog) << "KeyPress event delegated to QApplication::notify";
        return QApplication::notify(object, event);
    }

    return QApplication::notify(object, event);
}

void EditorApplication::pressSpace(DPushButton *pushButton)
{
    qCDebug(appLog) << "Enter pressSpace, button:" << pushButton->objectName();
    // 模拟空格键按下事件
    QKeyEvent pressSpace(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, " ");
    qCDebug(appLog) << "Sending space key press event to button";
    QApplication::sendEvent(pushButton, &pressSpace);
    // 设置定时
    QTimer::singleShot(80, this, [pushButton]() {
        qCDebug(appLog) << "Timer triggered, sending space key release to button:" << pushButton->objectName();
        // 模拟空格键松开事件
        QKeyEvent releaseSpace(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier, " ");
        QApplication::sendEvent(pushButton, &releaseSpace);
    });
    qCDebug(appLog) << "Exit pressSpace, scheduled release event in 80ms";
}
