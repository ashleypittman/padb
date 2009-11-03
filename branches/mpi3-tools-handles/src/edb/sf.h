/*
 * Copyright (c) 2004 Quadrics Ltd
 */

#ident "sf.h,v 1.8 2006/11/29 10:25:23 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/sf.h,v */

/*
  The new generation stats code.
  
  This code has to perform several tasks, it has to extract stats from
  a single running task, it has to extract stats from a collection of
  running tasks and it has to work off-line on saves stats information.
  
  In the first case you are reading from sys-v shared memory and in the
  latter from file(s).  As this code can be tricky in places it would be
  best to keep as much of it the same.
  
  So - Whats needed is a in-memory representation of the stats that can
  be populated from either sys-v or flat files.  In fact given the sys-v
  layout is a in-memory format a way of loading this from a file may be
  all thats needed.
  
  There is a potential problem in that this needs to scale to 1000's of
  processes so not all the stats can be loaded at once.
  
  You then need to consider what information you are trying to extract 
  from the stats, currently this consists of four things
  
  1) Dumping the whole lot.
  
  2) Dumping a summary.
  
  3) The group-deadlock detection trick.
  
  4) Check for progress - perform a delta.
  
  Dumping the whole lot is easy, you just load them in order.  The
  group-deadlock detection can again be done by iterating through each
  process.
  
  Collecting a summary of stats is a little harder, it's entirely possible
  that each process will have a different number of subsystems in use so
  a simple overlay isn't possible.  A more complex extendiable format is
  needed here.  Using the standard in-memory format would be possible with
  the slight change that it wouldn't be page-sized and could grow on demand
  to cope with differing subsystems across the job.  Doing the progress
  check from this basis is somewhat easy.
  
  The core functions required are:
  
  start/open.
  
  process a page.
  
  finish/close?
  
  As the mechanism for loading the stats into local memory can change a callback
  function seems in order although it's not immediatly apparant how this is going
  to be initialised.
  
  as for the higher level code structure or (edb) command line options I'm not sure
  but it looks like a two dimensional matrix is needed.
  
  
  |        | shared memory | file
  +--------+---------------+------
  |dump    | -k            |
  |        |               |
  +--------+---------------+------
  |summary | -Pk           |
  |        |               |
  +--------+---------------+------
  |group   | -Pkg          |
  |deadlock|               |
  +--------+---------------+------
  
*/

/*
  Document revisions:
  
  15 character NULL-terminated strings.  I've recently changed the library
  to use 16 character non-NULL terminated strings, these can be read with
  snprintf() or %.16s formatting options.
  
  The contents of the header page is common not just to all processes on
  a node (in which case it's the same page) but to all processes in the
  job.  
  
*/

/* Public */
struct elan_sysInstanceE {
    uint64_t   type;
    uint64_t   id;
    uint64_t   handle;
    uint64_t   valid;
    uint64_t   rail;
    uint64_t   next;
    uint64_t   stats;
    uint64_t   debugFlags;
};

struct p_header {
    uint64_t                 vp;
    uint64_t                 nvp;
    uint64_t                 local;
    uint64_t                 nlocal;
    struct elan_sysInstanceE instance;
};

struct sf {
    void *base;
    union {
	uint64_t *database;
	struct p_header *header;
    };
    int valid;
    int jobsize;
    int nlocal;
    size_t pagesize_d;
    size_t pagesize_h;
};

#define SF_INITIALIER { .valid = 0 }

struct sf_params {
    int key;
    size_t pagesize;
    size_t pagesize_h;
    int parallel;
    int target_vp;
};

extern void sf_params_init (struct sf_params *sfp);

extern int sf_from_file (struct sf *sf, char *fname);


/* Initialise everything, returns true or false */
extern int sf_init(struct sf *sf, struct sf_params *sfp, void *base, size_t size, int localId);

#define SF_ID_TO_DATAPAGE(SF,ID) (uint64_t *)((char *)(SF)->base +          \
					      (SF)->pagesize_h +            \
					      ((SF)->pagesize_d * (ID)))
    
extern struct sf *sf_copy(struct sf* in);

/*
 * Code for parsing the header page.
 *
 */

/* Get the number of subsystems */
extern uint64_t sf_getSysCount(struct sf *sf);

/* Convert from name to id, eg 'Tport' to 'ETYPE_TPORT' */
extern uint64_t sf_getId(struct sf *sf, char *name);

/* Reverse of the above. */
extern char *sf_getName(struct sf *sf, uint64_t id);

/* Get a pointer to the name descriptor */
extern int sf_getStatsNames(struct sf *sf, int id);

/* Get the number of stats for the type */
extern int sf_getStatsNameCount(struct sf *sf, int names, int stat_type);

/* Get the name of stat for a subsystem, type and index */
extern char *sf_getStatsNameDesc(struct sf *sf, int names, int stat_type, int id);

extern void sf_combine(struct sf *out, struct sf* in);

/*
 * Code for parsing the stats (data) pages.
 *
 */

/* Return the vp for a given localId */
extern int sf_vp(struct sf *sf, int local);

extern struct elan_sysInstanceE *sf_dataGetInst(struct sf *sf, uint64_t type, int id);

extern uint64_t *elan_dataGetStats(struct sf *sf, struct elan_sysInstanceE *inst);

/*
 * Code for dumping the stats to stdout
 *
 */


extern void sf_dump(struct sf *sf);

/* Dump the whole lot */
extern void sf_dump_all(struct sf *sf);

extern void sf_dump_vp(struct sf *sf,int vp);

extern void sf_raw_to_file(struct sf *sf, FILE *f);
extern void sf_header_to_file (FILE *f, char *header, size_t pagesize);
extern void sf_content_to_file (FILE *f, uint64_t *numbers, size_t pagesize);

extern void sf_set_debug_vp(struct sf *sf,int vp,uint64_t debug);

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
