
INSTALL_DIR=/usr/local/
CONFIG_DIR=/etc

FILES = Makefile minfo.c mpi_interface.h padb

minfo.x: minfo.c mpi_interface.h
	cc -g minfo.c -o minfo.x -ldl -Wall

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
	/bin/rm -f padb-2.5.tgz
	/bin/rm -rf padb-2.5
	mkdir padb-2.5
	/bin/cp ${FILES} padb-2.5
	svnversion > padb-2.5/svnversion
	tar -czf padb-2.5.tgz padb-2.5

