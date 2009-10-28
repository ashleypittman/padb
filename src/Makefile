
INSTALL_DIR=/usr/local/
CONFIG_DIR=/etc
VERSION=3.0-rc1
CC=gcc
CFLAGS=-Wall -g

FILES = Makefile minfo.c mpi_interface.h padb

minfo.x: minfo.c mpi_interface.h
	$(CC) minfo.c -o minfo.x -ldl $(CFLAGS)

install: minfo.x
	/bin/mkdir -p ${INSTALL_DIR}/bin
	/bin/cp minfo.x ${INSTALL_DIR}/bin/
	/bin/cp padb ${INSTALL_DIR}/bin/

make config_install:
	/bin/mkdir -p ${CONFIG_DIR}
	/bin/cp padb.conf ${CONFIG_DIR}/

clean:
	/bin/rm -f minfo.x

tarfile:
	/bin/rm -f padb-${VERSION}.tgz
	/bin/rm -rf padb-${VERSION}
	mkdir padb-${VERSION}
	/bin/cp ${FILES} padb-${VERSION}
	svnversion > padb-${VERSION}/svnversion
	tar -czf padb-${VERSION}.tgz padb-${VERSION}

tidy:
	perltidy -b -ce -w -se padb

pc:	padb
	perlcritic --brutal --verbose "%l: (%s) %m\n" padb > .pc.tmp || true
	/bin/mv .pc.tmp pc

report: pc
	./report.pl pc | tee report

