#!/bin/sh
# Syntax-checks the patched QML on the build machine.
#
# The N9's QML is Qt 4.7, but the grammar is the same one Qt 5 parses, so a
# Qt 5 qmlscene reports syntax errors faithfully. It cannot resolve
# com.nokia.meego or Qt.labs.gestures -- those lines are expected and are
# filtered out here. Everything else is a real finding.
#
# qmlscene has to run on the build machine: the phone this repo is edited from
# has no usable offscreen platform plugin (it core-dumps).
#
# The script proves its own sensitivity first by feeding qmlscene a file with a
# deliberate syntax error; a silent pass on a broken file means the check is
# worthless and we abort rather than report a green run.
set -e
cd "$(dirname "$0")/.."
HOST=$(sh "$HOME/ps/nfsshift-sfos/tools/buildhost.sh")
REMOTE=/tmp/meecast-wind-qmlcheck

ssh "$HOST" "rm -rf $REMOTE && mkdir -p $REMOTE"
rsync -a -e ssh qml/ "$HOST:$REMOTE/"

ssh "$HOST" "cd $REMOTE && sed -i 's/^import Qt 4\.7/import QtQuick 2.0/' *.qml

    canary() {
        cp WindRow.qml /tmp/mcw-canary.qml
        sed -i 's/spacing: 6/spacing: 6 }}}/' WindRow.qml
        out=\$(QT_QPA_PLATFORM=offscreen qmlscene --quit WindRow.qml 2>&1 | grep -c 'Syntax error' || true)
        cp /tmp/mcw-canary.qml WindRow.qml
        [ \"\$out\" -gt 0 ]
    }
    if ! canary; then
        echo 'ABORT: qmlscene did not flag a deliberately broken file -- check is not working' >&2
        exit 2
    fi

    rc=0
    for f in *.qml; do
        errs=\$(QT_QPA_PLATFORM=offscreen qmlscene --quit \"\$f\" 2>&1 \
               | grep -v 'module \"com.nokia.meego\" is not installed' \
               | grep -v 'module \"Qt.labs.gestures\" is not installed' \
               | grep -v '^\$' || true)
        if [ -n \"\$errs\" ]; then
            printf '%s:\n%s\n' \"\$f\" \"\$errs\"
            rc=1
        else
            printf '%-22s ok\n' \"\$f\"
        fi
    done
    exit \$rc"
