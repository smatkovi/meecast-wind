#!/bin/sh
# Unpacks the stock meecast deb into build/ so the other scripts have the
# artwork, the pristine QML and the Aegis manifest to work from.
#
#   tools/extract-deb.sh [path to meecast_1.1.33_armel.deb]
set -e
cd "$(dirname "$0")/.."

DEB=${1:-$HOME/Downloads/meecast_1.1.33_armel.deb}
[ -f "$DEB" ] || { echo "deb not found: $DEB" >&2; exit 1; }

rm -rf build/deb
mkdir -p build/deb
cp "$DEB" build/deb/
(cd build/deb && ar x "$(basename "$DEB")")

mkdir -p build/deb/data build/deb/control
tar xzf build/deb/data.tar.gz    -C build/deb/data
tar xzf build/deb/control.tar.gz -C build/deb/control

rm -rf build/share
cp -r build/deb/data/opt/com.meecast.omweather/share build/share

echo "artwork:  build/share/images, build/share/iconsets"
echo "manifest: build/deb/_aegis"
