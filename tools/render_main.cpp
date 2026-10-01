/* Renders the Events view widget on the device itself, out of the real
 * forecast cache, and says what it found.
 *
 * The widget lives inside the home screen process, where nothing can be
 * printed and nothing can be looked at. This runs the very same read() and
 * render() as a plain program, so a tile that stays blank can be pinned on
 * one half or the other: no data, or no drawing.
 *
 *   evrender            -> /tmp/meecast-events.png
 */
#include "forecastview.h"

#include <QApplication>
#include <QImage>

#include <stdio.h>

int
main(int argc, char **argv)
{
    QApplication app(argc, argv);
    using namespace ForecastView;

    fprintf(stderr, "== reading the cache\n");
    Data d = read();
    fprintf(stderr, "valid=%d station='%s' lastUpdate='%s'\n",
            (int)d.valid,
            d.station.toUtf8().constData(),
            d.lastUpdate.toUtf8().constData());
    fprintf(stderr, "current='%s' wind='%s %s' dayMax='%s %s' unit='%s'\n",
            d.currentTemperature.toUtf8().constData(),
            d.currentWindSpeed.toUtf8().constData(),
            d.currentWindDirection.toUtf8().constData(),
            d.dayWindSpeed.toUtf8().constData(),
            d.dayWindDirection.toUtf8().constData(),
            d.windUnit.toUtf8().constData());
    fprintf(stderr, "hours=%d days=%d\n", d.hours.size(), d.days.size());

    fprintf(stderr, "== drawing\n");
    const QImage img = render(d);
    fprintf(stderr, "image %dx%d null=%d\n",
            img.width(), img.height(), (int)img.isNull());
    const bool ok = img.save("/tmp/meecast-events.png");
    fprintf(stderr, "saved=%d\n", (int)ok);
    return ok ? 0 : 1;
}
