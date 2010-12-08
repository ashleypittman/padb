/*
 * Copyright (c) 2004 Quadrics Ltd
 */

#ident "edb.h,v 1.12 2005/10/21 10:53:03 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/edb.h,v */

#include <elan/elan.h>

#include <elan/etrace.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef offsetof
#define offsetof(T,F)	((int)&(((T *)0)->F))
#endif

#include <string.h>

#include <inttypes.h>

#ifdef S_SPLINT_S
#define PRIx64 "llx"
#define PRIu64 "llu"
#define PRId64 "lld"
#define PRIx32 "x"
#define PRIu32 "u"
#define PRId32 "d"
#endif

#include <elan4/lib_elf.h>

struct cproc {
    char           *cname;
    int             coreFd;
    char           *binName;
    int             binFd;
    int             wordSize;
    Elf            *coreElf;
    union {
	struct {
	    Elf64_Phdr     *corePhdr64;
	    Elf64_Ehdr     *coreEhdr64;
	};
	struct {
	    Elf32_Phdr     *corePhdr32;
	    Elf32_Ehdr     *coreEhdr32;
	};  
    };
};

struct local_eop {
    struct etrace_ops cb;
    uint64_t base;
};

struct x_info {
    FILE *file;
    int iwidth;
    char *indent;
    char *rindent;
};

struct elan4_trap_info64 {
    uint64_t          Magic;               /* Exception magic */
    uint64_t          elan3Compat;         /* Elan3 compatability pointer */
    uint64_t          trap;                /* QsNetII trap information */
    uint64_t          msg;                 /* Textual string representing error */
    uint64_t          base;                /* Pointer to elan_base (libelan.so) */
    uint64_t          exceptionMain;
    uint64_t          exceptionThread;
    uint64_t          cq_info;             /* Pointer to a copy of the cq if cproctrap */
    uint64_t          expansion[13];
    int               msgSize;             /* Size of message buffer */
    int               msgLen;              /* Length of actual message */
};


struct elan4_trap_info32 {
    uint32_t          Magic;               /* Exception magic */
    uint32_t          elan3Compat;         /* Elan3 compatability pointer */
    uint32_t          trap;                /* QsNetII trap information */
    uint32_t          msg;                 /* Textual string representing error */
    uint32_t          base;                /* Pointer to elan_base (libelan.so) */
    uint32_t          exceptionMain;
    uint32_t          exceptionThread;
    uint32_t          cq_info;              /* Pointer to a copy of the cq if cproctrap */
    uint32_t          expansion[13];
    int               msgSize;             /* Size of message buffer */
    int               msgLen;              /* Length of actual message */
};

#include <sys/ipc.h>
#include <sys/shm.h>

struct sysv {
    ELAN_ADDR ebase;
    void *base;
    int key;
    size_t size;
    int attached;
    int id;
    struct shmid_ds sid;
};

extern int verbose;

extern /*@null@*/ struct x_info *x_init (size_t width);
extern void x_free (/*@out@*/ /*@only@*/ struct x_info *xi);

extern void dump_ti (struct x_info *xi, struct tport_info *ti);

extern void trace_init(void);

extern int read_from_file (int fd, void *base, off_t offset, size_t len);

extern int fetch_string (struct etrace_ops *ops, char *local, uint64_t remote, size_t len);

extern void fetch_data_common(struct local_eop *eop);


/* From elfN.c */
/* elf64.c */
extern void dumpMemory32 (struct cproc *cproc);
extern uint64_t locate_linkmap32 (struct etrace_ops *ops, int pid);
extern int readMemory32 (void *handle, uint64_t addr, void *space, uint64_t bytes);
extern uint64_t scanForExceptionMagic32 (struct cproc *cproc);
extern int fetchti32 (struct etrace_ops cb, uint64_t tip, struct elan4_trap_info32 *ti);
/* elf32.c */
extern void dumpMemory64 (struct cproc *cproc);
extern uint64_t locate_linkmap64 (struct etrace_ops *ops, int pid);
extern int readMemory64 (void *handle, uint64_t addr, void *space, uint64_t bytes);
extern uint64_t scanForExceptionMagic64 (struct cproc *cproc);
extern int fetchti64 (struct etrace_ops cb, uint64_t tip, struct elan4_trap_info64 *ti);

/* Statistics collection */

/* Old, just dump very simple stats */
extern int dump_stats_eagle(void *pages, size_t size, size_t pagesize);

/* New, can do lots of things with it */
#include "sf.h"

/* elf.c */
extern void fetch_data_dead (char *cname, char *ename, int trap_dump);
extern void show_cq_from_file (char *cname, uint32_t start, uint32_t end, int raw);

/* ptrace.c */
extern void fetch_data_live (int pid);

/* parallel.c */
typedef enum {
    EDBSTAT_NOP, EDBSTAT_SHOW, EDBSTAT_RAW
} STAT_OP;

struct stats_options {
    STAT_OP todo;
};

extern void go_parallel (struct sf_params *sfp, struct stats_options *so);

/* edb.c */
extern int attach_sysv(struct sysv *v,void *base);


/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
