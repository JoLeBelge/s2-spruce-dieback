TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
TARGET = s2_timeSerie
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp

#QMAKE_CXXFLAGS = -Wno-c++11-narrowing

SOURCES += main.cpp \
    catalogue.cpp \
    cataloguesco.cpp \
    rasterfile.cpp \
    ts1pos.cpp \
    libzippp/src/libzippp.cpp \
    tuiles2OneDate.cpp \ \
    tuiles2onedatesco.cpp

LIBS = -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lzip -ltbb -fopenmp

LIBS += -L$$PWD/../../../usr/include/ -lsqlite3
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += /usr/include/gdal/
DEPENDPATH += /usr/include/gdal/

INCLUDEPATH += $$PWD/libzipp/src/
DEPENDPATH += $$PWD/libzipp/src/

INCLUDEPATH += $$PWD/rapidxml/
DEPENDPATH += $$PWD/rapidxml/

INCLUDEPATH += $$PWD/date/include/date/
DEPENDPATH += $$PWD/date/include/date/

HEADERS += \
    catalogue.h \
    cataloguesco.h \
    rapidjson/allocators.h \
    rapidjson/cursorstreamwrapper.h \
    rapidjson/document.h \
    rapidjson/encodedstream.h \
    rapidjson/encodings.h \
    rapidjson/filereadstream.h \
    rapidjson/filewritestream.h \
    rapidjson/fwd.h \
    rapidjson/istreamwrapper.h \
    rapidjson/memorybuffer.h \
    rapidjson/memorystream.h \
    rapidjson/ostreamwrapper.h \
    rapidjson/pointer.h \
    rapidjson/prettywriter.h \
    rapidjson/rapidjson.h \
    rapidjson/reader.h \
    rapidjson/schema.h \
    rapidjson/stream.h \
    rapidjson/stringbuffer.h \
    rapidjson/writer.h \
    rapidxml/rapidxml.hpp \
    rapidxml/rapidxml_iterators.hpp \
    rasterfile.h \
    ts1pos.h \
    libzippp/src/libzippp.h \
    date.h\
    tuiles2OneDate.h \ \
    tuiles2onedatesco.h
