TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp
QT += x11extras

LIBS += -lboost_program_options -lboost_filesystem -lboost_system -lboost_iostreams -lboost_thread -lzip -ltbb -fopenmp

LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += /usr/include/gdal/
DEPENDPATH += /usr/include/gdal/

INCLUDEPATH += $$PWD/../date/include/date/
DEPENDPATH += $$PWD/../date/include/date/

INCLUDEPATH += $$PWD/../libzipp/src/
DEPENDPATH += $$PWD/../libzipp/src/

INCLUDEPATH += $$PWD/../rapidxml/
DEPENDPATH += $$PWD/../rapidxml/

INCLUDEPATH += $$PWD/../
DEPENDPATH += $$PWD/../

TARGET = s2_oakDieback


SOURCES += \
        main.cpp \
        ../tuiles2OneDate.cpp \
        ../ts1pos.cpp \
        ../catalogue.cpp \
        ../rasterfile.cpp \
        ../libzippp/src/libzippp.cpp \

HEADERS += \
    s2_oakDieback.h \
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
    ../date/include/date/date.h\
    ../tuiles2OneDate.h
