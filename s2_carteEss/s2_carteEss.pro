TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp
QT += x11extras

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

LIBS += -lX11 -lboost_program_options -lboost_filesystem -lboost_system -lboost_iostreams -lboost_thread -lzip -ltbb -fopenmp

LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += /usr/include/gdal/
DEPENDPATH += /usr/include/gdal/

INCLUDEPATH += $$PWD/../../micmac/include/
DEPENDPATH += $$PWD/../../micmac/include/
INCLUDEPATH += $$PWD/../../../../../micmac/include/
DEPENDPATH += $$PWD/../../../../../micmac/include/

INCLUDEPATH += $$PWD/../date/include/date/
DEPENDPATH += $$PWD/../date/include/date/

INCLUDEPATH += $$PWD/../libzipp/src/
DEPENDPATH += $$PWD/../libzipp/src/

INCLUDEPATH += $$PWD/../rapidxml/
DEPENDPATH += $$PWD/../rapidxml/

INCLUDEPATH += $$PWD/../
DEPENDPATH += $$PWD/../





TARGET = s2_carteEss


SOURCES += \
        main.cpp \
        ../tuiles2OneDate.cpp \
        ../catalogueperiodpheno.cpp \
        ../ts1pos.cpp \
        ../catalogue.cpp \
        ../rasterfile.cpp \
        ../libzippp/src/libzippp.cpp \
        tuiles2onedatepheno.cpp

HEADERS += \
    carteEss.h \
    ../catalogue.h \
    ../rapidjson/allocators.h \
    ../rapidjson/cursorstreamwrapper.h \
    ../rapidjson/document.h \
    ../rapidjson/encodedstream.h \
    ../rapidjson/encodings.h \
    ../rapidjson/filereadstream.h \
    ../rapidjson/filewritestream.h \
    ../rapidjson/fwd.h \
    ../rapidjson/istreamwrapper.h \
    ../rapidjson/memorybuffer.h \
    ../rapidjson/memorystream.h \
    ../rapidjson/ostreamwrapper.h \
    ../rapidjson/pointer.h \
    ../rapidjson/prettywriter.h \
    ../rapidjson/rapidjson.h \
    ../rapidjson/reader.h \
    ../rapidjson/schema.h \
    ../rapidjson/stream.h \
    ../rapidjson/stringbuffer.h \
    ../rapidjson/writer.h \
    ../rapidxml/rapidxml.hpp \
    ../rapidxml/rapidxml_iterators.hpp \
    ../rasterfile.h \
    ../libzippp/src/libzippp.h \
    ../ts1pos.h \
    ../date.h\
    ../tuiles2OneDate.h \
    ../catalogueperiodpheno.h  \
    tuiles2onedatepheno.h \

