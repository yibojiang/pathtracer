CONFIG += qt
QMAKE_CC =/usr/local/bin/clang-omp
QMAKE_CXX =/usr/local/bin/clang-omp++
QMAKE_CXXFLAGS+= -fopenmp
QMAKE_LINK = /usr/local/bin/clang-omp++
LIBS += -fopenmp
HEADERS += src/*.h
SOURCES += src/*.cpp
