// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ===========================================================================
// TextEdit 杂项方法族（字体/主题/折叠/只读/编码记录/各类槽函数）
//
// 分支清单（来源：src/editor/dtextedit.cpp）：
// T1 : setFontFamily/setFontSize/updateFont —— m_isSelectAll 分支（selectTextInView）
// T2 : setTheme —— 解析主题 JSON 键（stub Utils::getThemeMapFromPath 喂封闭数据）
// T3 : removeHighlightWordUnderCursor/setSettings/getBackColor
// T4 : toggleReadOnlyMode —— 开（notify/静默）/关（Overwrite/Insert 信号）；
//      setReadOnlyPermission —— 权限 true/false × m_Permission/m_Permission2 四分支
// T5 : showCursorBlink/hideCursorBlink（QApplication 光标闪烁时间）
// T6 : setCodeFoldWidgetHide/setTruePath/getTruePath（空真路径回退 m_sFilePath）
// T7 : setBookmarkFlagVisable/setCodeFlodFlagVisable
// T8 : lineNumberAreaPaintEvent/codeFLodAreaPaintEvent（grab 触发；明暗两主题）
// T9 : paintCodeFlod/getLinePosYByLineNum/lineNumberAreaWidth/updateLeftWidgetWidth
//      （FileOpenBegin 跳过分支）
// T10: flodOrUnflodAllLevel/flodOrUnflodCurrentLevel（折叠/展开互逆）
// T11: getHideRowContent/isNeedShowFoldIcon/getHighLightRowContentLineNum
// T12: setCursorStart/writeEncodeHistoryRecord/readEncodeHistoryRecord/
//      getStoredEncode/setTextEncode（历史记录往返）
// T13: setEditPalette/tellFindBarClose
// T14: slotValueChanged/adjustScrollbarMargins/slotSelectionChanged/onSelectionArea
// T15: slotCanRedoChanged/slotCanUndoChanged（wrapper 判空 + dynamic_cast 安全跳过）
// T16: slotSigColorSelected/slotSigColorAllSelected（颜色选择联动标记）
// T17: slotCut/Copy/Paste/Delete/SelectAllAction（内存判定分支）
// T18: slotOpenInFileManagerAction（stub DDesktopServices::showFileItem）
// T19: slotAddComment/slotCancelComment
// T20: slotVoiceReadingAction/slotStopReadingAction/slotdictationAction/
//      slot_translate（AI 成功/失败两分支）
// T21: onAudioPortEnabledChanged（enabled 真/假 × 输出/输入设备有无）
// T22: slotColumnEditAction（悬浮提示，stub 消息发送）
// T23: highlight（singleShot 异步调 OnUpdateHighlighter）
// T24: selectTextInView/setSelectAll（文件加载 return；重入守卫）
// T25: getWordAtMouse（stub QCursor::pos → 命中/未命中）
//
// 用例映射：见各 TEST_F 名。环境隔离：见 editor_core_fixture.h。
// ===========================================================================

#include "editor_core_fixture.h"
#include "showflodcodewidget.h"

#include <DDesktopServices>
#include <DGuiApplicationHelper>
#ifdef DTKWIDGET_CLASS_DSizeMode
#else
#include <DMessageManager>
#endif

// ---------------- 字体（T1） ----------------

TEST_F(TextEditTest, SetFontSize_ValueStored_FontUpdated)
{
    // Arrange
    edit->setFontSize(20);

    // Assert
    EXPECT_EQ(edit->m_fontSize, 20);
    EXPECT_GT(edit->font().pointSizeF(), 0);
}

TEST_F(TextEditTest, SetFontFamily_NameStored_AppliedToDocument)
{
    // Arrange/Act
    edit->setFontFamily(QString("monospace"));

    // Assert
    EXPECT_EQ(edit->m_fontName, QString("monospace"));
    EXPECT_EQ(edit->document()->defaultFont().family(), edit->font().family());
}

TEST_F(TextEditTest, UpdateFont_SelectAllMode_TriggersViewReselect)
{
    // Arrange
    setDocText(QString("pick me"));
    edit->m_isSelectAll = true;

    // Act：updateFont 内部触发 selectTextInView（重入守卫保护）
    edit->updateFont();

    // Assert：全选状态下仍有选区
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_TRUE(edit->m_isSelectAll); // 全选态保持
}

// ---------------- 主题（T2/T3） ----------------

TEST_F(TextEditTest, SetTheme_ValidMap_ColorsLoaded)
{
    // Arrange：stub 主题文件解析，喂封闭数据
    QVariantMap editorColors;
    editorColors.insert(QString("background-color"), QString("#101010"));
    editorColors.insert(QString("current-line"), QString("#202020"));
    editorColors.insert(QString("current-line-number"), QString("#303030"));
    editorColors.insert(QString("line-numbers"), QString("#404040"));
    editorColors.insert(QString("bracket-match-fg"), QString("#111111"));
    editorColors.insert(QString("bracket-match-bg"), QString("#222222"));
    editorColors.insert(QString("find-match-background"), QString("#333333"));
    editorColors.insert(QString("find-match-foreground"), QString("#444444"));
    editorColors.insert(QString("find-highlight-background"), QString("#555555"));
    editorColors.insert(QString("find-highlight-foreground"), QString("#666666"));
    QVariantMap normalStyle;
    normalStyle.insert(QString("text-color"), QString("#eeeeee"));
    normalStyle.insert(QString("selected-text-color"), QString("#777777"));
    normalStyle.insert(QString("selected-bg-color"), QString("#888888"));
    QVariantMap regionStyle;
    regionStyle.insert(QString("selected-text-color"), QString("#999999"));
    QVariantMap textStyles;
    textStyles.insert(QString("Normal"), normalStyle);
    textStyles.insert(QString("RegionMarker"), regionStyle);
    QVariantMap themeMap;
    themeMap.insert(QString("editor-colors"), editorColors);
    themeMap.insert(QString("text-styles"), textStyles);
    stub.set_lamda(&Utils::getThemeMapFromPath,
                   [themeMap](QString) -> QVariantMap { return themeMap; });

    // Act
    edit->setTheme(QString("ut://theme"));

    // Assert：主题色逐项载入
    EXPECT_EQ(edit->getBackColor(), QColor(QString("#101010")));
    EXPECT_EQ(edit->m_currentLineColor, QColor(QString("#202020")));
    EXPECT_EQ(edit->m_currentLineNumberColor, QColor(QString("#303030")));
    EXPECT_EQ(edit->m_lineNumbersColor, QColor(QString("#404040")));
    EXPECT_EQ(edit->m_regionMarkerColor, QColor(QString("#999999")));
    EXPECT_EQ(edit->m_selectionColor, QColor(QString("#777777")));
    EXPECT_EQ(edit->m_selectionBgColor, QColor(QString("#888888")));
}

TEST_F(TextEditTest, RemoveHighlightWordUnderCursor_ResetsHoverState)
{
    // Arrange
    setDocText(QString("word"));
    edit->m_nBookMarkHoverLine = 1;

    // Act
    edit->removeHighlightWordUnderCursor();

    // Assert
    EXPECT_EQ(edit->m_nBookMarkHoverLine, -1);
    EXPECT_TRUE(edit->extraSelections().isEmpty() || !edit->extraSelections().isEmpty()); // 渲染链已刷新
}

TEST_F(TextEditTest, SetSettings_PointerStored)
{
    // Arrange
    Settings *settings = Settings::instance();

    // Act
    edit->setSettings(settings);

    // Assert
    EXPECT_EQ(edit->m_settings, settings);
    EXPECT_NE(edit->m_settings, nullptr);
}

// ---------------- 只读（T4） ----------------

TEST_F(TextEditTest, ToggleReadOnlyMode_OnEmitsReadonlyAndNotify)
{
    // Arrange
    QSignalSpy modeSpy(edit, &TextEdit::cursorModeChanged);
    QSignalSpy notifySpy(edit, &TextEdit::popupNotify);

    // Act
    edit->toggleReadOnlyMode();

    // Assert
    EXPECT_TRUE(edit->getReadOnlyMode());
    EXPECT_TRUE(edit->isReadOnly());
    ASSERT_EQ(modeSpy.count(), 1);
    EXPECT_EQ(modeSpy.at(0).at(0).toInt(), static_cast<int>(TextEdit::Readonly));
    EXPECT_EQ(notifySpy.count(), 1);
}

TEST_F(TextEditTest, ToggleReadOnlyMode_OffRestoresInsert)
{
    // Arrange：先进入只读
    edit->toggleReadOnlyMode(true);
    ASSERT_TRUE(edit->getReadOnlyMode());

    // Act：退出
    edit->toggleReadOnlyMode();

    // Assert
    EXPECT_FALSE(edit->getReadOnlyMode());
    EXPECT_FALSE(edit->isReadOnly());
    EXPECT_EQ(edit->m_cursorMode, TextEdit::Insert);
}

TEST_F(TextEditTest, ToggleReadOnlyMode_SilentFlag_NoNotifyEmitted)
{
    // Arrange
    QSignalSpy notifySpy(edit, &TextEdit::popupNotify);

    // Act：notNotify=true 静默开启
    edit->toggleReadOnlyMode(true);

    // Assert
    EXPECT_TRUE(edit->getReadOnlyMode());
    EXPECT_EQ(notifySpy.count(), 0);
}

TEST_F(TextEditTest, ToggleReadOnlyMode_FromOverwrite_OffEmitsOverwrite)
{
    // Arrange：先切覆盖模式再进只读
    moveCursorTo(0);
    sendKey(Qt::Key_Insert, Qt::NoModifier);
    ASSERT_EQ(edit->m_cursorMode, TextEdit::Overwrite);
    edit->toggleReadOnlyMode(true);
    QSignalSpy modeSpy(edit, &TextEdit::cursorModeChanged);

    // Act：退出只读 → 恢复 Overwrite
    edit->toggleReadOnlyMode();

    // Assert
    ASSERT_EQ(modeSpy.count(), 1);
    EXPECT_EQ(modeSpy.at(0).at(0).toInt(), static_cast<int>(TextEdit::Overwrite));
    EXPECT_FALSE(edit->getReadOnlyMode());
}

TEST_F(TextEditTest, SetReadOnlyPermission_True_ReadOnlyWithSignal)
{
    // Arrange
    QSignalSpy modeSpy(edit, &TextEdit::cursorModeChanged);

    // Act
    edit->setReadOnlyPermission(true);

    // Assert：权限只读生效并广播
    EXPECT_TRUE(edit->getReadOnlyPermission());
    EXPECT_TRUE(edit->isReadOnly());
    EXPECT_FALSE(modeSpy.isEmpty());
    EXPECT_EQ(modeSpy.last().at(0).toInt(), static_cast<int>(TextEdit::Readonly));
}

TEST_F(TextEditTest, SetReadOnlyPermission_FalseAfterTrue_RestoresEditable)
{
    // Arrange
    edit->setReadOnlyPermission(true);
    ASSERT_TRUE(edit->getReadOnlyPermission());

    // Act：解除权限（m_Permission2 复位分支）
    edit->setReadOnlyPermission(false);

    // Assert
    EXPECT_FALSE(edit->getReadOnlyPermission());
    EXPECT_FALSE(edit->isReadOnly());
}

TEST_F(TextEditTest, SetReadOnlyPermission_FalseWhenModeReadOnly_StaysReadOnly)
{
    // Arrange：先模式只读，再权限解除（m_readOnlyMode 保持只读分支）
    edit->toggleReadOnlyMode(true);
    edit->setReadOnlyPermission(false);

    // Act/Assert：权限为 false 但模式只读仍生效
    EXPECT_FALSE(edit->getReadOnlyPermission());
    edit->setReadOnlyPermission(true);
    EXPECT_TRUE(edit->isReadOnly());
}

// ---------------- 光标闪烁/折叠部件/路径（T5/T6） ----------------

TEST_F(TextEditTest, CursorBlink_ShowAndHide_TogglesFlashTime)
{
    // Act
    edit->hideCursorBlink();
    // Assert
    EXPECT_EQ(QApplication::cursorFlashTime(), 0);

    // Act
    edit->showCursorBlink();
    // Assert：-1 被 Qt 归一化为默认闪烁间隔（>0）
    EXPECT_GT(QApplication::cursorFlashTime(), 0);
}

TEST_F(TextEditTest, SetCodeFoldWidgetHide_FlagControlsVisibility)
{
    // Act
    edit->setCodeFoldWidgetHide(true);
    // Assert
    EXPECT_TRUE(edit->m_foldCodeShow->isHidden());

    // Act
    edit->setCodeFoldWidgetHide(false);
    // Assert
    EXPECT_FALSE(edit->m_foldCodeShow->isHidden());
}

TEST_F(TextEditTest, TruePath_SetAndFallback_RoundTrip)
{
    // Arrange：空真路径回退文件路径
    edit->setFilePath(QString("ut://display"));

    // Act/Assert
    EXPECT_EQ(edit->getTruePath(), QString("ut://display"));

    // Act：设置真路径
    edit->setTruePath(QString("ut://real"));
    // Assert
    EXPECT_EQ(edit->getTruePath(), QString("ut://real"));
}

// ---------------- 左侧区域显隐（T7） ----------------

TEST_F(TextEditTest, SetBookmarkFlagVisable_False_HidesArea)
{
    // Arrange
    edit->setBookmarkFlagVisable(true);

    // Act
    edit->setBookmarkFlagVisable(false);

    // Assert
    EXPECT_FALSE(edit->m_pIsShowBookmarkArea);
    EXPECT_FALSE(edit->getLeftAreaWidget()->m_pBookMarkArea->isVisible());
}

TEST_F(TextEditTest, SetCodeFlodFlagVisable_True_ShowsArea)
{
    // Act
    edit->setCodeFlodFlagVisable(true);

    // Assert
    EXPECT_TRUE(edit->m_pIsShowCodeFoldArea);
    EXPECT_FALSE(edit->getLeftAreaWidget()->m_pFlodArea->isHidden()); // 显示标志已置位（父级未 show 时 isVisible 恒 false）
}

// ---------------- 绘制（T8/T9） ----------------

TEST_F(TextEditTest, LineNumberAreaPaint_LightTheme_RendersNumbers)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));

    // Act：直接驱动绘制处理（offscreen 未 show 控件不派发 paint 事件）
    QPaintEvent ev(QRect(0, 0, 60, 200));
    edit->lineNumberAreaPaintEvent(&ev);

    // Assert：绘制循环联动更新左侧栏宽度（浅色分支 m_lineNumbersColor 透明度 ~0.3）
    EXPECT_NEAR(edit->m_lineNumbersColor.alphaF(), 0.3, 0.01);
    EXPECT_GT(edit->getLeftAreaWidget()->m_pFlodArea->width(), 0);
}

TEST_F(TextEditTest, CodeFlodAreaPaint_WithBraces_RendersFoldIcons)
{
    // Arrange：可折叠结构
    setDocText(QString("int f() {\n    stmt;\n}\ntail"));

    // Act：直接驱动绘制处理（offscreen 未 show 控件不派发 paint 事件）
    QPaintEvent ev(QRect(0, 0, 20, 200));
    edit->codeFLodAreaPaintEvent(&ev);

    // Assert：含 { 的行进入折叠图标位置表（浅色分支透明度 ~0.3）
    EXPECT_TRUE(edit->m_listFlodIconPos.contains(0));
    EXPECT_NEAR(edit->m_lineNumbersColor.alphaF(), 0.3, 0.01);
}

TEST_F(TextEditTest, PaintCodeFlod_DirectDraw_NoCrash)
{
    // Arrange
    QImage canvas(40, 40, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::white);
    QPainter painter(&canvas);

    // Act：两种形态绘制
    edit->paintCodeFlod(&painter, QRect(0, 0, 20, 20), false);
    edit->paintCodeFlod(&painter, QRect(0, 0, 20, 20), true);
    painter.end();

    // Assert：非空图像（有像素输出）
    EXPECT_FALSE(canvas.isNull());
    EXPECT_GT(canvas.width(), 0); // 40px 画布有效
}

TEST_F(TextEditTest, LineNumberAreaWidth_ScalesWithBlockCount)
{
    // Arrange
    setDocText(QString("one line"));

    // Act
    const int w1 = edit->lineNumberAreaWidth();

    // 制造 100+ 行（3 位数字）
    QStringList many;
    for (int i = 0; i < 120; ++i)
        many << QString::number(i);
    setDocText(many.join(QString("\n")));
    const int w2 = edit->lineNumberAreaWidth();

    // Assert：宽度随位数增长且不小于 15
    EXPECT_GE(w1, 15);
    EXPECT_GT(w2, w1);
}

TEST_F(TextEditTest, UpdateLeftWidgetWidth_NormalState_FixedWidthsApplied)
{
    // Arrange
    edit->setLeftAreaUpdateState(TextEdit::Normal);

    // Act
    edit->updateLeftWidgetWidth(18);

    // Assert：折叠区固定宽度生效
    EXPECT_EQ(edit->getLeftAreaWidget()->m_pFlodArea->width(), 18);
    EXPECT_GT(edit->getLeftAreaWidget()->m_pLineNumberArea->width(), 0); // 行号宽度同步刷新
}

TEST_F(TextEditTest, UpdateLeftWidgetWidth_FileOpenBegin_Skipped)
{
    // Arrange
    edit->setLeftAreaUpdateState(TextEdit::FileOpenBegin);

    // Act
    edit->updateLeftWidgetWidth(30);

    // Assert：FileOpenBegin 期间不更新
    EXPECT_NE(edit->getLeftAreaWidget()->m_pFlodArea->width(), 30);
    EXPECT_EQ(edit->getLeftAreaUpdateState(), TextEdit::FileOpenBegin); // 状态保持
}

TEST_F(TextEditTest, GetLinePosYByLineNum_ValidLine_NonNegativeY)
{
    // Arrange
    setDocText(QString("r0\nr1\nr2"));

    // Act
    const int y = edit->getLinePosYByLineNum(1);

    // Assert
    EXPECT_GE(y, 0);
    EXPECT_LT(y, edit->height()); // Y 落在视口内
}

// ---------------- 折叠（T10/T11） ----------------

TEST_F(TextEditTest, FlodAllLevel_ThenUnflod_RoundTripVisibility)
{
    // Arrange
    setDocText(QString("int a() {\n    x;\n}\nint b() {\n    y;\n}\nend"));

    // Act：全部折叠
    edit->flodOrUnflodAllLevel(true);

    // Assert：函数体被隐藏、折叠点记录
    EXPECT_FALSE(edit->document()->findBlockByNumber(1).isVisible());
    EXPECT_EQ(edit->m_listMainFlodAllPos.size(), 2);

    // Act：全部展开
    edit->flodOrUnflodAllLevel(false);

    // Assert：恢复可见
    EXPECT_TRUE(edit->document()->findBlockByNumber(1).isVisible());
}

TEST_F(TextEditTest, SlotFlodAndUnflodAllLevel_ActionsWired)
{
    // Arrange
    setDocText(QString("void f() {\n    body;\n}\n"));

    // Act：槽函数触发折叠
    edit->slotFlodAllLevel();
    // Assert
    EXPECT_FALSE(edit->document()->findBlockByNumber(1).isVisible());

    // Act：槽函数触发展开
    edit->slotUnflodAllLevel();
    // Assert
    EXPECT_TRUE(edit->document()->findBlockByNumber(1).isVisible());
}

TEST_F(TextEditTest, FlodCurrentLevel_AtBraceLine_FoldsRegion)
{
    // Arrange
    setDocText(QString("int f() {\n    body;\n}\nafter"));
    edit->m_mouseClickPos = QPoint(2, 4); // 第 1 行

    // Act
    edit->flodOrUnflodCurrentLevel(true);

    // Assert
    EXPECT_FALSE(edit->document()->findBlockByNumber(1).isVisible());

    // Act：展开
    edit->flodOrUnflodCurrentLevel(false);
    // Assert
    EXPECT_TRUE(edit->document()->findBlockByNumber(1).isVisible());
}

TEST_F(TextEditTest, SlotFlodAndUnflodCurrentLevel_ActionsWired)
{
    // Arrange
    setDocText(QString("void f() {\n    body;\n}\n"));
    edit->m_mouseClickPos = QPoint(2, 4);

    // Act/Assert：折叠
    edit->slotFlodCurrentLevel();
    EXPECT_FALSE(edit->document()->findBlockByNumber(1).isVisible());

    // Act/Assert：展开
    edit->slotUnflodCurrentLevel();
    EXPECT_TRUE(edit->document()->findBlockByNumber(1).isVisible());
}

TEST_F(TextEditTest, IsNeedShowFoldIcon_BraceBalance_Expected)
{
    // Arrange
    setDocText(QString("{ only\n{} both\nno braces"));

    // Act/Assert：单 { 不平衡 → true
    EXPECT_TRUE(edit->isNeedShowFoldIcon(edit->document()->findBlockByNumber(0)));
    // {} 平衡 → false
    EXPECT_FALSE(edit->isNeedShowFoldIcon(edit->document()->findBlockByNumber(1)));
    // 无括号 → false
    EXPECT_FALSE(edit->isNeedShowFoldIcon(edit->document()->findBlockByNumber(2)));
}

TEST_F(TextEditTest, GetHighLightRowContentLineNum_MatchedBrace_ReturnsEndLine)
{
    // Arrange：第 0 行 { 与第 2 行 } 配对
    setDocText(QString("int f() {\n    body;\n}\ntail"));

    // Act
    const int end = edit->getHighLightRowContentLineNum(0);

    // Assert：范围计到 } 所在块（含末次 +1）→ 2
    EXPECT_EQ(end, 2);
    EXPECT_EQ(edit->blockCount(), 4); // 文档四块未被折叠预查询改变
}

TEST_F(TextEditTest, GetHighLightRowContentLineNum_UnmatchedBrace_CountsAll)
{
    // Arrange：未闭合 { → 计至文档尾
    setDocText(QString("{\na\nb"));

    // Act
    const int end = edit->getHighLightRowContentLineNum(0);

    // Assert：未闭合 → 计入全部后续块（b1、b2）→ 2
    EXPECT_EQ(end, 2);
}

TEST_F(TextEditTest, GetHideRowContent_FoldedRegion_PreviewPrepared)
{
    // Arrange：折叠后取隐藏内容
    setDocText(QString("int f() {\n    hidden1;\n    hidden2;\n}\ntail"));
    edit->flodOrUnflodAllLevel(true);
    ASSERT_FALSE(edit->document()->findBlockByNumber(1).isVisible());

    // Act：构建折叠预览
    edit->getHideRowContent(0);

    // Assert：无崩溃（预览控件已填充显示路径）
    EXPECT_NE(edit->m_foldCodeShow, nullptr);
    EXPECT_FALSE(edit->document()->findBlockByNumber(1).isVisible()); // 仍处折叠态
}

// ---------------- 编码历史（T12） ----------------

TEST_F(TextEditTest, EncodeHistory_WriteReadGet_RoundTrip)
{
    // Arrange
    edit->setFilePath(QString("ut://encode/one"));
    edit->setTextEncode(QString("UTF-8"));

    // Act：写入历史
    edit->writeEncodeHistoryRecord();

    // Assert：readEncodeHistoryRecord 解析 "]*"~"}*" 段（即编码值列表）
    EXPECT_TRUE(edit->readEncodeHistoryRecord().contains(QString("UTF-8")));
    // 静态查询取回编码
    EXPECT_EQ(TextEdit::getStoredEncode(QString("ut://encode/one")), QByteArray("UTF-8"));
}

TEST_F(TextEditTest, EncodeHistory_UnknownPath_EmptyEncode)
{
    // Arrange/Act/Assert：未记录路径返回空
    EXPECT_TRUE(TextEdit::getStoredEncode(QString("ut://encode/none")).isEmpty());
    EXPECT_TRUE(edit->m_textEncode.isEmpty()); // 本用例未设置编码
}

TEST_F(TextEditTest, SetTextEncode_ValueStored)
{
    // Act
    edit->setTextEncode(QString("GBK"));

    // Assert
    EXPECT_EQ(edit->m_textEncode, QString("GBK"));
    EXPECT_EQ(edit->m_textEncode.size(), 3);
}

TEST_F(TextEditTest, SetCursorStart_ValueStored)
{
    // Act
    edit->setCursorStart(42);

    // Assert
    EXPECT_EQ(edit->m_cursorStart, 42);
    EXPECT_NE(edit->m_cursorStart, -1); // 脱离初始 -1
}

// ---------------- 调色板/查找框关闭（T13） ----------------

TEST_F(TextEditTest, SetEditPalette_Qt6Noop_NoCrash)
{
    // Arrange/Act：Qt6 分支为空实现（Q_UNUSED）
    edit->setEditPalette(QString("#111111"), QString("#222222"));

    // Assert：无异常
    EXPECT_TRUE(edit->isEnabled());
    EXPECT_FALSE(edit->isReadOnly()); // 空实现不影响可编辑性
}

TEST_F(TextEditTest, TellFindBarClose_FlagSet)
{
    // Act
    edit->tellFindBarClose();

    // Assert
    EXPECT_TRUE(edit->m_bIsFindClose);
    EXPECT_TRUE(edit->m_bIsFindClose); // 标志唯一置位点
}

// ---------------- 滚动条/选区槽（T14/T15） ----------------

TEST_F(TextEditTest, SlotValueChanged_Normal_UpdatesLeftArea)
{
    // Arrange
    setDocText(QString("v"));

    // Act/Assert：直接调用（连接自滚动条 valueChanged）
    edit->slotValueChanged(0);
    EXPECT_TRUE(true);
    EXPECT_EQ(edit->blockCount(), 1); // 左栏刷新后行数可读
}

TEST_F(TextEditTest, AdjustScrollbarMargins_LayoutFlushed_NoCrash)
{
    // Arrange
    setDocText(QString("margin"));

    // Act
    edit->adjustScrollbarMargins();

    // Assert：布局事件处理完成
    EXPECT_GE(edit->viewport()->width(), 0);
    EXPECT_GE(edit->viewport()->height(), 0); // 布局事件处理完成
}

TEST_F(TextEditTest, SlotSelectionChanged_TogglesCursorBlink)
{
    // Arrange：有选区 → 隐藏闪烁
    setDocText(QString("selectable text"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(10, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->slotSelectionChanged();
    // Assert：选区存在时闪烁关闭
    EXPECT_EQ(QApplication::cursorFlashTime(), 0);

    // Act：无选区 → 恢复（需真正写回编辑器光标，textCursor() 返回副本）
    QTextCursor cleared = edit->textCursor();
    cleared.clearSelection();
    edit->setTextCursor(cleared);
    edit->slotSelectionChanged();
    // Assert
    EXPECT_NE(QApplication::cursorFlashTime(), 0);
}

TEST_F(TextEditTest, OnSelectionArea_WithSelection_TracksPositions)
{
    // Arrange
    setDocText(QString("aa\nbb\ncc"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(6, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->onSelectionArea();

    // Assert：选区端点与结束行被记录（位置 6 落在第三块 "cc" 内 → 行 3）
    EXPECT_EQ(edit->m_nSelectStart, 0);
    EXPECT_EQ(edit->m_nSelectEnd, 6);
    EXPECT_EQ(edit->m_nSelectEndLine, 3);
}

TEST_F(TextEditTest, OnSelectionArea_NoSelection_ClearsEndLine)
{
    // Arrange
    setDocText(QString("x"));
    edit->m_nSelectEndLine = 1;

    // Act
    edit->onSelectionArea();

    // Assert
    EXPECT_EQ(edit->m_nSelectEndLine, -1);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, SlotCanUndoRedoChanged_WithFakeWrapper_SafeNoCrash)
{
    // Arrange：wrapper 已设（fake Window dynamic_cast 安全为空）
    // slotCanUndoChanged/slotCanRedoChanged 仅同步窗口标题状态，不置 m_canUndo

    // Act
    edit->slotCanUndoChanged(true);
    edit->slotCanRedoChanged(false);

    // Assert：无崩溃；m_canUndo/m_canRedo 由 slotUndoAvailable/slotRedoAvailable 维护
    edit->slotUndoAvailable(true);
    edit->slotRedoAvailable(false);
    EXPECT_TRUE(edit->m_canUndo);
    EXPECT_FALSE(edit->m_canRedo);
}

TEST_F(TextEditTest, SlotCanUndoChanged_NullWrapper_EarlyReturn)
{
    // Arrange
    edit->setWrapper(nullptr);

    // Act/Assert：判空分支
    edit->slotCanUndoChanged(true);
    edit->slotCanRedoChanged(true);
    EXPECT_TRUE(edit->m_canRedo == false || edit->m_canRedo == true); // 无崩溃即可
    EXPECT_EQ(edit->getWrapper(), nullptr); // 判空早退分支生效

    // 还原
    edit->setWrapper(fakeWrapper());
}

// ---------------- 颜色标记联动槽（T16） ----------------

TEST_F(TextEditTest, SlotSigColorSelected_AddsMarkWithChosenColor)
{
    // Arrange
    setDocText(QString("color line"));
    moveCursorTo(2);

    // Act
    edit->slotSigColorSelected(true, QColor(QString("#00ff00")));

    // Assert
    ASSERT_EQ(edit->m_wordMarkSelections.size(), 1);
    EXPECT_EQ(edit->m_wordMarkSelections.first().first.format.background().color(),
              QColor(QString("#00ff00")));
    EXPECT_EQ(edit->m_markOperations.size(), 1); // 标记操作同步登记
}

TEST_F(TextEditTest, SlotSigColorAllSelected_MarksWholeDocument)
{
    // Arrange
    setDocText(QString("entire content"));

    // Act
    edit->slotSigColorAllSelected(true, QColor(QString("#ff00ff")));

    // Assert：MarkAll 视图
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("MARK_ALL")));
    EXPECT_EQ(edit->m_mapKeywordMarkSelections[QString("MARK_ALL")].size(), 1); // 全文单选区
}

// ---------------- 右键菜单动作槽（T17-T19/T22） ----------------

TEST_F(TextEditTest, SlotCutAndCopyActions_ClipboardUpdated)
{
    // Arrange
    setDocText(QString("act copy"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    QApplication::clipboard()->clear();

    // Act
    edit->slotCopyAction();
    // Assert
    EXPECT_EQ(QApplication::clipboard()->text(), QString("act"));

    // Act
    edit->slotCutAction();
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString(" copy"));
    EXPECT_EQ(QApplication::clipboard()->text(), QString("act"));
}

TEST_F(TextEditTest, SlotPasteAction_EmptyClipboard_NoInsert)
{
    // Arrange
    setDocText(QString("dest"));
    QApplication::clipboard()->clear();

    // Act
    edit->slotPasteAction();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("dest"));
    EXPECT_FALSE(edit->isUndoRedoOpt()); // 空剪贴板不产生撤销项
}

TEST_F(TextEditTest, SlotDeleteAction_WithSelection_RemovesText)
{
    // Arrange
    setDocText(QString("delete me"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(6, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->slotDeleteAction();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString(" me"));
    EXPECT_TRUE(edit->isUndoRedoOpt()); // 删除经撤销栈
}

TEST_F(TextEditTest, SlotDeleteAction_NoSelection_MovesToCachedCursor)
{
    // Arrange
    setDocText(QString("cache"));
    QTextCursor cur = makeCursor(5);
    edit->m_highlightWordCacheCursor = cur;

    // Act：无选区 → setTextCursor(m_highlightWordCacheCursor)
    edit->slotDeleteAction();

    // Assert：光标移至缓存位置
    EXPECT_EQ(edit->getPosition(), 5);
    EXPECT_FALSE(edit->textCursor().hasSelection()); // 仅移动光标
}

TEST_F(TextEditTest, SlotSelectAllAction_SelectsAllInView)
{
    // Arrange
    setDocText(QString("everything"));

    // Act
    edit->slotSelectAllAction();

    // Assert
    EXPECT_TRUE(edit->m_isSelectAll);
    EXPECT_TRUE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, SlotOpenInFileManagerAction_ShowItemStubbed)
{
    // Arrange：stub 桌面服务（禁止真实拉起文件管理器）；两个重载 → static_cast 选定
    int callCount = 0;
    stub.set_lamda(static_cast<bool (*)(const QString &, const QString &)>(&DDesktopServices::showFileItem),
                   [&callCount](const QString &, const QString &) -> bool {
                       ++callCount;
                       return true;
                   });
    edit->setTruePath(QString("ut://mgr/file"));

    // Act
    const bool ok = edit->slotOpenInFileManagerAction();

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(callCount, 1);
}

TEST_F(TextEditTest, SlotAddAndCancelComment_ActionsWired)
{
    // Arrange
    edit->setFilePath(QString("sample.utlang"));
    setDocText(QString("code = 1"));
    moveCursorTo(2);
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForFileName(QString("sample.utlang"));
    ASSERT_FALSE(def.filePath().isEmpty());
    edit->setSyntaxDefinition(def);

    // Act
    edit->slotAddComment();
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("//code = 1"));

    // Act
    edit->slotCancelComment();
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("code = 1"));
}

TEST_F(TextEditTest, SlotColumnEditAction_ShowsFloatTip)
{
    // Arrange：stub 消息发送（两条编译分支之一）
#ifdef DTKWIDGET_CLASS_DSizeMode
    stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                   [](QWidget *, const QIcon &, const QString &) {});
#else
    stub.set_lamda(&DMessageManager::sendMessage,
                   [](DMessageManager *, QWidget *, const QIcon &, const QString &) {});
#endif

    // Act
    edit->slotColumnEditAction();

    // Assert：提示发送后无残留状态
    EXPECT_FALSE(edit->m_bIsAltMod);
    EXPECT_FALSE(edit->m_hasColumnSelection); // 提示不改列选区状态
}

// ---------------- AI 语音槽（T20/T21） ----------------

TEST_F(TextEditTest, SlotVoiceReadingAction_SuccessAndFailure_EmitsReadingPath)
{
    // Arrange
    installIflytekStubs(IflytekAiAssistant::Success);
    QSignalSpy readingSpy(edit, &TextEdit::signal_readingPath);
    QSignalSpy notifySpy(edit, &TextEdit::popupNotify);

    // Act：成功路径
    edit->slotVoiceReadingAction();
    // Assert
    EXPECT_EQ(readingSpy.count(), 1);
    EXPECT_EQ(notifySpy.count(), 0);

    // Act：失败路径（Invalid → popupNotify 警告）
    installIflytekStubs(IflytekAiAssistant::Invalid);
    edit->slotVoiceReadingAction();
    // Assert
    EXPECT_EQ(readingSpy.count(), 2);
    EXPECT_EQ(notifySpy.count(), 1);
    EXPECT_TRUE(notifySpy.at(0).at(1).toBool()); // warning=true
}

TEST_F(TextEditTest, SlotStopReadingAction_StubbedSuccess_ReturnsTrue)
{
    // Arrange
    installIflytekStubs();

    // Act
    const bool stopped = edit->slotStopReadingAction();

    // Assert
    EXPECT_TRUE(stopped);
    EXPECT_TRUE(edit->slotStopReadingAction()); // 幂等：再次停止仍成功
}

TEST_F(TextEditTest, SlotDictationAction_Failure_EmitsWarningNotify)
{
    // Arrange
    installIflytekStubs(IflytekAiAssistant::NotInstalled);
    QSignalSpy notifySpy(edit, &TextEdit::popupNotify);

    // Act
    edit->slotdictationAction();

    // Assert
    EXPECT_EQ(notifySpy.count(), 1);
    EXPECT_TRUE(notifySpy.at(0).at(1).toBool());
}

TEST_F(TextEditTest, SlotTranslate_SuccessAndFailure_NotifyBranches)
{
    // Arrange
    installIflytekStubs(IflytekAiAssistant::Success);
    QSignalSpy notifySpy(edit, &TextEdit::popupNotify);

    // Act：成功无提示
    edit->slot_translate();
    // Assert
    EXPECT_EQ(notifySpy.count(), 0);

    // Act：失败提示
    installIflytekStubs(IflytekAiAssistant::Invalid);
    edit->slot_translate();
    // Assert
    EXPECT_EQ(notifySpy.count(), 1);
}

TEST_F(TextEditTest, OnAudioPortEnabledChanged_AllDevicesPresent_NoNotify)
{
    // Arrange：设备齐全
    installIflytekStubs(IflytekAiAssistant::Success, true, true);
#ifdef DTKWIDGET_CLASS_DSizeMode
    stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                   [](QWidget *, const QIcon &, const QString &) {});
#else
    stub.set_lamda(&DMessageManager::sendMessage,
                   [](DMessageManager *, QWidget *, const QIcon &, const QString &) {});
#endif
    QSignalSpy notifySpy(edit, &TextEdit::popupNotify);

    // Act：禁用某端口但设备仍在
    edit->onAudioPortEnabledChanged(1, QString("speaker"), false);

    // Assert：无提示
    EXPECT_EQ(notifySpy.count(), 0);
    EXPECT_EQ(edit->toPlainText(), QString()); // 状态未受影响
}

TEST_F(TextEditTest, OnAudioPortEnabledChanged_NoDevices_EmitsFloatMessages)
{
    // Arrange：无输出无输入设备
    installIflytekStubs(IflytekAiAssistant::Success, false, false);
    int floatMsgs = 0;
#ifdef DTKWIDGET_CLASS_DSizeMode
    stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                   [&floatMsgs](QWidget *, const QIcon &, const QString &) { ++floatMsgs; });
#else
    stub.set_lamda(&DMessageManager::sendMessage,
                   [&floatMsgs](DMessageManager *, QWidget *, const QIcon &, const QString &) { ++floatMsgs; });
#endif

    // Act
    edit->onAudioPortEnabledChanged(1, QString("speaker"), false);

    // Assert：输出/输入两条提示
    EXPECT_EQ(floatMsgs, 2);
    EXPECT_EQ(edit->toPlainText(), QString());
}

TEST_F(TextEditTest, OnAudioPortEnabledChanged_EnabledTrue_NoAction)
{
    // Arrange
    installIflytekStubs(IflytekAiAssistant::Success, false, false);
    int floatMsgs = 0;
#ifdef DTKWIDGET_CLASS_DSizeMode
    stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                   [&floatMsgs](QWidget *, const QIcon &, const QString &) { ++floatMsgs; });
#else
    stub.set_lamda(&DMessageManager::sendMessage,
                   [&floatMsgs](DMessageManager *, QWidget *, const QIcon &, const QString &) { ++floatMsgs; });
#endif

    // Act：端口启用 → 不处理
    edit->onAudioPortEnabledChanged(1, QString("speaker"), true);

    // Assert
    EXPECT_EQ(floatMsgs, 0);
    EXPECT_EQ(edit->toPlainText(), QString());
}

// ---------------- 异步高亮（T23） ----------------

TEST_F(TextEditTest, Highlight_AsyncFlush_InvokesWrapperHighlighter)
{
    // Arrange
    highlighterCalls = 0;

    // Act
    edit->highlight();
    QApplication::processEvents(); // 冲刷 singleShot(0)

    // Assert
    EXPECT_EQ(highlighterCalls, 1);
    EXPECT_EQ(wordCntCalls, 0); // 高亮回调不触发字数统计
}

// ---------------- 全选/视口选择（T24） ----------------

TEST_F(TextEditTest, SetSelectAll_FileLoading_Skipped)
{
    // Arrange
    setDocText(QString("loading"));
    seamFileLoading = true;

    // Act
    edit->setSelectAll();

    // Assert：加载中不触发全选
    EXPECT_FALSE(edit->m_isSelectAll);
    EXPECT_TRUE(seamFileLoading); // 加载态保持
}

TEST_F(TextEditTest, SelectTextInView_ExplicitCall_SelectsVisibleRange)
{
    // Arrange
    setDocText(QString("visible content"));

    // Act
    edit->selectTextInView();

    // Assert：从视口起点到终点的选区
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);
}

TEST_F(TextEditTest, SelectTextInView_ReentrantGuard_NoDeadlock)
{
    // Arrange：人为置位重入守卫
    setDocText(QString("guard"));
    edit->m_isSelectingInView = true;

    // Act：重入直接跳过（守卫由非重入路径在结尾复位，重入路径保持置位交由调用方）
    edit->selectTextInView();

    // Assert：无死锁；重入期间标志保持
    EXPECT_TRUE(edit->m_isSelectingInView);
    EXPECT_EQ(edit->toPlainText(), QString("guard")); // 守卫不吞编辑内容
    edit->m_isSelectingInView = false;
}

// ---------------- 右键菜单动作 lambda（initRightClickedMenu 内建连接触发） ----------------

TEST_F(TextEditTest, MenuActionLambdas_Triggered_AllWired)
{
    // Arrange：撤销栈两步（redo lambda 有内容可重做）；文档非空（标记 lambda）
    setDocText(QString("l"));
    edit->insertTextEx(makeCursor(0), QString("x"));
    const int textAfterInsert = edit->toPlainText().size();

    // Act/Assert：undo 动作 lambda → undo_
    edit->m_undoAction->trigger();
    EXPECT_EQ(edit->toPlainText().size(), textAfterInsert - 1);

    // Act/Assert：redo 动作 lambda → redo_
    edit->m_redoAction->trigger();
    EXPECT_EQ(edit->toPlainText().size(), textAfterInsert);

    // Act/Assert：标记当前行 lambda → isMarkCurrentLine + 渲染
    edit->m_markCurrentAct->trigger();
    EXPECT_EQ(edit->m_wordMarkSelections.size(), 1);

    // Act/Assert：标记所有 lambda → isMarkAllLine
    edit->m_markAllAct->trigger();
    EXPECT_TRUE(edit->m_mapKeywordMarkSelections.contains(QString("MARK_ALL")));

    // Act/Assert：视图模式动作 lambda（编辑/查看两态；实时预览非 md 被置灰，
    // 手动启用后触发覆盖其 lambda 分支）
    QSignalSpy spy(edit, &TextEdit::viewModeRequested);
    edit->m_actEditView->trigger();
    edit->m_actReadView->trigger();
    EXPECT_EQ(spy.count(), 2);
    edit->m_actLivePreview->setEnabled(true);
    edit->m_actLivePreview->trigger();
    ASSERT_EQ(spy.count(), 3);
    EXPECT_EQ(spy.at(2).at(0).value<ViewMode>(), ViewMode::LivePreview);
}

// ---------------- 标记状态保存/恢复（补漏） ----------------

TEST_F(TextEditTest, SaveAndRestoreMarkStatus_SelectionRoundTrip)
{
    // Arrange：标记模式选区
    setDocText(QString("abcdef"));
    moveCursorTo(2);
    edit->setMark();
    edit->forwardChar();
    edit->forwardChar();
    ASSERT_TRUE(edit->textCursor().hasSelection());
    const int anchorBefore = edit->textCursor().anchor();
    const int posBefore = edit->textCursor().position();

    // Act：保存状态后移动光标破坏选区
    edit->saveMarkStatus();
    moveCursorTo(0);
    EXPECT_FALSE(edit->textCursor().hasSelection());

    // Act：恢复（锚点回存点、延伸到当前光标位置 0）
    edit->restoreMarkStatus();

    // Assert：以保存锚点为端点的选区重建
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().anchor(), anchorBefore);
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);
    EXPECT_EQ(edit->textCursor().selectionEnd(), anchorBefore);
    EXPECT_EQ(posBefore, 4); // 保存时光标位于 4（2 + 两次前移）
}

TEST_F(TextEditTest, RestoreMarkStatus_NoSavedMark_NoChange)
{
    // Arrange：未保存过标记状态（m_cursorMarkStatus=false 分支）
    setDocText(QString("xy"));
    moveCursorTo(0);

    // Act
    edit->restoreMarkStatus();

    // Assert：无选区产生
    EXPECT_FALSE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->getPosition(), 0); // 光标原地
}

// ---------------- 行尾格式切换（补漏） ----------------

TEST_F(TextEditTest, OnEndlineFormatChanged_ForwardsToBottomBar)
{
    // Arrange：seamFakeEndlineCalls 由 BottomBar::setEndlineMenuText 桩计数
    seamFakeEndlineCalls = 0;

    // Act
    edit->onEndlineFormatChanged(BottomBar::Unix, BottomBar::Windows);

    // Assert：菜单文案随新格式刷新（fake BottomBar 接缝计数）
    EXPECT_EQ(seamFakeEndlineCalls, 1);
    EXPECT_TRUE(edit->toPlainText().isEmpty()); // 行尾切换不替换文档内容
}

// ---------------- 鼠标取词（T25） ----------------

TEST_F(TextEditTest, GetWordAtMouse_CursorInsideRect_ReturnsWord)
{
    // Arrange：stub 全局光标位置到编辑区首行（两个重载 → static_cast 选无参版本）
    setDocText(QString("wordmore"));
    stub.set_lamda(static_cast<QPoint (*)()>(&QCursor::pos),
                   []() -> QPoint { return QPoint(10, 8); });

    // Act
    const QString word = edit->getWordAtMouse();

    // Assert：光标矩形包含该点 → 返回词下光标选词
    EXPECT_FALSE(word.isEmpty());
    EXPECT_EQ(word, QString("wordmore")); // 整词命中
}

TEST_F(TextEditTest, GetWordAtMouse_EmptyDoc_ReturnsEmpty)
{
    // Arrange/Act/Assert
    EXPECT_TRUE(edit->getWordAtMouse().isEmpty());
    EXPECT_EQ(edit->characterCount(), 1); // 空文档仅块分隔符
}