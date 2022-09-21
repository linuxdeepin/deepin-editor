// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detectcode.h"
#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <stdio.h>

QMap<QString, QByteArray> DetectCode::sm_LangsMap;

DetectCode::DetectCode()
{

}

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

    // 最低的检测准确度判断，低于90%需要调整策略
    static const float s_dMinConfidence = 0.9f;

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
        while (chardetconfidence < s_dMinConfidence
               && newContent.size() > suffix.size()
               && tryCount-- > 0) {
            // 移除可能的乱码尾部字符
            newContent.remove(newContent.size() - suffix.size(), 1);
            DetectCode::ChartDet_DetectingTextCoding(newContent, charDetectedResult, chardetconfidence);
        }
    } else {
        DetectCode::ChartDet_DetectingTextCoding(content, charDetectedResult, chardetconfidence);

        // 部分非unicode编码同为中文，例如 GB18030, BIG5 等中文编码，同样判断识别率，识别率较低手动干预多次检测
        int tryCount = 5;
        QByteArray newContent = content;
        while (chardetconfidence < s_dMinConfidence
               && !newContent.isEmpty()
               && tryCount-- > 0) {
            newContent.chop(1);
            DetectCode::ChartDet_DetectingTextCoding(newContent, charDetectedResult, chardetconfidence);
        }
    }
    ucharDetectdRet = charDetectedResult.toLatin1();

    /* uchardet识别编码 */
    if (ucharDetectdRet.contains("unknown") || ucharDetectdRet.contains("???") || ucharDetectdRet.isEmpty()) {
        ucharDetectdRet = DetectCode::UchardetCode(filepath);
    }

    /* icu识别编码 */
    icuDetectTextEncoding(filepath, icuDetectRetList);
    detectRet = selectCoding(ucharDetectdRet, icuDetectRetList);

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
        uchardet_delete(handle);
    }

    delete [] buff;
    buff = nullptr;

    if (charset == "MAC-CENTRALEUROPE") charset = "MACCENTRALEUROPE";
    if (charset == "MAC-CYRILLIC") charset = "MACCYRILLIC";
    if (charset.contains("WINDOWS-")) charset = charset.replace("WINDOWS-", "CP");
    return charset;
}

void DetectCode::icuDetectTextEncoding(const QString &filePath, QByteArrayList &listDetectRet)
{
    FILE *file;
    file = fopen(filePath.toLocal8Bit().data(), "rb");
    if (file == nullptr) {
        qInfo() << "fopen file failed.";
        return;
    }

    int len = 0;
    int iBuffSize = 4096;
    char *detected = nullptr;
    char *buffer = new char[iBuffSize];
    memset(buffer, 0, iBuffSize);

    int readed = 0;
    while (!feof(file)) {
        int32_t len = fread(buffer, 1, iBuffSize, file);
        readed += len;
        if (readed > 1 * 1024 * 1024) {
            break;
        }
        if (!detectTextEncoding(buffer, len, &detected, listDetectRet)) {
            break;
        }
    }

    delete [] buffer;
    buffer = nullptr;
    fclose(file);
}

bool DetectCode::detectTextEncoding(const char *data, size_t len, char **detected, QByteArrayList &listDetectRet)
{
    UCharsetDetector *csd;
    const UCharsetMatch **csm;
    int32_t match, matchCount = 0;

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
    //csm = ucsdet_detectAll_63(csd, &matchCount, &status);
    if (status != U_ZERO_ERROR) {
        return false;
    }

    for (match = 0; match < matchCount; match++) {
        const char *name = ucsdet_getName(csm[match], &status);
        int32_t confidence = ucsdet_getConfidence(csm[match], &status);
    }

    if (matchCount >= 3) {
        for (int i = 0; i < 3; i++) {
            auto str = ucsdet_getName(csm[i], &status);
            if (status != U_ZERO_ERROR) {
                return false;
            }
            listDetectRet << QByteArray(str);
        }


    } else if (matchCount > 0) {
        auto str = ucsdet_getName(csm[0], &status);
        listDetectRet << QByteArray(str);
    }


    ucsdet_close(csd);

    return true;
}

QByteArray DetectCode::selectCoding(QByteArray ucharDetectdRet, QByteArrayList icuDetectRetList)
{
    // 列表不允许为空
    if (icuDetectRetList.isEmpty()) {
        return QByteArray();
    }

    if (!ucharDetectdRet.isEmpty()) {
        if (ucharDetectdRet.contains(icuDetectRetList[0])) {
            return ucharDetectdRet;
        }

        if (!ucharDetectdRet.contains(icuDetectRetList[0])) {
            if (icuDetectRetList.contains("GB18030")) {
                return QByteArray("GB18030");
            } else {
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

int DetectCode::ChartDet_DetectingTextCoding(const char *str, QString &encoding, float &confidence)
{
    DetectObj *obj = detect_obj_init();

    if (obj == nullptr) {
        //qInfo() << "Memory Allocation failed\n";
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
    case CHARDET_OUT_OF_MEMORY :
        qInfo() << "On handle processing, occured out of memory\n";
        detect_obj_free(&obj);
        return CHARDET_OUT_OF_MEMORY;
    case CHARDET_NULL_OBJECT :
        qInfo() << "2st argument of chardet() is must memory allocation with detect_obj_init API\n";
        return CHARDET_NULL_OBJECT;
    }

#ifndef CHARDET_BOM_CHECK
    //qInfo() << "encoding:" << obj->encoding << "; confidence: " << obj->confidence;
#else
    // from 1.0.6 support return whether exists BOM
    qInfo() << "encoding:" << obj->encoding << "; confidence: " << obj->confidence << "; bom: " << obj->bom;
#endif

    encoding = obj->encoding;
    confidence = obj->confidence;
    detect_obj_free(&obj);

    return CHARDET_SUCCESS ;
}

bool DetectCode::ChangeFileEncodingFormat(QByteArray &inputStr, QByteArray &outStr, QString fromCode, QString toCode)
{
    if (fromCode == toCode) {
        outStr = inputStr;
        return true;
    }

    iconv_t handle = iconv_open(toCode.toLocal8Bit().data(), fromCode.toLocal8Bit().data());
    // int val = 1;
    // iconvctl(handle, ICONV_SET_DISCARD_ILSEQ, &val);

    if (handle != reinterpret_cast<iconv_t>(-1)) {
        char *inbuf = inputStr.data();
        size_t inbytesleft = static_cast<size_t>(inputStr.size()) + 1;
        size_t outbytesleft = 4 * inbytesleft;
        char *outbuf = new char[outbytesleft];
        char *tmp = outbuf;

        memset(outbuf, 0, outbytesleft);
        iconv(handle, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
        iconv_close(handle);
        outStr = QByteArray(tmp);

        delete [] tmp;
        tmp = nullptr;

        return true;

    } else {
        //qDebug()<<"编码转换失败";
        return  false;
    }
}

