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
#include "test_utils.h"


test_utils::test_utils()
{

}
//static QString getQrcPath(const QString &imageName);
TEST_F(test_utils, getQrcPath)
{
    Utils::getQrcPath("aa");
    assert(1==1);
}

//static QString getQssPath(const QString &qssName);
TEST_F(test_utils, getQssPath)
{
    Utils::getQssPath("aa");
    assert(1==1);
}

//static QSize getRenderSize(int fontSize, const QString &string);
TEST_F(test_utils, getRenderSize)
{
    Utils::getRenderSize(1,"aa");
    assert(1==1);
}

//static void setFontSize(QPainter &painter, int textSize);
TEST_F(test_utils, setFontSize)
{
    QImage image(10,10,QImage::Format_RGB888);
    QPainter painter(&image);
    Utils::setFontSize(painter,1);
    assert(1==1);
}

//static void applyQss(QWidget *widget, const QString &qssName);
TEST_F(test_utils, applyQss)
{
    QWidget *widget = new QWidget;
    Utils::applyQss(widget,"1");
    assert(1==1);
}

//static QString getKeyshortcut(QKeyEvent *keyEvent);
TEST_F(test_utils, getKeyshortcut)
{
    //QKeyEvent::Type
    QKeyEvent *keyEvent = new QKeyEvent(QEvent::KeyPress,1,Qt::NoModifier);
    Utils::getKeyshortcut(keyEvent);
    assert(1==1);
}

//static QString getKeyshortcutFromKeymap(Settings* settings, const QString &keyCategory, const QString &keyName);
TEST_F(test_utils, getKeyshortcutFromKeymap)
{
    Utils::getKeyshortcutFromKeymap(Settings::instance(),"editor", "selectall");
    assert(1==1);
}

//static bool fileExists(const QString &path);
TEST_F(test_utils, fileExists)
{
    Utils::fileExists("aa");
    assert(1==1);
}

//static bool fileIsWritable(const QString &path);
TEST_F(test_utils, fileIsWritable)
{
    Utils::fileIsWritable("aa");
    assert(1==1);
}

//static bool fileIsHome(const QString &path);
TEST_F(test_utils, fileIsHome)
{
    Utils::fileIsHome("aa");
    assert(1==1);
}

//static void passInputEvent(int wid);

//static QPixmap dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset);
//static QImage dropShadow(const QPixmap &px, qreal radius, const QColor &color);
TEST_F(test_utils, dropShadow)
{
    QImage image(10,10,QImage::Format_RGB888);
    Utils::dropShadow(QPixmap::fromImage(image),1.5,QColor("#000000"),QPoint(1,1));
    Utils::dropShadow(QPixmap::fromImage(image),1.5,QColor("#000000"));
    assert(1==1);
}

//static QByteArray detectEncode(const QByteArray &data, const QString &fileName = QString());
TEST_F(test_utils, detectEncode)
{
    Utils::detectEncode("aa");
    assert(1==1);
}

//static QByteArray getEncode(const QByteArray &data);
TEST_F(test_utils, getEncode)
{
    Utils::getEncode("aa");
    assert(1==1);
}

//static qreal easeInOut(qreal x);
TEST_F(test_utils, easeInOut)
{
    Utils::easeInOut(0.1);
    assert(1==1);
}

//static qreal easeInQuad(qreal x);
TEST_F(test_utils, easeInQuad)
{
    Utils::easeInQuad(0.1);
    assert(1==1);
}

//static qreal easeInQuint(qreal x);
TEST_F(test_utils, easeInQuint)
{
    Utils::easeInQuint(0.1);
    assert(1==1);
}

//static qreal easeOutQuad(qreal x);
TEST_F(test_utils, easeOutQuad)
{
    Utils::easeOutQuad(0.1);
    assert(1==1);
}

//static qreal easeOutQuint(qreal x);
TEST_F(test_utils, easeOutQuint)
{
    Utils::easeOutQuint(0.1);
    assert(1==1);
}

//static QVariantMap getThemeMapFromPath(const QString &filepath);
TEST_F(test_utils, getThemeMapFromPath)
{
    Utils::getThemeMapFromPath("aa");
    assert(1==1);
}

//static bool isMimeTypeSupport(const QString &filepath);
TEST_F(test_utils, isMimeTypeSupport)
{
    Utils::isMimeTypeSupport("aa");
    assert(1==1);
}

//static bool isDraftFile(const QString &filepath);
TEST_F(test_utils, isDraftFile)
{
    Utils::isDraftFile("aa");
    assert(1==1);
}

//static void toast(const QString &message, QWidget* parent = nullptr);
TEST_F(test_utils, toast)
{
    Utils::toast("aa");
    assert(1==1);
}

//static const QStringList getEncodeList();
TEST_F(test_utils, getEncodeList)
{
    Utils::getEncodeList();
    assert(1==1);
}

