
/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "ptrace.c,v 1.5 2005/10/21 10:53:03 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/ptrace.c,v */


#include "edb.h"

#define MACRO_BEGIN {
#define MACRO_END   }
#include <qsnet/list.h>
#include <errno.h>

#include <sys/ptrace.h>

#include <sys/types.h>
#include <sys/wait.h>

enum pstate {
    PROC_NULL = 0,
    PROC_ATTACHED,
    PROC_DETACHED
};

struct rproc {
    int pid;
    enum pstate state;
    struct list_head rproc_list;
};

static struct list_head proc_list;

static void
detach (struct rproc *proc)
{
    if ( verbose )
	printf("Detaching from proc %p.%d\n",proc,proc->pid);
    (void)ptrace(PTRACE_DETACH,proc->pid,0,0);
    proc->state = PROC_DETACHED;
    list_del(&proc->rproc_list);
}

static void
exitDetach (void)
{
    struct rproc *proc;
    struct list_head *list;
    struct list_head *list0;
    
    if (verbose)
	printf("Calling exit function\n");
    list_for_each_safe(list,list0,&proc_list)
	{
	    proc = list_entry(list,struct rproc, rproc_list);
	    if ( proc->state == PROC_ATTACHED )
		detach(proc);
	}
}

void
trace_init (void) {
    atexit(exitDetach);
    INIT_LIST_HEAD(&proc_list);	
}

static void
attach (struct rproc *proc)
{
    
    if ( verbose )
	printf("Attaching to proc %p.%d\n",proc,proc->pid);
    
    /* Older versions of libelan don't have this function,
     * call it now with NULL args so if the linker is going
     * to kill us make sure it does it before the attach 
     * as dying when you are ptracing another process is
     * not a good thing to do. */
    
    (void)elan_fetchInfo(NULL,0,NULL);
    
    errno=0;
    (void)ptrace(PTRACE_ATTACH,proc->pid,NULL,NULL);
    
    if (errno) {
	perror("attach: failed to read data");
	exit(1);
    }
    list_add(&proc->rproc_list,&proc_list);
    (void)waitpid(proc->pid, NULL, WUNTRACED);
    proc->state = PROC_ATTACHED;
}

static int
_fetch_data (void *pproc, uint64_t remote, void *local, uint64_t size)
{
    struct rproc *proc =  pproc;
    int *l = (int *)local;
    int *r = (int *)(uintptr_t)remote;
    
    if (verbose >4)
	printf("Fetching %"PRId64" bytes of data from %#"PRIx64"\n",
	       size, remote);
    
//    assert((~size|3));
    
    if ( remote < (1024*64) ) {
	printf("Base is to low (%#"PRIx64"), quiting\n",remote);
	exit(1);
    }
    
    do {
	errno=0;
	*l = ptrace(PTRACE_PEEKTEXT,proc->pid,r,NULL);
	if (errno) {
	    printf("failed to read data (%p), exiting\n",r);
	    perror("failed to read data, exiting");
	    exit (1);
	}
	r++;
	l++;
	size -= sizeof(int);
    } while (size);
    return 0;
}

void
fetch_data_live (int pid)
{
    struct rproc proc;            /* Local process information */
    struct local_eop eop;             /* Callback list */
    
    memset(&proc,0,sizeof(struct rproc));
    proc.pid = pid;
    proc.state = PROC_NULL;
    
    memset(&eop,0,sizeof(struct local_eop));
    
    eop.cb.handle = &proc;
    eop.cb.rcopy = _fetch_data;
    
    attach(&proc);
    
#ifdef _ILP32
    eop.base = locate_linkmap32(&eop.cb,pid);
#else
    eop.base = locate_linkmap64(&eop.cb,pid);
#endif
    
    if ( ! eop.base ) {    
	printf("Failed to find base in program, quitting\n");
	exit(1);
    }
    
    if ( eop.cb.rcopy(eop.cb.handle,eop.base,&eop.base,sizeof(ELAN_BASE *)) == -1 ) {
	printf("Failed to copy ELAN_BASE\n");
	exit(1);
    }
    
    fetch_data_common(&eop);
    
    detach(&proc);
    
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
