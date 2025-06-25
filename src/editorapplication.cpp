// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "editorapplication.h"
#include "common/settings.h"
#include <QDebug>

EditorApplication::EditorApplication(int &argc, char *argv[]) : DApplication(argc, argv)
{
    qDebug() << "Enter EditorApplication constructor";
    const char *descriptionText = QT_TRANSLATE_NOOP(
        "MainWindow", "Text Editor is a powerful tool for viewing and editing text files.");
    const QString acknowledgementLink = "https://www.deepin.org/original/deepin-editor/";
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    qDebug() << "Setting Qt::AA_UseHighDpiPixmaps attribute for Qt version < 6.0.0";
    setAttribute(Qt::AA_UseHighDpiPixmaps);
#else
    qDebug() << "Running with Qt version >= 6.0.0, skipping AA_UseHighDpiPixmaps";
#endif
    qDebug() << "Loading translator";
    loadTranslator();
    qDebug() << "Setting organization name: deepin";
    setOrganizationName("deepin");
    qDebug() << "Setting application name: deepin-editor";
    setApplicationName("deepin-editor");
    qDebug() << "Setting application display name: Text Editor";
    setApplicationDisplayName(QObject::tr("Text Editor"));
    qDebug() << "Setting application version:" << VERSION;
    setApplicationVersion(VERSION);
    qDebug() << "Setting product icon from theme: deepin-editor";
    setProductIcon(QIcon::fromTheme("deepin-editor"));
    qDebug() << "Setting product name: Text Editor";
    setProductName(DApplication::translate("MainWindow", "Text Editor"));
    qDebug() << "Setting application description";
    setApplicationDescription(DApplication::translate("MainWindow", descriptionText) + "\n");
    qDebug() << "Setting acknowledgement page:" << acknowledgementLink;
    setApplicationAcknowledgementPage(acknowledgementLink);
    qDebug() << "Setting quitOnLastWindowClosed to false";
    setQuitOnLastWindowClosed(false);
    qDebug() << "Exit EditorApplication constructor";
    qInfo() << "Application initialized with version:" << VERSION;
}

EditorApplication::~EditorApplication()
{
    qDebug() << "Enter EditorApplication destructor";
    // app结束时，释放
    if (nullptr != StartManager::instance()) {
        qDebug() << "Deleting StartManager instance";
        delete StartManager::instance();
    } else {
        qDebug() << "StartManager instance is already null";
    }
    qDebug() << "Exit EditorApplication destructor";
    qInfo() << "Application resources released";
}

void EditorApplication::handleQuitAction()
{
    qDebug() << "Enter handleQuitAction";
    QWidget* active = activeWindow();
    if (active) {
        qDebug() << "Active window found, closing it";
        active->close();
    } else {
        qWarning() << "No active window found to close";
    }
    qDebug() << "Exit handleQuitAction";
    qInfo() << "Quit action triggered, closing active window";
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
            qDebug() << "Return/Enter key event processed, simulated space key press";
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
            qDebug() << "Left key event processed, simulated space key press";
            return true;
        }
        qDebug() << "KeyPress event delegated to QApplication::notify";
        return QApplication::notify(object, event);
    }

    return QApplication::notify(object, event);
}

void EditorApplication::pressSpace(DPushButton *pushButton)
{
    qDebug() << "Enter pressSpace, button:" << pushButton->objectName();
    // 模拟空格键按下事件
    QKeyEvent pressSpace(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, " ");
    qDebug() << "Sending space key press event to button";
    QApplication::sendEvent(pushButton, &pressSpace);
    // 设置定时
    QTimer::singleShot(80, this, [pushButton]() {
        qDebug() << "Timer triggered, sending space key release to button:" << pushButton->objectName();
        // 模拟空格键松开事件
        QKeyEvent releaseSpace(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier, " ");
        QApplication::sendEvent(pushButton, &releaseSpace);
    });
    qDebug() << "Exit pressSpace, scheduled release event in 80ms";
}
