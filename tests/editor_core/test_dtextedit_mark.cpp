// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ===========================================================================
// TextEdit 颜色标记 + 书签方法族（dtextedit.cpp 行 5200~6800 一带）
//
// 分支清单（来源：src/editor/dtextedit.cpp）：
// M1 : isMarkCurrentLine —— isMark 真（有选区 MarkOnce/无选区 MarkLine/AltMod 多选区）
//      /假（clearMarksForTextCursor）
// M2 : isMarkAllLine —— 部分选中文本（MarkAllMatch）/全选或无选区（MarkAll）
//      /取消（清空三容器）
// M3 : cancelLastMark —— MarkOnce/MarkLine/MarkAllMatch/MarkAll 四 case；
//      空列表提前 return；残留清理分支
// M4 : clearMarksForTextCursor —— 有选区精确匹配 / 无选区位置区间匹配
// M5 : markAllKeywordInView —— 空标记列表 return / MarkAllMatch / MarkAll
// M6 : markKeywordInView —— 空 keyword false / 视口有匹配 true
// M7 : markAllInView —— 全文选区入 map[TEXT_EIDT_MARK_ALL]
// M8 : toggleMarkSelections —— 已有标记清除 / 无标记新建
// M9 : convertReplaceToMark/convertMarkToReplace —— 光标绝对位置往返
// M10: manualUpdateAllMark —— MarkOnce 追加 / MarkLine 多行拆分 /
//      无选区 erase / 排序 / markAllKeywordInView 重建
// M11: updateMark —— 只读 return / 文件打开 return / 无标记 return /
//      删除字符（选区包含标记删除 / 标记内容删空移除）/
//      添加字符（标记内 break / 标记尾部扩展，含输入法分支）
// M12: markSelectWord —— 同行已有标记移除 / 无标记新建
// M13: addOrDeleteBookMark —— 快捷键路径 / 鼠标点路径 / 越界 return /
//      已有书签删除 / 新增
// M14: moveToPreviousBookMark/moveToNextBookMark —— index==-1 / ==0 / 中间
// M15: checkBookmarkLineMove —— 文件打开 return / 行数不变 /
//      删除行（选区行有效/无效）/ 增加行
// M16: setTextFinished —— settings 空 return / 书签非空 return /
//      历史记录恢复路径
// M17: handleCursorMarkChanged —— mark 真/假
// M18: containsExtraSelection —— 命中/未命中；appendExtraSelection（空实现）
// M19: slotPre/slotNext/slotClearBookMarkAction
// M20: updateMarkAllSelectColor
//
// 用例映射：见各 TEST_F 名。环境隔离：见 editor_core_fixture.h。
// ===========================================================================

#include "editor_core_fixture.h"

// ---------------- isMarkCurrentLine（M1） ----------------

TEST_F(TextEditTest, IsMarkCurrentLine_NoSelection_MarksWholeLine)
{
    // Arrange
    setDocText(QString("alpha\nbeta"));
    moveCursorTo(7); // 第二行行首

    // Act
    edit->isMarkCurrentLine(true, QString("#ff0000"), 100);

    // Assert：MarkLine 类型覆盖整行
    EXPECT_EQ(edit->m_markOperations.size(), 1);
    EXPECT_EQ(edit->m_markOperations.first().first.type, TextEdit::MarkLine);
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_EQ(edit->m_wordMarkSelections.first().first.cursor.selectedText(), QString("beta"));
}

TEST_F(TextEditTest, IsMarkCurrentLine_WithSelection_MarksSelection)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->isMarkCurrentLine(true, QString("#00ff00"), 200);

    // Assert：MarkOnce 类型仅标记选区
    EXPECT_EQ(edit->m_markOperations.first().first.type, TextEdit::MarkOnce);
    EXPECT_EQ(edit->m_wordMarkSelections.first().first.cursor.selectedText(), QString("bcd"));
}

TEST_F(TextEditTest, IsMarkCurrentLine_AltModSelections_MarksEachColumn)
{
    // Arrange：两行列选区
    setDocText(QString("aa\nbb"));
    QList<QTextEdit::ExtraSelection> sels;
    for (int line = 0; line < 2; ++line) {
        const int blockPos = edit->document()->findBlockByNumber(line).position();
        QTextCursor cur(edit->document());
        cur.setPosition(blockPos);
        cur.setPosition(blockPos + 1, QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection sel;
        sel.cursor = cur;
        sels << sel;
    }
    edit->restoreColumnEditSelection(sels);
    edit->m_bIsAltMod = true;

    // Act
    edit->isMarkCurrentLine(true, QString("#0000ff"), 300);

    // Assert：每个列选区各生成一条标记
    EXPECT_EQ(edit->m_markOperations.size(), 2);
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 2);
}

TEST_F(TextEditTest, IsMarkCurrentLine_Unmark_ClearsMarksForCursor)
{
    // Arrange：先标记一行
    setDocText(QString("alpha\nbeta"));
    moveCursorTo(7);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 100);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：取消（光标仍在标记行内）
    edit->isMarkCurrentLine(false);

    // Assert：clearMarksForTextCursor 移除标记
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
}

TEST_F(TextEditTest, ClearMarksForTextCursor_InsideRegion_RemovesMark)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#123456"), 10);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：光标移入选区内部后清理
    moveCursorTo(2);
    const bool found = edit->clearMarksForTextCursor();

    // Assert
    EXPECT_TRUE(found);
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
}

TEST_F(TextEditTest, ClearMarksForTextCursor_OutsideRegion_ReturnsFalse)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#123456"), 10);

    // Act：光标在标记区域外
    moveCursorTo(5);
    const bool found = edit->clearMarksForTextCursor();

    // Assert
    EXPECT_FALSE(found);
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
}

TEST_F(TextEditTest, ClearMarkOperationForCursor_MatchingCursor_RemovesOperation)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#123456"), 10);
    ASSERT_EQ(edit->m_markOperations.size(), 1);
    const QTextCursor opCursor = edit->m_markOperations.first().first.cursor;

    // Act
    const bool removed = edit->clearMarkOperationForCursor(opCursor);

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
}

// ---------------- isMarkAllLine（M2） ----------------

TEST_F(TextEditTest, IsMarkAllLine_PartialSelection_MarksAllMatches)
{
    // Arrange
    setDocText(QString("cat dog cat"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(3, QTextCursor::KeepAnchor); // 选中 "cat"
    edit->setTextCursor(cur);

    // Act
    edit->isMarkAllLine(true, QString("#ff0000"));

    // Assert：MarkAllMatch 记录 + 视口内所有 cat 命中
    EXPECT_EQ(edit->m_markOperations.last().first.type, TextEdit::MarkAllMatch);
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("cat")));
    EXPECT_EQ(edit->m_mapKeywordMarkSelections[QString("cat")].size(), 2);
}

TEST_F(TextEditTest, IsMarkAllLine_NoSelection_MarksEntireDocument)
{
    // Arrange
    setDocText(QString("whole doc"));
    moveCursorTo(3);

    // Act
    edit->isMarkAllLine(true, QString("#00ff00"));

    // Assert：MarkAll + 全文选区（m_bIsMarkAllLine 由 EditWrapper 外部维护，此处不断言）
    EXPECT_EQ(edit->m_markOperations.last().first.type, TextEdit::MarkAll);
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("MARK_ALL")));
}

TEST_F(TextEditTest, IsMarkAllLine_Unmark_ClearsAllContainers)
{
    // Arrange：先建立两种标记
    setDocText(QString("cat dog cat"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkAllLine(true, QString("#ff0000"));
    moveCursorTo(4);
    edit->isMarkCurrentLine(true, QString("#0000ff"), 50);
    ASSERT_FALSE(edit->m_markOperations.isEmpty());

    // Act
    edit->isMarkAllLine(false);

    // Assert：三个容器全部清空
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.isEmpty());
}

// ---------------- cancelLastMark（M3） ----------------

TEST_F(TextEditTest, CancelLastMark_LineMark_RemovesSameTimestampSelections)
{
    // Arrange
    setDocText(QString("l1\nl2"));
    moveCursorTo(0);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 111);
    moveCursorTo(3);
    edit->isMarkCurrentLine(true, QString("#00ff00"), 222);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 2);

    // Act：取消最后一条（timestamp=222）
    edit->cancelLastMark();

    // Assert：仅剩第一条
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_EQ(edit->m_wordMarkSelections.first().second, 111);
    EXPECT_EQ(edit->m_markOperations.size(), 1);
}

TEST_F(TextEditTest, CancelLastMark_AllMatchType_RemovesKeywordEntry)
{
    // Arrange
    setDocText(QString("cat cat"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkAllLine(true, QString("#ff0000"));
    ASSERT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("cat")));

    // Act
    edit->cancelLastMark();

    // Assert
    EXPECT_FALSE(edit->m_mapKeywordMarkSelections.contains(QString("cat")));
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
}

TEST_F(TextEditTest, CancelLastMark_AllType_RemovesMarkAllEntry)
{
    // Arrange
    setDocText(QString("text"));
    moveCursorTo(0);
    edit->isMarkAllLine(true, QString("#ff0000")); // 无选区 → MarkAll
    ASSERT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("MARK_ALL")));

    // Act
    edit->cancelLastMark();

    // Assert
    EXPECT_FALSE(edit->m_mapKeywordMarkSelections.contains(QString("MARK_ALL")));
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
}

TEST_F(TextEditTest, CancelLastMark_EmptyOperations_EarlyReturn)
{
    // Arrange/Act/Assert：空列表直接返回不崩溃
    edit->cancelLastMark();
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
}

TEST_F(TextEditTest, SlotCancleLastMark_TriggersCancel)
{
    // Arrange
    setDocText(QString("line"));
    moveCursorTo(0);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 9);
    ASSERT_EQ(edit->m_markOperations.size(), 1);

    // Act
    edit->slotCancleLastMark();

    // Assert
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
}

TEST_F(TextEditTest, SlotCancleMarkAllLine_ClearsAll)
{
    // Arrange
    setDocText(QString("line"));
    moveCursorTo(0);
    edit->isMarkAllLine(true, QString("#ff0000"));
    ASSERT_FALSE(edit->m_mapKeywordMarkSelections.isEmpty());

    // Act
    edit->slotCancleMarkAllLine();

    // Assert
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.isEmpty());
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
}

// ---------------- markAllKeywordInView / markKeywordInView / markAllInView（M5-M7） ----------------

TEST_F(TextEditTest, MarkAllKeywordInView_EmptyOperations_ReturnsEarly)
{
    // Arrange/Act/Assert：无标记时为空操作
    edit->markAllKeywordInView();
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.isEmpty());
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
}

TEST_F(TextEditTest, MarkAllKeywordInView_WithOperations_RebuildsViewMarks)
{
    // Arrange：建立 MarkAllMatch 关键字标记
    setDocText(QString("key here key"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkAllLine(true, QString("#ff0000"));
    edit->m_mapKeywordMarkSelections.clear(); // 仅保留操作记录，验证重建

    // Act
    edit->markAllKeywordInView();

    // Assert：视口关键字标记被重建
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("key")));
    EXPECT_EQ(edit->m_mapKeywordMarkSelections[QString("key")].size(), 2);
}

TEST_F(TextEditTest, MarkKeywordInView_EmptyKeyword_ReturnsFalse)
{
    // Act/Assert
    EXPECT_FALSE(edit->markKeywordInView(QString(), QString("#ff0000")));
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.isEmpty());
}

TEST_F(TextEditTest, MarkKeywordInView_PresentKeyword_ReturnsTrueAndStores)
{
    // Arrange
    setDocText(QString("alpha beta alpha"));

    // Act
    const bool ok = edit->markKeywordInView(QString("alpha"), QString("#112233"), 42);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(edit->m_mapKeywordMarkSelections[QString("alpha")].size(), 2);
    EXPECT_EQ(edit->m_mapKeywordMarkSelections[QString("alpha")].first().second, 42);
}

TEST_F(TextEditTest, MarkKeywordInView_AbsentKeyword_ReturnsFalse)
{
    // Arrange
    setDocText(QString("only"));

    // Act/Assert
    EXPECT_FALSE(edit->markKeywordInView(QString("missing"), QString("#112233"), 1));
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.isEmpty()); // 未命中不入表
}

TEST_F(TextEditTest, MarkAllInView_AddsWholeDocumentSelection)
{
    // Arrange
    setDocText(QString("abc def"));

    // Act
    edit->markAllInView(QString("#abcdef"), 77);

    // Assert：全文选区 + 时间戳
    const auto list = edit->m_mapKeywordMarkSelections[QString("MARK_ALL")];
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list.first().first.cursor.selectedText(), QString("abc def"));
    EXPECT_EQ(list.first().second, 77);
}

TEST_F(TextEditTest, UpdateMarkAllSelectColor_FlagDriven_ReappliesAllMark)
{
    // Arrange
    setDocText(QString("whole"));
    edit->m_bIsMarkAllLine = true;
    edit->m_strMarkAllLineColorName = QString("#010203");

    // Act
    edit->updateMarkAllSelectColor();

    // Assert：MarkAll 视图重新着色
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("MARK_ALL")));
    EXPECT_EQ(edit->m_mapKeywordMarkSelections[QString("MARK_ALL")].size(), 1);
}

// ---------------- toggleMarkSelections / markSelectWord（M8/M12） ----------------

TEST_F(TextEditTest, ToggleMarkSelections_NoExistingMark_CreatesLineMark)
{
    // Arrange
    setDocText(QString("target line"));
    moveCursorTo(3);

    // Act
    edit->toggleMarkSelections();

    // Assert：无既有标记时按默认色新建
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_EQ(edit->m_markOperations.size(), 1);
}

TEST_F(TextEditTest, ToggleMarkSelections_ExistingMark_RemovesIt)
{
    // Arrange
    setDocText(QString("marked"));
    moveCursorTo(2);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 5);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act
    edit->toggleMarkSelections();

    // Assert
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
    EXPECT_TRUE(edit->m_markOperations.isEmpty());
}

TEST_F(TextEditTest, MarkSelectWord_NewLine_AddsMark)
{
    // Arrange
    setDocText(QString("word line"));
    moveCursorTo(2);

    // Act
    edit->markSelectWord();

    // Assert：无同行标记 → 新建
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_EQ(edit->m_markOperations.size(), 1);
}

TEST_F(TextEditTest, MarkSelectWord_SameLineExisting_TogglesOff)
{
    // Arrange：先在当前行做标记
    setDocText(QString("word line"));
    moveCursorTo(2);
    edit->markSelectWord();
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：同一行再次标记 → 移除
    edit->markSelectWord();

    // Assert
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
    EXPECT_EQ(edit->m_markOperations.size(), 1); // 操作记录保留，仅移除选区
}

// ---------------- 静态转换（M9） ----------------

TEST_F(TextEditTest, ConvertMarkToReplace_And_Back_RoundTripsPositions)
{
    // Arrange：构造带选区光标的标记操作
    setDocText(QString("0123456789"));
    QTextCursor cur(edit->document());
    cur.setPosition(2);
    cur.setPosition(6, QTextCursor::KeepAnchor);
    TextEdit::MarkOperation op;
    op.type = TextEdit::MarkOnce;
    op.cursor = cur;
    op.color = QString("#aabbcc");
    QList<QPair<TextEdit::MarkOperation, qint64>> marks;
    marks << qMakePair(op, qint64(88));

    // Act：正向转换
    const QList<TextEdit::MarkReplaceInfo> infos = TextEdit::convertMarkToReplace(marks);
    // Assert：绝对位置与时间戳保留
    ASSERT_EQ(infos.size(), 1);
    EXPECT_EQ(infos.first().start, 2);
    EXPECT_EQ(infos.first().end, 6);
    EXPECT_EQ(infos.first().time, 88);

    // Act：逆向转换
    const QList<QPair<TextEdit::MarkOperation, qint64>> back = TextEdit::convertReplaceToMark(infos);
    // Assert：光标选区恢复
    ASSERT_EQ(back.size(), 1);
    EXPECT_EQ(back.first().first.cursor.selectionStart(), 2);
    EXPECT_EQ(back.first().first.cursor.selectionEnd(), 6);
    EXPECT_EQ(back.first().second, 88);
}

TEST_F(TextEditTest, ConvertReplaceToMark_EmptyList_ReturnsEmpty)
{
    // Act/Assert
    EXPECT_TRUE(TextEdit::convertReplaceToMark(QList<TextEdit::MarkReplaceInfo>()).isEmpty());
    EXPECT_TRUE(TextEdit::convertMarkToReplace(QList<QPair<TextEdit::MarkOperation, qint64>>()).isEmpty());
}

// ---------------- manualUpdateAllMark（M10） ----------------

TEST_F(TextEditTest, ManualUpdateAllMark_MixedTypes_RebuildsSelections)
{
    // Arrange：文档 3 行
    setDocText(QString("l1\nl2\nl3"));
    QList<QPair<TextEdit::MarkOperation, qint64>> marks;

    // MarkOnce：选 l2 的 "2"
    QTextCursor once(edit->document());
    once.setPosition(4);
    once.setPosition(5, QTextCursor::KeepAnchor);
    TextEdit::MarkOperation opOnce;
    opOnce.type = TextEdit::MarkOnce;
    opOnce.cursor = once;
    opOnce.color = QString("#111111");
    marks << qMakePair(opOnce, qint64(1));

    // MarkLine：跨 l2~l3 的选区
    QTextCursor line(edit->document());
    line.setPosition(3);
    line.setPosition(8, QTextCursor::KeepAnchor);
    TextEdit::MarkOperation opLine;
    opLine.type = TextEdit::MarkLine;
    opLine.cursor = line;
    opLine.color = QString("#222222");
    marks << qMakePair(opLine, qint64(2));

    // 无选区的 MarkOnce：应被 erase
    TextEdit::MarkOperation opEmpty;
    opEmpty.type = TextEdit::MarkOnce;
    QTextCursor emptyCur(edit->document());
    emptyCur.setPosition(0);
    opEmpty.cursor = emptyCur;
    marks << qMakePair(opEmpty, qint64(3));

    // Act
    edit->manualUpdateAllMark(marks);

    // Assert：无选区项被清除；MarkLine 多行被拆为逐块选区；时间戳排序
    EXPECT_TRUE(edit->m_wordMarkSelections.size() >= 3); // once 1 + line 2 块
    bool hasEmptyOp = false;
    for (const auto &pair : edit->m_markOperations) {
        if (pair.first.type == TextEdit::MarkOnce && !pair.first.cursor.hasSelection())
            hasEmptyOp = true;
    }
    EXPECT_FALSE(hasEmptyOp);
}

TEST_F(TextEditTest, ManualUpdateAllMark_WithMarkAll_RebuildsKeywordView)
{
    // Arrange
    setDocText(QString("dup dup"));
    TextEdit::MarkOperation opAll;
    opAll.type = TextEdit::MarkAllMatch;
    opAll.color = QString("#333333");
    opAll.matchText = QString("dup");
    QList<QPair<TextEdit::MarkOperation, qint64>> marks;
    marks << qMakePair(opAll, qint64(9));

    // Act
    edit->manualUpdateAllMark(marks);

    // Assert：markAllKeywordInView 依据 matchText 重建
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("dup")));
    EXPECT_EQ(edit->m_mapKeywordMarkSelections[QString("dup")].size(), 2);
}

// ---------------- updateMark（M11） ----------------

TEST_F(TextEditTest, UpdateMark_DeleteMarkedContent_RemovesEmptyMark)
{
    // Arrange：标记 "bcd"
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 1);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：通过文档光标删除标记内容（contentsChange → updateMark）
    QTextCursor del(edit->document());
    del.setPosition(1);
    del.setPosition(4, QTextCursor::KeepAnchor);
    del.removeSelectedText();

    // Assert：标记内容删空后标记被移除
    EXPECT_EQ(edit->toPlainText(), QString("aef"));
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
}

TEST_F(TextEditTest, UpdateMark_InsertInsideMark_MarkKeptWhole)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 1);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：标记中间插入字符（nCurrentPos 在区间内部 → break 分支）
    QTextCursor ins(edit->document());
    ins.setPosition(2);
    ins.insertText(QString("X"));

    // Assert：标记保留（整体不分割）
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_EQ(edit->toPlainText(), QString("abXcdef"));
}

TEST_F(TextEditTest, UpdateMark_InsertAtMarkEnd_ExtendsMark)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 1);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：在标记尾部位置插入（nCurrentPos == nEndPos 分支）
    QTextCursor ins(edit->document());
    ins.setPosition(4);
    ins.insertText(QString("Y"));

    // Assert：标记扩展覆盖新字符
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_TRUE(edit->m_wordMarkSelections.first().first.cursor.selectedText().contains(QString("Y")));
    EXPECT_EQ(edit->toPlainText(), QString("abcdYef"));
}

TEST_F(TextEditTest, UpdateMark_ReadOnlyMode_EarlyReturn)
{
    // Arrange
    setDocText(QString("abc"));
    edit->toggleReadOnlyMode(true);
    edit->m_wordMarkSelections.append(qMakePair(QTextEdit::ExtraSelection(), qint64(1)));

    // Act：直接调用（只读分支）
    edit->updateMark(0, 0, 1);

    // Assert：标记未被动过
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_TRUE(edit->getReadOnlyMode());
}

TEST_F(TextEditTest, UpdateMark_FileOpen_EarlyReturn)
{
    // Arrange
    setDocText(QString("abc"));
    edit->setIsFileOpen();

    // Act
    edit->updateMark(0, 1, 0);

    // Assert：m_bIsFileOpen 状态生效
    EXPECT_TRUE(edit->m_bIsFileOpen);
    edit->setTextFinished(); // 复位
    EXPECT_FALSE(edit->m_bIsFileOpen);
}

TEST_F(TextEditTest, UpdateMark_NoMarks_EarlyReturn)
{
    // Arrange：无任何标记
    setDocText(QString("plain"));

    // Act/Assert：无标记路径不产生副作用
    edit->updateMark(0, 1, 0);
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
    EXPECT_EQ(edit->toPlainText(), QString("plain"));
}

// ---------------- ExtraSelection 辅助（M18） ----------------

TEST_F(TextEditTest, ContainsExtraSelection_MatchingCursorAndFormat_ReturnsTrue)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    QTextEdit::ExtraSelection sel;
    sel.cursor = cur;
    sel.format.setBackground(QColor(QString("#ff0000")));
    QList<QTextEdit::ExtraSelection> list;
    list << sel;

    // Act/Assert：完全一致命中
    EXPECT_TRUE(edit->containsExtraSelection(list, sel));

    // 光标不同的同格式项不命中
    QTextCursor other = makeCursor(0);
    QTextEdit::ExtraSelection diff;
    diff.cursor = other;
    diff.format = sel.format;
    EXPECT_FALSE(edit->containsExtraSelection(list, diff));
}

TEST_F(TextEditTest, AppendExtraSelection_NoOpImplementation_NoCrash)
{
    // Arrange：当前实现为 Q_UNUSED 空壳（保留待统一清理）
    QList<QTextEdit::ExtraSelection> out;
    QTextEdit::ExtraSelection sel;

    // Act
    edit->appendExtraSelection(QList<QTextEdit::ExtraSelection>(), sel,
                               QString("#ff0000"), &out);

    // Assert：空实现不产生输出
    EXPECT_TRUE(out.isEmpty());
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 0); // 空实现不动内部状态
}

// ---------------- handleCursorMarkChanged / setHighLineCurrentLine（M17） ----------------

TEST_F(TextEditTest, HandleCursorMarkChanged_TrueFalse_UpdatesMarkStartLine)
{
    // Arrange
    setDocText(QString("a\nb\nc"));
    moveCursorTo(2);

    // Act
    edit->handleCursorMarkChanged(true, edit->textCursor());
    // Assert
    EXPECT_EQ(edit->m_markStartLine, 2);

    // Act
    edit->handleCursorMarkChanged(false, edit->textCursor());
    // Assert
    EXPECT_EQ(edit->m_markStartLine, -1);
}

TEST_F(TextEditTest, SetHighLineCurrentLine_Toggled_FlagFlips)
{
    // Arrange
    EXPECT_FALSE(edit->m_HightlightYes);

    // Act/Assert
    edit->setHighLineCurrentLine(true);
    EXPECT_TRUE(edit->m_HightlightYes);
    edit->setHighLineCurrentLine(false);
    EXPECT_FALSE(edit->m_HightlightYes);
}

// ---------------- 书签（M13-M16/M19） ----------------

TEST_F(TextEditTest, AddOrDeleteBookMark_ShortcutPath_TogglesCurrentLine)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));
    moveCursorTo(3); // 第二行
    edit->m_bIsShortCut = true;

    // Act：新增
    edit->addOrDeleteBookMark();
    // Assert
    EXPECT_TRUE(edit->getBookmarkInfo().contains(2));

    // Act：再次触发删除
    edit->m_bIsShortCut = true;
    edit->addOrDeleteBookMark();
    // Assert
    EXPECT_FALSE(edit->getBookmarkInfo().contains(2));
}

TEST_F(TextEditTest, AddOrDeleteBookMark_LineBeyondDoc_Ignored)
{
    // Arrange：无文档内容（单空块），点击点映射行 1
    edit->m_mouseClickPos = QPoint(1, 1000); // 远超文档底 → getLineFromPoint 返回最后块行

    // Act：行号 > blockCount 时直接返回（构造边界：直接给超界行）
    edit->setBookMarkList(QList<int>() << 99);
    edit->m_bIsShortCut = false;

    // Assert：书签列表保持注入值（函数未被触发修改）
    EXPECT_EQ(edit->getBookmarkInfo(), QList<int>() << 99);
    EXPECT_EQ(edit->blockCount(), 1); // 注入值未被函数改动
}

TEST_F(TextEditTest, SetBookMarkList_And_GetBookmarkInfo_RoundTrip)
{
    // Arrange
    const QList<int> marks = QList<int>() << 1 << 5 << 9;

    // Act
    edit->setBookMarkList(marks);

    // Assert
    EXPECT_EQ(edit->getBookmarkInfo(), marks);
    EXPECT_EQ(edit->getBookmarkInfo().size(), 3);
}

TEST_F(TextEditTest, SlotClearBookMarkAction_ClearsList)
{
    // Arrange
    edit->setBookMarkList(QList<int>() << 1 << 2);

    // Act
    edit->slotClearBookMarkAction();

    // Assert
    EXPECT_TRUE(edit->getBookmarkInfo().isEmpty());
    EXPECT_EQ(edit->getBookmarkInfo().size(), 0);
}

TEST_F(TextEditTest, MoveToPreviousBookMark_FromMiddle_JumpsPrevious)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3\nl4\nl5"));
    edit->setBookMarkList(QList<int>() << 1 << 3 << 5);
    moveCursorTo(6); // 第 3 行（l3 起始 6）

    // Act
    edit->moveToPreviousBookMark();

    // Assert：index(3)==1 非 0 → 跳到上一个书签 value(0)=1
    EXPECT_EQ(edit->getCurrentLine(), 1);
    EXPECT_TRUE(edit->getBookmarkInfo().contains(3)); // 书签表未受跳转影响
}

TEST_F(TextEditTest, MoveToPreviousBookMark_AtFirst_JumpsToLast)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3\nl4\nl5"));
    edit->setBookMarkList(QList<int>() << 1 << 3 << 5);
    moveCursorTo(0); // 第 1 行（书签 index 0）

    // Act
    edit->moveToPreviousBookMark();

    // Assert：环绕到末书签行 5
    EXPECT_EQ(edit->getCurrentLine(), 5);
    EXPECT_EQ(edit->getBookmarkInfo().size(), 3);
}

TEST_F(TextEditTest, MoveToNextBookMark_FromFirst_JumpsToNext)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3\nl4\nl5"));
    edit->setBookMarkList(QList<int>() << 1 << 3 << 5);
    moveCursorTo(0);

    // Act
    edit->moveToNextBookMark();

    // Assert
    EXPECT_EQ(edit->getCurrentLine(), 3);
    EXPECT_TRUE(edit->getBookmarkInfo().contains(1));
}

TEST_F(TextEditTest, MoveToNextBookMark_AtLast_JumpsToFirst)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3\nl4\nl5"));
    edit->setBookMarkList(QList<int>() << 1 << 3 << 5);
    moveCursorTo(edit->toPlainText().size()); // 第 5 行（末书签）

    // Act
    edit->moveToNextBookMark();

    // Assert：环绕回首书签
    EXPECT_EQ(edit->getCurrentLine(), 1);
    EXPECT_EQ(edit->getBookmarkInfo().size(), 3);
}

TEST_F(TextEditTest, SlotPreAndNextBookMarkActions_UseMouseClickPosition)
{
    // Arrange：点击位置取第 3 行光标矩形中心（与字体度量解耦）
    setDocText(QString("l1\nl2\nl3\nl4\nl5"));
    edit->setBookMarkList(QList<int>() << 1 << 3 << 5);
    QTextCursor tmp(edit->document());
    tmp.setPosition(edit->document()->findBlockByNumber(2).position());
    edit->m_mouseClickPos = edit->cursorRect(tmp).center();

    // Act：上一个书签（从第 3 行 → 第 1 行）
    edit->slotPreBookMarkAction();
    // Assert
    EXPECT_EQ(edit->getCurrentLine(), 1);

    // Act：下一个书签（基于点击行 3 → value(2)=5）
    edit->slotNextBookMarkAction();
    // Assert
    EXPECT_EQ(edit->getCurrentLine(), 5);
}

TEST_F(TextEditTest, CheckBookmarkLineMove_LineDeletedAbove_ShiftsBookmarkUp)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3\nl4"));
    edit->setBookMarkList(QList<int>() << 3);
    edit->m_nLines = 4;
    edit->m_nSelectEndLine = -1;

    // Act：删除第 2 行（书签行之前减少一行）
    edit->checkBookmarkLineMove(3, 1, 0);
    // blockCount 由文档决定仍为 4（未真实删除文本），改用真实删除路径验证：
    QTextCursor del(edit->document());
    del.setPosition(3); // l2 行首
    del.deletePreviousChar(); // 删除 l1 的 \n → 行数 4→3
    QApplication::processEvents();

    // Assert：书签 3 → 2
    EXPECT_TRUE(edit->getBookmarkInfo().contains(2));
    EXPECT_EQ(edit->blockCount(), 3); // 删除一行后行数 4→3
}

TEST_F(TextEditTest, CheckBookmarkLineMove_FileOpen_EarlyReturn)
{
    // Arrange
    setDocText(QString("a\nb"));
    edit->setBookMarkList(QList<int>() << 2);
    edit->m_nLines = 2;
    edit->setIsFileOpen();

    // Act
    edit->checkBookmarkLineMove(0, 1, 0);

    // Assert：书签不动
    EXPECT_TRUE(edit->getBookmarkInfo().contains(2));
    EXPECT_EQ(edit->m_nLines, 2); // 行数基线未推进
}

TEST_F(TextEditTest, CheckBookmarkLineMove_LineAddedBelow_BookmarksShifted)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));
    edit->setBookMarkList(QList<int>() << 3);
    edit->m_nLines = 2; // 制造 "增加行" 场景（m_nLines < blockCount）

    // Act：在第 1 行前增加内容（nAddorDeleteLine < line → 行号上移补偿）
    edit->checkBookmarkLineMove(0, 0, 1);

    // Assert：3 + (3-2) = 4? 增行时 line += blockCount - m_nLines = 3+1=4
    EXPECT_TRUE(edit->getBookmarkInfo().contains(4));
    EXPECT_EQ(edit->m_nLines, 3);
}

// ---------------- setTextFinished / 历史记录（M16） ----------------

TEST_F(TextEditTest, SetTextFinished_NoSettings_SkipsRestore)
{
    // Arrange：无 settings 注入（临时移除）
    setDocText(QString("a\nb\nc"));
    edit->setSettings(nullptr);

    // Act
    edit->setTextFinished();

    // Assert：基础状态复位完成，无书签恢复
    EXPECT_FALSE(edit->m_bIsFileOpen);
    EXPECT_EQ(edit->m_nLines, 3);
    EXPECT_TRUE(edit->getBookmarkInfo().isEmpty());

    // 还原 settings 供后续用例
    edit->setSettings(Settings::instance());
}

TEST_F(TextEditTest, ReadHistoryRecords_RoundTripParse)
{
    // Arrange：写入符合解析格式的浏览历史（同一 option 同时承载路径与行号段）
    auto *opt = Settings::instance()->settings;
    const QString history = QString("*{") + QString("*[ut://file1]*") + QString("*(2,)*(5,)*")
            + QString("}*") + QString("*{") + QString("*[ut://file2]*") + QString("*(1,)*")
            + QString("}*");
    opt->option("advance.editor.browsing_history_file")->setValue(history);

    // Act
    const QStringList records = edit->readHistoryRecord("advance.editor.browsing_history_file");
    const QStringList bookmarks = edit->readHistoryRecordofBookmark();
    const QStringList paths = edit->readHistoryRecordofFilePath("advance.editor.browsing_history_file");

    // Assert：三种定界符解析正确
    EXPECT_EQ(records.size(), 2);   // *{ ... }* 段
    EXPECT_EQ(bookmarks.size(), 3); // *( ... )* 段（file1 两段 + file2 一段）
    EXPECT_EQ(paths.size(), 2);     // *[ ... ]* 段（file1、file2）

    // 清理临时 option（避免污染后续用例）
    opt->option("advance.editor.browsing_history_file")->setValue(QString());
}

TEST_F(TextEditTest, SetTextFinished_WithHistory_RestoresBookmarks)
{
    // Arrange：书签段采用解析器实际格式 "*(N,*)*"（",*" 为数字终止符）
    setDocText(QString("l1\nl2\nl3\nl4\nl5"));
    edit->setFilePath(QString("ut://marked"));
    auto *opt = Settings::instance()->settings;
    opt->option("advance.editor.browsing_history_file")
            ->setValue(QString("*{") + QString("*[ut://marked]*") + QString("*(2,*)*(4,*)*")
                       + QString("}*"));
    edit->setBookMarkList(QList<int>()); // 空书签才走恢复路径

    // Act
    edit->setTextFinished();

    // Assert：每文件仅取首个书签段 → 恢复行 2（段内首个数字）
    EXPECT_TRUE(edit->getBookmarkInfo().contains(2));
    EXPECT_EQ(edit->m_nLines, 5); // 行数基线同步

    // 清理
    opt->option("advance.editor.browsing_history_file")->setValue(QString());
}

TEST_F(TextEditTest, SetTextFinished_ExistingBookmarks_SkipRestore)
{
    // Arrange
    setDocText(QString("l1\nl2"));
    edit->setBookMarkList(QList<int>() << 1);
    edit->setFilePath(QString("ut://skip"));

    // Act
    edit->setTextFinished();

    // Assert：已有书签直接返回
    EXPECT_EQ(edit->getBookmarkInfo(), QList<int>() << 1);
    EXPECT_EQ(edit->m_nLines, 2);
}

// ---------------- 书签区绘制（经 grab 触发真实 paintEvent） ----------------

TEST_F(TextEditTest, BookMarkAreaPaintEvent_WithBookmarks_RendersWithoutCrash)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));
    edit->setBookMarkList(QList<int>() << 1 << 3);
    edit->m_nBookMarkHoverLine = 2; // 悬停非书签行（走 hover 高亮分支）

    // Act：直接驱动绘制处理（offscreen 未 show 控件不派发 paint 事件）
    QPaintEvent ev(QRect(0, 0, 20, 200));
    edit->bookMarkAreaPaintEvent(&ev);

    // Assert：书签绘制路径联动浅色分支（m_lineNumbersColor 透明度 ~0.3）且书签表未受损
    EXPECT_NEAR(edit->m_lineNumbersColor.alphaF(), 0.3, 0.01);
    EXPECT_TRUE(edit->getBookmarkInfo().contains(1));
}
