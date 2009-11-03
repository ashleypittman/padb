
/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "stats_eagle.c,v 1.3 2005/06/17 11:16:32 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/stats_eagle.c,v */


#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef S_SPLINT_S
#define PRIx64 "llx"
#define PRIu64 "llu"
#define PRId64 "lld"
#endif

int dump_stats_eagle(void *pages, size_t size, size_t pagesize) {
    char      *header = (char *)pages;
    uint64_t *payload = (uint64_t *)pages;
    int       localId = 0;
    
    if ( ! size )
	return 1;
    
    if ( strncmp(&header[0],"ELAN STATS",16) )
	return 1;
    
    if ( strncmp(&header[16],"Eagle",16) )
	return 1;
    
    size -= pagesize;
    
    do {
	payload = payload + (pagesize/sizeof(uint64_t));
	if ( payload[1] != 0 )
	    printf("localId %d transfered %#"PRIx64" bytes in %#"PRIx64" operations\n",
		   localId++,
		   payload[0],
		   payload[1]);
	
	size    -= pagesize;
    } while ( size );
    
    return 0;
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
