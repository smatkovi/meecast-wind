/* Renders the Events view widget to a PNG on a desktop, with made-up data.
 *
 * forecastview.cpp depends on nothing but QtGui, so this builds the very code
 * that ships and shows what the widget will actually look like -- the Nokia N9
 * is the only place it can otherwise be seen, and the Qt Simulator cannot host
 * a MeeGoTouch application extension at all.
 *
 * Fonts and the exact metrics will differ from the device ("Nokia Pure" is not
 * installed here), so this checks composition and alignment, not pixels.
 *
 *   preview <images-dir> <iconset-dir> <out-prefix>
 */
#include "forecastview.h"

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <cstdio>

using namespace ForecastView;

static Row
row(const QString& label, const QString& iconset, int icon,
    const QString& temp, const QString& speed, const QString& dir)
{
    Row r;
    r.label         = label;
    r.iconPath      = iconset + "/" + QString::number(icon) + ".png";
    r.temperature   = temp;
    r.windSpeed     = speed;
    r.windDirection = dir;
    return r;
}

static Data
sampleData(const QString& iconset)
{
    Data d;
    d.valid                = true;
    d.station              = "Wien";
    d.lastUpdate           = "20 Sep 14:32";
    d.description          = "leicht bewolkt";
    d.currentTemperature   = QString::fromUtf8("18°");
    d.currentIconPath      = iconset + "/3.png";
    d.currentWindSpeed     = "3";
    d.currentWindDirection = "SW";
    d.dayWindSpeed         = "7";
    d.dayWindDirection     = "NNE";
    d.windUnit             = "m/s";

    const char *hours[] = {"15:00","16:00","17:00","18:00","19:00","20:00","21:00","22:00"};
    const char *htemp[] = {"18","17","16","15","14","13","13","12"};
    const char *hspd[]  = {"3","4","4","5","5","6","6","5"};
    const char *hdir[]  = {"SW","SW","W","W","W","W","WNW","NW"};
    for (int i = 0; i < 8; ++i)
        d.hours.append(row(hours[i], iconset, 3 + i % 5,
                           QString::fromUtf8(htemp[i]) + QString::fromUtf8("°"),
                           hspd[i], hdir[i]));

    const char *days[]  = {"Today","Sun 21 Sep","Mon 22 Sep","Tue 23 Sep",
                           "Wed 24 Sep","Thu 25 Sep","Fri 26 Sep","Sat 27 Sep"};
    const char *dtemp[] = {"19/9","21/11","20/12","17/10","15/8","16/7","18/9","20/11"};
    const char *dspd[]  = {"3","5","6","4","7","2","3","4"};
    const char *ddir[]  = {"SW","W","NW","N","NNE","E","SSE","S"};
    for (int i = 0; i < 8; ++i)
        d.days.append(row(QString::fromUtf8(days[i]), iconset, 2 + i % 6,
                          QString::fromUtf8(dtemp[i]).replace("/", QString::fromUtf8("°/")) + QString::fromUtf8("°"),
                          dspd[i], ddir[i]));
    return d;
}

/* The feed is a dark surface; flatten the transparent widget onto one so the
 * PNG shows what the eye will see rather than a checkerboard. */
static void
save(const QImage& widget, const QString& path)
{
    QImage out(widget.size(), QImage::Format_RGB32);
    out.fill(QColor(20, 20, 22));
    QPainter p(&out);
    p.drawImage(0, 0, widget);
    p.end();
    out.save(path);
}

int
main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const QStringList a = app.arguments();
    if (a.size() < 4) {
        fprintf(stderr, "usage: preview <images-dir> <iconset-dir> <out-prefix>\n");
        return 2;
    }
    const QString iconset = a.at(2);
    const QString prefix  = a.at(3);

    const Data d = sampleData(iconset);
    save(render(d), prefix + "-full.png");

    Data empty;
    empty.station = "Wien";
    save(render(empty), prefix + "-nodata.png");

    fprintf(stderr, "widget is %dx%d\n", Width, Height);
    return 0;
}
