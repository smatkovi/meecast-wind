#!/bin/sh
# Builds libevents-meecast.so for the N9 on the build machine.
#
# MeeCast 1.1.33 is Qt4-era C++03, so this uses MADDE's own gcc 4.4 with its
# matching mmoc and meegotouch.prf -- not the GCC 14 cross toolchain in
# ~/ps/toolchains, which exists for the C++17 Snapszer port.
#
# core/ and plugin/ are both vendored from upstream commit 47e36bf2 ("Done
# version 1.1.33"), so nothing outside this repository is needed.
#
#   tools/build-plugin.sh            # -> build/libevents-meecast.so (stripped)
set -e
cd "$(dirname "$0")/.."

TARGET=harmattan_10.2011.34-1_rt1.2
REMOTE=/tmp/meecast-wind

HOST=$(sh "$HOME/ps/nfsshift-sfos/tools/buildhost.sh")
echo "== build host: $HOST"

echo "== syncing sources"
ssh "$HOST" "mkdir -p $REMOTE/core $REMOTE/meegotouchplugin"
rsync -a --delete -e ssh core/   "$HOST:$REMOTE/core/"
rsync -a --delete -e ssh plugin/ "$HOST:$REMOTE/meegotouchplugin/"

# grep -c so a clean build is not mistaken for a failure, and so the exit
# status comes from the compiler rather than from grep finding nothing.
ssh "$HOST" "set -e
    export PATH=\$HOME/QtSDK/Madde/bin:\$PATH

    echo '== building core'
    cd $REMOTE/core
    mad -t $TARGET qmake >/dev/null
    mad -t $TARGET make -j4 > /tmp/meecast-wind-core.log 2>&1 || {
        echo '-- core build failed'; tail -30 /tmp/meecast-wind-core.log; exit 1; }
    ls -la libomweather-core.a

    echo '== building plugin'
    cd $REMOTE/meegotouchplugin
    mad -t $TARGET qmake >/dev/null
    mad -t $TARGET make -j4 > /tmp/meecast-wind-plugin.log 2>&1 || {
        echo '-- plugin build failed'; tail -40 /tmp/meecast-wind-plugin.log; exit 1; }
    \$HOME/QtSDK/Madde/targets/$TARGET/bin/strip --strip-unneeded lib/libevents-meecast.so
    ls -la lib/libevents-meecast.so"

mkdir -p build
rsync -a -e ssh "$HOST:$REMOTE/meegotouchplugin/lib/libevents-meecast.so" build/
echo "== build/libevents-meecast.so"
file build/libevents-meecast.so
