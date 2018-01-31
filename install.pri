isEmpty(PREFIX){
    PREFIX = /usr
}

target.path = $${PREFIX}/bin/

desktop_files.path = $${PREFIX}/share/applications/
desktop_files.files = $$PWD/*.desktop

services.path = $${PREFIX}/share/dbus-1/system-services
services.files = $$PWD/*.service

policy.path = $${PREFIX}/share/polkit-1/actions/
policy.files = $${PWD}/*.policy

config.path = $${PREFIX}/share/dbus-1/system.d/
config.files = $${PWD}/*.conf

dman.path = $${PREFIX}/share/dman/
dman.files = $$PWD/dman/*

translations.path = $${PREFIX}/share/$${TARGET}/translations
translations.files = $$PWD/translations/*.qm

INSTALLS += target translations desktop_files services dman config policy


