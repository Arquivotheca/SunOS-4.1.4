/*	@(#)dsdreg.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/* 
 * The DSD controller has only one io register, write only
 */

struct dsddevice {
	u_char dsd_rxx;  /* Not used! Exists to fill Multibus word */
	u_char dsd_r0;   /* This one is actually used */
};

/*
 * Values written to DSD I/O space register
 */
#define	DSD_RESET	0x02
#define	DSD_START	0x01
#define	DSD_CLEAR	0x100

typedef struct {
	u_short swl_lo;
	u_short swl_hi;
} swlong_t;

swlong_t swlong();

/*
 * The WUB is in Multibus memory at an address
 * which is equal to 16 times the Multibus I/O
 * address
 */
struct dsdwub {		/* WUB: Wake-Up Block */
	u_short dsw_reserved: 13;	/* reserved */
	u_short	dsw_extdiag : 1;	/* extended diagnostics */
	u_short	dsw_linaddr : 1;	/* linear addressing */
	u_short	dsw_emul    : 1;	/* iSBC215 emulation, must be 1 */
	swlong_t dsw_ccbp;		/* pointer to CCB */
};
/* value to write to dsw_ext for extended status and linear addressing */
#define DSD_OPTIONS	7 

#define	DSD_BUSY	0xFF	/* dsc_busy */
#define	DSD_STATUS_POSTED	0xFF	/* status has been posted */

/*
 * The controller knows about a bunch of parameter blocks which are
 * linked together in Multibus memory.  Several of them are used 
 * nearly all the time.  I have put those together in one big block.
 * There is little to be gained from separaing them.
 */

struct dsdccb {        /* CCB: Channel Control Block */
	u_char	dsd_busy;	/* busy flag */
	u_char	dsd_01h;	/* 0x01 (?) */
	swlong_t dsd_cibp;	/* pointer to CIB */
	u_short dsd_reserved0;  /* reserved */
	u_char  dsd_busy2;      /* not used, but must be present */
	u_char  dsd_01h1;       /* yet another magic number */
	swlong_t dsd_cpp;        /* an unused pointer */
	u_short dsd_04h;        /* more magic, supposedly not used */
               		/* CIB: Controller Invocation Block */
	u_char	dsd_sumerr : 1;	/* Summary error */
	u_char	dsd_harderr: 1;	/* Hard error */
	u_char	dsd_unit   : 2;	/* unit number */
	u_char	dsd_dtype  : 1;	/* drive type 0=winnie 1=floppy */
	u_char	dsd_mchange: 1;	/* media change detected */
	u_char	dsd_seeked : 1;	/* seek completed */
	u_char	dsd_done   : 1;	/* operation completed */
	u_char		   : 8;	/* reserved */
	u_char	dsd_stsema;	/* status semaphore */
	u_char	dsd_cmsema;	/* command semaphore (unused) */
	long	dsd_handle;	/* reserved, but CCB points here (!) */
	swlong_t dsd_iopbp;	/* pointer to IOPB */
	long	dsd_reserved1;  /* reserved */
                   /*  Error Status buffer */
	u_char	dsd_error[13];	
	/* the 13 bytes have some significance, but the only one that */
	/* is really interesting is the extended error byte exterr */
	u_char	dsd_exterr;	/* abbreviated error number */
};
#define DSDCCBSZ sizeof(struct dsdccb)

struct dsdiopb {	/*  IOPB: I/O Parameter Block */
	long	dsi_reserved2;  /* reserved */
	swlong_t dsi_xcount;	/* actual transfer count */
	u_short	dsi_device;	/* device type */
	u_char	dsi_cmd;	/* command */
	u_char		: 3;	/* reserved */
	u_char	dsi_vol : 1;	/* 0=fixed, 1=removable */
	u_char		: 2;	/* reserved */
	u_char	dsi_unit : 2;	/* unit # */
	u_char  dsi_diagmod;	/* diagnostic modifier, usually 0 */
	u_char		    : 5;/* reserved */
	u_char	dsi_deldata : 1;/* allow deleted data (?) */
	u_char	dsi_noretry : 1;/* inhibit retries */
	u_char	dsi_nointr  : 1;/* inhibit interrupts */
	u_short	dsi_cylinder;	/* cylinder number */
	u_char	dsi_sector;	/* sector number */
	u_char	dsi_head;	/* head number */
	swlong_t dsi_bufp;	/* pointer to data */
	swlong_t dsi_count;	/* transfer count */
	swlong_t dsi_gap;	/* general addr ptr (unused) */
};
#define	IOPBSIZE	(sizeof(struct dsdiopb))

struct dsdinit {       	/* IOPB extension for initialization */
	u_short	dsx_ncyl;	/* # of cylinders */
	u_char	dsx_rheads;	/* # of removable heads */
	u_char	dsx_fheads;	/* # of fixed heads */
	u_char	dsx_bpslo;	/* bytes/sector (low byte) */
	u_char	dsx_nsect;	/* sectors / track */
	u_char	dsx_acyl;	/* # of alt cyls */
	u_char	dsx_bpshi;	/* bytes/sector (hi byte) */
};

struct	dsdfmt	{	/* format parameter block */
	u_char	dsf_fill1;	/* fill byte 1 */
	u_char	dsf_type;	/* type of track */
	u_char	dsf_fill3;	/* fill byte 3 */
	u_char	dsf_fill2;	/* fill byte 2 */
	u_char	dsf_intrlv;	/* interleave */
	u_char	dsf_fill4;	/* fill byte 4 */
};
struct  dsdbad	{	/* format defective track block */
	u_char	dsb_acyllo;	/* alt cyl low byte */
	u_char	dsb_type;	/* type of track, 0x80 for bad track */
	u_char	dsb_ahead;	/* alt head */
	u_char	dsb_acylhi;	/* alt cyl high byte */
	u_char	dsb_intrlv;	/* interleave */
	u_char	dsb_0x00;	/* magic number 0 */
};

/* track type codes for above type fields */
#define	DSD_NORMAL		0x00
#define	DSD_ALTERNATE	0x40
#define	DSD_DEFECTIVE	0x80
	
/* dsp_device */
#define	DSD_WINCH	0x0000
#define	DSD_FLOPPY	0x0001
#define	DSD_TAPE	0x0002

/* commands */
#define	DSD_INIT	0x00
#define	DSD_STATUS	0x01
#define	DSD_FORMAT	0x02
#define DSD_READ_ID     0x03
#define	DSD_READ	0x04
#define	DSD_VERIFY	0x05
#define	DSD_WRITE	0x06
#define	DSD_WRITE_BUF	0x07
#define	DSD_SEEK	0x08
#define DSD_RESTORE     0x0F  /* really a diagnostic command */
/*
 * For the DSD controller, the restore command just performs a
 * seek to track 0.  There is no explicit command for this,
 * so the diagnostic command with the appropriate modifier is
 * used instead. Subcode 02 on the diagnostic command 
 * means seek to track 0.  The seek command with track 0 as the
 * destination is not exactly what is wanted, since it returns
 * immediately.
 */
#define DSD_REST_MOD    0x02  /* diagnostic modifier */
