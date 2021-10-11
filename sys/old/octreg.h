/*	@(#)octreg.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Octal Serial Card device registers
 */

struct device {
	u_char	octstat;	/* status register */
	u_char	octdata;	/* data buffer */
	u_char	octcmd;		/* command register */
	u_char	octmode;	/* mode register */
};

/*
 * Registers and bits
 */

/* bits in mode register 1 */
#define	OCT_ONESB	0x40	/* one stop bit */
#define	OCT_ONEHSB	0x80	/* one and a half stop bits */
#define	OCT_TWOSB	0xc0	/* two stop bits */
#define	OCT_EPAR	0x20	/* even parity */
#define	OCT_PENABLE	0x10	/* parity enable */
#define	OCT_BITS6	0x04	/* 6 bit characters */
#define	OCT_BITS7	0x08	/* 7 bit characters */
#define	OCT_BITS8	0x0c	/* 8 bit characters */
#define	OCT_SYNC	0x00	/* synchronous clock */
#define	OCT_X1_CLK	0x01	/* asynch 1X clock */
#define	OCT_X16_CLK	0x01	/* asynch 16X clock */
#define	OCT_X64_CLK	0x01	/* asynch 64X clock */

/* bits in mode register 2 */
#define	OCT_MR2_INIT	0x30	/* internal RX and TX clocks */

/* bits in command register */
#define	OCT_RTS		0x20	/* request to send */
#define	OCT_RESET	0x10	/* reset error */
#define	OCT_BREAK	0x08	/* force break */
#define	OCT_RXEN	0x04	/* receiver enable */
#define	OCT_DTR		0x02	/* data terminal ready */
#define	OCT_TXEN	0x01	/* transmitter enable */

/* flags for modem-control */
#define	OCT_ON	(OCT_DTR|OCT_RTS)
#define	OCT_OFF	OCT_RTS

/* bits in status register */
#define	OCT_DSR		0x80	/* data set ready */
#define	OCT_CD		0x40	/* carrier detect */
#define	OCT_FE		0x20	/* framing error */
#define	OCT_DO		0x10	/* data overrun */
#define	OCT_PE		0x08	/* parity error */
#define	OCT_DONE	0x02	/* receiver has character */
#define	OCT_READY	0x01	/* transmitter ready */
 
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
