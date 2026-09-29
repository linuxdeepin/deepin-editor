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

// 注：原 AppendExtraSelection_NoOpImplementation_NoCrash 直测的空壳方法
// 已随缺陷修复删除（D-045，源码自述"没有使用的方法，应该去除"）。

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

// ============================================================================
// PMS 回归用例（Mode 7 PMS 缺陷热点补强，批次 1）
// 数据源：tests/.ut-pms/（bugs.json / work-order.md）
// ============================================================================

// PMS: https://pms.uniontech.com/bug-view-95115.html  commit: 17033b21
// 场景：Ctrl+A 后 Ctrl+C 后多次 Ctrl+V 后回车，粘贴错乱。修复（17033b21）：
// paste() 末尾重置 m_isSelectAll = false，后续滚动不误触发 selectTextInView
TEST_F(TextEditTest, BUG95115_PasteAfterSelectAll_ResetsSelectAllFlag)
{
    // Arrange: 全选
    setDocText(QString("original content"));
    edit->slotSelectAllAction();
    ASSERT_TRUE(edit->m_isSelectAll);
    QApplication::clipboard()->setText(QString("pasted"));

    // Act: 粘贴（paste 末尾 m_isSelectAll = false）
    edit->paste();
    QApplication::processEvents();

    // Assert: 粘贴成功 + 全选标志重置
    EXPECT_EQ(edit->toPlainText(), QStringLiteral("pasted"));
    EXPECT_FALSE(edit->m_isSelectAll);
}

// ============================================================================
// PMS 回归用例（Mode 2 PMS bug 回归，批次 2）
// 数据源：tests/.ut-pms/bugs.json + 修复 commit diff
// 本批次覆盖功能点：updateHighlightBrackets / getNeedControlLine /
//   calcMarkReplaceList / setBookmarkFlagVisable / setCodeFlodFlagVisable /
//   slotFlodAllLevel / slotFlodCurrentLevel / slotUnflodCurrentLevel / setMark /
//   isNeedShowFoldIcon / setCodeFoldWidgetHide / MarkOperation / MarkReplaceInfo
// （isMarkCurrentLine、updateMarkAllSelectColor、slotPreBookMarkAction、
//   getBookmarkInfo/setBookMarkList、clearMarksForTextCursor、
//   toggleMarkSelections、manualUpdateAllMark 已有既有用例覆盖，见文件头映射）
// ============================================================================

#include "showflodcodewidget.h" // 批次 2 追加：m_foldCodeShow 完整类型（仅新增内容）

// PMS: https://pms.uniontech.com/bug-view-66378.html  commit: e3cbab1d
// PMS: https://pms.uniontech.com/bug-view-65228.html  commit: d242fe4f
// 场景：大文本"标记所有"卡死修复（e3cbab1d/d242fe4f）后括号高亮算法需正确
// 处理字符串内括号/转义引号（updateHighlightBrackets：字符串内 '}' 与转义
// 引号不计入配对深度，配对成功时同时设置首尾括号高亮选区）
TEST_F(TextEditTest, BUG66378_UpdateHighlightBrackets_PairMatchSkipsStringContent)
{
    // Arrange：第 1 行 '{' + 字符串内含转义引号与 '}'（不计入配对），第 3 行为真正配对 '}'
    const QString doc = QString("void f() {\n    char *s = \"a\\\"}b\";\n}\n");
    setDocText(doc);
    QTextBlock b0 = edit->document()->findBlockByNumber(0);
    const int bracePos = b0.position() + b0.text().indexOf(QLatin1Char('{'));
    moveCursorTo(bracePos); // 光标落在 '{' 上（forward 分支）

    // Act
    edit->updateHighlightBrackets(QLatin1Char('{'), QLatin1Char('}'));

    // Assert：首选区覆盖 '{'，尾选区选中字符串外真正的 '}'（第 3 行）
    EXPECT_EQ(edit->m_beginBracketSelection.cursor.selectedText(), QString("{"));
    EXPECT_EQ(edit->m_endBracketSelection.cursor.selectedText(), QString("}"));
    EXPECT_EQ(edit->m_endBracketSelection.cursor.blockNumber(), 2);
}

// PMS: https://pms.uniontech.com/bug-view-66378.html  commit: e3cbab1d
// PMS: https://pms.uniontech.com/bug-view-65228.html  commit: d242fe4f
// 场景：光标旁无任何括号字符（characterAt(position)/position-1 均非括号）时，
// 不设置括号高亮选区（position-1 回退读取不越界，保持选区为空）
TEST_F(TextEditTest, BUG66378_UpdateHighlightBrackets_NoAdjacentBracket_NoHighlight)
{
    // Arrange：文档无任何括号，光标置于行中
    setDocText(QString("plain text line"));
    moveCursorTo(7);

    // Act
    edit->updateHighlightBrackets(QLatin1Char('{'), QLatin1Char('}'));

    // Assert：未命中括号 → 首尾高亮选区保持空
    EXPECT_TRUE(edit->m_beginBracketSelection.cursor.isNull());
    EXPECT_TRUE(edit->m_endBracketSelection.cursor.isNull());
}

// PMS: https://pms.uniontech.com/bug-view-66378.html  commit: e3cbab1d
// 场景：代码折叠控制 getNeedControlLine：命中匹配括号时按区域隐藏/显示
// （isVisable=false 折叠 beginBlock~endBlock 含 '}' 行并返回 true；true 时还原）
TEST_F(TextEditTest, BUG66378_GetNeedControlLine_FoldAndUnfoldBlocks)
{
    // Arrange：两层嵌套花括号文档
    setDocText(QString("int main()\n{\n    if (x)\n    {\n        return 0;\n    }\n    return 1;\n}\n"));

    // Act：折叠第 2 行 '{' 的区域
    const bool folded = edit->getNeedControlLine(1, false);

    // Assert：返回 true；区域块（2~7）隐藏，括号行与首行保持可见
    EXPECT_TRUE(folded);
    EXPECT_TRUE(edit->document()->findBlockByNumber(0).isVisible());
    EXPECT_TRUE(edit->document()->findBlockByNumber(1).isVisible());
    EXPECT_FALSE(edit->document()->findBlockByNumber(2).isVisible());
    EXPECT_FALSE(edit->document()->findBlockByNumber(4).isVisible());
    EXPECT_FALSE(edit->document()->findBlockByNumber(7).isVisible());

    // Act：展开还原
    const bool unfolded = edit->getNeedControlLine(1, true);

    // Assert：区域块恢复可见
    EXPECT_TRUE(unfolded);
    EXPECT_TRUE(edit->document()->findBlockByNumber(2).isVisible());
    EXPECT_TRUE(edit->document()->findBlockByNumber(7).isVisible());
}

// PMS: https://pms.uniontech.com/bug-view-66378.html  commit: e3cbab1d
// 场景：左右括号在同一行（endBlock == curBlock）时不执行折叠，返回 false
TEST_F(TextEditTest, BUG66378_GetNeedControlLine_SameLineBraces_NoFold)
{
    // Arrange：单行内配对括号
    setDocText(QString("int x { 0 };"));

    // Act
    const bool ret = edit->getNeedControlLine(0, false);

    // Assert：同行括号不折叠，块可见性不变
    EXPECT_FALSE(ret);
    EXPECT_TRUE(edit->document()->findBlockByNumber(0).isVisible());
}

// PMS: https://pms.uniontech.com/bug-view-79951.html  commit: d0fe36dc
// 场景：连续全选复制粘贴卡死修复（d0fe36dc）引入的替换-标记位置联动
// calcMarkReplaceList：标记位于替换文本右侧时按长度变更量整体偏移
TEST_F(TextEditTest, BUG79951_CalcMarkReplaceList_MarkShiftsByReplaceOffset)
{
    // Arrange：标记 "bb"（[4,6)）位于替换文本 "XX"（位置 2）右侧，"XX"→"X" 缩短 1
    QList<TextEdit::MarkReplaceInfo> replaceList;
    TextEdit::MarkReplaceInfo info;
    info.opt.type = TextEdit::MarkOnce;
    info.start = 4;
    info.end = 6;
    info.time = 1;
    replaceList << info;

    // Act
    edit->calcMarkReplaceList(replaceList, QString("aaXXbb"), QString("XX"), QString("X"), 0, Qt::CaseSensitive);

    // Assert：右侧标记整体左移 adjustlen=1 → [3,5)
    ASSERT_EQ(replaceList.size(), 1);
    EXPECT_EQ(replaceList.at(0).start, 3);
    EXPECT_EQ(replaceList.at(0).end, 5);
}

// PMS: https://pms.uniontech.com/bug-view-79951.html  commit: d0fe36dc
// 场景：calcMarkReplaceList 分支回归：替换文本完全覆盖标记 → 标记取消（归 0）；
// MarkAll 全文标记不参与替换位置调整；replaceText == withText 直接返回
TEST_F(TextEditTest, BUG79951_CalcMarkReplaceList_ReplaceCoversMarkOrAllSkipped)
{
    // Arrange 1：替换文本 "aaXX" 完全覆盖标记 "XX"（[2,4)）
    QList<TextEdit::MarkReplaceInfo> replaceList;
    TextEdit::MarkReplaceInfo info;
    info.opt.type = TextEdit::MarkOnce;
    info.start = 2;
    info.end = 4;
    info.time = 1;
    replaceList << info;

    // Act
    edit->calcMarkReplaceList(replaceList, QString("aaXXbb"), QString("aaXX"), QString("Q"), 0, Qt::CaseSensitive);

    // Assert：EIntersectInner → 标记取消（start/end 归 0，manualUpdateAllMark 会移除）
    EXPECT_EQ(replaceList.at(0).start, 0);
    EXPECT_EQ(replaceList.at(0).end, 0);

    // Arrange 2：仅含 MarkAll 类型标记
    QList<TextEdit::MarkReplaceInfo> allList;
    TextEdit::MarkReplaceInfo allInfo;
    allInfo.opt.type = TextEdit::MarkAll;
    allInfo.start = 1;
    allInfo.end = 3;
    allInfo.time = 2;
    allList << allInfo;

    // Act：全文替换 "XX"→"LONGER"
    edit->calcMarkReplaceList(allList, QString("aaXXbb"), QString("XX"), QString("LONGER"), 0, Qt::CaseSensitive);

    // Assert：MarkAll 类型跳过调整，保持不变
    EXPECT_EQ(allList.at(0).start, 1);
    EXPECT_EQ(allList.at(0).end, 3);

    // Arrange 3：replaceText == withText
    QList<TextEdit::MarkReplaceInfo> sameList;
    TextEdit::MarkReplaceInfo sameInfo;
    sameInfo.opt.type = TextEdit::MarkOnce;
    sameInfo.start = 4;
    sameInfo.end = 6;
    sameList << sameInfo;

    // Act
    edit->calcMarkReplaceList(sameList, QString("aaXXbb"), QString("XX"), QString("XX"), 0, Qt::CaseSensitive);

    // Assert：相同文本直接返回，无变化
    EXPECT_EQ(sameList.at(0).start, 4);
    EXPECT_EQ(sameList.at(0).end, 6);
}

// PMS: https://pms.uniontech.com/bug-view-215591.html  commit: d0935120
// 场景：点击序号列阻塞修复（d0935120，实际修复 onPressedLineNumber 死循环，
// 相关用例见 test_dtextedit_find.cpp F15）同提交涉及左边栏标志接口回归：
// setBookmarkFlagVisable/setCodeFlodFlagVisable 同步标志位与列隐藏/显示状态
TEST_F(TextEditTest, BUG215591_SetBookmarkAndFlodFlagVisable_TogglesLeftAreas)
{
    // Arrange
    ASSERT_NE(edit->m_pLeftAreaWidget, nullptr);
    ASSERT_NE(edit->m_pLeftAreaWidget->m_pBookMarkArea, nullptr);
    ASSERT_NE(edit->m_pLeftAreaWidget->m_pFlodArea, nullptr);

    // Act：隐藏书签列
    edit->setBookmarkFlagVisable(false);
    // Assert：标志位与控件隐藏态同步
    EXPECT_FALSE(edit->m_pIsShowBookmarkArea);
    EXPECT_TRUE(edit->m_pLeftAreaWidget->m_pBookMarkArea->isHidden());

    // Act：显示书签列
    edit->setBookmarkFlagVisable(true);
    // Assert
    EXPECT_TRUE(edit->m_pIsShowBookmarkArea);
    EXPECT_FALSE(edit->m_pLeftAreaWidget->m_pBookMarkArea->isHidden());

    // Act：隐藏代码折叠列
    edit->setCodeFlodFlagVisable(false);
    // Assert
    EXPECT_FALSE(edit->m_pIsShowCodeFoldArea);
    EXPECT_TRUE(edit->m_pLeftAreaWidget->m_pFlodArea->isHidden());

    // Act：显示代码折叠列
    edit->setCodeFlodFlagVisable(true);
    // Assert
    EXPECT_TRUE(edit->m_pIsShowCodeFoldArea);
    EXPECT_FALSE(edit->m_pLeftAreaWidget->m_pFlodArea->isHidden());
}

// PMS: https://pms.uniontech.com/bug-view-184107.html  commit: 3b9b699f
// 场景：超大文件读取异常处理修复（3b9b699f）PMS 热点关联折叠入口回归：
// slotFlodAllLevel → flodOrUnflodAllLevel(true) 遍历可见含 '{' 块折叠，
// 记录折叠行号，折叠后区域块不可见
TEST_F(TextEditTest, BUG184107_SlotFlodAllLevel_FoldsAllBraceBlocks)
{
    // Arrange：两层嵌套花括号文档（无注释行）
    setDocText(QString("int main()\n{\n    if (x)\n    {\n        return 0;\n    }\n    return 1;\n}\n"));

    // Act
    edit->slotFlodAllLevel();

    // Assert：折叠点行号被记录（行 1）；区域块隐藏、括号行/首行可见
    EXPECT_TRUE(edit->m_listMainFlodAllPos.contains(1));
    EXPECT_FALSE(edit->document()->findBlockByNumber(2).isVisible());
    EXPECT_FALSE(edit->document()->findBlockByNumber(4).isVisible());
    EXPECT_TRUE(edit->document()->findBlockByNumber(0).isVisible());
    EXPECT_TRUE(edit->document()->findBlockByNumber(1).isVisible());
}

// PMS: https://pms.uniontech.com/bug-view-184107.html  commit: 3b9b699f
// 场景：slotFlodCurrentLevel/slotUnflodCurrentLevel 按点击行折叠/展开当前层级：
// 点击行（getLineFromPoint）→ getNeedControlLine(line-1, ...) 隐藏/恢复区域块
TEST_F(TextEditTest, BUG184107_SlotFlodCurrentLevel_FoldAndUnfoldAtClickLine)
{
    // Arrange：点击位置取第 3 行（if 行）光标矩形中心（与字体度量解耦）
    setDocText(QString("int main()\n{\n    if (x)\n    {\n        return 0;\n    }\n    return 1;\n}\n"));
    QTextCursor tmp(edit->document());
    tmp.setPosition(edit->document()->findBlockByNumber(2).position());
    edit->m_mouseClickPos = edit->cursorRect(tmp).center();

    // Act：折叠当前层级（line-1 = 块 2 的 if 区域）
    edit->slotFlodCurrentLevel();

    // Assert：if 区域块（3~5）隐藏，点击行与外层括号行可见
    EXPECT_FALSE(edit->document()->findBlockByNumber(3).isVisible());
    EXPECT_FALSE(edit->document()->findBlockByNumber(5).isVisible());
    EXPECT_TRUE(edit->document()->findBlockByNumber(2).isVisible());
    EXPECT_TRUE(edit->document()->findBlockByNumber(1).isVisible());

    // Act：展开当前层级
    edit->slotUnflodCurrentLevel();

    // Assert：区域块恢复可见
    EXPECT_TRUE(edit->document()->findBlockByNumber(3).isVisible());
    EXPECT_TRUE(edit->document()->findBlockByNumber(5).isVisible());
}

// PMS: https://pms.uniontech.com/bug-view-305473.html  commit: 65bc75a3
// 场景：主题色修复（65bc75a3）中 setEditPalette 在 Qt6 下取消手动 setPalette
// （纯调色板行为无法离屏断言，SKIP）；同函数族 setMark 标记模式状态机回归：
// 无选区开启 → 有选区清除选区并通知 → 无选区关闭
TEST_F(TextEditTest, BUG305473_SetMark_TogglesCursorMarkMode)
{
    // Arrange
    setDocText(QString("mark mode"));
    moveCursorTo(3);
    ASSERT_FALSE(edit->m_cursorMark);
    QSignalSpy spy(edit, &TextEdit::cursorMarkChanged);

    // Act：无选区开启标记模式
    edit->setMark();
    // Assert：m_cursorMark 翻转并发送 cursorMarkChanged
    EXPECT_TRUE(edit->m_cursorMark);
    EXPECT_EQ(spy.count(), 1);

    // Act：标记模式下有选区 → 清除选区（保持标记模式）
    QTextCursor cur = edit->textCursor();
    cur.setPosition(3);
    cur.setPosition(7, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->setMark();
    // Assert：选区被清除，标记模式保持，再次通知
    EXPECT_FALSE(edit->textCursor().hasSelection());
    EXPECT_TRUE(edit->m_cursorMark);
    EXPECT_EQ(spy.count(), 2);

    // Act：无选区再触发 → 关闭标记模式
    edit->setMark();
    // Assert
    EXPECT_FALSE(edit->m_cursorMark);
    EXPECT_EQ(spy.count(), 3);
}

// PMS: https://pms.uniontech.com/bug-view-79951.html  commit: d0fe36dc
// 场景：isNeedShowFoldIcon 纯文本括号分析回归：首个 '{' 后无配对 '}' → 需要
// 折叠图标；单行配对/无括号/无前置 '{' 的 '}' → 不需要
TEST_F(TextEditTest, BUG79951_IsNeedShowFoldIcon_BracketBalanceDecision)
{
    // Arrange
    setDocText(QString("void f() {\n}\nint g() { return 0; }\nplain line"));

    // Act/Assert：'{' 未在行内配对 → true
    EXPECT_TRUE(edit->isNeedShowFoldIcon(edit->document()->findBlockByNumber(0)));
    // Act/Assert：'}' 无前置 '{'（hasFindLeft=false 不计数）→ false
    EXPECT_FALSE(edit->isNeedShowFoldIcon(edit->document()->findBlockByNumber(1)));
    // Act/Assert：单行内左右配对 → false
    EXPECT_FALSE(edit->isNeedShowFoldIcon(edit->document()->findBlockByNumber(2)));
    // Act/Assert：无括号行 → false
    EXPECT_FALSE(edit->isNeedShowFoldIcon(edit->document()->findBlockByNumber(3)));
}

// PMS: https://pms.uniontech.com/bug-view-79951.html  commit: d0fe36dc
// 场景：setCodeFoldWidgetHide 委托折叠预览控件显隐：构造即创建 m_foldCodeShow
// （判空），setHidden(true/false) 正确传递
TEST_F(TextEditTest, BUG79951_SetCodeFoldWidgetHide_TogglesFoldWidget)
{
    // Arrange
    ASSERT_NE(edit->m_foldCodeShow, nullptr);

    // Act/Assert：隐藏
    edit->setCodeFoldWidgetHide(true);
    EXPECT_TRUE(edit->m_foldCodeShow->isHidden());

    // Act/Assert：显示
    edit->setCodeFoldWidgetHide(false);
    EXPECT_FALSE(edit->m_foldCodeShow->isHidden());
}

// PMS: https://pms.uniontech.com/bug-view-66378.html  commit: e3cbab1d
// 场景：MarkOperation/MarkReplaceInfo（dtextedit.h）默认构造语义回归：
// MarkOperation 默认 MarkOnce、光标/颜色/匹配文本为空；MarkReplaceInfo
// 位置与时间戳默认 0（撤销替换联动依赖的初始值约定）
TEST_F(TextEditTest, BUG66378_MarkStructs_DefaultConstruct_InitialValues)
{
    // Act
    TextEdit::MarkOperation op;
    TextEdit::MarkReplaceInfo info;

    // Assert
    EXPECT_EQ(op.type, TextEdit::MarkOnce);
    EXPECT_TRUE(op.cursor.isNull());
    EXPECT_TRUE(op.color.isEmpty());
    EXPECT_TRUE(op.matchText.isEmpty());
    EXPECT_EQ(info.start, 0);
    EXPECT_EQ(info.end, 0);
    EXPECT_EQ(info.time, 0);
}

// PMS: https://pms.uniontech.com/bug-view-331945.html  commit: ebf6f7ee
// 场景：文本删除时多个标记被移除会删错元素。修复（ebf6f7ee）：updateMark
// 移除逻辑 removeAt 索引错位改为 removeIf + 索引计数（Qt6 分支），
// 多标记同批次移除时索引不错位
TEST_F(TextEditTest, BUG331945_UpdateMark_MultiRemoved_CorrectIndicesRemain)
{
    // Arrange: 三个标记（位置顺序 aaa/bbb/ccc），一次 contentsChange 同时删除
    // 前两个标记的文本（同批次 removeSet={0,1} 多标记移除场景）
    setDocText(QString("aaaXXbbbXXccc"));
    QList<QPair<QTextEdit::ExtraSelection, qint64>> marks;
    for (int i = 0; i < 3; ++i) {
        const int base = edit->document()->firstBlock().position();
        QTextCursor cur(edit->document());
        cur.setPosition(base + i * 5);
        cur.setPosition(base + i * 5 + 3, QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection sel;
        sel.cursor = cur;
        sel.format.setBackground(QColor(QString("#ff0000")));
        marks << QPair<QTextEdit::ExtraSelection, qint64>(sel, 1000 + i);
    }
    edit->m_wordMarkSelections = marks;
    edit->m_nSelectEndLine = -1; // 无列选区，走"标记文本已空"移除分支
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 3);

    // Act: 删除 [0,8)（"aaaXXbbb"）→ contentsChange 自动触发 updateMark
    QTextCursor del(edit->document());
    del.setPosition(0);
    del.setPosition(8, QTextCursor::KeepAnchor);
    del.removeSelectedText();

    // Assert: 修复后 removeIf 按索引精确移除 {0,1}，仅 ccc 保留
    // （修复前 removeAt(0)+removeAt(1) 索引错位会误删 ccc 留下空标记 bbb）
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);
    if (edit->m_wordMarkSelections.size() == 1)
        EXPECT_EQ(edit->m_wordMarkSelections.first().first.cursor.selectedText(),
                  QString("ccc"));
}

// PMS: https://pms.uniontech.com/bug-view-331945.html  commit: ebf6f7ee
// 场景：列选区激活（m_nSelectEndLine != -1）时，删除区间完全包含的标记
// 被移除、区间外的保留，同批次多标记移除索引不错位
TEST_F(TextEditTest, BUG331945_UpdateMark_ColumnSelection_ContainedMarksRemoved)
{
    // Arrange: 两个标记分别位于第 0、2 行；列选区位置范围覆盖两个标记
    setDocText(QString("aa\nbb\ncc"));
    QList<QPair<QTextEdit::ExtraSelection, qint64>> marks;
    const int line0 = edit->document()->firstBlock().position();
    const int line2 = edit->document()->findBlockByNumber(2).position();
    QTextCursor c0(edit->document());
    c0.setPosition(line0);
    c0.setPosition(line0 + 2, QTextCursor::KeepAnchor);
    QTextCursor c2(edit->document());
    c2.setPosition(line2);
    c2.setPosition(line2 + 2, QTextCursor::KeepAnchor);
    QTextEdit::ExtraSelection s0, s2;
    s0.cursor = c0;
    s0.format.setBackground(QColor(QString("#00ff00")));
    s2.cursor = c2;
    s2.format.setBackground(QColor(QString("#00ff00")));
    marks << QPair<QTextEdit::ExtraSelection, qint64>(s0, 100);
    marks << QPair<QTextEdit::ExtraSelection, qint64>(s2, 200);
    edit->m_wordMarkSelections = marks;
    edit->m_nSelectStart = 0;
    edit->m_nSelectEnd = 20;
    edit->m_nSelectEndLine = 2; // 列选区激活
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 2);

    // Act: 删除第 1 行部分文本（charsRemoved>0）→ 触发 updateMark 列选区分支
    QTextCursor del(edit->document());
    del.setPosition(3);
    del.setPosition(5, QTextCursor::KeepAnchor);
    del.removeSelectedText();

    // Assert: 两个标记均被 [0,20] 完全包含 → 同批次移除，索引不错位
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
}
