/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     liangweidong <liangweidong@uniontech.com>
*
* Maintainer: liangweidong <liangweidong@uniontech.com>
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
#include "detectcode.h"
#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <stdio.h>


QMap<QString,QByteArray> DetectCode::sm_LangsMap;

DetectCode::DetectCode()
{

}

QByteArray DetectCode::GetFileEncodingFormat(QString filepath, QByteArray content)
{
    QString charDetectedResult;
    QByteArray ucharDetectdRet;
    QByteArrayList icuDetectRetList;
    QByteArray detectRet;
    float chardetconfidence = 0;

    /* chardet识别编码 */
    QString str(content);
    bool bFlag = str.contains(QRegExp("[\\x4e00-\\x9fa5]+")); //匹配的是中文
    if (bFlag) {
        QByteArray newContent = content;
        newContent += "为增加探测率保留的中文";    //手动添加中文字符，避免字符长度太短而导致判断编码错误
        DetectCode::ChartDet_DetectingTextCoding(newContent, charDetectedResult, chardetconfidence);
    } else {
        DetectCode::ChartDet_DetectingTextCoding(content, charDetectedResult, chardetconfidence);
    }
    ucharDetectdRet = charDetectedResult.toLatin1();

    /* uchardet识别编码 */
    if (ucharDetectdRet == "unknown" || ucharDetectdRet == "???" || ucharDetectdRet.isEmpty()) {
       ucharDetectdRet = DetectCode::UchardetCode(filepath);
    }

    /* icu识别编码 */
    icuDetectTextEncoding(filepath, icuDetectRetList);
    detectRet = selectCoding(ucharDetectdRet, icuDetectRetList);

    if(detectRet == "ASCII" || detectRet.isEmpty()) {
        detectRet = "UTF-8";
    }

    return detectRet.toUpper();
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

    while (!feof(file)) {
        int32_t len = fread(buffer, 1, iBuffSize, file);
        if (detected == nullptr) {
            if (!detectTextEncoding(buffer, len, &detected, listDetectRet)) {
                break;
            }
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
        *detected = strdup(ucsdet_getName(csm[0], &status));
        listDetectRet << QByteArray(*detected);
        *detected = strdup(ucsdet_getName(csm[1], &status));
        listDetectRet << QByteArray(*detected);
        *detected = strdup(ucsdet_getName(csm[2], &status));
        listDetectRet << QByteArray(*detected);
        if (status != U_ZERO_ERROR) {
            return false;
        }
    } else if (matchCount > 0) {
        *detected = strdup(ucsdet_getName(csm[0], &status));
        listDetectRet << QByteArray(*detected);
    }

    ucsdet_close(csd);

    return true;
}

QByteArray DetectCode::selectCoding(QByteArray ucharDetectdRet, QByteArrayList icuDetectRetList)
{
    if (ucharDetectdRet.isEmpty() && icuDetectRetList.isEmpty()) {
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


QByteArray DetectCode::UchardetCode(QString filepath)
{
    //qDebug()<<"UchardetCode Begin:"<<QDateTime::currentDateTime().toString("hh:mm:ss");
    FILE* fp;
    QByteArray charset;

    size_t buffer_size = 0x10000;
    char* buff = new char[buffer_size];
    memset(buff,0,buffer_size);

    /* 通过样本字符分析文本编码 */
    uchardet_t handle = uchardet_new();

    /* 打开被检测文本文件，并读取一定数量的样本字符 */
    fp = fopen(filepath.toLocal8Bit().data(), "rb");

    if(fp){
        while (!feof(fp))
        {
            size_t len = fread(buff, 1, buffer_size, fp);
            int retval = uchardet_handle_data(handle, buff, len);
            if (retval != 0)
            {
                //qDebug()<<QStringLiteral("Uchardet分析编码失败")<<QString(buff);
                continue;
            }

            break;
        }
        fclose(fp);

        uchardet_data_end(handle);
        charset = uchardet_get_charset(handle);
        //qDebug()<<QStringLiteral("Uchardet文本的编码方式是:")<<charset;
        uchardet_delete(handle);
    }

    delete [] buff;
    buff = nullptr;

    if(charset == "MAC-CENTRALEUROPE") charset = "MACCENTRALEUROPE";
    if(charset == "MAC-CYRILLIC") charset = "MACCYRILLIC";
    if(charset.contains("WINDOWS-")) charset = charset.replace("WINDOWS-","CP");
    //qDebug()<<"UchardetCode End:"<<QDateTime::currentDateTime().toString("hh:mm:ss");
    return charset;
}

#if 0
QByteArray DetectCode::EncaDetectCode(QString filepath)
{
  // qDebug()<<"EncaDetectCode Begin:"<<QDateTime::currentDateTime().toString("hh:mm:ss");
  //"zh"中文 "be"俄罗斯 "bg"保加利亚 "cs"捷克文 "et"爱沙尼亚语 "hr"克罗地亚人[语]; "hu"匈牙利语 "lt"立陶宛 "lv"拉脱维亚语 "pl"波兰语 "ru"俄语 "sk"斯洛伐克人（语）  "sl"斯洛文尼亚人（语）  "uk"乌克兰人（语）
   sm_LangsMap.clear();

   const char* langArray[]={"zh","be","bg","cs","et","hr","hu","lt","lv","pl","ru","sk","sl","uk"};

   QByteArray charset;

   for (size_t i= 0; i < sizeof (langArray)/sizeof (const char *); i++) {

       EncaAnalyser analyser = nullptr;

       analyser = enca_analyser_alloc(langArray[i]);
       enca_set_threshold(analyser, 1.38);
       enca_set_multibyte(analyser, 1);
       enca_set_ambiguity(analyser, 1);
       enca_set_garbage_test(analyser, 1);

       size_t buffer_size = 0x10000;

       unsigned char* buff = new unsigned char[buffer_size];
       memset(buff,0,buffer_size);

       /* 打开被检测文本文件，并读取一定数量的样本字符 */
       FILE *fp; /* the processed file */
       fp = fopen(filepath.toLocal8Bit().data(), "rb");

       if(fp){
           while (!feof(fp))
           {
                size_t len = fread(buff, 1, buffer_size, fp);
                EncaEncoding encoding = enca_analyse(analyser,buff,len);
                charset = enca_charset_name(encoding.charset,EncaNameStyle::ENCA_NAME_STYLE_MIME);
                qDebug()<<QStringLiteral("ENCA文本的编码方式是:")<<charset;
                //识别文本编码识别
                if(encoding.charset == -1) continue;
                break;
           }

           enca_analyser_free(analyser);
           analyser = nullptr;

           delete [] buff;
           buff = nullptr;

           fclose(fp);
       }else {
            qDebug()<<QStringLiteral("ENCA打开失败:")<<filepath;
       }

       if(charset == "US-ASCII") charset = "ASCII";
       if(charset == "GB2312" || charset == "GBK") charset = "GB18030";
       if(charset == "ISO-10646-UCS-2") charset = "UTF-16BE";
       sm_LangsMap[langArray[i]] = charset;

       if(!charset.isEmpty()){
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

bool DetectCode::ChangeFileEncodingFormat(QByteArray &inputStr, QByteArray &outStr,QString fromCode, QString toCode)
{
    if (fromCode == toCode) {
        outStr = inputStr;
        return true;
    }

    iconv_t handle = iconv_open(toCode.toLocal8Bit().data(), fromCode.toLocal8Bit().data());
    // int val = 1;
    // iconvctl(handle, ICONV_SET_DISCARD_ILSEQ, &val);

    if (handle != reinterpret_cast<iconv_t>(-1)) {
        char* inbuf = inputStr.data();
        size_t inbytesleft = static_cast<size_t>(inputStr.size())+1;
        size_t outbytesleft = 4*inbytesleft;
        char* outbuf = new char[outbytesleft];
        char* tmp = outbuf;

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

