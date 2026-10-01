/* vim: set sw=4 ts=4 et: */
/*
 * This file is part of MeeCast
 *
 * Drawing the forecast shown in the Harmattan Events view.
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

/* This half deliberately depends on nothing but QtGui, so the layout can be
 * built and looked at on a desktop without a device or a Harmattan sysroot --
 * see tools/preview.sh. Reading the forecast lives in forecastread.cpp. */

#include "forecastview.h"

#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QPoint>

#include <libintl.h>

#ifndef MEECAST_IMAGES_PATH
#define MEECAST_IMAGES_PATH "/opt/com.meecast.omweather/share/images"
#endif

/* gettext, but never textdomain(): this code is a shared library living inside
 * the home screen process, and switching the process-wide default domain would
 * break translations for every other extension loaded beside it. */
#define TR(s) QString::fromUtf8(dgettext("omweather", s))

namespace ForecastView {

/* -- geometry ------------------------------------------------------------- */

/* The slot the home screen grants this extension is **fixed**:
 * meegotouchhome's theme says
 *
 *   MApplicationExtensionAreaStyle#WeatherExtensionArea {
 *       minimum-size: 12mm 9.6mm; preferred-size: ...; maximum-size: ...;
 *   }
 *
 * which on this screen is 120 x 96 -- measured from inside the widget
 * (see eventslog() in meegotouchplugin.cpp): the image was 480x636, the
 * MImageWidget was 480x636, and the extension around it was 120x96. Minimum
 * equals maximum, so it never grows; anything drawn bigger is simply clipped
 * to the top-left corner. That is why a tile built at 480 px wide showed
 * nothing but a station name and part of one temperature, however much was
 * drawn into it.
 *
 * So the tile is drawn at the size it gets, and the forecast lists live in
 * the notification feed underneath instead, where there is room for them. */
static const int Pad      = 4;
static const int RowH     = 26;

const int Width  = 120;
const int Height = 96;

/* The feed draws this over the user's wallpaper, so the panel stays mostly
 * transparent and leans on light text, the way the stock widget did. */
static const QColor ColorPrimary   = QColor(255, 255, 255);
static const QColor ColorSecondary = QColor(136, 147, 151);

/* -- helpers -------------------------------------------------------------- */

/* Icons would otherwise be re-read and rescaled on every repaint; the lists
 * hold at most a handful of distinct ones. */
static QImage
scaledIcon(const QString& path, int size)
{
    static QHash<QString, QImage> cache;
    const QString key = path + "@" + QString::number(size);
    QHash<QString, QImage>::const_iterator it = cache.constFind(key);
    if (it != cache.constEnd())
        return it.value();

    QImage img;
    if (path.isEmpty() || !img.load(path))
        img = QImage();
    else
        img = img.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    cache.insert(key, img);
    return img;
}

/* Same mapping as WeatherPage.qml's getAngle(): the arrow image points north
 * and the direction names where the wind blows from. */
static qreal
windAngle(const QString& d)
{
    if (d == "S")   return 0;
    if (d == "SSW") return 22.5;
    if (d == "SW")  return 45;
    if (d == "WSW") return 67.5;
    if (d == "W")   return 90;
    if (d == "WNW") return 112.5;
    if (d == "NW")  return 135;
    if (d == "NNW") return 157.5;
    if (d == "N")   return 180;
    if (d == "NNE") return 202.5;
    if (d == "NE")  return 225;
    if (d == "ENE") return 247.5;
    if (d == "E")   return 270;
    if (d == "ESE") return 292.5;
    if (d == "SE")  return 315;
    if (d == "SSE") return 337.5;
    return 0;
}

static void
drawWindArrow(QPainter& p, const QRect& box, const QString& direction)
{
    if (direction.isEmpty())
        return;
    const QImage bg    = scaledIcon(QString(MEECAST_IMAGES_PATH) + "/wind_direction_background.png", box.width());
    const QImage arrow = scaledIcon(QString(MEECAST_IMAGES_PATH) + "/wind_direction_arrow.png", box.width());
    if (!bg.isNull())
        p.drawImage(box.topLeft(), bg);
    if (arrow.isNull())
        return;

    p.save();
    p.translate(box.center().x() + 1, box.center().y() + 1);
    p.rotate(windAngle(direction));
    p.drawImage(QPoint(-arrow.width() / 2, -arrow.height() / 2), arrow);
    p.restore();
}

/* "3 m/s SW". The Beaufort scale is a bare number, as everywhere else in
 * MeeCast. Empty when there is no wind for this period. */
static QString
windText(const QString& speed, const QString& direction, const QString& unit)
{
    QString s;
    if (!speed.isEmpty()) {
        s = speed;
        if (unit != "Beaufort scale")
            s += " " + TR(unit.toUtf8().constData());
    }
    if (!direction.isEmpty()) {
        if (!s.isEmpty())
            s += " ";
        s += TR(direction.toUtf8().constData());
    }
    return s;
}

/* -- the widget ----------------------------------------------------------- */

QImage
render(const Data& data)
{
    QImage image(QSize(Width, Height), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter p;
    p.begin(&image);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (!data.valid) {
        p.setPen(ColorSecondary);
        p.setFont(QFont("Nokia Pure", 12));
        p.drawText(QRect(0, 0, Width, Height), Qt::AlignCenter, TR("No data"));
        p.end();
        return image;
    }

    /* Top: the icon and the temperature. 120 px wide is not much -- the
     * station name does not fit beside them and is dropped; whoever looks
     * here knows which station they set. */
    const QImage icon = scaledIcon(data.currentIconPath, 40);
    if (!icon.isNull())
        p.drawImage(QPoint(Pad, Pad), icon);

    p.setPen(ColorPrimary);
    p.setFont(QFont("Nokia Pure Bold", 22));
    p.drawText(QRect(Pad + 44, Pad, Width - Pad - 44 - Pad, 40),
               Qt::AlignVCenter | Qt::AlignRight, data.currentTemperature);

    /* Then the wind now, and under it the strongest wind of the day: the
     * reading of the moment says nothing about the afternoon, and the
     * afternoon is what one dresses for. */
    const int arrow = 18;
    int y = Pad + 42;

    const QString now = windText(data.currentWindSpeed, data.currentWindDirection,
                                 data.windUnit);
    if (!now.isEmpty()) {
        const QRect box(Pad, y + (RowH - arrow) / 2, arrow, arrow);
        drawWindArrow(p, box, data.currentWindDirection);
        p.setPen(ColorPrimary);
        p.setFont(QFont("Nokia Pure", 12));
        p.drawText(QRect(box.right() + 4, y, Width - box.right() - 4 - Pad, RowH),
                   Qt::AlignVCenter | Qt::AlignLeft, now);
        y += RowH;
    }

    /* Without the unit: it stands on the line above, and the whole line has
     * 112 px. With it, the direction fell off the edge. */
    if (!data.dayWindSpeed.isEmpty()) {
        QString day = TR("Day") + " max " + data.dayWindSpeed;
        if (!data.dayWindDirection.isEmpty())
            day += " " + TR(data.dayWindDirection.toUtf8().constData());
        p.setPen(ColorSecondary);
        p.setFont(QFont("Nokia Pure", 10));
        p.drawText(QRect(Pad, y, Width - 2 * Pad, RowH),
                   Qt::AlignVCenter | Qt::AlignLeft, day);
    }

    p.end();
    return image;
}

} // namespace ForecastView
