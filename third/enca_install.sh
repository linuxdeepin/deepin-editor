#!/bin/bash
#install iconv lib author liangweidong
cd enca-1.19
path=`pwd`
./configure --prefix=$path/../lib
make
make install