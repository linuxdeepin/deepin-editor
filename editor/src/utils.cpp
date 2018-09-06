/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include "utils.h"

#include <DSettings>
#include <DSettingsOption>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QJsonObject>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFontMetrics>
#include <QPainter>
#include <QString>
#include <QtMath>
#include <QWidget>
#include <KEncodingProber>
#include <QTextCodec>
#include <DToast>

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

QString Utils::getQrcPath(const QString &imageName)
{
    return QString(":/image/%1").arg(imageName);
}

QString Utils::getQssPath(const QString &qssName)
{
    return QString(":/qss/%1").arg(qssName);
}

QSize Utils::getRenderSize(int fontSize, const QString &string)
{
    QFont font;
    font.setPointSize(fontSize);
    QFontMetrics fm(font);

    int width = 0;
    int height = 0;
    foreach (auto line, string.split("\n")) {
        int lineWidth = fm.width(line);
        int lineHeight = fm.height();

        if (lineWidth > width) {
            width = lineWidth;
        }
        height += lineHeight;
    }

    return QSize(width, height);
}

void Utils::setFontSize(QPainter &painter, int textSize)
{
    QFont font = painter.font() ;
    font.setPointSize(textSize);
    painter.setFont(font);
}

void Utils::applyQss(QWidget *widget, const QString &qssName)
{
    QFile file(Utils::getQssPath(qssName));
    file.open(QFile::ReadOnly);
    QTextStream filetext(&file);
    QString stylesheet = filetext.readAll();
    widget->setStyleSheet(stylesheet);
    file.close();
}

QString Utils::getFileContent(const QString &filepath)
{
    QString content;

    QFile file(filepath);
    if (file.open(QFile::ReadOnly)) {
        auto fileContent = file.readAll();
        auto fileEncode = detectEncode(fileContent, filepath);

        QTextStream stream(&fileContent);
        stream.setCodec(fileEncode);
        content = stream.readAll();
    }

    file.close();

    return content;
}

float codecConfidenceForData(const QTextCodec *codec, const QByteArray &data, const QLocale::Country &country)
{
    qreal hep_count = 0;
    int non_base_latin_count = 0;
    qreal unidentification_count = 0;
    int replacement_count = 0;

    QTextDecoder decoder(codec);
    const QString &unicode_data = decoder.toUnicode(data);

    for (int i = 0; i < unicode_data.size(); ++i) {
        const QChar &ch = unicode_data.at(i);

        if (ch.unicode() > 0x7f)
            ++non_base_latin_count;

        switch (ch.script()) {
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
            hep_count += country == QLocale::Japan ? 1.2 : 0.5;
            unidentification_count += country == QLocale::Japan ? 0 : 0.3;
            break;
        case QChar::Script_Han:
            hep_count += country == QLocale::China ? 1.2 : 0.5;
            unidentification_count += country == QLocale::China ? 0 : 0.3;
            break;
        case QChar::Script_Hangul:
            hep_count += (country == QLocale::NorthKorea) || (country == QLocale::SouthKorea) ? 1.2 : 0.5;
            unidentification_count += (country == QLocale::NorthKorea) || (country == QLocale::SouthKorea) ? 0 : 0.3;
            break;
        default:
            // full-width character, emoji, 常用标点, 拉丁文补充1
            if ((ch.unicode() >= 0xff00 && ch <= 0xffef)
                    || (ch.unicode() >= 0x2600 && ch.unicode() <= 0x27ff)
                    || (ch.unicode() >= 0x2000 && ch.unicode() <= 0x206f)
                    || (ch.unicode() >= 0x80 && ch.unicode() <= 0xff)) {
                ++hep_count;
            } else if (ch.isSurrogate() && ch.isHighSurrogate()) {
                ++i;

                if (i < unicode_data.size()) {
                    const QChar &next_ch = unicode_data.at(i);

                    if (!next_ch.isLowSurrogate()) {
                        --i;
                        break;
                    }

                    uint unicode = QChar::surrogateToUcs4(ch, next_ch);

                    // emoji
                    if (unicode >= 0x1f000 && unicode <= 0x1f6ff) {
                        hep_count += 2;
                    }
                }
            } else if (ch.unicode() == QChar::ReplacementCharacter) {
                ++replacement_count;
            } else if (ch.unicode() > 0x7f) {
                // 因为UTF-8编码的容错性很低，所以未识别的编码只需要判断是否为 QChar::ReplacementCharacter 就能排除
                if (codec->name() != "UTF-8")
                    ++unidentification_count;
            }
            break;
        }
    }

    float c = qreal(hep_count) / non_base_latin_count / 1.2;

    c -= qreal(replacement_count) / non_base_latin_count;
    c -= qreal(unidentification_count) / non_base_latin_count;

    return qMax(0.0f, c);
}

QByteArray Utils::detectEncode(const QByteArray &data, const QString &fileName)
{
    // Return local encoding if nothing in file.
    if (data.isEmpty()) {
        return QTextCodec::codecForLocale()->name();
    }

    if (QTextCodec *c = QTextCodec::codecForUtfText(data, nullptr)) {
        return c->name();
    }

    KEncodingProber prober(KEncodingProber::Universal);
    KEncodingProber::ProberState state = prober.feed(data);

    return prober.encoding();
}

bool Utils::fileExists(const QString &path)
{
    QFileInfo check_file(path);

    return check_file.exists() && check_file.isFile();
}

bool Utils::fileIsWritable(const QString &path)
{
    QFileDevice::Permissions permissions = QFile(path).permissions();

    return permissions & QFileDevice::WriteUser;
}

bool Utils::fileIsHome(const QString &path)
{
    return path.startsWith(QDir::homePath());
}

QString Utils::getKeyshortcut(QKeyEvent *keyEvent)
{
    QStringList keys;
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    if (modifiers != Qt::NoModifier){
        if (modifiers.testFlag(Qt::ControlModifier)) {
            keys.append("Ctrl");
        }

        if (modifiers.testFlag(Qt::AltModifier)) {
            keys.append("Alt");
        }

        if (modifiers.testFlag(Qt::MetaModifier)) {
            keys.append("Meta");
        }

        if (modifiers.testFlag(Qt::ShiftModifier)) {
            keys.append("Shift");
        }
    }

    if(keyEvent->key() !=0 && keyEvent->key() != Qt::Key_unknown){
        keys.append(QKeySequence(keyEvent->key()).toString());
    }

    return keys.join("+");
}

QString Utils::getKeyshortcutFromKeymap(Settings* settings, const QString &keyCategory, const QString &keyName)
{
    return settings->settings->option(QString("shortcuts.%1.%2").arg(keyCategory).arg(keyName))->value().toString();
}

QPixmap Utils::dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset)
{
    QImage shadow = dropShadow(source, radius, color);
    shadow.setDevicePixelRatio(source.devicePixelRatio());


    QPainter pa(&shadow);
    pa.setCompositionMode(QPainter::CompositionMode_SourceOver);
    pa.drawPixmap(radius - offset.x(), radius - offset.y(), source);
    pa.end();

    return QPixmap::fromImage(shadow);
}

QImage Utils::dropShadow(const QPixmap &px, qreal radius, const QColor &color)
{
    if (px.isNull()) {
        return QImage();
    }

    QImage tmp(px.size() * px.devicePixelRatio() + QSize(radius * 2, radius * 2), QImage::Format_ARGB32_Premultiplied);
    tmp.setDevicePixelRatio(px.devicePixelRatio());
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tmpPainter.drawPixmap(QPoint(radius, radius), px);
    tmpPainter.end();

    // Blur the alpha channel.
    QImage blurred(tmp.size() * px.devicePixelRatio(), QImage::Format_ARGB32_Premultiplied);
    blurred.setDevicePixelRatio(px.devicePixelRatio());
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, radius, false, true);
    blurPainter.end();
    if (color == QColor(Qt::black)) {
        return blurred;
    }
    tmp = blurred;

    // Blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    return tmp;
}

qreal Utils::easeInOut(qreal x)
{
    return (1 - qCos(M_PI * x)) / 2;
}

qreal Utils::easeInQuad(qreal x)
{
    return qPow(x, 2);
}

qreal Utils::easeOutQuad(qreal x)
{
    return -1 * qPow(x - 1, 2) + 1;
}

qreal Utils::easeInQuint(qreal x)
{
    return qPow(x, 5);
}

qreal Utils::easeOutQuint(qreal x)
{
    return qPow(x - 1, 5) + 1;
}

QVariantMap Utils::getThemeMapFromPath(const QString &filepath)
{
    QFile file(filepath);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "Failed to open " << filepath;
    }

    QTextStream in(&file);
    const QString jsonStr = in.readAll();
    file.close();

    QByteArray jsonBytes = jsonStr.toLocal8Bit();
    QJsonDocument document = QJsonDocument::fromJson(jsonBytes);
    QJsonObject object = document.object();

    return object.toVariantMap();
}

bool Utils::isEditableFile(const QString &filepath)
{
    auto mimeType = QMimeDatabase().mimeTypeForFile(filepath).name();
    qDebug() << "*** " << filepath << mimeType;

    if (mimeType.startsWith("text/")) {
        return true;
    }

    QList<QString> mimeTypeWhiteList;
    // Please check full mime type list from: https://www.freeformatter.com/mime-types-list.html
    mimeTypeWhiteList
        << "application/cmd"
        << "application/javascript"
        << "application/json"
        << "application/pkix-cert"
        << "application/sql"
        << "application/vnd.nokia.qt.qmakeprofile"
        << "application/vnd.nokia.xml.qt.resource"
        << "application/x-asp"
        << "application/x-desktop"
        << "application/x-designer"
        << "application/x-empty"
        << "application/x-msdos-program"
        << "application/x-pearl"
        << "application/x-php"
        << "application/x-shellscript"
        << "application/x-sh"
        << "application/x-theme"
        << "application/x-csh"
        << "application/x-text"
        << "application/x-yaml"
        << "application/xml"
        << "application/yaml"
        << "application/x-zerosize"
        << "image/svg+xml";

    if (mimeTypeWhiteList.contains(mimeType)) {
        return true;
    }

    return false;
}

void Utils::toast(const QString &message, QWidget *parent)
{
    DToast *toast = new DToast(parent);
    int avaliableHeight = parent->height() - toast->height();
    int toastPaddingBottom = qMin(avaliableHeight / 2, 100);

    QObject::connect(toast, &DToast::visibleChanged, parent, [toast](bool visible) {
        if (visible == false) {
            toast->deleteLater();
        }
    });

    toast->setText(message);
    toast->setIcon(QIcon(Utils::getQrcPath("logo_24.svg")));
    toast->pop();

    toast->move((parent->width() - toast->width()) / 2,
                avaliableHeight - toastPaddingBottom);
}
