// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detectcode.h"
#include "../common/config.h"

#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QDateTime>

#include <stdio.h>

QMap<QString, QByteArray> DetectCode::sm_LangsMap;
// 最低的检测准确度判断，低于90%需要调整策略
static const float gs_dMinConfidence = 0.9f;

DetectCode::DetectCode() {}

/**
 * @brief 根据文件头内容 \a content 取得文件 \a filepath 字符编码格式
 * @param filepath  待获取字符编码文件
 * @param content   文件头内容
 * @return 文件字符编码格式
 *
 * @note 对于大文本文件，文件头内容 \a content 可能在文件中间截断，\a content 尾部带有被截断的字符，
 *      极大的降低字符编码识别率。为此，在识别率过低时裁剪尾部数据，重新检测以提高文本识别率。
 */
QByteArray DetectCode::GetFileEncodingFormat(QString filepath, QByteArray content)
{
    QString charDetectedResult;
    QByteArray ucharDetectdRet;
    QByteArrayList icuDetectRetList;
    QByteArray detectRet;
    float chardetconfidence = 0.0f;

    /* chardet识别编码 */
    QString str(content);
    // 匹配的是中文(仅在UTF-8编码下)
    bool bFlag = str.contains(QRegExp("[\\x4e00-\\x9fa5]+"));
    if (bFlag) {
        const QByteArray suffix = "为增加探测率保留的中文";
        QByteArray newContent = content;
        //手动添加中文字符，避免字符长度太短而导致判断编码错误
        newContent += suffix;
        DetectCode::ChartDet_DetectingTextCoding(newContent, charDetectedResult, chardetconfidence);

        // 大文本数据存在从文档中间截断的可能，导致unicode中文字符被截断，解析为乱码，处理部分情况
        // 根据文本中断的情况，每次尝试解析编码后移除尾部字符，直到识别率达到90%以上
        int tryCount = 5;
        while (chardetconfidence < gs_dMinConfidence && newContent.size() > suffix.size() && tryCount-- > 0) {
            // 移除可能的乱码尾部字符
            newContent.remove(newContent.size() - suffix.size(), 1);
            DetectCode::ChartDet_DetectingTextCoding(newContent, charDetectedResult, chardetconfidence);
        }
    } else {
        DetectCode::ChartDet_DetectingTextCoding(content, charDetectedResult, chardetconfidence);

        // 部分非unicode编码同为中文，例如 GB18030, BIG5 等中文编码，同样判断识别率，识别率较低手动干预多次检测
        int tryCount = 5;
        QByteArray newContent = content;
        while (chardetconfidence < gs_dMinConfidence && !newContent.isEmpty() && tryCount-- > 0) {
            newContent.chop(1);
            DetectCode::ChartDet_DetectingTextCoding(newContent, charDetectedResult, chardetconfidence);
        }
    }
    ucharDetectdRet = charDetectedResult.toLatin1();

    // uchardet识别编码 若识别率过低, 考虑是否非单字节编码格式。
    if (ucharDetectdRet.contains("unknown") || ucharDetectdRet.contains("ASCII") || ucharDetectdRet.contains("???") ||
        ucharDetectdRet.isEmpty() || chardetconfidence < gs_dMinConfidence) {
        ucharDetectdRet = DetectCode::UchardetCode(filepath);
    }

    // icu识别编码
    icuDetectTextEncoding(filepath, icuDetectRetList);
    detectRet = selectCoding(ucharDetectdRet, icuDetectRetList, chardetconfidence);

    if (detectRet.contains("ASCII") || detectRet.isEmpty()) {
        detectRet = "UTF-8";
    }

    return detectRet.toUpper();
}

QByteArray DetectCode::UchardetCode(QString filepath)
{
    FILE *fp;
    QByteArray charset;

    size_t buffer_size = 0x10000;
    char *buff = new char[buffer_size];
    memset(buff, 0, buffer_size);

    /* 通过样本字符分析文本编码 */
    uchardet_t handle = uchardet_new();

    /* 打开被检测文本文件，并读取一定数量的样本字符 */
    fp = fopen(filepath.toLocal8Bit().data(), "rb");

    if (fp) {
        while (!feof(fp)) {
            size_t len = fread(buff, 1, buffer_size, fp);
            int retval = uchardet_handle_data(handle, buff, len);
            if (retval != 0) {
                continue;
            }

            break;
        }
        fclose(fp);

        uchardet_data_end(handle);
        charset = uchardet_get_charset(handle);
    }

    uchardet_delete(handle);
    delete[] buff;
    buff = nullptr;

    if (charset == "MAC-CENTRALEUROPE")
        charset = "MACCENTRALEUROPE";
    if (charset == "MAC-CYRILLIC")
        charset = "MACCYRILLIC";
    if (charset.contains("WINDOWS-"))
        charset = charset.replace("WINDOWS-", "CP");
    return charset;
}

/**
 * @author guoshao
 * @brief  icuDetectTextEncoding() icu库编码识别
 * @param  filePath:文件路径，listDetectRet:编码识别结果存在的变量
 **/
void DetectCode::icuDetectTextEncoding(const QString &filePath, QByteArrayList &listDetectRet)
{
    FILE *file;
    file = fopen(filePath.toLocal8Bit().data(), "rb");
    if (file == nullptr) {
        qInfo() << "fopen file failed.";
        return;
    }

    size_t iBuffSize = 4096;
    char *detected = nullptr;
    char *buffer = new char[iBuffSize];
    memset(buffer, 0, iBuffSize);

    int readed = 0;
    while (!feof(file)) {
        size_t len = fread(buffer, 1, iBuffSize, file);
        readed += len;
        if (readed > 1 * 1024 * 1024) {
            break;
        }

        if (detectTextEncoding(buffer, len, &detected, listDetectRet)) {
            break;
        }
    }

    delete[] buffer;
    buffer = nullptr;
    fclose(file);
}

/**
 * @author guoshao
 * @brief  detectTextEncoding() icu库编码识别内层函数
 * @param  data:要识别的内容，len:要识别的内容的长度，detected:编码识别结果存在的变量，
 *         listDetectRet:编码识别结果存在的list
 * @return true:识别成功，false:识别失败
 **/
bool DetectCode::detectTextEncoding(const char *data, size_t len, char **detected, QByteArrayList &listDetectRet)
{
    Q_UNUSED(detected);

    UCharsetDetector *csd;
    const UCharsetMatch **csm;
    int32_t matchCount = 0;

    UErrorCode status = U_ZERO_ERROR;
    csd = ucsdet_open(&status);
    if (status != U_ZERO_ERROR) {
        return false;
    }

    ucsdet_setText(csd, data, len, &status);
    if (status != U_ZERO_ERROR) {
        return false;
    }

    csm = ucsdet_detectAll(csd, &matchCount, &status);
    if (status != U_ZERO_ERROR) {
        return false;
    }

    int readMax = 0;
    // 提高GB18030识别率时，拓展允许读取的编码列表
    if (Config::instance()->enableImproveGB18030()) {
        readMax = qMin(6, matchCount);
    } else {
        readMax = qMin(3, matchCount);
    }

    for (int i = 0; i < readMax; i++) {
        auto str = ucsdet_getName(csm[i], &status);
        if (status != U_ZERO_ERROR) {
            return false;
        }
        listDetectRet << QByteArray(str);
    }

    ucsdet_close(csd);
    return true;
}

/**
 * @author guoshao
 * @brief  selectCoding() 筛选识别出来的编码
 * @param  ucharDetectdRet:chardet/uchardet识别的编码结果，icuDetectRetList:编码识别结果存在的list
 * @return 筛选编码的结果
 **/
QByteArray DetectCode::selectCoding(QByteArray ucharDetectdRet, QByteArrayList icuDetectRetList, float confidence)
{
    // 列表不允许为空
    if (icuDetectRetList.isEmpty()) {
        return QByteArray();
    }

    if (!ucharDetectdRet.isEmpty()) {
        // 获取是否允许提高GB18030编码的策略
        if (Config::instance()->enableImproveGB18030()) {
            // 中文环境优先匹配GB18030编码
            if (QLocale::Chinese == QLocale::system().language()) {
                if (confidence < gs_dMinConfidence && icuDetectRetList.contains("GB18030")) {
                    return QByteArray("GB18030");
                }

                if (QByteArray("ASCII") == ucharDetectdRet) {
                    return QByteArray("GB18030");
                }
            }
        }

        if (ucharDetectdRet.contains(icuDetectRetList[0])) {
            return ucharDetectdRet;
        } else {
            if (icuDetectRetList.contains("GB18030")) {
                return QByteArray("GB18030");
            } else {
                // 筛选部分带后缀的编码格式，例如 UTF-16 BE 和 UTF-16
                if (icuDetectRetList[0].contains(ucharDetectdRet)) {
                    return icuDetectRetList[0];
                }

                return ucharDetectdRet;
            }
        }
    }

    if (ucharDetectdRet.isEmpty()) {
        if (icuDetectRetList.contains("GB18030")) {
            return QByteArray("GB18030");
        } else {
            return icuDetectRetList[0];
        }
    }

    return QByteArray();
}

#if 0 /* 因为开源协议存在法律冲突，停止使用libenca0编码识别库 */
QByteArray DetectCode::EncaDetectCode(QString filepath)
{
    /*
     * 编码区域对应的简称
     *"zh"中文 "be"俄罗斯 "bg"保加利亚 "cs"捷克文 "et"爱沙尼亚语 "hr"克罗地亚人[语]; "hu"匈牙利语 "lt"立陶宛
     * "lv"拉脱维亚语 "pl"波兰语 "ru"俄语 "sk"斯洛伐克人（语）  "sl"斯洛文尼亚人（语）  "uk"乌克兰人（语）
     */

    sm_LangsMap.clear();
    const char *langArray[] = {"zh", "be", "bg", "cs", "et", "hr", "hu", "lt", "lv", "pl", "ru", "sk", "sl", "uk"};
#if 0 /* EncaAnalyserState */
    size_t size;
    const char **lang = enca_get_languages(&size);
    QStringList langs;
    for (size_t i = 0; i < size; i++) {
        langs << lang[i];
    }
#endif

    QByteArray charset;

    for (size_t i = 0; i < sizeof(langArray) / sizeof(const char *); i++) {

        EncaAnalyser analyser = nullptr;
        analyser = enca_analyser_alloc(langArray[i]);
        enca_set_threshold(analyser, 1.38);
        enca_set_multibyte(analyser, 1);
        enca_set_ambiguity(analyser, 1);
        enca_set_garbage_test(analyser, 1);

        size_t buffer_size = 0x10000;

        unsigned char *buff = new unsigned char[buffer_size];
        memset(buff, 0, buffer_size);

        /* 打开被检测文本文件，并读取一定数量的样本字符 */
        FILE *fp; /* the processed file */
        fp = fopen(filepath.toLocal8Bit().data(), "rb");

        if (fp) {
            while (!feof(fp)) {
                size_t len = fread(buff, 1, buffer_size, fp);
                EncaEncoding encoding = enca_analyse(analyser, buff, len);
                charset = enca_charset_name(encoding.charset, EncaNameStyle::ENCA_NAME_STYLE_MIME);
                qDebug() << QStringLiteral("ENCA文本的编码方式是:") << charset;
                //识别文本编码识别
                if (encoding.charset == -1) continue;
                break;
            }

            enca_analyser_free(analyser);
            analyser = nullptr;

            delete [] buff;
            buff = nullptr;

            fclose(fp);
        } else {
            qDebug() << QStringLiteral("ENCA打开失败:") << filepath;
        }

        if (charset == "US-ASCII") charset = "ASCII";
        if (charset == "GB2312" || charset == "GBK") charset = "GB18030";
        if (charset == "ISO-10646-UCS-2") charset = "UTF-16BE";
        sm_LangsMap[langArray[i]] = charset;

        if (!charset.isEmpty()) {
            break;
        }
    }

    return charset;
}
#endif

/**
 * @brief ChartDet_DetectingTextCoding libchardet1编码识别库识别编码
 */
int DetectCode::ChartDet_DetectingTextCoding(const char *str, QString &encoding, float &confidence)
{
    DetectObj *obj = detect_obj_init();

    if (obj == nullptr) {
        // qInfo() << "Memory Allocation failed\n";
        return CHARDET_MEM_ALLOCATED_FAIL;
    }

    /* 另一种编码识别逻辑，暂且保留*/
    /*size_t buffer_size = 1024;
    char *buff = new char[buffer_size];
    memset(buff, 0, buffer_size);

    FILE *fp;
    fp = fopen(filepath.toLocal8Bit().data(), "rb");
    if (fp) {
        while (!feof(fp)) {
            size_t len = fread(buff, 1, buffer_size, fp);
            detect(buff, &obj);
            QString encoding = obj->encoding;
            qInfo() << "==== encoding: " << encoding;
            if (encoding == "") {
                continue;
            } else if (!encoding.isEmpty()) {
                break;
            }
            fclose(fp);
        }
    }

    delete [] buff;
    buff = nullptr;*/

#ifndef CHARDET_BINARY_SAFE
    // before 1.0.5. This API is deprecated on 1.0.5
    switch (detect(str, &obj))
#else
    // from 1.0.5
    switch (detect_r(str, strlen(str), &obj))
#endif
    {
        case CHARDET_OUT_OF_MEMORY:
            qInfo() << "On handle processing, occured out of memory\n";
            detect_obj_free(&obj);
            return CHARDET_OUT_OF_MEMORY;
        case CHARDET_NULL_OBJECT:
            qInfo() << "2st argument of chardet() is must memory allocation with detect_obj_init API\n";
            return CHARDET_NULL_OBJECT;
    }

#ifndef CHARDET_BOM_CHECK
        // qInfo() << "encoding:" << obj->encoding << "; confidence: " << obj->confidence;
#else
    // from 1.0.6 support return whether exists BOM
    qInfo() << "encoding:" << obj->encoding << "; confidence: " << obj->confidence << "; bom: " << obj->bom;
#endif

    encoding = obj->encoding;
    confidence = obj->confidence;
    detect_obj_free(&obj);

    return CHARDET_SUCCESS;
}

/**
 * @return 根据 UTF-8 字符编码，返回传入的字符串 \a buf 头部字符可能占用的字节数量
 * @note 不同编码UTF-8字节划分示例，除首个字节为0外
 *  如果一个字节的第一位是0，则这个字节单独就是一个字符；如果第一位是1，则连续有多少个1，就表示当前字符占用多少个字节。
 *  0000 0000-0000 007F | 0xxxxxxx
 *  0000 0080-0000 07FF | 110xxxxx 10xxxxxx
 *  0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
 *  0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 * @link https://zh.wikipedia.org/wiki/UTF-8
 */
int utf8MultiByteCount(char *buf, size_t size)
{
    // UTF 字符类型，单字符，中间字符，双字节，三字节，四字节
    enum UtfCharType {
        Single,
        Mid,
        DoubleBytes,
        ThreeBytes,
        FourBytes,
    };

    // 取得 UTF-8 字节序数值
    auto LeftBitFunc = [](char data) -> int {
        // 返回前导1的个数
        int res = 0;
        while (data & 0x80) {
            res++;
            data <<= 1;
        }
        return res;
    };

    int count = 0;
    while (size > 0 && count < FourBytes) {
        int leftBits = LeftBitFunc(*buf);
        switch (leftBits) {
            case Mid:
                count++;
                break;
            case DoubleBytes:
            case ThreeBytes:
            case FourBytes:
                return leftBits;
            default:
                // 超过4字节或单字节，均返回1
                return 1;
        }

        buf++;
        size--;
    }

    return count;
}

/**
 * @brief 将输入的字符序列 \a inputStr 从编码 \a fromCode 转换为编码 \a toCode, 并返回转换后的字符序列。
 * @return 字符编码转换是否成功
 */
bool DetectCode::ChangeFileEncodingFormat(QByteArray &inputStr,
                                          QByteArray &outStr,
                                          const QString &fromCode,
                                          const QString &toCode)
{
    if (fromCode == toCode) {
        outStr = inputStr;
        return true;
    }

    if (inputStr.isEmpty()) {
        outStr.clear();
        return true;
    }

    iconv_t handle = iconv_open(toCode.toLocal8Bit().data(), fromCode.toLocal8Bit().data());
    if (handle != reinterpret_cast<iconv_t>(-1)) {
        char *inbuf = inputStr.data();
        size_t inbytesleft = static_cast<size_t>(inputStr.size());
        size_t outbytesleft = 4 * inbytesleft;
        char *outbuf = new char[outbytesleft];
        char *bufferHeader = outbuf;
        size_t maxBufferSize = outbytesleft;

        memset(outbuf, 0, outbytesleft);

        int errorNum = 0;
        try {
            size_t ret = 0;
            do {
                ret = iconv(handle, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
                if (static_cast<size_t>(-1) == ret) {
                    // 记录错误信息
                    errorNum = errno;

                    // 遇到错误的输入，错误码 EILSEQ (84)，跳过当前位置并添加'?'
                    if (EILSEQ == errorNum) {
                        // 缓冲区不足跳出
                        if (outbytesleft == 0) {
                            break;
                        }

                        // 源编码为 UTF-8 时，可计算需跳过的字符数
                        size_t bytes = 1;
                        if (fromCode.toUpper() == "UTF-8") {
                            bytes = static_cast<size_t>(utf8MultiByteCount(inbuf, inbytesleft));
                        }
                        // 跳过错误字符，设置错误字符为'?'
                        outbytesleft--;
                        *outbuf = '?';
                        outbuf++;

                        // 待转换字节序结束，跳出
                        if (inbytesleft <= bytes) {
                            inbuf += inbytesleft;
                            inbytesleft = 0;
                            break;
                        }
                        inbuf += bytes;
                        inbytesleft -= bytes;

                        continue;
                    } else {
                        break;
                    }
                }
            } while (static_cast<size_t>(-1) == ret);

        } catch (const std::exception &e) {
            qWarning() << Q_FUNC_INFO << qPrintable("iconv convert encoding catching exception") << qPrintable(e.what());
        }

        if (errorNum) {
            qWarning() << Q_FUNC_INFO << qPrintable("iconv() convert text encoding error, errocode:") << errorNum;
        }
        iconv_close(handle);

        // 手动添加 UTF BOM 信息
        static QMap<QString, QByteArray> byteOrderMark = {{"UTF-16LE", QByteArray::fromHex("FFFE")},
                                                          {"UTF-16BE", QByteArray::fromHex("FEFF")},
                                                          {"UTF-32LE", QByteArray::fromHex("FFFE0000")},
                                                          {"UTF-32BE", QByteArray::fromHex("0000FEFF")}};
        outStr.append(byteOrderMark.value(toCode));

        // 计算 iconv() 实际转换的字节数
        size_t realConvertSize = maxBufferSize - outbytesleft;
        outStr += QByteArray(bufferHeader, static_cast<int>(realConvertSize));

        delete[] bufferHeader;
        bufferHeader = nullptr;

        return true;

    } else {
        qWarning() << Q_FUNC_INFO << qPrintable("Text encoding convert error, iconv_open() failed.");
        return false;
    }
}
