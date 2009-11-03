/*
 * Copyright (c) 2003,2004 Quadrics Ltd
 */

#ident "xml.c,v 1.3 2005/02/03 15:26:08 ashley Exp"
/*             /cvs/master/quadrics/elan4lib/edb/xml.c,v */

#include <edb.h>

/***********************************************************
 *                                                         *
 * The xml code...                                         *
 *                                                         *
 *                                                         *
 *                                                         *
 ***********************************************************/

#define x_d(TYPE,VALUE) \
  printf("%s"TYPE"\n",indent,VALUE);

#ifdef S_SPLINT_S
#define PRIx64 "llx"
#endif

#define FULL_TAG(XI,STRUCT,TAG,VALUE) \
  fprintf(XI->file,"%s<%s>"VALUE"</%s>\n",XI->indent,#TAG,(STRUCT)->TAG,#TAG)

/***********************************************************
 *                                                         *
 * The 'dumping' code...                                   *
 *                                                         *
 *                                                         *
 *                                                         *
 ***********************************************************/

void
x_free (struct x_info *xi)
{
    free(xi->rindent);
    free(xi);
}

static void
x_o (struct x_info *xi,char *tag)
{
    fprintf(xi->file,"%s<%s>\n",xi->indent,tag);
    xi->indent -= xi->iwidth;
}

static void
x_c (struct x_info *xi,char *tag)
{
    xi->indent += xi->iwidth;
    fprintf(xi->file,"%s</%s>\n",xi->indent,tag);
}

static void
dump_rx_local (struct x_info *xi,struct tport_rx_local *local)
{
    x_o(xi,"local");
    FULL_TAG(xi,local,rflags,"%#"PRIx64);
    FULL_TAG(xi,local,senderMask,"%#x");
    FULL_TAG(xi,local,senderSel,"%#x");
    FULL_TAG(xi,local,tagMask,"%#x");
    FULL_TAG(xi,local,tagSel,"%#x");
    FULL_TAG(xi,local,base,"%p");
    FULL_TAG(xi,local,size,"%zi");
    x_c(xi,"local");
}

static void
dump_rx_remote (struct x_info *xi,struct tport_rx_remote *remote)
{
    x_o(xi,"remote");
    FULL_TAG(xi,remote,sender,"%#x");
    FULL_TAG(xi,remote,tag,"%#x");
    FULL_TAG(xi,remote,size,"%zi");
    x_c(xi,"remote");
}

void
dump_rx_u (struct x_info *xi,struct tport_rx_unexpected *u)
{
    dump_rx_remote(xi,&u->remote);
}

static void
dump_rx_posted (struct x_info *xi,struct tport_rx_posted *posted)
{
    if ( posted->flags & TRX_MATCHED ) {
	x_o(xi,"matched");
	dump_rx_local(xi,&posted->local);
	if ( posted->flags & TRX_HAVEREMOTE )
	    dump_rx_remote(xi,&posted->remote);
	if ( posted->flags & TRX_SYSTEM ) {
	    ;
	    
	    //FULL_TAG(xi,&posted->local,userFslags,"%Lx");
	}
	x_c(xi,"matched");
    } else {
	x_o(xi,"posted");
	dump_rx_local(xi,&posted->local);
	x_c(xi,"posted");
    }
}

static void
dump_tx_posted (struct x_info *xi,struct tport_tx_posted *posted)
{
    x_o(xi,"posted");
    FULL_TAG(xi,&posted->local,userflags,"%#"PRIx64);
    FULL_TAG(xi,&posted->local,flags,"%#"PRIx64);
    FULL_TAG(xi,&posted->local,destvp,"%d");
    FULL_TAG(xi,&posted->local,sender,"%x");
    FULL_TAG(xi,&posted->local,tag,"%x");
    FULL_TAG(xi,&posted->local,base,"%p");
    FULL_TAG(xi,&posted->local,size,"%zi");
    x_c(xi,"posted");
}

void
dump_ti (struct x_info *xi, struct tport_info *ti)
{
    char tport_id[128];
    
    if (verbose)
	printf("Dumping ti %p flags %x rx %p tx %p\n",
	       ti,
	       ti->flags,
	       ti->rx_posted_list,
	       ti->tx_posted_list);
    
    sprintf(tport_id, "tport id=\"%p\"", ti->unique);
    
    x_o(xi,tport_id);
    
    if ( ti->flags & UNEXPECTED_OK ) {
	struct tport_rx_unexpected *u = ti->unexpect_list;
	x_o(xi,"unexpected");
	while (u) {
	    dump_rx_u(xi,u);
	    u = u->next;
	}
    } else {
	x_o(xi,"unexpected supported=\"no\"");
    }
    x_c(xi,"unexpected");
    
    x_o(xi,"rx");
    
    if ( ti->flags & POSTED_OK ) {
	struct tport_rx_posted *posted = ti->rx_posted_list;
	while (posted) {
	    dump_rx_posted(xi,posted);
	    posted = posted->next;
	}
    } else
	printf("Posted message queue not supported\n");
    x_c(xi,"rx");
    

    if ( ti->flags & TX_POSTED_OK ) {
	struct tport_tx_posted *posted = ti->tx_posted_list;
	x_o(xi,"tx");

	while (posted) {
	    dump_tx_posted(xi,posted);
	    posted = posted->next;
	}
	x_c(xi,"tx");
    } else {
	x_o(xi,"tx supported=\"no\"");
	x_c(xi,"tx");
    }
    x_c(xi,"tport");
}

#define MAXINDENT 6
/*@null@*/ struct x_info *
x_init (size_t width) {
    struct x_info *x_info = malloc(sizeof(struct x_info));
    int i;
    
    if ( ! x_info )
	return NULL;
    
    x_info->file = stdout;
    
    x_info->iwidth = width;
    x_info->indent = x_info->rindent = malloc(width*MAXINDENT+1);
    
    if ( ! x_info->rindent ) {
	free(x_info);
	return NULL;
    }
    
    memset(x_info->indent,0,width*MAXINDENT+1);
    
    for ( i = 0 ; i < MAXINDENT*width ; i ++ )
	x_info->indent[i] = ' ';
    
    x_info->indent[MAXINDENT*width] = '\0';
    
    x_info->indent += MAXINDENT*width;
    
    return x_info;
}

/*
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
