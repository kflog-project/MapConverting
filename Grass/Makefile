#
#
#

SHELL = /bin/sh

all: make_iso-bin make_converter

make_iso-bin:
	g++ -L/usr/lib/qt3/lib/ \
	-I. -I/usr/lib/qt3/include/ \
	-o iso-bin iso-bin.cpp \
	-lqt-mt

make_converter:
	g++ -L/usr/lib/qt3/lib/ \
	-I. -I/usr/lib/qt3/include/ \
	-o iso-bin iso-bin.cpp \
	-lqt-mt

make clean:
	rm -f *.o
	rm -f iso-bin
	rm -f converter

