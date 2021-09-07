/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ut_utils.h"

#include "../../src/common/settings.h"
#include "../../src/controls/tabbar.h"
#include "../../src/editor/editwrapper.h"
#include "../../src/widgets/window.h"
#include "../../src/startmanager.h"
#include "../../src/editor/dtextedit.h"
#include "../../src/common/utils.h"
#include "stub.h"
#include <QTextCodec>
#include <QByteArray>
#include <QMimeDatabase>

test_utils::test_utils()
{
}

void test_utils::SetUp()
{
    utils = new Utils;
    EXPECT_NE(utils,nullptr);
}

void test_utils::TearDown()
{
    delete utils;
    utils=nullptr;
}

//static QString getQrcPath(const QString &imageName);
TEST_F(test_utils, getQrcPath)
{
    EXPECT_NE(Utils::getQrcPath("aa").contains("aa"),false);

}

//static QString getQssPath(const QString &qssName);
TEST_F(test_utils, getQssPath)
{
    EXPECT_NE(Utils::getQssPath("aa").contains("aa"),false);

}

//static QSize getRenderSize(int fontSize, const QString &string);
TEST_F(test_utils, getRenderSize)
{
    EXPECT_NE(Utils::getRenderSize(1, "aa").width(),0);

}

//static void setFontSize(QPainter &painter, int textSize);
TEST_F(test_utils, setFontSize)
{
    QImage image(10, 10, QImage::Format_RGB888);
    QPainter painter(&image);
    Utils::setFontSize(painter, 1);
    EXPECT_NE(painter.font().pixelSize(),0);

}

//static void applyQss(QWidget *widget, const QString &qssName);
TEST_F(test_utils, applyQss)
{
    QWidget *widget = new QWidget;
    Utils::applyQss(widget, "1");

    EXPECT_NE(widget,nullptr);
    widget->deleteLater();
}

//static QString getKeyshortcut(QKeyEvent *keyEvent);
TEST_F(test_utils, getKeyshortcut)
{
    //QKeyEvent::Type
    QKeyEvent *keyEvent = new QKeyEvent(QEvent::KeyPress, 1, Qt::NoModifier);
    EXPECT_EQ(Utils::getKeyshortcut(keyEvent).contains("+"),false);

}

//static QString getKeyshortcutFromKeymap(Settings* settings, const QString &keyCategory, const QString &keyName);
TEST_F(test_utils, getKeyshortcutFromKeymap)
{
    EXPECT_NE(Utils::getKeyshortcutFromKeymap(Settings::instance(), "editor", "selectall")," ");

}

//static bool fileExists(const QString &path);
TEST_F(test_utils, fileExists)
{
    EXPECT_NE(Utils::fileExists("aa"),true);
}

//static bool fileIsWritable(const QString &path);
TEST_F(test_utils, fileIsWritable)
{
    EXPECT_NE(Utils::fileIsWritable("aa"),true);
}

//static bool fileIsHome(const QString &path);
TEST_F(test_utils, fileIsHome)
{
    EXPECT_NE(Utils::fileIsHome("aa"),true);

}

//static void passInputEvent(int wid);

//static QPixmap dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset);
//static QImage dropShadow(const QPixmap &px, qreal radius, const QColor &color);
TEST_F(test_utils, dropShadow)
{
    QImage image(10, 10, QImage::Format_RGB888);
    EXPECT_NE(Utils::dropShadow(QPixmap::fromImage(image), 1.5, QColor("#000000"), QPoint(1, 1)).isNull(),true);
    Utils::dropShadow(QPixmap::fromImage(image), 1.5, QColor("#000000"));

}

//static QByteArray detectEncode(const QByteArray &data, const QString &fileName = QString());
TEST_F(test_utils, detectEncode)
{
    EXPECT_NE(Utils::detectEncode("aa").size(),0);
}

TEST_F(test_utils, detectEncode2)
{
    QByteArray array;
    array.clear();
    EXPECT_NE(utils->detectEncode(array).size(),0);
}

QTextCodec *stub_codecForUtfText(const QByteArray &ba, QTextCodec *defaultCodec)
{
    return nullptr;
}

//Stub:ACCESS_PRIVATE_STATIC_FUN(QTextCodec, QTextCodec(const QByteArray &, QTextCodec *), codecForUtfText);

TEST_F(test_utils, detectEncode3)
{
    typedef QTextCodec *(*fptr)(const QByteArray &, QTextCodec *);
    fptr A_foo = (fptr)(&QTextCodec::codecForUtfText);

    Stub st;
    st.set(A_foo, stub_codecForUtfText);
    QByteArray array("-");
    EXPECT_NE(utils->detectEncode(array).size(),0);
}

QMimeType stub_mimeTypeForData(const QByteArray &data)
{
    QMimeType type;
    QMimeDatabase base;
    type = base.mimeTypeForName("application/xml");
    return type;
}

TEST_F(test_utils, detectEncode4)
{
    Stub st;
    st.set((QMimeType(QMimeDatabase::*)(const QByteArray &) const)ADDR(QMimeDatabase, mimeTypeForData), stub_mimeTypeForData);
    EXPECT_NE(utils->detectEncode("aa", nullptr).size(),0);
}

//static QByteArray getEncode(const QByteArray &data);
TEST_F(test_utils, getEncode)
{
    EXPECT_NE(Utils::getEncode("aa").isEmpty(),true);

}

//static qreal easeInOut(qreal x);
TEST_F(test_utils, easeInOut)
{
    EXPECT_NE(Utils::easeInOut(0.1),0);
}

//static qreal easeInQuad(qreal x);
TEST_F(test_utils, easeInQuad)
{
    EXPECT_NE(Utils::easeInQuad(0.1),0);
}

//static qreal easeInQuint(qreal x);
TEST_F(test_utils, easeInQuint)
{
    EXPECT_NE(Utils::easeInQuint(0.1),0);
}

//static qreal easeOutQuad(qreal x);
TEST_F(test_utils, easeOutQuad)
{
    EXPECT_NE(Utils::easeOutQuad(0.1),0);
}

//static qreal easeOutQuint(qreal x);
TEST_F(test_utils, easeOutQuint)
{
    EXPECT_NE(Utils::easeOutQuint(0.1),0);
}

//static QVariantMap getThemeMapFromPath(const QString &filepath);
TEST_F(test_utils, getThemeMapFromPath)
{
   EXPECT_NE(Utils::getThemeMapFromPath("aa").isEmpty(),false);
}

//static bool isMimeTypeSupport(const QString &filepath);
TEST_F(test_utils, isMimeTypeSupport)
{
   EXPECT_NE(Utils::isMimeTypeSupport("aa"),false);
}

//static bool isDraftFile(const QString &filepath);
TEST_F(test_utils, isDraftFile)
{
    EXPECT_NE(Utils::isDraftFile("aa"),true);
}

//static const QStringList getEncodeList();
TEST_F(test_utils, getEncodeList)
{
    Utils::getEncodeList();
    EXPECT_NE(Utils::renderSVG("", QSize(40, 40), false).isNull(),false);

}

extern float codecConfidenceForData(const QTextCodec *codec, const QByteArray &data, const QLocale::Country &country);
TEST_F(test_utils, codecConfidenceForData)
{
}
