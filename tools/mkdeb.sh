#!/bin/sh
# Packages the patched QML and the rebuilt Events view plugin as a patch deb
# that installs over the stock meecast 1.1.33.
#
# Run tools/build-plugin.sh first.
#
#   tools/mkdeb.sh [version]              # QML + Events view widget
#   tools/mkdeb.sh --qml-only [version]   # only the application's lists
#
# The two halves are packaged separately on purpose. The QML is the low-risk
# one -- Aegis does not validate it and a mistake is visible in the app -- and
# it can go on the phone the moment it is reachable. The plugin replaces the
# Events view tile, so if the rebuilt .so fails to load, the tile disappears
# and that is worse than stock; keep it installable on its own.
set -e
cd "$(dirname "$0")/.."

QML_ONLY=no
if [ "$1" = "--qml-only" ]; then
    QML_ONLY=yes
    shift
fi

VERSION=${1:-1.0}
STAGE=build/stage

if [ "$QML_ONLY" = yes ]; then
    PKG=meecast-wind-qml
    ARCH=all
    OUT=build/${PKG}_${VERSION}_all.deb
else
    PKG=meecast-wind
    ARCH=armel
    OUT=build/${PKG}_${VERSION}_armel.deb
    [ -f build/libevents-meecast.so ] || {
        echo "build/libevents-meecast.so missing -- run tools/build-plugin.sh" >&2
        exit 1
    }
fi

rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" "$STAGE/opt/com.meecast.omweather/share/omweather/qml"

cp qml/WeatherPage.qml qml/FullWeatherPage.qml qml/WindRow.qml \
   "$STAGE/opt/com.meecast.omweather/share/omweather/qml/"

if [ "$QML_ONLY" = no ]; then
    mkdir -p "$STAGE/usr/lib/meegotouch/applicationextensions"
    cp build/libevents-meecast.so "$STAGE/usr/lib/meegotouch/applicationextensions/"
fi

# Replaces: is what lets dpkg put these files over ones meecast owns while
# meecast itself stays installed. The version is pinned because the QML is
# patched against 1.1.33 line for line.
#
# No Aegis credentials are requested: the stock _aegis asks only for
# omweather-qml, and this package ships no executable of its own.
if [ "$QML_ONLY" = yes ]; then
    SUMMARY=" Adds wind speed and direction to the day list and the hourly list of
 MeeCast 1.1.33. The Events view is left alone."
    CONFLICTS=""
else
    SUMMARY=" Adds wind speed and direction to the day list and the hourly list of
 MeeCast 1.1.33, and replaces the Events view widget with one that shows
 the current conditions plus a switchable hourly and multi-day forecast,
 wind on every line."
    CONFLICTS="Conflicts: meecast-wind-qml
Replaces: meecast, meecast-wind-qml"
fi
[ -n "$CONFLICTS" ] || CONFLICTS="Conflicts: meecast-wind
Replaces: meecast, meecast-wind"

cat > "$STAGE/DEBIAN/control" <<EOF
Package: $PKG
Version: $VERSION
Architecture: $ARCH
Maintainer: Sebastian Matkovich <sebastian.matkovich@gmail.com>
Depends: meecast (= 1.1.33)
$CONFLICTS
Section: user/desktop
Priority: optional
Aegis-Manifest: empty
Description: Wind speed and direction for MeeCast
$SUMMARY
 .
 The unit follows Settings > Units in MeeCast; choose m/s there.
 .
 Reinstalling or upgrading meecast overwrites this patch; install it again
 afterwards. To go back to stock, reinstall meecast_1.1.33_armel.deb.
EOF

python3 tools/mkdeb.py "$STAGE" "$OUT"
echo
python3 tools/mkdeb.py --info "$OUT"
