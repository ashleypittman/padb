


%.o: %.c edb.h
	cc -c -o $@ -pthread $<

edb: edb.o
	cc -o edb edb.o xml.o

xml.o: xml.c
	cc -c xml.c -o xml.o -pthread

elf32.c: elfN.c
	sed s/TSIZE/32/g edb/elfN.c > $@

elf64.c: elfN.c
	sed s/TSIZE/64/g edb/elfN.c > $@

