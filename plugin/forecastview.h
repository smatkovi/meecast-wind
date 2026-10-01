/* vim: set sw=4 ts=4 et: */
/*
 * This file is part of MeeCast
 *
 * Reading and drawing the forecast shown in the Harmattan Events view.
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
#ifndef FORECASTVIEW_H
#define FORECASTVIEW_H

#include <QString>
#include <QList>
#include <QImage>
#include <QRect>

/* The widget reads the forecast cache itself rather than waiting to be fed
 * over D-Bus. com.meecast.applet's SetCurrentData carries only the current
 * item and no wind at all, so an hourly and a daily list with wind on every
 * line cannot be built from it without changing the interface and rebuilding
 * omweather-qml and predaemon in lockstep. Core parses the very same cache
 * the application shows, which also keeps the two from ever disagreeing.
 */

namespace ForecastView {

    /* One line of the list. Empty wind strings mean the source left this
     * period without wind and the line simply omits it. */
    struct Row {
        QString label;          /* "15:00", or "Sa 20 Sep" */
        QString iconPath;
        QString temperature;    /* already formatted, including the degree sign */
        QString windSpeed;      /* in windUnit, no unit suffix */
        QString windDirection;  /* "SW" */
    };

    struct Data {
        Data() : valid(false) {}

        bool valid;             /* false when no station or no cached forecast */
        QString station;
        QString lastUpdate;
        QString description;
        QString currentTemperature;
        QString currentIconPath;
        QString currentWindSpeed;
        QString currentWindDirection;
        /* The strongest wind of today, over all periods the cache has for it.
         * The current reading alone says nothing about the afternoon, which
         * is the whole reason to look at the wind before leaving. Empty when
         * today has no wind anywhere in the cache. */
        QString dayWindSpeed;
        QString dayWindDirection;
        QString windUnit;       /* "m/s", "km/h", "mi/h" or "Beaufort scale" */
        QList<Row> hours;
        QList<Row> days;
    };

    /* How many lines each list shows. Both are drawn, one under the other:
     * the hours answer "now and the next few hours", the days "the rest of
     * the week", and switching between them with a tab meant the one you
     * wanted was always the one not shown. Five hours is fifteen hours ahead
     * at the three-hour spacing the sources deliver. */
    enum { HourCount = 5, DayCount  = 7 };

    /* Size of the image handed to the Events view. */
    extern const int Width;
    extern const int Height;

    /* Re-reads config.xml and the current station's forecast cache. Safe to
     * call repeatedly: it drops Core's singletons first, so a station or unit
     * changed in the application is picked up. */
    Data read();

    /* Draws the whole widget. Never fails: without data it paints the station
     * name and a short notice, so tapping still opens the application. */
    QImage render(const Data& data);

} // namespace ForecastView

#endif // FORECASTVIEW_H
