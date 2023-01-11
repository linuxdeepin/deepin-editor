// SPDX-FileCopyrightText: 2017-2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>

class LeftAreaTextEdit;
class TextEdit;

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(LeftAreaTextEdit *leftAreaWidget);
    ~LineNumberArea() override;

    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override;

private:
    LeftAreaTextEdit *m_leftAreaWidget;
};

#endif
