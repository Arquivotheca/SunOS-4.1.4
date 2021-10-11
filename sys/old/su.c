#ifndef lint
static	char sccsid[] = "@(#)su.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "su.h"
#if NSU > 0
/*
 *  Sun-1 on-board UART driver.
 *
 *  NOTE:
 *	This driver is specific to the Sun-1.
 *	Actually, is it a driver for the Intel 8274
 *	Multi-Protocol Serial Controller chip.
 *
 * TODO:
 *	test modem control
 *	try status-affects-vector mode
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

#include "../sun/clock.h"
#include "../sun/mmu.h"
#include "../sun/cpu.h"
#include "../sundev/mbvar.h"
#include "../sundev/sureg.h"

#include <mon/sunmon.h>

/*
 * Driver information for auto-configuration stuff.
 */
int	suprobe(), suattach(), suintr();
struct	mb_device *suinfo[NSU];
u_long	sustd[] = { SUADDR, 0 };
struct	mb_driver sudriver = {
	suprobe, 0, suattach, 0, 0, suintr, sustd, 0, 0,
	"su", suinfo, 0, 0, MDR_OBIO
};

#define	NSULINE 	(NSU*2)
 
int	sustart();
int	ttrstrt();
struct	tty su_tty[NSULINE];
int	su_cnt = { NSULINE };
int	suact;

#ifdef BIGBUFFER
/*
 * Buffer and pointers used by asm code in locore.s
 * to scan for input characters every clock tick.
 */
char	suibuf[100*3];		/* input buffer (100 chars * 3 bytes/char) */
char	*suwptr = suibuf;	/* write pointer */
char	*surptr = suibuf;	/* read pointer */
char	*suend = &suibuf[sizeof (suibuf)];
int	su_timer;		/* input timer started? */
int	supoll();
#endif

/*
 * Software copy of write only registers
 */
char	su_wr5[NSULINE];	/* modem control, mostly */
short	su_clk[NSULINE];	/* clock rate (speed) */

char	susoftCAR[NSU];

/*
 * Disable line if used for console I/O
 */
extern	char sudisable[NSULINE];

short	su_speeds[] =
	{ 0,3072,2048,1396,1142,1024,768,512,256,128,85,64,32,16,8,4 };
 
#define	ISPEED	B9600
#define	IFLAGS	(EVENP|ODDP|ECHO)

#ifdef BIGBUFFER
#define	splsu	spl6
#else
#define	splsu	spl5
#endif

suprobe(reg)
	caddr_t reg;
{

	if (*RomVecPtr->v_InSource == IoUARTA)
		sudisable[0] = 1;
	else if (*RomVecPtr->v_InSource == IoUARTB)
		sudisable[1] = 1;
	return (cpu == SUN_1);
}

suattach(md)
	register struct mb_device *md;
{
	register struct tty *tp = &su_tty[md->md_unit*2];
	register struct device *suaddr = (struct device *)md->md_addr;
	register int i;

	for (i = 0; i < 2; i++, tp++, suaddr++) {
		tp->t_addr = (caddr_t)suaddr;
		suaddr->sucsr = SUWR0_RESET_ALL;
		DELAY(4);
		suaddr->sucsr = 2;
		if (i == 0)	/* channel A only */
			suaddr->sucsr = SUWR2_INIT;
		else		/* channel B only */
			suaddr->sucsr = 1;
		/*
		 * Set up enough characteristics so
		 * monitor "ut" will still work.
		 */
		suaddr->sucsr = 4;
		suaddr->sucsr = SUWR4_1_STOP|SUWR4_X16_CLK;
		suaddr->sucsr = 3;
		suaddr->sucsr = SUWR3_RX_8|SUWR3_INIT;
		suaddr->sucsr = 5;
		suaddr->sucsr = SUWR5_TX_8|SUWR5_TXENABLE|SUWR5_DTR|SUWR5_RTS;
	}
	susoftCAR[md->md_unit] = md->md_flags;
#ifdef BIGBUFFER
	if (su_timer == 0) {
		su_timer = 1;
		timeout(supoll, (caddr_t)0, 1);
	}
#endif
}

/*ARGSUSED*/
suopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
 
	unit = minor(dev);
	if (unit >= su_cnt)
		return (ENXIO);
	tp = &su_tty[unit];
	tp->t_oproc = sustart;
	tp->t_state |= TS_WOPEN;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = ISPEED;
		tp->t_flags = IFLAGS;
		/* tp->t_state |= TS_HUPCLS; */
		suparam(unit);
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	(void) sumctl(dev, SU_ON, DMSET);
	(void) splsu();
	if ((susoftCAR[unit>>1]&(1<<(unit&1))) ||
	    (sumctl(dev, 0, DMGET)&SURR0_CD))
		tp->t_state |= TS_CARR_ON;
	while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	(void) spl0();
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}
 
/*ARGSUSED*/
suclose(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	register struct device *suaddr;
 
	unit = minor(dev);
	tp = &su_tty[unit];
	(*linesw[tp->t_line].l_close)(tp);
	suaddr = (struct device *)tp->t_addr;
	(void) splsu();
	suaddr->sucsr = 5;
	suaddr->sucsr = (su_wr5[unit] &= ~SUWR5_BREAK);
	(void) spl0();
	if ((tp->t_state&(TS_HUPCLS|TS_WOPEN)) || (tp->t_state&TS_ISOPEN) == 0)
		(void) sumctl(dev, SU_OFF, DMSET);
	ttyclose(tp);
	(void) splsu();
	if ((tp->t_state & (TS_ISOPEN|TS_WOPEN)) == 0) {
		suaddr->sucsr = 1;
		suaddr->sucsr = SUWR1_INIT & ~SUWR1_RIE;
	}
	(void) spl0();
}
 
suread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
 
	tp = &su_tty[minor(dev)];
	(*linesw[tp->t_line].l_read)(tp, uio);
}
 
suwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
 
	tp = &su_tty[minor(dev)];
	(*linesw[tp->t_line].l_write)(tp, uio);
}
 
#ifdef BIGBUFFER
supoll()
{
	register struct tty *tp;
	register int c, s1;
	register int unit;
	struct device *suaddr;
	int s;

	while (surptr != suwptr) {
		unit = *surptr++;
		tp = &su_tty[unit];
		c = *surptr++;
		s1 = *surptr++;
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
			if ((tp->t_state&TS_WOPEN) == 0)
#endif
			goto cont;
		}
		if (s1 & SURR1_FE)
			if (tp->t_flags & RAW)
				c = 0;
			else
				c = tp->t_intrc;
		if (s1&SURR1_DO) {
			s = splsu();
			suaddr = (struct device *)tp->t_addr;
			suaddr->sucsr = 1;
			suaddr->sucsr = SUWR1_INIT;
			(void) splx(s);
			/* printf("su,%c: silo overflow\n", 'a'+unit); */
		}
		if (s1&SURR1_PE)
			if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
			  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
				goto cont;
#if NBK > 0
		if (tp->t_line == NETLDISC) {
			c &= 0177;
			BKINPUT(c, tp);
		} else
#endif
			(*linesw[tp->t_line].l_rint)(c, tp);
cont:
		if (surptr >= suend)
			surptr = suibuf;
	}
	timeout(supoll, (caddr_t)0, 1);
}
#endif

suintr()
{
	register struct tty *tp;
	register int c, s0, s1;
	register struct device *suaddr;
	register int unit, st, overrun, serviced = 0;
 
	for (tp = su_tty, unit = 0; unit < NSULINE; tp++, unit++) {
	if (sudisable[unit])
		continue;
	suaddr = (struct device *)tp->t_addr;
	suaddr->sucsr = SUWR0_RESET_STATUS;
	if (suaddr->sucsr & SURR0_CD) {	/* carrier up? */
		if ((tp->t_state&TS_CARR_ON) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
			tp->t_state |= TS_CARR_ON;
		}
	} else {	/* no carrier */
		if (tp->t_state&TS_CARR_ON) {
			gsignal(tp->t_pgrp, SIGHUP);
			gsignal(tp->t_pgrp, SIGCONT);
			flushtty(tp, FREAD|FWRITE);
		}
		tp->t_state &= ~TS_CARR_ON;
	}
#ifndef BIGBUFFER
	overrun = 0;
	st = SURR0_DONE|SURR0_BREAK;
	while ((s0 = suaddr->sucsr) & st) {
		c = suaddr->subuf;
		suaddr->sucsr = 1;
		s1 = suaddr->sucsr;
		suaddr->sucsr = SUWR0_RESET_ERRORS;
		st = SURR0_DONE;	/* don't look for break again */
		serviced++;
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
			if ((tp->t_state&TS_WOPEN) == 0)
#endif
			continue;
		}
		if ((s0 & SURR0_BREAK) /* || (s1 & SURR1_FE) */)
			if (tp->t_flags & RAW)
				c = 0;
			else
				c = tp->t_intrc;
		if ((s1&SURR1_DO) /* && overrun == 0 */) {
			suaddr->sucsr = 1;
			suaddr->sucsr = SUWR1_INIT;
			if (overrun == 0) {
				/* printf("su,%c: silo overflow\n", 'a'+unit); */
				overrun = 1;
			}
		}
		if (s1&SURR1_PE)
			if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
			  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
				continue;
#if NBK > 0
		if (tp->t_line == NETLDISC) {
			c &= 0177;
			BKINPUT(c, tp);
		} else
#endif
			(*linesw[tp->t_line].l_rint)(c, tp);
	}
	}
	for (tp = su_tty, unit = 0; unit < NSULINE; tp++, unit++) {
	if (sudisable[unit])
		continue;
	suaddr = (struct device *)tp->t_addr;
#endif BIGBUFFER
	if (suaddr->sucsr&SURR0_READY) {
		suaddr->sucsr = SUWR0_RESET_TXINT;
		serviced++;
		tp->t_state &= ~TS_BUSY;
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			sustart(tp);
	}
	}
	return (serviced);
}
 
/*ARGSUSED*/
suioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register struct device *suaddr;
	int error;
 
	tp = &su_tty[unit];
	suaddr = (struct device *)tp->t_addr;
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
		if (cmd == TIOCSETP || cmd == TIOCSETN)
			suparam(unit);
		return (error);
	}
	switch(cmd) {

	case TIOCSBRK:
		(void) splsu();
		suaddr->sucsr = 5;
		suaddr->sucsr = (su_wr5[unit] |= SUWR5_BREAK);
		(void) spl0();
		break;
	case TIOCCBRK:
		(void) splsu();
		suaddr->sucsr = 5;
		suaddr->sucsr = (su_wr5[unit] &= ~SUWR5_BREAK);
		(void) spl0();
		break;
	case TIOCSDTR:
		(void) sumctl(dev, SU_ON, DMBIS);
		break;
	case TIOCCDTR:
		(void) sumctl(dev, SU_OFF, DMBIC);
		break;
	case TIOCMSET:
		(void) sumctl(dev, dmtosu(*(int *)data), DMSET);
		break;
	case TIOCMBIS:
		(void) sumctl(dev, dmtosu(*(int *)data), DMBIS);
		break;
	case TIOCMBIC:
		(void) sumctl(dev, dmtosu(*(int *)data), DMBIC);
		break;
	case TIOCMGET:
		*(int *)data = sutodm(sumctl(dev, 0, DMGET));
		break;
	default:
		return (ENOTTY);
	}
	return (0);
}

dmtosu(bits)
	register int bits;
{
	register int b = 0;

	if (bits & DML_CAR) b |= SURR0_CD;
	if (bits & DML_CTS) b |= SURR0_CTS;
	if (bits & DML_RTS) b |= SUWR5_RTS;
	if (bits & DML_DTR) b |= SUWR5_DTR;
	return (b);
}

sutodm(bits)
	register int bits;
{
	register int b = 0;

	if (bits & SURR0_CD) b |= DML_CAR;
	if (bits & SURR0_CTS) b |= DML_CTS;
	if (bits & SUWR5_RTS) b |= DML_RTS;
	if (bits & SUWR5_DTR) b |= DML_DTR;
	return (b);
}
 
suparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct device *suaddr;
	register int wr3, wr4, wr5, speed;
	int s;
 
	tp = &su_tty[unit];
	suaddr = (struct device *)tp->t_addr;
	s = splsu();
	suaddr->sucsr = 1;
	suaddr->sucsr = SUWR1_INIT;
	if (tp->t_ispeed == 0) {
		(void) sumctl(unit, SU_OFF, DMSET);	/* hang up line */
		(void) splx(s);
		return;
	}
	wr3 = SUWR3_INIT;
	wr4 = SUWR4_X16_CLK;
	wr5 = (su_wr5[unit] & (SUWR5_RTS|SUWR5_DTR)) | SUWR5_TXENABLE;
	if (tp->t_ispeed == B134) {	/* what a joke! */
		wr3 |= SUWR3_RX_6;
		wr4 |= SUWR4_PARITY_ENABLE | SUWR4_PARITY_EVEN;
		wr5 |= SUWR5_TX_6;
	} else if (tp->t_flags&(RAW|LITOUT)) {
		wr3 |= SUWR3_RX_8;
		wr5 |= SUWR5_TX_8;
	} else {
		wr3 |= SUWR3_RX_7;
		wr4 |= SUWR4_PARITY_ENABLE;
		wr5 |= SUWR5_TX_7;
	}
	if (tp->t_flags & EVENP)
		wr4 |= SUWR4_PARITY_EVEN;
	if (tp->t_ispeed == B110)
		wr4 |= SUWR4_2_STOP;
	else if (tp->t_ispeed == B134)
		wr4 |= SUWR4_1_5_STOP;
	else
		wr4 |= SUWR4_1_STOP;
	suaddr->sucsr = 4; suaddr->sucsr = wr4;
	suaddr->sucsr = 3; suaddr->sucsr = wr3;
	suaddr->sucsr = 5; suaddr->sucsr = wr5;
	su_wr5[unit] = wr5;
	speed = su_speeds[tp->t_ispeed&017];
	if (su_clk[unit] != speed) {
		CLKADDR->clk_cmd = CLK_LMODE | (UARTTIMER+unit);
		CLKADDR->clk_data = CLK_UART_MODE;
		CLKADDR->clk_cmd = CLK_LLOAD | (UARTTIMER+unit);
		CLKADDR->clk_data = speed;
		CLKADDR->clk_cmd = CLK_GO | (1<<(UARTTIMER-1+unit));
		su_clk[unit] = speed;
	}
	(void) splx(s);
}
 
sustart(tp)
	register struct tty *tp;
{
	register struct device *suaddr;
	register int c, s;
 
	suaddr = (struct device *)tp->t_addr;
	s = splsu();
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
		suaddr->subuf = c;	/* 8274 adds parity if desired */
		tp->t_state |= TS_BUSY;
	} else {
		timeout(ttrstrt, (caddr_t)tp, c&0177);
		tp->t_state |= TS_TIMEOUT;
	}
out:
	(void) splx(s);
}
 
sumctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct device *suaddr;
	register int unit, mbits, s;

	unit = minor(dev);
	suaddr = (struct device *)su_tty[unit].t_addr;
	s = splsu();
	mbits = su_wr5[unit] & (SUWR5_RTS|SUWR5_DTR);
	suaddr->sucsr = SUWR0_RESET_STATUS;
	mbits |= suaddr->sucsr & (SURR0_CD|SURR0_CTS);
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
	su_wr5[unit] &= ~(SUWR5_RTS|SUWR5_DTR);
	suaddr->sucsr = 5;
	suaddr->sucsr = (su_wr5[unit] |= mbits&(SUWR5_RTS|SUWR5_DTR));
	(void) splx(s);
	return (mbits);
}
#endif
