
FILES = Makefile minfo.c mpi_interface.h padb

minfo.x: minfo.c mpi_interface.h
	cc -g minfo.c -o minfo.x -ldl -Wall

clean:
	/bin/rm -f minfo.x

tarfile:
	/bin/rm -f padb-2.5.tgz
	/bin/rm -rf padb-2.5
	mkdir padb-2.5
	/bin/cp ${FILES} padb-2.5
	svnversion > padb-2.5/svnversion
	tar -czf padb-2.5.tgz padb-2.5

