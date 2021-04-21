#!/bin/bash
#install iconv lib author liangweidong
cd uchardet-0.0.7
path=`pwd`
cmake . --prefix=$path/../lib  -DCMAKE_INSTALL_LIBDIR=lib
make
make install