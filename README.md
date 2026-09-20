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
| days | `GetDataForTime(midnight + 15h + n*24h)`, n = 0..7 |
| nights | `GetDataForTime(midnight + 3h + n*24h)` |
| hours | `GetDataForTime(current_hour + n*3600, true)` where `StartTime()+60` matches, over 5 days |

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

## Status

* [x] `WindRow.qml`, wired into the day list and the hourly list. Syntax
      checked; **not yet seen on the device** -- the N9 was unreachable.
* [x] Plugin source recovered, version pinned, toolchain proven on the
      pristine source.
* [ ] Events view: read the cache, day/hourly tabs, wind on every row.
* [ ] Patch deb.
* [ ] Confirm on the device that openweathermap.org actually fills
      `wind_speed`/`wind_direction` on **hourly** periods -- the hourly rows
      depend on it. `cat ~/.config/com.meecast.omweather/openweathermap.org_*`
