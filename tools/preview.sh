#!/bin/sh
# Renders the Events view widget to PNGs on the build machine, so the layout
# can be looked at without the N9.
#
# It compiles the shipped forecastview.cpp against the host Qt with made-up
# data (tools/preview_main.cpp). The MeeGoTouch extension cannot run anywhere
# but the device -- the Qt Simulator has no Events view -- so this is the only
# way to see the composition before packaging.
#
# The artwork comes from the installed deb; point IMAGES/ICONSET at a tree
# extracted from it. Defaults match tools/extract-deb.sh.
#
#   tools/preview.sh            # -> build/preview-{hours,days,nodata}.png
set -e
cd "$(dirname "$0")/.."

REMOTE=/tmp/meecast-wind-preview
SHARE=${SHARE:-build/share}

if [ ! -d "$SHARE/images" ]; then
    echo "No artwork at $SHARE/images -- run tools/extract-deb.sh first." >&2
    exit 1
fi

HOST=$(sh "$HOME/ps/nfsshift-sfos/tools/buildhost.sh")
echo "== preview host: $HOST"

ssh "$HOST" "rm -rf $REMOTE && mkdir -p $REMOTE/share"
rsync -a -e ssh plugin/forecastview.cpp plugin/forecastview.h \
                tools/preview_main.cpp "$HOST:$REMOTE/"
rsync -a -e ssh "$SHARE/images" "$SHARE/iconsets" "$HOST:$REMOTE/share/"

ssh "$HOST" "set -e
    cd $REMOTE
    cat > preview.pro <<'EOF'
TEMPLATE = app
TARGET = preview
QT += gui
CONFIG -= app_bundle
DEFINES += MEECAST_IMAGES_PATH=\\\\\\\"$REMOTE/share/images\\\\\\\"
HEADERS = forecastview.h
SOURCES = forecastview.cpp preview_main.cpp
EOF
    qmake >/dev/null && make -j4 > build.log 2>&1 || { tail -30 build.log; exit 1; }
    QT_QPA_PLATFORM=offscreen ./preview share/images share/iconsets/Meecast preview"

mkdir -p build
rsync -a -e ssh "$HOST:$REMOTE/preview-*.png" build/
ls -la build/preview-*.png
