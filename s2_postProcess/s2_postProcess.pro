TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

QT += x11extras

SOURCES += \
        cpostprocess.cpp \
        main.cpp
LIBS += -L$$OUT_PWD/../../../../../micmac/lib/ -lelise
LIBS += -L$$OUT_PWD/../../../../../micmac/lib/ -lANN
LIBS += -lX11 -lboost_program_options -lboost_filesystem

LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/usr/include/gdal/
DEPENDPATH += $$PWD/usr/include/gdal/

INCLUDEPATH += $$PWD/../../../../../micmac/include/
DEPENDPATH += $$PWD/../../../../../micmac/include/

PRE_TARGETDEPS += $$OUT_PWD/../../../../../micmac/lib/libelise.a
PRE_TARGETDEPS += $$OUT_PWD/../../../../../micmac/lib/libANN.a

HEADERS += \
    cpostprocess.h
