#!/bin/bash
workspace=$1
appname=$2

cd $workspace

dpkg-buildpackage -b -d -uc -us

project_path=$(cd `dirname $0`; pwd)
#获取工程名
project_name="${project_path##*/}"
echo $project_name

#获取打包生成文件夹路径
pathname=$(find . -name obj*)

echo $pathname

cd $pathname/test

mkdir -p coverage

lcov --directory ../ --capture --output-file ./coverage/coverage.info

#以下几行是过滤一些我们不感兴趣的文件的覆盖率信息，各模块根据自模块实际情况填写过滤信息
lcov --remove ./coverage/coverage.info '*/${project_name}_test_autogen/*' '*/${project_name}_autogen/*' '*/usr/include/*' '*/dbuslogin1manager*' '*obj-x86_64-linux-gnu/*' '*.h*' '*/test/*' -o ./coverage/coverage.info

genhtml -o ../report ./coverage/coverage.info

cd ../report

line=`cat index.html | grep headerCovTableEntryLo | awk '{print $2}'| cut -d '>' -f 2 | head -n 1`
func=`cat index.html | grep headerCovTableEntryLo | awk '{print $2}' | cut -d '>' -f 2 | tail -n 1`

cd $workspace
touch $appname.csv
echo $appname,"行覆盖率: "$line"% 函数覆盖率: "$func"%"> $appname.csv
