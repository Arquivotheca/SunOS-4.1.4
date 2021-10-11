/*	@(#)sureg.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun-1 on-board UARTS.
 */
#ifndef LOCORE
struct device {
	u_char	subuf;		/* character buffer */
	u_char	sudum1;
	u_char	sucsr;		/* control/status register */
	u_char	sudum2;
};
#else
#define	SUBUF	0		/* subuf offset */
#define	SUCSR	2		/* sucsr offset */

#define	SUSIZE	4		/* sizeof (struct device) for locore.s */
#endif

#define	BIGBUFFER		/* use bigbuffer input polling */

#define	SUADDR	0x600000	/* fixed address of device */

/*
 * Registers and bits
 */

/* bits in RR0 */
#define	SURR0_DONE		0x01	/* receive buffer full */
#define	SURR0_READY		0x04	/* transmit buffer empty */
#define	SURR0_CD		0x08	/* carrier detect */
#define	SURR0_CTS		0x20	/* CTS input */
#define	SURR0_BREAK		0x80	/* received break detected */

/* bits in RR1 */
#define	SURR1_ALL_SENT		0x01	/* all transmitter buffers empty */
#define	SURR1_PE		0x10	/* parity error */
#define	SURR1_DO		0x20	/* data overrun */
#define	SURR1_FE		0x40	/* framing error */

/* bits in WR0 */
#define	SUWR0_RESET_STATUS	0x10	/* reset status bit changes */
#define	SUWR0_RESET_ALL		0x18	/* reset entire UART */
#define	SUWR0_RESET_TXINT	0x28	/* reset transmitter interrupt */
#define	SUWR0_RESET_ERRORS	0x30	/* reset read character errors */
#define	SUWR0_CLR_INTR		0x38	/* clear interrupt */

/* bits in WR1 */
#define	SUWR1_MIE		0x01	/* modem status change intr enable */
#define	SUWR1_TIE		0x02	/* transmitter interrupt enable */
#define	SUWR1_STAT_AFF_VECT	0x04	/* status affects vector */
#define	SUWR1_RIE		0x10	/* receiver interrupt enable */

#ifdef BIGBUFFER
#define	SUWR1_INIT	(SUWR1_MIE|SUWR1_TIE)
#else
#define	SUWR1_INIT	(SUWR1_MIE|SUWR1_TIE|SUWR1_RIE)
#endif

/* bits in WR2 (channel A only) */
#define	SUWR2_BOTH_INTR		0x00	/* both channels interrupt */
#define	SUWR2_A_DMA_B_INTR	0x01	/* channel A does DMA, B interrupts */
#define	SUWR2_BOTH_DMA		0x02	/* both channels do DMA */
#define	SUWR2_RX_HIGH_PRI	0x04	/* receiver pri > transmitter pri */
#define	SUWR2_8085_INTR_1	0x00	/* 8085 mode 1 interrupt */
#define	SUWR2_8085_INTR_2	0x08	/* 8085 mode 2 interrupt */
#define	SUWR2_8086_INTR		0x10	/* 8086 interrupt mode */

#define	SUWR2_INIT	(SUWR2_BOTH_INTR|SUWR2_8086_INTR|SUWR2_RX_HIGH_PRI)

/* bits in WR3 */
#define	SUWR3_RXENABLE		0x01	/* receiver enable */
#define	SUWR3_AUTO_ENABLE	0x20	/* auto-enable receiver/transmitter */
#define	SUWR3_RX_6		0x80	/* receive 6 bit characters */
#define	SUWR3_RX_7		0x40	/* receive 7 bit characters */
#define	SUWR3_RX_8		0xC0	/* receive 8 bit characters */

#define	SUWR3_INIT	(SUWR3_RXENABLE|SUWR3_AUTO_ENABLE)

/* bits in WR4 */
#define	SUWR4_PARITY_ENABLE	0x01	/* parity enable */
#define	SUWR4_PARITY_EVEN	0x02	/* even parity */
#define	SUWR4_1_STOP		0x04	/* 1 stop bit */
#define	SUWR4_1_5_STOP		0x08	/* 1.5 stop bits */
#define	SUWR4_2_STOP		0x0C	/* 2 stop bits */
#define	SUWR4_X16_CLK		0x40	/* clock is 16x */

/* bits in WR5 */
#define	SUWR5_RTS		0x02	/* RTS output */
#define	SUWR5_TXENABLE		0x08	/* transmitter enable */
#define	SUWR5_BREAK		0x10	/* send break */
#define	SUWR5_TX_6		0x40	/* transmit 6 bit characters */
#define	SUWR5_TX_7		0x20	/* transmit 7 bit characters */
#define	SUWR5_TX_8		0x60	/* transmit 8 bit characters */
#define	SUWR5_DTR		0x80	/* DTR output */

#define	SU_ON	(SUWR5_DTR|SUWR5_RTS)
#define	SU_OFF	0

/* bits in dm lsr, copied from dh.c */
#define	DML_DSR		0000400		/* data set ready, not a real DM bit */
#define	DML_RNG		0000200		/* ring */
#define	DML_CAR		0000100		/* carrier detect */
#define	DML_CTS		0000040		/* clear to send */
#define	DML_SR		0000020		/* secondary receive */
#define	DML_ST		0000010		/* secondary transmit */
#define	DML_RTS		0000004		/* request to send */
#define	DML_DTR		0000002		/* data terminal ready */
#define	DML_LE		0000001		/* line enable */
