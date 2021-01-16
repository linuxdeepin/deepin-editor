 /* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEXTEDIT_H
#define TEXTEDIT_H
#include "linenumberarea.h"
#include "bookmarkwidget.h"
#include "FlashTween.h"
#include "codeflodarea.h"
#include "../common/settings.h"
#include "../widgets/ColorSelectWdg.h"
#include "uncommentselection.h"
//添加自定义撤销重做栈
#include "inserttextundocommand.h"
#include "deletetextundocommand.h"
#include <QUndoStack>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Repository>
#include <QAction>
#include <DMenu>
#include <QPaintEvent>
#include <DPlainTextEdit>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QFont>
#include <DApplicationHelper>
#include <QtDBus>
#include <QGestureEvent>
#include <QProxyStyle>


const QString SELECT_HIGHLIGHT_COLOR = "#2CA7F8";
enum ConvertCase { UPPER, LOWER, CAPITALIZE };

class ShowFlodCodeWidget;
class LeftAreaTextEdit;
class EditWrapper;

class TextEdit : public DPlainTextEdit
{
    Q_OBJECT

public:
    enum CursorMode {
        Insert,
        Overwrite,
        Readonly
    };

    TextEdit(QWidget *parent = nullptr);
    ~TextEdit() override;

    //在光标处添加删除内容

    //直接插入文本
    void insertTextEx(QTextCursor,QString);

    //直接删除字符 删除选择或一个字符
    void deleteTextEx(QTextCursor);

    //插入选择文本字符
    void insertSelectTextEx(QTextCursor,QString);

    //插入列编辑文本字符
    void insertColumnEditTextEx(QString text);
    //插入带选择字符
    void deleteSelectTextEx(QTextCursor);

    //初始化右键菜单
    void initRightClickedMenu();

    //弹窗右键菜单
    void popRightMenu(QPoint pos = QPoint());
    //
    void setWrapper(EditWrapper *);

    /**
     * @brief getFilePath 获取打开文件路径
     * @return 打开文件路径
     */
    inline QString getFilePath() { return m_sFilePath;};
    //
    inline void setFilePath(QString file) { m_sFilePath = file;}
    //
    inline LeftAreaTextEdit* getLeftAreaWidget() { return m_pLeftAreaWidget;};
    //是否撤销重做操作
    bool isUndoRedoOpt() {return (m_pUndoStack->canRedo()||m_pUndoStack->canUndo());};
    //判断是否修改
    bool getModified() { return (document()->isModified() && isUndoRedoOpt());}

    int getCurrentLine();
    int getCurrentColumn();
    int getPosition();
    int getScrollOffset();
    DMenu *getHighlightMenu();


    void forwardChar();
    void backwardChar();
    void forwardWord();
    void backwardWord();
    void forwardPair();
    void backwardPair();

    int blockCount() const;
    int characterCount() const;
    QTextBlock firstVisibleBlock();

    void moveToStart();
    void moveToEnd();
    void moveToStartOfLine();
    void moveToEndOfLine();
    void moveToLineIndentation();
    void nextLine();
    void prevLine();
    void jumpToLine(int line, bool keepLineAtCenter);

    void moveCursorNoBlink(QTextCursor::MoveOperation operation,
                           QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    void newline();
    void openNewlineAbove();
    void openNewlineBelow();
    void moveLineDownUp(bool up);
    void scrollLineUp();
    void scrollLineDown();
    void scrollUp();
    void scrollDown();
    void duplicateLine();
    void copyLines();
    void cutlines();

    /**
     * @brief joinLines 合并行
     */
    void joinLines();

    void killLine();
    void killCurrentLine();
    void killBackwardWord();
    void killForwardWord();

    void escape();
    void indentText();
    void unindentText();
    void setTabSpaceNumber(int number);
    void convertWordCase(ConvertCase convertCase);
    QString capitalizeText(QString text);

    void keepCurrentLineAtCenter();
    void scrollToLine(int scrollOffset, int row, int column);

    void setLineWrapMode(bool enable);
    void setFontFamily(QString fontName);
    void setFontSize(int fontSize);
    void updateFont();

    void replaceAll(const QString &replaceText, const QString &withText);
    void replaceNext(const QString &replaceText, const QString &withText);
    void replaceRest(const QString &replaceText, const QString &withText);
    void beforeReplace(QString _);

    bool findKeywordForward(const QString &keyword);

    void removeKeywords();
    bool highlightKeyword(QString keyword, int position);
    void updateCursorKeywordSelection(int position, bool findNext);
    void updateHighlightLineSelection();
    bool updateKeywordSelections(QString keyword,QTextCharFormat charFormat,QList<QTextEdit::ExtraSelection> *listSelection);
    void renderAllSelections();


    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void codeFLodAreaPaintEvent(QPaintEvent *event);
    void setBookmarkFlagVisable(bool isVisable,bool bIsFirstOpen = false);
    void setCodeFlodFlagVisable(bool isVisable,bool bIsFirstOpen = false);
    void setTheme(const QString &path);
    bool highlightWordUnderMouse(QPoint pos);
    void removeHighlightWordUnderCursor();
    void setSettings(Settings *settings);
    //设置tabbar修改状态是否带*,不改变document 修改属性
    void setTabbarModified(bool modified);
    void copySelectedText();
    void cutSelectedText();
    void pasteText();
    void setMark();
    void unsetMark();
    bool tryUnsetMark();
    void exchangeMark();
    void saveMarkStatus();
    void restoreMarkStatus();
    void completionWord(QString word);
    QString getWordAtMouse();
    QString getWordAtCursor();
    void toggleReadOnlyMode();
    void toggleComment(bool sister);
    int getNextWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode);
    int getPrevWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode);
    bool atWordSeparator(int position);
    void showCursorBlink();
    void hideCursorBlink();
    void setReadOnlyPermission(bool permission);
    bool getReadOnlyPermission();
    bool getReadOnlyMode();
    void hideRightMenu();
    void clearBlack();
    void flodOrUnflodAllLevel(bool isFlod);
    void flodOrUnflodCurrentLevel(bool isFlod);
    void getHideRowContent(int iLine);
    bool isNeedShowFoldIcon(QTextBlock block);
    int  getHighLightRowContentLineNum(int iLine);

    /**
     * @author liumaochuan ut000616
     * @brief getLinePosYByLineNum 根据行号获得行Y轴坐标
     * @param iLine 行
     * @return 行Y轴坐标
     */
    int  getLinePosYByLineNum(int iLine);
    bool ifHasHighlight();

    //书签功能相关
    /**
     * @author liumaochuan ut000616
     * @brief bookMarkAreaPaintEvent 绘制书签
     * @param event 书签区域的绘制事件
     */
    void bookMarkAreaPaintEvent(QPaintEvent *event);

    /**
     * @author liumaochuan ut000616
     * @brief getLineFromPoint 得到鼠标点击位置所在的行
     * @param point 鼠标点击位置
     * @return 鼠标点击位置所在的行
     */
    int getLineFromPoint(const QPoint &point);

    /**
     * @author liumaochuan ut000616
     * @brief moveToPreviousBookMark 移动到上一个书签
     */
    void moveToPreviousBookMark();

    /**
     * @author liumaochuan ut000616
     * @brief moveToNextBookMark 移动到下一个书签
     */
    void moveToNextBookMark();

    /**
     * @author liumaochuan ut000616
     * @brief checkBookmarkLineMove 检测书签行移动
     * @param from 文本变化时光标位置
     * @param charsRemoved 移除的字符数
     * @param charsAdded 添加的字符数
     */
    void checkBookmarkLineMove(int from, int charsRemoved, int charsAdded);

    /**
     * @author liumaochuan ut000616
     * @brief setIsFileOpen 设置是否在读取文件
     */
    void setIsFileOpen();

    /**
     * @author liumaochuan ut000616
     * @brief setTextFinished 读取文件结束
     */
    void setTextFinished();

    /**
     * @author liumaochuan ut000616
     * @brief readHistoryRecord 读取书签相关记录
     * @return 书签相关记录列表
     */
    QStringList readHistoryRecord(QString key);

    /**
     * @author liumaochuan ut000616
     * @brief readHistoryRecordofBookmark 读取书签记录
     * @return 书签记录列表
     */
    QStringList readHistoryRecordofBookmark();

    /**
     * @author liumaochuan ut000616
     * @brief readHistoryRecordofFilePath 读取添加了书签的文件路径记录
     * @return 文件路径列表
     */
    QStringList readHistoryRecordofFilePath(QString key);

    /**
     * @author liumaochuan ut000616
     * @brief writeHistoryRecord 写入书签相关记录
     */
    void writeHistoryRecord();

    //标记功能相关
    /**
     * @author liumaochuan ut000616
     * @brief isMarkCurrentLine 标记或取消标记当前行
     * @param isMark true为标记，false为取消标记
     * @param strColor 标记格式
     */
    void isMarkCurrentLine(bool isMark, QString strColor = "");

    /**
     * @author liumaochuan ut000616
     * @brief isMarkAllLine 标记或取消标记所有
     * @param isMark true为标记，false为取消标记
     * @param strColor 标记格式
     */
    void isMarkAllLine(bool isMark, QString strColor = "");

    /**
     * @author liumaochuan ut000616
     * @brief cancelLastMark 取消上一个标记
     */
    void cancelLastMark();

    /**
     * @brief markSelectWord 标记选择的文本
     */
    void markSelectWord();

    /**
     * @author liumaochuan ut000616
     * @brief updateMark 更新标记
     * @param from 文本变化时光标位置
     * @param charsRemoved 移除的字符数
     * @param charsAdded 添加的字符数
     */
    void updateMark(int from, int charsRemoved, int charsAdded);

    //QTextEdit :: ExtraSelection结构提供了一种为文档中的给定选择,指定字符格式的方法。
    /**
     * @author liumaochuan ut000616
     * @brief containsExtraSelection 指定字符格式列表是否包含该指定字符格式
     * @param listSelections 指定字符格式列表
     * @param selection 指定字符格式
     * @return true or false
     */
    bool containsExtraSelection(QList<QTextEdit::ExtraSelection> listSelections, QTextEdit::ExtraSelection selection);

    /**
     * @author liumaochuan ut000616
     * @brief appendExtraSelection 在指定字符格式列表添加指定字符格式
     * @param wordMarkSelections 指定字符格式列表
     * @param selection 指定字符格式
     * @param markColor 指定字符颜色格式
     * @param listSelections 添加的指定字符格式列表
     */
    void appendExtraSelection(QList<QTextEdit::ExtraSelection> wordMarkSelections, QTextEdit::ExtraSelection selection
                              , QString strColor, QList<QTextEdit::ExtraSelection> *listSelections);

    void setCursorStart(int pos);
    void writeEncodeHistoryRecord();
    QStringList readEncodeHistoryRecord();
    /**
     * @brief tellFindBarClose 通知查找框关闭
     */
    void tellFindBarClose();
    /**
     * @author liumaochuan ut000616
     * @brief setEditPalette 设置textEdit的颜色
     * @param activeColor active时的颜色
     * @param inactiveColor inactive时的颜色
     */
    void setEditPalette(const QString &activeColor, const QString &inactiveColor);
    /**
     * @author liumaochuan ut000616
     * @brief setCodeFoldWidgetHide 代码折叠悬浮预览
     * @param isHidden 是否隐藏
     */
    void setCodeFoldWidgetHide(bool isHidden);

    /**
     * @brief setTruePath 设置真实文件路径
     * @param qstrTruePath　真实文件路径
     */
    void setTruePath(QString qstrTruePath);

    /**
     * @brief getTruePath 获取真实文件路径
     * @return 真实文件路径
     */
    QString getTruePath();

    /**
     * @brief getBookmarkInfo 得到书签信息
     * @return 书签信息
     */
    QList<int> getBookmarkInfo();

    /**
     * @brief setBookMarkList 设置书签
     * @param bookMarkList 书签列表
     */
    void setBookMarkList(QList<int> bookMarkList);

signals:
    void clickFindAction();
    void clickReplaceAction();
    void clickJumpLineAction();
    void clickFullscreenAction();
    void cursorMarkChanged(bool mark, QTextCursor cursor);
    void cursorModeChanged(CursorMode mode);
    void hightlightChanged(const QString &name);
    void popupNotify(QString notify);
    void signal_readingPath();
    void signal_setTitleFocus();
public slots:
    /**
     * @author liumaochuan ut000616
     * @brief addOrDeleteBookMark 添加或删除书签
     */
    void addOrDeleteBookMark();
    //更新左边区域界面　梁卫东　２０２０－０９－０９　１３：５３：５８
    void updateLeftAreaWidget();
    void handleScrollFinish();
    void setSyntaxDefinition(KSyntaxHighlighting::Definition def);
    
    void slot_translate();
	
	//书签右键菜单功能
    void setHighLineCurrentLine(bool ok);
    void upcaseWord();
    void downcaseWord();
    void capitalizeWord();
    void transposeChar();

    void handleCursorMarkChanged(bool mark, QTextCursor cursor);

    void adjustScrollbarMargins();
    void onSelectionArea();
    void fingerZoom(QString name, QString direction, int fingers);
protected:
    bool event(QEvent* evt) override;   //触摸屏event事件
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
private:
    void unCommentSelection();
    void setComment();
    void removeComment();

    //去除"*{*" "*}*" "*{*}*"跳过当做普通文本处理不折叠　梁卫东２０２０－０９－０１　１７：１６：４１
    bool blockContainStrBrackets(int line);
    bool setCursorKeywordSeletoin(int position, bool findNext);
    void cursorPositionChanged();
    void updateHighlightBrackets(const QChar &openChar, const QChar &closeChar);

    void getNeedControlLine(int line, bool isVisable);
    //触摸屏功能函数
    bool gestureEvent(QGestureEvent *event);
    void tapGestureTriggered(QTapGesture*);
    void tapAndHoldGestureTriggered(QTapAndHoldGesture*);
    void panTriggered(QPanGesture*);
    void pinchTriggered(QPinchGesture*);
    void swipeTriggered(QSwipeGesture*);
    //add for single refers to the sliding
    void slideGestureY(qreal diff);
    void slideGestureX(qreal diff);
public:
    int getFirstVisibleBlockId() const;
public:
    bool bIsSetLineNumberWidth = true;
    bool m_pIsShowCodeFoldArea;
    bool m_pIsShowBookmarkArea;

private:
    EditWrapper *m_wrapper;
    QPropertyAnimation *m_scrollAnimation;

    QList<QTextEdit::ExtraSelection> m_findMatchSelections;///< “查找”的字符格式（所有查找的字符）
    QTextEdit::ExtraSelection m_beginBracketSelection;
    QTextEdit::ExtraSelection m_endBracketSelection;
    QTextEdit::ExtraSelection m_currentLineSelection;
    QTextEdit::ExtraSelection m_findHighlightSelection;///< “查找”的字符格式（当前位置字符）
    QTextEdit::ExtraSelection m_wordUnderCursorSelection;
    QList<QTextEdit::ExtraSelection> m_wordMarkSelections;///< 记录标记的列表（分行记录）
    QMap<int,QList<QTextEdit::ExtraSelection>> m_mapWordMarkSelections;///< 记录标记的表（按标记动作记录）
    QTextEdit::ExtraSelection m_markAllSelection;///< “标记所有”的字符格式
    QList<QTextEdit::ExtraSelection> m_markFoldHighLightSelections;

    QTextCursor m_highlightWordCacheCursor;
    QTextCursor m_wordUnderPointerCursor;

    int m_lineNumberPaddingX = 5;

    int m_restoreColumn;
    int m_restoreRow;

    int m_tabSpaceNumber = 4;

    KSyntaxHighlighting::Repository m_repository;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter = nullptr;

    DMenu *m_rightMenu;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_deleteAction;
    QAction *m_selectAllAction;
    QAction *m_findAction;
    QAction *m_replaceAction;
    QAction *m_jumpLineAction;
    QAction *m_enableReadOnlyModeAction;
    QAction *m_disableReadOnlyModeAction;
    QAction *m_fullscreenAction;
    QAction *m_exitFullscreenAction;
    QAction *m_openInFileManagerAction;
    QAction *m_toggleCommentAction;
    QAction *m_voiceReadingAction;
    QAction *m_stopReadingAction;
    QAction *m_dictationAction;
    QAction *m_translateAction;
    QAction *m_columnEditAction;

    QAction *m_addBookMarkAction;
    QAction *m_cancelBookMarkAction;
    QAction *m_clearBookMarkAction;
    QAction *m_preBookMarkAction;
    QAction *m_nextBookMarkAction;
    QAction *m_flodAllLevel;
    QAction *m_unflodAllLevel;
    QAction *m_flodCurrentLevel;
    QAction *m_unflodCurrentLevel;

    //yanyuhan
     //颜色标记、折叠/展开、书签、列编辑、设置注释、取消注释;
 //    QAction *m_colorMarkAction;
    DMenu *m_collapseExpandMenu;
    DMenu *m_colorMarkMenu;
    QAction *m_cancleMarkCurrentLine;
    QAction *m_cancleMarkAllLine;
    QAction *m_cancleLastMark;

    //颜色选择控件替换下面action 1 2 3 4
    QWidgetAction *m_actionColorStyles;
    QAction *m_markCurrentAct;

     //颜色选择控件替换下面action 1 2 3 4
    QWidgetAction *m_actionAllColorStyles;
    QAction *m_markAllAct;

    QAction *m_addComment;
    QAction *m_cancelComment;

    DMenu *m_convertCaseMenu;
    QAction *m_upcaseAction;
    QAction *m_downcaseAction;
    QAction *m_capitalizeAction;

    ShowFlodCodeWidget  *m_foldCodeShow;

    bool m_canUndo;
    bool m_canRedo;
    bool m_HightlightYes;
    bool m_haveWordUnderCursor;

    bool m_cursorMark = false;
    int m_markStartLine = -1;

    Settings *m_settings;

    bool m_readOnlyMode = false;
    bool m_cursorMarkStatus = false;
    int m_cursorMarkPosition = 0;
    int m_cursorWidthChangeDelay = 2000;
    bool m_bReadOnlyPermission = false;

    int m_fontSize = 16;
    QString m_fontName;

    Comment::CommentDefinition m_commentDefinition;

    QStringList m_wordSepartors = QStringList({
            // English separator.
            ".", ",", "?", "!", "@", "#", "$", ":", ";", "-", "<", ">", "[", "]", "(", ")", "{", "}", "=", "/", "+", "%", "&", "^", "*", "\"", "'", "`", "~", "|", "\\", "\n",
            // Chinese separator.
            "。", "，", "？", "！", "￥", "：", "；", "《", "》", "【", "】", "（", "）", " "
        });

    QColor m_currentLineColor;
    QColor m_backgroundColor;
    QColor m_lineNumbersColor;
    QColor m_currentLineNumberColor;
    QColor m_regionMarkerColor;
    QColor m_selectionColor;
    QColor m_selectionBgColor;

    QPoint m_mouseClickPos;///< 鼠标点击位置
    QPoint m_menuPos;

    bool m_highlighted = false;

    QTextCharFormat m_bracketMatchFormat;
    QTextCharFormat m_findMatchFormat;
    QTextCharFormat m_findHighlightFormat;
    CursorMode m_cursorMode;

    DMenu *m_hlGroupMenu;
    QActionGroup *m_hlActionGroup;

    QPoint m_lastTouchBeginPos;
    QPointer<QTimer> m_updateEnableSelectionByMouseTimer;
    int m_touchTapDistance = -1;

    QFont m_fontLineNumberArea;///< 绘制行号的字体
    QList<int> m_listBookmark;///< 存储书签的list
    int m_nBookMarkHoverLine;///< 悬浮效果书签所在的行
    int m_nLines;///< 文本总行数
    bool m_bIsFileOpen;///< 是否在读取文件（导致文本变化）
    bool m_bIsShortCut;///< 是否在使用书签快捷键

    //存储所有有折叠标记的位置，包含不可见区域
    QList<int> m_listFlodFlag;
    //包含当前可见区域的标志
    QList<int> m_listFlodIconPos;

    QString m_qstrCommitString;///< 输入法输入的字符
    bool m_bIsInputMethod;///< 是否是输入法输入
    int m_nSelectEndLine;///< 选择结束时后鼠标所在行
    int m_nSelectStart;///< 选择开始时的鼠标位置
    int m_nSelectEnd;///< 选择结束时的鼠标位置

    int m_cursorStart=-1;
    QString m_textEncode;

    //触摸屏
    enum GestureAction{
        GA_null,
        GA_tap,
        GA_slide,
        GA_pinch,
        GA_hold,
        GA_pan,
        GA_swipe
    };

    qreal m_scaleFactor = 1;
    qreal m_currentStepScaleFactor = 1;
    qint64 m_tapBeginTime = 0;
    Qt::GestureState m_tapStatus = Qt::NoGesture;
    GestureAction m_gestureAction = GA_null;

	//add for single refers to the sliding
    FlashTween tweenX;
    FlashTween tweenY;
    qreal changeY = {0.0};
    qreal changeX = {0.0};
    qreal durationY = {0.0};
    qreal durationX = {0.0};
    bool m_slideContinueX {false};
    bool m_slideContinueY {false};
    int m_lastMouseYpos;
    int m_lastMouseXpos;
    ulong m_lastMouseTimeX;
    ulong m_lastMouseTimeY;
    qreal m_stepSpeedY = 0;
    qreal m_stepSpeedX = 0;
    bool m_bIsDoubleClick {false};
    bool m_bBeforeIsDoubleClick {false};

    QList<QTextEdit::ExtraSelection> m_altModSelections;
    QTextCursor m_altStartTextCursor;//开始按住alt鼠标点击光标位置
    QTextCursor m_altEndTextCursor;//结束按住alt鼠标点击光标位置
    bool m_bIsAltMod=false;
    int m_redoCount = 0;
    QStringList m_pastText;
    bool m_hasColumnSelection= false;

    //鼠标事件的位置
    int m_startX = 0;
    int m_startY = 0;
    int m_endX = 0;
    int m_endY = 0;

    bool m_bIsFindClose = false;///< 关闭查找框事件是否发生
    QString m_qstrTruePath;///< 源文件路径

private:
    LeftAreaTextEdit *m_pLeftAreaWidget = nullptr;
    QString m_sFilePath;///＜打开文件路径
    //自定义撤销重做栈
    QUndoStack* m_pUndoStack = nullptr;
};
#endif
