/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "edb.c,v 1.24 2006/11/29 10:25:23 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/edb.c,v */


#include "edb.h"

#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <string.h>

#include <elf.h>

#include <stdio.h>
#include <link.h>
#include <signal.h>
#include <getopt.h>

int verbose = 0;


/***********************************************************
 *                                                         *
 * The ptrace code...                                      *
 *                                                         *
 *                                                         *
 *                                                         *
 ***********************************************************/

int
read_from_file (int fd, void *base, off_t offset, size_t len)
{
    
    if ( lseek(fd,offset,SEEK_SET) != offset )
	return -1;
    
    if ( read(fd,base,len) != len )
	return -1;
    
    return 0;
}

/* The totalview callback is (handle, remote, size,local) */

int
fetch_string (struct etrace_ops *ops, char *local, uint64_t remote, size_t len)
{
    uint64_t rstr = remote;
    char *lstr = local;
    size_t l = 4;
    
    do {
	if (ops->rcopy(ops->handle,rstr,lstr,4) == -1 ) {
	    printf("Warning, failed to read string\n");
	    return -1;
	}
	if ( strlen(lstr) < 4 )
	    return 0;
	l    += 4;
	lstr += 4;
	rstr += 4;
    } while ( l < len );
    printf("Read %zi bytes but didn't find end of string %s\n",l,local);
    return -1;
}



/* When using ptrace it's important to detach from all processes
 * or they become zombies and they stick around a block the 
 * RMS partition.  Apply a signal handler to reduce the risk
 * of this.
 */
static void
segv (int sig)
{
    
    printf("Caught signal %d\n",sig);
    
    exit(1);
}

static void
_catchSegv (void)
{
    struct sigaction sa;
    int i;
    int sig;
    int caught[] = { SIGSEGV, SIGINT, SIGHUP };
    
    memset(&sa,0,sizeof(struct sigaction));
    sa.sa_handler = segv;
    
    for ( sig = 0 ; sig < 3 ; sig++ ) {
	i = sigaction(caught[sig],&sa,NULL);
	if ( i != 0 )
	    printf("Failed to register signal handler for signal %d: %d\n",caught[sig],i);
    }
}



/***********************************************************
 *                                                         *
 * The reading code...                                     *
 *                                                         *
 *                                                         *
 *                                                         *
 ***********************************************************/

#define WALK_LIST(LIST,NAME) do {es = esa->LIST; \
	       		while ( es ) { \
			  printf("%s at %p\n",NAME,es->addr); \
			es = es->next; \
		}} while (0)

void
fetch_data_common(struct local_eop *eop)
{
    
    struct elan_sys_all *esa;
    struct elan_sys *es;
    struct tport_info *ti;
    struct x_info *x_info;
    
    if ( (esa = elan_fetchInfo(&eop->cb,0,(void *)(uintptr_t)eop->base)) == NULL ) {
	printf("elan_fetchInfo() failed\n");
	exit(1);
    }
    
    if ( verbose )
	printf("This is process %d/%d %p\n",
	       esa->base->state->vp,
	       esa->base->state->nvp,
	       esa->tport_list);
    
    if (verbose)
	WALK_LIST(tport_list,"tport");
    
    x_info = x_init(2);
    
    es = esa->tport_list;
    while (es) {
	ti = elan_tportRetrive(&eop->cb,es->addr);
	if ( ! ti ) {
	    printf("Failed to fetch tport information, exiting\n");
	    exit(1);
	}
	
	dump_ti(x_info,ti);
	elan_ti_free(ti);
	es = es->next;
    };
    
    elan_esa_free(esa);
    
    x_free(x_info);
    
}




/***********************************************************
 *                                                         *
 * The annoying but needed initilisation code              *
 *                                                         *
 *                                                         *
 *                                                         *
 ***********************************************************/

/* Statistics gathering code */

#define PAGESIZE (sysconf(_SC_PAGESIZE))

struct str {
    char entry[16];
};

extern int attach_sysv(struct sysv *v, void *base)
{
    v->id = shmget(v->key,0,0600);
    
    if ( v->id == -1 ) {
	if ( verbose )
	    perror("Failed to attach to shared memory");
	v->attached = FALSE;
	return(FALSE);
    }
    
    if ( shmctl(v->id,IPC_STAT,&v->sid) == -1 ) {
	if ( verbose )
	    perror("smhctl failed");
	v->attached = FALSE;
	return(FALSE);
    }
    
    if ( base )
	munmap(base,v->sid.shm_segsz);
    
    v->base = shmat(v->id,base,0);
    
    if ( v->base == (void *)-1) {
	if ( verbose ) {
	    fprintf(stderr,"shmat failed %d,%p %zi\n",v->id,base,v->sid.shm_segsz);
	    perror("smhat failed");
	}
	v->attached = FALSE;
	return(FALSE);
    }
    
    v->size = v->sid.shm_segsz;
    
    return(TRUE);
}

static void dump_stats_file (char *fname)
{
    struct sf sf = SF_INITIALIER;
    
    
    if ( sf_from_file(&sf,fname) < 0 ) {
	perror("Couldn't load stats:");
	exit(1);
    }
    
    sf_dump(&sf);
    return;
}

static void dump_stats (struct sf_params *sfp, struct stats_options *so)
{
    struct str *esh;
    size_t size;
    struct sysv v = { .key = sfp->key };
    
    if ( ! attach_sysv(&v,NULL) ) {
	perror("Failed to attach to shared memory");
	exit(1);
    }
    
    esh = v.base;
    
    size = v.sid.shm_segsz;
    
    /* Now check the header */
    if ( strncmp((char *)&esh[0],"ELAN STATS",16) ) {
	if ( verbose )
	    printf("Stats header incorrect \"%s\"\n", (char *)&esh[0]);
	else
	    printf("Stats header incorrect\n");
	return;
    }
    
    if ( ! strncmp((char *)&esh[1], "Eagle", 16) ) {
	size_t pagesize = (size_t)PAGESIZE;
	if (dump_stats_eagle(esh,size,pagesize) != 0)
	    perror("Failed to load statistics");
	return;
    }
    
    {
	struct sf sf = SF_INITIALIER;
	if ( sf_init(&sf, sfp, v.base, v.sid.shm_segsz,0) ) {
	    if ( so->todo == EDBSTAT_RAW ) {
		sf_raw_to_file(&sf,stdout);
	    } else if ( sfp->target_vp == -1 )
		sf_dump_all(&sf);
	    else
		sf_dump_vp(&sf,sfp->target_vp);
	    return;
	}
    }

    if ( verbose )
	printf("Stats type incorrect \"%s\"\n", (char *)&esh[1]);
    else
	printf("Stats type incorrect\n");
}

/* helper function for main below */
static int go_stats (struct sf_params *sfp, struct stats_options *so, char *fname) {
    
    if ( so->todo == EDBSTAT_NOP )
	so->todo = EDBSTAT_SHOW;
    
    if ( sfp->key ) {
	if ( sfp->parallel )
	    go_parallel(sfp,so);
	else
	    dump_stats(sfp,so);
    }
    
    if ( fname ) {
	dump_stats_file(fname);
    }
    
    return 0;
}

static void set_debug (struct sf_params *sfp, uint64_t debug) {
    struct str *esh;
    size_t size;
    struct sysv v = { .key = sfp->key };
    
    if ( ! attach_sysv(&v,NULL) ) {
	perror("Failed to attach to shared memory");
	exit(1);
    }
    
    esh = v.base;
    
    size = v.sid.shm_segsz;
    
    /* Now check the header */
    if ( strncmp((char *)&esh[0],"ELAN STATS",16) ) {
	if ( verbose )
	    printf("Stats header incorrect \"%s\"\n", (char *)&esh[0]);
	else
	    printf("Stats header incorrect\n");
	return;
    }
    
    {
	struct sf sf = SF_INITIALIER;
	if ( sf_init(&sf, sfp, v.base, v.sid.shm_segsz,0) ) {
	    sf_set_debug_vp(&sf,sfp->target_vp,debug);
	    return;
	}
    }
    
    if ( verbose )
	printf("Stats type incorrect \"%s\"\n", (char *)&esh[1]);
    else
	printf("Stats type incorrect\n");
    
}

#define  U_OPT_C(SHORT,LONG,STR) fprintf(f," -%c, --%-16s %s\n",SHORT,LONG,STR)
#define  U_OPT(LONG,STR) fprintf(f,"     --%-16s %s\n",LONG,STR)

static void
usage(FILE *f, char *prog, int exitcode)
{
    
    fprintf(f,"Usage: %s [OPTIONS]...\n",prog);
    fprintf(f,"Extract information from a parallel program\n");
    
    fprintf(f,"\nActions\n");
    U_OPT_C('s',"stats","Show job communication statistics");
    U_OPT_C('q',"queues","Show active message queues");
    U_OPT_C('C',"cq-dump","Dump command stream data");
    U_OPT_C('d',"debug=FLAGS","Set libelan debug flags");
    U_OPT_C('h',"help","Show this help");
    
    fprintf(f,"\nInput source\n");
    U_OPT_C('c',"core=FILE","core file");
    U_OPT_C('e',"exe=FILE","executable");
    U_OPT_C('p',"pid=PID","Attach to process id");
    U_OPT_C('k',"key=KEY","Shared memory key");
    U_OPT_C('f',"file=FILE","Load statistics from file");

    fprintf(f,"\nOptions\n");
    
    U_OPT("cq-start=INDEX","Start dump at index");
    U_OPT("cq-end=INDEX","Finish dump at index");
    U_OPT("cq-raw","Show raw command stream");
    
    U_OPT("pagesize=SIZE","'page' size used for statistics");
    U_OPT("pagesize-header=SIZE","'page' size used for statistics header page");
    
    U_OPT("target-vp=VP","Only dump statistics for given vp");
    
    U_OPT("parallel","Run in parallel mode");
    U_OPT("stats-raw","Show stats as raw data");
    
    exit(exitcode);
}

/***********************************************************
 *                                                         *
 * And everyone's favorite, main().                        *
 *                                                         *
 *                                                         *
 *                                                         *
 ***********************************************************/

uint64_t read64 (char *arg)
{
    uint64_t val = 0;
    sscanf(arg, "%" SCNx64, &val);
    return val;
}

#define OPT_CQSTART   10000
#define OPT_CQEND     10001
#define OPT_STATSIZE  10002
#define OPT_STATHSIZE 10003
#define OPT_TARGETVP  10004
#define OPT_STATRAW   10005

typedef enum {
    OP_NULL, OP_STATS, OP_QUEUE, OP_CQ, OP_TRAP, OP_DEBUG
} EDB_OP;

int
main (int argc, char **argv) {
    int   c;
    int   cq_raw = FALSE;
    uint32_t cq_start = (uint32_t)-1;
    uint32_t cq_end = (uint32_t)-1;
    uint64_t debug = 0;
    
    char *fname = NULL;
    char *cname = NULL;
    char *ename = NULL;
    int   pid = 0;
    EDB_OP op = OP_NULL;
    EDB_OP sop = OP_NULL;
    struct sf_params sfp = { .target_vp = -1 };
    struct stats_options so = { 0 };
    
    struct option long_options[] =
	{
	    /* Flags */
	    {"verbose",     no_argument, &verbose, 1},
	    {"cq-raw",      no_argument, &cq_raw, 1},
	    {"parallel",    no_argument, &sfp.parallel, 1},
	    
	    /* Where to look for input */
	    {"core",        required_argument, NULL, (int)'c'},
	    {"exe",         required_argument, NULL, (int)'e'},
	    {"pid",         required_argument, NULL, (int)'p'},
	    {"key",         required_argument, NULL, (int)'k'},
	    {"file",        required_argument, NULL, (int)'f'},
	    
	    /* What to do */
	    {"stats",       no_argument,       NULL, (int)'s'},
	    {"queues",      no_argument,       NULL, (int)'q'},
	    {"cq-dump",     no_argument,       NULL, (int)'C'},
	    {"help",        no_argument,       NULL, (int)'h'},
	    {"stats-raw",   no_argument,       NULL, OPT_STATRAW},
	    {"debug",       required_argument, NULL, (int)'d'},
	    

	    /* options */
	    
	    {"cq-start",    required_argument, NULL, OPT_CQSTART},
	    {"cq-end",      required_argument, NULL, OPT_CQEND},
	    
	    {"pagesize",    required_argument, NULL, OPT_STATSIZE},
	    {"pagesize-header", required_argument, NULL, OPT_STATHSIZE},
	    {"target-vp",   required_argument, NULL, OPT_TARGETVP},
	    
	    {NULL, 0, NULL, 0}
	};
    int option_index = 0;
    
    while (1)
    {
	/* getopt_long stores the option index here. */

	
	c = getopt_long (argc, argv, "c:p:k:hf:sqCgr:v",
			 long_options, &option_index);
	
	/* Detect the end of the options. */
	if (c == -1)
	    break;
	
	switch (c)
	{
	case 0:
	    break;
	    
	case 'c':
	    sop = OP_TRAP;
	    cname = optarg;
	    break;
	    
	case 'e':
	    cname = optarg;
	    break;
	    
	case 'p':
	    sop = OP_QUEUE;
	    pid = strtol(optarg,NULL,0);
	    break;

	case 'k':
	    sop = OP_STATS;
	    sfp.key = strtol(optarg,NULL,0);
	    break;
	    
	case 'f':
	    sop = OP_STATS;
	    fname = optarg;
	    break;	    
	    
	case 's':
	    op = OP_STATS;
	    break;
	    
	case 'q':
	    op = OP_QUEUE;
	    break;
	    
	case 'C':
	    op = OP_CQ;
	    break;
	    
	case 'd':
	    op = OP_DEBUG;
	    debug = read64(optarg);
	    break;
	    
	case OPT_STATRAW:
	    op = OP_STATS;
	    so.todo = EDBSTAT_RAW;
	    break;
	    
	case OPT_CQSTART:
	    cq_start = (uint32_t)strtol(optarg,NULL,0);
	    break;
	    
	case OPT_CQEND:
	    cq_end = (uint32_t)strtol(optarg,NULL,0);
	    break;

	case OPT_STATSIZE:
	    sfp.pagesize = (size_t)strtol(optarg,NULL,0);
	    break;
	    
	case OPT_STATHSIZE:
	    sfp.pagesize_h = (size_t)strtol(optarg,NULL,0);
	    break;
	    
	case OPT_TARGETVP:
	    sfp.target_vp = (int) strtol(optarg,NULL,0);
	    break;
	    	    
	case 'v':
	    verbose++;
	    break;

	case 'h':
	    usage(stdout,argv[0],0);
	    break;
	    
	case '?':
	    usage(stderr,argv[0],1);
	    break;
	    
	default:
	    usage(stderr,argv[0],1);
	}
    }
    
    /* Print any remaining command line arguments (not options). */
    if ( argc == optind + 2 ) {
	op = OP_TRAP;
	ename = argv[optind++];
	cname = argv[optind++];
    } else if (optind < argc) {
	usage(stderr,argv[0],1);
    }
    
    if ( op == OP_NULL ) {
	if ( verbose )
	    printf("No operation specified, picking one from the input source %d\n",sop);
	op = sop;
    }
    
    if ( op == OP_NULL )
	usage(stderr,argv[0],1);
    
    switch (op)
    {
    case OP_QUEUE:
	trace_init();
	_catchSegv();
	fetch_data_live(pid);
	break;
    case OP_STATS:
    {
	go_stats(&sfp,&so,fname);
	break;
    }
    case OP_CQ:
	show_cq_from_file(cname,cq_start,cq_end,cq_raw);
	break;
	
    case OP_TRAP:
	fetch_data_dead(cname,ename,1);
	break;

    case OP_DEBUG:
	set_debug(&sfp,debug);
	break;
	
    case OP_NULL:
    default:
	usage(stderr,argv[0],1);
    }
    
    exit (0);
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
