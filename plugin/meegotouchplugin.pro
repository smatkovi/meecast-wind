
QT += declarative network

TEMPLATE = lib
#TEMPLATE = app
CONFIG += plugin \
    dbus \
    qdbus \
    gui \
    link_pkgconfig \
    mobility \
    meegotouch

MOBILITY = publishsubscribe

HEADERS = meegotouchplugin.h dbusadaptor.h eventfeedif.h  weatherdataif.h forecastview.h
SOURCES = meegotouchplugin.cpp dbusadaptor.cpp eventfeedif.cpp  weatherdataif.cpp forecastview.cpp forecastread.cpp

# The Events view reads the forecast cache itself, so it needs Core -- the same
# static library and the same dependencies predaemon links against.
QT += xml
INCLUDEPATH += ../core
LIBS += ../core/libomweather-core.a
PKGCONFIG += libcurl
PKGCONFIG += sqlite3


TARGET = $$qtLibraryTarget(events-meecast)
#TARGET = test
DESTDIR = lib
target.path += /usr/lib/meegotouch/applicationextensions/ 
INSTALLS += target desktop_entry applet package datasmallcontour

#desktop_entry.path =  /usr/share/meegotouch/applicationextensions/ 
desktop_entry.path =  /opt/com.meecast.omweather/share/applet 
desktop_entry.files = *.desktop


applet.path =  /opt/com.meecast.omweather/share/omweather/qml 
applet.files = *.qml

package.path =  /opt/com.meecast.omweather/share/packages
package.files = *.deb

datasmallcontour.files += data/smallcontour/*.png
datasmallcontour.path = /opt/com.meecast.omweather/share/images/smallcontour

contextreg.files = data/meecast.context
contextreg.path = /usr/share/contextkit/providers
INSTALLS += contextreg

# mmoc is not given the sysroot prefix that the g++ wrapper applies to
# -I/usr/include/meegotouch, and then fails with "Undefined interface".
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/../meegotouch
