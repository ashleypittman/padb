bin_SCRIPTS = padb
libexec_PROGRAMS = minfo
minfo_CFLAGS = -ldl
minfo_SOURCES = minfo.c mpi_interface.h
EXTRA_DIST = padb

tidy:
	perltidy -b -ce -w -se padb

pc:	padb
	perlcritic --brutal --verbose "%l: (%s) %m\n" padb > .pc.tmp || true
	/bin/mv .pc.tmp pc

report: pc
	./report.pl pc | tee report
