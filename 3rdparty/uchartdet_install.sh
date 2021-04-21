#!/bin/bash
#install iconv lib author liangweidong
cd uchardet-0.0.7
path=`pwd`
cmake . -DCMAKE_INSTALL_LIBDIR=$path/../lib
make
make install