# MeeCast wind patch (Nokia N9 / MeeGo Harmattan)

Adds wind speed and direction to MeeCast 1.1.33 on the N9, in the app's day
list and hourly list, and in the Events view (the feed on the home screen).
Ships as a patch .deb that installs over the stock `meecast` package.

Only the openweathermap.org source still works, so that is what this targets.

## Where the code came from

MeeCast is GPL and upstream is <https://github.com/meecast/meecast>. No
disassembly was needed:

* The installed QML is plain text in the deb under
  `/opt/com.meecast.omweather/share/omweather/qml`, and it is **identical**
  to the QML at upstream commit `47e36bf2` ("Done version 1.1.33",
  2020-11-11) -- the same day as the shipped `libevents-meecast.so`.
* The Events view plugin source was deleted from master in commit `90021222`
  ("Next step for destroing harmattan ..."). It is recovered from
  `47e36bf2:meecast/meegotouchplugin/`.
* `meecast/core/data/data.xsd` at `47e36bf2` is byte-identical to the
  `data.xsd` installed by the deb, which confirms the version pin.

`orig/` holds the pristine 1.1.33 QML so `diff orig qml` shows the patch.

## The two halves of the job

**The app (QML only).** `WeatherPage.qml`'s day rows and
`FullWeatherPage.qml`'s hourly rows show date, icon and temperatures and
nothing else. Both now carry a `WindRow` underneath the date. The
current-conditions block and the day/night/now detail grid already showed
wind upstream and are untouched.

The models already expose what is needed: `dataitem.cpp` `roleNames()` maps
`wind_speed` and `wind_direction`, and every list on both pages is the same
`DataModel`, so `model.wind_speed` just works.

**m/s is a setting, not a patch.** Core converts wind into
`Config.windspeedunit` before QML ever sees it, so the unit comes from
Settings > Units. `WindRow` appends whatever that unit is, and reproduces
upstream's special case of printing the Beaufort scale as a bare number.

**The Events view (C++).** `libevents-meecast.so` is a MeeGoTouch application
extension that paints a 127x96 `QImage` with station, icon and temperature.
It is a pure D-Bus receiver: `com.meecast.applet`'s `SetCurrentData` carries
`station, temperature, temperature_hi, temperature_low, icon, description,
until_valid_time, current, lockscreen, standbyscreen, last_update` -- no wind
and no forecast beyond the current item.

So extending that signature would mean rebuilding `omweather-qml` and
`predaemon` too and keeping three binaries in lockstep. Instead the plugin
reads the forecast cache itself, the way `predeamon.cpp` does:

    Config::Instance(getConfigPath() + "config.xml", ...)
    DataParser::Instance(station->fileName(), "/opt/.../share/xsd/data.xsd")

The cache is `~/.config/com.meecast.omweather/<source>_<station id>` and its
schema (`data.xsd`) has `wind_speed` and `wind_direction` per `<period>`.
`SetCurrentData` then only serves as the "cache was rewritten, re-read and
repaint" trigger.

Day / night / hourly are split exactly as `qt-qml/controller.cpp` does it, so
the widget and the app never disagree:

| list  | rule |
| --- | --- |
| days | `GetDataForTime(midnight + 15h + 1s + n*24h)`, n = 0..6 |
| nights | `GetDataForTime(midnight + 3h + n*24h)` |
| hours | `GetDataForTime(current_hour + n*3600, true)` where `StartTime()+60` matches, over 5 days |
| today's strongest wind | the largest `WindSpeed()` over `GetDataForTime(midnight + n*3600)`, n = 0..23 |

`midnight` there is the **station's** midnight, not the phone's: controller.cpp
shifts into the station's zone, truncates the day with `gmtime`, then shifts
back through the phone's own offset. Using plain local midnight instead puts
the tile's "Today" on a different period than the app's first day row whenever
the two zones differ. `current_hour` on the other hand really is the phone's,
`localtime` with `tm_min = 1`.

## Building

The toolchain is MADDE's own gcc 4.4 on the build machine (`tools/buildhost.sh`
in `nfsshift-sfos` picks LAN or tunnel), target
`harmattan_10.2011.34-1_rt1.2`. The GCC 14 cross toolchain used for Snapszer
is wrong here: this is Qt4-era C++03 and needs MADDE's matching moc and its
`meegotouch.prf` qmake feature.

One fix is needed to build the plugin outside a real Harmattan scratchbox:
`meegotouch.prf` emits a bare `-I/usr/include/meegotouch`. MADDE's g++ wrapper
resolves that against the sysroot, but `mmoc` does not, and moc then fails with
`Error: Undefined interface` on `Q_INTERFACES(MApplicationExtensionInterface)`.
`meegotouchplugin.pro` therefore adds

    INCLUDEPATH += $$[QT_INSTALL_HEADERS]/../meegotouch

With that, the **pristine** plugin builds and, stripped, comes out at 110,928
bytes against the shipped 107,624 with an identical `NEEDED` list -- the
toolchain reproduces the original.

## Checking the QML

    tools/check-qml.sh

Qt 4.7 and Qt 5 share the QML grammar, so a Qt 5 `qmlscene` on the build
machine reports syntax errors faithfully; `com.nokia.meego` and
`Qt.labs.gestures` cannot resolve there and are filtered out. The script
first feeds qmlscene a deliberately broken file and aborts if that is not
flagged -- an early version of this check silently passed everything, so it
now proves itself before reporting a green run.

This is a syntax check only. Layout and real data still need the device.

## Two Harmattan traps

* **Aegis.** A `.so` copied over the installed one refuses to load, because
  its hash no longer matches the package refhashlist. The plugin must go in
  via `dpkg -i`. QML files are not validated, so those can be iterated over
  scp freely.
* **Reinstalling `meecast` overwrites this patch.** Reinstall the patch deb
  afterwards.

## The Events view widget

480 x 636, drawn into one `QImage` allocated once:

* station and the time of the last update,
* the current conditions with the wind **now** and, under it, the strongest
  wind of **today** over every period the cache holds for it ("Day max 7 m/s
  NNE"). The reading of the moment says nothing about the afternoon, and the
  afternoon is what one dresses for,
* a heading "Hours" and the next five three-hourly periods,
* a heading "Day" and the coming seven days,

each forecast line with its own wind. A tap anywhere opens MeeCast.

**There were tab headers here until 1.1.** Hours and days turned out to be
wanted at the same time -- which hour to leave, and what the rest of the week
looks like -- and whichever tab was showing, it was the other one that was
wanted. The Events feed scrolls, so height is the cheap thing to spend; both
lists are simply drawn one under the other now. That also removes the tab
hit-testing and `~/.config/com.meecast.omweather/eventsview.conf` -- a tap
that is not a tab is one less thing that can go wrong inside the home screen
process.

No MButton and no CSS: the stock `meecast-extension.css` is never installed,
so the tile takes its size from the image alone. Horizontal swipes are left
alone -- the home screen owns those.

The extension lives in its own `mapplicationextensionrunner` process, which
the home screen starts. Killing that runner does **not** bring it back; after
installing a new `.so`, restart `meegotouchhome` (kill it, upstart respawns
it) and the runner comes back with the new library.

`gettext` is used as `dgettext("omweather", ...)`, never `textdomain()`: this
is a shared library inside the home screen process and switching the
process-wide domain would break every other extension loaded beside it.

## Looking at the widget without the device

    tools/extract-deb.sh        # artwork out of the stock deb
    tools/preview.sh            # -> build/preview-{hours,days,nodata}.png

`forecastview.cpp` (drawing) deliberately depends on nothing but QtGui, and
`forecastread.cpp` (Core) is separate, so the shipped drawing code compiles
against a desktop Qt and renders the widget to a PNG with made-up data. A
MeeGoTouch application extension cannot run anywhere but the device -- the Qt
Simulator has no Events view -- so this is the only way to see the layout
before packaging. Fonts differ ("Nokia Pure" is not installed on a desktop),
so it checks composition, not pixels.

Alternating row shading was tried this way and dropped: even a wash light
enough not to fight the feed's backdrop made the text on every second line
read as bold.

The preview binary also unit-tests `tabAt()`, which decides whether a tap hit
a tab header. It works in fractions of the widget's actual size rather than
image pixels, because `MImageWidget` scales the image to whatever width the
feed grants the extension -- a test in 480-space would drift far enough on a
narrower feed to put the tab band over the first forecast row. That is not
observable on the device until it misbehaves, hence the test.

## Packaging

    tools/build-plugin.sh
    tools/mkdeb.sh --qml-only   # -> build/meecast-wind-qml_1.0_all.deb
    tools/mkdeb.sh              # -> build/meecast-wind_1.0_armel.deb

Two packages on purpose, and they conflict with each other. The QML half is
the low-risk one: Aegis does not validate it and a mistake is visible in the
app. The plugin replaces the Events view tile, so if the rebuilt `.so` fails
to load the tile disappears, which is worse than stock -- keep it installable
on its own once the QML half is known good.

`Replaces: meecast` is what lets dpkg lay these files over ones the stock
package owns. To go back to stock, reinstall `meecast_1.1.33_armel.deb`. `tools/mkdeb.py` is the Harmattan packager from the Snapszer
port: Harmattan's dpkg is 1.15.x, so members must be `debian-binary`,
`control.tar.gz`, `data.tar.gz`, gzip only, and without GNU ar's trailing
slash on member names.

On the phone:

    devel-su dpkg -i meecast-wind-qml_1.0_all.deb     # app lists only
    devel-su dpkg -i meecast-wind_1.0_armel.deb       # and the Events view
    killall mapplicationextensionrunner    # or reboot, to reload the extension

## Licence

MeeCast is GPL and this is a derived work: the recovered plugin sources keep
their upstream headers ("either version 2.1 of the License, or (at your
option) any later version"), and the new files here follow them. `orig/` holds
the pristine 1.1.33 QML so `diff orig qml` shows exactly what was changed.

## Status

* [x] `WindRow.qml`, wired into the day list and the hourly list.
* [x] Plugin source recovered, version pinned, toolchain proven on the
      pristine source (stripped: 110,928 bytes against the shipped 107,624,
      identical `NEEDED`).
* [x] Events view: reads the cache, hourly/daily tabs, wind on every row.
      Layout reviewed in the desktop preview.
* [x] Patch deb builds: `meecast-wind_1.0_armel.deb`.
* [ ] **Nothing has run on the N9 yet** -- it was unreachable for this whole
      session (`No route to host`). Everything below is unverified.
* [ ] Confirm openweathermap.org actually fills `wind_speed` /
      `wind_direction` on **hourly** periods; the hourly rows and the hourly
      tab both depend on it:
      `cat ~/.config/com.meecast.omweather/openweathermap.org_*`
* [ ] Confirm the new plugin loads at all (Aegis, and Core being linked into
      the home screen process) and that the tile is not clipped by the feed.
