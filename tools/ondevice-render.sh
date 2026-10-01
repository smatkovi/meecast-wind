#!/bin/sh
# Builds evrender (tools/render_main.cpp) for the N9 and leaves it in build/.
#
# Same toolchain as tools/build-plugin.sh; it links the same Core and the same
# two forecast files the plugin does, so what it prints and draws is what the
# tile would have drawn. Run it on the device with DISPLAY=:0.
set -e
cd "$(dirname "$0")/.."

TARGET=harmattan_10.2011.34-1_rt1.2
REMOTE=/tmp/meecast-wind

HOST=$(sh "$HOME/ps/nfsshift-sfos/tools/buildhost.sh")
echo "== build host: $HOST"

ssh "$HOST" "mkdir -p $REMOTE/evrender"
rsync -a -e ssh plugin/forecastview.cpp plugin/forecastview.h plugin/forecastread.cpp \
                tools/render_main.cpp "$HOST:$REMOTE/evrender/"

ssh "$HOST" "set -e
    export PATH=\$HOME/QtSDK/Madde/bin:\$PATH
    cd $REMOTE/evrender
    cat > evrender.pro <<'EOF2'
TEMPLATE = app
TARGET = evrender
QT += gui xml network
CONFIG -= app_bundle
CONFIG += link_pkgconfig
INCLUDEPATH += ../core
LIBS += ../core/libomweather-core.a
PKGCONFIG += libcurl
PKGCONFIG += sqlite3
HEADERS = forecastview.h
SOURCES = forecastview.cpp forecastread.cpp render_main.cpp
EOF2
    mad -t $TARGET qmake >/dev/null
    mad -t $TARGET make -j4 > /tmp/meecast-evrender.log 2>&1 || {
        tail -40 /tmp/meecast-evrender.log; exit 1; }
    ls -la evrender"

mkdir -p build
rsync -a -e ssh "$HOST:$REMOTE/evrender/evrender" build/
file build/evrender
