# Unix/Linux makefile for GPC 2.31
#
# Riley Rainey  (riley.rainey@websimulations.com)

CFLAGS = -O -g

libgenpolyclip.a: gpc.o
	rm -f $@
	ar crs $@ $<
	ranlib $@

clean:
	rm -f libgenpolyclip.a *.o core *~

install: libgenpolyclip.a
	-mkdir -p /usr/local/lib
	-mkdir -p /usr/local/include
	install libgenpolyclip.a /usr/local/lib/libgenpolyclip.a
	install gpc.h /usr/local/include/gpc.h

