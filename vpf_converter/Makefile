#
#
#

SHELL = /bin/sh

all: make_bin make_kacheln

make_bin:
	g++ -L. -L/usr/lib/qt3/lib/ \
		-I. -I/usr/lib/qt3/include/ \
		-o map-bin map-bin.cpp \
		-lqt-mt


make_kacheln:
	g++ -L. -L/usr/lib/qt3/lib/ \
		-I. -I/usr/lib/qt3/include/ \
		-o vmap_kacheln vmap_kacheln.cpp \
		-lqt-mt -lgenpolyclip

