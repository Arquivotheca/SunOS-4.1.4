/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#ident "@(#)audio_4231.c 1.1 94/10/31 SMI"

/*
 * AUDIO Chip driver - for CS 4231
 *
 * The basic facts:
 * 	- The digital representation is 8-bit u-law by default.
 *	  The high order bit is a sign bit, the low order seven bits
 *	  encode amplitude, and the entire 8 bits are inverted.
 */

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/file.h>
#include <sys/debug.h>
#include <machine/intreg.h>
#include <sundev/mbvar.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#ifdef	sun4m
#include <machine/param.h>

/* Global variables */
extern int Audio_debug;
#endif

#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/audio_4231.h>


/*
 * Make the C version of the interrupt handler the default except on sun4c
 * machines where the SPARC assembly version can be used
 * XXX later fix.
 */
#if !defined(STANDALONE)
#include <sbusdev/aclone.h>
#endif

#include "audiodebug.h"


/* External routines */
extern addr_t	map_regs();
extern void	report_dev();
extern int	splaudio();


/*
 * Local routines
 */
static int audio_4231_attach();
static int audio_4231_identify();
static int audio_4231_open();
static void audio_4231_close();
static aud_return_t audio_4231_ioctl();
static aud_return_t audio_4231_mproto();
static void audio_4231_start();
static void audio_4231_stop();
static unsigned int audio_4231_setflag();
static aud_return_t audio_4231_setinfo();
static void audio_4231_queuecmd();
static void audio_4231_flushcmd();
static void audio_4231_chipinit();
static void audio_4231_loopback();
static unsigned int audio_4231_outport();
static unsigned int audio_4231_output_muted();
static unsigned int audio_4231_inport();
static unsigned int audio_4231_play_gain();
static unsigned int audio_4231_record_gain();
static unsigned int audio_4231_monitor_gain();
static void audio_4231_playintr();
static void audio_4231_recintr();
static void audio_4231_pollready();
extern unsigned int audio_4231_cintr();

static void audio_4231_initlist();
static void audio_4231_insert();
static void audio_4231_remove();
static void audio_4231_clear();
static void audio_4231_samplecalc();
static unsigned int audio_4231_sampleconv();
static void audio_4231_recordend();
static void audio_4231_initcmdp();
void audio_4231_pollpipe();
void audio_4231_poll_ppipe();
void audio_4231_workaround();


/* Local declarations */
cs_unit_t	*cs_units;		/* device ctrlr array */
static unsigned int cs_units_size;	/* size of allocated devices array */
static unsigned int	devcount = 0;	/* number of devices found */
int audio_4231_bsize = AUD_CS4231_BSIZE;
static int counter = 0;
static unsigned int CS4231_reva;

/* External Declarations */

extern	struct map *dvmamap;		/* pointer to DVMA resource map */

/*
 * XXX - This driver only supports one CS 4231 device
 */
#define	MAXUNITS	(1)

#define	RECORD_DIRECTION 1
#define	PLAY_DIRECTION	0

#define	AUDIO_ENCODING_DVI	(104)	/* DVI ADPCM PCM XXXXX */
#define	AUDIO_DEV_CS4231	(5)	/* should be in audioio.h XXXX */
#define	CS_TIMEOUT	9000000
#define	CS_POLL_TIMEOUT	100000



/*
 * Declare audio ops vector for CS4231 support routines
 */
static struct aud_ops audio_4231_ops = {
	audio_4231_close,
	audio_4231_ioctl,
	audio_4231_start,
	audio_4231_stop,
	audio_4231_setflag,
	audio_4231_setinfo,
	audio_4231_queuecmd,
	audio_4231_flushcmd
};

/*
 * Declare ops vector for auto configuration.
 * It must be named audioamd_ops, since the name is constructed from the
 * device name when config writes ioconf.c.
 */
struct dev_ops audiocs_ops = {
	1,			/* revision */
	audio_4231_identify,		/* identify routine */
	audio_4231_attach,		/* attach routine */
};



/*
 * Streams declarations
 */

static struct module_info audio_4231_modinfo = {
	AUD_CS4231_IDNUM,		/* module ID number */
	AUD_CS4231_NAME,		/* module name */
	AUD_CS4231_MINPACKET,	/* min packet size accepted */
	AUD_CS4231_MAXPACKET,	/* max packet size accepted */
	AUD_CS4231_HIWATER,	/* hi-water mark */
	AUD_CS4231_LOWATER,	/* lo-water mark */
};

/*
 * Queue information structure for read queue
 */
static struct qinit audio_4231_rinit = {
	(int(*)())audio_rput,	/* put procedure */
	(int(*)())audio_rsrv,	/* service procedure */
	audio_4231_open,	/* called on startup */
	(int(*)())audio_close,	/* called on finish */
	NULL,			/* for 3bnet only */
	&audio_4231_modinfo,	/* module information structure */
	NULL,			/* module statistics structure */
};

/*
 * Queue information structure for write queue
 */
static struct qinit audio_4231_winit = {
	(int(*)())audio_wput,	/* put procedure */
	(int(*)())audio_wsrv,	/* service procedure */
	NULL,			/* called on startup */
	NULL,			/* called on finish */
	NULL,			/* for 3bnet only */
	&audio_4231_modinfo,	/* module information structure */
	NULL,			/* module statistics structure */
};

static struct streamtab audio_4231_str_info = {
	&audio_4231_rinit,	/* qinit for read side */
	&audio_4231_winit,	/* qinit for write side */
	NULL,			/* mux qinit for read */
	NULL,			/* mux qinit for write */
				/* list of modules to be pushed */
};


static char *audio4231_modules[] = {
	0,			/* no modules defined yet */
};

struct streamtab audio_4231_info = {
	&audio_4231_rinit,	/* qinit for read side */
	&audio_4231_winit,	/* qinit for write side */
	NULL,			/* mux qinit for read */
	NULL,			/* mux qinit for write */
	audio4231_modules,	/* list of modules to be pushed */
};

static apc_dma_list_t dma_played_list[DMA_LIST_SIZE];
static apc_dma_list_t dma_recorded_list[DMA_LIST_SIZE];

/*
 * Loadable module wrapper
 */
#if defined(CS_LOADABLE) || defined(VDDRV)
#include <sys/conf.h>
#include <sun/autoconf.h>
#include <sun/vddrv.h>

static struct cdevsw audiocs_cdevsw = {
	0, 0, 0, 0, 0, 0, 0, 0, &audio_4231_info, 0
};

static struct vdldrv audiocs_modldrv = {
	VDMAGIC_DRV,		/* Type of module */
	"CS4231 audio driver",	/* Descriptive name */
	&audiocs_ops,		/* Address of dev_ops */
	NULL,			/* bdevsw */
	&audiocs_cdevsw,	/* cdevsw */
	0,			/* Drv_blockmajor, 0 means let system choose */
	0,			/* Drv_charmajor, 0 means let system choose */
};


/*
 * Called by modload/modunload.
 */
int
audio_4231_init(function_code, vdp, vdi, vds)
	uint		function_code;
	struct vddrv	*vdp;
	addr_t		vdi;
	struct vdstat	*vds;
{
	int s, i;
#ifdef lint
	(void) amd_init(0, vdp, vdi, vds);
#endif

	switch (function_code) {
	case VDLOAD:		/* Load module */
		/* Set pointer to loadable driver structure */
		vdp->vdd_vdtab = (struct vdlinkage *)&audiocs_modldrv;

		return (0);

	case VDUNLOAD: {	/* Unload module */
		cs_unit_t	*unitp;
		unsigned int	i;
		int		s;

#if !defined(STANDALONE)
		/* Unregister our aclone mappings */
		aclone_unregister((caddr_t)&audio_4231_info);
#endif

		/* Cannot unload driver if any minor device is open */
		s = splaudio();
		for (i = 0; i < devcount; i++) {
			unitp = &(cs_units[i]);
			if (unitp->output.as.openflag ||
			    unitp->input.as.openflag ||
			    unitp->control.as.openflag) {
				(void) splx(s);
				return (EBUSY);
			}
		}
		(void) splx(s);

		/*
		 * Remove device from interrupt queues.
		 */
		(void) remintr(unitp->dip->devi_intr->int_pri,
		    audio_4231_cintr);
		/*
		 * Deallocate structures allocated in attach
		 * routine
		 */
		for (i = 0; i < devcount; i++) {
			kmem_free(cs_units[i].output.as.cmdlist.memory,
			    (u_int)cs_units[i].output.as.cmdlist.size);
			kmem_free(cs_units[i].input.as.cmdlist.memory,
				(u_int)cs_units[i].input.as.cmdlist.size);
		}
		return (0);
	}

	case VDSTAT:
		return (0);

	default:
		return (EINVAL);
	}
}
#endif /* VDDRV */

/*
 * Called by match_driver() and add_drv_layer() in autoconf.c
 * Returns TRUE if the given string refers to this driver, else FALSE.
 */

static int
audio_4231_identify(name)
	char	*name;
{

	if (strcmp(name, "SUNW,CS4231") == 0) {
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Attach to the device.
 */
static int
audio_4231_attach(dip)
	struct dev_info *dip;
{
	aud_stream_t *as, *output_as, *input_as;
	cs_unit_t *unitp;
	struct aud_cmd *pool;
	unsigned int instance;
	caddr_t regs;
	char name[16];		/* XXX - A Constant! */
	int i;
#if !defined(STANDALONE)
	dev_t		csdev;
#endif
	static int	cs_spl = 0;

	ATRACEINIT();
	ATRACEPRINT("audiocs: debugging version of audio driver\n");
	/*
	 * Each unit has a 'aud_state_t' that contains generic audio
	 * device state information.  Also, each unit has a
	 * 'cs_unit_t' that contains device-specific data.
	 * Allocate storage for them here.
	 */
	if (cs_units == NULL) {
		cs_units_size = MAXUNITS * sizeof (cs_unit_t);
		cs_units = (cs_unit_t *)kmem_zalloc(cs_units_size,
				    KMEM_NOSLEEP);
		if (cs_units == NULL)
			return (-1);
	} else {
		/* We only support one audio device at this time */
		(void) printf("audiocs: Can't support multiple audio devs.\n");
		return (-1);
	}

	unitp = &cs_units[devcount];
	/*
	 * Identify the audio device and assign a unit number.  Get the
	 * address of this unit's audio device state structure.
	 *
	 * XXX - With the sun4c architecture, we know that exactly one
	 * device is there; we should probe anyway, to be safe.
	 */
	dip->devi_unit = 0;

	unitp->distate.devdata = (void *)&cs_units[dip->devi_unit];
	unitp->distate.audio_spl = ipltospl(dip->devi_intr->int_pri);

	cs_spl = MAX(cs_spl, unitp->distate.audio_spl);

	if (devcount > MAXUNITS) {
		(void) printf("audiocs: multiple audio devices?\n");
		return (-1);
	}


	/*
	 * Allocate command list buffers, initialized below
	 */
	unitp->allocated_size = AUD_CS4231_CMDPOOL * sizeof (aud_cmd_t);
	unitp->allocated_memory = (caddr_t *)kmem_zalloc(unitp->allocated_size,
	    KMEM_NOSLEEP);
	if (unitp->allocated_memory == NULL)
		return (-1);

		unitp->dip = dip;
		unitp->distate.devdata = (caddr_t)unitp;
		unitp->distate.monitor_gain = 0;
		unitp->distate.output_muted = FALSE;
		unitp->distate.ops = &audio_4231_ops;
		unitp->chip = (struct aud_4231_chip *)regs;
		unitp->playcount = 0;
		unitp->recordcount = 0;
		unitp->typ_playlength = 0;
		unitp->recordlastent = 0;

		/*
		 * Set up pointers between audio streams
		 */
		unitp->control.as.control_as = &unitp->control.as;
		unitp->control.as.play_as = &unitp->output.as;
		unitp->control.as.record_as = &unitp->input.as;
		unitp->output.as.control_as = &unitp->control.as;
		unitp->output.as.play_as = &unitp->output.as;
		unitp->output.as.record_as = &unitp->input.as;
		unitp->input.as.control_as = &unitp->control.as;
		unitp->input.as.play_as = &unitp->output.as;
		unitp->input.as.record_as = &unitp->input.as;

		as = &unitp->control.as;
		output_as = as->play_as;
		input_as = as->record_as;

		ASSERT(as != NULL);
		ASSERT(output_as != NULL);
		ASSERT(input_as != NULL);

		/*
		 * Initialize the play stream
		 */
		output_as->v = &unitp->distate;
		output_as->type = AUDTYPE_DATA;
		output_as->mode = AUDMODE_AUDIO;
		output_as->info.gain = AUD_CS4231_DEFAULT_PLAYGAIN;
		output_as->info.sample_rate = AUD_CS4231_SAMPLERATE;
		output_as->info.channels = AUD_CS4231_CHANNELS;
		output_as->info.precision = AUD_CS4231_PRECISION;
		output_as->info.encoding = AUDIO_ENCODING_ULAW;
		output_as->info.minordev = CS_MINOR_RW;
		output_as->info.balance = AUDIO_MID_BALANCE;
		output_as->input_size = 0;

		/*
		 * Set the default output port according to capabilities
		 */
		output_as->info.avail_ports = AUDIO_SPEAKER |
					    AUDIO_HEADPHONE | AUDIO_LINE_OUT;
		output_as->info.port = AUDIO_SPEAKER;

		/*
		 * Initialize the record stream (by copying play stream
		 * and correcting some values)
		 */
		input_as->v = &unitp->distate;
		input_as->type = AUDTYPE_DATA;
		input_as->mode = AUDMODE_AUDIO;
		input_as->info = output_as->info;
		input_as->info.gain = AUD_CS4231_DEFAULT_RECGAIN;
		input_as->info.sample_rate = AUD_CS4231_SAMPLERATE;
		input_as->info.channels = AUD_CS4231_CHANNELS;
		input_as->info.precision = AUD_CS4231_PRECISION;
		input_as->info.encoding = AUDIO_ENCODING_ULAW;
		input_as->info.minordev = CS_MINOR_RO;
		input_as->info.avail_ports = AUDIO_MICROPHONE |
		    AUDIO_LINE_IN | AUDIO_INTERNAL_CD_IN;
		input_as->info.port = AUDIO_MICROPHONE;
		input_as->input_size = audio_4231_bsize;

		/*
		 * Control stream info
		 */
		as->v = &unitp->distate;
		as->type = AUDTYPE_CONTROL;
		as->mode = AUDMODE_NONE;
		as->info.minordev = CS_MINOR_CTL;

		/*
		 * Initialize virtual chained DMA command block free
		 * lists.  Reserve a couple of command blocks for record
		 * buffers.  Then allocate the rest for play buffers.
		 */
		pool = (aud_cmd_t *)unitp->allocated_memory;
		unitp->input.as.cmdlist.free = NULL;
		unitp->output.as.cmdlist.free = NULL;
		for (i = 0; i < AUD_CS4231_CMDPOOL; i++) {
			struct aud_cmdlist *list;

			list = (i < AUD_CS4231_RECBUFS) ?
			    &unitp->input.as.cmdlist :
			    &unitp->output.as.cmdlist;
			pool->next = list->free;
			list->free = pool++;
		}

	/*
	 * Map in the registers for this device.  There is only one set
	 * of registers, so we quit if we see some different number.
	 */
	ASSERT(dip != NULL);
	if (dip->devi_nreg == 1) {
		CSR(as) = ((struct aud_4231_chip *)
			    map_regs(dip->devi_reg->reg_addr,
				    dip->devi_reg->reg_size,
				    dip->devi_reg->reg_bustype));
		ASSERT(CSR(as) != NULL);
	} else {
		(void) printf("audiocs%d: warning: bad regs. specification\n",
			dip->devi_unit);
		return (-1);
	}
	/*
	 * Add the interrupt for this device, so that we get interrupts
	 * when they occur.  We only expect one hard interrupt address.
	 */

	if (dip->devi_nintr == 1) {
		addintr(dip->devi_intr->int_pri, audio_4231_cintr,
		    dip->devi_name, dip->devi_unit);
	} else {
		(void) printf("audio%d: warning: bad interrupt specification\n",
			dip->devi_unit);
		return (-1);
	}

	report_dev(dip);

#if !defined(STANDALONE)
	/* Register with audioclone device */
	csdev = aclone_register((caddr_t)&audio_4231_info,
				    0, /* slot 0 is reserved for cs */
				    10 + CS_MINOR_RW, /* audio */
				    10 + CS_MINOR_RO, /* audioro */
				    10 + CS_MINOR_CTL); /* audioctl */

	if (csdev < 0)
		(void) printf("audioamd: could not register with aclone!\n");
#endif
		/*
		 * Initialize the audio chip
		 */
		audio_4231_chipinit(unitp);
		devcount++;

	return (0);
}



static int
audio_4231_open(q, dp, oflag, sflag)
	queue_t		*q;
	dev_t		dp;
	int		oflag;
	int		sflag;
{
	cs_unit_t *unitp;
	aud_stream_t *as = NULL;
	int		minornum = minor(dp);
	int status;

	/*
	 * Necessary for use with audioclone.
	 */
	if (minornum == CS_MINOR_CTL_OLD) {
		printf("audiocs: old control minor num %d has changed to %d\n",
			CS_MINOR_CTL_OLD, CS_MINOR_CTL);
		u.u_error = ENODEV;
		return (OPENFAIL);
	}
	/*
	 * If this is an open from audioclone, normalize both dev
	 * and minornum; see call to aclone_register to find out about "10"
	 */
	if (minornum >= 10) {
		minornum -= 10;	/* XXX DIC */
		dp = makedev(major(dp), minornum);
	}


	/*
	 * Handle clone device opens
	 */
	if (sflag == CLONEOPEN) {
		switch (minornum) {
		case 0:				/* Normal clone open */
		case CS_MINOR_RW:		/* Audio clone open */
			if (oflag & FWRITE) {
				dp = makedev(major(dp), CS_MINOR_RW);
				break;
			}
			/* fall through */
		case CS_MINOR_RO:	/* Audio clone open */
			dp = makedev(major(dp), CS_MINOR_RO);
			break;

		default:
			u.u_error = ENODEV;
			return (OPENFAIL);
		}
	} else if (minornum == 0) {
		/*
		 * Because the system temporarily uses the streamhead
		 * corresponding to major,0 during a clone open, and because
		 * audio_open() can sleep, audio drivers are not allowed
		 * to use major,0 as a valid device number.
		 *
		 * A sleeping clone open and a non-clone use of maj,0
		 * can mess up the reference counts on vnodes/snodes
		 */
		u.u_error = ENODEV;
		return (OPENFAIL);
	}

	minornum = minor(dp);

	/*
	 * Get address of generic audio status structure
	 */
	unitp = &cs_units[0]; /* only one unit */

	/*
	 * Get the correct audio stream
	 */
	if (minornum == unitp->output.as.info.minordev || minornum == 0)
		as = &unitp->output.as;
	else if (minornum == unitp->input.as.info.minordev)
		as = &unitp->input.as;
	else if (minornum == unitp->control.as.info.minordev)
		as = &unitp->control.as;

	if (as == NULL)	{
		u.u_error = ENODEV;
		return (OPENFAIL);
	}

	ATRACE(audio_4231_open, 'OPEN', as);

	/* Set input buffer size now, in case the value was patched */
	as->record_as->input_size = audio_4231_bsize;

	if (ISDATASTREAM(as) && ((oflag & (FREAD|FWRITE)) == FREAD))
		as = as->record_as;

	if (audio_open(as, q, oflag, sflag) == OPENFAIL) {
		ATRACE(audio_4231_open, 'FAIL', as);
		return (OPENFAIL);
	}
	/*
	 * Reset to 8bit u-law mono (default) on open.
	 * This is here for compatibility with the man page
	 * interface for open of /dev/audio as described in audio(7).
	 */
	if (as == as->play_as && as->record_as->readq == NULL) {
		as->play_as->info.sample_rate = AUD_CS4231_SAMPLERATE;
		as->play_as->info.channels = AUD_CS4231_CHANNELS;
		as->play_as->info.precision = AUD_CS4231_PRECISION;
		as->play_as->info.encoding = AUDIO_ENCODING_ULAW;
		UNITP(as)->hw_output_inited = FALSE;
		UNITP(as)->hw_input_inited = FALSE;
		ATRACE(audio_4231_open, 'PLAY', as);
	} else if (as == as->record_as && as->play_as->readq == NULL) {
		as->record_as->info.sample_rate = AUD_CS4231_SAMPLERATE;
		as->record_as->info.channels = AUD_CS4231_CHANNELS;
		as->record_as->info.precision = AUD_CS4231_PRECISION;
		as->record_as->info.encoding = AUDIO_ENCODING_ULAW;
		UNITP(as)->hw_output_inited = FALSE;
		UNITP(as)->hw_input_inited = FALSE;
		ATRACE(audio_4231_open, 'RECD', as);
	}

	if (ISDATASTREAM(as) && (oflag & FREAD))
		audio_process_record(as->record_as);

	ATRACE(audio_4231_open, 'DONE', as);
	return (minornum);
}


/*
 * Device-specific close routine, called from generic module.
 * Must be called with UNITP lock held.
 */
static void
audio_4231_close(as)
	aud_stream_t *as;
{
	cs_unit_t *unitp;
	unitp = UNITP(as);

	/*
	 * Reset status bits.  The device will already have been stopped.
	 */
	if (as == as->play_as) {
		unitp->output.samples = 0;
		unitp->output.error = FALSE;
		unitp->chip->pioregs.iar = IAR_MCE;
		DELAY(100);
		unitp->chip->pioregs.iar = IAR_MCD;
		audio_4231_pollready();
	} else {
		unitp->input.samples = 0;
		unitp->input.error = FALSE;
	}

	ATRACE(audio_4231_close, 'CLOS', as);
}


/*
 * Process ioctls not already handled by the generic audio handler.
 *
 * If AUDIO_CHIP is defined, we support ioctls that allow user processes
 * to muck about with the device registers.
 * Must be called with UNITP lock held.
 */
static aud_return_t
audio_4231_ioctl(as, mp, iocp)
	aud_stream_t		*as;
	mblk_t			*mp;
	struct iocblk		*iocp;
{
	aud_return_t		change;
	int			s;

	change = AUDRETURN_NOCHANGE;		/* detect device state change */

	switch (iocp->ioc_cmd) {

	case AUDIO_GETDEV:
		{
		int	*ip;

		ip = ((int *)mp->b_cont->b_rptr);
		mp->b_cont->b_wptr += sizeof (int);
		*ip = AUDIO_DEV_CS4231;
		iocp->ioc_count = sizeof (int);
		}
		break;
	default:
einval:
		iocp->ioc_error = EINVAL;
		break;
	}
	return (change);
}

/*
 * The next routine is used to start reads or writes.
 * If there is a change of state, enable the chip.
 * If there was already i/o active in the desired direction,
 * or if i/o is paused, don't bother re-initializing the chip.
 * Must be called with UNITP lock held.
 */
static void
audio_4231_start(as)
	aud_stream_t *as;
{
	cs_stream_t *css;
	int pause;
	cs_unit_t *unitp;

	ATRACE(audio_4231_start, '  AS', as);
	unitp = UNITP(as);
	if (as == as->play_as) {
		ATRACE(audio_4231_start, 'OUAS', as);
		css = &UNITP(as)->output;
	} else {
		ATRACE(audio_4231_start, 'INAS', as);
		css = &UNITP(as)->input;
	}

	pause = as->info.pause;

	/*
	 * If we are paused this must mean that we were paused while
	 * playing or recording. In this case we just wasnt to hit
	 * the APC_PPAUSE or APC_CPAUSE bits and resume from where
	 * we left off. If we are starting or re-starting we just
	 * want to start playing as in the normal case.
	 */

	/* If already active, paused, or nothing queued to the device, done */
	if (css->active || pause || (css->cmdptr == NULL)) {
		ATRACE(audio_4231_start, 'RET ', as);
		return;
	}


#ifdef NOTDEF
	if (pause && (as == as->output_as) && css->active == TRUE) {
			CSR(as)->dmaregs.dmacsr &= ~APC_PPAUSE;
	} else if (pause && (as == as->input_as) && css->active == TRUE) {
			CSR(as)->dmaregs.dmacsr &= ~APC_CPAUSE;
	}
#endif

	css->active = TRUE;
	if (!UNITP(as)->hw_output_inited ||
	    !UNITP(as)->hw_input_inited) {
		UNITP(as)->chip->pioregs.iar =
		    IAR_MCE | PLAY_DATA_FR;
		UNITP(as)->chip->pioregs.idr =
		    DEFAULT_DATA_FMAT;
		UNITP(as)->chip->pioregs.iar =
		    IAR_MCE | CAPTURE_DFR;
		UNITP(as)->chip->pioregs.idr =
		    DEFAULT_DATA_FMAT;
		DELAY(100);	/* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		UNITP(as)->hw_output_inited = TRUE;
		UNITP(as)->hw_input_inited = TRUE;
	}

	if (!(pause) && (as == as->play_as)) {
		ATRACE(audio_4231_start, 'IDOU', as);
		audio_4231_initlist((apc_dma_list_t *)
		    dma_played_list, UNITP(as));
		/*
		 * We must clear off the pause first. If you
		 * don't it won't continue from a abort.
		 */
		CSR(as)->dmaregs.dmacsr &= ~APC_PIE;
		CSR(as)->dmaregs.dmacsr &= ~APC_PPAUSE;
		audio_4231_playintr(unitp);
		CSR(as)->dmaregs.dmacsr |= PLAY_SETUP;
		audio_4231_pollready();
		CSR(as)->pioregs.iar = INTERFACE_CR;
		CSR(as)->pioregs.idr |= PEN_ENABLE;
	} else if (!(pause) && (as == as->record_as)) {
		ATRACE(audio_4231_start, 'IDIN', as);
		audio_4231_initlist((apc_dma_list_t *)
		    dma_recorded_list, UNITP(as));
		CSR(as)->dmaregs.dmacsr &= ~APC_CIE;
		CSR(as)->dmaregs.dmacsr &= ~APC_CPAUSE;
		audio_4231_recintr(unitp);
		CSR(as)->dmaregs.dmacsr |= CAP_SETUP;
		CSR(as)->pioregs.iar = INTERFACE_CR;
		CSR(as)->pioregs.idr |= CEN_ENABLE;
	}
}


/*
 * The next routine is used to stop reads or writes.
 * Must be called with UNITP lock held.
 */
static void
audio_4231_stop(as)
	aud_stream_t *as;
{
	int x = 0;
	int s = 0;

	ATRACE(audio_4231_stop, '  AS', as);


	/*
	 * For record.
	 * You HAVE to make sure that all of the DMA is freed up
	 * on the stop and DMA is first stopped in this routine.
	 * Otherwise the flow of code will progress in such a way
	 * that the dma memory will be freed, the device closed
	 * and an intr will come in and scribble on supposedly
	 * clean memory (verify_pattern in streams with 0xcafefeed)
	 * This causes a panic in allocb...on the subsequent alloc of
	 * this block of previously freed memory.
	 * The CPAUSE stops the dma and the recordend frees up the
	 * dma. We poll for the pipe empty bit rather than handling it
	 * in the intr routine because it breaks the code flow since stop and
	 * pause share the same functionality.
	 */

	if (as == as->play_as) {
		UNITP(as)->output.active = FALSE;
		UNITP(as)->chip->dmaregs.dmacsr |= (APC_PPAUSE);
		audio_4231_poll_ppipe(UNITP(as));
		audio_4231_samplecalc(UNITP(as), UNITP(as)->typ_playlength,
				    PLAY_DIRECTION);
		audio_4231_clear((apc_dma_list_t *)dma_played_list,
				    UNITP(as));
		audio_process_play(&UNITP(as)->output.as);
		UNITP(as)->chip->pioregs.iar = IAR_MCE | INTERFACE_CR;
		UNITP(as)->chip->pioregs.idr &= PEN_DISABLE;
		UNITP(as)->chip->pioregs.iar = IAR_MCE;
		DELAY(100);
		UNITP(as)->chip->pioregs.iar = IAR_MCD;
		audio_4231_pollready();
	} else {
		UNITP(as)->input.active = FALSE;
		CSR(as)->dmaregs.dmacsr |= (APC_CPAUSE);
		audio_4231_pollpipe(UNITP(as));
		audio_4231_recordend(UNITP(as), dma_recorded_list);
	}
}


/* Get or set a particular flag value */

static unsigned int
audio_4231_setflag(as, op, val)
	aud_stream_t	*as;
	enum aud_opflag op;
	unsigned int	val;
{
	cs_stream_t	*cp;
	int s = 0;

	ASSERT(as != NULL);
	cp = (as == as->play_as) ? &UNITP(as)->output : &UNITP(as)->input;
	ASSERT(cp != NULL);

	switch (op) {
	case AUD_ERRORRESET: {
		int	s;

		/* read/reset error flag atomically */
		s = splaudio();
		val = cp->error;
		cp->error = FALSE;
		(void) splx(s);
		break;
	    }

	/* GET only */
	case AUD_ACTIVE:
		val = cp->active;
		break;
	}
	return (val);
}


/*
 * Get or set device-specific information in the audio state structure.
 * XXXX NEEDS spls() added.....
 */
static aud_return_t
audio_4231_setinfo(as, mp, iocp)
	aud_stream_t		*as;
	mblk_t			*mp;
	struct iocblk		*iocp;
{
	cs_unit_t *unitp;
	struct aud_4231_chip *chip;
	audio_info_t *ip;
	unsigned int sample_rate, channels, precision, encoding;
	unsigned int o_sample_rate, o_channels, o_precision, o_encoding;
	unsigned int gain;
	unsigned int capcount, playcount;
	unsigned char balance;

	unsigned int	tmp_bits;
	int error = 0;
	int s = 0;

	unitp = UNITP(as);

	/*
	 * Set device-specific info into device-independent structure
	 */
	capcount =
	    audio_4231_sampleconv(&unitp->input, unitp->chip->dmaregs.dmacc);
	playcount =
	    audio_4231_sampleconv(&unitp->output, unitp->chip->dmaregs.dmapc);

	if (playcount > unitp->output.samples) {
		if (unitp->output.samples > 0) {
			as->play_as->info.samples = unitp->output.samples;
		} else {
			as->play_as->info.samples = 0;
		}
	} else {
		as->play_as->info.samples =
				    unitp->output.samples - playcount;
	}
	if (capcount > unitp->input.samples) {
		if (unitp->input.samples > 0) {
			as->record_as->info.samples = unitp->input.samples;
		} else {
			as->record_as->info.samples = 0;
		}
	} else {
		as->record_as->info.samples =
			    unitp->input.samples - capcount;
	}


	as->play_as->info.active = unitp->output.active;
	as->record_as->info.active = unitp->input.active;

	/*
	 * If getinfo, 'mp' is NULL...we're done
	 */
	if (mp == NULL)
		return (AUDRETURN_NOCHANGE);

	ip = (audio_info_t *)(void *)mp->b_cont->b_rptr;

	chip = unitp->chip;

	s = splaudio();

	/*
	 * If any new value matches the current value, there
	 * should be no need to set it again here.
	 * However, it's work to detect this so don't bother.
	 */
	if (Modify(ip->play.gain) || Modifyc(ip->play.balance)) {
		if (Modify(ip->play.gain))
			gain = ip->play.gain;
		else
			gain = as->play_as->info.gain;

		if (Modifyc(ip->play.balance))
			balance = ip->play.balance;
		else
			balance = as->play_as->info.balance;

		as->play_as->info.gain = audio_4231_play_gain(chip,
		    gain, balance);
		as->play_as->info.balance = balance;
	}

	if (Modify(ip->record.gain) || Modifyc(ip->record.balance)) {
		if (Modify(ip->record.gain))
			gain = ip->record.gain;
		else
			gain = as->record_as->info.gain;

		if (Modifyc(ip->record.balance))
			balance = ip->record.balance;
		else
			balance = as->record_as->info.balance;

		as->record_as->info.gain = audio_4231_record_gain(chip,
		    gain, balance);
		as->record_as->info.balance = balance;
	}

/*
 * XXXXXX FIX ME
	if (Modify(ip->record.input_size)) {
		if ((as != as->record_as) &&
		    ((as == as->play_as) && !(as->openflag & FREAD)) ||
		    (ip->record.input_size <= 0) ||
		    (ip->record.input_size > AUD_CS4231_MAX_BSIZE)) {
			error = EINVAL;
		} else {
			as->record_as->info.buffer_size =
				    ip->record.buffer_size;
		}
	}
*/

	if (Modify(ip->monitor_gain)) {
		as->v->monitor_gain = audio_4231_monitor_gain(chip,
		    ip->monitor_gain);
	}

	if (Modifyc(ip->output_muted)) {
		as->v->output_muted = audio_4231_output_muted(unitp,
		    ip->output_muted);
	}


	if (Modify(ip->play.port)) {
		as->play_as->info.port = audio_4231_outport(chip,
		    ip->play.port);
	}

	if (Modify(ip->record.port)) {
		as->record_as->info.port = audio_4231_inport(chip,
					    ip->record.port);
	}

	/*
	 * Save the old settings on any error of the folowing 4
	 * reset all back to the old and exit.
	 * DBRI compatability.
	 */

	o_sample_rate = as->info.sample_rate;
	o_encoding = as->info.encoding;
	o_precision = as->info.precision;
	o_channels = as->info.channels;


	/*
	 * Set the sample counters atomically, returning the old values.
	 */
	if (Modify(ip->play.samples) || Modify(ip->record.samples)) {
		if (as->play_as->info.open) {
			as->play_as->info.samples = unitp->output.samples;
			if (Modify(ip->play.samples))
				unitp->output.samples = ip->play.samples;
		}

		if (as->record_as->info.open) {
			as->record_as->info.samples = unitp->input.samples;
			if (Modify(ip->record.samples))
				unitp->input.samples = ip->record.samples;
		}
	}

	if (Modify(ip->play.sample_rate))
		sample_rate = ip->play.sample_rate;
	else if (Modify(ip->record.sample_rate))
		sample_rate = ip->record.sample_rate;
	else
		sample_rate = as->info.sample_rate;

	if (Modify(ip->play.channels))
		channels = ip->play.channels;
	else if (Modify(ip->record.channels))
		channels = ip->record.channels;
	else
		channels = as->info.channels;


	if (Modify(ip->play.precision))
		precision = ip->play.precision;
	else if (Modify(ip->record.precision))
		precision = ip->record.precision;
	else
		precision = as->info.precision;

	if (Modify(ip->play.encoding))
		encoding = ip->play.encoding;
	else if (Modify(ip->record.encoding))
		encoding = ip->record.encoding;
	else
		encoding = as->info.encoding;

	/*
	 * If setting to the current format, do not do anything.  Otherwise
	 * check and see if this is a valid format.
	 */
	if ((sample_rate == as->info.sample_rate) &&
	    (channels == as->info.channels) &&
	    (precision == as->info.precision) &&
	    (encoding == as->info.encoding) &&
	    (unitp->hw_output_inited == TRUE ||
		unitp->hw_input_inited == TRUE)) {
		goto done;
	}

	/*
	 * If we get here we must want to change the data format
	 * Changing the data format is done for both the play and
	 * record side for now.
	 */

	switch (sample_rate) {
	case 8000:		/* ULAW and ALAW */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_8000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_8000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR8000;
		break;
	case 9600:		/* SPEECHIO */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_9600);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_9600);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR9600;
		break;
	case 11025:
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_11025);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_11025);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR11025;
		break;
	case 16000:		/* G_722 */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_16000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_16000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR16000;
		break;
	case 18900:		/* CDROM_XA_C */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_18900);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_18900);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR18900;
		break;
	case 22050:
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_22050);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_22050);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR22050;
		break;
	case 32000:		/* DAT_32 */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_32000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_32000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR32000;
		break;
	case 37800:		/* CDROM_XA_AB */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_37800);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_37800);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR37800;
		break;
	case 44100:		/* CD_DA */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_44100);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_44100);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR44100;
		break;
	case 48000:		/* DAT_48 */
		chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_48000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
		tmp_bits = chip->pioregs.idr;
		chip->pioregs.idr = CHANGE_DFR(tmp_bits, CS4231_DFR_48000);
		DELAY(100); /* chip bug workaround */
		audio_4231_pollready();
		DELAY(1000);	/* chip bug */
		sample_rate = AUD_CS4231_SAMPR48000;
		break;
	default:
		error = EINVAL;
		break;
	} /* switch on sampling rate */

	if (Modify(ip->play.encoding) ||
		    (Modify(ip->record.encoding)) && error != EINVAL) {
		/*
		 * If a process wants to modify the play or record format,
		 * another process can not have it open for recording.
		 */
		if (as->record_as->info.open &&
		    as->play_as->info.open &&
		    (as->record_as->readq != as->play_as->readq)) {
			error = EBUSY;
			goto playdone;
		}

		/*
		 * Control stream cannot modify the play format
		 */
		if ((as != as->play_as) && (as != as->record_as)) {
			error = EINVAL;
			goto playdone;
		}

		switch (encoding) {
		case AUDIO_ENCODING_ULAW:
			if (Modify(channels) && (channels != 1))
				error = EINVAL;
			chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_ULAW);
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CS4231_MONO_ON(tmp_bits);
			chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_ULAW);
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CS4231_MONO_ON(tmp_bits);
			channels = 0x01;
			encoding = AUDIO_ENCODING_ULAW;
			break;
		case AUDIO_ENCODING_ALAW:
			if (Modify(channels) && (channels != 1))
				error = EINVAL;
			chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_ALAW);
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CS4231_MONO_ON(tmp_bits);
			chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_ALAW);
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CS4231_MONO_ON(tmp_bits);

			channels = 0x01;
			encoding = AUDIO_ENCODING_ALAW;
			break;
		case AUDIO_ENCODING_LINEAR:
			if (Modify(channels) && (channels != 2) &&
				    (channels != 1))
				error = EINVAL;
			chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_LINEARBE);
			tmp_bits = chip->pioregs.idr;
			if (channels == 2) {
				chip->pioregs.idr =
				    CS4231_STEREO_ON(tmp_bits);
			} else {
				chip->pioregs.idr =
				    CS4231_MONO_ON(tmp_bits);
			}
			chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_LINEARBE);
			tmp_bits = chip->pioregs.idr;
			if (channels == 2) {
				chip->pioregs.idr =
				    CS4231_STEREO_ON(tmp_bits);
			} else {
				chip->pioregs.idr =
				    CS4231_MONO_ON(tmp_bits);
			}
			encoding = AUDIO_ENCODING_LINEAR;
			break;
		case AUDIO_ENCODING_DVI:
			/* XXXX REV 2.0 FUTURE SUPPORT HOOK */
			if (Modify(channels) && (channels != 2) &&
				    (channels != 1))
				error = EINVAL;
			chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_ADPCM);
			tmp_bits = chip->pioregs.idr;
			if (channels == 2) {
				chip->pioregs.idr =
				    CS4231_STEREO_ON(tmp_bits);
			} else {
				chip->pioregs.idr =
				    CS4231_MONO_ON(tmp_bits);
			}
			chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
			tmp_bits = chip->pioregs.idr;
			chip->pioregs.idr =
			    CHANGE_ENCODING(tmp_bits, CS4231_DFR_ADPCM);
			tmp_bits = chip->pioregs.idr;
			if (channels == 2) {
				chip->pioregs.idr =
				    CS4231_STEREO_ON(tmp_bits);
			} else {
				chip->pioregs.idr =
				    CS4231_MONO_ON(tmp_bits);
			}
			encoding = AUDIO_ENCODING_DVI;
			break;
		default:
			error = EINVAL;
		} /* switch on audio encoding */
	playdone:;

	}

	unitp->hw_output_inited = TRUE;
	unitp->hw_input_inited = TRUE;


done:
	/*
	 * Update the "real" info structure (and others) accordingly
	 */

	if (error == EINVAL) {
		sample_rate = o_sample_rate;
		encoding = o_encoding;
		precision = o_precision;
		channels = o_channels;
	}

	/*
	 * one last chance to make sure that we have something
	 * working. Set to defaults if one of the 4 horsemen
	 * is zero.
	 */

	if (sample_rate == 0 || encoding == 0 || precision == 0 ||
			    channels == 0) {
		sample_rate = AUD_CS4231_SAMPLERATE;
		channels = AUD_CS4231_CHANNELS;
		precision = AUD_CS4231_PRECISION;
		encoding = AUDIO_ENCODING_ULAW;
		unitp->hw_output_inited = FALSE;
		unitp->hw_input_inited = FALSE;
	}

	ip->play.sample_rate = ip->record.sample_rate = sample_rate;
	ip->play.channels = ip->record.channels = channels;
	ip->play.precision = ip->record.precision = precision;
	ip->play.encoding = ip->record.encoding = encoding;

	as->record_as->info.sample_rate = sample_rate;
	as->record_as->info.channels = channels;
	as->record_as->info.precision = precision;
	as->record_as->info.encoding = encoding;

	as->play_as->info.sample_rate = sample_rate;
	as->play_as->info.channels = channels;
	as->play_as->info.precision = precision;
	as->play_as->info.encoding = encoding;

	as->control_as->info.sample_rate = sample_rate;
	as->control_as->info.channels = channels;
	as->control_as->info.precision = precision;
	as->control_as->info.encoding = encoding;

	chip->pioregs.iar = IAR_MCD;
	DELAY(100); /* chip bug workaround */
	audio_4231_pollready();
	DELAY(1000);	/* chip bug */

	iocp->ioc_error = error;

	(void) splx(s);
	return (AUDRETURN_CHANGE);
} /* end of setinfo */


static void
audio_4231_queuecmd(as, cmdp)
	aud_stream_t *as;
	aud_cmd_t *cmdp;
{
	cs_stream_t *css;

	ATRACE(audio_4231_queuecmd, '  AS', as);
	ATRACE(audio_4231_queuecmd, ' CMD', cmdp);

	if (as == as->play_as)
		css = &UNITP(as)->output;
	else
		css = &UNITP(as)->input;

	/*
	 * This device doesn't do packets, so make each buffer its own
	 * packet.
	 */
	cmdp->lastfragment = cmdp;

	/*
	 * If the virtual controller command list is NULL, then the interrupt
	 * routine is probably disabled.  In the event that it is not,
	 * setting a new command list below is safe at low priority.
	 */
	if (css->cmdptr == NULL && !css->active) {
		ATRACE(audio_4231_queuecmd, 'NULL', as->cmdlist.head);
		css->cmdptr = as->cmdlist.head;
		if (!css->active)
			audio_4231_start(as); /* go, if not paused */
	}
}

/*
 * Flush the device's notion of queued commands.
 * Must be called with UNITP lock held.
 */
static void
audio_4231_flushcmd(as)
	aud_stream_t *as;
{
	ATRACE(audio_4231_flushcmd, 'SA  ', as);
	DELAY(250);
	if (as == as->play_as)
		UNITP(as)->output.cmdptr = NULL;
	else
		UNITP(as)->input.cmdptr = NULL;
}

/*
 * Initialize the audio chip to a known good state.
 */
static void
audio_4231_chipinit(unitp)
	cs_unit_t *unitp;
{
	struct aud_4231_chip *chip;

	chip = unitp->chip;
	ASSERT(chip != NULL);

	/*
	 * The APC has a bug where the reset is not done
	 * until you do the next pio to the APC. This
	 * next write to the CSR causes the posted reset to
	 * happen.
	 */
	unitp->chip->dmaregs.dmacsr = APC_RESET;
	unitp->chip->dmaregs.dmacsr = 0x00;
	unitp->chip->dmaregs.dmacsr |= APC_CODEC_PDN;
	DELAY(20);
	unitp->chip->dmaregs.dmacsr &= ~(APC_CODEC_PDN);
	chip->pioregs.iar |= IAR_MCE;
	DELAY(100); /* chip bug workaround */
	audio_4231_pollready();
	DELAY(1000);	/* chip bug */
	chip->pioregs.iar = IAR_MCE | MISC_IR; /* Reg. 12 */
	chip->pioregs.idr = MISC_IR_MODE2;
	chip->pioregs.iar = IAR_MCE | PLAY_DATA_FR; /* Reg. 8 */
	chip->pioregs.idr = DEFAULT_DATA_FMAT;
	DELAY(100); /* chip bug workaround */
	audio_4231_pollready();
	DELAY(1000);	/* chip bug */
	chip->pioregs.iar = IAR_MCE | CAPTURE_DFR;
	chip->pioregs.idr = DEFAULT_DATA_FMAT;
	DELAY(100); /* chip bug workaround */
	audio_4231_pollready();
	DELAY(1000);	/* chip bug */

	chip->pioregs.iar = VERSION_R;

	if (chip->pioregs.idr & CS4231A)
		CS4231_reva = TRUE;
	else
		CS4231_reva = FALSE;

	/* Turn on the Output Level Bit to be 2.8 Vpp */
	chip->pioregs.iar = IAR_MCE | ALT_FEA_EN1R;
	chip->pioregs.idr = (char)OLB_ENABLE; /* cast for lint */

	/* Turn on the hi pass filter */
	chip->pioregs.iar = IAR_MCE | ALT_FEA_EN2R;
	if (CS4231_reva)
		chip->pioregs.idr = (HPF_ON | XTALE_ON);
	else
		chip->pioregs.idr = (HPF_ON);

	chip->pioregs.iar = IAR_MCE | MONO_IOCR;
	chip->pioregs.idr = 0x00;


	/* Init the play and Record gain registers */

	unitp->output.as.info.gain = audio_4231_play_gain(chip,
	    AUD_CS4231_DEFAULT_PLAYGAIN, AUDIO_MID_BALANCE);
	unitp->input.as.info.gain = audio_4231_record_gain(chip,
	    AUD_CS4231_DEFAULT_RECGAIN, AUDIO_MID_BALANCE);
	unitp->input.as.info.port = audio_4231_inport(chip,
		    AUDIO_MICROPHONE);
	unitp->output.as.info.port = audio_4231_outport(chip,
		    AUDIO_SPEAKER);
	unitp->distate.monitor_gain = audio_4231_monitor_gain(chip,
		    LOOPB_OFF);

	chip->pioregs.iar = (u_char)IAR_MCD;
	audio_4231_pollready();
	chip->pioregs.iar = IAR_MCE | INTERFACE_CR;
	chip->pioregs.idr &= ACAL_DISABLE;
	chip->pioregs.iar = (u_char)IAR_MCD;
	audio_4231_pollready();

	unitp->distate.output_muted = audio_4231_output_muted(unitp, 0x0);
}


/*
 * Set or clear internal loopback for diagnostic purposes.
 * Must be called with UNITP lock held.
 */
static void
audio_4231_loopback(unitp, loop)
	cs_unit_t *unitp;
	unsigned int loop;
{

	unitp->chip->pioregs.iar = LOOPB_CR;
	unitp->chip->pioregs.idr = (loop ? LOOPB_ON: 0);
}

static unsigned int
audio_4231_output_muted(unitp, val)
	cs_unit_t *unitp;
	unsigned int val;
{
	/*
	 * Just do the mute on Index 6 & 7 R&L output.
	 */
	if (val) {
		unitp->chip->pioregs.iar = L_OUTPUT_CR;
		unitp->chip->pioregs.idr |= OUTCR_MUTE;
		unitp->chip->pioregs.iar = R_OUTPUT_CR;
		unitp->chip->pioregs.idr |= OUTCR_MUTE;
		unitp->distate.output_muted = TRUE;
	} else {

		unitp->chip->pioregs.iar = L_OUTPUT_CR;
		unitp->chip->pioregs.idr &= OUTCR_UNMUTE;
		unitp->chip->pioregs.iar = R_OUTPUT_CR;
		unitp->chip->pioregs.idr &= OUTCR_UNMUTE;
		unitp->distate.output_muted = FALSE;
	}

	return (unitp->distate.output_muted);

}

static unsigned int
audio_4231_inport(chip, val)
	struct aud_4231_chip *chip;
	unsigned int val;

{

	unsigned int ret_val;


	/*
	 * In the 4231 we can have line, MIC  or CDROM_IN enabled at
	 * one time. We cannot have a mix of all. MIC is mic on
	 * Index 0, LINE and CDROM (AUX1) are on Index 0 and 1 through
	 * the 4231 mux.
	 */
	ret_val = 0;

	if (val & AUDIO_INTERNAL_CD_IN) {
		chip->pioregs.iar = L_INPUT_CR;
		chip->pioregs.idr =
			CDROM_ENABLE(chip->pioregs.idr);
		chip->pioregs.iar = R_INPUT_CR;
		chip->pioregs.idr =
			CDROM_ENABLE(chip->pioregs.idr);
		ret_val = AUDIO_INTERNAL_CD_IN;
	}
	if ((val & AUDIO_LINE_IN)) {
		chip->pioregs.iar = L_INPUT_CR;
		chip->pioregs.idr =
		    LINE_ENABLE(chip->pioregs.idr);
		chip->pioregs.iar = R_INPUT_CR;
		chip->pioregs.idr =
		    LINE_ENABLE(chip->pioregs.idr);
		ret_val = AUDIO_LINE_IN;
	} else if (val & AUDIO_MICROPHONE) {
		chip->pioregs.iar = L_INPUT_CR;
		chip->pioregs.idr =
		    MIC_ENABLE(chip->pioregs.idr);
		chip->pioregs.iar = R_INPUT_CR;
		chip->pioregs.idr =
		    MIC_ENABLE(chip->pioregs.idr);
		ret_val = AUDIO_MICROPHONE;
	}

	return (ret_val);
}
/*
 *
 */
static unsigned int
audio_4231_outport(chip, val)
	struct aud_4231_chip *chip;
	unsigned int val;
{
	unsigned int ret_val;

	/*
	 *  Disable everything then selectively enable it.
	 */

	ret_val = 0;
	chip->pioregs.iar = MONO_IOCR;
	chip->pioregs.idr |= MONOIOCR_SPKRMUTE;
	chip->pioregs.iar = PIN_CR;
	chip->pioregs.idr |= PINCR_LINE_MUTE;
	chip->pioregs.idr |= PINCR_HDPH_MUTE;

	if (val & AUDIO_SPEAKER) {
		chip->pioregs.iar = MONO_IOCR;
		chip->pioregs.idr &= ~MONOIOCR_SPKRMUTE;
		ret_val |= AUDIO_SPEAKER;
	}
	if (val & AUDIO_HEADPHONE) {
		chip->pioregs.iar = PIN_CR;
		chip->pioregs.idr &= ~PINCR_HDPH_MUTE;
		ret_val |= AUDIO_HEADPHONE;
	}

	if (val & AUDIO_LINE_OUT) {
		chip->pioregs.iar = PIN_CR;
		chip->pioregs.idr &= ~PINCR_LINE_MUTE;
		ret_val |= AUDIO_LINE_OUT;
	}

	return (ret_val);
}

static unsigned int
audio_4231_monitor_gain(chip, val)
	struct aud_4231_chip *chip;
	unsigned int val;
{
	int aten;

	aten = AUD_CS4231_MON_MAX_ATEN -
	    (val * (AUD_CS4231_MON_MAX_ATEN + 1) /
			    (AUDIO_MAX_GAIN + 1));

	/*
	 * Normal monitor registers are the index 13. Line monitor for
	 * now can be registers 18 and 19. Which are actually MIX to
	 * OUT directly We don't use these for now 8/3/93.
	 */

	chip->pioregs.iar = LOOPB_CR;
	if (aten >= AUD_CS4231_MON_MAX_ATEN) {
		chip->pioregs.idr = LOOPB_OFF;
	} else {

		/*
		 * Loop Back enable
		 * is in bit 0, 1 is reserved, thus the shift 2.
		 * all other aten and gains are in the low order
		 * bits, this one has to be differnt and be in the
		 * high order bits sigh...
		 */
		chip->pioregs.idr = ((aten << 2) | LOOPB_ON);
	}

	/*
	 * We end up returning a value slightly different than the one
	 * passed in - *most* applications expect this.
	 */
	return ((val == AUDIO_MAX_GAIN) ? AUDIO_MAX_GAIN :
	    ((AUD_CS4231_MAX_DEV_ATEN - aten) * (AUDIO_MAX_GAIN + 1) /
	    (AUD_CS4231_MAX_DEV_ATEN + 1)));
}

/*
 * Convert play gain to chip values and load them.
 * Return the closest appropriate gain value.
 * Must be called with UNITP lock held.
 */
static unsigned int
audio_4231_play_gain(chip, val, balance)
	struct aud_4231_chip *chip;
	unsigned int val;
	unsigned char balance;
{

	unsigned int tmp_val, r, l;
	unsigned int la, ra;
	unsigned char old_gain;


	r = l = val;
	if (balance < AUDIO_MID_BALANCE) {
		r = MAX(0, (int)(val -
		    ((AUDIO_MID_BALANCE - balance) << AUDIO_BALANCE_SHIFT)));
	} else if (balance > AUDIO_MID_BALANCE) {
		l = MAX(0, (int)(val -
		    ((balance - AUDIO_MID_BALANCE) << AUDIO_BALANCE_SHIFT)));
	}

	if (l == 0) {
		la = AUD_CS4231_MAX_DEV_ATEN;
	} else {
		la = AUD_CS4231_MAX_ATEN -
		    (l * (AUD_CS4231_MAX_ATEN + 1) / (AUDIO_MAX_GAIN + 1));
	}
	if (r == 0) {
		ra = AUD_CS4231_MAX_DEV_ATEN;
	} else {
		ra = AUD_CS4231_MAX_ATEN -
		    (r * (AUD_CS4231_MAX_ATEN + 1) / (AUDIO_MAX_GAIN + 1));
	}

	/* Load output gain registers */


	chip->pioregs.iar = L_OUTPUT_CR;
	old_gain = chip->pioregs.idr;
	chip->pioregs.idr = GAIN_SET(old_gain, la);
	chip->pioregs.iar = R_OUTPUT_CR;
	old_gain = chip->pioregs.idr;
	chip->pioregs.idr = GAIN_SET(old_gain, ra);

	if ((val == 0) || (val == AUDIO_MAX_GAIN)) {
		tmp_val = val;
	} else {
		if (l == val) {
			tmp_val = ((AUD_CS4231_MAX_ATEN - la) *
			    (AUDIO_MAX_GAIN + 1) /
				    (AUD_CS4231_MAX_ATEN + 1));
		} else if (r == val) {
			tmp_val = ((AUD_CS4231_MAX_ATEN - ra) *
			    (AUDIO_MAX_GAIN + 1) /
				    (AUD_CS4231_MAX_ATEN + 1));
		}
	}
	return (tmp_val);
}


/*
 * Convert record gain to chip values and load them.
 * Return the closest appropriate gain value.
 * Must be called with UNITP lock held.
 */
static unsigned int
audio_4231_record_gain(chip, val, balance)
	struct aud_4231_chip *chip;
	unsigned int val;
	unsigned char balance;
{

	unsigned int tmp_val, r, l;
	unsigned int lg, rg;

	r = l = val;
	tmp_val = 0;

	if (balance < AUDIO_MID_BALANCE) {
		r = MAX(0, (int)(val -
		    ((AUDIO_MID_BALANCE - balance) << AUDIO_BALANCE_SHIFT)));
	} else if (balance > AUDIO_MID_BALANCE) {
		l = MAX(0, (int)(val -
		    ((balance - AUDIO_MID_BALANCE) << AUDIO_BALANCE_SHIFT)));
	}
	lg = l * (AUD_CS4231_MAX_GAIN + 1) / (AUDIO_MAX_GAIN + 1);
	rg = r * (AUD_CS4231_MAX_GAIN + 1) / (AUDIO_MAX_GAIN + 1);

	/* Load input gain registers */
	chip->pioregs.iar = L_INPUT_CR;
	tmp_val = chip->pioregs.idr;
	chip->pioregs.idr = RECGAIN_SET(tmp_val, lg);

	chip->pioregs.iar = R_INPUT_CR;
	tmp_val = chip->pioregs.idr;
	chip->pioregs.idr = RECGAIN_SET(tmp_val, rg);

	/*
	 * We end up returning a value slightly different than the one
	 * passed in - *most* applications expect this.
	 */
	if (l == val) {
		if (l == 0) {
			tmp_val = 0;
		} else {
			tmp_val = ((lg + 1) * AUDIO_MAX_GAIN) /
			    (AUD_CS4231_MAX_GAIN + 1);
		}
	} else if (r == val) {
		if (r == 0) {
			tmp_val = 0;
		} else {

			tmp_val = ((rg + 1) * AUDIO_MAX_GAIN) /
			    (AUD_CS4231_MAX_GAIN + 1);
		}
	}

	return (tmp_val);


}


/*
 * Common interrupt routine. vectors to play of record.
 */
unsigned int
audio_4231_cintr()
{
	cs_unit_t *unitp;
	struct aud_4231_chip *chip;
	long dmacsr, rc;

	/* Acquire spin lock */

	/*
	 * Figure out which chip interrupted.
	 * Since we only have one chip, we punt and assume device zero.
	 */
	unitp = &cs_units[0];

	rc = 0;

	chip = unitp->chip;
	dmacsr = chip->dmaregs.dmacsr;	/* read and store the APC csr */

	chip->dmaregs.dmacsr = dmacsr;	/* clear all possible ints */

	ATRACE(audio_4231_cintr, 'RSCD', dmacsr);

	if ((dmacsr & APC_CMI) && (unitp->input.active != TRUE)) {
		ATRACE(audio_4231_cintr, 'PCON', dmacsr);
		unitp->input.active = FALSE;
		rc = 1;
	}

	if ((dmacsr & APC_PMI) && (unitp->output.active != TRUE)) {
		ATRACE(audio_4231_cintr, 'LPON', dmacsr);
		/*
		 * We want to pause here rather than in the playintr routine
		 */
		unitp->chip->dmaregs.dmacsr |= (APC_PPAUSE);
		unitp->chip->pioregs.iar = INTERFACE_CR;
		unitp->chip->pioregs.idr &= PEN_DISABLE;

		audio_4231_workaround(unitp);

		audio_4231_samplecalc(unitp, unitp->typ_playlength,
				    PLAY_DIRECTION);
		audio_4231_clear((apc_dma_list_t *)dma_played_list,
				    unitp);
		unitp->output.active = FALSE;
		unitp->output.cmdptr = NULL;
		rc = 1;
	}

	if (dmacsr & APC_PI) {
		if (dmacsr & APC_PD) {
			unitp->output.samples +=
			    audio_4231_sampleconv(&unitp->output,
				    unitp->typ_playlength);
			audio_4231_playintr(unitp);
		}

		rc = 1;

	}

	if (dmacsr & APC_CI) {
		if (dmacsr & APC_CD) {
			unitp->input.samples +=
			    audio_4231_sampleconv(&unitp->input,
				    unitp->typ_reclength);
			audio_4231_recintr(unitp);
		}

		rc = 1;
	}

	/*
	 * If we get a bus error make a valiant attempt to continue.
	 * rather than just hang the audio process.
	 */
	if (dmacsr & APC_EI) {
		if ((dmacsr & APC_PMI)) {
			ATRACE(audio_4231_cintr, 'ERR ', dmacsr);
			unitp->chip->dmaregs.dmacsr |= (APC_PPAUSE);
			unitp->chip->pioregs.iar = INTERFACE_CR;
			unitp->chip->pioregs.idr &= PEN_DISABLE;

			audio_4231_workaround(unitp);

			audio_4231_samplecalc(unitp, unitp->typ_playlength,
					    PLAY_DIRECTION);
			audio_4231_clear((apc_dma_list_t *)dma_played_list,
					    unitp);
			unitp->output.active = FALSE;
			unitp->output.cmdptr = NULL;
		}

		if ((dmacsr & APC_CMI)) {
			unitp->input.active = FALSE;
			unitp->input.cmdptr = NULL;
		}

		rc = 1;
	}
	return (rc);
}

static void
audio_4231_playintr(unitp)
	cs_unit_t *unitp;
{
	aud_cmd_t *cmdp;
	cs_stream_t *ds;
	caddr_t buf_dma_handle;
	unsigned int length;
	int lastcount = 0;
	int need_processing = 0;

	ds = &unitp->output;		/* Get cs stream pointer */

	lastcount = (unitp->playcount % DMA_LIST_SIZE);
	if (lastcount > 0)
		lastcount--;
	else
		lastcount = DMA_LIST_SIZE - 1;
	ATRACE(audio_4231_playintr, 'tsal', lastcount);
	ATRACE(audio_4231_playintr, 'yalp', unitp->playcount);

	cmdp = unitp->output.cmdptr;

	if (cmdp == NULL) {
		ATRACE(audio_4231_playintr, 'NuLL', cmdp);
		unitp->output.active = FALSE;
		goto done;
	}
	ATRACE(audio_4231_playintr, 'Pcmd', cmdp);

	if (cmdp == dma_played_list[lastcount].cmdp) {
		ATRACE(audio_4231_playintr, 'emas', cmdp);
		if (cmdp->next != NULL) {
			cmdp = cmdp->next;
			unitp->output.cmdptr = cmdp;
			unitp->output.active = TRUE;
			ATRACE(audio_4231_playintr, 'dmcn', cmdp);
		} else {
			ATRACE(audio_4231_playintr, 'Null', cmdp);
			unitp->output.active = FALSE;
			unitp->output.cmdptr = NULL;
			goto done;
		}

	}

	if (unitp->output.active) {
		unitp->output.error = FALSE;
		ATRACE(audio_4231_playintr, ' DMC', unitp->output.cmdptr);

		/*
		 * Ignore null and non-data buffers
		 */
		while (cmdp != NULL && (cmdp->skip || cmdp->done)) {
			cmdp->done = TRUE;
			ATRACE(audio_4231_playintr, 'SCDM', cmdp);
			need_processing++;
			cmdp = cmdp->next;
			unitp->output.cmdptr = cmdp;
		}

		/*
		 * Check for flow error EOF??
		 */
		if (cmdp == NULL) {
			/* Flow error condition */
			unitp->output.error = TRUE;
			unitp->output.active = FALSE;
			need_processing++;
			ATRACE(audio_4231_playintr, 'LLUN', cmdp);
			goto done;
		}

		if (unitp->output.cmdptr->skip ||
			    unitp->output.cmdptr->done) {
			ATRACE(audio_4231_playintr, 'piks', cmdp);
			goto done;
		}

		/*
		 * Transfer play data
		 */
		/*
		 * Setup for DMA transfers to the buffer from the device
		 */
		length = cmdp->enddata - cmdp->data;

		if (cmdp->data == NULL || length == NULL ||
			    length > audio_4231_bsize) {
			cmdp->skip = TRUE;
			need_processing++;
			goto done;
		}
		ATRACE(audio_4231_playintr, 'LNGT', length);

		buf_dma_handle = (caddr_t)mb_nbmapalloc((struct map *)dvmamap,
				    (caddr_t)cmdp->data,
				    length, MDR_BIGSBUS|MB_CANTWAIT,
				    (func_t)0, (caddr_t)0);

		if (buf_dma_handle == (caddr_t)0) {
			cmdp->skip = TRUE;
			printf("DMA_SETUP failed in play!\n");
			return;
		}

		if (buf_dma_handle) {
			unitp->chip->dmaregs.dmapnva =
			    (u_long)buf_dma_handle;
			unitp->chip->dmaregs.dmapnc =
			    length;
			ATRACE(audio_4231_playintr, 'DMCP',
			    unitp->output.cmdptr);
			audio_4231_insert(cmdp, buf_dma_handle,
				    unitp->playcount,
				    dma_played_list, unitp);
			unitp->playcount++;
			ATRACE(audio_4231_playintr, 'PCNT', counter);
			counter++;

			if (cmdp->next != NULL) {
				unitp->output.cmdptr = cmdp->next;
			}
			ATRACE(audio_4231_playintr, 'DMCN',
			    unitp->output.cmdptr);
			unitp->typ_playlength = length;
		} else {
			printf("apc audio: NULL DMA handle!\n");
		}

	} /* END OF PLAY */

done:
	/*
	 * If no IO is active, shut down device interrupts and
	 * dma from the dma engine. As for the codec we are going
	 * to get another interrupt from the dma engine telling us
	 * that the fifo's have emptied and we can shut down the
	 * codec for capturing. We'll just handle that in the
	 * common interrupt handler. We also will do the final sample
	 * count there.
	 */

	audio_4231_remove(unitp->playcount, dma_played_list, unitp, FALSE);
	if (need_processing)
		audio_xmit_garbage_collect(&unitp->output.as);
}

static void
audio_4231_recintr(unitp)
	cs_unit_t *unitp;
{
	aud_cmd_t *cmdp;
	cs_stream_t *ds;
	caddr_t buf_dma_handle;
	unsigned int length;
	int e;
	int int_active = 0;

#define	Interrupt	1
#define	Active		2

	ds = &unitp->input;		/* Get cs stream pointer */

	ATRACE(audio_4231_recintr, 'LOCk', &unitp->input);

	/* General end of record condition */
	if (ds->active != TRUE) {
		ATRACE(audio_4231_recintr, 'RREA', &unitp->input);
		int_active |= Interrupt;
		goto done;
	}

	if (unitp->input.active) {
		cmdp = unitp->input.cmdptr;

		/*
		 * Ignore null and non-data buffers
		 */
		while (cmdp != NULL && (cmdp->skip || cmdp->done)) {
			cmdp->done = TRUE;
			cmdp = cmdp->next;
			unitp->input.cmdptr = cmdp;
		}

		/*
		 * Check for flow error EOF??
		 */
		if (cmdp == NULL) {
			/* Flow error condition */
			unitp->input.error = TRUE;
			unitp->input.active = FALSE;
			int_active |= Interrupt;
			ATRACE(audio_4231_recintr, 'LLUN', cmdp);
			goto done;
		}

		/*
		 * Setup for DMA transfers to the buffer from the device
		 */
		length = cmdp->enddata - cmdp->data;

		if (cmdp->data == NULL || length == NULL ||
			    length > audio_4231_bsize) {
			cmdp->skip = TRUE;
			goto done;
		}

		buf_dma_handle = (caddr_t)mb_nbmapalloc((struct map *)dvmamap,
				    (caddr_t)cmdp->data,
				    length, MDR_BIGSBUS|MB_CANTWAIT,
				    (func_t)0, (caddr_t)0);

		if (buf_dma_handle == (caddr_t)0) {
			cmdp->skip = TRUE;
			printf("DMA_SETUP failed in record!\n");
			return;
		}

		if (buf_dma_handle) {
			audio_4231_initcmdp(cmdp,
			    unitp->input.as.info.encoding);
			unitp->chip->dmaregs.dmacnva =
			    (u_long)buf_dma_handle;
			unitp->chip->dmaregs.dmacnc = length;
			audio_4231_insert(cmdp, buf_dma_handle,
				    unitp->recordcount,
				    dma_recorded_list,
				    unitp);
			if (unitp->recordcount < 1)
				cmdp->data = cmdp->enddata;
			unitp->recordcount++;
			int_active |= Active;
			if (cmdp->next != NULL) {
				unitp->input.cmdptr = cmdp->next;
			} else {
				cmdp->skip = TRUE;
			}
			unitp->typ_reclength = length;
		} else {
			printf("apc audio: NULL DMA handle!\n");
		}

	} /* END OF RECORD */

done:
	/*
	 * If no IO is active, shut down device interrupts and
	 * dma from the dma engine. As for the codec we are going
	 * to get another interrupt from the dma engine telling us
	 * that the fifo's have emptied and we can shut down the
	 * codec for capturing. We'll just handle that in the
	 * common interrupt handler.
	 */
	if (!(int_active & Active)) {
		unitp->chip->dmaregs.dmacsr |= (APC_CPAUSE);
		audio_4231_pollpipe(unitp);
		audio_4231_recordend(unitp,
			    dma_recorded_list);
	} else {
		audio_4231_remove(unitp->recordcount,
		    dma_recorded_list, unitp, FALSE);
		audio_process_record(&unitp->input.as);
	}
} /* END OF RECORD */


static void
audio_4231_initlist(dma_list, unitp)
	apc_dma_list_t *dma_list;
	cs_unit_t *unitp;
{
	int i;
	int s = 0;

	s = splaudio();
	for (i = 0; i < DMA_LIST_SIZE; i ++) {
		dma_list[i].cmdp = (aud_cmd_t *)NULL;
		dma_list[i].buf_dma_handle = (caddr_t)NULL;
	}
	if (dma_list == dma_recorded_list) {
		unitp->recordcount = 0;
	} else {
		unitp->playcount = 0;
	}
	(void) splx(s);
}

static void
audio_4231_insert(cmdp, buf_dma_handle, count, dma_list, unitp)
	aud_cmd_t *cmdp;
	caddr_t buf_dma_handle;
	unsigned int count;
	apc_dma_list_t *dma_list;
	cs_unit_t *unitp;
{
	int s;

	s = splaudio();
	count %= DMA_LIST_SIZE;
	if (dma_list[count].cmdp == (aud_cmd_t *)NULL) {
		dma_list[count].cmdp = cmdp;
		dma_list[count].buf_dma_handle = buf_dma_handle;
		if (dma_list == dma_recorded_list) {
			unitp->recordlastent = count;
		}
	} else {
		printf("apc audio: insert dma handle failed!\n");
	}
	(void) splx(s);
}

static void
audio_4231_remove(count, dma_list, unitp, clear)
	unsigned int count;
	apc_dma_list_t *dma_list;
	cs_unit_t *unitp;
	unsigned int clear;
{
	int s;

	s = splaudio();
	count = ((count - 3) % DMA_LIST_SIZE);
	if (dma_list[count].cmdp != (aud_cmd_t *)NULL) {
		if (dma_list == dma_recorded_list)  {
			dma_list[count].cmdp->data =
			    dma_list[count].cmdp->enddata;
		}
		dma_list[count].cmdp->done = TRUE;

		if (dma_list == dma_recorded_list)  {
			audio_process_record(&unitp->input.as);
		} else {
			if (!clear)
				audio_process_play(&unitp->output.as);
			else
				audio_xmit_garbage_collect(&unitp->output.as);
		}

		dma_list[count].cmdp = (aud_cmd_t *)NULL;
		if (dma_list[count].buf_dma_handle != 0) {
			mb_mapfree(dvmamap,
				    (int *)&dma_list[count].buf_dma_handle);
		}
		dma_list[count].buf_dma_handle = NULL;
	}
	(void) splx(s);
}

/*
 * Called on a stop condition to free up all of the dma queued
 */
static void
audio_4231_clear(dma_list, unitp)
	apc_dma_list_t *dma_list;
	cs_unit_t *unitp;
{
	int i, s;

	ATRACE(audio_4231_clear, 'RLCu', unitp);
	for (i = 3; i < (DMA_LIST_SIZE + 3); i++) {
		audio_4231_remove(i, dma_list, unitp, FALSE);
	}
	if (dma_list == dma_recorded_list) {
		unitp->recordcount = 0;
	} else {
		unitp->playcount = 0;
	}
}

static void
audio_4231_pollready()
{
	cs_unit_t *unitp;
	register unsigned int x = 0;
	int s = 0;

	unitp = &cs_units[0];

	/*
	 * Wait to see if chip is out of mode change enable
	 */
	while (unitp->chip->pioregs.iar == IAR_NOTREADY &&
			    x <= CS_TIMEOUT) {
			x++;
	}

	x = 0;

	/*
	 * Wait to see if chip has done the autocalibrate
	 */
	unitp->chip->pioregs.iar = TEST_IR;

	while (unitp->chip->pioregs.idr == AUTOCAL_INPROGRESS &&
		x <= CS_TIMEOUT) {
			x++;
	}
}

static void
audio_4231_samplecalc(unitp, dma_len, direction)
	cs_unit_t *unitp;
	unsigned int dma_len;
	unsigned int direction;
{
	unsigned int samples, ncount, ccount = 0;

	/* 1 is recording, 0 is playing XXX Better way??? */

	if (direction) {
		dma_len = audio_4231_sampleconv(&unitp->input, dma_len);
		samples = unitp->input.samples;
		ncount = audio_4231_sampleconv(&unitp->input,
				    unitp->chip->dmaregs.dmacnc);
		ccount = audio_4231_sampleconv(&unitp->input,
				    unitp->chip->dmaregs.dmacc);
		unitp->input.samples =
			    ((samples - ncount) + (dma_len - ccount));
	} else {
		dma_len = audio_4231_sampleconv(&unitp->output, dma_len);
		samples = unitp->output.samples;
		ncount = audio_4231_sampleconv(&unitp->output,
				    unitp->chip->dmaregs.dmapnc);
		ccount = audio_4231_sampleconv(&unitp->output,
				    unitp->chip->dmaregs.dmapc);
		unitp->output.samples =
			    ((samples - ncount) + (dma_len - ccount));
	}
}

/*
 * Converts byte counts to sample counts
 */
static unsigned int
audio_4231_sampleconv(stream, length)
	cs_stream_t *stream;
	unsigned int length;
{

	unsigned int samples;

	if (stream->as.info.channels == 2) {
		samples = (length/2);
	} else {
		samples = length;
	}
	if (stream->as.info.encoding == AUDIO_ENCODING_LINEAR) {
			samples = samples/2;
	}
	return (samples);
}

/*
 * This routine is used to adjust the ending record cmdp->data
 * since it is set in the intr routine we need to look it up
 * in the circular buffer, mark it as done and adjust the
 * cmdp->data point based on the current count in the capture
 * count. Once this is done call audio_process_input() and
 * also call audio_4231_clear() to free up all of the dma_handles.
 */
static void
audio_4231_recordend(unitp, dma_list)
	cs_unit_t *unitp;
	apc_dma_list_t *dma_list;
{

	unsigned int	count, capcount, ncapcount, recend;
	int i, s;

	s = splaudio();
	count = unitp->recordlastent;
	ATRACE(audio_4231_recordend, 'STAL', count);
	capcount = unitp->chip->dmaregs.dmacc;
	ncapcount = unitp->chip->dmaregs.dmacnc;

	recend = ncapcount - capcount;
	if (dma_list[count].cmdp != (aud_cmd_t *)NULL) {
		dma_list[count].cmdp->data =
		    (dma_list[count].cmdp->enddata - recend);
		dma_list[count].cmdp->done = TRUE;
		audio_process_record(&unitp->input.as);

	}


	for (i = 0; i < DMA_LIST_SIZE; i++) {
		if (dma_list[i].buf_dma_handle != 0) {
			if (dma_list[i].buf_dma_handle != 0) {
				mb_mapfree(dvmamap,
				    (int *)&dma_list[i].buf_dma_handle);
			} else {
				call_debug("Null handlep");
			}
			dma_list[i].cmdp = (aud_cmd_t *)NULL;
			dma_list[i].buf_dma_handle = (caddr_t)NULL;
		}
	}

		unitp->recordcount = 0;
	(void) splx(s);
}


/*
 * XXXXX this is a way gross hack to prevent the static from
 * being played at the end of the record. Until the *real*
 * cause can be found this will at least silence the
 * extra data.
 */
static void
audio_4231_initcmdp(cmdp, format)
	aud_cmd_t *cmdp;
	unsigned int format;
{

	int i;
	unsigned int zerosample = 0;

	switch (format) {
	case AUDIO_ENCODING_ULAW:
		zerosample = 0xff; /* silence for ulaw format */
		break;
	case AUDIO_ENCODING_ALAW:
		zerosample = 0xd5;	/* zerosample for alaw */
		break;
	case AUDIO_ENCODING_LINEAR:
		zerosample = 0x00;	/* zerosample for linear */
		break;
	}
	ATRACE(audio_4231_initcmdp, 'PDMC', cmdp);
	ATRACE(audio_4231_initcmdp, 'TAMF', format);

	for (; cmdp->data <= cmdp->enddata; ) {
		*cmdp->data++ = zerosample;
	}
}

void
audio_4231_pollpipe(unitp)
	cs_unit_t *unitp;
{

	int x = 0;

	while (!(unitp->chip->dmaregs.dmacsr & APC_CM) &&
			    x <= CS_POLL_TIMEOUT) {
		x++;
	}

	unitp->chip->dmaregs.dmacsr | = APC_CMI;
}

void
audio_4231_poll_ppipe(unitp)
	cs_unit_t *unitp;
{

	int x = 0;

	while (!(unitp->chip->dmaregs.dmacsr & APC_PM) &&
			    x <= CS_POLL_TIMEOUT) {
		x++;
	}

	unitp->chip->dmaregs.dmacsr | = APC_PMI;
}

void
audio_4231_workaround(unitp)
	cs_unit_t *unitp;
{
	/*
	 * This workaround is so that the 4231 will run a logical
	 * zero sample through the DAC when playback is disabled.
	 * Otherwise there can be a "zipper" noise when adjusting
	 * the play gain at idle.
	 */
	if (!CS4231_reva) {
		unitp->chip->pioregs.iar = IAR_MCE;
		DELAY(100);
		unitp->chip->pioregs.iar = (u_char)IAR_MCD;
		DELAY(100);
		audio_4231_pollready();
		DELAY(1000);
	}
}
