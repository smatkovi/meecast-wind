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

static const int Pad      = 12;
static const int HeaderH  = 40;
static const int CurrentH = 104;
static const int SepH     = 2;
static const int TabsH    = 48;
static const int RowH     = 34;

const int Width  = 480;
const int Height = HeaderH + CurrentH + SepH + TabsH + SepH + RowH * RowCount;

static const int TabsTop = HeaderH + CurrentH + SepH;
static const int RowsTop = TabsTop + TabsH + SepH;

/* The feed draws this over the user's wallpaper, so the panel stays mostly
 * transparent and leans on light text, the way the stock widget did. */
static const QColor ColorPrimary   = QColor(255, 255, 255);
static const QColor ColorSecondary = QColor(136, 147, 151);
static const QColor ColorSeparator = QColor(255, 255, 255, 40);
static const QColor ColorTabActive = QColor(255, 255, 255, 28);

QRect
tabRect(int tab)
{
    const int w = Width / TabCount;
    return QRect(tab * w, TabsTop, w, TabsH);
}

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

static void
drawRows(QPainter& p, const QList<Row>& rows, const QString& unit)
{
    const int iconSize  = 26;
    const int arrowSize = 26;

    /* No alternating row shading: a wash light enough not to fight the feed's
     * own backdrop still made the text on every second line read as bold. */
    for (int i = 0; i < rows.size() && i < RowCount; ++i) {
        const int top = RowsTop + i * RowH;

        const Row& r = rows.at(i);

        p.setPen(ColorSecondary);
        p.setFont(QFont("Nokia Pure", 14));
        p.drawText(QRect(Pad, top, 128, RowH), Qt::AlignVCenter | Qt::AlignLeft, r.label);

        const QImage icon = scaledIcon(r.iconPath, iconSize);
        if (!icon.isNull())
            p.drawImage(QPoint(Pad + 136, top + (RowH - icon.height()) / 2), icon);

        p.setPen(ColorPrimary);
        p.setFont(QFont("Nokia Pure", 15));
        p.drawText(QRect(Pad + 170, top, 104, RowH), Qt::AlignVCenter | Qt::AlignRight, r.temperature);

        const QRect arrowBox(Pad + 292, top + (RowH - arrowSize) / 2, arrowSize, arrowSize);
        drawWindArrow(p, arrowBox, r.windDirection);

        p.setPen(ColorSecondary);
        p.setFont(QFont("Nokia Pure", 14));
        p.drawText(QRect(arrowBox.right() + 8, top, Width - arrowBox.right() - 8 - Pad, RowH),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   windText(r.windSpeed, r.windDirection, unit));
    }
}

/* -- the widget ----------------------------------------------------------- */

QImage
render(const Data& data, int activeTab)
{
    QImage image(QSize(Width, Height), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter p;
    p.begin(&image);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    /* Header: station on the left, time of the last update on the right. */
    p.setPen(ColorPrimary);
    p.setFont(QFont("Nokia Pure", 17));
    p.drawText(QRect(Pad, 0, Width - 2 * Pad - 150, HeaderH),
               Qt::AlignVCenter | Qt::AlignLeft,
               data.station.isEmpty() ? TR("Unknown") : data.station);

    p.setPen(ColorSecondary);
    p.setFont(QFont("Nokia Pure", 13));
    p.drawText(QRect(Width - Pad - 150, 0, 150, HeaderH),
               Qt::AlignVCenter | Qt::AlignRight, data.lastUpdate);

    if (!data.valid) {
        p.setPen(ColorSecondary);
        p.setFont(QFont("Nokia Pure", 15));
        p.drawText(QRect(Pad, HeaderH, Width - 2 * Pad, CurrentH),
                   Qt::AlignCenter, TR("No data"));
        p.end();
        return image;
    }

    /* Current conditions. */
    const QImage icon = scaledIcon(data.currentIconPath, 88);
    if (!icon.isNull())
        p.drawImage(QPoint(Pad, HeaderH + (CurrentH - icon.height()) / 2), icon);

    p.setPen(ColorPrimary);
    p.setFont(QFont("Nokia Pure Bold", 32));
    p.drawText(QRect(Pad + 100, HeaderH + 6, 150, 58),
               Qt::AlignVCenter | Qt::AlignLeft, data.currentTemperature);

    p.setPen(ColorSecondary);
    p.setFont(QFont("Nokia Pure", 15));
    p.drawText(QRect(Pad + 100, HeaderH + 64, Width - Pad - 100 - Pad, 32),
               Qt::AlignVCenter | Qt::AlignLeft, data.description);

    const int curArrow = 30;
    const QRect curArrowBox(Width - Pad - 190, HeaderH + 16, curArrow, curArrow);
    drawWindArrow(p, curArrowBox, data.currentWindDirection);
    p.setPen(ColorPrimary);
    p.setFont(QFont("Nokia Pure", 16));
    p.drawText(QRect(curArrowBox.right() + 8, curArrowBox.top(),
                     Width - curArrowBox.right() - 8 - Pad, curArrow),
               Qt::AlignVCenter | Qt::AlignLeft,
               windText(data.currentWindSpeed, data.currentWindDirection, data.windUnit));

    p.fillRect(QRect(Pad, HeaderH + CurrentH, Width - 2 * Pad, SepH), ColorSeparator);

    /* Tabs. */
    const char *labels[TabCount];
    labels[TabHours] = "Hours";
    labels[TabDays]  = "Day";

    for (int t = 0; t < TabCount; ++t) {
        const QRect r = tabRect(t);
        const bool on = (t == activeTab);
        if (on)
            p.fillRect(r, ColorTabActive);

        const QImage tabIcon = scaledIcon(
            QString(MEECAST_IMAGES_PATH) + (t == TabHours ? "/clock.png" : "/day.png"), 28);
        const QString text = TR(labels[t]);

        p.setFont(QFont(on ? "Nokia Pure Bold" : "Nokia Pure", 15));
        const int textW = p.fontMetrics().width(text);
        const int block = (tabIcon.isNull() ? 0 : tabIcon.width() + 8) + textW;
        int x = r.left() + (r.width() - block) / 2;

        if (!tabIcon.isNull()) {
            p.drawImage(QPoint(x, r.top() + (r.height() - tabIcon.height()) / 2), tabIcon);
            x += tabIcon.width() + 8;
        }
        p.setPen(on ? ColorPrimary : ColorSecondary);
        p.drawText(QRect(x, r.top(), textW, r.height()), Qt::AlignVCenter | Qt::AlignLeft, text);

        if (on)
            p.fillRect(QRect(r.left() + 16, r.bottom() - 2, r.width() - 32, 3), ColorPrimary);
    }

    p.fillRect(QRect(Pad, TabsTop + TabsH, Width - 2 * Pad, SepH), ColorSeparator);

    drawRows(p, activeTab == TabDays ? data.days : data.hours, data.windUnit);

    p.end();
    return image;
}

} // namespace ForecastView
