// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
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

static const QStringList SupportedTextMimeTypes = {
    "application/cmd",
    "application/javascript",
    "application/json",
    "application/pkix-cert",
    "application/octet-stream",
    "application/sql",
    "application/vnd.apple.mpegurl",
    "application/vnd.nokia.qt.qmakeprofile",
    "application/vnd.nokia.xml.qt.resource",
    "application/x-desktop",
    "application/x-designer",
    "application/x-empty",
    "application/x-msdos-program",
    "application/x-pearl",
    "application/x-php",
    "application/x-shellscript",
    "application/x-sh",
    "application/x-theme",
    "application/x-cue",
    "application/x-csh",
    "application/x-asp",
    "application/x-subrip",
    "application/x-text",
    "application/x-trash",
    "application/x-xbel",
    "application/x-yaml",
    "application/x-pem-key",
    "application/xml",
    "application/yaml",
    "application/x-zerosize",
    "image/svg+xml",
    "application/x-perl",
    "application/x-ruby",
    "application/x-mpegURL",
    "application/x-wine-extension-ini",
    "model/vrml",
    "application/pkix-cert+pem",
    "application/x-pak",
    "application/x-code-workspace",
    "application/toml",
    "audio/x-mod",
    "application/pkix-attr-cert",
    "application/x-x509-ca-cert"
};

QString Utils::m_systemLanguage;

QString Utils::getQrcPath(const QString &imageName)
{
    qCDebug(appLog) << "Enter getQrcPath, imageName:" << imageName;
    QString path = QString(":/images/%1").arg(imageName);
    qCDebug(appLog) << "Exit getQrcPath, return path:" << path;
    return path;
}

QString Utils::getQssPath(const QString &qssName)
{
    qCDebug(appLog) << "Enter getQssPath, qssName:" << qssName;
    QString path = QString(":/qss/%1").arg(qssName);
    qCDebug(appLog) << "Exit getQssPath, return path:" << path;
    return path;
}

QSize Utils::getRenderSize(int fontSize, const QString &string)
{
    qCDebug(appLog) << "Entering getRenderSize with fontSize:" << fontSize << "and string:" << string.left(50);
    QFont font;
    font.setPointSize(fontSize);
    QFontMetrics fm(font);

    int width = 0;
    int height = 0;

    for (const QString &line : string.split("\n")) {
        int lineWidth = fm.horizontalAdvance(line);
        int lineHeight = fm.height();

        if (lineWidth > width) {
            qCDebug(appLog) << "New max width found:" << lineWidth;
            width = lineWidth;
        }

        height += lineHeight;
    }

    qCDebug(appLog) << "Leaving getRenderSize, returning QSize(" << width << "," << height << ")";
    return QSize(width, height);
}

void Utils::setFontSize(QPainter &painter, int textSize)
{
    qCDebug(appLog) << "Entering setFontSize with textSize:" << textSize;
    QFont font = painter.font();
    font.setPointSize(textSize);
    painter.setFont(font);
    qCDebug(appLog) << "Leaving setFontSize";
}

void Utils::applyQss(QWidget *widget, const QString &qssName)
{
    qCDebug(appLog) << "Entering applyQss for widget" << widget->objectName() << "with qss" << qssName;
    QFile file(Utils::getQssPath(qssName));
    file.open(QFile::ReadOnly);
    QTextStream filetext(&file);
    QString stylesheet = filetext.readAll();
    widget->setStyleSheet(stylesheet);
    file.close();
    qCDebug(appLog) << "Leaving applyQss";
}

float Utils::codecConfidenceForData(const QTextCodec *codec, const QByteArray &data, const QLocale::Country &country)
{
    qCDebug(appLog) << "Entering codecConfidenceForData for codec:" << codec->name();
    qreal hep_count = 0;
    int non_base_latin_count = 0;
    qreal unidentification_count = 0;
    int replacement_count = 0;

    QTextDecoder decoder(codec);
    const QString &unicode_data = decoder.toUnicode(data);

    for (int i = 0; i < unicode_data.size(); ++i) {
        const QChar &ch = unicode_data.at(i);

        if (ch.unicode() > 0x7f) {
            qCDebug(appLog) << "Non-base Latin character found:" << ch;
            ++non_base_latin_count;
        }

        switch (ch.script()) {
            case QChar::Script_Hiragana:
            case QChar::Script_Katakana:
                qCDebug(appLog) << "Hiragana or Katakana character found:" << ch;
                hep_count += (country == QLocale::Japan) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::Japan) ? 0 : 0.3;
                break;
            case QChar::Script_Han:
                qCDebug(appLog) << "Chinese character found:" << ch;
                hep_count += (country == QLocale::China) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::China) ? 0 : 0.3;
                break;
            case QChar::Script_Hangul:
                qCDebug(appLog) << "Hangul character found:" << ch;
                hep_count += (country == QLocale::NorthKorea) || (country == QLocale::SouthKorea) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::NorthKorea) || (country == QLocale::SouthKorea) ? 0 : 0.3;
                break;
            case QChar::Script_Cyrillic:
                qCDebug(appLog) << "Cyrillic character found:" << ch;
                hep_count += (country == QLocale::Russia) ? 1.2 : 0.5;
                unidentification_count += (country == QLocale::Russia) ? 0 : 0.3;
                break;
            case QChar::Script_Devanagari:
                qCDebug(appLog) << "Devanagari character found:" << ch;
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

    qCDebug(appLog) << "Leaving codecConfidenceForData, confidence:" << c;
    return qMax(0.0f, c);
}

QByteArray Utils::detectEncode(const QByteArray &data, const QString &fileName)
{
    qCDebug(appLog) << "Entering detectEncode for file:" << fileName;
    // Return local encoding if nothing in file.
    if (data.isEmpty()) {
        qCDebug(appLog) << "Data is empty, returning codec for locale.";
        return QTextCodec::codecForLocale()->name();
    }

    if (QTextCodec *c = QTextCodec::codecForUtfText(data, nullptr)) {
        qCDebug(appLog) << "Detected UTF text, codec:" << c->name();
        return c->name();
    }

    QMimeDatabase mime_database;
    const QMimeType &mime_type =
        fileName.isEmpty() ? mime_database.mimeTypeForData(data) : mime_database.mimeTypeForFileNameAndData(fileName, data);
    const QString &mimetype_name = mime_type.name();
    qCDebug(appLog) << "MIME type:" << mimetype_name;
    KEncodingProber::ProberType proberType = KEncodingProber::Universal;

    if (mimetype_name == QStringLiteral("application/xml") || mimetype_name == QStringLiteral("text/html") ||
        mimetype_name == QStringLiteral("application/xhtml+xml")) {
        qCDebug(appLog) << "XML/HTML mimetype found, checking for charset meta tag.";
        const QString &_data = QString::fromLatin1(data);
        QRegularExpression pattern("<\\bmeta.+\\bcharset=(?'charset'\\S+?)\\s*['\"/>]");

        pattern.setPatternOptions(QRegularExpression::DontCaptureOption | QRegularExpression::CaseInsensitiveOption);
        const QString &charset =
            pattern
                .match(
                    _data, 0, QRegularExpression::PartialPreferFirstMatch, QRegularExpression::DontCheckSubjectStringMatchOption)
                .captured("charset");

        if (!charset.isEmpty()) {
            qCDebug(appLog) << "Found charset in meta tag:" << charset;
            return charset.toLatin1();
        }

        pattern.setPattern("<\\bmeta\\s+http-equiv=\"Content-Language\"\\s+content=\"(?'language'[a-zA-Z-]+)\"");

        const QString &language =
            pattern
                .match(
                    _data, 0, QRegularExpression::PartialPreferFirstMatch, QRegularExpression::DontCheckSubjectStringMatchOption)
                .captured("language");

        if (0 != language.size()) {
            qCDebug(appLog) << "Found language in meta tag:" << language;
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
            qCDebug(appLog) << "Set prober type based on language:" << proberType;
        }
    } else if (mimetype_name == "text/x-python") {
        qCDebug(appLog) << "Python mimetype found, checking for coding meta tag.";
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
            qCDebug(appLog) << "i.first != proberType && qFuzzyIsNull(prober_confidence)";
            prober_confidence = pre_confidence;
            prober_encoding = pre_encoding;
        }

    confidence:
        qCDebug(appLog) << "go to here: confidence!";
        if (QTextCodec *codec = QTextCodec::codecForName(prober_encoding)) {
            if (def_codec == codec) {
                qCDebug(appLog) << "def_codec == codec";
                def_codec = nullptr;
            }

            float c = Utils::codecConfidenceForData(codec, data, i.second);

            if (prober_confidence > 0.5) {
                qCDebug(appLog) << "prober_confidence > 0.5";
                c = c / 2 + prober_confidence / 2;
            } else {
                qCDebug(appLog) << "prober_confidence <= 0.5";
                c = c / 3 * 2 + prober_confidence / 3;
            }

            if (c > confidence) {
                qCDebug(appLog) << "c > confidence!";
                confidence = c;
                encoding = prober_encoding;
            }

            if (i.first == KEncodingProber::ChineseTraditional && c < 0.5) {
                qCDebug(appLog) << "i.first == KEncodingProber::ChineseTraditional && c < 0.5";
                // test Big5
                c = Utils::codecConfidenceForData(QTextCodec::codecForName("Big5"), data, i.second);

                if (c > 0.5 && c > confidence) {
                    qCDebug(appLog) << " 0.5 set Big5";
                    confidence = c;
                    encoding = "Big5";
                }
            }
        }

        if (i.first != proberType) {
            qCDebug(appLog) << "i.first != proberType";
            // 使用 proberType 类型探测出的结果结合此国家再次做编码检查
            i.first = proberType;
            prober_confidence = pre_confidence;
            prober_encoding = pre_encoding;
            goto confidence;
        }
    }

    if (def_codec && Utils::codecConfidenceForData(def_codec, data, QLocale::system().country()) > confidence) {
        qCDebug(appLog) << "return codec's name";
        return def_codec->name();
    }

    qCDebug(appLog) << "final return!";
    return encoding;
}

QByteArray Utils::getEncode(const QByteArray &data)
{
    qCDebug(appLog) << "Enter getEncode";
    // try to get HTML header encoding.
    if (QTextCodec *codecForHtml = QTextCodec::codecForHtml(data, nullptr)) {
        qCDebug(appLog) << "codecForHtml" << codecForHtml->name();
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
        qCDebug(appLog) << "codec is null";
        return QByteArray();
    }

    qCDebug(appLog) << "codec" << codec->name();
    return codec->name();
}

bool Utils::fileExists(const QString &path)
{
    qCDebug(appLog) << "Enter fileExists, path:" << path;
    QFileInfo check_file(path);
    qCDebug(appLog) << "check_file" << check_file.exists() << check_file.isFile();
    bool exists = check_file.exists() && check_file.isFile();
    qCDebug(appLog) << "Exit fileExists, file exists:" << exists;
    return exists;
}

QString Utils::getFilePath(const QString &filePath)
{
    QFileInfo info(filePath);
    QString canonicalPath = info.canonicalFilePath();
    // canonicalFilePath() returns empty string for non-existent files
    // Use absoluteFilePath() as fallback to support creating new files
    if (canonicalPath.isEmpty()) {
        return info.absoluteFilePath();
    }
    return canonicalPath;
}

bool Utils::fileIsWritable(const QString &path)
{
    qCDebug(appLog) << "Enter fileIsWritable, path:" << path;
    QFileDevice::Permissions permissions = QFile(path).permissions();
    qCDebug(appLog) << "permissions" << permissions;
    bool writable = permissions & QFileDevice::WriteUser;
    qCDebug(appLog) << "Exit fileIsWritable, is writable:" << writable;
    return writable;
}

bool Utils::fileIsHome(const QString &path)
{
    qCDebug(appLog) << "Enter fileIsHome, path:" << path;
    bool isHome = path.startsWith(QDir::homePath());
    qCDebug(appLog) << "Exit fileIsHome, isHome:" << isHome;
    return isHome;
}

QString Utils::getKeyshortcut(QKeyEvent *keyEvent)
{
    qCDebug(appLog) << "Enter getKeyshortcut";
    QStringList keys;
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    if (modifiers != Qt::NoModifier) {
        qCDebug(appLog) << "modifiers" << modifiers;
        if (modifiers.testFlag(Qt::MetaModifier)) {
            qCDebug(appLog) << "modifiers.testFlag(Qt::MetaModifier)";
            keys.append("Meta");
        }

        if (modifiers.testFlag(Qt::ControlModifier)) {
            qCDebug(appLog) << "modifiers.testFlag(Qt::ControlModifier)";
            keys.append("Ctrl");
        }

        if (modifiers.testFlag(Qt::AltModifier)) {
            qCDebug(appLog) << "modifiers.testFlag(Qt::AltModifier)";
            keys.append("Alt");
        }

        if (modifiers.testFlag(Qt::ShiftModifier)) {
            qCDebug(appLog) << "modifiers.testFlag(Qt::ShiftModifier)";
            keys.append("Shift");
        }

        // 添加小键盘处理，若为小键盘按键按下，组合键需添加 Num ，例如 Ctrl+Num+6 / Ctrl+Num+Up
        if (modifiers.testFlag(Qt::KeypadModifier)) {
            qCDebug(appLog) << "modifiers.testFlag(Qt::KeypadModifier)";
            keys.append("Num");
        }
    }

    if (keyEvent->key() != 0 && keyEvent->key() != Qt::Key_unknown) {
        qCDebug(appLog) << "keyEvent->key() != 0 && keyEvent->key() != Qt::Key_unknown";
        keys.append(QKeySequence(keyEvent->key()).toString());
    }

    for (int i = 0; i < keys.count(); i++) {
        if (keys.value(i).contains("Return")) {
            qCDebug(appLog) << "keys.value(i).contains(Return)";
            keys.replace(i, "Enter");
        }
        // 与 Return→Enter 同理：统一为常见写法，和配置/显示 "Ctrl+Shift+Tab" 一致
        if (keys.value(i) == "Backtab") {
            keys.replace(i, "Tab");
        }
    }

    qCDebug(appLog) << "Exit getKeyshortcut, keys:" << keys.join("+");
    return keys.join("+");
}

QString Utils::getKeyshortcutFromKeymap(Settings *settings, const QString &keyCategory, const QString &keyName)
{
    qCDebug(appLog) << "Enter getKeyshortcutFromKeymap, keyCategory:" << keyCategory << "keyName:" << keyName;
    return settings->settings->option(QString("shortcuts.%1.%2").arg(keyCategory).arg(keyName))->value().toString();
}

QPixmap Utils::dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset)
{
    qCDebug(appLog) << "Enter dropShadow";
    QImage shadow = dropShadow(source, radius, color);
    shadow.setDevicePixelRatio(source.devicePixelRatio());

    QPainter pa(&shadow);
    pa.setCompositionMode(QPainter::CompositionMode_SourceOver);
    pa.drawPixmap(radius - offset.x(), radius - offset.y(), source);
    pa.end();

    qCDebug(appLog) << "Exit dropShadow";
    return QPixmap::fromImage(shadow);
}

QImage Utils::dropShadow(const QPixmap &px, qreal radius, const QColor &color)
{
    qCDebug(appLog) << "Enter dropShadow";
    if (px.isNull()) {
        qCDebug(appLog) << "px is null";
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
        qCDebug(appLog) << "color is black";
        return blurred;
    }
    tmp = blurred;

    // Blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    qCDebug(appLog) << "Exit dropShadow";
    return tmp;
}

qreal Utils::easeInOut(qreal x)
{
    qCDebug(appLog) << "Enter easeInOut, x:" << x;
    return (1 - qCos(M_PI * x)) / 2;
}

qreal Utils::easeInQuad(qreal x)
{
    qCDebug(appLog) << "Enter easeInQuad, x:" << x;
    return qPow(x, 2);
}

qreal Utils::easeOutQuad(qreal x)
{
    qCDebug(appLog) << "Enter easeOutQuad, x:" << x;
    return -1 * qPow(x - 1, 2) + 1;
}

qreal Utils::easeInQuint(qreal x)
{
    qCDebug(appLog) << "Enter easeInQuint, x:" << x;
    return qPow(x, 5);
}

qreal Utils::easeOutQuint(qreal x)
{
    qCDebug(appLog) << "Enter easeOutQuint, x:" << x;
    return qPow(x - 1, 5) + 1;
}

QVariantMap Utils::getThemeMapFromPath(const QString &filepath)
{
    qCDebug(appLog) << "Enter getThemeMapFromPath, filepath:" << filepath;
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(appLog) << "Failed to open theme file:" << filepath;
        return QVariantMap();
    }

    QTextStream in(&file);
    const QString jsonStr = in.readAll();
    file.close();

    QByteArray jsonBytes = jsonStr.toLocal8Bit();
    QJsonDocument document = QJsonDocument::fromJson(jsonBytes);
    QJsonObject object = document.object();

    qCDebug(appLog) << "Exit getThemeMapFromPath, object:" << object.toVariantMap();
    return object.toVariantMap();
}

bool Utils::isMimeTypeSupport(const QString &filepath)
{
    qCDebug(appLog) << "Enter isMimeTypeSupport, filepath:" << filepath;
    QMimeType mime = QMimeDatabase().mimeTypeForFile(filepath, QMimeDatabase::MatchMode::MatchContent);
    QString mimeType = mime.name();
    qCDebug(appLog) << "mimeType:" << mimeType;
    if (mimeType.startsWith("text/")) {
        qCDebug(appLog) << "mimeType starts with text/";
        return true;
    }

    if (filepath.endsWith("pub")) {
        qCDebug(appLog) << "filepath ends with pub";
        return true;
    }

    if (SupportedTextMimeTypes.contains(mimeType)) {
        qCDebug(appLog) << "mimeType contains textMimeTypes";
        return true;
    }

    // Check if the MIME type inherits from any supported type
    // e.g. application/schema+json inherits from application/json
    if (mime.isValid()) {
        for (const QString &supported : SupportedTextMimeTypes) {
            // 万能二进制类型不参与 inherits() 判定：
            // 几乎所有二进制 mime（如 image/png）都继承自 application/octet-stream，
            // 若参与判定会导致二进制文件被误放行为可打开的文本类型
            if (supported == QLatin1String("application/octet-stream")) {
                continue;
            }
            if (mime.inherits(supported)) {
                qCDebug(appLog) << "mimeType inherits from supported type:" << supported;
                return true;
            }
        }
    }

    qCDebug(appLog) << "mimeType does not contain textMimeTypes";
    return false;
}

bool Utils::isDraftFile(const QString &filepath)
{
    qCDebug(appLog) << "Enter isDraftFile, filepath:" << filepath;
    QString draftDir = QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first())
                           .filePath("blank-files");
    draftDir = QDir::cleanPath(draftDir);
    QString dir = QFileInfo(filepath).dir().absolutePath();
    qCDebug(appLog) << "dir:" << dir;
    qCDebug(appLog) << "draftDir:" << draftDir;
    qCDebug(appLog) << "isDraftFile:" << (dir == draftDir);
    return dir == draftDir;
}

/**
 * @param filepath 文件路径
 * @return 返回传入文件路径 \a filepath 是否在备份文件夹 backup-files 中
 */
bool Utils::isBackupFile(const QString &filepath)
{
    qCDebug(appLog) << "Enter isBackupFile, filepath:" << filepath;
    QString backupDir = QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first())
                            .filePath("backup-files");
    QString dir = QFileInfo(filepath).dir().absolutePath();
    qCDebug(appLog) << "dir:" << dir;
    qCDebug(appLog) << "backupDir:" << backupDir;
    qCDebug(appLog) << "isBackupFile:" << (dir == backupDir);
    return dir == backupDir;
}

QStringList Utils::cleanPath(const QStringList &filePaths)
{
    qCDebug(appLog) << "Enter cleanPath, filePaths:" << filePaths;
    QStringList paths;
    for (QString path : filePaths) {
        paths.push_back(QDir::cleanPath(path));
    }

    qCDebug(appLog) << "Exit cleanPath, paths:" << paths;
    return paths;
}

/**
 * @return 返回程序使用的默认数据(存放临时、备份文件)存放位置，不同环境下路径不同
 *  [debian]    /home/user/.local/share/deepin/deepin-editor/
 *  [linglong]  /home/user/.linglong/org.deepin.editor/share/deepin/deepin-editor/
 */
QString Utils::localDataPath()
{
    qCDebug(appLog) << "Enter localDataPath";
    auto dataPaths = Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation));
    qCDebug(appLog) << "dataPaths:" << dataPaths;
    return dataPaths.isEmpty() ? QDir::homePath() + "/.local/share/deepin/deepin-editor/" : dataPaths.first();
}

const QStringList Utils::getEncodeList()
{
    qCDebug(appLog) << "Enter getEncodeList";
    QStringList encodeList;

    for (int mib : QTextCodec::availableMibs()) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);
        QString encodeName = QString(codec->name()).toUpper();

        if (encodeName != "UTF-8" && !encodeList.contains(encodeName)) {
            qCDebug(appLog) << "encodeName:" << encodeName;
            encodeList.append(encodeName);
        }
    }

    qCDebug(appLog) << "encodeList:" << encodeList;
    encodeList.sort();
    encodeList.prepend("UTF-8");
    qCDebug(appLog) << "Exit getEncodeList, encodeList:" << encodeList;
    return encodeList;
}

QPixmap Utils::renderSVG(const QString &filePath, const QSize &size, bool bIsScale)
{
    qCDebug(appLog) << "Enter renderSVG, filePath:" << filePath << "size:" << size << "bIsScale:" << bIsScale;
    int scaled = 1;

    if (qApp->devicePixelRatio() == 1.25 && bIsScale) {
        qCDebug(appLog) << "qApp->devicePixelRatio() == 1.25 && bIsScale";
        scaled = 2;
    }

    qCDebug(appLog) << "scaled:" << scaled;
    QPixmap pixmap(size * scaled);
    pixmap.fill(Qt::transparent);
    qCDebug(appLog) << "pixmap.fill(Qt::transparent)";
    QImageReader reader;
    qCDebug(appLog) << "reader.setFileName(filePath)";
    reader.setFileName(filePath);
    qCDebug(appLog) << "reader.canRead():" << reader.canRead();
    if (reader.canRead()) {
        reader.setScaledSize(size * scaled);
        qCDebug(appLog) << "reader.setScaledSize(size * scaled)";
        pixmap = QPixmap::fromImage(reader.read());
        qCDebug(appLog) << "pixmap.setDevicePixelRatio(scaled)";
        pixmap.setDevicePixelRatio(scaled);
    } else {
        qCDebug(appLog) << "reader.canRead() is false";
        pixmap.load(filePath);
    }

    qCDebug(appLog) << "Exit renderSVG, pixmap:" << pixmap;
    return pixmap;
}

QList<QColor> Utils::getHiglightColorList()
{
    qCDebug(appLog) << "Enter getHiglightColorList";
    QList<QColor> listColor;
    listColor.append(QColor("#FFA503"));
    listColor.append(QColor("#FF1C49"));
    listColor.append(QColor("#9023FC"));
    listColor.append(QColor("#3468FF"));
    listColor.append(QColor("#00C7E1"));
    listColor.append(QColor("#05EA6B"));
    listColor.append(QColor("#FEF144"));
    listColor.append(QColor("#D5D5D1"));
    qCDebug(appLog) << "Exit getHiglightColorList, listColor:" << listColor;
    return listColor;
}

void Utils::clearChildrenFocus(QObject *objParent)
{
    qCDebug(appLog) << "Enter clearChildrenFocus, objParent:" << objParent;
    // 可以获取焦点的控件名称列表
    QStringList foucswidgetlist;
    // foucswidgetlist << "QLineEdit" << TERM_WIDGET_NAME;

    // qCDebug(appLog) << "checkChildrenFocus start" << objParent->children().size();
    for (QObject *obj : objParent->children()) {
        if (!obj->isWidgetType()) {
            qCDebug(appLog) << "obj is not a widget";
            continue;
        }
        QWidget *widget = static_cast<QWidget *>(obj);
        if (Qt::NoFocus != widget->focusPolicy()) {
            qCDebug(appLog) << "widget->focusPolicy() != Qt::NoFocus";
            // qCDebug(appLog) << widget << widget->focusPolicy() << widget->metaObject()->className();
            if (!foucswidgetlist.contains(widget->metaObject()->className())) {
                qCDebug(appLog) << "widget->setFocusPolicy(Qt::NoFocus)";
                widget->setFocusPolicy(Qt::NoFocus);
            }
        }
        qCDebug(appLog) << "clearChildrenFocus(obj)";
        clearChildrenFocus(obj);
    }

    // qCDebug(appLog) << "checkChildrenFocus over" << objParent->children().size();
    qCDebug(appLog) << "Exit clearChildrenFocus";
}

void Utils::clearChildrenFoucusEx(QWidget *pWidget)
{
    qCDebug(appLog) << "Enter clearChildrenFoucusEx, pWidget:" << pWidget;
    pWidget->clearFocus();

    QObjectList childern = pWidget->children();

    if (childern.size() <= 0)
        return;

    foreach (QObject *child, childern) {
        if (!child->isWidgetType()) {
            qCDebug(appLog) << "child is not a widget";
            continue;
        }

        QWidget *obj = static_cast<QWidget *>(child);
        qCDebug(appLog) << "clearChildrenFoucusEx(obj)";
        clearChildrenFoucusEx(obj);
    }
    qCDebug(appLog) << "Exit clearChildrenFoucusEx";
}

void Utils::setChildrenFocus(QWidget *pWidget, Qt::FocusPolicy policy)
{
    qCDebug(appLog) << "Enter setChildrenFocus, pWidget:" << pWidget << "policy:" << policy;
    pWidget->setFocusPolicy(policy);

    QObjectList childern = pWidget->children();

    if (childern.size() <= 0)
        return;

    foreach (QObject *child, childern) {
        if (!child->isWidgetType()) {
            qCDebug(appLog) << "child is not a widget";
            continue;
        }

        QWidget *obj = static_cast<QWidget *>(child);
        qCDebug(appLog) << "setChildrenFocus(obj, policy)";
        setChildrenFocus(obj, policy);
    }
    qCDebug(appLog) << "Exit setChildrenFocus";
}

int Utils::getProcessCountByName(const char *pstrName)
{
    qCDebug(appLog) << "Enter getProcessCountByName, pstrName:" << pstrName;

    if (NULL == pstrName || strlen(pstrName) == 0) {
        qCDebug(appLog) << "pstrName is null";
        return -1;
    }

    QProcess process;
    process.start("ps", QStringList() << "-ef");
    if (!process.waitForFinished(5000)) {
        qCDebug(appLog) << "ps command timeout";
        return -1;
    }

    QString output = process.readAllStandardOutput();
    QString processName = QString::fromUtf8(pstrName);
    int count = 0;

    for (const QString &line : output.split('\n')) {
        if (line.contains(processName)) {
            count++;
        }
    }

    qCDebug(appLog) << "Exit getProcessCountByName, count:" << count;
    return count;
}

void Utils::killProcessByName(const char *pstrName)
{
    qCDebug(appLog) << "Enter killProcessByName, pstrName:" << pstrName;

    if (pstrName == NULL || strlen(pstrName) == 0) {
        qCDebug(appLog) << "pstrName is null or empty";
        return;
    }

    QProcess process;
    process.start("killall", QStringList() << QString::fromUtf8(pstrName));
    process.waitForFinished(5000);

    qCDebug(appLog) << "Exit killProcessByName";
}

QString Utils::getStringMD5Hash(const QString &input)
{
    qCDebug(appLog) << "Enter getStringMD5Hash, input:" << input;
    QByteArray byteArray = input.toUtf8();
    qCDebug(appLog) << "byteArray:" << byteArray;
    QByteArray md5Path = QCryptographicHash::hash(byteArray, QCryptographicHash::Md5);
    qCDebug(appLog) << "md5Path:" << md5Path;
    return md5Path.toHex();
}

bool Utils::activeWindowFromDock(quintptr winId)
{
    qCDebug(appLog) << "Enter activeWindowFromDock, winId:" << winId;
    bool bRet = false;
    // 优先采用V23接口
    QDBusInterface dockDbusInterfaceV23(
        "org.deepin.dde.daemon.Dock1", "/org/deepin/dde/daemon/Dock1", "org.deepin.dde.daemon.Dock1");
    if (dockDbusInterfaceV23.isValid()) {
        QDBusReply<void> reply = dockDbusInterfaceV23.call("ActivateWindow", winId);
        if (!reply.isValid()) {
            qCDebug(appLog) << "call v23 org.deepin.dde.daemon.Dock1 failed" << reply.error();
        } else {
            qCDebug(appLog) << "call v23 org.deepin.dde.daemon.Dock1 success";
            return true;
        }
    }

    QDBusInterface dockDbusInterfaceV20(
        "com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", "com.deepin.dde.daemon.Dock");
    if (dockDbusInterfaceV20.isValid() && !bRet) {
        QDBusReply<void> reply = dockDbusInterfaceV20.call("ActivateWindow", winId);
        if (!reply.isValid()) {
            qCDebug(appLog) << "call v20 com.deepin.dde.daemon.Dock failed" << reply.error();
            bRet = false;
        } else {
            qCDebug(appLog) << "call v20 com.deepin.dde.daemon.Dock success";
            return true;
        }
    }

    qCDebug(appLog) << "Exit activeWindowFromDock, bRet:" << bRet;
    return bRet;
}

bool Utils::isShareDirAndReadOnly(const QString &filePath)
{
    qCDebug(appLog) << "Enter isShareDirAndReadOnly, filePath:" << filePath;
    bool ret = false;

    const QString sharePath = "/var/lib/samba/usershares";
    QDir shareDir(sharePath);
    if (shareDir.exists()) {
        qCDebug(appLog) << "shareDir exists";
        QFileInfo fileInfo(filePath);
        auto name = fileInfo.dir().dirName();
        if (shareDir.exists(name)) {
            qCDebug(appLog) << "shareDir.exists(name)";
            QFile file(sharePath + "/" + name);
            if (file.open(QIODevice::ReadOnly)) {
                QString fileContent = file.readAll();
                qCDebug(appLog) << "fileContent:" << fileContent;
                // 精确匹配权限字段 ":R"：后跟逗号、行尾或文件末尾，
                // 避免 contains(":R") 子串误命中 ":RW" 等可写配置
                if (fileContent.contains(QRegularExpression(":R(,|\\n|$)")))
                    ret = true;
                file.close();
            } else {
                qCDebug(appLog) << "file.open(QIODevice::ReadOnly) failed";
            }
        }
    }

    qCDebug(appLog) << "Exit isShareDirAndReadOnly, ret:" << ret;
    return ret;
}

QString Utils::getSystemLan()
{
    qCDebug(appLog) << "Enter getSystemLan";
    if (!m_systemLanguage.isEmpty()) {
        qCDebug(appLog) << "m_systemLanguage is not empty";
        return m_systemLanguage;
    } else {
        switch (getSystemVersion()) {
            case V23:
                m_systemLanguage = QLocale::system().name();
                qCDebug(appLog) << "m_systemLanguage:" << m_systemLanguage;
                break;
            default: {
                QDBusInterface ie("com.deepin.daemon.LangSelector",
                                  "/com/deepin/daemon/LangSelector",
                                  "com.deepin.daemon.LangSelector",
                                  QDBusConnection::sessionBus());
                m_systemLanguage = ie.property("CurrentLocale").toString();
                qCDebug(appLog) << "m_systemLanguage:" << m_systemLanguage;
                break;
            }
        }

        qCDebug(appLog) << "getSystemLan is" << m_systemLanguage;
        qCDebug(appLog) << "Exit getSystemLan";
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
    qCDebug(appLog) << "Enter getSystemVersion";
    QString version = DSysInfo::majorVersion();
    if (version.toInt() >= 23) {
        qCDebug(appLog) << "version.toInt() >= 23";
        return V23;
    }
    qCDebug(appLog) << "Exit getSystemVersion, version:" << version;
    // 其它版本默认V20
    return V20;
}

// judge whether the protocol is wayland
bool Utils::isWayland()
{
    qCDebug(appLog) << "Enter isWayland";
    static QString protocol;
    if (protocol.isEmpty()) {
        protocol = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
    }
    qCDebug(appLog) << "Exit isWayland, protocol:" << protocol;
    return protocol.contains("wayland");
}

QString Utils::lineFeed(const QString &text, int nWidth, const QFont &font, int nElidedRow)
{
    qCDebug(appLog) << "Enter lineFeed, text:" << text << "nWidth:" << nWidth << "font:" << font << "nElidedRow:" << nElidedRow;
    if (nElidedRow < 0) {
        qCDebug(appLog) << "nElidedRow < 0";
        nElidedRow = 2;
    }
    qCDebug(appLog) << "nElidedRow:" << nElidedRow;
    QString strText = text;
    QStringList strListLine;
    QFontMetrics fm(font);
    // 一行就直接中间截断显示
    if (1 == nElidedRow) {
        qCDebug(appLog) << "1 == nElidedRow";
        return fm.elidedText(text, Qt::ElideMiddle, nWidth);
    }
    qCDebug(appLog) << "strText:" << strText;
    if (!strText.isEmpty()) {
        for (int i = 1; i <= strText.size(); i++) {
            if (fm.horizontalAdvance(strText.left(i)) >= nWidth) {
                qCDebug(appLog) << "fm.horizontalAdvance(strText.left(i)) >= nWidth";
                if (strListLine.size() + 1 == nElidedRow) {
                    qCDebug(appLog) << "strListLine.size() + 1 == nElidedRow";
                    break;
                }
                qCDebug(appLog) << "strListLine.append(strText.left(i - 1))";
                strListLine.append(strText.left(i - 1));
                strText = strText.right(strText.size() - i + 1);
                qCDebug(appLog) << "strText:" << strText;
                i = 0;
            }
        }
    }
    qCDebug(appLog) << "strListLine:" << strListLine;
    // 多行时，对最后一行字符左侧省略
    if (!strListLine.isEmpty()) {
        strText = fm.elidedText(strText, Qt::ElideLeft, nWidth);
        strListLine.append(strText);
        strText = strListLine.join('\n');
    }
    qCDebug(appLog) << "Exit lineFeed, strText:" << strText;
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
    qCDebug(appLog) << "Enter checkRegionIntersect, x1:" << x1 << "y1:" << y1 << "x2:" << x2 << "y2:" << y2;
    if (y1 < x2) {
        qCDebug(appLog) << "y1 < x2";
        return ERight;
    } else if (x1 > y2) {
        qCDebug(appLog) << "x1 > y2";
        return ELeft;
    } else {
        // 区间存在交集，判断交集类型
        // 活动区间左边界超过固定区间左边界
        bool outLeftBound = x1 > x2;
        // 活动区间右边界超过固定区间右边界
        bool outRightBound = y1 < y2;

        if (outLeftBound && outRightBound) {
            qCDebug(appLog) << "outLeftBound && outRightBound";
            return EIntersectOutter;
        } else if (outLeftBound) {
            qCDebug(appLog) << "outLeftBound";
            return EIntersectLeft;
        } else if (outRightBound) {
            qCDebug(appLog) << "outRightBound";
            return EIntersectRight;
        } else {
            qCDebug(appLog) << "else";
            return EIntersectInner;
        }
    }
    qCDebug(appLog) << "Exit checkRegionIntersect";
}

/**
 * @return 取得当前文本编辑器支持的编码格式，按区域划分，从文件 :/encodes/encodes.ini 中读取
 * @note 非多线程安全，仅在 gui 线程调用
 */
QVector<QPair<QString, QStringList>> Utils::getSupportEncoding()
{
    qCDebug(appLog) << "Enter getSupportEncoding";
    static QVector<QPair<QString, QStringList>> s_groupEncodeVec;
    if (s_groupEncodeVec.isEmpty()) {
        qCDebug(appLog) << "s_groupEncodeVec is empty";
        QVector<QPair<QString, QStringList>> tmpEncodeVec;

        QFile file(":/encodes/encodes.ini");
        QString data;
        if (file.open(QIODevice::ReadOnly)) {
            qCDebug(appLog) << "file.open(QIODevice::ReadOnly) success";
            data = QString::fromUtf8(file.readAll());
            file.close();
        } else {
            qCDebug(appLog) << "file.open(QIODevice::ReadOnly) failed";
        }
        qCDebug(appLog) << "data:" << data;
        QTextStream readStream(&data, QIODevice::ReadOnly);
        while (!readStream.atEnd()) {
            qCDebug(appLog) << "readStream.readLine()";
            QString group = readStream.readLine();
            qCDebug(appLog) << "group:" << group;
            if (!group.startsWith('[') || !group.endsWith(']')) {
                qCWarning(appLog) << "unexpected group line, skipped:" << group;
                continue;
            }
            QString key = group.mid(1, group.length() - 2).trimmed();
            QString encodes = readStream.readLine();
            qCDebug(appLog) << "encodes:" << encodes;
            int eqIdx = encodes.indexOf('=');
            if (eqIdx == -1) {
                qCWarning(appLog) << "unexpected encodes line, skipped:" << encodes;
                continue;
            }
            QString value = encodes.mid(eqIdx + 1).trimmed();
            QStringList encodeNames;
            const QStringList parts = value.split(',', Qt::SkipEmptyParts);
            for (const QString &part : parts) {
                const QString name = part.trimmed();
                if (!name.isEmpty()) {
                    encodeNames.append(name);
                }
            }
            qCDebug(appLog) << "value:" << value;
            tmpEncodeVec.append(QPair<QString, QStringList>(key, encodeNames));
        }
        qCDebug(appLog) << "tmpEncodeVec:" << tmpEncodeVec;
        s_groupEncodeVec = tmpEncodeVec;
    }
    qCDebug(appLog) << "Exit getSupportEncoding, s_groupEncodeVec:" << s_groupEncodeVec;
    return s_groupEncodeVec;
}

/**
 * @return 取得当前文本编辑器支持的编码格式列表
 */
QStringList Utils::getSupportEncodingList()
{
    qCDebug(appLog) << "Enter getSupportEncodingList";
    QStringList encodingList;
    auto supportEncoding = getSupportEncoding();
    qCDebug(appLog) << "supportEncoding:" << supportEncoding;
    for (auto encodingData : supportEncoding) {
        encodingList.append(encodingData.second);
    }
    std::sort(encodingList.begin(), encodingList.end());
    qCDebug(appLog) << "Exit getSupportEncodingList, encodingList:" << encodingList;
    return encodingList;
}

QString Utils::libPath(const QString &strlib)
{
    qCDebug(appLog) << "Enter libPath, strlib:" << strlib;
    QDir dir;
    QString path = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    dir.setPath(path);
    QStringList list =
        dir.entryList(QStringList() << (strlib + "*"), QDir::NoDotAndDotDot | QDir::Files);  // filter name with strlib
    qCDebug(appLog) << "list:" << list;
    if (list.contains(strlib)) {
        qCDebug(appLog) << "list contains strlib";
        return strlib;
    }
    qCDebug(appLog) << "list does not contain strlib";
    list.sort();
    qCDebug(appLog) << "list sorted:" << list;
    if (list.size() <= 0) {
        qCDebug(appLog) << "list size <= 0";
        return "";
    }
    qCDebug(appLog) << "list size > 0";
    return list.last();
}

void Utils::loadCustomDLL()
{
    qCDebug(appLog) << "Enter loadCustomDLL";
    // 解析ZPD定制需求提供的库libzpdcallback.so
    LoadLibNames tmp;
    QByteArray zpdDll = Utils::libPath("libzpdcallback.so").toLatin1();
    qCDebug(appLog) << "zpdDll:" << zpdDll;
    if (QFile::exists(zpdDll)) {
        qCDebug(appLog) << "QFile::exists(zpdDll)";
        tmp.chZPDDLL = zpdDll.data();
    } else {
        qCDebug(appLog) << "QFile::exists(zpdDll) failed";
        tmp.chZPDDLL = NULL;
    }
    setLibNames(tmp);
    qCDebug(appLog) << "Exit loadCustomDLL";
}

/**
 * @brief 根据传入的文件路径 \a filePath 返回是否允许对此文件内容进行剪切或拷贝
 * @param filePath 文件完整路径
 * @return 是否允许剪切或拷贝
 */
bool Utils::enableClipCopy(const QString &filePath)
{
    qCDebug(appLog) << "Enter enableClipCopy, filePath:" << filePath;
#if _ZPD_
    if (getLoadZPDLibsInstance()->m_document_clip_copy) {
        // intercept 输出为1,拦截操作
        static const int disableFlag = 1;
        int intercept = 1;
        getLoadZPDLibsInstance()->m_document_clip_copy(filePath.toUtf8().data(), &intercept);
        qCDebug(appLog) << "intercept:" << intercept;
        if (disableFlag == intercept) {
            qCWarning(appLog) << qPrintable("ZPD access control! Disable clip or copy document!");
            return false;
        }
    }
#endif
    qCDebug(appLog) << "Exit enableClipCopy, return true";
    return true;
}

/**
 * @brief 文件关闭前记录当前关闭的文件路径 \a filePath
 * @param filePath 文件完整路径
 */
void Utils::recordCloseFile(const QString &filePath)
{
    qCDebug(appLog) << "Enter recordCloseFile, filePath:" << filePath;
#if _ZPD_
    if (getLoadZPDLibsInstance()->m_document_close) {
        qCDebug(appLog) << "getLoadZPDLibsInstance()->m_document_close";
        getLoadZPDLibsInstance()->m_document_close(filePath.toUtf8().data());
    } else {
        qCDebug(appLog) << "getLoadZPDLibsInstance()->m_document_close is null";
    }
#endif
    qCDebug(appLog) << "Exit recordCloseFile";
}

/**
   @brief 发送弹窗提示消息 \a message , 使用 \a icon 设置提示图标，\a par 是浮动窗口计算位置的父窗口。
        这个函数效果和 DMessageManager::sendMessage() 类似，但是提示信息的文字字体将跟随 qApp 变化,
        而不是直接使用父窗口的字体。
 */
void Utils::sendFloatMessageFixedFont(QWidget *par, const QIcon &icon, const QString &message)
{
    qCDebug(appLog) << "Enter sendFloatMessageFixedFont, par:" << par << "icon:" << icon << "message:" << message;
    // 以下代码和 DMessageManager::sendMessage() 流程一致。
    if (QWidget *content = par->findChild<QWidget *>("_d_message_manager_content", Qt::FindDirectChildrenOnly)) {
        auto msgWidgets = content->findChildren<DFloatingMessage *>(QString(), Qt::FindDirectChildrenOnly);
        auto text_message_count = std::count_if(msgWidgets.begin(), msgWidgets.end(), [](DFloatingMessage *msg) {
            return bool(msg->messageType() == DFloatingMessage::TransientType);
        });

        // TransientType 类型的通知消息，最多只允许同时显示三个
        if (text_message_count >= 3) {
            qCDebug(appLog) << "text_message_count >= 3";
            return;
        }
    }

    // 浮动临时提示信息，自动销毁
    DFloatingMessage *floMsg = new DFloatingMessage(DFloatingMessage::TransientType);
    floMsg->setAttribute(Qt::WA_DeleteOnClose);
    floMsg->setIcon(icon);
    floMsg->setMessage(message);
    floMsg->setFont(qApp->font());
    qCDebug(appLog) << "floMsg:" << floMsg;
#ifdef DTKWIDGET_CLASS_DSizeMode
    // 绑定 qApp 字体变更信号
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, floMsg, [=](const QFont &font) {
        floMsg->setFont(font);
    });
#endif
    qCDebug(appLog) << "DMessageManager::instance()->sendMessage(par, floMsg)";
    DMessageManager::instance()->sendMessage(par, floMsg);
    qCDebug(appLog) << "Exit sendFloatMessageFixedFont";
}

/**
 * @brief Gets system memory information from /proc/meminfo
 * @param[out] totalMemory Total system memory in KB
 * @param[out] freeMemory Available memory in KB (MemAvailable if exists, otherwise MemFree+Buffers+Cached)
 * @return True if memory info was successfully read and parsed, false otherwise
 */
bool Utils::getSystemMemoryInfo(qlonglong &totalMemory, qlonglong &freeMemory)
{
    qCDebug(appLog) << "Enter getSystemMemoryInfo";
    qlonglong memFree = 0;
    qlonglong buffers = 0;
    qlonglong cached = 0;
    bool valueOk = false;

    QFile file(PROC_MEMINFO_PATH);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(appLog) << "Utils: Failed to open" << PROC_MEMINFO_PATH << "for memory check.";
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
            qCDebug(appLog) << "totalMemory:" << totalMemory;
        } else if (line.startsWith("MemFree:")) {
            memFree = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qCDebug(appLog) << "memFree:" << memFree;
        } else if (line.startsWith("Buffers:")) {
            buffers = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qCDebug(appLog) << "buffers:" << buffers;
        } else if (line.startsWith("Cached:")) {
            cached = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qCDebug(appLog) << "cached:" << cached;
        } else if (line.startsWith("MemAvailable:")) {
            freeMemory = line.section(':', 1, 1).trimmed().section(' ', 0, 0).toLongLong(&valueOk);
            qCDebug(appLog) << "freeMemory:" << freeMemory;
            break;
        }
        if (stream.atEnd() && freeMemory == 0) {
            freeMemory = memFree + buffers + cached;
            qCDebug(appLog) << "freeMemory:" << freeMemory;
        }
    }
    qCDebug(appLog) << "Exit getSystemMemoryInfo, totalMemory:" << totalMemory << "freeMemory:" << freeMemory;
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
    qCDebug(appLog) << "Enter isMemorySufficientForOperation, operationType:" << operationType << "operationDataSize:" << operationDataSize << "currentDocumentSize:" << currentDocumentSize;

    qlonglong memoryFree = 0;
    qlonglong memoryTotal = 0;

    if (!getSystemMemoryInfo(memoryTotal, memoryFree)) {
        // Conservatively allow the operation if memory info cannot be read
        qCWarning(appLog) << "Failed to get system memory info, conservatively allowing operation";
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
                qCWarning(appLog) << "Utils: Insufficient memory for raw operation. Needed:" << operationDataSize << "Available:" << availableMemoryBytes;
                return false;
            }
            break;

        case OperationType::CopyOperation: {
            // Copy operation: Estimated memory consumption = data size * factor
            qlonglong estimatedMemoryNeeded = operationDataSize * COPY_CONSUME_MEMORY_MULTIPLE;
            if (estimatedMemoryNeeded > availableMemoryBytes) {
                qCWarning(appLog) << "Utils: Insufficient memory for copy operation. Needed(est):" << estimatedMemoryNeeded << "Available:" << availableMemoryBytes;
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
                qCWarning(appLog) << "Utils: Insufficient memory for paste operation (paste data). Needed(est):" << estimatedMemoryNeededForPaste << "Available:" << availableMemoryBytes;
                return false;
            }

            // Check if the estimated total document size after paste exceeds total system memory
            if (estimatedTotalDocMemory > totalMemoryBytes) {
                qCWarning(appLog) << "Utils: Paste operation might exceed total system memory. Estimated total doc memory:" << estimatedTotalDocMemory << "Total system memory:" << totalMemoryBytes;
                return false;
            }

            // Check specific threshold: Restrict paste size if document reaches 800MB
            const qlonglong DOC_SIZE_LIMIT_800MB = 800LL * DATA_SIZE_1024 * DATA_SIZE_1024;
            const qlonglong PASTE_SIZE_LIMIT_500KB = 500LL * DATA_SIZE_1024;
            if (currentDocumentSize > DOC_SIZE_LIMIT_800MB && operationDataSize > PASTE_SIZE_LIMIT_500KB) {
                qCWarning(appLog) << "Utils: Paste operation restricted. Document size exceeds 800MB and paste data exceeds 500KB.";
                return false;
            }
            break;
        }

        default:
            qCDebug(appLog) << "default";
            break;
    }

    qCDebug(appLog) << "Exit isMemorySufficientForOperation, return true";
    return true; // Memory is sufficient
}
