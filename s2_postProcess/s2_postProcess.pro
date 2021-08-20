TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

QT += x11extras

SOURCES += \
        cpostprocess.cpp \
        esoney.cpp \
        main.cpp

# un paramétrage différent pour l'ordi de traitement et le mien
contains(pl,s2jo) {
LIBS += -L$$OUT_PWD/../../micmac/lib/ -lelise
LIBS += -L$$OUT_PWD/../../micmac/lib/ -lANN
PRE_TARGETDEPS += $$OUT_PWD/../../micmac/lib/libelise.a
PRE_TARGETDEPS += $$OUT_PWD/../../micmac/lib/libANN.a

} else {
LIBS += -L$$OUT_PWD/../../../../../micmac/lib/ -lelise
LIBS += -L$$OUT_PWD/../../../../../micmac/lib/ -lANN
PRE_TARGETDEPS += $$OUT_PWD/../../../../../micmac/lib/libelise.a
PRE_TARGETDEPS += $$OUT_PWD/../../../../../micmac/lib/libANN.a
}

LIBS += -lX11 -lboost_program_options -lboost_filesystem

LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += /usr/include/gdal/
DEPENDPATH += /usr/include/gdal/

INCLUDEPATH += $$PWD/../../micmac/include/
DEPENDPATH += $$PWD/../../micmac/include/
INCLUDEPATH += $$PWD/../../../../../micmac/include/
DEPENDPATH += $$PWD/../../../../../micmac/include/

HEADERS += \
    cpostprocess.h \
    esoney.h
