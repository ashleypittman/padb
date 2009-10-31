
/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "stats_falcon.c,v 1.13 2006/11/29 10:25:23 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/stats_falcon.c,v */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <edb.h>

#include <ctype.h>

#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * This stats stuff is all new and shiny, we need to work out
 * the best way to present the information to the user
 * 
 * Currently alls stats are printed, there should be a way
 * to select this, both on subsystems and ids.
 */

struct str {
    char entry[16];
};

#if 0
extern void sf_dump_raw(void *base, size_t size)
{
    struct str *strings = (struct str *)base;
    size_t pagesize = (size_t)sysconf(_SC_PAGESIZE);
    uint64_t *numbers = (uint64_t *) ((uintptr_t)base + pagesize);
    int nstrings = pagesize / sizeof(struct str);
    int nnumbers = (size - pagesize) / sizeof(uint64_t);
    int i;
    int lastShown = -1;
    
    for ( i = 0 ; i < nstrings ; i++ ) {
	char *c = &strings[i];
	if ( c[0] != 0 )
	    printf("%d \"%.16s\"\n",i,&c[0]);
    }


    for ( i = 0 ; i < nnumbers ; i++ ) {
	if ( i==0 || numbers[i] != 0 ) {
	    if ( lastShown != i-1 ) {
		if ( i - lastShown < 10 ) {
		    int j;
		    for ( j = lastShown+1 ; j < i ; j++ )
			printf("%3d %#5x \"%#18"PRIx64"\"\n",j, j, numbers[j]);
		} else {
		    printf("<...>\n");
		}
	    }
	    printf("%3d %#5x \"%#18"PRIx64"\"\n",i, i, numbers[i]);
	    lastShown = i;
	}
    }    
}

static int _get_str (char *in, off_t *off, char *out)
{
    off_t out_off = 0;
    while ( in[*off] != ',' ) {
	out[out_off++] = in[(*off)++];
    }
    out[out_off] = '\0';
    (*off)++;
    return 0;
}

static int _get_hex (char *in, off_t *off, uint64_t *out)
{
    int res;
    long long l;
    res = sscanf(&in[*off],"%llx",&l);
    if ( res != 1 )
	return -1;
    *out = (uint64_t)l;
    while ( in[(*off)++] != ',' )
	;
    return 0;
}
#endif

static char *get_str (char *out, char *in)
{
    while ( *in != ',' ) {
	*out++ = *in++;
    }
    *out = '\0';
    in++;
    return in;
}

static char *get_hex (uint64_t *out, char *in)
{
//    printf("%p %p\n",out,in);
    *out = 0;
    sscanf(in, "%" PRIx64, out);
    while ( *in++ != ',' )
	;
    return in;
}

static char *get_dec (uint64_t *out, char *in)
{
    sscanf(in, "%" PRId64, out);
    while ( *in++ != ',' )
	;
    return in;
}

extern void sf_params_init (struct sf_params *sfp)
{
    if ( sfp->pagesize ) {
	if ( sfp->pagesize_h == 0 )
	    sfp->pagesize_h = sfp->pagesize;
    } else {
	size_t pagesize = (size_t)sysconf(_SC_PAGESIZE);
	if ( pagesize < 8192 )
	    pagesize = 8192;
	sfp->pagesize_h = sfp->pagesize = pagesize;
    }
}

/*
 * This functions isn't finished yet, it currently only works when
 * a single process has saved to file.
 * 
 * Pagesize: Assume something really big here as you don't know where
 * the file was generated.  Make a note to add the pagesize
 * to the header page somewhere.
 */
extern int sf_from_file (struct sf *sf, char *fname)
{
    size_t pagesize = (size_t)(64*1024);
    struct stat s;
    char *raw, *raw0;
    char *bufEnd;
    char *buildPage = malloc(pagesize*2);
    uint64_t *dataPage;
    int fd;
    int res;
    
    if ( ! buildPage )
	return -1;
    
    memset(buildPage,0,pagesize);
    
    dataPage = (uint64_t *)&buildPage[pagesize];
    
    memset(dataPage,0,pagesize);
    
    if ( stat(fname,&s) < 0 ) {
	free(buildPage);
	return -1;
    }
    
    /* Malloc more size than required to check the file isn't
     * growing as we read it */    
    if ( (raw = raw0 = malloc((size_t)s.st_size+1)) == NULL ) {
	free(buildPage);
	return -1;
    }
    
    bufEnd = raw + s.st_size -1;
    
    if ( (fd = open(fname,O_RDONLY)) == -1 ) {
	free(buildPage);
	free(raw);
	return -1;
    }
    
    if ( (res = read(fd,raw,(size_t)s.st_size+1)) != s.st_size ) {
	free(buildPage);
	free(raw);
	(void)close(fd);
	return -1;
    }
    
    {
	char tmp[16];
	int i;
	
	for ( i = 0 ; i < 4 ; i++ ) {
	    raw = get_str(&buildPage[i*16],raw);
	}
	
	do {
	    memset(&tmp[0],0,16);
	    raw = get_str(&tmp[0], raw);
	    i = atoi(&tmp[0]);
	    raw = get_str(&tmp[0], raw);
	    
	    strncpy(&buildPage[i*16],&tmp[0],15);
	    
	    if ( *raw == '\n' )
		raw++;
	    
	} while ( *raw != '\n' );
    }
    
    do {
	
	{
	    uint64_t index = 0;
	    int i;
	    
	    for ( i = 0 ; i < 4 ; i++ ) {
		raw = get_hex(&dataPage[i],raw);
	    }
	    
	    do {
		raw = get_dec(&index, raw);
		raw = get_hex(&dataPage[index], raw);
		
		if ( *raw == '\n' )
		    raw++;
		
	    } while ( *raw != '\n' );
	}
	
    } while ( raw != bufEnd );
    
    free(raw0);
    
    sf->base = buildPage;
    sf->database = dataPage;
    sf->valid = TRUE;
    sf->jobsize = 1;
    sf->nlocal = 1;
    sf->pagesize_d = sf->pagesize_h = pagesize;
    
    return 0;
}

extern int sf_init (struct sf *sf, struct sf_params *sfp, void *base, size_t size, int localId)
{
    struct str *esh = base;
    uint64_t *data;
    
#if 0
    sf_dump_raw(base,size);
#endif
    
    sf->base = base;
    sf->valid = TRUE;
    
    if ( strncmp((char *)&esh[1], "falcon", 16) )
	sf->valid = FALSE;
    
    if ( strncmp((char *)&esh[0], "ELAN STATS", 16) )
	sf->valid = FALSE;
    
    if ( ! sf->valid )
	return sf->valid;
    
#if 0
    if ( atoi(&esh[0]) > 3 )
	printf("Warning, more stats than I know about %d\n",atoi(&esh[0]));    	
#endif
    
    sf_params_init(sfp);
    
    sf->pagesize_d = sfp->pagesize;
    sf->pagesize_h = sfp->pagesize_h;
        
    /* Read the first stat for the job size */
    data = (uint64_t *)((char *)sf->base + (sf->pagesize_h));
    
    sf->jobsize = data[1];
    sf->nlocal = data[3];
    
    sf->database = SF_ID_TO_DATAPAGE(sf,localId);
    
    return TRUE;
}

extern struct sf *sf_copy(struct sf* in)
{
    struct sf* out;
    
    out = malloc(sizeof(struct sf));
    if ( out == 0 )
	return NULL;
    
    memcpy(out,in,sizeof(struct sf));
    
    out->base = malloc(out->pagesize_h);
    if ( out->base == NULL ) {
	free(out);
	return NULL;
    }
    memcpy(out->base, in->base, out->pagesize_h);
    
    out->database = malloc(out->pagesize_d);
    if ( out->database == NULL ) {
	free(out->base);
	free(out);
	return NULL;
    }
    memcpy(out->database, in->database, out->pagesize_d);
    
    out->nlocal = 1;
    return out;
}

#ifdef UNUSED
static void *header_to_addr(void *base, int pos)
{
    char *c = (char *)base;
    c += 16*pos;
    return c;
}
#endif

static char *header_to_str(void *base, int pos)
{
    char *c = (char *)base;
    c += 16*pos;
    if ( isalpha(*c) )
	return c;
    else
	return NULL;
}

static uint64_t header_to_int(void *base, int pos)
{
    char *c = (char *)base;
    c += 16*pos;
    
    if ( isdigit(*c) )
	return (uint64_t)atoi((void *)c);
    else
	return -1;
}

extern uint64_t sf_getSysCount(struct sf *sf)
{
    return header_to_int(sf->base, 3);
}

extern uint64_t sf_getId(struct sf *sf, char *name)
{
    uint64_t i;
    
    uint64_t count = sf_getSysCount(sf);
    
    int myoffset = 4;
    
    for ( i = 0 ; i < count ; i++ ) {
	
	if ( !strncasecmp(name,header_to_str(sf->base,myoffset),15) )
	    return i;
	
	myoffset += 2;
    }
    return -1;
}

/* Convert from subsystem id to name */
extern char *sf_getName(struct sf *sf, uint64_t id)
{
    if ( id > header_to_int(sf->base, 3) )
	return NULL;
    
    return header_to_str(sf->base,4+(id*2));
}

/* Return the offset of the names of the stats for a given subsystem */
extern int sf_getStatsNames(struct sf *sf, int id)
{
    int i = 4 + (id * 2);
    int pos = header_to_int(sf->base,i+1);
    return pos;
}

/* Find the number of stats of a type */
extern int sf_getStatsNameCount(struct sf *sf, int names, int stat_type)
{
    int tcount = header_to_int(sf->base, 2);
    if ( stat_type > tcount )
	return -1;
    return header_to_int(sf->base, stat_type + names);
}

extern char *sf_getStatsNameDesc(struct sf *sf, int names, int stat_type, int id)
{
    int offset = id + names + header_to_int(sf->base, 2);
    while (stat_type--) {
	offset += sf_getStatsNameCount(sf,names,stat_type);
    }
    return header_to_str(sf->base, offset);
}

extern int sf_vp(struct sf *sf, int local)
{
    uint64_t *data = SF_ID_TO_DATAPAGE(sf,local);
    
    if ( local < 0 || local >= sf->nlocal )
	return -1;
    
    return data[0];
}

extern struct elan_sysInstanceE *sf_dataGetInst(struct sf *sf, uint64_t type, int id)
{
    uint64_t *data = sf->database;
    int offset = 4;
    int found = 0;
    
    while ( offset != 0 ) {
	struct elan_sysInstanceE *inst = (struct elan_sysInstanceE *)&data[offset];
	
	offset = inst->next;
	
	if ( inst->type != type )
	    continue;

#if 0
	printf("Subsystem %"PRId64": '%s'.  Handle: %#"PRIx64" id: %#"PRIx64" rail: %"PRId64"\n",
	       inst->type,
	       sf_getName(sf,inst->type),
	       inst->handle,
	       inst->id,
	       inst->rail);
#endif
	
	if ( found == id )
	    return inst;
	
	found++;
    };
    return NULL;
}

extern uint64_t *elan_dataGetStats(struct sf *sf, struct elan_sysInstanceE *inst)
{
    uint64_t *data = sf->database;
    if ( ! inst->stats )
	return NULL;
    return &data[inst->stats];
}

static void merge_count(uint64_t *inout, uint64_t *in)
{
    inout[0] += in[0];
}

static void merge_tally(uint64_t *inout, uint64_t *in)
{
    inout[0] += in[0];
    inout[1] += in[1];
    inout[2] += in[2];
}

static void merge_bin(uint64_t *inout, uint64_t *in)
{
    int i;
    
    /* The bins and overflow */
    for ( i = 0 ; i < 31 ; i++ )
	inout[i] += in[i];
    
    /* Min */
    if ( in[32] < inout[32] )
	inout[32] = in[32];
    
    /* Max */
    if ( in[33] > inout[33] )
	inout[33] = in[33];
    
    /* nob */
    inout[34] += in[34];
}

static void merge_attr(uint64_t *inout, uint64_t *in)
{
    if ( in[0] != inout[0] )
	inout[0] = -1;
}

static void merge_all (struct sf *sf, int type, uint64_t *inout, uint64_t *in)
{
    struct str *strings = (struct str *)sf->base;
    
    int nStatTypes = atoi((const char *)&strings[2]);
    
    int desc = atoi((const char *)&strings[(5+(type*2))]);
    int nCount = atoi((const char *)&strings[desc]);
    int nTally = atoi((const char *)&strings[desc+1]);
    int nBin = atoi((const char *)&strings[desc+2]);
    int nAttr = nStatTypes >= 4 ? atoi((const char *)&strings[desc+3]) : 0;
    int i;
    
    desc += nStatTypes;
    
    for ( i = 0 ; i < nAttr ; i++ ) {
	int offset = i+nCount+(nTally*3)+(nBin*35);
	merge_attr(&inout[offset],&in[offset]);
    }
    
    
    for ( i = 0 ; i < nCount ; i++ ) {
	int offset = i;
	merge_count(&inout[offset],&in[offset]);
    }
    
    for ( i = 0 ; i < nTally ; i++ ) {
	int offset = (i*3)+nCount;
	merge_tally(&inout[offset],&in[offset]);
    }
    
    for ( i = 0 ; i < nBin ; i++ ) {
	int offset = (i*35)+nCount+(nTally*3);
	merge_bin(&inout[offset],&in[offset]);
    }
}

static int inst_cmp (struct elan_sysInstanceE *l, struct elan_sysInstanceE *r)
{
    if ( l->type != r->type )
	return FALSE;
    
    if ( l->id != r->id )
	return FALSE;
    
    return TRUE;
}

static struct elan_sysInstanceE *find_inst (struct sf *sf, struct elan_sysInstanceE *instIn)
{
    struct elan_sysInstanceE *inst = &sf->header->instance;
    
    do {
	if ( inst_cmp(inst,instIn) ) {
	    return inst;
	}
	
	inst = inst->next ? (struct elan_sysInstanceE *)&sf->database[inst->next] : NULL;
    } while ( inst );
    
    return inst;
}

extern void sf_combine (struct sf *out, struct sf *in)
{
    uint64_t type;
    uint64_t ntypes = sf_getSysCount(out);
    struct elan_sysInstanceE *instIn, *instOut;
    int id;
    
    for ( type = 0 ; type < ntypes ; type++ ) {
	
	id = 0;
	instIn = sf_dataGetInst(in,type,id);
	
	while ( instIn ) {
	    
	    instOut = find_inst(out,instIn);
	    if ( instOut && ( instOut->stats != 0 ) && (instIn->stats!=0) )
		merge_all(out, type, &out->database[instOut->stats], &in->database[instIn->stats]);
	    
	    id++;
	    instIn = sf_dataGetInst(in,type,id);
	}
    }
}

char *bin_sizes[] = { "0 bytes",
		      "1 byte",
		      "2 bytes",
		      "4 bytes",
		      "8 bytes",
		      "16 bytes",
		      "32 bytes",
		      "64 bytes",
		      "128 bytes",
		      "256 bytes",
		      "512 bytes",
		      "1kb",
		      "2kb",
		      "4kb",
		      "8kb",
		      "16kb",
		      "32kb",
		      "64kb",
		      "128kb",
		      "256kb",
		      "512kb",
		      "1mb",
		      "2mb",
		      "4mb",
		      "8mb",
		      "16mb",
		      "32mb",
		      "64mb",
		      "128mb",
		      "256mb",
		      "512mb",
		      "overflow"};

char *scales[] = { "Bytes",
		   "Kilobytes",
		   "Megabytes",
		   "Gigabytes",
		   "Terabytes",
		   "Petabytes",
		   "Exabytes"};

#define BIN_ENTRY "%9s: %10"PRId64

static void show_bin_contents (uint64_t *data, int *entries)
{
    if ( entries[0] == -1 )
	return;
    
    if ( entries[1] == -1 ) {
	printf("  "BIN_ENTRY"\n",
	       bin_sizes[entries[0]],
	       data[entries[0]]);
	entries[0] = -1;
	return;
    }
    
    if ( entries[2] == -1 ) {
	printf("  "BIN_ENTRY" "BIN_ENTRY"\n",
	       bin_sizes[entries[0]],
	       data[entries[0]],
	       bin_sizes[entries[1]],
	       data[entries[1]]);
	entries[0] = -1;
	entries[1] = -1;
	return;
    }
    
    printf("  "BIN_ENTRY" "BIN_ENTRY" "BIN_ENTRY"\n",
	   bin_sizes[entries[0]],
	   data[entries[0]],
	   bin_sizes[entries[1]],
	   data[entries[1]],
	   bin_sizes[entries[2]],
	   data[entries[2]]);
    entries[0] = -1;
    entries[1] = -1;
    entries[2] = -1;
    return;
}

static void bin_maybe_show(uint64_t *data, int *entries, int entry)
{
    int i;
    for ( i = 0 ; i < 3 ; i++ )
	if ( entries[i] == -1 ) {
	    entries[i] = entry;
	    break;
	}
    if ( i == 2 )
	show_bin_contents(data, entries);
}

/* Print stats for a instance, return offset of next inst */
static uint64_t dump_stat_inst (char *base, uint64_t *data, struct elan_sysInstanceE *inst)
{
    int offset0 = inst->stats;
    struct str *strings = (struct str *)base;
    int nStatTypes;

    if ( ! inst->stats ) {
#if 0
	printf("Subsystem '%.16s' id: %#"PRIx64" has no statistics recorded.\n",
	       (char *)&strings[(4+(inst->type*2))],
	       inst->id);
#endif
	return inst->next;
    }
    
    if ( inst->rail == ELAN_RAIL_ALL )
	printf("Subsystem '%.16s' id: %"PRId64" Handle: %#"PRIx64" rail: ELAN_RAIL_ALL\n",
	       (char *)&strings[(4+(inst->type*2))],
	       inst->id,
	       inst->handle);
    else
	printf("Subsystem '%.16s' id: %"PRId64" Handle: %#"PRIx64" rail: %"PRId64"\n",
	       (char *)&strings[(4+(inst->type*2))],
	       inst->id,
	       inst->handle,
	       inst->rail);
    
    nStatTypes = atoi((const char *)&strings[2]);
    
    {
	int desc = atoi((const char *)&strings[(5+(inst->type*2))]);
	int nCount = atoi((const char *)&strings[desc]);
	int nTally = atoi((const char *)&strings[desc+1]);
	int nBin = atoi((const char *)&strings[desc+2]);
	int nAttr = nStatTypes >= 4 ? atoi((const char *)&strings[desc+3]) : 0;
	int i;
	
	desc += nStatTypes;
	
	{
	    int offset = offset0+nCount+(nTally*3)+(nBin*35);
	    int toshow = -1;
	    for ( i = 0 ; i < nAttr ; i++ ) {
		if ( data[offset+i] != (uint64_t)-1 ) {
		    if ( toshow != -1 ) {
			printf("Attribute: '%.16s' = '%"PRId64"', '%.16s' = '%"PRId64"'\n",
			       (char *)&strings[desc+toshow+nCount+nTally+nBin],
			       data[offset+toshow],
			       (char *)&strings[desc+i+nCount+nTally+nBin],
			       data[offset+i]);
			toshow = -1;
		    } else { 
			toshow = i;
		    }
		}
	    }
	    if ( toshow != -1 ) {
		printf("  Attribute: '%.16s' = '%"PRId64"'\n",
		       (char *)&strings[desc+toshow+nCount+nTally+nBin],
		       data[offset+toshow]);
	    }
	}
	
	{
	    int offset = offset0;
	    int toshow = -1;
	    for ( i = 0 ; i < nCount ; i++ ) {
		if ( data[offset+i] ) {
		    if ( toshow != -1 ) {
			printf("  Counter: '%.16s' = '%"PRId64"', '%.16s' = '%"PRId64"'\n",
			       (char *)&strings[desc+toshow],
			       data[offset+toshow],
			       (char *)&strings[desc+i],
			       data[offset+i]);
			toshow = -1;
		    } else {
			toshow = i;
		    }
		}
	    }
	    if ( toshow != -1 ) {
		printf("  Counter: '%.16s' %"PRId64"\n",
		       (char *)&strings[desc+toshow],
		       data[offset+toshow]);
	    }
	}
	
	for ( i = 0 ; i < nTally ; i++ ) {
	    int offset = offset0+(i*3)+nCount;
	    if ( data[offset] )
		printf("%16.16s: Total: %"PRId64" Active: %"PRId64" HWM: %"PRId64"\n",
		       (char *)&strings[desc+i+nCount],
		       data[offset],
		       data[offset+1],
		       data[offset+2]);
	}
	
	for ( i = 0 ; i < nBin ; i++ ) {
	    int entries[3] = { -1,-1,-1 };
	    int offset = offset0+(i*35)+nCount+(nTally*3);
	    if ( data[offset+34] || data[offset] ) {
		double total = (double)data[offset+34];
		int scale = 0;
		int j;
		
		while ( total > 1024 ) {
		    total = total / 1024;
		    scale++;
		}
		
		printf("%16.16s: min %"PRId64" max %"PRId64" total %"PRId64" (%0.2f %s)\n",
		       (char *)&strings[desc+i+nCount+nTally],
		       data[offset+32],
		       data[offset+33],
		       data[offset+34],
		       total,
		       scale <= (sizeof(scales)/sizeof(char *)) ? scales[scale] : "something really big" );
		
		for ( j = 0 ; j < 32 ; j++ )
		    if ( data[offset+j] )
			bin_maybe_show(&data[offset], &entries[0], j);
		
		show_bin_contents(&data[offset], &entries[0]);
	    }
	}
    }
    return inst->next;
}

extern void sf_dump(struct sf *sf)
{
    uint64_t *data = sf->database;
    int offset = 4;
    
    while ( offset != 0 )
	offset = dump_stat_inst(sf->base, data, (struct elan_sysInstanceE *)&data[offset]);
    
}

extern void sf_dump_all(struct sf *sf)
{
    int local;
    int offset;
    
    /* Loop for local processes */
    for ( local = 0 ; local < sf->nlocal ; local++ ) {
	uint64_t *data = SF_ID_TO_DATAPAGE(sf,local);
	
	assert(data[1] != 0);
	
	printf("This is process %"PRId64" / %"PRId64" globally and %"PRId64" / %"PRId64" locally\n",
	       data[0],
	       data[1],
	       data[2],
	       data[3]);
	
	offset = 4;
	while ( offset != 0 )
	    offset = dump_stat_inst(sf->base, data, (struct elan_sysInstanceE *)&data[offset]);
	
	printf("Finished dumping stats for local %d/%d\n",local,sf->nlocal);
    }
    
}

extern void sf_raw_to_file(struct sf *sf, FILE *f)
{
    int i;
    void *base = sf->base;
    
    sf_header_to_file(stdout, 
		      base,
		      sf->pagesize_h);
    
    base += sf->pagesize_h;
    /* For each of the parallel processes */
    for ( i = 0 ; i < sf->nlocal ; i++ ) {
	sf_content_to_file(stdout,base,sf->pagesize_d);
	base += sf->pagesize_d;
    }
}

extern void sf_dump_vp(struct sf *sf,int vp)
{
    int local;
    int offset;
    
    /* Loop for local processes */
    for ( local = 0 ; local < sf->nlocal ; local++ ) {
	uint64_t *data = SF_ID_TO_DATAPAGE(sf,local);
	
	assert(data[1] != 0);
	
	if ( data[0] != vp )
	    continue;
	
	printf("This is process %"PRId64" / %"PRId64" globally and %"PRId64" / %"PRId64" locally\n",
	       data[0],
	       data[1],
	       data[2],
	       data[3]);
	
	offset = 4;
	while ( offset != 0 )
	    offset = dump_stat_inst(sf->base, data, (struct elan_sysInstanceE *)&data[offset]);
    }
    
    
    
}

/* Print stats for a instance, return offset of next inst */
static uint64_t set_debug_inst (char *base, uint64_t *data, struct elan_sysInstanceE *inst, uint64_t debug)
{
    struct str *strings = (struct str *)base;
    
    if ( ! strcmp("Core", (char *)&strings[(4+(inst->type*2))]) ) {
	inst->debugFlags = debug;
    }
    
    /* Belt and braces... */
    inst->debugFlags = debug;
    return inst->next;
}

extern void sf_set_debug_vp(struct sf *sf,int vp,uint64_t debug)
{
    int local;
    int offset;
    
    /* Loop for local processes */
    for ( local = 0 ; local < sf->nlocal ; local++ ) {
	uint64_t *data = SF_ID_TO_DATAPAGE(sf,local);
	
	assert(data[1] != 0);
	
	if ( vp != -1 && data[0] != vp )
	    continue;
	
	offset = 4;
	while ( offset != 0 )
	    offset = set_debug_inst(sf->base, data, (struct elan_sysInstanceE *)&data[offset], debug);
    }
}

void sf_header_to_file (FILE *f, char *header, size_t pagesize)
{
    int nstrings = pagesize/16;
    int i;
    int written = 0;
    
    written += fprintf(f,"%.16s,%.16s,",&header[0],&header[16]);
    header += 32;
    written += fprintf(f,"%.16s,%.16s,",&header[0],&header[16]);
    header += 32;
    
    for ( i = 4 ; i < nstrings ; i++ ) {
	if ( header[0] != 0 ) {
	    written += fprintf(f,"%d,%.16s,",i,&header[0]);
	}
	
	header += 16;
	
	if ( written > 60 ) {
	    fprintf(f,"\n");
	    written = 0;
	}
    }
    fprintf(f,"\n\n");
}

void sf_content_to_file (FILE *f, uint64_t *numbers, size_t pagesize)
{
    int nnumbers = pagesize/(sizeof(uint64_t));
    int i;
    int written = 0;
    
    written += fprintf(f,"%#"PRIx64",%#"PRIx64",%#"PRIx64",%#"PRIx64",",
		       numbers[0],
		       numbers[1],
		       numbers[2],
		       numbers[3]);
    
    for ( i = 4 ; i < nnumbers ; i++ ) {
	if (  numbers[i] != 0 ) {
	    written += fprintf(f,"%d,%#"PRIx64",",i, numbers[i]);
	}
	if ( written > 56 ) {
	    fprintf(f,"\n");
	    written = 0;
	}
    }    
    fprintf(f,"\n\n");
}


/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
