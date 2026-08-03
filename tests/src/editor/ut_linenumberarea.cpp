// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtest/gtest.h"
#include "../../src/editor/linenumberarea.h"
#include "../../src/editor/leftareaoftextedit.h"
#include "../../src/editor/dtextedit.h"
#include "../stub.h"
#include <QMouseEvent>

// LineNumberArea::getPressPoint()
TEST(UT_LineNumberArea, getPressPoint)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("aaa\nbbb\nccc");
    LeftAreaTextEdit *leftArea = new LeftAreaTextEdit(edit);
    LineNumberArea *area = new LineNumberArea(leftArea);

    // default press point
    QPoint pt = area->getPressPoint();
    EXPECT_TRUE(pt.isNull());

    delete area;
    delete leftArea;
    delete edit;
}

// LineNumberArea::mousePressEvent(QMouseEvent*)
TEST(UT_LineNumberArea, mousePressEvent)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("aaa\nbbb\nccc");
    LeftAreaTextEdit *leftArea = new LeftAreaTextEdit(edit);
    LineNumberArea *area = new LineNumberArea(leftArea);

    // use an out-of-range x so onPressedLineNumber returns early (safe path)
    QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress,
                                         QPoint(-1, 0),
                                         Qt::LeftButton,
                                         Qt::LeftButton,
                                         Qt::NoModifier);
    area->mousePressEvent(event);

    delete event;
    delete area;
    delete leftArea;
    delete edit;
}
