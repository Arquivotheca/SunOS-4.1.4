#ifndef lint
static	char sccsid[] = "@(#)sg.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 *  Sun-2 Sound Generator Driver (TI SN76489AN)
 */
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../sun/pte.h"
#include "../sundev/mbvar.h"
#include "../sundev/video.h"

/*
 * Driver information for auto-configuration stuff.
 */
int	sgprobe(), sgattach();
struct	mb_device *sginfo[1];	/* XXX only supports 1 chip */
u_long	sgstd[] = { 0 };	/* no standard addresses */
struct	mb_driver sgdriver = {
	sgprobe, 0, sgattach, 0, 0, 0, sgstd, 0, 1, "sg", sginfo, 0, 0, 0
};

/*
 * States driver can be in.
 */
#define	S_CLOSED	0
#define	S_OPEN		1
#define	S_DELAY1	2
#define	S_DELAY2	3
#define	S_DELAY3	4
#define	S_DELAY4	5

int	sgstate;	/* current state of driver */
u_int	sgdelay;	/* microseconds to delay */

#define	SGBUFSIZ	512
#define	SGPRI		29		/* priority to sleep at when delaying */

extern	wakeup();
extern	int hz;

/*ARGSUSED*/
sgprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register int c;

	if (pokec((char *)reg, 0)) 
		return (0);
	return (1);
}

/*ARGSUSED*/
sgattach(md)
	register struct mb_device *md;
{

}

/*ARGSUSED*/
sgopen(dev, flag)
	dev_t dev;
	int flag;
{
	struct videoctl *vc = (struct videoctl *)0xee3800;
 
	if (sgstate != S_CLOSED)
		return (ENXIO);
	sgstate = S_OPEN;
	sgdelay = 0;
	vc->vc_sg_en = 1;
	return (0);
}

/*ARGSUSED*/
sgclose(dev, flag)
	dev_t dev;
	int flag;
{
	struct videoctl *vc = (struct videoctl *)0xee3800;
 
	vc->vc_sg_en = 0;
	if (sgdelay) {
		untimeout(wakeup, (caddr_t)&sgdelay);
		sgdelay = 0;
	}
	sgstate = S_CLOSED;
	return (0);
}

/*ARGSUSED*/
sgwrite(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	register u_char *sgp = (u_char *)sginfo[0]->md_addr;
	register u_char *cp;
	register int cc;
	int error = 0;
	u_char obuf[SGBUFSIZ];

	/*
	 * Process the user's data in at most SGBUFSIZ chunks.
	 */
	while (uio->uio_resid > 0) {
		/*
		 * Grab a hunk of data from the user.
		 */
		cc = uio->uio_iov->iov_len;
		if (cc == 0) {
			uio->uio_iovcnt--;
			uio->uio_iov++;
			if (uio->uio_iovcnt < 0) {
				printf("sg: iovcnt < 0\n");
				error = EIO;
				break;
			}
			continue;
		}
		if (cc > SGBUFSIZ)
			cc = SGBUFSIZ;
		cp = obuf;
		error = uiomove(cp, cc, UIO_WRITE, uio);
		if (error)
			break;
		while (cc > 0) {
			switch (sgstate) {
			case S_CLOSED:
			case S_OPEN:
			default:
				if ((*cp&0xc0) != 0x40) {
					*sgp = *cp;
					DELAY(8);
				} else
					sgstate = S_DELAY1;
				break;

			case S_DELAY1:
			case S_DELAY2:
			case S_DELAY3:
				sgdelay <<= 8;
				sgdelay |= *cp;
				sgstate++;
				break;

			case S_DELAY4:
				sgdelay <<= 8;
				sgdelay |= *cp;
				if (sgdelay < 4*(1000000/hz)) {
					DELAY(sgdelay);
				} else {
					int s = spl6();

					timeout(wakeup, (caddr_t)&sgdelay,
					    sgdelay / (1000000/hz));
					sleep((caddr_t)&sgdelay, SGPRI);
					(void) splx(s);
				}
				sgdelay = 0;
				sgstate = S_OPEN;
				break;
			}
			cp++;
			cc--;
		}
	}
	return (error);
}
