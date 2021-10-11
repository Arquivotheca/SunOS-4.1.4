#ifndef lint
static	char sccsid[] = "@(#)oct.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "oct.h"
#if NOCT > 0
/*
 * Central Data Octal Serial Board driver
 *
 * This driver mimics dh.c; see it for explanation of common code.
 *
 * TODO:
 *	test modem control
 */
#include "bk.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/bk.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"

#include "../sundev/mbvar.h"
#include "../sundev/octreg.h"

/*
 * Driver information for auto-configuration stuff.
 */
int	octprobe(), octattach(), octintr();
struct	mb_device *octinfo[NOCT];
u_long	octstd[] = { 0x520, 0 };
struct	mb_driver octdriver =
	{ octprobe, 0, octattach, 0, 0, octintr, octstd, 0, 0, "oct", octinfo };

#define	NOCTLINE 	(NOCT*8)
 
int	octstart();
int	ttrstrt();
struct	tty oct_tty[NOCTLINE];

/*
 * Software carrier.
 */
char	octsoftCAR[NOCT];

/*
 * The octal serial card doesn't interrupt on carrier
 * transitions, so we have to use a timer to watch it.
 */
char	oct_timer;		/* timer started? */

char	oct_speeds[] =
	{ 0,0,1,2,3,4,9,5,6,7,8,10,12,14,15,13 };
 
#ifndef PORTSELECTOR
#define	ISPEED	B9600
#define	IFLAGS	(EVENP|ODDP|ECHO)
#else
#define	ISPEED	B4800
#define	IFLAGS	(EVENP|ODDP)
#endif

octprobe(reg)
	caddr_t reg;
{

	if (peek((short *)reg) < 0)
		return (0);
	return (8 * sizeof (struct device));
}

octattach(md)
	register struct mb_device *md;
{
	register struct tty *tp = &oct_tty[md->md_unit*8];
	register struct device *octaddr = (struct device *)md->md_addr;
	register int cntr;
	extern octscan();

	for (cntr = 0; cntr < 8; cntr++) {
		tp->t_addr = (caddr_t)octaddr;
		tp++;
		octaddr++;
	}
	octsoftCAR[md->md_unit] = md->md_flags;
	if (oct_timer == 0) {
		oct_timer++;
		timeout(octscan, (caddr_t)0, hz);
	}
}

/*ARGSUSED*/
octopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
 
	unit = minor(dev);
	if (unit >= NOCTLINE)
		return (ENXIO);
	tp = &oct_tty[unit];
	if (tp->t_addr == 0)
		return (ENXIO);
	tp->t_oproc = octstart;
	tp->t_state |= TS_WOPEN;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = ISPEED;
		tp->t_flags = IFLAGS;
		/* tp->t_state |= TS_HUPCLS; */
		octparam(unit);
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	(void) octmctl(dev, OCT_ON, DMSET);
	(void) spl5();
	while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	(void) spl0();
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}
 
/*ARGSUSED*/
octclose(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	register struct device *octaddr;
 
	unit = minor(dev);
	tp = &oct_tty[unit];
	(*linesw[tp->t_line].l_close)(tp);
	octaddr = (struct device *)tp->t_addr;
	octaddr->octcmd &= ~OCT_BREAK;
	if ((tp->t_state&(TS_HUPCLS|TS_WOPEN)) || (tp->t_state&TS_ISOPEN) == 0)
		(void) octmctl(dev, OCT_OFF, DMSET);
	ttyclose(tp);
}
 
octread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
 
	tp = &oct_tty[minor(dev)];
	(*linesw[tp->t_line].l_read)(tp, uio);
}
 
octwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
 
	tp = &oct_tty[minor(dev)];
	(*linesw[tp->t_line].l_write)(tp, uio);
}
 
octintr()
{
	register struct tty *tp;
	register u_char s;
	register int c;
	register struct device *octaddr;
	int serviced = 0;
 
	/*
	 * First service all receiver interrupts
	 * so as not to lose characters.
	 */
	for (tp = oct_tty; tp < &oct_tty[NOCTLINE]; tp++) {
		octaddr = (struct device *)tp->t_addr;
		while ((s = octaddr->octstat) & OCT_DONE) { /* char present */
			c = octaddr->octdata;
			serviced++;
			if ((tp->t_state & TS_ISOPEN) == 0) {
				wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
				if ((tp->t_state&TS_WOPEN) == 0)
#endif
				continue;
			}
			if (s&OCT_FE) {
				octaddr->octcmd |= OCT_RESET;
				if (tp->t_flags & RAW)
					c = 0;
				else
					c = tp->t_intrc;
			}
#ifdef notdef
			if (s&OCT_DO && overrun == 0) {
				octaddr->octcmd |= OCT_RESET;
				printf("oct%d,%d: silo overflow\n",
					minor(tp->t_dev)>>3,
					minor(tp->t_dev)&7);
				overrun = 1;
			}
#endif
			if (s&OCT_PE) {
				octaddr->octcmd |= OCT_RESET;
				if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
				  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
					continue;
			}
#if NBK > 0
			if (tp->t_line == NETLDISC) {
				c &= 0177;
				BKINPUT(c, tp);
			} else
#endif
				(*linesw[tp->t_line].l_rint)(c, tp);
		}
	} /* end of "for unit" */

	/*
	 * Now service all transmitter interrupts.
	 */
	for (tp = oct_tty; tp < &oct_tty[NOCTLINE]; tp++) {
		octaddr = (struct device *)tp->t_addr;
		if (octaddr->octstat & OCT_READY) {
			octaddr->octcmd &= ~OCT_TXEN;
			tp->t_state &= ~TS_BUSY;
			serviced++;
			if (tp->t_line)
				(*linesw[tp->t_line].l_start)(tp);
			else
				octstart(tp);
		}
	}
	return (serviced);
}
 
/*ARGSUSED*/
octioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register struct device *octaddr;
	int error;
 
	tp = &oct_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
		if (cmd==TIOCSETP || cmd==TIOCSETN)
			octparam(unit);
		return (error);
	}
	switch(cmd) {

	case TIOCSBRK:
		octaddr = (struct device *)tp->t_addr;
		octaddr->octcmd |= OCT_BREAK;
		break;
	case TIOCCBRK:
		octaddr = (struct device *)tp->t_addr;
		octaddr->octcmd &= ~OCT_BREAK;
		break;
	case TIOCSDTR:
		octaddr = (struct device *)tp->t_addr;
		octaddr->octcmd |= OCT_DTR;
		break;
	case TIOCCDTR:
		octaddr = (struct device *)tp->t_addr;
		octaddr->octcmd &= ~OCT_DTR;
		break;
	case TIOCMSET:
		(void) octmctl(dev, dmtooct(*(int *)data), DMSET);
		break;
	case TIOCMBIS:
		(void) octmctl(dev, dmtooct(*(int *)data), DMBIS);
		break;
	case TIOCMBIC:
		(void) octmctl(dev, dmtooct(*(int *)data), DMBIC);
		break;
	case TIOCMGET:
		*(int *)data = octtodm(octmctl(dev, 0, DMGET));
		break;
	default:
		return (ENOTTY);
	}
	return (0);
}

dmtooct(bits)
	register int bits;
{
	register int b = 0;

	if (bits & DML_DTR) b |= OCT_DTR;
	if (bits & DML_CAR) b |= OCT_DSR;
	if (bits & DML_RTS) b |= OCT_RTS;
	if (bits & DML_DSR) b |= OCT_DSR;
	return (b);
}

octtodm(bits)
	register int bits;
{
	register int b = 0;

	if (bits & OCT_DTR) b |= DML_DTR;
	if (bits & OCT_DSR) b |= DML_DSR|DML_CAR;
	if (bits & OCT_RTS) b |= DML_RTS;
	return (b);
}
 
octparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct device *octaddr;
	register int mr1, mr2;
	int s;
 
	tp = &oct_tty[unit];
	octaddr = (struct device *)tp->t_addr;
	if (tp->t_ispeed == 0) {
		octaddr->octcmd &= ~OCT_DTR;	/* hang up the line */
		return;
	}
	mr2 = oct_speeds[tp->t_ispeed&017] | OCT_MR2_INIT;
	if (tp->t_ispeed == B134)
		mr1 = OCT_BITS6|OCT_PENABLE|OCT_EPAR|OCT_X1_CLK;
	else if (tp->t_flags&(RAW|LITOUT))
		mr1 = OCT_BITS8|OCT_X1_CLK;
	else
		mr1 = OCT_BITS7|OCT_PENABLE|OCT_X1_CLK;
	if (tp->t_flags & EVENP)
		mr1 |= OCT_EPAR;
	if (tp->t_ispeed == B110)
		mr1 |= OCT_TWOSB;
	else if (tp->t_ispeed == B134)
		mr1 |= OCT_ONEHSB;
	else
		mr1 |= OCT_ONESB;
	s = spl5();
	octaddr->octcmd |= OCT_RESET;
	octaddr->octmode = mr1;
	octaddr->octmode = mr2;
	octaddr->octcmd |= OCT_RXEN;
	(void) splx(s);
}
 
octstart(tp)
	register struct tty *tp;
{
	register struct device *octaddr;
	register int c;
	int s;
 
	octaddr = (struct device *)tp->t_addr;
	s = spl5();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	c = getc(&tp->t_outq);
	if (c <= 0177 || (tp->t_flags&(RAW|LITOUT))) {
		octaddr->octcmd |= OCT_TXEN;
		octaddr->octdata = c;		/* 2651 adds parity */
	} else {
		timeout(ttrstrt, (caddr_t)tp, c&0x7f);
		tp->t_state |= TS_TIMEOUT;
		goto out;
	}
	tp->t_state |= TS_BUSY;
out:
	(void) splx(s);
}
 
octmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct device *octaddr;
	register int unit, mbits, b, s;

	unit = minor(dev);
	octaddr = (struct device *)oct_tty[unit].t_addr;
	s = spl5();
	mbits = octaddr->octstat&(OCT_CD|OCT_DSR);
	mbits |= octaddr->octcmd&(OCT_DTR|OCT_RTS);
	switch (how) {
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return (mbits);
	}
	b = octaddr->octcmd;
	b &= ~(OCT_DTR|OCT_RTS);
	b |= mbits&(OCT_DTR|OCT_RTS);
	octaddr->octcmd = b;
	(void) splx(s);
	return (mbits);
}
 
octscan()
{
	register struct device *octaddr;
	register struct tty *tp;
	register int i, car;
 
	for (i = 0, tp = oct_tty; i < NOCTLINE ; i++, tp++) {
		octaddr = (struct device *)tp->t_addr;
		if (octaddr == 0)
			continue;
		car = 0;
		if (octsoftCAR[i>>3]&(1<<(i&07)))
			car = 1;
		else
			car = octaddr->octstat&OCT_DSR;	/* no CD, use DSR */
		if (car) {
			/* carrier present */
			if ((tp->t_state & TS_CARR_ON) == 0) {
				wakeup((caddr_t)&tp->t_rawq);
				tp->t_state |= TS_CARR_ON;
			}
		} else {
			if ((tp->t_state&TS_CARR_ON) &&
			    (tp->t_flags&NOHANG)==0) {
				/* carrier lost */
				if (tp->t_state&TS_ISOPEN) {
					gsignal(tp->t_pgrp, SIGHUP);
					gsignal(tp->t_pgrp, SIGCONT);
					octaddr->octcmd &= ~OCT_DTR;
					ttyflush(tp, FREAD|FWRITE);
				}
				tp->t_state &= ~TS_CARR_ON;
			}
		}
	}
	timeout(octscan, (caddr_t)0, 2*hz);
}

/*
 * Reset state of driver if Multibus reset was necessary.
 * Reset parameters and restart transmission on open lines.
 */
octreset()
{
	register int unit;
	register struct tty *tp;
	register struct mb_device *md;

	for (unit = 0; unit < NOCTLINE; unit++) {
		md = octinfo[unit >> 3];
		if (md == 0 || md->md_alive == 0)
			continue;
		if (unit%8 == 0)
			printf(" oct%d", unit>>3);
		tp = &oct_tty[unit];
		if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
			octparam(unit);
			(void) octmctl(unit, OCT_ON, DMSET);
			tp->t_state &= ~TS_BUSY;
			octstart(tp);
		}
	}
}
#endif
