#
#
#

SHELL = /bin/sh

all: make_genoclip make_bin make_kacheln

make_genoclip:
	cd gpc && make

make_bin:
	g++ -L. -L/usr/lib/qt3/lib/ \
		-I. -I/usr/lib/qt3/include/ \
		-o map-bin map-bin.cpp \
		-lqt-mt


make_kacheln:
	g++ -L. -L/usr/lib/qt3/lib/ \
		-L ./gpc/ \
		-I. -I/usr/lib/qt3/include/ \
		-o vmap_kacheln vmap_kacheln.cpp \
		-lqt-mt -lgenpolyclip

clean:
	rm -f *.o
	rm -f map-bin
	rm -f vmap_kacheln
	cd gpc && make clean


