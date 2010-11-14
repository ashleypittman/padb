/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "elfN.c,v 1.14 2005/11/03 11:23:04 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/elfN.c,v */

/*
 * This file contains the routines needed to grup around inside 
 * elf data structures.  It is compiled and linked *twice* during
 * the build to allow both thirty two and sixty four bit elf objects
 * to be dealt with by the same binary.
 *
 * To do this this file is copied (by sed) during the build procedure
 * so be aware that if you edit the copy and recompile your changes
 * will get lost.
 *
 * The correct file to edit is edb/elfN.c  When the file is "copied"
 * all instances of "TSIZE" are replaced with "32" or "64".
 *
 * There is then a rather bizarre test to detect which file is being
 * compiled and change some defines accordingly.
 * 
 * Note also that this support isn't complete yet.
 * 
 */

#include <unistd.h>

#include "edb.h"

#include <elan4/library.h>

#include <elf.h>
#include <link.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined (LINUX_IA64)
#define START_POINT (void *)0x4000000000000000
#elif defined (LINUX_I386)
#define START_POINT (void *)0x08048000
#elif defined (LINUX_X86_64)
/* I don't know when this changed but apparantly it has
 * (GNAT:7840) showed a failure in this code and changing
 * this value fixed it.  Possibly it's kernel version
 * and possibly exec-shield will randomise this value in
 * the future anyway. */
#if 0
#define START_POINT (void *)0x40000000
#else
#define START_POINT (void *)0x400000
#endif
#else
#error Unknown arch, cant continue
#endif

#if TSIZE < 33
// #warning compiling the thirty two bit version
#else
// #warning compiling the sixty four bit version
#endif

#define TARGET_TYPE uintTSIZE_t
#define TARGET_PTR "%#"PRIxTSIZE
/***********************************************************
 * Core file parsing code...                               *
 ***********************************************************/


static uint64_t
find_sym_in_tablesTSIZE (struct etrace_ops *ops,
		      struct link_map *map,
		      uint64_t hash,
		      int nchains,
		      uint64_t symtab,
		      uint64_t strtab,
		      char *sym_name)
{
    ElfTSIZE_Sym sym;
    ElfTSIZE_Sym *sym_p;
    char str[128];
    int i = 0;
    int check;
    
    if ( verbose > 3 )
	printf("New table mapped at %#"PRIx64" containing %d chains\n",strtab,nchains);
    if (hash) {
	while (i < nchains) {
	
	    if ( ops->rcopy(ops->handle,
			    (uint64_t)(uintptr_t)((unsigned long)symtab + (i * sizeof(ElfTSIZE_Sym))),
			    &sym,
			    sizeof(ElfTSIZE_Sym)) == -1 ) {
		printf("Failed to read from process\n");
		return 0;
	    }
	    
	    i++;
	    check = 0;
		
	    if ( sym.st_info == (unsigned char)ELFTSIZE_ST_INFO(STB_GLOBAL,STT_OBJECT))
		check = 1;
	
	    if ( sym.st_value == 6 )
		check = 0;
	
	    if ( ELFTSIZE_ST_TYPE(sym.st_info) == (unsigned char)STT_FILE )
		check = 0;
	
	    if ( check ) {
		/* read symbol name from the string table */
	    
//	    str = (char *) strtab + sym.st_name;
	    
		if ( fetch_string(ops,&str[0],strtab + sym.st_name,128) == -1 ) {
		    if ( verbose > 2 )
			printf("Failed to find string, returning nothing\n");
		    return 0;
		}
	    
		if ( verbose > 2 )
		    printf("String is type %x bind %x other %x size %x value "TARGET_PTR" shndx %x name %lx '%s'\n",
			   (int) ELFTSIZE_ST_TYPE(sym.st_info),
			   (int) ELFTSIZE_ST_BIND(sym.st_info),
			   (int) sym.st_other,
			   (int) sym.st_size,
			   sym.st_value,
			   (int) sym.st_shndx,
			   (long) sym.st_name,
			   str);
		
		/* compare it with our symbol*/
		if (strcmp(&str[0], sym_name) == 0) {
		    
		    if ( verbose > 2 )
			printf("\nSuccess: got it %s "TARGET_PTR" "TARGET_PTR" "TARGET_PTR"\n",
			       &str[0],
			       (TARGET_TYPE)map->l_addr,
			       sym.st_value,
			       (TARGET_TYPE)map->l_addr + sym.st_value);
		    
		    if ( sym.st_value ) {
			return (uint64_t)(map->l_addr + sym.st_value);
		    }
		}
	    }
	}
	
	if ( verbose > 2 )
	    printf("Found nothing\n");
	
	/* no symbol found, return 0 */
	return 0;
    } else {
	/* If there is no DT_HASH, we can still walk the symbol table linearly until we find what we're after
	   or we get an invalid memory reference */
         /* First entry is always blank, second one always has no name */
	sym_p = (void *)symtab;
	sym_p += 2;

	if ( ops->rcopy(ops->handle, (uint64_t)(uintptr_t)sym_p,
	                &sym, sizeof(ElfTSIZE_Sym)) == -1 ) {
	    printf("Failed to read from process\n");
	    return 0;
	}

	while (sym.st_name != 0) {
	    int check = 0;
	    if ( sym.st_info == (unsigned char)ELFTSIZE_ST_INFO(STB_GLOBAL,STT_OBJECT))
		check = 1;
	
	    if ( sym.st_value == 6 )
		check = 0;
	
	    if ( ELFTSIZE_ST_TYPE(sym.st_info) == (unsigned char)STT_FILE )
		check = 0;

	    if ( check ) {
		if ( fetch_string(ops,&str[0],strtab + sym.st_name,128) == -1 ) {
		    if ( verbose > 2 )
			printf("Failed to find string, returning nothing\n");
		    return 0;
		}

		if ( verbose > 2 )
		    printf("String is type %x bind %x other %x size %x value "TARGET_PTR" shndx %x name %lx '%s'\n",
			    (int) ELFTSIZE_ST_TYPE(sym.st_info),
			    (int) ELFTSIZE_ST_BIND(sym.st_info),
			    (int) sym.st_other,
			    (int) sym.st_size,
			    sym.st_value,
			    (int) sym.st_shndx,
			    (long) sym.st_name,
			    str);

		/* compare it with our symbol*/
		if (strcmp(&str[0], sym_name) == 0) {

		    if ( verbose > 2 )
			printf("\nSuccess: got it %s "TARGET_PTR" "TARGET_PTR" "TARGET_PTR"\n",
				&str[0],
				(TARGET_TYPE)map->l_addr,
				sym.st_value,
				(TARGET_TYPE)map->l_addr + sym.st_value);

		    if ( sym.st_value ) {
			return (uint64_t)(map->l_addr + sym.st_value);
		    }
		}
	    }

	    sym_p++;
	    if ( ops->rcopy(ops->handle, (uint64_t)(uintptr_t)sym_p,
			&sym, sizeof(ElfTSIZE_Sym)) == -1 ) {
		printf("Failed to read from process\n");
		return 0;
	    }
	}

	return 0;
    }
}

static void
readSegmentDataTSIZE (struct cproc *cproc, int segNo, size_t offset, char *space, size_t size)
{
    ElfTSIZE_Phdr  *p = &cproc->corePhdrTSIZE[segNo];
    size_t fileOffset = cproc->corePhdrTSIZE[segNo].p_offset + offset;
    ssize_t bytes;
    
    if ( offset+size > p->p_filesz ) {

	printf("Warning offset is to big %zi %zi "TARGET_PTR" "TARGET_PTR"\n",
	       offset,
	       offset+size,
	       p->p_filesz,
	       p->p_vaddr);
	exit(10);
    }
    
    if (lseek (cproc->coreFd, fileOffset, SEEK_SET) != fileOffset)
    {
	printf ("edb: cannot seek to segment %d offset %zi in core file", 
		segNo, offset);
	perror ("");
	exit(1);
    }
    
    bytes = read(cproc->coreFd, space, size);
    
    if ( bytes != size )
    {
	printf ("edb: cannot read segment %d offset %zi in core file %zi/%zi\n",
		segNo, offset, bytes, size);
	exit (1);
    }
}

void
dumpMemoryTSIZE (struct cproc *cproc)
{
    ElfTSIZE_Phdr    *p = cproc->corePhdrTSIZE;
    int i;
    
    for (i = 0; i < cproc->coreEhdrTSIZE->e_phnum; i++, p++)
    {
	printf("This segment[%d/%d] (%d) goes from "TARGET_PTR" to "TARGET_PTR" "TARGET_PTR" "TARGET_PTR"\n",
	       i,cproc->coreEhdrTSIZE->e_phnum,
	       p->p_type,
	       p->p_vaddr,
	       p->p_vaddr+p->p_memsz,
	       p->p_offset,
	       p->p_filesz);
    }
}
 
int
fetchtiTSIZE (struct etrace_ops cb, uint64_t tip, struct elan4_trap_infoTSIZE *ti)
{
    if ( cb.rcopy(cb.handle,tip,ti,sizeof(struct elan4_trap_infoTSIZE)) == -1 )
	return -1;
    return 0;
}

int
readMemoryTSIZE (void *handle, uint64_t addr, void *space, uint64_t bytes)
{
    struct cproc *cproc = (struct cproc *)handle;
    ElfTSIZE_Phdr    *p = cproc->corePhdrTSIZE;
    int i;
    
    if ( verbose > 1 )
	printf("TSIZE Fetching %"PRId64" bytes of data from %#"PRIx64"\n",
	       bytes, addr);
    
    for (i = 0; i < cproc->coreEhdrTSIZE->e_phnum; i++)
    {
	if (p->p_type == PT_LOAD)
	{
	    if (addr >= p->p_vaddr && addr < (p->p_vaddr+p->p_filesz))
	    {
		if ( verbose > 1 )
		    printf("This region[%d] is from "TARGET_PTR" to "TARGET_PTR", offset "TARGET_PTR" %#"PRIx64" %#"PRIx64"\n",
			   i,
			   p->p_vaddr,
			   p->p_vaddr+p->p_memsz,
			   p->p_offset,
			   addr,
			   addr+bytes);
		
		if ((addr+bytes) > p->p_vaddr+p->p_memsz)
		{
		    printf ("edb: cannot read memory %#"PRIx64"->%#"PRIx64" (across segments)\n",
			    addr, addr+bytes);
		    return -1;
		}
		
		if ( (addr - p->p_vaddr) + bytes > p->p_filesz ) {
		    printf("Data is not in core file, looking elsewhere %d\n",i);
		    if ( i == 1 ) {
			if (read_from_file(cproc->binFd ,space, addr - p->p_vaddr ,bytes)==-1 ) {
			    return -1;
			}
			return 0;
		    }
		    return -1;
		} else {
		    readSegmentDataTSIZE (cproc, i, addr - p->p_vaddr, space, bytes);
		    return 0;
		}
	    }
	}
	p++;
    }
    printf ("edbTSIZE: cannot read memory %#"PRIx64"->%#"PRIx64" from core file\n", addr, addr+bytes);
    exit(1);
}

#define BUFFER_SIZE 	(size_t)8192
uint64_t
scanForExceptionMagicTSIZE (struct cproc *cproc)
{
    ElfTSIZE_Phdr      *p = cproc->corePhdrTSIZE;
    unsigned int i;
    char *ptr;
    static char    buffer[BUFFER_SIZE];
    size_t	   bytes;
    size_t         bytesToRead;
    TARGET_TYPE    vaddr;

    for (i = 0; i < cproc->coreEhdrTSIZE->e_phnum; i++, p++)
    {
	if (p->p_type == PT_LOAD)
	{
	    
	    if (lseek (cproc->coreFd, cproc->corePhdrTSIZE[i].p_offset, SEEK_SET) != cproc->corePhdrTSIZE[i].p_offset)
	    {
		fprintf (stderr, "edb: cannot seek to segment %d file offset "TARGET_PTR" ",
			 i,
			 cproc->corePhdrTSIZE[i].p_offset);
		perror ("");
	    }
	    else
	    {
		for (vaddr = p->p_vaddr, bytes = p->p_filesz; bytes != 0; 
		     vaddr += bytesToRead, bytes -= bytesToRead)
		{
		    bytesToRead = (bytes > BUFFER_SIZE) ? BUFFER_SIZE : bytes;
		    
		    if (read (cproc->coreFd, buffer, bytesToRead) < 0)
		    {
			fprintf (stderr, "edb: cannot read segment %d", i);
			perror ("");
			exit (1);
		    }
		    
		    for (ptr = buffer; ptr < buffer + bytesToRead; ptr += sizeof(uintTSIZE_t))
		    {
			/* Look for the elan3_exeception structure (see libelan3/elanlib.c) */
			/* Looks like a elan4_trap_info */
			
			if (*((uintTSIZE_t *) ptr) == (uintTSIZE_t)ELAN4_TRAP_MAGIC)
			{
			    if ( verbose )
				printf ("edb: found debug information at "TARGET_PTR"\n",
					vaddr + (TARGET_TYPE)(ptr - buffer));
			    
			    /* The elan3_exceptions struct consists of a long followed
			     * by a SYS_EXCEPTION_SPACE pointer; we return the offset
			     * of this pointer
			     */
			    return (vaddr + (ptr - buffer));
			}
		    }
		}
	    }
	}
    }
    return (0);
}

/* resolve the tables for symbols*/
static uint64_t
resolv_tablesTSIZE (struct etrace_ops *ops, struct link_map *map)
{
    ElfTSIZE_Dyn dyn;
    uint64_t addr;
    uint64_t hash = 0;
    int nchains;
    uint64_t symtab = 0;
    uint64_t strtab = 0;
        
    addr =  (uint64_t)(uintptr_t)map->l_ld;
    
    if ( verbose )
	printf("Looking in linkmap %#"PRIx64"\n",addr);
    
    if (ops->rcopy(ops->handle, addr, &dyn, sizeof(ElfTSIZE_Dyn))==-1)
	return 0;
    
    while (dyn.d_tag) {
	switch (dyn.d_tag) {
	case DT_HASH:
	    
	    if ( verbose > 2 )
		printf("Read nchains from "TARGET_PTR" %p\n",dyn.d_un.d_ptr, (void *)map->l_addr);

	    if (ops->rcopy(ops->handle,
				    (uint64_t)(uintptr_t)dyn.d_un.d_ptr, &hash, sizeof(hash)) == -1) {
		    return 0;
	    }
	    
	    if (ops->rcopy(ops->handle,
			   (uint64_t)(uintptr_t)(dyn.d_un.d_ptr + 4),
			   &nchains,
			   sizeof(nchains)) == -1 ) {
		return 0;
	    }
	    break;
	case DT_STRTAB:
	    strtab = dyn.d_un.d_ptr;
	    break;
	case DT_SYMTAB:
	    symtab = dyn.d_un.d_ptr;
	    break;
	default:
	    break;
	}
	addr += sizeof(ElfTSIZE_Dyn);
	if (ops->rcopy(ops->handle,addr, &dyn, sizeof(ElfTSIZE_Dyn))==-1) {
	    return 0;
	}
    }
    return (find_sym_in_tablesTSIZE(ops,map,hash,nchains,symtab,strtab,"elan_base"));
}





/* locate link-map in pid's memory */
uint64_t locate_linkmapTSIZE (struct etrace_ops *ops, int pid)
{
    ElfTSIZE_Ehdr ehdr;
    ElfTSIZE_Phdr phdr;
    ElfTSIZE_Dyn dyn;
    struct link_map *link = malloc(sizeof(struct link_map));
    ElfTSIZE_Phdr *phdr_addr;
    ElfTSIZE_Dyn *dyn_addr;
    void *map_addr;
    void *got;
    
    if ( ! link )
	exit(1);
    
    /* 
     * first we check from elf header, mapped at 0x08048000, the offset
     * to the program header table from where we try to locate
     * PT_DYNAMIC section.
     */
    
#if 0
    {
	/* Good code but based on the poor assumption that the executable is listed
	 * first in the maps file.  It isn't, the file is sorted. */
	int fd;
	char fname[128];
	char header[128] = {0};
	void *base = NULL;
	size_t size;
	int matched;
	sprintf(fname,"/proc/%d/maps",pid);
	fd = open(fname,O_RDONLY);
	size = read(fd,header,128);
	close(fd);
	matched = sscanf(header,"%p",&base);
	printf("Fd %d %zi %d name %s %p %p\n",fd,size,matched,fname,base,START_POINT);
    }
#endif
    
    if (ops->rcopy(ops->handle, (uint64_t)START_POINT, &ehdr, sizeof(ElfTSIZE_Ehdr)) == -1 ) {
	printf("Failed to load anything...\n");
	exit(1);
    }
    
    if ( strncmp(&ehdr.e_ident[1],"ELF",3) != 0 ) {
	printf("Could not find ELF header at entry point (%p)\n",START_POINT);
	exit(1);
    }
    
    phdr_addr = (void *)(uintptr_t)(START_POINT + ehdr.e_phoff);
    
    ops->rcopy(ops->handle,(uint64_t)(uintptr_t)phdr_addr, &phdr, sizeof(ElfTSIZE_Phdr));
    
    while (phdr.p_type != PT_DYNAMIC) {
	phdr_addr ++;
	if (ops->rcopy(ops->handle,(uint64_t)(uintptr_t)phdr_addr , &phdr,sizeof(ElfTSIZE_Phdr))==-1)
	    return 0;
    }
    
    /* 
     * now go through dynamic section until we find address of the GOT
     */
    
    ops->rcopy(ops->handle,(uint64_t)(uintptr_t)phdr.p_vaddr, &dyn, sizeof(ElfTSIZE_Dyn));
    
    dyn_addr = (ElfTSIZE_Dyn *)(uintptr_t)phdr.p_vaddr;
    
    while (dyn.d_tag != DT_PLTGOT) {
	dyn_addr ++;
	ops->rcopy(ops->handle,(uint64_t)(uintptr_t)dyn_addr, &dyn, sizeof(ElfTSIZE_Dyn));
    }
    
    got = (void *)(uintptr_t)dyn.d_un.d_ptr;
#ifndef S_SPLINT_S
#ifndef LINUX_IA64
    got = (void *)((uintptr_t)got + sizeof(long));
//    (char *)got += sizeof(long);
#endif
#endif
    
    /* 
     * now just read first link_map item and return it 
     */
    ops->rcopy(ops->handle, (uint64_t)(uintptr_t)got, &map_addr, sizeof(void *));
    ops->rcopy(ops->handle, (uint64_t)(uintptr_t)map_addr, link, sizeof(struct link_map));
    
    do {
	uint64_t b;
	char name[4];
	
	if ( verbose > 2 )
	    printf("The link_map looks like %p %p %p %p %p\n",
		   (void *)link->l_addr,
		   link->l_name,
		   link->l_ld,
		   link->l_next,
		   link->l_prev);
	

	/* Grab 4 bytes because ptrace won't allow less */
	ops->rcopy(ops->handle, (uint64_t)(uintptr_t)link->l_name, &name, 4);
	if ( *name == 0 ) {
	    if ( verbose > 2 )
		printf("Skipping anonymous map\n");
	    b = 0;
	} else {
	    b = (uint64_t)resolv_tablesTSIZE(ops,link);
	}
	if ( b ) {
	    free(link);
	    return b;
	}
	
	if ( link->l_next )
	    ops->rcopy(ops->handle, (uint64_t)(uintptr_t)link->l_next, link, sizeof(struct link_map));
	else {
	    free(link);	 
	    link = NULL;
	}
    } while (link);
    return 0;
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
