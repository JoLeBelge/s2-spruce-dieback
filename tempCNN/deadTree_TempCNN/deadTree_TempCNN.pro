TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
TARGET = tempCNN

CONFIG += console c++17

LIBS +=-lboost_filesystem -lboost_program_options

INCLUDEPATH += /home/jo/app/libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/include/torch/csrc/api/include/
INCLUDEPATH += /home/jo/app/libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/include/

#
LIBS += -L$$PWD/../../../libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/lib/ -ltorch -ltorch_cpu -lc10

SOURCES += \
        main.cpp

HEADERS += \
    model.h




INCLUDEPATH += $$PWD/../../../libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/include
DEPENDPATH += $$PWD/../../../libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/include

Please USE libtorch-cxx11-abi-shared-with-deps

#PRE_TARGETDEPS += $$PWD/../../../libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/lib/libtorch_cpu.so
PRE_TARGETDEPS += $$PWD/../../../libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/lib/libc10.so
PRE_TARGETDEPS += $$PWD/../../../libtorch-cxx11-abi-shared-with-deps-2.6.0+cpu/libtorch/lib/libtorch.so
