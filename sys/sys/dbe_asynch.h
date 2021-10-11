/*	@(#)dbe_asynch.h 1.1 94/10/31  90/09/25 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Database asynchronous disk I/O - works with raw disk partitions */

#ifndef _sys_dbe_asynch_h
#define _sys_dbe_asynch_h

#include <sys/asynch.h>
#include <sys/buf.h>

#ifdef KERNEL

extern int dbe_asynchio_enabled;		/* 0 = disabled, 1 = enabled */
extern int dbe_arw(), dbe_aio_comp_check(), dbe_aiodone();
extern caddr_t dbe_aio_comp_reap();
extern void dbe_astop();

extern int (*id_get_strategy())();

typedef struct dbe_aio_info	*dbe_handle_t;	/* opaque pointer */

struct auio {
    struct buf	auio_buf;		     /* I/O buffer structure */
    char	*auio_base;		     /* user buffer */
    u_int	auio_len;		     /* number of bytes */
    off_t	auio_offset;		     /* disk offset */
    aio_result_t *auio_result;

    u_int	auio_nbytes;		     /* bytes actually transfer */
    int		auio_error;

    int		auio_flags;
    struct auio	*auio_next;
    struct auio	*auio_prev;
    struct dbe_aio_info	*auio_dai;
};

/* bits in auio_flags */
#define AUIO_AVAIL	0		     /* auio is available */
#define AUIO_BUSY	1		     /* auio is not available */
#define AUIO_DONE	2		     /* I/O completed, but not polled */

struct daipglock {
	addr_t		dl_addr;
	u_short		dl_keepcnt;
};

#define DAIPGLOCKINCR	500
#define AVAILDLADDR	((addr_t)-1)

/* Per-process completion list info */
struct dbe_aio_info {
        struct proc     *dai_procp;
        int             dai_pid;
        int             dai_flags;
        short           dai_pending_count;
        short           dai_comp_count;
        struct auio     *dai_comp_first;
        struct auio     *dai_comp_last;
	int		dai_pglocksize;
	int		dai_pglocklast;
	struct daipglock *dai_pglocks;
};
#define DAI_AVAIL       0       /* element is available for use */
#define DAI_BUSY        1       /* element is not available for use */

/* status codes returned by dbe_arw() and dbe_aio_comp_check() */
#define DBE_AIO_ERROR           	-1      /* failure */
#define DBE_AIO_SUCCESS         	0       /* successfully done */
#define DBE_AIO_UNAVAIL         	1       /* not enabled or unavailable */
#define DBE_AIO_PENDING    		2       /* i/o(s) pending completion */
#define DBE_AIO_COMPLETE		3       /* i/o(s) have completed */
#define DBE_AIO_NONE_OUTSTANDING	4       /* no i/o pending or complete */

#endif /* KERNEL */
#endif /* _sys_dbe_asynch_h */
