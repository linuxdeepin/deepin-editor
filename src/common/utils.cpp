// SPDX-FileCopyrightText: 2011-2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"

#include <DSettings>
#include <DSettingsOption>
#include <DMessageManager>
#include <DSysInfo>
#include <DGuiApplicationHelper>

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
#include <QStandardPaths>
#include <KEncodingProber>
#include <QTextCodec>
#include <QImageReader>
#include <QCryptographicHash>
#include "qprocess.h"

#include <QLibraryInfo>

extern "C" {
#include "../basepub/load_libs.h"
}

DCORE_USE_NAMESPACE

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void
qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

QString Utils::m_systemLanguage;

QString Utils::getQrcPath(const QString &imageName)
{
    qDebug() << "Enter getQrcPath, imageName:" << imageName;
    QString path = QString(":/images/%1").arg(imageName);
    qDebug() << "Exit getQrcPath, return path:" << path;
    return path;
}

QString Utils::getQssPath(const QString &qssName)
{
    qDebug() << "Enter getQssPath, qssName:" << qssName;
    QString path = QString(":/qss/%1").arg(qssName);
    qDebug() << "Exit getQssPath, return path:" << path;
    return path;
}

QSize Utils::getRenderSize(int fontSize, const QString &string)
{
    qDebug() << "Entering getRenderSize with fontSize:" << fontSize << "and string:" << string.left(50);
    QFont font;
    font.setPointSize(fontSize);
    QFontMetrics fm(font);

    int width = 0;
    int height = 0;

    for (const QString &line : string.split("\n")) {
        int lineWidth = fm.horizontalAdvance(line);
        int lineHeight = fm.height();

        if (lineWidth > width) {
            qDebug() << "New max width found:" << lineWidth;
            width = lineWidth;
        }

        height += lineHeight;
    }

    qDebug() << "Leaving getRenderSize, returning QSize(" << width << "," << height << ")";
    return QSize(width, height);
}

void Utils::setFontSize(QPainter &painter, int textSize)
{
    qDebug() << "Entering setFontSize with textSize:" << textSize;
    QFont font = painter.font();
    font.setPointSize(textSize);
    painter.setFont(font);
    qDebug() << "Leaving setFontSize";
}

void Utils::applyQss(QWidget *widget, const QString &qssName)
{
    qDebug() << "Entering applyQss for widget" << widget->objectName() << "with qss" << qssName;
    QFile file(Utils::getQssPath(qssName));
    file.open(QFile::ReadOnly);
    QTextStream filetext(&file);
    QString stylesheet = filetext.readAll();
    widget->setStyleSheet(stylesheet);
    file.close();
    qDebug() << "Leaving applyQss";
}

float Utils::codecConfidenceForData(const QTextCodec *codec, const QByteArray &data, const QLocale::Country &country)
{
    qDebug() << "Entering codecConfidenceForData for codec:" << codec->name();
    qreal hep_count = 0;
    int non_base_latin_count = 0;
    qreal unidentification_count = 0;
    int replacement_count = 0;

    QTextDecoder decoder(codec);
    const QString &unicode_data = decoder.toUnicode(data);

    for (int i = 0; i < unicode_data.size(); ++i) {
        const QChar &ch = unicode_data.at(i);

        if (ch.unicode() > 0x7f) {
            qDebug() << "Non-base Latin character found:" << ch;
            ++non_base_latin_count;
        }

        switch (ch.script()) {
            case QChar::Script_Hiragana:
            case QChar::Script_Katakana:
                qDebug() << "Hiragana or Katakana character found:" << ch;
                hep_count += (country == QLocale::Japan) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::Japan) ? 0 : 0.3;
                break;
            case QChar::Script_Han:
                qDebug() << "Chinese character found:" << ch;
                hep_count += (country == QLocale::China) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::China) ? 0 : 0.3;
                break;
            case QChar::Script_Hangul:
                qDebug() << "Hangul character found:" << ch;
                hep_count += (country == QLocale::NorthKorea) || (country == QLocale::SouthKorea) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::NorthKorea) || (country == QLocale::SouthKorea) ? 0 : 0.3;
                break;
            case QChar::Script_Cyrillic:
                qDebug() << "Cyrillic character found:" << ch;
                hep_count += (country == QLocale::Russia) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::Russia) ? 0 : 0.3;
                break;
            case QChar::Script_Devanagari:
                qDebug() << "Devanagari character found:" << ch;
                hep_count += (country == QLocale::Nepal || country == QLocale::India) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::Nepal || country == QLocale::India) ? 0 : 0.3;
                break;
            default:
                // full-width character, emoji, 常用标点, 拉丁文补充1，天城文补充，CJK符号和标点符号（如：【】）
                if ((ch.unicode() >= 0xff00 && ch.unicode() <= 0xffef) || (ch.unicode() >= 0x2600 && ch.unicode() <= 0x27ff) ||
                    (ch.unicode() >= 0x2000 && ch.unicode() <= 0x206f) || (ch.unicode() >= 0x80 && ch.unicode() <= 0xff) ||
                    (ch.unicode() >= 0xa8e0 && ch.unicode() <= 0xa8ff) || (ch.unicode() >= 0x0900 && ch.unicode() <= 0x097f) ||
                    (ch.unicode() >= 0x3000 && ch.unicode() <= 0x303f)) {
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

    qDebug() << "Leaving codecConfidenceForData, confidence:" << c;
    return qMax(0.0f, c);
}

QByteArray Utils::detectEncode(const QByteArray &data, const QString &fileName)
{
    qDebug() << "Entering detectEncode for file:" << fileName;
    // Return local encoding if nothing in file.
    if (data.isEmpty()) {
        qDebug() << "Data is empty, returning codec for locale.";
        return QTextCodec::codecForLocale()->name();
    }

    if (QTextCodec *c = QTextCodec::codecForUtfText(data, nullptr)) {
        qDebug() << "Detected UTF text, codec:" << c->name();
        return c->name();
    }

    QMimeDatabase mime_database;
    const QMimeType &mime_type =
        fileName.isEmpty() ? mime_database.mimeTypeForData(data) : mime_database.mimeTypeForFileNameAndData(fileName, data);
    const QString &mimetype_name = mime_type.name();
    qDebug() << "MIME type:" << mimetype_name;
    KEncodingProber::ProberType proberType = KEncodingProber::Universal;

    if (mimetype_name == QStringLiteral("application/xml") || mimetype_name == QStringLiteral("text/html") ||
        mimetype_name == QStringLiteral("application/xhtml+xml")) {
        qDebug() << "XML/HTML mimetype found, checking for charset meta tag.";
        const QString &_data = QString::fromLatin1(data);
        QRegularExpression pattern("<\\bmeta.+\\bcharset=(?'charset'\\S+?)\\s*['\"/>]");

        pattern.setPatternOptions(QRegularExpression::DontCaptureOption | QRegularExpression::CaseInsensitiveOption);
        const QString &charset =
            pattern
                .match(
                    _data, 0, QRegularExpression::PartialPreferFirstMatch, QRegularExpression::DontCheckSubjectStringMatchOption)
                .captured("charset");

        if (!charset.isEmpty()) {
            qDebug() << "Found charset in meta tag:" << charset;
            return charset.toLatin1();
        }

        pattern.setPattern("<\\bmeta\\s+http-equiv=\"Content-Language\"\\s+content=\"(?'language'[a-zA-Z-]+)\"");

        const QString &language =
            pattern
                .match(
                    _data, 0, QRegularExpression::PartialPreferFirstMatch, QRegularExpression::DontCheckSubjectStringMatchOption)
                .captured("language");

        if (0 != language.size()) {
            qDebug() << "Found language in meta tag:" << language;
            QLocale l(language);

            switch (l.script()) {
                case QLocale::ArabicScript:
                    proberType = KEncodingProber::Arabic;
                    break;
                case QLocale::SimplifiedChineseScript:
                    proberType = KEncodingProber::ChineseSimplified;
                    break;
                case QLocale::TraditionalChineseScript:
                    proberType = KEncodingProber::ChineseTraditional;
                    break;
                case QLocale::CyrillicScript:
                    proberType = KEncodingProber::Cyrillic;
                    break;
                case QLocale::GreekScript:
                    proberType = KEncodingProber::Greek;
                    break;
                case QLocale::HebrewScript:
                    proberType = KEncodingProber::Hebrew;
                    break;
                case QLocale::JapaneseScript:
                    proberType = KEncodingProber::Japanese;
                    break;
                case QLocale::KoreanScript:
                    proberType = KEncodingProber::Korean;
                    break;
                case QLocale::ThaiScript:
                    proberType = KEncodingProber::Thai;
                    break;
                default:
                    break;
            }
            qDebug() << "Set prober type based on language:" << proberType;
        }
    } else if (mimetype_name == "text/x-python") {
        qDebug() << "Python mimetype found, checking for coding meta tag.";
        QRegularExpression pattern("^#coding\\s*:\\s*(?'coding'\\S+)$");
        QTextStream stream(data);

        pattern.setPatternOptions(QRegularExpression::DontCaptureOption | QRegularExpression::CaseInsensitiveOption);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        stream.setCodec("latin1");
#else
        stream.setEncoding(QStringConverter::Latin1);
#endif

        while (!stream.atEnd()) {
            const QString &_data = stream.readLine();
            const QString &coding = pattern.match(_data, 0).captured("coding");

            if (!coding.isEmpty()) {
                return coding.toLatin1();
            }
        }
    }

    // for CJK
    const QList<QPair<KEncodingProber::ProberType, QLocale::Country>> fallback_list{
        {KEncodingProber::ChineseSimplified, QLocale::China},
        {KEncodingProber::ChineseTraditional, QLocale::China},
        {KEncodingProber::Japanese, QLocale::Japan},
        {KEncodingProber::Korean, QLocale::NorthKorea},
        //{KEncodingProber::Cyrillic, QLocale::Russia},
        //{KEncodingProber::Greek, QLocale::Greece},
        //{proberType, QLocale::system().country()}
    };

    KEncodingProber prober(proberType);
    prober.feed(data);
    float pre_confidence = prober.confidence();
    QByteArray pre_encoding = prober.encoding();

    QTextCodec *def_codec = QTextCodec::codecForLocale();
    QByteArray encoding;
    float confidence = 0;

    for (auto i : fallback_list) {
        prober.setProberType(i.first);
        prober.feed(data);

        float prober_confidence = prober.confidence();
        QByteArray prober_encoding = prober.encoding();

        if (i.first != proberType && qFuzzyIsNull(prober_confidence)) {
            qDebug() << "i.first != proberType && qFuzzyIsNull(prober_confidence)";
            prober_confidence = pre_confidence;
            prober_encoding = pre_encoding;
        }

    confidence:
        qDebug() << "go to here: confidence!";
        if (QTextCodec *codec = QTextCodec::codecForName(prober_encoding)) {
            if (def_codec == codec) {
                qDebug() << "def_codec == codec";
                def_codec = nullptr;
            }

            float c = Utils::codecConfidenceForData(codec, data, i.second);

            if (prober_confidence > 0.5) {
                qDebug() << "prober_confidence > 0.5";
                c = c / 2 + prober_confidence / 2;
            } else {
                qDebug() << "prober_confidence <= 0.5";
                c = c / 3 * 2 + prober_confidence / 3;
            }

            if (c > confidence) {
                qDebug() << "c > confidence!";
                confidence = c;
                encoding = prober_encoding;
            }

            if (i.first == KEncodingProber::ChineseTraditional && c < 0.5) {
                qDebug() << "i.first == KEncodingProber::ChineseTraditional && c < 0.5";
                // test Big5
                c = Utils::codecConfidenceForData(QTextCodec::codecForName("Big5"), data, i.second);

                if (c > 0.5 && c > confidence) {
                    qDebug() << " 0.5 set Big5";
                    confidence = c;
                    encoding = "Big5";
                }
            }
        }

        if (i.first != proberType) {
            qDebug() << "i.first != proberType";
            // 使用 proberType 类型探测出的结果结合此国家再次做编码检查
            i.first = proberType;
            prober_confidence = pre_confidence;
            prober_encoding = pre_encoding;
            goto confidence;
        }
    }

    if (def_codec && Utils::codecConfidenceForData(def_codec, data, QLocale::system().country()) > confidence) {
        qDebug() << "return codec's name";
        return def_codec->name();
    }

    qDebug() << "final return!";
    return encoding;
}

QByteArray Utils::getEncode(const QByteArray &data)
{
    qDebug() << "Enter getEncode";
    // try to get HTML header encoding.
    if (QTextCodec *codecForHtml = QTextCodec::codecForHtml(data, nullptr)) {
        qDebug() << "codecForHtml" << codecForHtml->name();
        return codecForHtml->name();
    }

    QTextCodec *codec = nullptr;
    KEncodingProber prober(KEncodingProber::Universal);
    prober.feed(data.constData(), data.size());

    // we found codec with some confidence ?
    if (prober.confidence() > 0.5) {
        codec = QTextCodec::codecForName(prober.encoding());
    }

    if (!codec) {
        qDebug() << "codec is null";
        return QByteArray();
    }

    qDebug() << "codec" << codec->name();
    return codec->name();
}

bool Utils::fileExists(const QString &path)
{
    qDebug() << "Enter fileExists, path:" << path;
    QFileInfo check_file(path);
    qDebug() << "check_file" << check_file.exists() << check_file.isFile();
    bool exists = check_file.exists() && check_file.isFile();
    qDebug() << "Exit fileExists, file exists:" << exists;
    return exists;
}

bool Utils::fileIsWritable(const QString &path)
{
    qDebug() << "Enter fileIsWritable, path:" << path;
    QFileDevice::Permissions permissions = QFile(path).permissions();
    qDebug() << "permissions" << permissions;
    bool writable = permissions & QFileDevice::WriteUser;
    qDebug() << "Exit fileIsWritable, is writable:" << writable;
    return writable;
}

bool Utils::fileIsHome(const QString &path)
{
    qDebug() << "Enter fileIsHome, path:" << path;
    bool isHome = path.startsWith(QDir::homePath());
    qDebug() << "Exit fileIsHome, isHome:" << isHome;
    return isHome;
}

QString Utils::getKeyshortcut(QKeyEvent *keyEvent)
{
    qDebug() << "Enter getKeyshortcut";
    QStringList keys;
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    if (modifiers != Qt::NoModifier) {
        qDebug() << "modifiers" << modifiers;
        if (modifiers.testFlag(Qt::MetaModifier)) {
            qDebug() << "modifiers.testFlag(Qt::MetaModifier)";
            keys.append("Meta");
        }

        if (modifiers.testFlag(Qt::ControlModifier)) {
            qDebug() << "modifiers.testFlag(Qt::ControlModifier)";
            keys.append("Ctrl");
        }

        if (modifiers.testFlag(Qt::AltModifier)) {
            qDebug() << "modifiers.testFlag(Qt::AltModifier)";
            keys.append("Alt");
        }

        if (modifiers.testFlag(Qt::ShiftModifier)) {
            qDebug() << "modifiers.testFlag(Qt::ShiftModifier)";
            keys.append("Shift");
        }

        // 添加小键盘处理，若为小键盘按键按下，组合键需添加 Num ，例如 Ctrl+Num+6 / Ctrl+Num+Up
        if (modifiers.testFlag(Qt::KeypadModifier)) {
            qDebug() << "modifiers.testFlag(Qt::KeypadModifier)";
            keys.append("Num");
        }
    }

    if (keyEvent->key() != 0 && keyEvent->key() != Qt::Key_unknown) {
        qDebug() << "keyEvent->key() != 0 && keyEvent->key() != Qt::Key_unknown";
        keys.append(QKeySequence(keyEvent->key()).toString());
    }

    for (int i = 0; i < keys.count(); i++) {
        if (keys.value(i).contains("Return")) {
            qDebug() << "keys.value(i).contains(Return)";
            keys.replace(i, "Enter");
        }
    }

    qDebug() << "Exit getKeyshortcut, keys:" << keys.join("+");
    return keys.join("+");
}

QString Utils::getKeyshortcutFromKeymap(Settings *settings, const QString &keyCategory, const QString &keyName)
{
    qDebug() << "Enter getKeyshortcutFromKeymap, keyCategory:" << keyCategory << "keyName:" << keyName;
    return settings->settings->option(QString("shortcuts.%1.%2").arg(keyCategory).arg(keyName))->value().toString();
}

QPixmap Utils::dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset)
{
    qDebug() << "Enter dropShadow";
    QImage shadow = dropShadow(source, radius, color);
    shadow.setDevicePixelRatio(source.devicePixelRatio());

    QPainter pa(&shadow);
    pa.setCompositionMode(QPainter::CompositionMode_SourceOver);
    pa.drawPixmap(radius - offset.x(), radius - offset.y(), source);
    pa.end();

    qDebug() << "Exit dropShadow";
    return QPixmap::fromImage(shadow);
}

QImage Utils::dropShadow(const QPixmap &px, qreal radius, const QColor &color)
{
    qDebug() << "Enter dropShadow";
    if (px.isNull()) {
        qDebug() << "px is null";
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
        qDebug() << "color is black";
        return blurred;
    }
    tmp = blurred;

    // Blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    qDebug() << "Exit dropShadow";
    return tmp;
}

qreal Utils::easeInOut(qreal x)
{
    qDebug() << "Enter easeInOut, x:" << x;
    return (1 - qCos(M_PI * x)) / 2;
}

qreal Utils::easeInQuad(qreal x)
{
    qDebug() << "Enter easeInQuad, x:" << x;
    return qPow(x, 2);
}

qreal Utils::easeOutQuad(qreal x)
{
    qDebug() << "Enter easeOutQuad, x:" << x;
    return -1 * qPow(x - 1, 2) + 1;
}

qreal Utils::easeInQuint(qreal x)
{
    qDebug() << "Enter easeInQuint, x:" << x;
    return qPow(x, 5);
}

qreal Utils::easeOutQuint(qreal x)
{
    qDebug() << "Enter easeOutQuint, x:" << x;
    return qPow(x - 1, 5) + 1;
}

QVariantMap Utils::getThemeMapFromPath(const QString &filepath)
{
    qDebug() << "Enter getThemeMapFromPath, filepath:" << filepath;
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open theme file:" << filepath;
        return QVariantMap();
    }

    QTextStream in(&file);
    const QString jsonStr = in.readAll();
    file.close();

    QByteArray jsonBytes = jsonStr.toLocal8Bit();
    QJsonDocument document = QJsonDocument::fromJson(jsonBytes);
    QJsonObject object = document.object();

    qDebug() << "Exit getThemeMapFromPath, object:" << object.toVariantMap();
    return object.toVariantMap();
}

bool Utils::isMimeTypeSupport(const QString &filepath)
{
    qDebug() << "Enter isMimeTypeSupport, filepath:" << filepath;
    QString mimeType = QMimeDatabase().mimeTypeForFile(filepath).name();
    qDebug() << "mimeType:" << mimeType;
    if (mimeType.startsWith("text/")) {
        qDebug() << "mimeType starts with text/";
        return true;
    }

    if (filepath.endsWith("pub")) {
        qDebug() << "filepath ends with pub";
        return true;
    }
    // Please check full mime type list from: https://www.freeformatter.com/mime-types-list.html
    QStringList textMimeTypes;
    textMimeTypes << "application/cmd"
                  << "application/javascript"
                  << "application/json"
                  << "application/pkix-cert"
                  << "application/octet-stream"
                  << "application/sql"
                  << "application/vnd.apple.mpegurl"
                  << "application/vnd.nokia.qt.qmakeprofile"
                  << "application/vnd.nokia.xml.qt.resource"
                  << "application/x-desktop"
                  << "application/x-designer"
                  << "application/x-empty"
                  << "application/x-msdos-program"
                  << "application/x-pearl"
                  << "application/x-php"
                  << "application/x-shellscript"
                  << "application/x-sh"
                  << "application/x-theme"
                  << "application/x-cue"
                  << "application/x-csh"
                  << "application/x-asp"
                  << "application/x-subrip"
                  << "application/x-text"
                  << "application/x-trash"
                  << "application/x-xbel"
                  << "application/x-yaml"
                  << "application/x-pem-key"
                  << "application/xml"
                  << "application/yaml"
                  << "application/x-zerosize"
                  << "image/svg+xml"
                  << "application/x-perl"
                  << "application/x-ruby"
                  << "application/x-mpegURL"
                  << "application/x-wine-extension-ini"
                  << "model/vrml"
                  << "application/pkix-cert+pem"
                  << "application/x-pak"
                  << "application/x-code-workspace"
                  << "application/toml"
                  << "audio/x-mod"
                  << "application/pkix-attr-cert"
                  << "application/x-x509-ca-cert";

    if (textMimeTypes.contains(mimeType)) {
        qDebug() << "mimeType contains textMimeTypes";
        return true;
    }

    qDebug() << "mimeType does not contain textMimeTypes";
    return false;
}

bool Utils::isDraftFile(const QString &filepath)
{
    qDebug() << "Enter isDraftFile, filepath:" << filepath;
    QString draftDir = QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first())
                           .filePath("blank-files");
    draftDir = QDir::cleanPath(draftDir);
    QString dir = QFileInfo(filepath).dir().absolutePath();
    qDebug() << "dir:" << dir;
    qDebug() << "draftDir:" << draftDir;
    qDebug() << "isDraftFile:" << (dir == draftDir);
    return dir == draftDir;
}

/**
 * @param filepath 文件路径
 * @return 返回传入文件路径 \a filepath 是否在备份文件夹 backup-files 中
 */
bool Utils::isBackupFile(const QString &filepath)
{
    qDebug() << "Enter isBackupFile, filepath:" << filepath;
    QString backupDir = QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first())
                            .filePath("backup-files");
    QString dir = QFileInfo(filepath).dir().absolutePath();
    qDebug() << "dir:" << dir;
    qDebug() << "backupDir:" << backupDir;
    qDebug() << "isBackupFile:" << (dir == backupDir);
    return dir == backupDir;
}

QStringList Utils::cleanPath(const QStringList &filePaths)
{
    qDebug() << "Enter cleanPath, filePaths:" << filePaths;
    QStringList paths;
    for (QString path : filePaths) {
        paths.push_back(QDir::cleanPath(path));
    }

    qDebug() << "Exit cleanPath, paths:" << paths;
    return paths;
}

/**
 * @return 返回程序使用的默认数据(存放临时、备份文件)存放位置，不同环境下路径不同
 *  [debian]    /home/user/.local/share/deepin/deepin-editor/
 *  [linglong]  /home/user/.linglong/org.deepin.editor/share/deepin/deepin-editor/
 */
QString Utils::localDataPath()
{
    qDebug() << "Enter localDataPath";
    auto dataPaths = Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation));
    qDebug() << "dataPaths:" << dataPaths;
    return dataPaths.isEmpty() ? QDir::homePath() + "/.local/share/deepin/deepin-editor/" : dataPaths.first();
}

const QStringList Utils::getEncodeList()
{
    qDebug() << "Enter getEncodeList";
    QStringList encodeList;

    for (int mib : QTextCodec::availableMibs()) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);
        QString encodeName = QString(codec->name()).toUpper();

        if (encodeName != "UTF-8" && !encodeList.contains(encodeName)) {
            qDebug() << "encodeName:" << encodeName;
            encodeList.append(encodeName);
        }
    }

    qDebug() << "encodeList:" << encodeList;
    encodeList.sort();
    encodeList.prepend("UTF-8");
    qDebug() << "Exit getEncodeList, encodeList:" << encodeList;
    return encodeList;
}

QPixmap Utils::renderSVG(const QString &filePath, const QSize &size, bool bIsScale)
{
    qDebug() << "Enter renderSVG, filePath:" << filePath << "size:" << size << "bIsScale:" << bIsScale;
    int scaled = 1;

    if (qApp->devicePixelRatio() == 1.25 && bIsScale) {
        qDebug() << "qApp->devicePixelRatio() == 1.25 && bIsScale";
        scaled = 2;
    }

    qDebug() << "scaled:" << scaled;
    QPixmap pixmap(size * scaled);
    pixmap.fill(Qt::transparent);
    qDebug() << "pixmap.fill(Qt::transparent)";
    QImageReader reader;
    qDebug() << "reader.setFileName(filePath)";
    reader.setFileName(filePath);
    qDebug() << "reader.canRead():" << reader.canRead();
    if (reader.canRead()) {
        reader.setScaledSize(size * scaled);
        qDebug() << "reader.setScaledSize(size * scaled)";
        pixmap = QPixmap::fromImage(reader.read());
        qDebug() << "pixmap.setDevicePixelRatio(scaled)";
        pixmap.setDevicePixelRatio(scaled);
    } else {
        qDebug() << "reader.canRead() is false";
        pixmap.load(filePath);
    }

    qDebug() << "Exit renderSVG, pixmap:" << pixmap;
    return pixmap;
}

QList<QColor> Utils::getHiglightColorList()
{
    qDebug() << "Enter getHiglightColorList";
    QList<QColor> listColor;
    listColor.append(QColor("#FFA503"));
    listColor.append(QColor("#FF1C49"));
    listColor.append(QColor("#9023FC"));
    listColor.append(QColor("#3468FF"));
    listColor.append(QColor("#00C7E1"));
    listColor.append(QColor("#05EA6B"));
    listColor.append(QColor("#FEF144"));
    listColor.append(QColor("#D5D5D1"));
    qDebug() << "Exit getHiglightColorList, listColor:" << listColor;
    return listColor;
}

void Utils::clearChildrenFocus(QObject *objParent)
{
    qDebug() << "Enter clearChildrenFocus, objParent:" << objParent;
    // 可以获取焦点的控件名称列表
    QStringList foucswidgetlist;
    // foucswidgetlist << "QLineEdit" << TERM_WIDGET_NAME;

    // qDebug() << "checkChildrenFocus start" << objParent->children().size();
    for (QObject *obj : objParent->children()) {
        if (!obj->isWidgetType()) {
            qDebug() << "obj is not a widget";
            continue;
        }
        QWidget *widget = static_cast<QWidget *>(obj);
        if (Qt::NoFocus != widget->focusPolicy()) {
            qDebug() << "widget->focusPolicy() != Qt::NoFocus";
            // qDebug() << widget << widget->focusPolicy() << widget->metaObject()->className();
            if (!foucswidgetlist.contains(widget->metaObject()->className())) {
                qDebug() << "widget->setFocusPolicy(Qt::NoFocus)";
                widget->setFocusPolicy(Qt::NoFocus);
            }
        }
        qDebug() << "clearChildrenFocus(obj)";
        clearChildrenFocus(obj);
    }

    // qDebug() << "checkChildrenFocus over" << objParent->children().size();
    qDebug() << "Exit clearChildrenFocus";
}

void Utils::clearChildrenFoucusEx(QWidget *pWidget)
{
    qDebug() << "Enter clearChildrenFoucusEx, pWidget:" << pWidget;
    pWidget->clearFocus();

    QObjectList childern = pWidget->children();

    if (childern.size() <= 0)
        return;

    foreach (QObject *child, childern) {
        if (!child->isWidgetType()) {
            qDebug() << "child is not a widget";
            continue;
        }

        QWidget *obj = static_cast<QWidget *>(child);
        qDebug() << "clearChildrenFoucusEx(obj)";
        clearChildrenFoucusEx(obj);
    }
    qDebug() << "Exit clearChildrenFoucusEx";
}

void Utils::setChildrenFocus(QWidget *pWidget, Qt::FocusPolicy policy)
{
    qDebug() << "Enter setChildrenFocus, pWidget:" << pWidget << "policy:" << policy;
    pWidget->setFocusPolicy(policy);

    QObjectList childern = pWidget->children();

    if (childern.size() <= 0)
        return;

    foreach (QObject *child, childern) {
        if (!child->isWidgetType()) {
            qDebug() << "child is not a widget";
            continue;
        }

        QWidget *obj = static_cast<QWidget *>(child);
        qDebug() << "setChildrenFocus(obj, policy)";
        setChildrenFocus(obj, policy);
    }
    qDebug() << "Exit setChildrenFocus";
}

int Utils::getProcessCountByName(const char *pstrName)
{
    qDebug() << "Enter getProcessCountByName, pstrName:" << pstrName;
    FILE *fp = NULL;
    int count = -1;
    char command[1024];

    if (NULL == pstrName || strlen(pstrName) == 0) {
        qDebug() << "pstrName is null";
        return count;
    }

    memset(command, 0, sizeof(command));
    sprintf(command, "ps -ef | grep %s | grep -v grep | wc -l", pstrName);
    qDebug() << "command:" << command;
    if ((fp = popen(command, "r")) != NULL) {
        char buf[1024];
        memset(buf, 0, sizeof(buf));
        if ((fgets(buf, sizeof(buf) - 1, fp)) != NULL) {
            qDebug() << "buf:" << buf;
            count = atoi(buf);
        }
        pclose(fp);
    } else {
        qDebug() << ">>> popen error";
    }
    qDebug() << "Exit getProcessCountByName, count:" << count;
    return count;
}

void Utils::killProcessByName(const char *pstrName)
{
    qDebug() << "Enter killProcessByName, pstrName:" << pstrName;
    if (pstrName != NULL && strlen(pstrName) > 0) {
        char command[1024];
        memset(command, 0, sizeof(command));
        sprintf(command, "killall %s", pstrName);
        qDebug() << "command:" << command;
        system(command);
    }
    qDebug() << "Exit killProcessByName";
}

QString Utils::getStringMD5Hash(const QString &input)
{
    qDebug() << "Enter getStringMD5Hash, input:" << input;
    QByteArray byteArray = input.toUtf8();
    qDebug() << "byteArray:" << byteArray;
    QByteArray md5Path = QCryptographicHash::hash(byteArray, QCryptographicHash::Md5);
    qDebug() << "md5Path:" << md5Path;
    return md5Path.toHex();
}

bool Utils::activeWindowFromDock(quintptr winId)
{
    qDebug() << "Enter activeWindowFromDock, winId:" << winId;
    bool bRet = false;
    // 优先采用V23接口
    QDBusInterface dockDbusInterfaceV23(
        "org.deepin.dde.daemon.Dock1", "/org/deepin/dde/daemon/Dock1", "org.deepin.dde.daemon.Dock1");
    if (dockDbusInterfaceV23.isValid()) {
        QDBusReply<void> reply = dockDbusInterfaceV23.call("ActivateWindow", winId);
        if (!reply.isValid()) {
            qDebug() << "call v23 org.deepin.dde.daemon.Dock1 failed" << reply.error();
        } else {
            qDebug() << "call v23 org.deepin.dde.daemon.Dock1 success";
            return true;
        }
    }

    QDBusInterface dockDbusInterfaceV20(
        "com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", "com.deepin.dde.daemon.Dock");
    if (dockDbusInterfaceV20.isValid() && !bRet) {
        QDBusReply<void> reply = dockDbusInterfaceV20.call("ActivateWindow", winId);
        if (!reply.isValid()) {
            qDebug() << "call v20 com.deepin.dde.daemon.Dock failed" << reply.error();
            bRet = false;
        } else {
            qDebug() << "call v20 com.deepin.dde.daemon.Dock success";
            return true;
        }
    }

    qDebug() << "Exit activeWindowFromDock, bRet:" << bRet;
    return bRet;
}

bool Utils::isShareDirAndReadOnly(const QString &filePath)
{
    qDebug() << "Enter isShareDirAndReadOnly, filePath:" << filePath;
    bool ret = false;

    const QString sharePath = "/var/lib/samba/usershares";
    QDir shareDir(sharePath);
    if (shareDir.exists()) {
        qDebug() << "shareDir exists";
        QFileInfo fileInfo(filePath);
        auto name = fileInfo.dir().dirName();
        if (shareDir.exists(name)) {
            qDebug() << "shareDir.exists(name)";
            QFile file(sharePath + "/" + name);
            if (file.open(QIODevice::ReadOnly)) {
                QString fileContent = file.readAll();
                qDebug() << "fileContent:" << fileContent;
                if (fileContent.contains(":R"))
                    ret = true;
                file.close();
            } else {
                qDebug() << "file.open(QIODevice::ReadOnly) failed";
            }
        }
    }

    qDebug() << "Exit isShareDirAndReadOnly, ret:" << ret;
    return ret;
}

QString Utils::getSystemLan()
{
    qDebug() << "Enter getSystemLan";
    if (!m_systemLanguage.isEmpty()) {
        qDebug() << "m_systemLanguage is not empty";
        return m_systemLanguage;
    } else {
        switch (getSystemVersion()) {
            case V23:
                m_systemLanguage = QLocale::system().name();
                qDebug() << "m_systemLanguage:" << m_systemLanguage;
                break;
            default: {
                QDBusInterface ie("com.deepin.daemon.LangSelector",
                                  "/com/deepin/daemon/LangSelector",
                                  "com.deepin.daemon.LangSelector",
                                  QDBusConnection::sessionBus());
                m_systemLanguage = ie.property("CurrentLocale").toString();
                qDebug() << "m_systemLanguage:" << m_systemLanguage;
                break;
            }
        }

        qDebug() << "getSystemLan is" << m_systemLanguage;
        qDebug() << "Exit getSystemLan";
        return m_systemLanguage;
    }
}

/**
 * @return 获取当前系统版本并返回
 * @note 现在大于 V23 的环境同样返回V23
 *
 * TODO: 新版镜像稳定后更新为更新的版本
 */
Utils::SystemVersion Utils::getSystemVersion()
{
    qDebug() << "Enter getSystemVersion";
    QString version = DSysInfo::majorVersion();
    if (version.toInt() >= 23) {
        qDebug() << "version.toInt() >= 23";
        return V23;
    }
    qDebug() << "Exit getSystemVersion, version:" << version;
    // 其它版本默认V20
    return V20;
}

// judge whether the protocol is wayland
bool Utils::isWayland()
{
    qDebug() << "Enter isWayland";
    static QString protocol;
    if (protocol.isEmpty()) {
        protocol = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
    }
    qDebug() << "Exit isWayland, protocol:" << protocol;
    return protocol.contains("wayland");
}

QString Utils::lineFeed(const QString &text, int nWidth, const QFont &font, int nElidedRow)
{
    qDebug() << "Enter lineFeed, text:" << text << "nWidth:" << nWidth << "font:" << font << "nElidedRow:" << nElidedRow;
    if (nElidedRow < 0) {
        qDebug() << "nElidedRow < 0";
        nElidedRow = 2;
    }
    qDebug() << "nElidedRow:" << nElidedRow;
    QString strText = text;
    QStringList strListLine;
    QFontMetrics fm(font);
    // 一行就直接中间截断显示
    if (1 == nElidedRow) {
        qDebug() << "1 == nElidedRow";
        return fm.elidedText(text, Qt::ElideMiddle, nWidth);
    }
    qDebug() << "strText:" << strText;
    if (!strText.isEmpty()) {
        for (int i = 1; i <= strText.size(); i++) {
            if (fm.horizontalAdvance(strText.left(i)) >= nWidth) {
                qDebug() << "fm.horizontalAdvance(strText.left(i)) >= nWidth";
                if (strListLine.size() + 1 == nElidedRow) {
                    qDebug() << "strListLine.size() + 1 == nElidedRow";
                    break;
                }
                qDebug() << "strListLine.append(strText.left(i - 1))";
                strListLine.append(strText.left(i - 1));
                strText = strText.right(strText.size() - i + 1);
                qDebug() << "strText:" << strText;
                i = 0;
            }
        }
    }
    qDebug() << "strListLine:" << strListLine;
    // 多行时，对最后一行字符左侧省略
    if (!strListLine.isEmpty()) {
        strText = fm.elidedText(strText, Qt::ElideLeft, nWidth);
        strListLine.append(strText);
        strText = strListLine.join('\n');
    }
    qDebug() << "Exit lineFeed, strText:" << strText;
    return strText;
}

/**
 * @brief 判断 [ \a x1, \a y1] 和 [ \a x2, \a y2] 区间是否存在交集
 * @param x1 固定区间左边界
 * @param y1 固定区间右边界
 * @param x2 移动区间左边界
 * @param y2 移动区间右边界
 * @return RegionType 返回重叠区间类型
 */
Utils::RegionIntersectType Utils::checkRegionIntersect(int x1, int y1, int x2, int y2)
{
    qDebug() << "Enter checkRegionIntersect, x1:" << x1 << "y1:" << y1 << "x2:" << x2 << "y2:" << y2;
    if (y1 < x2) {
        qDebug() << "y1 < x2";
        return ERight;
    } else if (x1 > y2) {
        qDebug() << "x1 > y2";
        return ELeft;
    } else {
        // 区间存在交集，判断交集类型
        // 活动区间左边界超过固定区间左边界
        bool outLeftBound = x1 > x2;
        // 活动区间右边界超过固定区间右边界
        bool outRightBound = y1 < y2;

        if (outLeftBound && outRightBound) {
            qDebug() << "outLeftBound && outRightBound";
            return EIntersectOutter;
        } else if (outLeftBound) {
            qDebug() << "outLeftBound";
            return EIntersectLeft;
        } else if (outRightBound) {
            qDebug() << "outRightBound";
            return EIntersectRight;
        } else {
            qDebug() << "else";
            return EIntersectInner;
        }
    }
    qDebug() << "Exit checkRegionIntersect";
}

/**
 * @return 取得当前文本编辑器支持的编码格式，按区域划分，从文件 :/encodes/encodes.ini 中读取
 * @note 非多线程安全，仅在 gui 线程调用
 */
QVector<QPair<QString, QStringList>> Utils::getSupportEncoding()
{
    qDebug() << "Enter getSupportEncoding";
    static QVector<QPair<QString, QStringList>> s_groupEncodeVec;
    if (s_groupEncodeVec.isEmpty()) {
        qDebug() << "s_groupEncodeVec is empty";
        QVector<QPair<QString, QStringList>> tmpEncodeVec;

        QFile file(":/encodes/encodes.ini");
        QString data;
        if (file.open(QIODevice::ReadOnly)) {
            qDebug() << "file.open(QIODevice::ReadOnly) success";
            data = QString::fromUtf8(file.readAll());
            file.close();
        } else {
            qDebug() << "file.open(QIODevice::ReadOnly) failed";
        }
        qDebug() << "data:" << data;
        QTextStream readStream(&data, QIODevice::ReadOnly);
        while (!readStream.atEnd()) {
            qDebug() << "readStream.readLine()";
            QString group = readStream.readLine();
            qDebug() << "group:" << group;
            QString key = group.mid(1, group.length() - 2);
            QString encodes = readStream.readLine();
            qDebug() << "encodes:" << encodes;
            QString value = encodes.mid(8, encodes.length() - 2);
            qDebug() << "value:" << value;
            tmpEncodeVec.append(QPair<QString, QStringList>(key, value.split(",")));
        }
        qDebug() << "tmpEncodeVec:" << tmpEncodeVec;
        s_groupEncodeVec = tmpEncodeVec;
    }
    qDebug() << "Exit getSupportEncoding, s_groupEncodeVec:" << s_groupEncodeVec;
    return s_groupEncodeVec;
}

/**
 * @return 取得当前文本编辑器支持的编码格式列表
 */
QStringList Utils::getSupportEncodingList()
{
    qDebug() << "Enter getSupportEncodingList";
    QStringList encodingList;
    auto supportEncoding = getSupportEncoding();
    qDebug() << "supportEncoding:" << supportEncoding;
    for (auto encodingData : supportEncoding) {
        encodingList.append(encodingData.second);
    }
    std::sort(encodingList.begin(), encodingList.end());
    qDebug() << "Exit getSupportEncodingList, encodingList:" << encodingList;
    return encodingList;
}

QString Utils::libPath(const QString &strlib)
{
    qDebug() << "Enter libPath, strlib:" << strlib;
    QDir dir;
    QString path = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    dir.setPath(path);
    QStringList list =
        dir.entryList(QStringList() << (strlib + "*"), QDir::NoDotAndDotDot | QDir::Files);  // filter name with strlib
    qDebug() << "list:" << list;
    if (list.contains(strlib)) {
        qDebug() << "list contains strlib";
        return strlib;
    }
    qDebug() << "list does not contain strlib";
    list.sort();
    qDebug() << "list sorted:" << list;
    if (list.size() <= 0) {
        qDebug() << "list size <= 0";
        return "";
    }
    qDebug() << "list size > 0";
    return list.last();
}

void Utils::loadCustomDLL()
{
    qDebug() << "Enter loadCustomDLL";
    // 解析ZPD定制需求提供的库libzpdcallback.so
    LoadLibNames tmp;
    QByteArray zpdDll = Utils::libPath("libzpdcallback.so").toLatin1();
    qDebug() << "zpdDll:" << zpdDll;
    if (QFile::exists(zpdDll)) {
        qDebug() << "QFile::exists(zpdDll)";
        tmp.chZPDDLL = zpdDll.data();
    } else {
        qDebug() << "QFile::exists(zpdDll) failed";
        tmp.chZPDDLL = NULL;
    }
    setLibNames(tmp);
    qDebug() << "Exit loadCustomDLL";
}

/**
 * @brief 根据传入的文件路径 \a filePath 返回是否允许对此文件内容进行剪切或拷贝
 * @param filePath 文件完整路径
 * @return 是否允许剪切或拷贝
 */
bool Utils::enableClipCopy(const QString &filePath)
{
    qDebug() << "Enter enableClipCopy, filePath:" << filePath;
#if _ZPD_
    if (getLoadZPDLibsInstance()->m_document_clip_copy) {
        // intercept 输出为1,拦截操作
        static const int disableFlag = 1;
        int intercept = 1;
        getLoadZPDLibsInstance()->m_document_clip_copy(filePath.toUtf8().data(), &intercept);
        qDebug() << "intercept:" << intercept;
        if (disableFlag == intercept) {
            qWarning() << qPrintable("ZPD access control! Disable clip or copy document!");
            return false;
        }
    }
#endif
    qDebug() << "Exit enableClipCopy, return true";
    return true;
}

/**
 * @brief 文件关闭前记录当前关闭的文件路径 \a filePath
 * @param filePath 文件完整路径
 */
void Utils::recordCloseFile(const QString &filePath)
{
    qDebug() << "Enter recordCloseFile, filePath:" << filePath;
#if _ZPD_
    if (getLoadZPDLibsInstance()->m_document_close) {
        qDebug() << "getLoadZPDLibsInstance()->m_document_close";
        getLoadZPDLibsInstance()->m_document_close(filePath.toUtf8().data());
    } else {
        qDebug() << "getLoadZPDLibsInstance()->m_document_close is null";
    }
#endif
    qDebug() << "Exit recordCloseFile";
}

/**
   @brief 发送弹窗提示消息 \a message , 使用 \a icon 设置提示图标，\a par 是浮动窗口计算位置的父窗口。
        这个函数效果和 DMessageManager::sendMessage() 类似，但是提示信息的文字字体将跟随 qApp 变化,
        而不是直接使用父窗口的字体。
 */
void Utils::sendFloatMessageFixedFont(QWidget *par, const QIcon &icon, const QString &message)
{
    qDebug() << "Enter sendFloatMessageFixedFont, par:" << par << "icon:" << icon << "message:" << message;
    // 以下代码和 DMessageManager::sendMessage() 流程一致。
    if (QWidget *content = par->findChild<QWidget *>("_d_message_manager_content", Qt::FindDirectChildrenOnly)) {
        auto msgWidgets = content->findChildren<DFloatingMessage *>(QString(), Qt::FindDirectChildrenOnly);
        auto text_message_count = std::count_if(msgWidgets.begin(), msgWidgets.end(), [](DFloatingMessage *msg) {
            return bool(msg->messageType() == DFloatingMessage::TransientType);
        });

        // TransientType 类型的通知消息，最多只允许同时显示三个
        if (text_message_count >= 3) {
            qDebug() << "text_message_count >= 3";
            return;
        }
    }

    // 浮动临时提示信息，自动销毁
    DFloatingMessage *floMsg = new DFloatingMessage(DFloatingMessage::TransientType);
    floMsg->setAttribute(Qt::WA_DeleteOnClose);
    floMsg->setIcon(icon);
    floMsg->setMessage(message);
    floMsg->setFont(qApp->font());
    qDebug() << "floMsg:" << floMsg;
#ifdef DTKWIDGET_CLASS_DSizeMode
    // 绑定 qApp 字体变更信号
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, floMsg, [=](const QFont &font) {
        floMsg->setFont(font);
    });
#endif
    qDebug() << "DMessageManager::instance()->sendMessage(par, floMsg)";
    DMessageManager::instance()->sendMessage(par, floMsg);
    qDebug() << "Exit sendFloatMessageFixedFont";
}

/**
 * @brief Gets system memory information from /proc/meminfo
 * @param[out] totalMemory Total system memory in KB
 * @param[out] freeMemory Available memory in KB (MemAvailable if exists, otherwise MemFree+Buffers+Cached)
 * @return True if memory info was successfully read and parsed, false otherwise
 */
bool Utils::getSystemMemoryInfo(qlonglong &totalMemory, qlonglong &freeMemory)
{
    qDebug() << "Enter getSystemMemoryInfo";
    qlonglong memFree = 0;
    qlonglong buffers = 0;
    qlonglong cached = 0;
    bool valueOk = false;

    QFile file(PROC_MEMINFO_PATH);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Utils: Failed to open" << PROC_MEMINFO_PATH << "for memory check.";
        return false;
    }

    QByteArray allData = file.readAll();
    file.close();

    QTextStream stream(allData);
    QString line;
    while (!stream.atEnd()) {
        line = stream.readLine();
        if (line.startsWith("MemTotal:")) {
            totalMemory = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qDebug() << "totalMemory:" << totalMemory;
        } else if (line.startsWith("MemFree:")) {
            memFree = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qDebug() << "memFree:" << memFree;
        } else if (line.startsWith("Buffers:")) {
            buffers = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qDebug() << "buffers:" << buffers;
        } else if (line.startsWith("Cached:")) {
            cached = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qDebug() << "cached:" << cached;
        } else if (line.startsWith("MemAvailable:")) {
            freeMemory = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qDebug() << "freeMemory:" << freeMemory;
            break;
        }
        if (stream.atEnd() && freeMemory == 0) {
            freeMemory = memFree + buffers + cached;
            qDebug() << "freeMemory:" << freeMemory;
        }
    }
    qDebug() << "Exit getSystemMemoryInfo, totalMemory:" << totalMemory << "freeMemory:" << freeMemory;
    return (totalMemory > 0 && freeMemory > 0);
}

/**
 * @brief Checks if the system has sufficient memory to perform the specified operation.
 * @param operationType The type of operation (Copy/Paste).
 * @param operationDataSize The size of the data involved in the operation (bytes).
 * @param currentDocumentSize The current size of the document (characters/bytes).
 * @return True if memory is sufficient, false otherwise.
 */
bool Utils::isMemorySufficientForOperation(OperationType operationType, qlonglong operationDataSize, qlonglong currentDocumentSize)
{
    qDebug() << "Enter isMemorySufficientForOperation, operationType:" << operationType << "operationDataSize:" << operationDataSize << "currentDocumentSize:" << currentDocumentSize;

    qlonglong memoryFree = 0;
    qlonglong memoryTotal = 0;

    if (!getSystemMemoryInfo(memoryTotal, memoryFree)) {
        // Conservatively allow the operation if memory info cannot be read
        qWarning() << "Failed to get system memory info, conservatively allowing operation";
        return true;
    }

    // Convert KB to Bytes for comparison
    qlonglong availableMemoryBytes = memoryFree * DATA_SIZE_1024;
    qlonglong totalMemoryBytes = memoryTotal * DATA_SIZE_1024;

    // Judge based on operation type
    switch (operationType) {
        case OperationType::RawOperation:
            // Raw operation: Directly compare the data size with available memory
            if (operationDataSize > availableMemoryBytes) {
                qWarning() << "Utils: Insufficient memory for raw operation. Needed:" << operationDataSize << "Available:" << availableMemoryBytes;
                return false;
            }
            break;
            
        case OperationType::CopyOperation: {
            // Copy operation: Estimated memory consumption = data size * factor
            qlonglong estimatedMemoryNeeded = operationDataSize * COPY_CONSUME_MEMORY_MULTIPLE;
            if (estimatedMemoryNeeded > availableMemoryBytes) {
                qWarning() << "Utils: Insufficient memory for copy operation. Needed(est):" << estimatedMemoryNeeded << "Available:" << availableMemoryBytes;
                return false;
            }
            break;
        }
            
        case OperationType::PasteOperation: {
            // Paste operation: Estimated memory consumption = paste data size * factor
            qlonglong estimatedMemoryNeededForPaste = operationDataSize * PASTE_CONSUME_MEMORY_MULTIPLE;
            // Estimated total document memory after paste
            qlonglong estimatedTotalDocMemory = (currentDocumentSize + operationDataSize) * PASTE_CONSUME_MEMORY_MULTIPLE;

            // Check if pasting the data itself would cause insufficient memory
            if (estimatedMemoryNeededForPaste > availableMemoryBytes) {
                qWarning() << "Utils: Insufficient memory for paste operation (paste data). Needed(est):" << estimatedMemoryNeededForPaste << "Available:" << availableMemoryBytes;
                return false;
            }

            // Check if the estimated total document size after paste exceeds total system memory
            if (estimatedTotalDocMemory > totalMemoryBytes) {
                qWarning() << "Utils: Paste operation might exceed total system memory. Estimated total doc memory:" << estimatedTotalDocMemory << "Total system memory:" << totalMemoryBytes;
                return false;
            }

            // Check specific threshold: Restrict paste size if document reaches 800MB
            const qlonglong DOC_SIZE_LIMIT_800MB = 800LL * DATA_SIZE_1024 * DATA_SIZE_1024;
            const qlonglong PASTE_SIZE_LIMIT_500KB = 500LL * DATA_SIZE_1024;
            if (currentDocumentSize > DOC_SIZE_LIMIT_800MB && operationDataSize > PASTE_SIZE_LIMIT_500KB) {
                qWarning() << "Utils: Paste operation restricted. Document size exceeds 800MB and paste data exceeds 500KB.";
                return false;
            }
            break;
        }

        default:
            qDebug() << "default";
            break;
    }

    qDebug() << "Exit isMemorySufficientForOperation, return true";
    return true; // Memory is sufficient
}
