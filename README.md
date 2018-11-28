# Deepin Editor

Deepin Editor is a desktop text editor that supports common text editing features.

## Dependencies

In debian, use below command to install compile dependencies:

`sudo apt install qt5-qmake qt5-default libdtkcore-dev libdtkwidget-dev libxcb-util0-dev libxcb1-dev libqt5x11extras5-dev libprocps-dev libxext-dev libxtst-dev libpolkit-qt5-1-dev libkf5syntaxhighlighting-dev libdtkwm-dev libkf5codecs-dev qttools5-dev`

## Installation

~~Build auto save daemon~~

```
sudo cp com.deepin.editor.policy /usr/share/polkit-1/actions
sudo cp com.deepin.editor.conf /usr/share/dbus-1/system.d/
sudo cp com.deepin.editor.daemon.service /usr/share/dbus-1/system-services

cd ./daemon
mkdir build
cd build
qmake ..
make
sudo ./deepin-editor-daemon
```

* Build editor

```
cd ./editor
mkdir build
cd build
qmake ..
make
./deepin-editor
```

## Usage

Below is keymap list for deepin-editor:

## Config file

configure save at: ~/.config/deepin/deepin-editor/config.conf

## Getting help

Any usage issues can ask for help via

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org)
* [WiKi](http://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

Deepin Editor is licensed under [GPLv3](LICENSE).