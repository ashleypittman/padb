/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 * Copyright (c) 2009, Ashley Pittman.
 */

#ident "elfN.c,v 1.14 2005-11-03 11:23:04 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/elfN.c,v */

#include <string.h>
#include <stdlib.h>

#include <dlfcn.h>
#include <unistd.h>
#include "mpi_interface.h"

struct dll_entry_points {
    char *(*dll_error_string)           (int);
    void  (*setup_basic_callbacks)      (mqs_basic_callbacks *);
    int   (*setup_image)                (mqs_image *, mqs_image_callbacks *);
    int   (*image_has_queues)           (mqs_image *, char **);
    int   (*setup_process)              (mqs_process *, mqs_process_callbacks *);
    int   (*process_has_queues)         (mqs_process *, char **);
    void  (*update_communicator_list)   (mqs_process *);
    int   (*setup_communicator_iterator)(mqs_process *);
    int   (*get_communicator)           (mqs_process *, mqs_communicator *);
    int   (*next_communicator)          (mqs_process *);
    int   (*get_global_rank)            (mqs_process *);
    int   (*get_comm_coll_state)        (mqs_process *, int, int *, int *);
    int   (*get_comm_group)             (mqs_process *, int *);
    int   (*setup_operation_iterator)   (mqs_process *, int);
    int   (*next_operation)             (mqs_process *, mqs_pending_operation *);
};

struct image {
    mqs_image_info *blob;
};

struct process {
    struct image *image;
    int rank;
    mqs_process_info *blob;
};

struct type {
    char   name[128];
    int    size;
};

struct dll_entry_points dll_ep = {};

char *collective_names[] = { "Barrier",
			     "Bcast",
			     "Allgather",
			     "Allgatherv",
			     "Allreduce",
			     "Alltoall",
			     "Alltoallv",
			     "Reduce_Scatter",
			     "Reduce",
			     "Gather",
			     "Gatherv",
			     "Scan",
			     "Scatter",
			     "Scatterv" };

char *op_types[] = { "pending_send",
		     "pending_receive",
		     "unexpected_message" };

char *op_status[] = { "pending",
		      "matched",
		      "complete" };

void show_string (char *desc, char *str)
{
    printf("zzz: str:%d %s\n%s\n",
	   (int) strlen(str),
	   desc,
	   str);
}

void show_warning (const char *msg)
{
    show_string("warning",(char *)msg);
}

void show_dll_error_code (int res)
{
    char *msg;
    msg = dll_ep.dll_error_string(res);
    show_string("dllerror",msg);
}

void die (char *msg)
{
    show_string("dmsg",msg);
    show_string("exit","die");

    fflush(NULL);
    exit(0);
}

void die_with_code (int res, char *msg)
{
    show_dll_error_code(res);
    die(msg);
}

#define QUERY_SIZE 1280

/* Query (via proxy) gdb for the answer to a question,
 * store the restult in ans, returns -1 on failure */
int ask(char *req, char *ans)
{
    char buff[QUERY_SIZE+3];
    int nbytes;
    printf("req: %s\n",req);
    fflush(NULL);    
    nbytes = read(0,buff,QUERY_SIZE+3);
    if ( nbytes < 0 )
	return -1;
    if ( memcmp(buff,"ok ",3 ) )
	return -1;
    buff[nbytes-1] = '\000';
    memcpy(ans,&buff[3],nbytes -3);
    return 0;
}

void *find_sym (char *type, char *name) {
    char req[128];
    char ans[128];
    int i;
    void *addr = NULL;
    sprintf(req,"%s %s",type,name);
    
    i = ask(req,ans);
    if ( i != 0 )
	return NULL;
    
    i = sscanf(ans, "%p",&addr);
    if ( i != 1 ) {
	printf("Failed sscanf %d %p\n",i,addr);
	return NULL;
    }
    
    return addr;
}

struct mqs_basic_callbacks bcb;
struct mqs_image_callbacks icb;
struct mqs_process_callbacks pcb;

void show_msg (const char *msg)
{
    show_string("dlldebugmessage",(char *)msg);
}

char *get_msg (int msg )
{
    return("Error: get_msg");
}

void image_put (mqs_image *image, mqs_image_info *blob)
{
    struct image *i = (struct image *)image;
    i->blob = blob;
}

mqs_image_info *image_get (mqs_image *image)
{
    struct image *i = (struct image *)image;
    return i->blob;
}

void process_put (mqs_process *process, mqs_process_info *blob)
{
    struct process *p = (struct process *)process;
    p->blob = blob;
}

mqs_process_info *process_get (mqs_process *process)
{
    struct process *p = (struct process *)process;
    return p->blob;
}

void get_type_size (mqs_process *proc, mqs_target_type_sizes *ts)
{
    ts->short_size     = sizeof(short);
    ts->int_size       = sizeof(int);
    ts->long_size      = sizeof(long);
    ts->long_long_size = sizeof(long long);
    ts->pointer_size   = sizeof(void *);
}

int find_function (mqs_image *image, char *name, mqs_lang_code lang, mqs_taddr_t *addr)
{
    void *base;
    base = find_sym("func",name);
    if ( base ) {
	if ( addr )
	    *addr = (mqs_taddr_t)base;
	return mqs_ok;
    }
    return mqs_no_information;
}

int find_symbol (mqs_image *image, char *name, mqs_taddr_t *addr)
{
    void *base = find_sym("sym",name);
    if ( base ) {
	if ( addr ) {
	    *addr = (mqs_taddr_t)base;
	}
	return mqs_ok;
    }
    return mqs_no_information;
}

int req_to_int (char *req,int *res)
{
    char ans[128];
    int i;
    
    i = ask(req,ans);
    if ( i != 0 )
	return -1;
    i = sscanf(ans, "%d",res);
    if ( i != 1 ) {
	return -1;
    }
    return 0;
}

mqs_type *find_type (mqs_image *image, char *name, mqs_lang_code lang)
{
    char req[128];
    int i;
    struct type *t = malloc(sizeof(struct type));
    if ( ! t )
	return NULL;
    
    strncpy(t->name,name,128);
    sprintf(req,"size %s",name);
    
    i = req_to_int(req,&t->size);
    if ( i != 0 )
	return NULL;
    
    return (mqs_type *)t;
}

int find_offset (mqs_type *type, char *name)
{
    char req[128];
    int i,offset;
    struct type *t = (struct type *)type;
    
    sprintf(req,"offset %s %s",t->name,name);
    
    i = req_to_int(req,&offset);
    if ( i != 0 )
	return -1;
    
    return offset;
}

int find_sizeof (mqs_type *type)
{
    struct type *t = (struct type *)type;
    return t->size;
}

int find_rank (mqs_process *process)
{
    struct process *p = (struct process *)process;
    if ( p->rank == -1 )
	show_warning("DLL called find_rank before setup_process!");
    return p->rank;
}

mqs_image *find_image (mqs_process *process)
{
    struct process *p = (struct process *)process;
    return (mqs_image *)p->image;
}

int _find_data (mqs_process *proc, mqs_taddr_t addr, int size, void *base)
{
    char req[128];
    char _ans[QUERY_SIZE];
    char *ans = &_ans[0];
    int i;
    char *local = base;
    
    char *ptr;
    if ( size == 0 )
	return mqs_ok;
    
    sprintf(req,"data %p %d",(void *)addr,size);
    
    i = ask(req,ans);
    if ( i != 0 )
	return mqs_no_information;
    
    for ( i = 0 ; i < size ; i++ ) {
	char *e;
	int j;
	e = strtok_r(ans," ",&ptr);
	ans = NULL;
	
	sscanf(e,"%i",&j);
	local[i] = j;
	// printf("Converted [%d] '%s' '%x'\n",i,e,j);
    }
    
    return mqs_ok;
}

// Data comes back as 0x?? for each char, space required is (N*5)-1
// 256 bytes read need 1279 bytes of space
int find_data (mqs_process *proc, mqs_taddr_t addr, int size, void *base)
{
    char *local = base;
    int offset = 256;
    int res;
    // printf("Trying to read data for %d from %p\n",size,(void *)addr);    
    if ( ! addr ) {
	return mqs_no_information;
    }
    do {
	if ( offset > size )
	    offset = size;
	res = _find_data(proc,addr,offset,local);
	if ( res != mqs_ok )
	    return mqs_no_information;
	
	addr  += offset;
	local += offset;
	size  -= offset;
    } while ( size );
    return mqs_ok;
}

void convert_data (mqs_process *proc, const void *a, void *b, int size)
{
    memcpy(b,(void *)a,size);
}

int fetch_string (void *bc,void *local, mqs_taddr_t remote, int size)
{
    char req[128];
    char ans[128];
    int i;
    
    sprintf(req,"string %d %p",size,(void *)remote);
    i = ask(req,ans);
    if ( i != 0 )
	return -1;
    strncpy(local,ans,size);
    return 0;
}

int fetch_image (char *local)
{
    char ans[QUERY_SIZE];
    int i = ask("image", ans);
    if ( i != 0 )
	return -1;
    strncpy(local,ans,QUERY_SIZE);
    return 0;
}

int msg_id = 0;

int show_comm (struct process *p, mqs_communicator *comm, int c)
{
    if ( comm->local_rank >= 0 )
	printf("out: c:%d rank:%d\n",
	       c,
	       (int)comm->local_rank);
    
    printf("out: c:%d size:%d\n",
	   c,
	   (int)comm->size);
    
    printf("out: c:%d str:%d name\n%s\n",
	   c,
	   (int) strlen(comm->name),
	   comm->name);
    
    printf("out: c:%d id:%ld\n",
	   c,
	   comm->unique_id);
    
    return c++; /* This is not a political statement although if it was I'd stand by it */
}

void show_comm_members (mqs_process *target_process, mqs_communicator *comm, int comm_id)
{
    int *ranks = malloc(comm->size*sizeof(int));
    int r = dll_ep.get_comm_group(target_process,ranks);
    if ( r == mqs_ok ) {
	int i;
	for ( i = 0 ; i < comm->size ; i++ ) {
	    printf("out: c:%d rt:%d\n",
		   comm_id,
		   ranks[i]);
	}
    }
    free(ranks);
}

void show_comm_coll_state (mqs_process *target_process, mqs_communicator *comm, int comm_id)
{
    int i;
    for ( i = 0 ; i < 14 ; i++ ) {
	int seq = -1;
	int active = -1;
	int r = dll_ep.get_comm_coll_state(target_process,i,&seq,&active);
	if ( r == mqs_ok ) {
	    if ( seq != 0 )
		printf("comm%d: Collective '%s': call count %d, %sactive\n",
		       comm_id,
		       collective_names[i],
		       seq,
		       active ? "" : "not ");
	} else if ( r != mqs_no_information ) {
	    show_dll_error_code(r);
	}
    }
}

void show_op (mqs_pending_operation *op, int msgid, int type)
{
    int i;
    int all = 0;
    
    if ( type == mqs_pending_sends || op->status == mqs_st_matched || op->status == mqs_st_complete )
	all = 1;
    
    printf("msg%d: Operation %d (%s) status %d (%s)\n",
	   msgid,type,op_types[type],op->status,op_status[op->status]);
    printf("msg%d: Rank local %d global %d\n",
	   msgid,(int)op->desired_local_rank, (int)op->desired_global_rank);
    if ( all )
	printf("msg%d: Actual local %d global %d\n",
	       msgid, (int)op->actual_local_rank, (int)op->actual_global_rank);
    if ( all ) 
	printf("msg%d: Size desired %d actual %d\n",
	       msgid, (int)op->desired_length, (int)op->actual_length);
    else
	printf("msg%d: Size desired %d\n",
	       msgid, (int)op->desired_length);
    printf("msg%d: tag_wild %d\n",msgid,op->tag_wild);
    if ( all ) 
	printf("msg%d: Tag desired %d actual %d\n",
	       msgid, (int)op->desired_tag, (int)op->actual_tag);
    else
	printf("msg%d: Tag desired %d\n",msgid, (int)op->desired_tag);
    printf("msg%d: system_buffer %d\n",msgid,op->system_buffer);
    printf("msg%d: Buffer %p\n",msgid,(void *)op->buffer);
    
    i = 0;
    do {
	if ( op->extra_text[i][0] )
	    printf("msg%d: '%s'\n",msgid,op->extra_text[i]);
	else
	    i = 10;
    } while ( i++ < 5 );
}

void load_ops (mqs_process *target_process,int type)
{

    int res = dll_ep.setup_operation_iterator(target_process,type);
    if ( res != mqs_ok ) {
	if ( res != mqs_ok && res != mqs_no_information )
	    printf("Setup operation iterator failed %d for type %d\n",res,type);
	return;
    }
    
    do {
	mqs_pending_operation op = {};
	res = dll_ep.next_operation(target_process,&op);
	if ( res == mqs_ok ) {
	    show_op(&op,msg_id,type);
	    msg_id++;
	} else if ( res != mqs_end_of_list ) {
	    printf("Res from mqs_pending_operation is %d type %d\n",res,type);
	}
	
    } while ( res == mqs_ok );
}

void load_all_ops (mqs_process *target_process)
{
    msg_id = 0;
    load_ops(target_process,mqs_pending_receives);
    load_ops(target_process,mqs_unexpected_messages);
    load_ops(target_process,mqs_pending_sends);
}

#define DLSYM_LAX(VAR,HANDLE,NAME) VAR.NAME = dlsym(HANDLE,"mqs_" #NAME)

#define DLSYM(VAR,HANDLE,NAME) do {			      \
	if ( (DLSYM_LAX(VAR,HANDLE,NAME)) == NULL ) {	      \
	    show_warning("Failed to load symbol mqs_" #NAME); \
	    return -1;					      \
	}						      \
    } while (0)

/* Try and load the dll from a given filename, returns true if successfull.
 * populates the contents of dll_ep if true.
 */
int load_msgq_dll(char *filename)
{
    void *dlhandle;
    
    dlhandle = dlopen(filename,RTLD_NOW);
    if ( ! dlhandle ) 
	return -1;

    DLSYM(dll_ep,dlhandle,setup_basic_callbacks);
    DLSYM(dll_ep,dlhandle,setup_image);
    DLSYM(dll_ep,dlhandle,image_has_queues);
    DLSYM(dll_ep,dlhandle,setup_process);
    DLSYM(dll_ep,dlhandle,process_has_queues);
    DLSYM(dll_ep,dlhandle,dll_error_string);
    DLSYM(dll_ep,dlhandle,update_communicator_list);
    DLSYM(dll_ep,dlhandle,setup_communicator_iterator);
    DLSYM(dll_ep,dlhandle,get_communicator);
    DLSYM(dll_ep,dlhandle,next_communicator);
    DLSYM(dll_ep,dlhandle,setup_operation_iterator);
    DLSYM(dll_ep,dlhandle,next_operation);
    DLSYM(dll_ep,dlhandle,get_comm_group);

    DLSYM_LAX(dll_ep,dlhandle,get_global_rank);
    DLSYM_LAX(dll_ep,dlhandle,get_comm_coll_state);
    return 0;
}

#define PATH_MAX 1024

int
main ()
{
    int res;
    char *dll_name;
    int comm_id = 0;
    
    struct image   image;
    struct process process;

    mqs_image   *target_image   = (mqs_image   *)&image;
    mqs_process *target_process = (mqs_process *)&process;

    dll_name = getenv("MPINFO_DLL");
    if ( ! dll_name ) {
	
	void *base = find_sym("sym","MPIR_dll_name");
	if ( ! base ) {
	    die("Could not find MPIR_dll_name symbol");
	}
	dll_name = malloc(PATH_MAX);
	if ( fetch_string(NULL,dll_name,(mqs_taddr_t)base,PATH_MAX) != 0 ) {
	    die("Could not read value of MPIR_dll_name");
	}
    }

    if ( load_msgq_dll(dll_name) != 0 ) {
	die("Could not load symbols from dll");
    }
        
    bcb.mqs_malloc_fp           = malloc;
    bcb.mqs_free_fp             = free;
    bcb.mqs_dprints_fp          = show_msg;
    bcb.mqs_errorstring_fp      = get_msg;
    bcb.mqs_put_image_info_fp   = image_put;
    bcb.mqs_get_image_info_fp   = image_get;
    bcb.mqs_put_process_info_fp = process_put;
    bcb.mqs_get_process_info_fp = process_get;
    
    dll_ep.setup_basic_callbacks(&bcb);
    
    icb.mqs_get_type_sizes_fp = get_type_size;
    icb.mqs_find_function_fp  = find_function;
    icb.mqs_find_symbol_fp    = find_symbol;
    icb.mqs_find_type_fp      = find_type;
    icb.mqs_field_offset_fp   = find_offset;
    icb.mqs_sizeof_fp         = find_sizeof;
    
    res = dll_ep.setup_image(target_image,&icb);
    if ( res != mqs_ok ) {
	die_with_code(res,"setup_image() failed");
    }
    
    {
	char *user_message = NULL;
	res = dll_ep.image_has_queues(target_image,&user_message);
	if ( user_message ) {
	    show_string("ihqm",user_message);
	}
	if ( res != mqs_ok ) {
	    die_with_code(res,"image_has_queues() failed");
	}
    }
    
    pcb.mqs_get_global_rank_fp = find_rank;
    pcb.mqs_get_image_fp       = find_image;
    pcb.mqs_fetch_data_fp      = find_data;
    pcb.mqs_target_to_host_fp  = convert_data;
    
    process.rank  = -1;
    process.image = &image;
    
    res = dll_ep.setup_process(target_process,&pcb);
    if ( res != mqs_ok ) {
	die_with_code(res,"mqs_setup_process() failed");
    }

    if ( dll_ep.get_global_rank ) {
	process.rank = dll_ep.get_global_rank(target_process);
    } else {
	/* Load the rank into p */
	req_to_int("rank", &process.rank);
    }
    
    {
	char *user_message = NULL;
	res = dll_ep.process_has_queues(target_process,&user_message);
	if ( user_message )
	    show_string("phqm",user_message);
	if ( res != mqs_ok ) {
	    die_with_code(res,"process_has_queues() failed");
	}
    }
    
    dll_ep.update_communicator_list(target_process);
    
    res = dll_ep.setup_communicator_iterator(target_process);
    if ( res != mqs_ok ) {
	die_with_code(res,"setup_communicator_iterator() failed");
    }
    
    do {
	mqs_communicator comm = {};
	
	res = dll_ep.get_communicator(target_process,&comm);
	if ( res != mqs_ok ) {
	    die_with_code(res,"get_communicator() failed");
	}
	
	show_comm(&process,&comm,comm_id);
	
	if ( comm.size > 1 ) {
	    
	    if ( dll_ep.get_comm_group )
		show_comm_members(target_process,&comm,comm_id);
	    
	    if ( dll_ep.get_comm_coll_state )
		show_comm_coll_state(target_process,&comm,comm_id);
	    
	    load_all_ops(target_process);
	    
	}
	printf("done\n");
	
	res = dll_ep.next_communicator(target_process);
	comm_id++;
	
    } while ( res == mqs_ok );

    show_string("exit","ok");
    return 0;
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
