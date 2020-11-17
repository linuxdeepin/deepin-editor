#!/bin/bash
#install iconv lib author liangweidong
cd libiconv-1.16
path=`pwd`
./configure --enable-extra-encodings --prefix=$path/../lib --enable-static --disable-shared
make
make install