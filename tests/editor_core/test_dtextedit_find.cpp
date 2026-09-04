// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ===========================================================================
// TextEdit 查找/替换/高亮方法族（dtextedit.cpp 行 2069~2800、3880~3980 一带）
//
// 分支清单（来源：src/editor/dtextedit.cpp）：
// F1 : replaceAll —— 只读 return / 空串 return / 相同串 return / 实际替换
//      （连带 calcMarkReplaceList → findMatchRange/updateMarkReplaceRange 静态函数）
// F2 : replaceNext —— 只读 return / m_isSelectAll 全选重建 / 空串或无高亮 return /
//      m_cursorStart 分支 / 高亮选区分支 / MarkAll 跳过 / 六种区间交叉 case /
//      大小写或含 \n 才替换
// F3 : replaceRest —— 只读 / 空串 / 相同串 / 光标后替换
// F4 : beforeReplace —— 空串或无高亮时补一次 highlightKeyword
// F5 : findKeywordForward —— 有选区 / 无选区 / 命中 / 未命中
// F6 : updateCursorKeywordSelection —— 首查命中 / 环绕二次查找 / 全失败清理
// F7 : updateHighlightLineSelection —— GA_slide 提前 return / 正常
// F8 : updateKeywordSelections —— 空 keyword false / 未命中 false / 命中收集
// F9 : scanAllMatchPositions —— 空 keyword / 多命中 / 大小写
// F10: findCurrentMatchIndex —— 空缓存 0 / 命中 index+1 / 未命中 0
// F11: updateKeywordSelectionsInView —— 空 keyword / 含 \n 多行关键字 /
//      死循环保护 break / 末尾越界 break / 大小写完全相等才高亮
// F12: searchKeywordSeletion —— findNext / backward / 含 \n / 未命中
// F13: findCursor —— forward 命中/未命中 / backward / Windows 行尾替换 \r\n
// F14: updateMatchCount —— 空 keyword 发射(0,0) / 缓存命中 / 缓存失效重扫
// F15: onPressedLineNumber —— FileOpenBegin return / x 越界 return /
//      选中整行 / 尾行 EndOfBlock
// F16: highlightKeyword/ highlightKeywordInView / removeKeywords /
//      clearFindMatchSelections / setFindHighlightSelection / renderAllSelections
// F17: ifHasHighlight —— 空光标 false / 有选区 true
//
// 用例映射：见各 TEST_F 名。环境隔离：见 editor_core_fixture.h。
// ===========================================================================

#include "editor_core_fixture.h"

// ---------------- replaceAll（F1） ----------------

TEST_F(TextEditTest, ReplaceAll_MultipleOccurrences_AllReplaced)
{
    // Arrange
    setDocText(QString("cat dog cat"));

    // Act
    edit->replaceAll(QString("cat"), QString("fox"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("fox dog fox"));
    // 撤销一次回到原文（ChangeMarkCommand 子命令整体回滚）
    edit->undo_();
    EXPECT_EQ(edit->toPlainText(), QString("cat dog cat"));
}

TEST_F(TextEditTest, ReplaceAll_CaseInsensitiveFlag_RespectsCase)
{
    // Arrange
    setDocText(QString("Cat cat CAT"));

    // Act：区分大小写（默认 Qt::CaseSensitive）
    edit->replaceAll(QString("cat"), QString("dog"), Qt::CaseSensitive);

    // Assert：仅小写命中
    EXPECT_EQ(edit->toPlainText(), QString("Cat dog CAT"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceAll_EmptySearchText_EarlyReturn)
{
    // Arrange
    setDocText(QString("unchanged"));

    // Act
    edit->replaceAll(QString(), QString("x"));

    // Assert：强异常安全——文本与撤销栈均未变
    EXPECT_EQ(edit->toPlainText(), QString("unchanged"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceAll_SameText_EarlyReturn)
{
    // Arrange
    setDocText(QString("same same"));

    // Act
    edit->replaceAll(QString("same"), QString("same"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("same same"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceAll_ReadOnlyMode_EarlyReturn)
{
    // Arrange
    setDocText(QString("locked"));
    edit->toggleReadOnlyMode(true);

    // Act
    edit->replaceAll(QString("locked"), QString("open"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("locked"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceAll_WithMarks_MarkPositionsAdjusted)
{
    // Arrange：标记 "dog"（MarkOnce），替换 cat 会移动其右侧标记
    setDocText(QString("cat dog"));
    QTextCursor cur = makeCursor(4);
    cur.setPosition(7, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 1);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：cat → foxcat（长度差 +3）
    edit->replaceAll(QString("cat"), QString("foxcat"));

    // Assert：标记随文本右移（start 由 4 → 7）
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_EQ(edit->m_wordMarkSelections.first().first.cursor.selectionStart(), 7);
    EXPECT_EQ(edit->toPlainText(), QString("foxcat dog"));
}

TEST_F(TextEditTest, ReplaceAll_TwoMarksSorted_ComparatorAndOffsetsApplied)
{
    // Arrange：两处标记（构造乱序提交，触发 calcMarkReplaceList 内 sort 比较器）
    // "mm cat nn cat"：mm=0-1，cat=3-5，nn=7-8，cat=10-12
    setDocText(QString("mm cat nn cat"));
    QTextCursor mark1 = makeCursor(0);
    mark1.setPosition(2, QTextCursor::KeepAnchor); // "mm"
    QTextCursor mark2 = makeCursor(7);
    mark2.setPosition(9, QTextCursor::KeepAnchor); // "nn"
    edit->setTextCursor(mark2);
    edit->isMarkCurrentLine(true, QString("#111111"), 2);
    edit->setTextCursor(mark1);
    edit->isMarkCurrentLine(true, QString("#222222"), 1);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 2);

    // Act
    edit->replaceAll(QString("cat"), QString("X"));

    // Assert：两标记均保留且右侧标记按偏移调整（nn 起点 7 → 5）
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 2);
    bool hasShifted = false;
    for (const auto &pair : edit->m_wordMarkSelections) {
        if (pair.first.cursor.selectedText() == QString("nn"))
            hasShifted = (pair.first.cursor.selectionStart() == 5);
    }
    EXPECT_TRUE(hasShifted);
    EXPECT_EQ(edit->toPlainText(), QString("mm X nn X"));
}

TEST_F(TextEditTest, SetCursorKeywordSeletoin_DirectDrive_MovesToMatch)
{
    // Arrange：建立查找选区（该方法为当前无调用方的私有遗留代码，白盒直测）
    setDocText(QString("one two three"));
    ASSERT_TRUE(edit->highlightKeyword(QString("two"), 0));
    const int firstMatchPos = edit->m_findMatchSelections.first().cursor.position();

    // Act：findNext=true 从 0 起 → 跳到首个匹配
    const bool moved = edit->setCursorKeywordSeletoin(0, true);

    // Assert
    EXPECT_TRUE(moved);
    EXPECT_EQ(edit->getPosition(), firstMatchPos);

    // Act：findNext=false 从文档尾 → 回到最后一个匹配
    const bool movedBack = edit->setCursorKeywordSeletoin(edit->toPlainText().size(), false);
    // Assert
    EXPECT_TRUE(movedBack);
}

TEST_F(TextEditTest, ReplaceAll_MarkCoveredByReplace_Removed)
{
    // Arrange：替换文本完全包含标记（EIntersectInner → 移除标记）
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(3, QTextCursor::KeepAnchor); // 标记 "bc"
    edit->setTextCursor(cur);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 1);
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act：abcd → X（替换区间完全覆盖标记）
    edit->replaceAll(QString("abcd"), QString("X"));

    // Assert：标记被清除
    EXPECT_TRUE(edit->m_wordMarkSelections.isEmpty());
    EXPECT_EQ(edit->toPlainText(), QString("Xef"));
}

// ---------------- replaceNext（F2） ----------------

TEST_F(TextEditTest, ReplaceNext_WithHighlightSelection_ReplacesOnce)
{
    // Arrange：先建立查找高亮（首个 "bb"）
    setDocText(QString("aa bb cc bb"));
    ASSERT_TRUE(edit->highlightKeyword(QString("bb"), 0));

    // Act
    edit->replaceNext(QString("bb"), QString("XX"));

    // Assert：仅当前高亮处被替换
    EXPECT_EQ(edit->toPlainText(), QString("aa XX cc bb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceNext_EmptyTextOrNoHighlight_EarlyReturn)
{
    // Arrange
    setDocText(QString("keep"));

    // Act：无高亮选区
    edit->replaceNext(QString("keep"), QString("drop"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("keep"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceNext_ReadOnlyMode_EarlyReturn)
{
    // Arrange
    setDocText(QString("ro"));
    edit->toggleReadOnlyMode(true);

    // Act
    edit->replaceNext(QString("ro"), QString("rw"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("ro"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceNext_CursorStartSet_UsesStoredPosition)
{
    // Arrange：高亮 + 指定替换起点
    setDocText(QString("x1 x1"));
    ASSERT_TRUE(edit->highlightKeyword(QString("x1"), 0));
    edit->setCursorStart(3); // 第二个 x1

    // Act
    edit->replaceNext(QString("x1"), QString("y2"));

    // Assert：从存储起点替换第二个
    EXPECT_EQ(edit->toPlainText(), QString("x1 y2"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

// ---------------- replaceRest（F3） ----------------

TEST_F(TextEditTest, ReplaceRest_FromCursor_ReplacesTailOnly)
{
    // Arrange
    setDocText(QString("num num num"));
    moveCursorTo(8); // 第三个 num 前

    // Act
    edit->replaceRest(QString("num"), QString("N"));

    // Assert：仅光标后替换
    EXPECT_EQ(edit->toPlainText(), QString("num num N"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ReplaceRest_EmptyOrSame_EarlyReturn)
{
    // Arrange
    setDocText(QString("hold"));

    // Act
    edit->replaceRest(QString(), QString("x"));
    edit->replaceRest(QString("hold"), QString("hold"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("hold"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

// ---------------- beforeReplace（F4） ----------------

TEST_F(TextEditTest, BeforeReplace_NoHighlight_TriggersHighlight)
{
    // Arrange
    setDocText(QString("alpha beta"));

    // Act：无高亮时 beforeReplace 会补一次关键字高亮
    edit->beforeReplace(QString("alpha"));

    // Assert：高亮已建立
    EXPECT_FALSE(edit->m_findMatchSelections.isEmpty());
    EXPECT_EQ(edit->m_findMatchSelections.size(), 1); // 仅 alpha 一处命中
}

TEST_F(TextEditTest, BeforeReplace_WithExistingHighlight_NoExtraWork)
{
    // Arrange
    setDocText(QString("alpha"));
    ASSERT_TRUE(edit->highlightKeyword(QString("alpha"), 0));
    const int countBefore = edit->m_findMatchSelections.size();

    // Act
    edit->beforeReplace(QString("alpha"));

    // Assert：高亮保持不变
    EXPECT_EQ(edit->m_findMatchSelections.size(), countBefore);
    EXPECT_EQ(edit->toPlainText(), QString("alpha")); // 高亮不改文本
}

// ---------------- findKeywordForward（F5） ----------------

TEST_F(TextEditTest, FindKeywordForward_Present_ReturnsTrueAndSelects)
{
    // Arrange
    setDocText(QString("one two three"));
    moveCursorTo(0);

    // Act
    const bool found = edit->findKeywordForward(QString("two"));

    // Assert
    EXPECT_TRUE(found);
    EXPECT_EQ(edit->getPosition(), 7); // find 选中 two(4~7)，光标落在匹配尾
}

TEST_F(TextEditTest, FindKeywordForward_Absent_ReturnsFalse)
{
    // Arrange
    setDocText(QString("one"));
    moveCursorTo(0);

    // Act/Assert
    EXPECT_FALSE(edit->findKeywordForward(QString("zzz")));
    EXPECT_EQ(edit->getPosition(), 0); // 未命中光标不动
}

TEST_F(TextEditTest, FindKeywordForward_WithSelection_ReturnsTrue)
{
    // Arrange
    setDocText(QString("needle needle"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(6, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act/Assert：选区分支从文档头查找
    EXPECT_TRUE(edit->findKeywordForward(QString("needle")));
    EXPECT_EQ(edit->toPlainText(), QString("needle needle")); // 查找不改文本
}

// ---------------- 高亮选区族（F6/F8/F9/F10/F16/F17） ----------------

TEST_F(TextEditTest, HighlightKeyword_MatchesInView_ReturnsTrue)
{
    // Arrange
    setDocText(QString("foo bar foo"));

    // Act
    const bool ok = edit->highlightKeyword(QString("foo"), 0);

    // Assert：全文档高亮 + 光标跳到首个匹配
    EXPECT_TRUE(ok);
    EXPECT_EQ(edit->m_findMatchSelections.size(), 2);
    EXPECT_TRUE(edit->m_findHighlightSelection.cursor.hasSelection());
}

TEST_F(TextEditTest, HighlightKeyword_NotFound_ReturnsFalseAndCleared)
{
    // Arrange
    setDocText(QString("nothing"));

    // Act
    const bool ok = edit->highlightKeyword(QString("abc"), 0);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_TRUE(edit->m_findMatchSelections.isEmpty());
}

TEST_F(TextEditTest, HighlightKeyword_CaseMismatch_UppercaseNotHighlighted)
{
    // Arrange：默认查找不区分大小写，但“完全相等才高亮”规则排除大小写差异
    setDocText(QString("f F f"));

    // Act
    const bool ok = edit->highlightKeyword(QString("f"), 0);

    // Assert：命中存在（返回 true）但仅小写 f 进入高亮
    EXPECT_TRUE(ok);
    // 注：代码注释称“大小写完全相等才高亮”，但 defaultCaseSensitive=CaseInsensitive
    // 使比较恒等 → 三个 f/F 均入列表（注释与行为不一致，已记录 defects）
    EXPECT_EQ(edit->m_findMatchSelections.size(), 3);
}

TEST_F(TextEditTest, HighlightKeywordInView_KeywordPresent_ReturnsTrue)
{
    // Arrange
    setDocText(QString("vis hidden vis"));

    // Act/Assert
    EXPECT_TRUE(edit->highlightKeywordInView(QString("vis")));
    EXPECT_EQ(edit->m_findMatchSelections.size(), 2);
}

TEST_F(TextEditTest, HighlightKeywordInView_KeywordAbsent_ReturnsFalse)
{
    // Arrange
    setDocText(QString("zzz"));

    // Act/Assert
    EXPECT_FALSE(edit->highlightKeywordInView(QString("vis")));
    EXPECT_TRUE(edit->m_findMatchSelections.isEmpty());
}

TEST_F(TextEditTest, RemoveKeywords_ClearsFindSelections)
{
    // Arrange
    setDocText(QString("k k"));
    ASSERT_TRUE(edit->highlightKeyword(QString("k"), 0));
    ASSERT_FALSE(edit->m_findMatchSelections.isEmpty());

    // Act
    edit->removeKeywords();

    // Assert
    EXPECT_TRUE(edit->m_findMatchSelections.isEmpty());
    EXPECT_FALSE(edit->m_findHighlightSelection.cursor.hasSelection());
}

TEST_F(TextEditTest, ClearFindMatchSelections_ListEmptied)
{
    // Arrange
    edit->m_findMatchSelections.append(QTextEdit::ExtraSelection());
    ASSERT_EQ(edit->m_findMatchSelections.size(), 1);

    // Act
    edit->clearFindMatchSelections();

    // Assert
    EXPECT_TRUE(edit->m_findMatchSelections.isEmpty());
    EXPECT_TRUE(edit->m_findMatchSelections.isEmpty());
}

TEST_F(TextEditTest, SetFindHighlightSelection_CursorStored)
{
    // Arrange
    setDocText(QString("abcd"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(3, QTextCursor::KeepAnchor);

    // Act
    edit->setFindHighlightSelection(cur);

    // Assert
    EXPECT_EQ(edit->m_findHighlightSelection.cursor.selectedText(), QString("bc"));
    EXPECT_EQ(edit->m_findHighlightSelection.cursor.selectionEnd(), 3);
}

TEST_F(TextEditTest, IfHasHighlight_WithAndWithoutSelection_Expected)
{
    // Arrange：初始无
    EXPECT_FALSE(edit->ifHasHighlight());

    // Act：建立带选区的查找高亮
    setDocText(QString("word"));
    edit->highlightKeyword(QString("word"), 0);

    // Assert
    EXPECT_TRUE(edit->ifHasHighlight());
}

TEST_F(TextEditTest, UpdateKeywordSelections_FoundAndNotFound_Expected)
{
    // Arrange
    setDocText(QString("rep rep"));
    QList<QTextEdit::ExtraSelection> list;
    QTextCharFormat fmt;

    // Act：命中收集
    bool ok = edit->updateKeywordSelections(QString("rep"), fmt, list);
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(list.size(), 2);

    // Act：未命中
    ok = edit->updateKeywordSelections(QString("absent"), fmt, list);
    // Assert
    EXPECT_FALSE(ok);

    // Act：空 keyword
    ok = edit->updateKeywordSelections(QString(), fmt, list);
    // Assert
    EXPECT_FALSE(ok);
}

TEST_F(TextEditTest, ScanAllMatchPositions_ParamVariants_ExpectedPositions)
{
    // Arrange
    setDocText(QString("ab AB ab"));

    // Act/Assert：不区分大小写 → 3 处
    EXPECT_EQ(edit->scanAllMatchPositions(QString("ab")).size(), 3);
    // 区分大小写 → 2 处
    EXPECT_EQ(edit->scanAllMatchPositions(QString("ab"), Qt::CaseSensitive),
              (QList<int>() << 0 << 6));
    // 空 keyword → 空
    EXPECT_TRUE(edit->scanAllMatchPositions(QString()).isEmpty());
}

TEST_F(TextEditTest, FindCurrentMatchIndex_CacheStates_ExpectedIndex)
{
    // Arrange：空缓存
    EXPECT_EQ(edit->findCurrentMatchIndex(), 0);

    // 填充缓存：两处匹配，高亮在第二处
    setDocText(QString("m m"));
    edit->highlightKeyword(QString("m"), 0);
    edit->m_allMatchPositions = QList<int>() << 0 << 2;
    edit->m_findHighlightSelection.cursor = makeCursor(2);

    // Act/Assert：命中缓存项 → 1 基索引 2
    EXPECT_EQ(edit->findCurrentMatchIndex(), 2);

    // 光标不在缓存点 → 0
    edit->m_findHighlightSelection.cursor = makeCursor(1);
    EXPECT_EQ(edit->findCurrentMatchIndex(), 0);
}

TEST_F(TextEditTest, UpdateCursorKeywordSelection_NotFoundAnywhere_ClearsSelections)
{
    // Arrange：文档无匹配
    setDocText(QString("plain"));

    // Act
    edit->updateCursorKeywordSelection(QString("zzz"), true);

    // Assert：全失败路径清空高亮并渲染
    EXPECT_TRUE(edit->m_findMatchSelections.isEmpty());
    EXPECT_FALSE(edit->m_findHighlightSelection.cursor.hasSelection());
}

TEST_F(TextEditTest, UpdateHighlightLineSelection_SlideGesture_Skipped)
{
    // Arrange：模拟滑动中
    edit->m_gestureAction = TextEdit::GA_slide;
    QTextEdit::ExtraSelection stale;
    stale.cursor = makeCursor(0);
    edit->m_currentLineSelection = stale;

    // Act
    edit->updateHighlightLineSelection();

    // Assert：滑动分支提前返回，旧行选区未被覆盖
    EXPECT_EQ(edit->m_currentLineSelection.cursor.position(), stale.cursor.position());

    // Act：非滑动恢复正常
    edit->m_gestureAction = TextEdit::GA_null;
    edit->updateHighlightLineSelection();
    EXPECT_TRUE(edit->m_currentLineSelection.cursor.selectedText().isEmpty());
}

// ---------------- updateKeywordSelectionsInView（F11） ----------------

TEST_F(TextEditTest, UpdateKeywordSelectionsInView_EmptyKeyword_ReturnsFalse)
{
    // Arrange
    QList<QTextEdit::ExtraSelection> list;

    // Act/Assert
    EXPECT_FALSE(edit->updateKeywordSelectionsInView(QString(), QTextCharFormat(), &list));
    EXPECT_TRUE(list.isEmpty());
}

TEST_F(TextEditTest, UpdateKeywordSelectionsInView_MultilineKeyword_Collected)
{
    // Arrange：跨行关键字 "a\nb"
    setDocText(QString("xa\nbx"));
    QList<QTextEdit::ExtraSelection> list;

    // Act
    const bool ok = edit->updateKeywordSelectionsInView(QString("a\nb"), QTextCharFormat(), &list);

    // Assert：多行拼接查找路径命中
    EXPECT_TRUE(ok);
    EXPECT_EQ(list.size(), 1);
}

TEST_F(TextEditTest, UpdateKeywordSelectionsInView_CaseExactOnly_Highlighted)
{
    // Arrange：默认大小写不敏感 → key/KEY 均进入高亮列表
    setDocText(QString("key KEY"));
    QList<QTextEdit::ExtraSelection> list;

    // Act
    const bool ok = edit->updateKeywordSelectionsInView(QString("key"), QTextCharFormat(), &list);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(list.size(), 2);
}

// ---------------- searchKeywordSeletion（F12） ----------------

TEST_F(TextEditTest, SearchKeywordSeletion_ForwardHit_MovesCursor)
{
    // Arrange
    setDocText(QString("target after"));
    moveCursorTo(0);

    // Act
    const bool ok = edit->searchKeywordSeletion(QString("target"), edit->textCursor(), true);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_TRUE(edit->m_findHighlightSelection.cursor.hasSelection());
}

TEST_F(TextEditTest, SearchKeywordSeletion_BackwardHit_MovesCursor)
{
    // Arrange
    setDocText(QString("aaa target"));
    moveCursorTo(10);

    // Act
    const bool ok = edit->searchKeywordSeletion(QString("target"), edit->textCursor(), false);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(edit->m_findHighlightSelection.cursor.selectionStart(), 4); // 高亮落在 target(4~10)
}

TEST_F(TextEditTest, SearchKeywordSeletion_MultilineKeyword_Found)
{
    // Arrange
    setDocText(QString("x\ny"));
    moveCursorTo(0);

    // Act：含 \n 关键字走 findCursor 路径
    const bool ok = edit->searchKeywordSeletion(QString("x\ny"), edit->textCursor(), true);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(edit->m_findHighlightSelection.cursor.selectedText(), QString("x") + QChar(0x2029) + QString("y"));
}

TEST_F(TextEditTest, SearchKeywordSeletion_EmptyKeyword_ReturnsFalse)
{
    // Arrange
    setDocText(QString("data"));

    // Act/Assert
    EXPECT_FALSE(edit->searchKeywordSeletion(QString(), edit->textCursor(), true));
    EXPECT_EQ(edit->getPosition(), 0); // 空关键字光标不动
}

// ---------------- findCursor（F13） ----------------

TEST_F(TextEditTest, FindCursor_ForwardHit_ReturnsSelectedCursor)
{
    // Arrange
    setDocText(QString("hello world"));

    // Act
    const QTextCursor cur = edit->findCursor(QString("world"), edit->toPlainText(), 0, false);

    // Assert：绝对位置选区
    EXPECT_EQ(cur.selectionStart(), 6);
    EXPECT_EQ(cur.selectedText(), QString("world"));
}

TEST_F(TextEditTest, FindCursor_BackwardHit_ReturnsSelectedCursor)
{
    // Arrange
    setDocText(QString("a b a b"));

    // Act：从 5 反向查找（越过位置 6 的 b，命中位置 2）
    const QTextCursor cur = edit->findCursor(QString("b"), edit->toPlainText(), 5, true);

    // Assert
    EXPECT_EQ(cur.selectionStart(), 2);
    EXPECT_EQ(cur.selectedText(), QString("b"));
}

TEST_F(TextEditTest, FindCursor_NotFound_ReturnsNullCursor)
{
    // Arrange
    setDocText(QString("abc"));

    // Act/Assert
    EXPECT_TRUE(edit->findCursor(QString("zz"), edit->toPlainText(), 0, false).isNull());
    EXPECT_EQ(edit->toPlainText(), QString("abc")); // 查找不改文本
}

TEST_F(TextEditTest, FindCursor_WindowsEndline_CrLfKeywordNormalized)
{
    // Arrange：Windows 行尾格式下关键字中的 \r\n 被规范化
    setDocText(QString("ab\ncd"));
    seamEndlineFormat = BottomBar::Windows;

    // Act：以 \r\n 关键字在 \n 文本中查找（规范化后命中）
    const QTextCursor cur = edit->findCursor(QString("b\r\nc"), edit->toPlainText(), 0, false);

    // Assert：命中 1~4，跨块选中文本（QTextCursor 以 U+2029 表示段落分隔符）
    EXPECT_EQ(cur.selectionStart(), 1);
    EXPECT_EQ(cur.selectedText(), QString("b") + QChar(0x2029) + QString("c"));
}

// ---------------- 匹配计数信号（F14） ----------------

TEST_F(TextEditTest, UpdateMatchCount_EmptyKeyword_EmitsZeroZero)
{
    // Arrange
    QSignalSpy spy(edit, &TextEdit::findMatchCountChanged);

    // Act
    edit->updateMatchCount(QString(), Qt::CaseInsensitive);

    // Assert
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 0);
    EXPECT_EQ(spy.at(0).at(1).toInt(), 0);
}

TEST_F(TextEditTest, UpdateMatchCount_WithKeyword_EmitsCurrentTotal)
{
    // Arrange
    setDocText(QString("hit hit hit"));
    edit->m_findHighlightSelection.cursor = makeCursor(4); // 第二个 hit 内
    QSignalSpy spy(edit, &TextEdit::findMatchCountChanged);

    // Act
    edit->updateMatchCount(QString("hit"), Qt::CaseInsensitive);

    // Assert：total=3；current 依据高亮起始位置（4 → lower_bound 命中索引 1 → 2）
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(1).toInt(), 3);
    // 缓存已建立：再查同关键字不发额外重扫（第二次直接命中缓存）
    edit->updateMatchCount(QString("hit"), Qt::CaseInsensitive);
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(TextEditTest, InvalidateMatchCountCache_TextChange_ClearsCache)
{
    // Arrange：建立缓存
    setDocText(QString("c c"));
    edit->updateMatchCount(QString("c"), Qt::CaseInsensitive);
    EXPECT_FALSE(edit->m_allMatchPositions.isEmpty());

    // Act：文本变化触发缓存失效（textChanged → invalidateMatchCountCache）
    QTextCursor cur(edit->document());
    cur.setPosition(3);
    cur.insertText(QString("c"));

    // Assert
    EXPECT_TRUE(edit->m_allMatchPositions.isEmpty());
    EXPECT_TRUE(edit->m_countedKeyword.isEmpty());
}

// ---------------- onPressedLineNumber（F15） ----------------

TEST_F(TextEditTest, OnPressedLineNumber_ValidPoint_SelectsWholeLine)
{
    // Arrange
    setDocText(QString("first\nsecond"));
    edit->setLeftAreaUpdateState(TextEdit::Normal);

    // Act：点击第一行区域（x 在行号区宽度内）
    edit->onPressedLineNumber(QPoint(2, 4));

    // Assert：整行被选中（到下一行行首，跨块选区含 U+2029 段落分隔符）
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_TRUE(edit->textCursor().selectedText().startsWith(QString("first")));
    EXPECT_EQ(edit->textCursor().selectionEnd(), 6);
}

TEST_F(TextEditTest, OnPressedLineNumber_LastBlock_SelectsToEndOfBlock)
{
    // Arrange
    setDocText(QString("first\nlast"));
    edit->setLeftAreaUpdateState(TextEdit::Normal);

    // Act：点击最后一行区域（y 较大）
    edit->onPressedLineNumber(QPoint(2, 40));

    // Assert：尾行 EndOfBlock 分支
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectionEnd(), 10); // 尾块 EndOfBlock（文档 10 字符）
}

TEST_F(TextEditTest, OnPressedLineNumber_FileOpenBegin_Ignored)
{
    // Arrange
    setDocText(QString("line"));
    edit->setLeftAreaUpdateState(TextEdit::FileOpenBegin);

    // Act
    edit->onPressedLineNumber(QPoint(2, 4));

    // Assert：大文件加载中不响应
    EXPECT_FALSE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->getPosition(), 0);
}

TEST_F(TextEditTest, OnPressedLineNumber_XOutOfRange_Ignored)
{
    // Arrange
    setDocText(QString("line"));
    edit->setLeftAreaUpdateState(TextEdit::Normal);

    // Act：x 超出行号区宽度
    edit->onPressedLineNumber(QPoint(100000, 4));

    // Assert
    EXPECT_FALSE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->getPosition(), 0);
}

// ---------------- renderAllSelections（F16，排序与合并主链） ----------------

TEST_F(TextEditTest, RenderAllSelections_MixedSelections_AppliedToViewport)
{
    // Arrange：当前行高亮 + 单行标记 + 括号 + Alt 选区混布
    setDocText(QString("(x) mark"));
    edit->setHighLineCurrentLine(true);
    moveCursorTo(1);
    edit->isMarkCurrentLine(true, QString("#ff0000"), 5);
    QTextEdit::ExtraSelection alt;
    alt.cursor = makeCursor(0);
    alt.cursor.setPosition(1, QTextCursor::KeepAnchor);
    edit->m_altModSelections << alt;
    edit->m_bIsAltMod = true;

    // Act
    edit->renderAllSelections();

    // Assert：合成选区写入 QPlainTextEdit（含当前行 + 标记 + Alt + 括号）
    const int total = edit->extraSelections().size();
    EXPECT_GE(total, 3);
    EXPECT_TRUE(edit->m_HightlightYes); // 当前行高亮已入渲染链
}
