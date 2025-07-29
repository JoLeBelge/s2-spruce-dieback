TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
TARGET = dc_tool
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp

LIBS = -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lzip -fopenmp  -lcurl -lgdal

INCLUDEPATH += /usr/include/gdal/
DEPENDPATH += /usr/include/gdal/

INCLUDEPATH += /usr/include/x86_64-linux-gnu/curl/

INCLUDEPATH += /home/jo/app/force/src/modules/higher-level/
DEPENDPATH += /home/jo/app/force/src/modules/higher-level/
INCLUDEPATH += /home/jo/app/force/src/modules/cross-level/
DEPENDPATH += /home/jo/app/force/src/modules/cross-level/

SOURCES += \
        dc.cpp \
        dcs.cpp \
        main.cpp \
        ../../force/src/modules/higher-level/param-hl.c \

SOURCES +=/home/jo/app/force/src/modules/cross-level/alloc-cl.c\
/home/jo/app/force/src/modules/cross-level/cite-cl.c\
/home/jo/app/force/src/modules/cross-level/cube-cl.c\
/home/jo/app/force/src/modules/cross-level/date-cl.c\
/home/jo/app/force/src/modules/cross-level/datesys-cl.c\
/home/jo/app/force/src/modules/cross-level/dir-cl.c\
/home/jo/app/force/src/modules/cross-level/download-cl.c\
/home/jo/app/force/src/modules/cross-level/enum-cl.c\
/home/jo/app/force/src/modules/cross-level/gdalopt-cl.c\
#/home/jo/app/force/src/modules/cross-level/imagefuns-cl.c\
/home/jo/app/force/src/modules/cross-level/konami-cl.c\
/home/jo/app/force/src/modules/cross-level/lock-cl.c\
/home/jo/app/force/src/modules/cross-level/param-cl.c\
#/home/jo/app/force/src/modules/cross-level/pca-cl.c\
/home/jo/app/force/src/modules/cross-level/quality-cl.c\
/home/jo/app/force/src/modules/cross-level/queue-cl.c\
/home/jo/app/force/src/modules/cross-level/read-cl.c\
#/home/jo/app/force/src/modules/cross-level/stats-cl.c\
/home/jo/app/force/src/modules/cross-level/string-cl.c\
/home/jo/app/force/src/modules/cross-level/sun-cl.c\
/home/jo/app/force/src/modules/cross-level/sys-cl.c\
#/home/jo/app/force/src/modules/cross-level/table-cl.c\
/home/jo/app/force/src/modules/cross-level/tile-cl.c\
/home/jo/app/force/src/modules/cross-level/utils-cl.c\
#/home/jo/app/force/src/modules/cross-level/warp-cl.c

HEADERS +=/home/jo/app/force/src/modules/cross-level/alloc-cl.h\
/home/jo/app/force/src/modules/cross-level/cite-cl.h\
/home/jo/app/force/src/modules/cross-level/const-cl.h\
/home/jo/app/force/src/modules/cross-level/cube-cl.h\
/home/jo/app/force/src/modules/cross-level/date-cl.h\
/home/jo/app/force/src/modules/cross-level/datesys-cl.h\
/home/jo/app/force/src/modules/cross-level/dir-cl.h\
/home/jo/app/force/src/modules/cross-level/download-cl.h\
/home/jo/app/force/src/modules/cross-level/enum-cl.h\
/home/jo/app/force/src/modules/cross-level/gdalopt-cl.h\
#/home/jo/app/force/src/modules/cross-level/imagefuns-cl.h\
/home/jo/app/force/src/modules/cross-level/konami-cl.h\
/home/jo/app/force/src/modules/cross-level/lock-cl.h\
/home/jo/app/force/src/modules/cross-level/param-cl.h\
#/home/jo/app/force/src/modules/cross-level/pca-cl.h\
/home/jo/app/force/src/modules/cross-level/quality-cl.h\
/home/jo/app/force/src/modules/cross-level/queue-cl.h\
/home/jo/app/force/src/modules/cross-level/read-cl.h\
#/home/jo/app/force/src/modules/cross-level/stats-cl.h\
/home/jo/app/force/src/modules/cross-level/string-cl.h\
/home/jo/app/force/src/modules/cross-level/sun-cl.h\
/home/jo/app/force/src/modules/cross-level/sys-cl.h\
#/home/jo/app/force/src/modules/cross-level/table-cl.h\
/home/jo/app/force/src/modules/cross-level/tile-cl.h\
/home/jo/app/force/src/modules/cross-level/utils-cl.h\
#/home/jo/app/force/src/modules/cross-level/warp-cl.h


#SOURCES += $$system(ls /home/jo/app/force/src/modules/cross-level/*.c | grep -v '.*vector-cl.c')
#HEADERS += $$system(ls /home/jo/app/force/src/modules/cross-level/*.h)
# le pipe avec grep permet d'afficher un résultat par ligne et de retirer des fichiers
#ls /home/jo/app/force/src/modules/cross-level/*.c | grep -v '.*vector-.*'


HEADERS += \
    dc.h \
    dcs.h \
    ../../force/src/modules/higher-level/param-hl.h \
