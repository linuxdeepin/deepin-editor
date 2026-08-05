# AT-SPI 扫描报告 - deepin-editor

## 扫描信息

- **仓库**: linuxdeepin/deepin-editor
- **分支**: master
- **扫描日期**: 2026-08-05
- **扫描方法**: 源码静态分析（人工审查头文件与实现文件）
- **扫描范围**: src/ 目录下所有头文件和实现文件

## 结果摘要

| 指标 | 数值 |
|------|------|
| 待补全控件数 | 37 |
| 涉及文件数 | 11 |
| 补全前 setAccessibleName 调用数 | 0 |
| 补全后 setAccessibleName 预期调用数 | 37 |

## AT-SPI 组件缺口清单

### 1. Window (src/widgets/window.h / window.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 1 | Window 主窗口 | DMainWindow | "EditorWindow" | window.cpp 构造函数 |

### 2. Tabbar (src/controls/tabbar.h / tabbar.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 2 | Tabbar | DTabBar | "EditorTabbar" | tabbar.cpp 构造函数 |
| 3 | AddButton | DIconButton | "AddTabButton" | window.cpp initTitlebar() |

### 3. FindBar (src/controls/findbar.h / findbar.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 4 | FindBar 面板 | DFloatingWidget | "FindBar" | findbar.cpp 构造函数 |
| 5 | 查找输入框 m_editLine | LineBar | "FindInput" | findbar.cpp 构造函数 |
| 6 | 查找下一个按钮 | QPushButton | "FindNextButton" | findbar.cpp 构造函数 |
| 7 | 查找上一个按钮 | QPushButton | "FindPrevButton" | findbar.cpp 构造函数 |
| 8 | 替换切换按钮 | QPushButton | "SwitchToReplaceButton" | findbar.cpp 构造函数 |
| 9 | 关闭按钮 | DIconButton | "FindBarCloseButton" | findbar.cpp 构造函数 |
| 10 | 查找标签 m_findLabel | QLabel | "FindLabel" | findbar.cpp 构造函数 |

### 4. ReplaceBar (src/controls/replacebar.h / replacebar.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 11 | ReplaceBar 面板 | DFloatingWidget | "ReplaceBar" | replacebar.cpp 构造函数 |
| 12 | 查找输入框 m_replaceLine | LineBar | "ReplaceFindInput" | replacebar.cpp 构造函数 |
| 13 | 替换输入框 m_withLine | LineBar | "ReplaceWithInput" | replacebar.cpp 构造函数 |
| 14 | 替换按钮 m_replaceButton | QPushButton | "ReplaceButton" | replacebar.cpp 构造函数 |
| 15 | 全部替换按钮 | QPushButton | "ReplaceAllButton" | replacebar.cpp 构造函数 |
| 16 | 替换剩余按钮 | QPushButton | "ReplaceRestButton" | replacebar.cpp 构造函数 |
| 17 | 跳过按钮 | QPushButton | "SkipButton" | replacebar.cpp 构造函数 |
| 18 | 关闭按钮 | DIconButton | "ReplaceBarCloseButton" | replacebar.cpp 构造函数 |
| 19 | 替换标签 m_replaceLabel | QLabel | "ReplaceLabel" | replacebar.cpp 构造函数 |
| 20 | 替换为标签 m_withLabel | QLabel | "WithLabel" | replacebar.cpp 构造函数 |

### 5. JumpLineBar (src/controls/jumplinebar.h / jumplinebar.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 21 | JumpLineBar 面板 | DFloatingWidget | "JumpLineBar" | jumplinebar.cpp 构造函数 |
| 22 | 行号输入框 | DSpinBox | "JumpLineInput" | jumplinebar.cpp 构造函数 |
| 23 | 关闭按钮 | DIconButton | "JumpLineCloseButton" | jumplinebar.cpp 构造函数 |
| 24 | 标签 m_label | QLabel | "JumpLineLabel" | jumplinebar.cpp 构造函数 |

### 6. ThemePanel (src/thememodule/themepanel.h / themepanel.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 25 | ThemePanel 面板 | QWidget | "ThemePanel" | themepanel.cpp 构造函数 |

### 7. BottomBar (src/widgets/bottombar.h / bottombar.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 26 | BottomBar 底栏 | QWidget | "BottomBar" | bottombar.cpp 构造函数 |
| 27 | 位置标签 | DLabel | "PositionLabel" | bottombar.cpp 构造函数 |
| 28 | 字符数标签 | DLabel | "CharCountLabel" | bottombar.cpp 构造函数 |
| 29 | 光标状态标签 | DLabel | "CursorStatusLabel" | bottombar.cpp 构造函数 |
| 30 | 编码菜单 | DDropdownMenu | "EncodeMenu" | bottombar.cpp 构造函数 |
| 31 | 高亮菜单 | DDropdownMenu | "HighlightMenu" | bottombar.cpp 构造函数 |
| 32 | 缩放比例标签 | DLabel | "ScaleLabel" | bottombar.cpp 构造函数 |
| 33 | 格式菜单 | DDropdownMenu | "FormatMenu" | bottombar.cpp 构造函数 |
| 34 | 进度条 | DProgressBar | "ProgressBar" | bottombar.cpp 构造函数 |

### 8. DDropdownMenu (src/widgets/ddropdownmenu.h / ddropdownmenu.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 35 | 下拉按钮 | DToolButton | "DropdownMenuButton" | ddropdownmenu.cpp 构造函数 |

### 9. TextEdit (src/editor/dtextedit.h / dtextedit.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 36 | 文本编辑区 | DPlainTextEdit | "TextEditor" | dtextedit.cpp 构造函数 |

### 10. EditWrapper (src/editor/editwrapper.h / editwrapper.cpp)
| # | 控件 | 类型 | 建议 accessibleName | 位置 |
|---|------|------|---------------------|------|
| 37 | 编辑包装器 | QWidget | "EditWrapper" | editwrapper.cpp 构造函数 |

## UI 层级结构

```
DMainWindow "EditorWindow"
├── DTitlebar
│   ├── Tabbar (DTabBar) "EditorTabbar"
│   │   └── DIconButton "AddTabButton"
│   ├── DMenu (主菜单)
│   └── 窗口按钮 (选项/最小化/最大化/关闭)
├── QStackedWidget (m_editorWidget)
│   └── EditWrapper "EditWrapper"
│       ├── LeftAreaTextEdit (行号/书签/折叠)
│       ├── TextEdit (DPlainTextEdit) "TextEditor"
│       └── WarningNotices
├── FindBar (DFloatingWidget) "FindBar"
│   ├── QLabel "FindLabel"
│   ├── LineBar "FindInput"
│   ├── QPushButton "FindPrevButton"
│   ├── QPushButton "FindNextButton"
│   ├── QPushButton "SwitchToReplaceButton"
│   └── DIconButton "FindBarCloseButton"
├── ReplaceBar (DFloatingWidget) "ReplaceBar"
│   ├── QLabel "ReplaceLabel"
│   ├── LineBar "ReplaceFindInput"
│   ├── QLabel "WithLabel"
│   ├── LineBar "ReplaceWithInput"
│   ├── QPushButton "ReplaceButton"
│   ├── QPushButton "ReplaceAllButton"
│   ├── QPushButton "ReplaceRestButton"
│   ├── QPushButton "SkipButton"
│   └── DIconButton "ReplaceBarCloseButton"
├── JumpLineBar (DFloatingWidget) "JumpLineBar"
│   ├── QLabel "JumpLineLabel"
│   ├── DSpinBox "JumpLineInput"
│   └── DIconButton "JumpLineCloseButton"
├── ThemePanel (QWidget) "ThemePanel"
│   └── ThemeListView
└── BottomBar (QWidget) "BottomBar"
    ├── DLabel "PositionLabel"
    ├── DLabel "CharCountLabel"
    ├── DLabel "CursorStatusLabel"
    ├── DDropdownMenu "EncodeMenu"
    │   └── DToolButton "DropdownMenuButton"
    ├── DDropdownMenu "HighlightMenu"
    │   └── DToolButton "DropdownMenuButton"
    ├── DLabel "ScaleLabel"
    ├── DDropdownMenu "FormatMenu"
    │   └── DToolButton "DropdownMenuButton"
    ├── DLabel "ProgressLabel"
    └── DProgressBar "ProgressBar"
```

## 补全策略

为每个控件在构造完成后立即添加 `setAccessibleName()` 或 `setObjectName()` 调用，确保 AT-SPI 接口能够正确识别各 UI 元素。