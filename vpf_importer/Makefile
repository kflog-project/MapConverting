#
#
#

SHELL = /bin/sh

all:
	g++ -DHAVE_CONFIG_H -I.  -O2 -c -o vpf_importer.o vpf_importer.cpp
	ar cru libvpf.a vpfbase.o value.o table.o tablemgr.o component.o database.o library.o coverage.o feature.o line.o contour.o polygon.o label.o property.o tile.o vpf-summary.o vpf-dump.o vpf-topology.o
	ranlib libvpf.a
	g++  -g -O2   -o vpf_importer  vpf_importer.o libvpf.a -lm 
