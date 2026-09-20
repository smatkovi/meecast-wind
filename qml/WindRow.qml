//import QtQuick 1.1
import Qt 4.7

/* Wind speed and direction for a single forecast row.
 *
 * The day list on WeatherPage and the hourly list on FullWeatherPage have no
 * room left on the line that carries the date, the icon and the temperatures,
 * so this goes underneath it as a second, quieter line:
 *
 *     [arrow] 3 m/s SW
 *
 * speed is whatever Core has already converted to Config.windspeedunit, so
 * picking "m/s" under Settings > Units is what makes this read in m/s. The
 * Beaufort scale is a bare number and gets no unit appended, exactly as the
 * current-conditions block on WeatherPage does it.
 *
 * A source that leaves a period without wind renders nothing at all rather
 * than a row of "N/A".
 */
Row {
    id: windrow

    property string speed: "N/A"
    property string direction: "N/A"
    property int arrowSize: 24
    property int pointSize: 14
    property color textColor: "#889397"

    property bool hasSpeed: (speed != "" && speed != "N/A")
    property bool hasDirection: (direction != "" && direction != "N/A")

    /* Same mapping as WeatherPage.getAngle(): the arrow image points north,
     * and the direction names where the wind blows from. */
    function getAngle(s)
    {
        switch (s){
        case 'S':   return 0;
        case 'SSW': return 22.5;
        case 'SW':  return 45;
        case 'WSW': return (45+22.5);
        case 'W':   return 90;
        case 'WNW': return (90+22.5);
        case 'NW':  return (90+45);
        case 'NNW': return (180-22.5);
        case 'N':   return 180;
        case 'NNE': return (180+22.5);
        case 'NE':  return (180+45);
        case 'ENE': return (270-22.5);
        case 'E':   return 270;
        case 'ESE': return (270+22.5);
        case 'SE':  return (270+45);
        case 'SSE': return (360-22.5);
        }
        return 0;
    }

    visible: hasSpeed || hasDirection
    height: arrowSize
    spacing: 6

    Item {
        width: windrow.arrowSize
        height: windrow.arrowSize
        visible: windrow.hasDirection

        Image {
            anchors.fill: parent
            source: Config.imagespath + "/wind_direction_background.png"
            smooth: true
        }
        Image {
            anchors.fill: parent
            source: Config.imagespath + "/wind_direction_arrow.png"
            smooth: true
            transform: Rotation {
                origin.x: windrow.arrowSize / 2
                origin.y: windrow.arrowSize / 2
                angle: windrow.getAngle(windrow.direction)
            }
        }
    }

    Text {
        anchors.verticalCenter: parent.verticalCenter
        visible: windrow.hasSpeed
        color: windrow.textColor
        font.pointSize: windrow.pointSize
        text: (Config.windspeedunit == "Beaufort scale")
              ? windrow.speed
              : windrow.speed + ' ' + Config.tr(Config.windspeedunit)
    }

    Text {
        anchors.verticalCenter: parent.verticalCenter
        visible: windrow.hasDirection
        color: windrow.textColor
        font.pointSize: windrow.pointSize
        text: Config.tr(windrow.direction)
    }
}
