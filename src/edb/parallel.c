/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "parallel.c,v 1.19 2006/11/06 16:00:19 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/parallel.c,v */


#include "edb.h"

#define MACRO_BEGIN   {
#define MACRO_END     }
#include <qsnet/list.h>

#include <pthread.h>
#include <assert.h>

/***********************************************************
 *                                                         *
 * The parallel code...                                    *
 *                                                         *
 * Note this doesn't work yet, its needs some design        *
 * rethinks, you would need to take a resource id and work *
 * out the pids and base pointer yourself to do this       *
 * properly.                                               *
 *                                                         *
 ***********************************************************/

#define GLOBAL_EXIT  127
#define GLOBAL_ERROR 123

/*
 * How it works.
 *
 * This program is started by padb or some such, doesn't really matter, it runs on at least
 * 1 process per node (maybe more in future?).  vp 0 starts a server thread which does all
 * the real work.
 *
 */

enum req_type {
    R_READ,
    R_ATTACH,
    R_DETACH,
    R_QUIT
};

struct request {
    enum req_type  type;
    void          *client_handle;
    union {
	size_t     size;
	uint32_t   vp;
    };
};

enum rep_type {
    R_SIGNON,
    R_REPLY
};

struct reply {
    enum rep_type  type;
    void          *server_handle;
    void          *client_handle;
    union {
	uint32_t   vp;
	char       data[1];
    };
    struct sysv    v;
};

struct remote_process {
    struct reply r;
    struct list_head list;
    struct sf sf;
};

struct remote_vp {
    struct remote_process *rp;
    int local;
};


struct server_info {
    ELAN_QUEUE_RX *qrup;    /* Used for sending to the waiters */
    
    ELAN_QUEUE_TX *qtdown;    /* Used for sending to the waiters */
    ELAN_QUEUE    *q;
    
    long waitType;
    struct elan_state *es;
    int nvp;
    struct remote_vp *vps;
    
    struct stats_options so;
    
    struct remote_process *last_rp;
    
    struct sf_params *sfp;
    void *sb;
    size_t sbsize;
};

static void show_stats(struct server_info *si)
{
    int vp;
    struct sf *total = NULL;
    
    /* For each of the parallel processes */
    for ( vp = 0 ; vp < si->nvp ; vp++ ) {
	struct remote_process *rp = si->vps[vp].rp;
	
	/* Collect the stats */
	elan_wait(elan_get(si->es,
			   rp->r.v.base,
			   si->sb,
			   rp->r.v.size,
			   rp->r.vp),ELAN_POLL_EVENT);
	
	memset(&rp->sf, 0, sizeof(struct sf));
	
	if ( ! sf_init(&rp->sf,si->sfp,si->sb,rp->r.v.size,si->vps[vp].local) ) {
	    printf("Remote stats error\n");
	    exit(1);
	}
	
	if ( total == NULL )
	    total = sf_copy(&rp->sf);
	else
	    sf_combine(total,&rp->sf);
	
    }
    sf_dump(total);
}

static uint64_t *rvp_to_buff(struct server_info *si, int vp)
{
    struct remote_vp *rvp = &si->vps[vp];
    struct remote_process *rp = rvp->rp;
    if ( rp != si->last_rp ) {
	
	elan_wait(elan_get(si->es,
			   rp->r.v.base,
			   si->sb,
			   rp->r.v.size,
			   rp->r.vp),ELAN_POLL_EVENT);
	si->last_rp = rp;
    }
    return (si->sb + si->sfp->pagesize_h + (rvp->local * si->sfp->pagesize));
}

static char *get_header(struct server_info *si)
{
    rvp_to_buff(si,0);
    return si->sb;
}

static void raw_stats(struct server_info *si)
{
    int vp;
    
    sf_params_init(si->sfp);
    
    sf_header_to_file(stdout, 
		      get_header(si),
		      si->sfp->pagesize_h);
    
    /* For each of the parallel processes */
    for ( vp = 0 ; vp < si->nvp ; vp++ ) {
	uint64_t *data = rvp_to_buff(si,vp);
	
	sf_content_to_file(stdout,data,si->sfp->pagesize);
    }
}

static void *server_thread(struct server_info *si)
{
    struct reply *rep;
    int found = 0;
    
    struct list_head rlist;
    
    si->qtdown = elan_queueTxInit(si->es,si->q,ELAN_RAIL_ALL,0);
    
    INIT_LIST_HEAD(&rlist);
    
    /* TODO: You want to be able to accept multiple signons per node (and ignore some of them)
     * and also need a re-think if this is going to be used as a two way communications 
     * system */
    do {
	rep = elan_queueRxWait(si->qrup,NULL,si->waitType);
	
	switch(rep->type) {
	case R_SIGNON:
	{
	    struct remote_process *rp = malloc(sizeof(struct remote_process));
	    
	    if ( rp == NULL )
		exit(GLOBAL_ERROR);
	    
	    memset(rp,0,sizeof(struct remote_process));
	    memcpy(&rp->r, rep, sizeof(struct remote_process));
	    list_add(&rp->list, &rlist);
	    
	    if ( found == 0 ) {
		si->sbsize = rp->r.v.size;
		si->sb = malloc(si->sbsize);
		if (si->sb==NULL)
		    exit(GLOBAL_ERROR);
	    }
	    
	    if ( rp->r.v.size > si->sbsize ) {
		si->sbsize = rp->r.v.size;
		free(si->sb);
		si->sb = malloc(si->sbsize);
	    }
	    
	    /* If we have a elan address then use it, otherwise trust the
	     * main address that arrived over the network. */
	    if ( rp->r.v.ebase ) 
		rp->r.v.base = elan_elan2main(si->es,rp->r.v.ebase);
	    
	    elan_wait(elan_get(si->es,rp->r.v.base, si->sb, rp->r.v.size, rp->r.vp),si->waitType);
	    
	    if ( ! sf_init(&rp->sf,si->sfp,si->sb,rp->r.v.size,0) ) {
		printf("Remote stats error\n");
		exit(GLOBAL_ERROR);
	    }
	    
	    if ( verbose )
		printf("Signon of %d vps from %d, found = %d jobsize = %d\n",
		       rp->sf.nlocal,
		       rp->r.vp,
		       found,
		       rp->sf.jobsize);
	    
	    assert(rp->sf.jobsize != 0);
	    
	    if ( found == 0 ) {
		si->nvp = rp->sf.jobsize;
		si->vps = malloc(sizeof(struct remote_vp)*si->nvp);
		if ( si->vps == NULL )
		    exit(GLOBAL_ERROR);
		memset(si->vps,0,sizeof(struct remote_vp)*si->nvp);
	    } else {
		if ( si->nvp != rp->sf.jobsize )
		    exit(GLOBAL_ERROR);
	    }
	    
	    {
		int local;
		int vp;
		for ( local = 0 ; local < rp->sf.nlocal ; local++ ) {
		    vp = sf_vp(&rp->sf,local);
		    si->vps[vp].rp = rp;
		    si->vps[vp].local = local;
		}
	    }
	    
	    found += rp->sf.nlocal;
	    
	    break;
	}
	case R_REPLY:
	    break;
	}
    } while ( found != si->nvp );

    switch (si->so.todo)
    {
    case EDBSTAT_SHOW:
	show_stats(si);
	break;
    case EDBSTAT_RAW:
	raw_stats(si);
	break;
    case EDBSTAT_NOP:
	;
    }
        

    {

	/* Shutdown: This code is correct and was essential for 
	 * a while, however approx 1% of times not all the clients
	 * see this exit call, possibly due to trapping on the
	 * sender */
	struct remote_process *rp;
	struct list_head *list;
	struct request req = { .type = R_QUIT };
	
	list_for_each(list, &rlist) {
	    rp = list_entry(list, struct remote_process, list);
	    
	    if ( rp->r.vp == si->es->vp )
		continue;
	    
	    elan_wait(elan_queueTx(si->qtdown,
				   rp->r.vp,
				   &req,
				   sizeof(struct request),
				   ELAN_RAIL_ALL),
		      si->waitType);
	    
	}

    }
    
    /* Exit with GLOBAL_EXIT_OK (rms/resource.h) which will cause
     * rms to kill all processes */
    exit(GLOBAL_EXIT);
}

ELAN_GALLOC *client_ga = NULL;
ELAN_QUEUE_RX *qrs;
struct stats_options *sog;

/* Hack alert: due to problems with address space and the gallocshm/munmap/shmat
 * code in client_thread don't start the server thread until this code has
 * completed.   Failure to do so results in the server thread causing mappings
 * to magically appear just where we don't want them.
 */

static void client_thread(ELAN_STATE *s, ELAN_QUEUE_RX *qr, ELAN_QUEUE *q, struct sf_params *sfp, long waitType)
{
    ELAN_QUEUE_TX *qt = elan_queueTxInit(s,q,ELAN_RAIL_ALL,0);
    int            nvp;
    int            self_server = 0;
    
    {
	struct sf       sf = SF_INITIALIER;
	struct reply    rep = { .type = R_SIGNON };
	
	void *g = NULL;
	
	/* A bit messy this: create a global allocator at a common ebase everwhere,
	 * allocate the memory at any alignment, convert it to a main address
	 * and align it well (1mb).  Then pass the ELAN_ADDR over the network and
	 * have the server_thread do a local elan2main() on it's side of the network.
	 *
	 * Easier in elan3 however, we do not support non-consistant memory maps
	 * in the elan3 codebase so just whatever the kernel gives us and assume
	 * it's got a constant elan translation.
	 */	
	
	/* 16 Mb allocator */
	
	if ( client_ga ) {
	    /* non-aligned 2Mb size */
	    ELAN_ADDR gaddr = elan_galloc(client_ga,NULL,0,2*1024*1024);
	    
	    g = elan_elan2main(s,gaddr);
	    
	    /* Mb aligned Mb size */
	    g = (void *)ELAN_ALIGNUP(g, 1024*1024);
	    
	    rep.v.ebase = elan_main2elan(s,g);
	}
	
	rep.v.key = sfp->key;
	
	if ( attach_sysv(&rep.v,g) ) {
	    if ( ! sf_init(&sf, sfp, rep.v.base, rep.v.size,0) ) {
		printf("Cant read local elan stats\n");
		if ( s->vp == 0 ) {
		    self_server = 1;
		} else {
		    exit(1);
		}
	    }
	} else {
	    if ( verbose )
		printf("Failed to attach to shared memory key %x\n",rep.v.key);
	    if ( s->vp == 0 ) {
		self_server = 1;
	    } else {
		exit(1);
	    }
	}
	
	nvp = sf.jobsize;
	
	rep.vp = s->vp;
	
	if ( s->vp == 0 ) {
	    pthread_t *pthread_descs = malloc(sizeof(pthread_t));
	    struct server_info *si = malloc(sizeof(struct server_info));
	    
	    if ( pthread_descs == NULL || si == NULL )
		exit(1);
	
	    memset(si,0,sizeof(struct server_info));
	    si->waitType = waitType;
	    si->qrup = qrs;
	    si->q = q;
	    si->sfp = sfp;
	    si->so = *sog;
	    si->es = s;
	    if ( self_server ) {
		server_thread(si);
		exit(0);
	    } else {
		pthread_create(pthread_descs, NULL, (void *)server_thread,si);
	    }
	}
	
	elan_wait(elan_queueTx(qt,
			       0,
			       &rep,
			       sizeof(struct reply),
			       ELAN_RAIL_ALL),
		  waitType);
    }
    
    {
	struct request *req;
	
	do {
	    req = elan_queueRxWait(qr,NULL,waitType);
	    
	    switch(req->type) {
	    case R_READ:
		break;
	    case R_ATTACH:
		break;
	    case R_DETACH:
		break;
	    case R_QUIT:
		break;
	    }
	} while (req->type != R_QUIT);
	if ( verbose )
	    printf("%d: got exit request\n",s->vp);
    }
    exit(0);
    
}

/* 
 * XXX: As it stands there are (at least) two errors in the way 
 * this happens, firstly if driving this from padb -g (group
 * deadlock detection) it's entirely possible the cause is a
 * faulty cable so we will never see the signons and also hang
 * on startup.  Secondly if the job is smaller than the resource
 * we get extra instances started which fail to attach to the 
 * shared memory and exit.  This is probably OK as long as the 
 * server thread is in both the job and the resource.
 *
 * The correct way to fix both of these is to know how many signons
 * to expect and spin waiting for them all using some kind of 
 * timeout value.
 *
 * Another problem is that process 0 might die in client_thread
 * if there is no local shared memory, all client threads need to
 * signon and the server_thread needs to accept them all.
 *
 * Another problem is that some processes of the parallel job may
 * have exited and hence the local shared memory will have been torn
 * down leading to incomplete stats data.  Currently this results in
 * hang.
 */
void
go_parallel (struct sf_params *sfp, struct stats_options *so)
{
    ELAN_STATE    *s;
    
    /* The Q everyone uses - note that the TX here points to the server Q */
    ELAN_QUEUE    *q;
    ELAN_QUEUE_RX *qr;
    
    /* The server Q */
    ELAN_QUEUE    *qs;
    
    int            qSize = elan_queueMaxSlotSize(NULL);
    long           waitType = 10;
    
    s = elan_init(0);
    elan_attach(s);
    
    /* The q the slaves use, only has 1 slot */
    q = elan_allocQueue(s);
    qr = elan_queueRxInit(s,q,1,qSize,ELAN_RAIL_ALL,0);
    
    qs = elan_allocQueue(s);
    qrs = elan_queueRxInit(s,qs,16,qSize,ELAN_RAIL_ALL,0);
    
    if ( verbose )
	printf("Allocated Qs at %p %p\n",
	       q,qs);
    
    elan_enable_network (s);
    
    /* Create a vaddr global allocator if running on elan4 so we can deal
     * with potentially non-global main2elan translations, this isn't 
     * needed (and indeed doesn't work) on elan3 as the underlying library
     * doesn't support that anyway */
    if ( elan_queueMaxSlotSize(s) != 320 ) {
	client_ga = elan_gallocVaddrCreate(s,0xf00ff00f00000000,1024*1024*16,0);
	assert(client_ga);
    }
    
    if ( verbose )
	printf("Allocated ga at %p\n",
	       client_ga);
    
    sog = so;
    
    client_thread(s, qr, qs, sfp, waitType);
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
