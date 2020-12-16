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

QByteArray DetectCode::GetFileEncodingFormat(QString filepath)
{
    QByteArray charset = DetectCode::EncaDetectCode(filepath);

    if(charset == "unknown" || charset == "???" || charset.isEmpty()){
       charset = DetectCode::UchardetCode(filepath);
    }
    if(charset == "ASCII" || charset.isEmpty()) charset = "UTF-8";
    return charset;
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
                qDebug()<<QStringLiteral("Uchardet分析编码失败")<<QString(buff);
                continue;
            }

            break;
        }
        fclose(fp);

        uchardet_data_end(handle);
        charset = uchardet_get_charset(handle);
        qDebug()<<QStringLiteral("Uchardet文本的编码方式是:")<<charset;
        uchardet_delete(handle);

        delete [] buff;
        buff = nullptr;
    }

     if(charset == "MAC-CENTRALEUROPE") charset = "MACCENTRALEUROPE";
     if(charset == "MAC-CYRILLIC") charset = "MACCYRILLIC";
     if(charset.contains("WINDOWS-")) charset = charset.replace("WINDOWS-","CP");
     qDebug()<<"UchardetCode End:"<<QDateTime::currentDateTime().toString("hh:mm:ss");
     return charset;
}

QByteArray DetectCode::EncaDetectCode(QString filepath)
{
  // qDebug()<<"EncaDetectCode Begin:"<<QDateTime::currentDateTime().toString("hh:mm:ss");
  //"zh"中文 "be"俄罗斯 "bg"保加利亚 "cs"捷克文 "et"爱沙尼亚语 "hr"克罗地亚人[语]; "hu"匈牙利语 "lt"立陶宛 "lv"拉脱维亚语 "pl"波兰语 "ru"俄语 "sk"斯洛伐克人（语）  "sl"斯洛文尼亚人（语）  "uk"乌克兰人（语）
    sm_LangsMap.clear();

    const char* langArray[]={"zh","be","bg","cs","et","hr","hu","lt","lv","pl","ru","sk","sl","uk"};
  // EncaAnalyserState
//   size_t size;
//   const char** lang = enca_get_languages(&size);
//   QStringList langs;
//    for (size_t i=0;i <size;i++) {
//       langs<<lang[i];
//    }
//   qDebug()<<langs<<lang[size-2];

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
       sm_LangsMap[langArray[i]] = charset;

       if(!charset.isEmpty()){
           break;
       }
   }

   return charset;
}


bool DetectCode::ChangeFileEncodingFormat(QByteArray &inputStr, QByteArray &outStr,QString fromCode, QString toCode)
{
    if(fromCode == toCode){
        qDebug()<<"=====编码转换相同:"<<fromCode;
        outStr = inputStr;
        return true;
    }

   // qDebug()<<"=====编码转化 Begin:"<<QDateTime::currentDateTime().toString("hh:mm:ss");
    iconv_t handle =iconv_open(toCode.toLocal8Bit().data(),fromCode.toLocal8Bit().data());
    int val = 1;
    int res = iconvctl(handle,ICONV_SET_DISCARD_ILSEQ,&val);

    if(handle != reinterpret_cast<iconv_t>(-1)){
        char* inbuf = inputStr.data();
        size_t inbytesleft = static_cast<size_t>(inputStr.size())+1;
        //qDebug()<<inbuf<<inbytesleft;

        size_t outbytesleft = 4*inbytesleft;

        char* outbuf = new char[outbytesleft];
        char* tmp = outbuf;
        memset(outbuf,0,outbytesleft);

       size_t len = iconv(handle,&inbuf,&inbytesleft,&outbuf,&outbytesleft);
       iconv_close(handle);

       outStr = QByteArray(tmp);
       qDebug()<<"编码转换前 文本大小:"<<inputStr.length()/*<<QString::fromUtf8(inputStr)*/;
       qDebug()<<"转换编码后 文本大小:"<<outStr.length()/*<<QString::fromUtf8(outStr)*/;

        delete [] tmp;
        tmp = nullptr;
        //qDebug()<<"======编码转化 End:"<<QDateTime::currentDateTime().toString("hh:mm:ss");
        return true;
    }else {
        qDebug()<<"编码转换失败";
        return  false;
    }
}

