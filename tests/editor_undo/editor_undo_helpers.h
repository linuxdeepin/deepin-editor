// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// editor_undo 批次（B5）测试公共设施：
//  - ut::ensureApp()：offscreen QApplication（QT_QPA_PLATFORM 由 CMake 测试属性与
//    运行环境注入，本文件不读取/设置环境变量，无 qputenv/qunsetenv 配对问题）
//  - ut::toLf()：QTextCursor::selectedText() 以 U+2029 表示换行，比较前统一归一化
//  - ut::makeSelection()：按起止绝对位置构造绑定文档的 ExtraSelection
//  - RecordingCommand：undo/redo 顺序记录命令（UndoList / ChangeMarkCommand 子命令）
//  - EditorUndoTestBase：接缝状态 + 空壳 TextEdit 的通用夹具基类
//
// 夹具基类说明：各测试 Fixture 命名为 {ClassName}Test : public EditorUndoTestBase，
// 符合 test-types.md §3.4.3 派生夹具约定（共享 SetUp：重置并安装接缝、构造编辑器；
// 共享 TearDown：卸载接缝、销毁编辑器）。被测 QUndoCommand 子类无信号发射
//（QUndoCommand 非 QObject），故不使用 QSignalSpy，副作用经接缝记录与文档状态断言。

#ifndef AUTOTESTS_EDITOR_UNDO_HELPERS_H
#define AUTOTESTS_EDITOR_UNDO_HELPERS_H

#include <gtest/gtest.h>

#include <QApplication>
#include <QList>
#include <QPlainTextEdit>
#include <QString>
#include <QTextCursor>
#include <QTextDocument>

#include "editor_undo_seam.h"
#include "editwrapper.h"
#include "../widgets/bottombar.h"

namespace ut {

// 确保 offscreen QApplication 存在（Qt widget 类构造必需）。
inline QApplication *ensureApp()
{
    if (!QApplication::instance()) {
        static int argc = 1;
        static char argv0[] = "test_editor_undo";
        static char *argv[] = { argv0, nullptr };
        static QApplication app(argc, argv);
        return &app;
    }
    return qobject_cast<QApplication *>(QApplication::instance());
}

// U+2029（段落分隔符）→ '\n'，用于比较 QTextCursor::selectedText() 结果。
inline QString toLf(const QString &s)
{
    QString r = s;
    r.replace(QChar(0x2029), QChar('\n'));
    return r;
}

// 构造绑定 doc 的选区光标：anchor=startPos，position=endPos。
inline QTextCursor cursorAt(QTextDocument *doc, int startPos, int endPos)
{
    QTextCursor cursor(doc);
    cursor.setPosition(startPos);
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    return cursor;
}

inline QTextEdit::ExtraSelection makeSelection(QTextDocument *doc, int startPos, int endPos)
{
    QTextEdit::ExtraSelection selection;
    selection.cursor = cursorAt(doc, startPos, endPos);
    return selection;
}

// 以普通 QWidget 内存伪装 BottomBar*/Window*/EditWrapper*：这三者的接缝成员均为
// 非虚函数，直接调用记录桩，不触碰伪造对象的 vtable，安全可控。
inline BottomBar *fakeBottomBar(QObject *host)
{
    return reinterpret_cast<BottomBar *>(host);
}

}  // namespace ut

// 顺序记录命令：undo/redo 向夹具成员 orderLog 追加 "name:undo"/"name:redo"。
// orderLog 指向夹具成员（非 static/全局），用例间无残留。
class RecordingCommand : public QUndoCommand
{
public:
    explicit RecordingCommand(QStringList *orderLog, const QString &name, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , m_orderLog(orderLog)
        , m_name(name)
    {
    }

    void undo() override
    {
        if (m_orderLog)
            *m_orderLog << (m_name + ":undo");
    }

    void redo() override
    {
        if (m_orderLog)
            *m_orderLog << (m_name + ":redo");
    }

private:
    QStringList *m_orderLog = nullptr;
    QString m_name;
};

// 通用夹具基类：接缝状态成员 + 空壳 TextEdit（行为等价 DPlainTextEdit）。
class EditorUndoTestBase : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // 空壳 TextEdit 为 QWidget 派生，须 QApplication；offscreen 平台无真实窗口。
        ut::ensureApp();
    }

    void SetUp() override
    {
        seam = EditorUndoSeam();  // 全部计数器/记录归零
        ut_install_seam(&seam);
        edit = new TextEdit();
    }

    void TearDown() override
    {
        ut_install_seam(nullptr);
        delete edit;
        edit = nullptr;
    }

    QString docText() const { return edit->document()->toPlainText(); }

    EditorUndoSeam seam;
    TextEdit *edit = nullptr;
};

#endif  // AUTOTESTS_EDITOR_UNDO_HELPERS_H
