/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#include <DSettings>

void GenerateSettingTranslate()
{
    auto group_baseName = QObject::tr("Basic");
    auto base_fontName = QObject::tr("Font Style");
    auto base_font_familyName = QObject::tr("Font:");
    auto base_font_sizeName = QObject::tr("Font Size:");
    auto group_shortcutsName = QObject::tr("Shortcuts");
    auto shortcuts_keymapName = QObject::tr("Keymap");
    auto shortcurs_keymap_keymap = QObject::tr("Keymap:");
    auto shortcuts_windowName = QObject::tr("Window");
    auto shortcuts_window_addblanktabName = QObject::tr("New tab:");
    auto shortcuts_window_newwindowName = QObject::tr("New window:");
    auto shortcuts_window_savefileName = QObject::tr("Save:");
    auto shortcuts_window_saveasfileName = QObject::tr("Save as:");
    auto shortcuts_window_selectnexttabName = QObject::tr("Next tab:");
    auto shortcuts_window_selectprevtabName = QObject::tr("Previous tab:");
    auto shortcuts_window_closetabName = QObject::tr("Close tab:");
    auto shortcuts_window_closeothertabsName = QObject::tr("Close other tabs:");
    auto shortcuts_window_restoretabName = QObject::tr("Restore tab:");
    auto shortcuts_window_openfileName = QObject::tr("Open file:");
    auto shortcuts_window_incrementfontsizeName = QObject::tr("Increase font size:");
    auto shortcuts_window_decrementfontsizeName = QObject::tr("Decrease font size:");
    auto shortcuts_window_resetfontsizeName = QObject::tr("Reset font size:");
    auto shortcuts_window_togglefullscreenName = QObject::tr("Toggle fullscreen:");
    auto shortcuts_window_findName = QObject::tr("Find:");
    auto shortcuts_window_replaceName = QObject::tr("Replace:");
    auto shortcuts_window_jumptolineName = QObject::tr("Go to line:");
    auto shortcuts_window_savepositionName = QObject::tr("Save cursor position:");
    auto shortcuts_window_restorepositionName = QObject::tr("Reset cursor position:");
    auto shortcuts_window_escapeName = QObject::tr("Exit:");
    auto shortcuts_window_displayshortcutsName = QObject::tr("Display shortcuts:");
    auto shortcuts_window_printName = QObject::tr("Print:");
    auto group_editorName = QObject::tr("Editor");
    auto shortcuts_editor_indentlineName = QObject::tr("Increase indent:");
    auto shortcuts_editor_backindentlineName = QObject::tr("Decrease indent:");
    auto shortcuts_editor_forwardcharName = QObject::tr("Forward character:");
    auto shortcuts_editor_backwardcharName = QObject::tr("Backward character:");
    auto shortcuts_editor_forwardwordName = QObject::tr("Forward word:");
    auto shortcuts_editor_backwarwordName = QObject::tr("Backward word:");
    auto shortcuts_editor_nextlineName = QObject::tr("Next line:");
    auto shortcuts_editor_prevlineName = QObject::tr("Previous line:");
    auto shortcuts_editor_newlineName = QObject::tr("New line:");
    auto shortcuts_editor_opennewlineaboveName = QObject::tr("New line above:");
    auto shortcuts_editor_opennewlinebelowName = QObject::tr("New line below:");
    auto shortcuts_editor_duplicatelineName = QObject::tr("Duplicate line:");
    auto shortcuts_editor_killlineName = QObject::tr("Delete to end of line:");
    auto shortcuts_editor_killcurrentlineName = QObject::tr("Delete current line:");
    auto shortcuts_editor_swaplineupName = QObject::tr("Swap line up:");
    auto shortcuts_editor_swaplinedownName = QObject::tr("Swap line down:");
    auto shortcuts_editor_scrolllineupName = QObject::tr("Scroll up one line:");
    auto shortcuts_editor_scrolllinedownName = QObject::tr("Scroll down one line:");
    auto shortcuts_editor_scrollupName = QObject::tr("Page up:");
    auto shortcuts_editor_scrolldownName = QObject::tr("Page down:");
    auto shortcuts_editor_movetoendoflineName = QObject::tr("Move to end of line:");
    auto shortcuts_editor_movetostartoflineName = QObject::tr("Move to start of line:");
    auto shortcuts_editor_movetoendName = QObject::tr("Move to end of text:");
    auto shortcuts_editor_movetostartName = QObject::tr("Move to start of text:");
    auto shortcuts_editor_movetolineindentationName = QObject::tr("Move to line indentation:");
    auto shortcuts_editor_upcasewordName = QObject::tr("Upper case:");
    auto shortcuts_editor_downcasewordName = QObject::tr("Lower case:");
    auto shortcuts_editor_capitalizewordName = QObject::tr("Capitalize:");
    auto shortcuts_editor_killbackwardwordName = QObject::tr("Delete backward word:");
    auto shortcuts_editor_killforwardwordName = QObject::tr("Delete forward word:");
    auto shortcuts_editor_forwardpairName = QObject::tr("Forward over a pair:");
    auto shortcuts_editor_backwardpairName = QObject::tr("Backward over a pair:");
    auto shortcuts_editor_selectallName = QObject::tr("Select all:");
    auto shortcuts_editor_copyName = QObject::tr("Copy:");
    auto shortcuts_editor_cutName = QObject::tr("Cut:");
    auto shortcuts_editor_pasteName = QObject::tr("Paste:");
    auto shortcuts_editor_transposecharName = QObject::tr("Transpose character:");
    auto shortcuts_editor_setmarkName = QObject::tr("Mark:");
    auto shortcuts_editor_exchangemarkName = QObject::tr("Unmark:");
    auto shortcuts_editor_copylinesName = QObject::tr("Copy line:");
    auto shortcuts_editor_cutlinesName = QObject::tr("Cut line:");
    auto shortcuts_editor_joinlinesName = QObject::tr("Merge lines:");
    auto shortcuts_editor_togglereadonlymodeName = QObject::tr("Read-Only mode:");
    auto shortcuts_editor_togglecommentName = QObject::tr("Toggle comment:");
    auto shortcuts_editor_undoName = QObject::tr("Undo:");
    auto shortcuts_editor_redoName = QObject::tr("Redo:");
    auto group_advanceName = QObject::tr("Advanced");
    auto advance_windowName = QObject::tr("Window");
    auto advance_window_windowstateName = QObject::tr("Window size:");
    auto advance_editor_tabspacenumberName = QObject::tr("Tab width:");

    auto base_font_wordwrapText = QObject::tr("Word Wrap");
}
