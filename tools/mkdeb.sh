#!/bin/sh
# Packages the patched QML and the rebuilt Events view plugin as a patch deb
# that installs over the stock meecast 1.1.33.
#
# Run tools/build-plugin.sh first.
#
#   tools/mkdeb.sh [version]     # -> build/meecast-wind_<version>_armel.deb
set -e
cd "$(dirname "$0")/.."

VERSION=${1:-1.0}
STAGE=build/stage
OUT=build/meecast-wind_${VERSION}_armel.deb

[ -f build/libevents-meecast.so ] || {
    echo "build/libevents-meecast.so missing -- run tools/build-plugin.sh" >&2
    exit 1
}

rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" \
         "$STAGE/opt/com.meecast.omweather/share/omweather/qml" \
         "$STAGE/usr/lib/meegotouch/applicationextensions"

cp qml/WeatherPage.qml qml/FullWeatherPage.qml qml/WindRow.qml \
   "$STAGE/opt/com.meecast.omweather/share/omweather/qml/"
cp build/libevents-meecast.so "$STAGE/usr/lib/meegotouch/applicationextensions/"

# Replaces: is what lets dpkg put these files over ones meecast owns while
# meecast itself stays installed. The version is pinned because the QML is
# patched against 1.1.33 line for line.
#
# No Aegis credentials are requested: the stock _aegis asks only for
# omweather-qml, and this package ships no executable of its own.
cat > "$STAGE/DEBIAN/control" <<EOF
Package: meecast-wind
Version: $VERSION
Architecture: armel
Maintainer: Sebastian Matkovich <sebastian.matkovich@gmail.com>
Depends: meecast (= 1.1.33)
Replaces: meecast
Section: user/desktop
Priority: optional
Aegis-Manifest: empty
Description: Wind speed and direction for MeeCast
 Adds wind speed and direction to the day list and the hourly list of
 MeeCast 1.1.33, and replaces the Events view widget with one that shows
 the current conditions plus a switchable hourly and multi-day forecast,
 wind on every line.
 .
 The unit follows Settings > Units in MeeCast; choose m/s there.
 .
 Reinstalling or upgrading meecast overwrites this patch; install it again
 afterwards.
EOF

python3 tools/mkdeb.py "$STAGE" "$OUT"
echo
python3 tools/mkdeb.py --info "$OUT"
