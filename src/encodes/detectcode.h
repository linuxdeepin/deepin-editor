// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DETECTCODE_H
#define DETECTCODE_H
#include <QString>
#include <QMap>
#include <chardet/chardet.h>
#include <uchardet/uchardet.h>
#include <iconv.h>

#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/ucsdet.h>
#include <unicode/umachine.h>
#include <unicode/urename.h>

/*
*
* 文本编码识别引用第三库识别 chardet1 uchardet
* chardet识别不了使用uchardet
* 编码转换库使用iconv
*
* author:梁卫东 2020年10月15日16:56:11
*
*/

class QByteArray;
class QString;


class DetectCode
{
public:
    DetectCode();

    //enca 识别文本编码
    #if 0 /* 因为开源协议存在法律冲突，停止使用libenca0编码识别库 */
    static QByteArray EncaDetectCode (QString filepath);
    #endif

    /**
     * @brief ChartDet_DetectingTextCoding libchardet1编码识别库识别编码
     */
    static int ChartDet_DetectingTextCoding(const char *str, QString &encoding, float &confidence);

    //uchardet 识别文编编码
    static QByteArray UchardetCode(QString filepath);
    /**
     * @author guoshao
     * @brief  icuDetectTextEncoding() icu库编码识别
     * @param  filePath:文件路径，listDetectRet:编码识别结果存在的变量
     * @return
     **/
    static void icuDetectTextEncoding(const QString &filePath, QByteArrayList &listDetectRet);
    /**
     * @author guoshao
     * @brief  detectTextEncoding() icu库编码识别内层函数
     * @param  data:要识别的内容，len:要识别的内容的长度，detected:编码识别结果存在的变量，
     *         listDetectRet:编码识别结果存在的list
     * @return true:识别成功，false:识别失败
     **/
    static bool detectTextEncoding(const char *data, size_t len, char **detected, QByteArrayList &listDetectRet);
    /**
     * @author guoshao
     * @brief  selectCoding() 筛选识别出来的编码
     * @param  ucharDetectdRet:chardet/uchardet识别的编码结果，icuDetectRetList:编码识别结果存在的list
     * @return 筛选编码的结果
     **/
    static QByteArray selectCoding(QByteArray ucharDetectdRet, QByteArrayList icuDetectRetList);

    //获取文件编码方式
    static QByteArray GetFileEncodingFormat(QString filepath, QByteArray content = QByteArray(""));

    static bool ChangeFileEncodingFormat(QByteArray& inputStr,QByteArray& outStr,QString fromCode,QString toCode=QString("UTF-8"));
private:
    static  QMap<QString,QByteArray> sm_LangsMap;
};


#endif // DETECTCODE_H
