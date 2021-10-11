#ifndef lint
static	char sccsid[] = "@(#)ufs_nd.c 1.1 94/10/31";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "nd.h"
#if NND > 0

/*
 * Net disk driver.
 */

#include "nd.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/map.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <machine/psl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <sun/ndio.h>

#define	NUNITS		4	/* max units per user (/dev/nd0 to 3) */
#define	NPUBLICS 	8	/* max public units (/dev/ndp0 to 7) */
#define	NLOCALS		32	/* max local units (/dev/ndl0 to 31) */
#define	NUSERS		20	/* max nonpublic clients per server */
#define	NACTIVE		4	/* max active requests per user */
#define	NDUTIMER	2	/* timeout value in 1/4 sec. units */

int	ndutimer = NDUTIMER;
/*
 * The "units" which are accessed each contain a separate filesystem.
 * A unit can either correspond to a normal "whole" partition,
 * or it can be a subpartition.
 */
struct nddev {
	dev_t	nd_dev;		/* whole partition being divided */
	short	nd_valid;	/* this entry in use (nd_dev can be zero) */
	int	nd_off;		/* offset of subpartition */
	int	nd_size;	/* size of subpartition */
};

/*
 * A user must be "registered" in this structure before we will
 * honor requests from him.  Each "user" is allowed
 * NUNITS of filesystems.
 */
struct nduser {
	struct	in_addr nu_address;	/* internet address */
	int	nu_numreq;		/* total number of requests serviced */
	struct	ether_addr nu_eaddr;	/* ether address */
	int	nu_maxpacks;		/* max packets client can rcv at once */
	struct	nddev nu_unit[NUNITS];	/* NUNITS filesystems */
};

/*
 * User/server request queue.  The same format is used on both
 * the user and server side, although the use of some fields
 * is slightly different.  For example nr_pbuf (the physical
 * buffer pointer) is only used by the server.  
 */
struct ndreq {
	struct	ndreq *nr_next,*nr_prev;	/* links to next and prev */
	struct	buf *nr_buf;	/* buffer header associated */
	struct	ndpack nr_pack;	/* prototype packet */
	int	nr_xcount;	/* xmit count */
	int	nr_xmaxpacks;	/* max to xmit between acks */
	caddr_t	nr_caddr;	/* current transfer address, physical;
				   nr_pack.np_caddr is relative */
	int	nr_ccount;	/* current transfer count remaining */
	caddr_t	nr_pbuf;	/* physical buffer */
	int	nr_pcount;	/*  and size */
	int	nr_puse;	/*  and use count */
	int	nr_timer;	/* time to live or rexmit */
	int	nr_timeouts;	/* number of timeouts */
	int	nr_state;	/* state variable, below */
	int	nr_inq;		/* request is in server queue */
};
/* nr_state values */
#define	FIRST		1
#define	ACTIVE		2
#define	DONEWAIT	3		/* user side, waiting for NDOPDONE */
#define	IOWAIT		4		/* server, waiting for disk IO */
#define	IODONE		5

/*
 * nd software status.
 */
struct ndsoftc {
	struct	nddev nd_local[NLOCALS];  	/* local units */
	struct	nddev nd_public[NPUBLICS];	/* public units */
	struct	nduser nd_user[NUSERS];		/* user units */
	struct	in_addr nd_addr;	/* server internet address */
	struct	buf nd_tab;		/* for queueing by ndstrategy */
	struct	buf nd_bufr;		/* raw header for physio */
	struct	buf nd_bufl;		/* header for local IO */
	struct	buf *nd_buflp;		/* pointer to real buffer */
	int	nd_seq;			/* sequence number */
	struct	ndreq nd_quser;		/* queue head for user requests */
	struct	ndreq nd_qserv;		/* queue head for server action */
	u_char	nd_lockserv;		/* server identified and initted */
	u_char	nd_sdead;		/* server still dead flag */
	u_char	nd_son;			/* server on */
	u_char	nd_sver;		/* server version number */
	u_char	nd_uver;		/* user version number */
	u_char	nd_majdev;		/* nd block major device number */
	u_char	nd_nactive;		/* number of active user requests */
	u_char	nd_first;		/* 1st time init complete */
	int	nd_curoff;		/* current offset */
	struct map *nd_memmap;		/* nd buffer space */
} nd;

struct	ndstat ndstat;		/* statistics area */
struct	ifnet *ndif;		/* interface pointer */
#define	NDDEFMAXPACKS	6
int	ndclient_maxpacks = NDDEFMAXPACKS;

#define	NDMAPSIZE	64
#define	NDMEMSIZE	(128*1024)	/* amount of memory used by nd server */

struct	map ndmemmap[NDMAPSIZE];	/* allocation map for buffer memory */
caddr_t	ndmem;				/* pointer to buf mem space */
caddr_t	ndmemall();

int	ndiodone(),ndlociodone(),nd_timeout();
struct	ndreq *ndumatch(),*ndsmatch();
struct	ndpack *ndgetnp();

#define	EACHUSER nr = nd.nd_quser.nr_next; nr != &nd.nd_quser; nr = nr->nr_next
#define	EACHSERV nr = nd.nd_qserv.nr_next; nr != &nd.nd_qserv; nr = nr->nr_next

/* special values of "b_blkno" field are used for "out-of-band" info */
#define	BLKNO_SIZE 0x10000000	/* ndsize request */
#define	BLKNO_FIRST BLKNO_SIZE	/* used in b_blkno range check */

/* #define ndsafe 0x0001	/* force all strategy calls to use this dev */


ndopen(dev, flag)
	dev_t dev;
{
	if (ndif == 0)	/* no INET interface */
		return (ENXIO);
	/*
	 * Make sure interface address has been initialized to prevent
 	 * hangs.  Initialization for diskless nodes occurs via ndsize(swapdev)
	 * before we ever get called.
	 */
	if ((minor(dev) & NDMINLOCAL) == 0 && !address_known(ndif))
		return (ENETUNREACH);
	if ((minor(dev) & NDMINPUBLIC) && (flag & FWRITE))
		return (EROFS);
	return (0);
}

ndstrategy(bp)
	register struct buf *bp;
{
	register struct buf *dp;
	register int unit;
	register struct nddev *ndd;
	int s, mindev;

	if (!address_known(ndif) && nd.nd_majdev == major(rootdev))
			revarp_myaddr(ndif);
	mindev = minor(bp->b_dev);
	unit = mindev & NDMINUNIT;
	if (bp->b_bcount <= 0) {
		goto bad;
	}

	bp_mapin(bp);			/* make data kernel addressable */

	/*
	 * If a normal operation, place the buf
	 * on our strategy queue.
	 */
	if ((mindev & NDMINLOCAL) == 0) {
		s = splnet();
		bp->av_forw = (struct buf *)NULL;
		if (nd.nd_tab.b_actf == NULL)
			nd.nd_tab.b_actf = bp;
		else
			nd.nd_tab.b_actl->av_forw = bp;
		nd.nd_tab.b_actl = bp;
		ndstart();
		(void) splx(s);
		return;
	}

	/*
	 * If NDMINLOCAL is set in the minor device (/dev/ndl*),
	 * a "local" subfilesystem is being referenced without
	 * net activity.  Map bp->b_dev and b_blkno into the 
	 * filesystem of which it is a part.  It would be easy
	 * to just change these fields in bp, but this would
	 * interfere with buffer cache searches by getblk;
	 * instead we use a static buf header of our own.
	 */
	if (unit >= NLOCALS || (ndd = &nd.nd_local[unit])->nd_valid == 0) {
		goto bad;
	}
	s = spl6();
	dp = &nd.nd_bufl;
	while (dp->b_flags & B_BUSY) {
		ndstat.ns_lbusy++;
		dp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)dp, PRIBIO);
	}
	*dp = *bp;			/* make a copy and alter it */
	dp->b_flags |= (B_BUSY|B_CALL);
	dp->b_iodone = ndlociodone;	/* call this fn when io done */
	nd.nd_buflp = bp;		/* save the real buf address */
	if (ndmapbuf(dp,ndd) == 0) {
		dp->b_flags = 0;
		(void) splx(s);
		goto bad;
	}
	/* save the mapped resid and count, since driver overwrites */
	bp->b_resid = dp->b_resid; 
	bp->b_bcount = dp->b_bcount;
	(void) splx(s);
#ifdef ndsafe
	if (dp->b_dev != ndsafe) {
		printf("ndsafe %x ",dp->b_dev);
		dp->b_dev = ndsafe;
	}
#endif ndsafe
	(*bdevsw[major(dp->b_dev)].d_strategy)(dp);
	return;
bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

/*ARGSUSED*/
ndioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	struct nddev *ndd;
	register int i, j, l;
	int free, s, err=0;
	register struct ndiocuser *nu;
	register struct ndiocether *ne;
	register struct nduser *ndu;
	struct file *fp;

	if (!suser()) 
		return (u.u_error);
	s = splnet();
	switch (cmd) {
	case NDIOCSON:		/* server on */
		if (ndif->if_memmap == 0) {
			if (ndmem == 0) {
				ndmem = kmem_alloc(NDMEMSIZE);
				rminit(ndmemmap, (long)NDMEMSIZE, (long)ndmem,
				    "nd buffer space", NDMAPSIZE);
			}
			nd.nd_memmap = ndmemmap;
		} else
			nd.nd_memmap = ndif->if_memmap;
		nd.nd_son = 1;
		break;

	case NDIOCSOFF:		/* server off */
		nd.nd_son = 0;
		if (ndmem) {
			ndmemwait();
			kmem_free(ndmem, NDMEMSIZE);
			rminit(ndmemmap, (long)0, (long)0,
			    "nd buffer space", 1);
			ndmem = 0;
		}
		break;

	case NDIOCSAT:		/* server at ipaddr */
		nd.nd_addr = *(struct in_addr *)addr;
		nd.nd_lockserv = 0;
		break;

	case NDIOCETHER:
		ne = (struct ndiocether *)addr;
		for (j = 0, ndu = nd.nd_user; j < NUSERS; j++, ndu++) 
			if (ndu->nu_address.s_addr == ne->ne_iaddr.s_addr)
				break;
		if (j >= NUSERS)
			goto inval;
		/*
		 * Install ARP entry for sake of Reverse ARP
		 */
		ndu->nu_eaddr = ne->ne_eaddr;
		ndu->nu_maxpacks = ne->ne_maxpacks;
		(void) arpseten(&ndu->nu_address, &ndu->nu_eaddr, 1);
		break;

	case NDIOCVER:		/* version number */
		nd.nd_sver = (*(int *)addr) & 0xF;
		break;

	case NDIOCCLEAR:	/* clear tables */
		nd.nd_son = 0;
		nd.nd_curoff = 0;
		bzero((caddr_t)nd.nd_local, sizeof nd.nd_local);
		bzero((caddr_t)nd.nd_public, sizeof nd.nd_public);
		bzero((caddr_t)nd.nd_user, sizeof nd.nd_user);
		break;

	case NDIOCUSER: 	/* set client parameters */
		nu = (struct ndiocuser *)addr;
		if ((fp = getf(nu->nu_mydev)) == NULL ||
		    (fp->f_type == DTYPE_SOCKET) ||
		    ((struct vnode *)fp->f_data)->v_type != VBLK ||
		    (major(((struct vnode *)fp->f_data)->v_rdev) >= nblkdev)) {
			err = ENOTBLK;
			goto out;
		}
		i = nu->nu_hisunit;
		l = nu->nu_mylunit;
		if (i < 0 || (l > 0 && l >= NLOCALS))
			goto inval;
		if (nu->nu_addr.s_addr == 0) {	/* public */
			if (i >= NPUBLICS)
				goto inval;
			ndd = &nd.nd_public[i];
		} else {		/* private */
			if (i >= NUNITS)
				goto inval;
			/*
			 * find a matching user net address or
			 * an empty slot.
			 */
			free = -1;
			for (j = 0; j < NUSERS; j++) {
				if (free < 0 &&
				    nd.nd_user[j].nu_address.s_addr == 0) {
					free = j;
					continue;
				}
				if (nd.nd_user[j].nu_address.s_addr ==
				    nu->nu_addr.s_addr)
					goto found;
			}
			if (free < 0) {
				err = ENOSPC;
				goto out;
			}
			j = free;
			nd.nd_user[j].nu_address = nu->nu_addr;
		found:
			ndd = &nd.nd_user[j].nu_unit[i];
		}
		/*
		 * set the serving filesystem dev and 
		 * subunit number.
		 */
		ndd->nd_dev = ((struct vnode *)fp->f_data)->v_rdev;
		if (nu->nu_myoff < 0)
			ndd->nd_off = nd.nd_curoff;
		else
			ndd->nd_off = nu->nu_myoff;
		ndd->nd_size = nu->nu_mysize;
		if (ndd->nd_size > 0) {
			nd.nd_curoff = ndd->nd_off + ndd->nd_size;
		} else {
			ndd->nd_off = 0;
			nd.nd_curoff = 0;
		}
		ndd->nd_valid = 1;
		if (l < 0)	/* no local unit */
			goto out;
		nd.nd_local[l] = *ndd;	/* copy entry */
		break;

	default:
		goto inval;
	}
	goto out;
inval:
	err = EINVAL;
out:
	(void) splx(s);
	return (err);
}

void
ndminphys(bp)
	register struct buf *bp;
{

	if (bp->b_bcount > NDMAXIO)
		bp->b_bcount = NDMAXIO;
}

ndread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(ndstrategy, &nd.nd_bufr, dev, B_READ, ndminphys, uio));
}

ndwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(ndstrategy, &nd.nd_bufr, dev, B_WRITE, ndminphys, uio));
}

ndsize(dev)
	dev_t dev;
{
	static struct buf b;
	static int size;
	int ndnull();

	if (ndif == 0)
		return (-1);
	size = 0;
	b.b_blkno = BLKNO_SIZE;
	b.b_flags = (B_BUSY | B_READ);
	b.b_bcount = sizeof (size);
	b.b_un.b_addr = (char *)&size;
	b.b_dev = dev;
	ndstrategy(&b);
	(void) biowait(&b);
	return (size);
}

/*
 * Check for new buffers from the strategy routine.  Called from
 * ndstrategy and timeout.
 */
ndstart()
{
	register struct buf *bp,*dp;
	struct mbuf *m;
	register struct ndreq *nr;

	dp = &nd.nd_tab;
	while (bp = dp->b_actf) {
		if (nd.nd_nactive >= NACTIVE)
			break;
		if ((m = m_get(M_DONTWAIT, MT_DATA)) == 0)
			break;
		m->m_off = MMINOFF;
		nr = mtod(m, struct ndreq *);
		bzero((caddr_t)nr, sizeof *nr);
		insque(nr, &nd.nd_quser);
		nd.nd_nactive++;
		nr->nr_buf = bp;
		dp->b_actf = bp->av_forw;
		nr->nr_state = FIRST;
		nduout(nr);
	}
}

/*
 * Perform output for user request nr.
 */
nduout(nr)
	register struct ndreq *nr;
{
	register struct buf *bp;
	register struct ndpack *np,*npr;
	register struct sockaddr_in *sin;

	bp = nr->nr_buf;
	npr = &nr->nr_pack;
	switch (nr->nr_state) {
	case FIRST:
		nr->nr_caddr = bp->b_un.b_addr;
		nr->nr_ccount = bp->b_bcount;
		npr->np_op = ((bp->b_flags & B_READ) ? NDOPREAD : NDOPWRITE);
		npr->np_min = minor(bp->b_dev);
		npr->np_blkno = bp->b_blkno;
		npr->np_bcount = bp->b_bcount;
		npr->np_caddr = 0;
		npr->np_seq = nd.nd_seq++;
		npr->np_ver = nd.nd_uver;
		if (in_netof(nd.nd_addr) != INADDR_ANY) 
			npr->np_ip.ip_dst = nd.nd_addr;	/* ZZZ set src too? */
		else {
			sin = (struct sockaddr_in *)
				&(ndif->if_addrlist->ifa_broadaddr);
			npr->np_ip.ip_dst = sin->sin_addr;
		}
		nr->nr_xcount = 0;
		nr->nr_xmaxpacks = ndclient_maxpacks;
		nr->nr_state = ACTIVE;
		/* fall into */

	case ACTIVE:
		nr->nr_timer = ndutimer << nr->nr_timeouts;
		if (nr->nr_timer > 240)
			nr->nr_timer = 240;
		if ((npr->np_op & NDOPCODE) == NDOPWRITE)
			ndsenddata(nr);
		else {
			if ((np = ndgetnp(npr)) == 0) 
				return;
			nr->nr_state = DONEWAIT;
			ndoutput(np);
		}
		break;

	default:
		break;
	}
	return;
}

/*
 * First time initialization.  Called from protocol startup.
 */
nd_init()
{
	register struct ifnet *ifp;
	extern struct ifnet loif;
	register i;
	extern dev_t dumpdev;

	/*
	 * first time initialization.  
	 */
	if (nd.nd_first)
		return;
	nd.nd_first++;

	nd.nd_qserv.nr_next = nd.nd_qserv.nr_prev = &nd.nd_qserv;
	nd.nd_quser.nr_next = nd.nd_quser.nr_prev = &nd.nd_quser;
	for (ifp = ifnet; ifp != 0; ifp = ifp->if_next) {
	    if (ifp != &loif) break;
	}
	if (ifp == 0) {
		return;
	}
	nd.nd_seq = 0;  /* need better seq init XXX */
	for (i = 0; i < nblkdev; i++)
		if (bdevsw[i].d_strategy == ndstrategy) {
			nd.nd_majdev = i;
			break;
		}
	if (i >= nblkdev)
		panic("nd: no bdevsw");
	ndif = ifp;
	timeout(nd_timeout, (caddr_t) 0, hz / 4);  /* start timeout routine */
}

/*
 * Process ND input packets, called from ip_input.
 */
/*ARGSUSED*/
nd_input(m, ifp)
	register struct mbuf *m;
	struct ifnet *ifp;
{
	register struct ndreq *nr;
	register struct buf *bp;
	register struct ndpack *np, *npr;
	register int i;
	int wack;		/* want acknowledge */
	caddr_t addr;

	ndstat.ns_rpacks++;
	if (m->m_off > MMAXOFF || m->m_len < sizeof *np) {
		if ((m = m_pullup(m, sizeof *np)) == 0) {
			ndstat.ns_nobufs++;
			return;
		}
	}
	np = mtod(m, struct ndpack *);

	np->np_seq = ntohl(np->np_seq);
	np->np_blkno = ntohl(np->np_blkno);
	np->np_bcount = ntohl(np->np_bcount);
	np->np_resid = ntohl(np->np_resid);
	np->np_caddr = ntohl(np->np_caddr);
	np->np_ccount = ntohl(np->np_ccount);
	wack = np->np_op & NDOPWAIT;

	if ((np->np_op & NDOPDONE) == 0)
		goto request;

	/*
	 * this is a response to our read/write request.
	 */
	if ((nr = ndumatch(np)) == 0) 
		goto drop;			/* no matching user request */

	bp = nr->nr_buf;
	npr = &nr->nr_pack;
	switch (npr->np_op & NDOPCODE)  {

	case NDOPWRITE:			 	/* write response */
		if (np->np_caddr != npr->np_caddr) {
			ndstat.ns_wseq++;
			goto drop;
		}
		if (nr->nr_ccount <= 0) {
			if (np->np_error) {
				bp->b_flags |= B_ERROR;
				bp->b_error = np->np_error;
			}
			bp->b_resid = np->np_resid;
			goto next;
		}
		nr->nr_timer = ndutimer;	/* extend life of request */
		nr->nr_timeouts = 0;
		nr->nr_state = ACTIVE;
		nr->nr_xcount = 0;	/* count up to nr_maxpacks again */
		nduout(nr);
		goto drop;

	case NDOPREAD:			 	/* read response */
		i = np->np_ccount;
		if (np->np_caddr != npr->np_caddr || i <= 0 || i > NDMAXDATA) {
			ndstat.ns_rseq++;
			goto drop;
		}
		nr->nr_timer = ndutimer;	/* extend life of request */
		nr->nr_timeouts = 0;
		i = MIN(i, nr->nr_ccount);	/* copy from mbufs to contig */
		addr = nr->nr_caddr;
		nr->nr_caddr += i;
		npr->np_caddr += i;
		nr->nr_ccount -= i;
		if (nr->nr_ccount <= 0) {
			if (np->np_error) {
				bp->b_flags |= B_ERROR;
				bp->b_error = np->np_error;
			}
			bp->b_resid = np->np_resid;
		} else if (wack) {		/* need another read */
			nr->nr_state = ACTIVE;
			nduout(nr);
		}
		/* do read before copy to enhance overlapping */
		m->m_len -= sizeof (struct ndpack);
		m->m_off += sizeof (struct ndpack);
		(void) m_movtoc(m, addr, i);
		/* np no longer valid here */
		m = NULL;
		if (nr->nr_ccount <= 0)
			goto next;		/* no more to read */
		return;

	default:
		printf("nd: bad resp op code\n");
	}
request:
	/*
	 * This is a request to be serviced.
	 */
	switch (np->np_op & NDOPCODE) {
	case NDOPREAD:
		if ((nr = ndsmatch(np)) == 0)
			goto drop;		/* couldn't queue request */
		npr = &nr->nr_pack;
		switch (nr->nr_state) {
		case FIRST:
			nddiskstrat(nr);	/* start the disk read */
			goto drop;

		default:
		case IOWAIT:
			goto drop;		/* io will terminate soon */

		case DONEWAIT:
			nr->nr_state = ACTIVE;
			/* drop thru */

		case ACTIVE:
			break;
		}
		/* 
		 * user is requesting next group of packets.
		 */
		if (np->np_caddr != npr->np_caddr) {
			ndstat.ns_rseq++;  /* seq error */
			goto drop;
		}
		nr->nr_timer = NDXTIMER;	/* extend life of request */
		nr->nr_xcount = 0;	/* start counting up to nr_xmaxpacks */
		(void) ndsout(nr);
		/* if the read request just completed, nr may be gone now */
		goto drop;

	case NDOPWRITE:
		if ((nr = ndsmatch(np)) == 0)
			goto drop;	/* couldn't queue request */
		switch (nr->nr_state) {
		case FIRST:
			nr->nr_state = ACTIVE;

		case ACTIVE:
			break;

		case IOWAIT:
		default:
			goto drop;
		}
		npr = &nr->nr_pack;
		i = np->np_ccount;
		if (np->np_caddr != npr->np_caddr) {
			ndstat.ns_wseq++;  /* seq error */
			goto drop;
		}
		if (i <= 0 || i > NDMAXDATA) {
			ndstat.ns_badreq++;
			(void) nderrreply(np, ENXIO);
			goto drop;
		}
		nr->nr_timer = NDXTIMER;	/* extend life of request */
		i = MIN(i, nr->nr_ccount);	/* copy mbufs to user contig */
		m->m_len -= sizeof (struct ndpack);
		m->m_off += sizeof (struct ndpack);
		(void) m_movtoc(m, nr->nr_caddr, i);
		nr->nr_ccount -= i;
		nr->nr_caddr += i;
		npr->np_caddr += i;
		if (nr->nr_ccount <= 0)
			nddiskstrat(nr);	/* start the disk write */
		else if (wack)  		/* he wants an acknowledge */
			(void) ndsout(nr);
		return;

	case NDOPERROR:
		if((nr = ndumatch(np)) == 0)
			goto drop;	/* no matching request */
		if ((nr->nr_pack.np_op & NDOPCODE) == NDOPWRITE &&
		    (np->np_ver & 0xF) != nd.nd_uver)
			printf("nd: version change\n");
		bp = nr->nr_buf;
		bp->b_flags |= B_ERROR;
		bp->b_error = np->np_error;
		/* XXX message to console? */
		goto next;

	default:
		printf("nd: bad req op code\n");
		return;
	}
	/* NOTREACHED */

next:
	/*
	 * User request is complete, iodone the
	 * buf and cleanup the request entry.
	 */
	if (nd.nd_sdead) {
		nd.nd_sdead = 0;	/* server no longer dead */
		printf("nd: server OK\n");
	}
	ndclear(nr);
	iodone(bp);
	nd.nd_nactive--;
	ndstart();
drop:
	if (m)
		m_freem(m);
	return;
}

/*
 * Perform next server output.
 */
ndsout(nr)
	register struct ndreq *nr;
{
	register struct ndpack *np,*npr;
	register struct buf *bp;
	int clear = 0;

	npr = &nr->nr_pack;
	if ((npr->np_op & NDOPCODE) == NDOPWRITE) {
		/*
		 * server response to user write.  If disk just
		 * finished, report error status.
		 */
		if ((np = ndgetnp(npr)) == 0) 
			return (0);
		if (nr->nr_state == IODONE &&
		    ((bp=nr->nr_buf)->b_flags & B_ERROR))
			np->np_error = bp->b_error ? bp->b_error : EIO;
		ndoutput(np);
		if (nr->nr_state == IODONE)
			clear = 1;
	} else {
		/*
		 * Server response to user read;  send next packet(s).
		 */
		if (nr->nr_state == IODONE &&
		    ((bp=nr->nr_buf)->b_flags & B_ERROR))
			npr->np_error = bp->b_error ? bp->b_error : EIO;
		if (nr->nr_state == IODONE)
			nr->nr_state = ACTIVE;
		ndsenddata(nr);
		if (nr->nr_ccount <= 0)
			clear = 1;
	}
	if (clear) {
		ndsclear(nr);
		return (1);
	}
	return (0);
}

/*
 * Timeout user and server requests.
 */
nd_timeout()
{
	register struct ndreq *nr;
	int s = splnet();

	ndstart();	/* pickup any leftover strategy queue */
	/*
	 * clear old server requests.
	 */
top:	/* re-enter when nd_qserv modified */
	for (EACHSERV) {
		if (nr->nr_state == IOWAIT) 
			continue;
		if (--nr->nr_timer <= 0) {
			ndsclear(nr);
			ndstat.ns_stimo++;
			goto top;
		}
	}
	/*
	 * retransmit user requests;  console message when
	 * server first becomes hung.
	 */
	for (EACHUSER) {
		if (--nr->nr_timer > 0) 
			continue;
		nr->nr_state = FIRST;
		ndstat.ns_rexmits++;
		nr->nr_timeouts++;
		nduout(nr);
		if (nr->nr_timeouts < 8)
			continue;
		ndstat.ns_utimo++;
		if (nd.nd_sdead == 0) {
			if (nd.nd_lockserv)
				printf("nd: disk server not responding;");
			else
				printf("nd: no disk server responding;");
			printf(" still trying.\n");
			nd.nd_sdead = 1;
		}
	}
	(void) splx(s);
	timeout(nd_timeout, (caddr_t) 0, hz / 4);
}

/*
 * Map buffer onto subfilesystem;  returns 0 if access invalid.
 */
ndmapbuf(bp, ndd)
	register struct buf *bp;
	register struct nddev *ndd;
{
	int resid;

	if (bp->b_blkno < 0 || bp->b_bcount <= 0)
		return (0);
	if (ndd->nd_valid == 0)	/* no forwarding device address */
		return (0);
	bp->b_dev = ndd->nd_dev;
	if (ndd->nd_size < 0)	/* no mapping needed */
		return (1);
	if (bp->b_blkno >= ndd->nd_size)
		return (0);	/* start blk out of range */
	/* is the end byte offset in range? */
	resid = dbtob(bp->b_blkno) + bp->b_bcount - dbtob(ndd->nd_size);
	if (resid > 0) {	/* end out of range, truncate */
		bp->b_resid = resid;
		bp->b_bcount -= resid;
	} else
		bp->b_resid = 0;
	bp->b_blkno += ndd->nd_off;
	return (1);
}

/*
 * Local io done;  called from iodone by real device driver at interrupt
 * level.  Finish up the io.
 */
ndlociodone(bp)
	register struct buf *bp;  /* our fake buffer (&nd.nd_bufl) */
{
	register struct buf *dp = nd.nd_buflp;  /* the real buffer */

	if (bp->b_flags & B_ERROR) {
		dp->b_flags |= B_ERROR;
	}
	dp->b_error = bp->b_error;
	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	iodone(dp);
	return (0);
}

/*
 * Match this reply packet with a user request.  If we are still
 * broadcasting, lock onto the replying server.
 */
struct ndreq *
ndumatch(np)
	register struct ndpack *np;
{
	register struct ndreq *nr;

	for (EACHUSER) 
		if (np->np_seq == nr->nr_pack.np_seq)
			goto match;
	ndstat.ns_noumatch++;
	return (0);
match:
	if (nd.nd_lockserv == 0) {
		extern char *inet_ntoa();

		nd.nd_lockserv = 1;
		nd.nd_addr = np->np_ip.ip_src;
printf("Locking onto ND server at %s\n", inet_ntoa(nd.nd_addr));
		nd.nd_uver = np->np_ver & 0xF;
	}
	return (nr);
}

/*
 * Match a request packet with an active server request.
 * If no match, setup a request entry to handle it.
 */
struct ndreq *
ndsmatch(np)
	register struct ndpack *np;
{
	register struct ndreq *nr;
	register struct nduser *nu;
	register struct nddev *ndd;
	register struct buf *bp;
	register struct ndpack *npr;
	int i;
	struct ether_addr *ap,*arpgeten();
	struct mbuf *m;
	caddr_t buff;
	int maxpacks;

	for (EACHSERV)
		if (np->np_seq == nr->nr_pack.np_seq &&
		    np->np_ip.ip_src.s_addr == nr->nr_pack.np_ip.ip_dst.s_addr){
			return (nr);
		}
	if (nd.nd_son == 0)	/* if server off, skip new req's */
		return (0);
	if ((np->np_op & NDOPCODE) == NDOPWRITE && 
	    (np->np_ver & 0xF) != nd.nd_sver) {
		(void) nderrreply(np, ENXIO);
		goto badreq;
	}
	/*
	 * Decode the minor device field and see if there is a
	 * corresponding public or private filesystem.
	 */
	i = (np->np_min & NDMINUNIT);
	if (in_netof(np->np_ip.ip_src) == INADDR_ANY) {
		/*
		 * Client doesn't know his IP address, get the ether
		 * address from ARP and search for this.
		 */
		if ((ap = arpgeten(&np->np_ip.ip_src)) == 0)
			return (0);	/* if not an ether address, give up. */
		for (nu = &nd.nd_user[0]; nu < &nd.nd_user[NUSERS]; nu++) {
			if (bcmp((caddr_t)&nu->nu_eaddr, (caddr_t)ap,
			    sizeof (*ap)) == 0) {
				/*
				 * Force ARP mapping so client doesnt have
				 * to resolve 1st time.  Client will latch
				 * src and dst addresses.
				 */
				(void) arpseten(&nu->nu_address,
				    &nu->nu_eaddr, 0);
				np->np_ip.ip_src = nu->nu_address;
				break;
			}
		}
	} else {
		/*
		 * Client knows his IP address; search for it.
		 */
		for (nu = &nd.nd_user[0]; nu < &nd.nd_user[NUSERS]; nu++)
			if (nu->nu_address.s_addr == np->np_ip.ip_src.s_addr)
				break;
	}
	maxpacks = 0;
	if (nu < &nd.nd_user[NUSERS]) {
		nu->nu_numreq++;
		maxpacks = nu->nu_maxpacks;
	}
	if (maxpacks < 1)
		maxpacks = NDDEFMAXPACKS;
	if ((np->np_min & NDMINPUBLIC) == 0 ||	/* private or broadcast */
	    in_lnaof(np->np_ip.ip_dst) == 0) {
		if (nu >= &nd.nd_user[NUSERS])
			return (0);
	}
	if ((np->np_min & NDMINPUBLIC) == 0) {
		if (i >= NUNITS || (ndd = &nu->nu_unit[i])->nd_valid == 0) {
			return (0);
		}
	} else {	/* asking for a public unit */
		if (i >= NPUBLICS || (ndd = &nd.nd_public[i])->nd_valid == 0)
			return (0); /* don't reply, other serv might have it */
		if ((np->np_op & NDOPCODE) != NDOPREAD) {
			(void) nderrreply(np, EIO);
			goto badreq;
		}
	}
	if (np->np_bcount <= 0 || np->np_bcount > NDMAXIO) {
		(void) nderrreply(np, ENXIO);
		goto badreq;
	}
	if (np->np_blkno < BLKNO_FIRST && (np->np_blkno < 0 ||
	    (ndd->nd_size >= 0 && np->np_blkno >= ndd->nd_size))) {
		(void) nderrreply(np, ENXIO);
		goto badreq;
	}
	if (np->np_caddr != 0) {  /* not the initial sequence number */
		ndstat.ns_iseq++;
		goto badreq;
	}
	buff = ndmemall(np->np_bcount);
	if (buff == NULL) {
		ndstat.ns_nobufs++;
		return (0);
	}
	if ((m = m_get(M_DONTWAIT, MT_DATA)) == 0) {
		ndmemfree(buff, np->np_bcount);
		ndstat.ns_nobufs++;
		return (0);
	}
	m->m_off = MMINOFF;
	nr = mtod(m, struct ndreq *);
	bzero((caddr_t)nr, sizeof *nr);
	if ((m = m_get(M_DONTWAIT, MT_DATA)) == 0) {
		(void) m_free(dtom(nr));
		ndmemfree(buff, np->np_bcount);
		ndstat.ns_nobufs++;
		return (0);
	}
	m->m_off = MMINOFF;
	bp = mtod(m, struct buf *);
	bzero((caddr_t)bp, sizeof *bp);
	nr->nr_buf = bp;
	nr->nr_pbuf = buff;
	nr->nr_pcount = np->np_bcount;
	nr->nr_puse = 1;
	bp->b_un.b_addr = nr->nr_caddr = buff;
	bp->b_bcount = nr->nr_ccount = np->np_bcount;
	npr = &nr->nr_pack;
	bcopy((caddr_t)np, (caddr_t)npr, sizeof *np);
	npr->np_ip.ip_dst = npr->np_ip.ip_src;
	npr->np_ip.ip_src.s_addr = 0;
	npr->np_op &= NDOPCODE;
	npr->np_op |= NDOPDONE;
	npr->np_ver = nd.nd_sver;
	nr->nr_timer = NDXTIMER;	/* time to live */
	nr->nr_state = FIRST;
	nr->nr_xmaxpacks = maxpacks;
	if ((np->np_op & NDOPCODE) == NDOPREAD)
		bp->b_flags = B_READ;
	else
		bp->b_flags = B_WRITE;
	bp->b_flags |= (B_BUSY|B_CALL);
	bp->b_iodone = ndiodone;
	bp->b_blkno = np->np_blkno;
	npr->np_resid = bp->b_resid; 	/* save the resid (driver clobbers) */
	insque(nr, &nd.nd_qserv);
	nr->nr_inq = 1;
	if (np->np_blkno >= BLKNO_FIRST)
		switch (np->np_blkno) {
		case BLKNO_SIZE:
			if (ndd->nd_size < 0)
				*((int *)nr->nr_caddr) = 
			    (*bdevsw[major(ndd->nd_dev)].d_psize)(ndd->nd_dev);
			else
				*((int *)nr->nr_caddr) = ndd->nd_size;
			nr->nr_state = ACTIVE;
			break;

		default:
			ndsclear(nr);
			goto badreq;
		}
	else
		(void) ndmapbuf(bp, ndd);	/* map subfilesystems */
	return (nr);
badreq:
	ndstat.ns_badreq++;
	return (0);
}

/*
 * Return an error reply to a remote user.
 */
nderrreply(np, err)
	register struct ndpack *np;
{
	register struct ndpack *npr;

	if (in_lnaof(np->np_ip.ip_dst) == 0)
		return (0);		/* don't reply to broadcasts */
	if ((npr = ndgetnp(np)) == 0)
		return (0);
	npr->np_ip.ip_dst = npr->np_ip.ip_src;
	npr->np_ip.ip_src.s_addr = 0;
	npr->np_op = NDOPERROR;
	npr->np_error = err;
	npr->np_ver = nd.nd_sver;
	ndoutput(npr);
	return (1);
}

/*
 * Output a packet to the ip layer.
 */
ndoutput(np)
	register struct ndpack *np;
{
	register len = 0;
	register struct mbuf *m, *m0 = dtom(np);
	int error;

	np->np_seq = htonl(np->np_seq);
	np->np_blkno = htonl(np->np_blkno);
	np->np_bcount = htonl(np->np_bcount);
	np->np_resid = htonl(np->np_resid);
	np->np_caddr = htonl(np->np_caddr);
	np->np_ccount = htonl(np->np_ccount);

	for (m = m0; m; m = m->m_next)
		len += m->m_len;
	np->np_ip.ip_len = len;
	np->np_ip.ip_ttl = NDXTIMER/2 + 2;
	np->np_ip.ip_p = IPPROTO_ND;
	/* 
	 * no options, route to interface, allow broadcast.
	 */
	ndstat.ns_xpacks++;
	error = ip_output(m0, (struct mbuf *)0, (struct route *)0,
			IP_ROUTETOIF|IP_ALLOWBROADCAST);
	if (error)
		printf("nd: output error %d\n", error);
}

/*
 * Setup an ndpack buffer for transmission.  If npr is nonzero,
 * copy it to the newly allocated buffer.
 */
struct ndpack *
ndgetnp(npr)
	register struct ndpack *npr;
{
	register struct mbuf *m;
	register struct ndpack *np;

	if ((m = m_get(M_DONTWAIT, MT_DATA)) == 0) {
		ndstat.ns_nobufs++;
		return (0);
	}
	m->m_off = MMAXOFF - sizeof *np;
	m->m_len = sizeof *np;
	np = mtod(m, struct ndpack *);
	if (npr)
		bcopy((caddr_t)npr, (caddr_t)np, sizeof *np);
	return (np);
}

/*
 * Send up to nr_maxpacks of data packets.
 */
ndsenddata(nr)
	register struct ndreq *nr;
{
	register int i;
	register struct mbuf *m;
	register struct ndpack *np, *npr;
	int sending;
	struct mbuf *n, *mclgetx();
	int ndclfree();

	npr = &nr->nr_pack;
	sending = 1;
	while (sending) {
		if ((m = m_get(M_DONTWAIT, MT_DATA)) == 0) 
			goto nobuf;
		m->m_off = MMAXOFF - sizeof *np;
		m->m_len = sizeof *np;
		np = mtod(m, struct ndpack *);
		bcopy((caddr_t)npr, (caddr_t)np, sizeof *np);
		np->np_ccount = i = MIN(NDMAXDATA, nr->nr_ccount);
		n = mclgetx(ndclfree, (int)nr, nr->nr_caddr, i, M_DONTWAIT);
		if (n == 0) {
			(void) m_free(m);
			goto nobuf;
		}
		if (nr->nr_puse)
			nr->nr_puse++;
		m->m_next = n;
		nr->nr_caddr += i;
		nr->nr_ccount -= i;
		npr->np_caddr += i;
		/* 
		 * if write count exhausted or nr_maxpacks, set
		 * the NDOPWAIT bit to let the server know
		 * that we're waiting for an NDOPDONE.
		 */
		if (nr->nr_ccount <= 0 ||
		    ++nr->nr_xcount >= nr->nr_xmaxpacks) {
			np->np_op |= NDOPWAIT;
			nr->nr_state = DONEWAIT;
			sending = 0;
		}
		ndoutput(np);
	}
	return;
nobuf:
	ndstat.ns_nobufs++;
}

/*
 * Clear a server request
 */
ndsclear(nr)
	register struct ndreq *nr;
{
	int s = splimp();

	if (nr->nr_inq) {
		remque(nr);
		nr->nr_inq = 0;
		ndclfree(nr);
	}
	(void) splx(s);
}

/*
 * Free server buffer after last use
 */
ndclfree(nr)
	register struct ndreq *nr;
{

	if (nr->nr_puse && --nr->nr_puse == 0) {
		ndmemfree(nr->nr_pbuf, (long)nr->nr_pcount);
		if (nr->nr_buf)
			(void) m_free(dtom(nr->nr_buf));
		(void) m_free(dtom(nr));
	}
}

/*
 * Clear out a user request structure.
 */
ndclear(nr)
	register struct ndreq *nr;
{

	remque(nr);
	(void) m_free(dtom(nr));
}

/*
 * Iodone routine, called from biodone (via b_iodone) when a
 * server disk request completes.  bp->b_flags & B_DONE
 * has already been set, just request an ND software interrupt
 * so we can find it.
 */
/*ARGSUSED*/
ndiodone(bp)
	struct buf *bp;
{
	int ndintr();

	softcall(ndintr, (caddr_t)0);
}

/*
 * Called at splnet when server disk IO completes.
 * Find the request and continue processing.
 */
ndintr()
{
	register struct ndreq *nr;
	int s = splnet();

top:	/* re-enter when nd_qserv modified */
	for (EACHSERV) {
		if (nr->nr_state != IOWAIT) 
			continue;
		if (nr->nr_buf->b_flags & B_DONE) {
			nr->nr_state = IODONE;
			if (ndsout(nr))
				goto top;
		}
	}
	(void) splx(s);
}

/*
 * Call real disk strategy routine to start IO.
 */
nddiskstrat(nr)
	register struct ndreq *nr;
{
	register struct buf *bp;

	bp = nr->nr_buf;
#ifdef ndsafe
	if (bp->b_dev != ndsafe) {
		printf("ndsafe %x ", bp->b_dev);
		bp->b_dev = ndsafe;
	}
#endif ndsafe
	(*bdevsw[major(bp->b_dev)].d_strategy)(bp);
	nr->nr_state = IOWAIT;
}


/*
 * Allocate and free memory from nd server memory pool.
 */
int ndmemout, ndmemwant;

caddr_t
ndmemall(size)
	long size;
{
	int s = splimp();
	caddr_t addr;
	long rmalloc();

	size = roundup(size, NDMAXDATA);
	addr = (caddr_t)rmalloc(nd.nd_memmap, size);
	if (addr)
		ndmemout += size;
	(void) splx(s);
	return (addr);
}

ndmemfree(addr, size)
	char *addr;
	long size;
{
	int s = splimp();

	size = roundup(size, NDMAXDATA);
	rmfree(nd.nd_memmap, size, (long)addr);
	ndmemout -= size;
	if (ndmemwant)
		wakeup((caddr_t)&ndmemwant);
	(void) splx(s);
}

ndmemwait()
{
	int s = splimp();

	while (ndmemout > 0) {
		ndmemwant++;
		(void) sleep((caddr_t)&ndmemwant, PZERO-1);
	}
	ndmemwant = 0;
	(void) splx(s);
}

/*
 * Crash dump.  Must have interrupts enabled to use net.
 */
nddump(dev, addr, blkno, nblk)
	dev_t dev;
	caddr_t addr;
	daddr_t blkno, nblk;
{
	static struct buf b;
	int nsect, s;
	int ndnull();
	int err = 0;

	if (minor(dev) != 1)		/* only allow "swap" device */
		return (ENXIO);
	if (ndif == 0)
		return (ENXIO);
	(*ndif->if_init)(ndif->if_unit);
	bzero((caddr_t)&b, sizeof b);
	s = spl0();
	do {
		b.b_bcount = MIN(NDMAXIO, dbtob(nblk));
		b.b_un.b_addr = addr;
		b.b_flags = (B_WRITE | B_BUSY | B_CALL);
		b.b_iodone = ndnull;
		b.b_dev = makedev(0, 1);
		b.b_blkno = blkno;
		ndstrategy(&b);
		CDELAY((b.b_flags & B_DONE), 4000000)		/* ~4 seconds */
		if ((b.b_flags & (B_DONE | B_ERROR)) != B_DONE) {
			err = EIO;
			break;
		}
		addr += b.b_bcount;
		nsect = btodb(b.b_bcount);
		nblk -= nsect;
		blkno += nsect;
	} while (nblk > 0);
	(void) splx(s);
	return (err);
}

ndnull()
{

	return (0);
}
#endif NND > 0
