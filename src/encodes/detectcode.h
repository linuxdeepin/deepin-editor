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
