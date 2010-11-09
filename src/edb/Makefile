all: edb

%.o: %.c edb.h
	cc -g -c -o $@ -pthread $<

edb: edb.o xml.o ptrace.o stats_eagle.o stats_falcon.o parallel.o elf.o elf64.o elf32.o
	cc -g -o edb edb.o xml.o ptrace.o stats_eagle.o stats_falcon.o parallel.o elf.o elf64.o elf32.o  -lelan -lpthread

xml.o: xml.c
	cc -c xml.c -o xml.o -pthread

elf32.c: elfN.c
	sed s/TSIZE/32/g elfN.c > $@

elf64.c: elfN.c
	sed s/TSIZE/64/g elfN.c > $@

tags: elf32.c elf64.c *.c *.h
	ctags *.c *.h

clean:
	rm -f *.o edb elf32.c elf64.c 
