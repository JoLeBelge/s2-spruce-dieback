TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
TARGET = dc_tool
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp

LIBS = -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lgdal -lcurl#-fopenmp

INCLUDEPATH += /usr/include/gdal/
DEPENDPATH += /usr/include/gdal/

INCLUDEPATH += /usr/include/x86_64-linux-gnu/curl/

#qmake pl=scotty
!contains( pl, scotty ) {
    # drivers contains 'network'
    message( "Configuring for  jo build..." )
   USER = "jo"
} else {
 USER = "grf"
}



INCLUDEPATH += /home/$$USER/app/force/src/modules/higher-level/
DEPENDPATH += /home/$$USER/app/force/src/modules/higher-level/
INCLUDEPATH += /home/$$USER/app/force/src/modules/cross-level/
DEPENDPATH += /home/$$USER/app/force/src/modules/cross-level/

SOURCES += \
        dc.cpp \
        dcs.cpp \
        main.cpp \
        ../../force/src/modules/higher-level/param-hl.c \

SOURCES +=/home/$$USER/app/force/src/modules/cross-level/alloc-cl.c\
/home/$$USER/app/force/src/modules/cross-level/cite-cl.c\
/home/$$USER/app/force/src/modules/cross-level/cube-cl.c\
/home/$$USER/app/force/src/modules/cross-level/date-cl.c\
/home/$$USER/app/force/src/modules/cross-level/datesys-cl.c\
/home/$$USER/app/force/src/modules/cross-level/dir-cl.c\
/home/$$USER/app/force/src/modules/cross-level/download-cl.c\
/home/$$USER/app/force/src/modules/cross-level/enum-cl.c\
/home/$$USER/app/force/src/modules/cross-level/gdalopt-cl.c\
#/home/$$USER/app/force/src/modules/cross-level/imagefuns-cl.c\
/home/$$USER/app/force/src/modules/cross-level/konami-cl.c\
/home/$$USER/app/force/src/modules/cross-level/lock-cl.c\
/home/$$USER/app/force/src/modules/cross-level/param-cl.c\
#/home/$$USER/app/force/src/modules/cross-level/pca-cl.c\
/home/$$USER/app/force/src/modules/cross-level/quality-cl.c\
/home/$$USER/app/force/src/modules/cross-level/queue-cl.c\
/home/$$USER/app/force/src/modules/cross-level/read-cl.c\
#/home/$$USER/app/force/src/modules/cross-level/stats-cl.c\
/home/$$USER/app/force/src/modules/cross-level/string-cl.c\
/home/$$USER/app/force/src/modules/cross-level/sun-cl.c\
/home/$$USER/app/force/src/modules/cross-level/sys-cl.c\
#/home/$$USER/app/force/src/modules/cross-level/table-cl.c\
/home/$$USER/app/force/src/modules/cross-level/tile-cl.c\
/home/$$USER/app/force/src/modules/cross-level/utils-cl.c\
#/home/$$USER/app/force/src/modules/cross-level/warp-cl.c

HEADERS +=/home/$$USER/app/force/src/modules/cross-level/alloc-cl.h\
/home/$$USER/app/force/src/modules/cross-level/cite-cl.h\
/home/$$USER/app/force/src/modules/cross-level/const-cl.h\
/home/$$USER/app/force/src/modules/cross-level/cube-cl.h\
/home/$$USER/app/force/src/modules/cross-level/date-cl.h\
/home/$$USER/app/force/src/modules/cross-level/datesys-cl.h\
/home/$$USER/app/force/src/modules/cross-level/dir-cl.h\
/home/$$USER/app/force/src/modules/cross-level/download-cl.h\
/home/$$USER/app/force/src/modules/cross-level/enum-cl.h\
/home/$$USER/app/force/src/modules/cross-level/gdalopt-cl.h\
#/home/$$USER/app/force/src/modules/cross-level/imagefuns-cl.h\
/home/$$USER/app/force/src/modules/cross-level/konami-cl.h\
/home/$$USER/app/force/src/modules/cross-level/lock-cl.h\
/home/$$USER/app/force/src/modules/cross-level/param-cl.h\
#/home/$$USER/app/force/src/modules/cross-level/pca-cl.h\
/home/$$USER/app/force/src/modules/cross-level/quality-cl.h\
/home/$$USER/app/force/src/modules/cross-level/queue-cl.h\
/home/$$USER/app/force/src/modules/cross-level/read-cl.h\
#/home/$$USER/app/force/src/modules/cross-level/stats-cl.h\
/home/$$USER/app/force/src/modules/cross-level/string-cl.h\
/home/$$USER/app/force/src/modules/cross-level/sun-cl.h\
/home/$$USER/app/force/src/modules/cross-level/sys-cl.h\
#/home/$$USER/app/force/src/modules/cross-level/table-cl.h\
/home/$$USER/app/force/src/modules/cross-level/tile-cl.h\
/home/$$USER/app/force/src/modules/cross-level/utils-cl.h\
#/home/$$USER/app/force/src/modules/cross-level/warp-cl.h


#SOURCES += $$system(ls /home/$$USER/app/force/src/modules/cross-level/*.c | grep -v '.*vector-cl.c')
#HEADERS += $$system(ls /home/$$USER/app/force/src/modules/cross-level/*.h)
# le pipe avec grep permet d'afficher un résultat par ligne et de retirer des fichiers
#ls /home/$$USER/app/force/src/modules/cross-level/*.c | grep -v '.*vector-.*'


HEADERS += \
    dc.h \
    dcs.h \
    ../../force/src/modules/higher-level/param-hl.h \
