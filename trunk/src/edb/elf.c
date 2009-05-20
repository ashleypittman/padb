/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "elf.c,v 1.9 2005/07/01 13:48:13 addy Exp"
/*             /cvs/master/quadrics/elan4lib/edb/elf.c,v */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <edb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <elan4/library.h>
#include <elan4/lib_cq_dump.h>

static int
openCoreFile (struct cproc *cproc)
{
    struct stat    buf;
    
    if (stat(cproc->cname, &buf) == -1)
    {
	printf ("edb: cannot open core file '%s':\n", cproc->cname);
	perror (""); exit (1);
    }
    
    if ((cproc->coreFd = open (cproc->cname, O_RDONLY)) == -1)
    {
	printf ("edb: cannot read core file '%s'\n", cproc->cname);
	return -1;
    }
    
    if (elan4_elf_version (EV_CURRENT) == EV_NONE)
	return -1;
    
    if ((cproc->coreElf = elan4_elf_begin (cproc->coreFd, ELF_C_READ, (Elf *) 0)) == NULL)
    {
	printf ("edb: elan4_elf_begin(): %s\n", elan4_elf_errmsg(0));
	return -1;
    }
    
    if (elan4_elf_kind (cproc->coreElf) != ELF_K_ELF)
    {
	printf ("edb: '%s' is not a ELF file\n", cproc->cname);
	return -1;
    }
    
    if (! (cproc->coreEhdr64 = elan4_elf64_getehdr (cproc->coreElf)))
    {
	if (! (cproc->coreEhdr32 = elan4_elf32_getehdr (cproc->coreElf)))
	{
	    printf ("edb: elan4_elf_getehdr(): %s\n", elan4_elf_errmsg(0));
	    return -1;
	}
    }
    
    if (cproc->coreEhdr64->e_type != ET_CORE)
    {
	printf ("edb: %s is not a core file\n", cproc->cname);
	return -1;
    }
    
    switch ( cproc->coreEhdr64->e_machine ) {
    case EM_386:
	cproc->wordSize = 32;
	if (! (cproc->corePhdr32 = elan4_elf32_getphdr (cproc->coreElf)))
	{
	    printf ("edb: elan4_elf_getphdr(): %s\n", elan4_elf_errmsg(0));
	    return -1;
	}
	break;
    case EM_IA_64:
    case EM_X86_64:
	if (! (cproc->corePhdr64 = elan4_elf64_getphdr (cproc->coreElf)))
	{
	    printf ("edb: elan4_elf_getphdr(): %s\n", elan4_elf_errmsg(0));
	    return -1;
	}
	
	cproc->wordSize = 64;
	break;
    default:
	printf("Core file machine type not recognised: %d\n",
	       cproc->coreEhdr64->e_machine );
	return -1;
    }
    
    if ( cproc->binName ) {
	cproc->binFd = open(cproc->binName,O_RDONLY);
	if ( cproc->binFd == -1 ) {
	    printf("Failed to open exe file for reading, %s\n",cproc->binName);
	    return -1;
	}
    } else
	cproc->binFd = -1;
    
    return 0;
}

static int extract_ti (struct local_eop *eop, struct elan4_trap_info64 *ti64, char *cname)
{    
    struct elan4_trap_info32 elan4_ti32;
    struct cproc *cproc;
    uint64_t tip;
    
    if ( (cproc = malloc(sizeof(struct cproc))) == NULL )
	return -1;
    
    memset(cproc,0,sizeof(struct cproc));
    
    cproc->cname = cname;
    
    memset(eop,0,sizeof(struct local_eop));
    
    eop->cb.handle = cproc;
    
    if (openCoreFile(cproc)!=0)
	exit(1);
    
    if ( cproc->wordSize == 64 ) {
	eop->cb.rcopy = readMemory64;
	
	if ( verbose > 2 )
	    dumpMemory64(cproc);
	
	if ( ( tip = scanForExceptionMagic64(cproc) ) == 0 ) {
	    printf("Failed to find exception info in core file\n");
	    exit(1);
	}
	
	if ( fetchti64(eop->cb, tip, ti64) == -1 ) {
	    printf("Failed to copy status information\n");
	    exit(1);
	}
	
    } else if ( cproc->wordSize == 32 ) {
	
	/* To make this code common we load the 32bit one and 
	 * then copy the values to the 64 bit one so the rest of
	 * this function just works(tm) */
	
	eop->cb.rcopy = readMemory32;
	if ( verbose > 2 )
	    dumpMemory32(cproc);
	
	if ( ( tip = scanForExceptionMagic32(cproc) ) == 0 ) {
	    printf("Failed to find exception info in core file\n");
	    exit(1);
	}
	if ( fetchti32(eop->cb, tip, &elan4_ti32) == -1 ) {
	    printf("Failed to copy status information\n");
	    exit(1);
	}
	
	ti64->trap            = elan4_ti32.trap;
	ti64->elan3Compat     = elan4_ti32.elan3Compat;
	ti64->msg             = elan4_ti32.msg;
	ti64->base            = elan4_ti32.base;
	ti64->exceptionMain   = elan4_ti32.exceptionMain;
	ti64->exceptionThread = elan4_ti32.exceptionThread;
	ti64->msgSize         = elan4_ti32.msgSize;
	ti64->msgLen          = elan4_ti32.msgLen;
	ti64->cq_info         = elan4_ti32.cq_info;	
    } else {
	printf("Can't work with this word size %d\n",
	       cproc->wordSize);
	exit(1);
    }
    return 0;
}

static void raw_from_buf (uint64_t *buf, uint32_t start, uint32_t end, int nelem)
{
    int wrap = end <= start;
    
    if ( start == end )
	end--;
    
    if ( start & 3 )
	start = ELAN_ALIGNUP(start,4) - 4;
    
    end++;
    end = (ELAN_ALIGNUP(end,4) + 3 ) % nelem;
    
    end -= 3;
    
    if ( (start != end) && (start != ((end - 4) % nelem)) )
	wrap = FALSE;
    
    do {
	printf ("%5d - %016" PRIx64 " %016" PRIx64 " %016" PRIx64 " %016" PRIx64 "\n", 
		start,
		buf[start+0], buf[start+1], 
		buf[start+2], buf[start+3]);
	
	if ( start == end && wrap == TRUE )
	    wrap = FALSE;
	
	start += 4;
	start %= nelem;
	
    } while ( (start != end) || (wrap == TRUE) );
    
    return;
}

void
show_cq_from_file (char *cname, uint32_t start, uint32_t end, int raw)
{
    struct local_eop          eop;
    struct elan4_trap_info64  elan4_ti64;
    struct elan4_trap_cqinfo  cq_info;
    ELAN4_USER_TRAP           trap;
    uint64_t                 *cq_contents;
    uint32_t                  bestStart;
    uint32_t                  bestEnd;
    int                       nslots;
    
    extract_ti(&eop,&elan4_ti64,cname);
    
    if ( elan4_ti64.cq_info == 0 ) {
	printf("No command stream stored in core file\n");
	return;
    }
    
    if (eop.cb.rcopy(eop.cb.handle, elan4_ti64.cq_info, &cq_info, sizeof(struct elan4_trap_cqinfo)) == -1 ) {
	printf("Failed to copy cq contents from core file\n");
	exit (1);
    }
    
    cq_contents = malloc((size_t)cq_info.cq_size);
    
    if ( cq_contents == NULL ) {
	printf("Failed to allocate %zi bytes for command queue data\n",(size_t)cq_info.cq_size);
	exit (1);
    }
    if (eop.cb.rcopy(eop.cb.handle, cq_info.cq_data, cq_contents, cq_info.cq_size) == -1 ) {
	printf("Failed to copy cq contents from core file\n");
	exit (1);
    }
    if (eop.cb.rcopy(eop.cb.handle, elan4_ti64.trap, &trap, sizeof(ELAN4_USER_TRAP)) == -1 ) {
	printf("Failed to copy trap info from core file\n");
	exit (1);
    }
    {
	E4_uint64 CQ_QueuePtrs = trap.ut_trap.cproc.tr_qdesc.CQ_QueuePtrs;
	unsigned long insertPtr, completedPtr;
	E4_uint64 cq_space = cq_info.cq_space;
	int cq_size = cq_info.cq_size;
	
	completedPtr = CQ_CompletedPtr(CQ_QueuePtrs);
	insertPtr    = CQ_InsertPtr(CQ_QueuePtrs);
	
	if ((CQ_QueuePtrs & CQ_RevB_ReorderingQueue))
	{
	    /* On Elan4 RevB we have out of ordering command queue support; for this type of CmdQ
	     * the CQ_HoldingValue is a bitmask with each bit representing which
	     * dword offsets from the CQ_InsertPtr are valid
	     */
	    E4_uint32 oooMask =  trap.ut_trap.cproc.tr_qdesc.CQ_HoldingValue;
	    
	    for (; (oooMask & 1) != 0; oooMask >>= 1)
		insertPtr = (insertPtr & ~(cq_size-1)) | ((insertPtr + sizeof (E4_uint64)) & (cq_size-1));
	}
	
	nslots = cq_size>>3;
	
	bestStart = (completedPtr - cq_space)/8;
	bestEnd = (insertPtr - cq_space)/8;
	
	if ( start != (uint32_t)-1 && end == (uint32_t)-1 )
	    end = (start - 1) % nslots;
	
	if ( start == (uint32_t)-1 ) {
	    printf("Found command q, size %zi\n",(size_t)cq_size);
	    
	    printf("\n");
	    printf("This command Q has %d slots numbered from 0 to %d\n",nslots,nslots-1);
	    printf("Unprocessed entries are found in %d to %d\n\n",bestStart,bestEnd);
	    
	    start = bestStart;
	    end = bestEnd;
	}
	
	printf("Dumping Command Q from %d (%#x bytes) to %d (%#x bytes)\n",
	       start,
	       start*8,
	       end,
	       end*8);
	
	if ( raw )
	    raw_from_buf(cq_contents,start,end,nslots);
	else
	    elan4_cq_translate_cq(cq_contents,
				  start*8,
				  end*8,
				  cq_size,
				  1);
    }    
}

void
fetch_data_dead (char *cname, char *ename, int trap_dump)
{
    struct elan4_trap_info64 elan4_ti64;
    struct local_eop eop;
    
    extract_ti(&eop,&elan4_ti64,cname);
        
    if ( elan4_ti64.elan3Compat != 0 ) {
	int res;
	printf("Elan3 core file found...\n");
	if ( ! ename ) {
	    printf("Can't parse Elan3 core files without specifying a executable (-e)\n");
	    exit(1);
	}
	fflush(NULL);
	res = execlp("edb.elan3", "edb.elan3", ename, cname, NULL);
	perror("execlp failed");
	exit(2);
    }
    
    if ( trap_dump ) {
	if ( elan4_ti64.exceptionMain != 0 ) {
	    char local[ELAN4_MAX_TRAP_STRING];
	    if (eop.cb.rcopy(eop.cb.handle, elan4_ti64.exceptionMain, &local, ELAN4_MAX_TRAP_STRING) == -1 ) {
		printf("Failed to copy exception info from core file\n");
		exit (1);
	    }
	    printf("%s",local);
	}
	if ( elan4_ti64.exceptionThread != 0 ) {
	    char local[ELAN4_MAX_TRAP_STRING];
	    if (eop.cb.rcopy(eop.cb.handle, elan4_ti64.exceptionThread, &local, ELAN4_MAX_TRAP_STRING) == -1 ) {
		printf("Failed to copy exception info from core file\n");
		exit (1);
	    }
	    printf("%s",local);
	}
	if ( elan4_ti64.msgLen != 0 ) {
	    char local[ELAN4_MAX_TRAP_STRING];
	    if (eop.cb.rcopy(eop.cb.handle, elan4_ti64.msg, &local, (uint64_t)elan4_ti64.msgLen) == -1 ) {
		printf("Failed to copy exception info from core file\n");
		exit (1);
	    }
	    local[elan4_ti64.msgLen] = '\0';
	    printf("%s",local);
	}
	if ( elan4_ti64.cq_info != 0 ) {
	    void *cq_contents;
	    ELAN4_USER_TRAP trap;
	    struct elan4_trap_cqinfo cq_info;
	    
	    if (eop.cb.rcopy(eop.cb.handle, elan4_ti64.cq_info, &cq_info, sizeof(struct elan4_trap_cqinfo)) == -1 ) {
		printf("Failed to copy cq info from core file\n");
		exit (1);
	    }
	    
	    printf("cq_info data %#"PRIx64" space %#"PRIx64" size %#"PRIx32"\n",
		   cq_info.cq_data, cq_info.cq_space, cq_info.cq_size);
	    
	    cq_contents = malloc((size_t)cq_info.cq_size);
	    
	    if ( cq_contents == NULL ) {
		printf("Failed to allocate %zi bytes for command queue data\n",(size_t)cq_info.cq_size);
		exit (1);
	    }

	    if (eop.cb.rcopy(eop.cb.handle, cq_info.cq_data, cq_contents, cq_info.cq_size) == -1 ) {
		printf("Failed to copy cq contents from core file\n");
		exit (1);
	    }
	    if (eop.cb.rcopy(eop.cb.handle, elan4_ti64.trap, &trap, sizeof(ELAN4_USER_TRAP)) == -1 ) {
		printf("Failed to copy trap info from core file\n");
		exit (1);
	    }
	    {
		E4_uint64 CQ_QueuePtrs = trap.ut_trap.cproc.tr_qdesc.CQ_QueuePtrs;
		unsigned long insertPtr, completedPtr;
		E4_uint64 cq_space = cq_info.cq_space;
		int cq_size = cq_info.cq_size;
		
		completedPtr = CQ_CompletedPtr(CQ_QueuePtrs);
		insertPtr    = CQ_InsertPtr(CQ_QueuePtrs);
		
		if ((CQ_QueuePtrs & CQ_RevB_ReorderingQueue))
		{
		    /* On Elan4 RevB we have out of ordering command queue support; for this type of CmdQ
		     * the CQ_HoldingValue is a bitmask with each bit representing which
		     * dword offsets from the CQ_InsertPtr are valid
		     */
		    E4_uint32 oooMask =  trap.ut_trap.cproc.tr_qdesc.CQ_HoldingValue;
		    
		    for (; (oooMask & 1) != 0; oooMask >>= 1)
			insertPtr = (insertPtr & ~(cq_size-1)) | ((insertPtr + sizeof (E4_uint64)) & (cq_size-1));
		}
		
		fflush(NULL);
		
		elan4_cq_translate_cq(cq_contents,
				      (completedPtr - cq_space),
				      (insertPtr - cq_space),
				      cq_size,
				      1);
		
	    }
	}
	exit(0);
    }
    
    eop.base = elan4_ti64.base;
    
    if ( ! eop.base ) {
	/* This could do more... */
	printf("Program didn't initilise properly, cannot continue\n");
	exit(0);
    }
    
    if ( verbose )
	printf("Found base at %#"PRIx64"\n", eop.base);
    
    fetch_data_common(&eop);
    
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
