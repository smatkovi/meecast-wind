/* vim: set sw=4 ts=4 et: */
/*
 * This file is part of MeeCast
 *
 * Reading the forecast shown in the Harmattan Events view out of the cache
 * MeeCast Core maintains.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
*/
/*******************************************************************************/

/* Split from forecastview.cpp so that the drawing half depends on nothing but
 * QtGui and can be built and looked at on a desktop; see tools/preview.sh.
 * This half is the one that needs Core. */

#include "forecastview.h"

#include <QDateTime>
#include <QLocale>

#include <libintl.h>
#include <limits.h>
#include <time.h>

#include "config.h"
#include "dataparser.h"
#include "stationlist.h"
#include "station.h"
#include "data.h"

#define DATA_XSD_PATH   "/opt/com.meecast.omweather/share/xsd/data.xsd"
#define CONFIG_XSD_PATH "/opt/com.meecast.omweather/share/xsd/config.xsd"

/* gettext, but never textdomain(): this code is a shared library living inside
 * the home screen process, and switching the process-wide default domain would
 * break translations for every other extension loaded beside it. */
#define TR(s) QString::fromUtf8(dgettext("omweather", s))

namespace ForecastView {

/* Formats one wind speed the way DataItem::wind_speed() does, so the widget
 * and the application never round differently. An empty string means the
 * source gave this period no wind. */
static QString
windSpeedString(Core::Data *d, const std::string& unit)
{
    if (!d)
        return QString();
    d->WindSpeed().units(unit);
    if (d->WindSpeed().value(true) == INT_MAX)
        return QString();
    return QString::number(d->WindSpeed().value(), 'f', 0);
}

static QString
windDirectionString(Core::Data *d)
{
    if (!d)
        return QString();
    QString s = QString::fromUtf8(d->WindDirection().c_str());
    if (s == "N/A")
        return QString();
    return s;
}

static QString
iconPath(Core::Config *config, Core::Data *d)
{
    if (!config || !d)
        return QString();
    return QString::fromUtf8(config->iconspath().c_str()) + "/"
         + QString::fromUtf8(config->iconSet().c_str()) + "/"
         + QString::number(d->Icon()) + ".png";
}

/* "18", or "18/9" when a period only carries a range, as the daily ones do.
 */
static QString
temperatureString(Core::Data *d, const std::string& unit, bool range)
{
    if (!d)
        return QString();
    const QString deg = QString::fromUtf8("°");

    d->temperature().units(unit);
    if (!range && d->temperature().value(true) != INT_MAX)
        return QString::number(d->temperature().value(), 'f', 0) + deg;

    d->temperature_hi().units(unit);
    d->temperature_low().units(unit);
    const bool hasHi  = d->temperature_hi().value(true)  != INT_MAX;
    const bool hasLow = d->temperature_low().value(true) != INT_MAX;

    QString s;
    if (hasHi)
        s = QString::number(d->temperature_hi().value(), 'f', 0) + deg;
    if (hasHi && hasLow)
        s += "/";
    if (hasLow)
        s += QString::number(d->temperature_low().value(), 'f', 0) + deg;

    if (s.isEmpty() && d->temperature().value(true) != INT_MAX)
        s = QString::number(d->temperature().value(), 'f', 0) + deg;
    return s;
}

static Row
makeRow(Core::Config *config, Core::Data *d, const QString& label, bool range)
{
    Row r;
    r.label         = label;
    r.iconPath      = iconPath(config, d);
    r.temperature   = temperatureString(d, config->TemperatureUnit(), range);
    r.windSpeed     = windSpeedString(d, config->WindSpeedUnit());
    r.windDirection = windDirectionString(d);
    return r;
}

Data
read()
{
    Data out;

    /* Drop what Core cached last time: the application may have switched
     * station, changed units or rewritten the forecast since. */
    Core::DataParser::DeleteInstance();
    Core::Config::DeleteInstance();

    Core::Config *config = 0;
    try {
        config = Core::Config::Instance(Core::AbstractConfig::getConfigPath() + "config.xml",
                                        CONFIG_XSD_PATH);
    }
    catch (const std::string&) { return out; }
    catch (const char *)       { return out; }
    catch (...)                { return out; }

    if (!config || config->stationsList().size() == 0)
        return out;
    if (config->current_station_id() < 0 ||
        (unsigned int)config->current_station_id() >= config->stationsList().size())
        return out;

    out.station  = QString::fromUtf8(config->stationname().c_str());
    out.windUnit = QString::fromUtf8(config->WindSpeedUnit().c_str());

    Core::DataParser *dp = 0;
    try {
        dp = Core::DataParser::Instance(
                 config->stationsList().at(config->current_station_id())->fileName(),
                 DATA_XSD_PATH);
    }
    catch (const std::string&) { return out; }
    catch (const char *)       { return out; }
    catch (...)                { return out; }

    if (!dp)
        return out;

    out.lastUpdate = QDateTime::fromTime_t(dp->LastUpdate()).toString("dd MMM hh:mm");

    /* Current conditions. */
    Core::Data *cur = dp->data().GetDataForTime(time(NULL));
    if (cur) {
        out.currentIconPath      = iconPath(config, cur);
        out.currentTemperature   = temperatureString(cur, config->TemperatureUnit(), false);
        out.currentWindSpeed     = windSpeedString(cur, config->WindSpeedUnit());
        out.currentWindDirection = windDirectionString(cur);
        if (cur->Text() != "")
            out.description = QString::fromUtf8(dgettext("omweather", cur->Text().c_str()));
        out.valid = true;
    }

    /* Days and hours are picked exactly as qt-qml/controller.cpp does it, so
     * the widget lists the same periods the application lists.
     *
     * Midnight is the station's midnight, not the phone's: the day rows of a
     * station in another time zone would otherwise be shifted against the
     * application's. The arithmetic below is controller.cpp's, quirks and all
     * -- it converts into the station's zone, truncates the day there, then
     * converts back through the phone's own offset. */
    int timezone = dp->timezone();
    struct tm utc_tm;
    struct tm local_tm;
    time_t now = time(NULL);
    gmtime_r(&now, &utc_tm);
    localtime_r(&now, &local_tm);
    utc_tm.tm_isdst = 0;
    local_tm.tm_isdst = 0;
    const int localtimezone = (mktime(&local_tm) - mktime(&utc_tm)) / 3600;

    time_t midnight = time(NULL) + 3600 * timezone;
    struct tm *tmp = gmtime(&midnight);
    tmp->tm_sec = 0; tmp->tm_min = 0; tmp->tm_hour = 0;
    tmp->tm_isdst = 0;
    midnight = mktime(tmp) - 3600 * timezone + 3600 * localtimezone;

    for (int day = 0; day < RowCount; ++day) {
        const time_t t = midnight + 15 * 3600 + 1 + (time_t)day * 24 * 3600;
        Core::Data *d = dp->data().GetDataForTime(t);
        if (!d)
            continue;
        QDateTime dt = QDateTime::fromTime_t(d->StartTime());
        const QString label = (day == 0)
            ? TR("Today")
            : QLocale().toString(dt, "ddd") + " " + QLocale().toString(dt, "dd MMM");
        out.days.append(makeRow(config, d, label, true));
        out.valid = true;
    }

    now = time(NULL);
    tmp = localtime(&now);
    tmp->tm_sec = 0; tmp->tm_min = 1; tmp->tm_isdst = 1;
    const time_t currentHour = mktime(tmp);   /* the phone's hour, as the app does */

    for (int i = 0; out.hours.size() < RowCount && i < 5 * 24 * 3600; i += 3600) {
        Core::Data *d = dp->data().GetDataForTime(currentHour + i, true);
        if (!d || d->StartTime() + 60 != currentHour + i)
            continue;
        QDateTime dt = QDateTime::fromTime_t(d->StartTime());
        out.hours.append(makeRow(config, d, dt.toString("hh:mm"), false));
        out.valid = true;
    }

    return out;
}

} // namespace ForecastView
