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

#ifndef DETECTCODE_H
#define DETECTCODE_H
#include <QString>
#include <QMap>
#include <chardet/chardet.h>
#include <uchardet/uchardet.h>
//#include <enca.h>
#include <iconv.h>
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/ucsdet.h>

/**********************************************
*
* 文本编码识别引用三个第三库识别
* 1:chardet  2:uchardet  3:icu
* 首次识别使用chardet
* chardet识别不出来再用uchardet识别
* uchardet识别不出来再用icu识别
* 编码转换库使用iconv
*
***********************************************/

class QByteArray;
class QString;


class DetectCode
{
public:
    DetectCode();

    //enca 识别文本编码
    /* 因为开源协议存在法律冲突，停止使用libenca0编码识别库 */
    //static QByteArray EncaDetectCode (QString filepath);

    /**
     * @brief ChartDet_DetectingTextCoding libchardet1编码识别库识别编码
     */
    static int ChartDet_DetectingTextCoding(const char *str, QString &encoding, float &confidence);

    //uchardet 识别文编编码
    static QByteArray UchardetCode(QString filepath);

    /**
     * @author guoshao ut000455
     * @brief  icuDetectTextEncoding() icu库编码识别
     * @param  filePath:文件路径，listDetectRet:编码识别结果存在的变量
     * @return
     **/
    static void icuDetectTextEncoding(const QString &filePath, QByteArrayList &listDetectRet);

    /**
     * @author guoshao ut000455
     * @brief  detectTextEncoding() icu库编码识别内层函数
     * @param  data:要识别的内容，len:要识别的内容的长度，detected:编码识别结果存在的变量，
     *         listDetectRet:编码识别结果存在的list
     * @return true:识别成功，false:识别失败
     **/
    static bool detectTextEncoding(const char *data, size_t len, char **detected, QByteArrayList &listDetectRet);

    /**
     * @author guoshao ut000455
     * @brief  selectCoding() 筛选识别出来的编码
     * @param  ucharDetectdRet:chardet/uchardet识别的编码结果，icuDetectRetList:编码识别结果存在的list
     * @return 筛选编码的结果
     **/
    static QByteArray selectCoding(QByteArray ucharDetectdRet, QByteArrayList icuDetectRetList);

    //获取文件编码方式
    static QByteArray GetFileEncodingFormat (QString filepath, QByteArray content = QByteArray(""));

    static bool ChangeFileEncodingFormat(QByteArray& inputStr,QByteArray& outStr,QString fromCode,QString toCode=QString("UTF-8"));
private:
    static  QMap<QString,QByteArray> sm_LangsMap;
};


#endif // DETECTCODE_H
