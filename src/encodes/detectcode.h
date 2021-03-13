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
#include <uchardet/uchardet.h>
#include <enca.h>
#include <libcharset.h>
#include <iconv.h>
/*
*
* 文本编码识别引用第三库识别 enca uchardet
* enca识别不了使用uchardet
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
    static QByteArray EncaDetectCode (QString filepath);

    //uchardet 识别文编编码
    static QByteArray UchardetCode(QString filepath);

    //获取文件编码方式
    static QByteArray GetFileEncodingFormat (QString filepath);

    static bool ChangeFileEncodingFormat(QByteArray& inputStr,QByteArray& outStr,QString fromCode,QString toCode=QString("UTF-8"));
private:
    static  QMap<QString,QByteArray> sm_LangsMap;
};


#endif // DETECTCODE_H
