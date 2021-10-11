/*
 * Copyright (c) 1989-93 by Sun Microsystems, Inc.
 */

/*
 * @(#)audio_4231.h 1.1 94/10/31 SMI
 */


/*
 * Macros for distinguishing between minor devices
 */
#define	CS_UNITMASK	(0x0f)
#define	CS_UNIT(dev)	((geteminor(dev)) & CS_UNITMASK)
#define	CS_ISCTL(dev)	(((geteminor(dev)) & CS_MINOR_CTL) != 0)
#define	CS_CLONE_BIT	(0x10)

#define	CS_MINOR_RW	(1)
#define	CS_MINOR_RO	(2)
#define	CS_MINOR_CTL	(3)
#define	CS_MINOR_CTL_OLD	(128)

/*
 * Default Driver constants for the 4231
 */
#define	AUD_CS4231_PRECISION	(8)		/* Bits per sample unit */
#define	AUD_CS4231_CHANNELS	(1)		/* Channels per sample frame */
#define	AUD_CS4231_SAMPLERATE	(8000)		/* Sample frames per second */
#define	AUD_CS4231_SAMPR16000	(16000)
#define	AUD_CS4231_SAMPR11025	(11025)
#define	AUD_CS4231_SAMPR18900	(18900)
#define	AUD_CS4231_SAMPR22050	(22050)
#define	AUD_CS4231_SAMPR32000	(32000)
#define	AUD_CS4231_SAMPR37800	(37800)
#define	AUD_CS4231_SAMPR44100	(44100)
#define	AUD_CS4231_SAMPR48000	(48000)
#define	AUD_CS4231_SAMPR9600	(9600)
#define	AUD_CS4231_SAMPR8000	AUD_CS4231_SAMPLERATE

/*
 * This is the default size of the STREAMS buffers we send down the
 * read side and the maximum record buffer size that can be specified
 * by the user. The record buffer size must be at least 8 < 8192 since
 * in audio_process_record there is a call to allocb for BSIZE +8. If
 * the size goes over 8192 the alllocb in the streams code scribbles
 * on the rootvfs pointer. 
 */

#define	AUD_CS4231_BSIZE	(8180)		/* Record buffer size */
#define	AUD_CS4231_MAX_BSIZE	(65536)		/* Maximum buffer_size */
#define	AUDIO_INTERNAL_CD_IN	0x04


/*
 * Buffer allocation
 */
#define	AUD_CS4231_CMDPOOL	(100)	/* total command block buffer pool */
#define	AUD_CS4231_RECBUFS	(50)	/* number of record command blocks */
#define	AUD_CS4231_PLAYBUFS	(AUD_79C30_CMDPOOL - AUD_79C30_RECBUFS)

/*
 * Driver constants
 */
#define	AUD_CS4231_IDNUM	(0x6175)
#define	AUD_CS4231_NAME		"audiocs"
#define	AUD_CS4231_MINPACKET	(0)
#define	AUD_CS4231_MAXPACKET	(8180)	/* for better accuracy */
#define	AUD_CS4231_HIWATER	(57000)
#define	AUD_CS4231_LOWATER	(32000)

/*
 * Default gain settings
 */
#define	AUD_CS4231_DEFAULT_PLAYGAIN	(132)	/* play gain initialization */
#define	AUD_CS4231_DEFAULT_RECGAIN	(126)	/* gain initialization */

#define	AUD_CS4231_MIN_ATEN	(0)	/* Minimum attenuation */
#define	AUD_CS4231_MAX_ATEN	(31)	/* Maximum usable attenuation */
#define	AUD_CS4231_MAX_DEV_ATEN	(63)	/* Maximum device attenuation */
#define	AUD_CS4231_MIN_GAIN	(0)
#define	AUD_CS4231_MAX_GAIN	(15)

/*
 * Monitor Gain settings
 */
#define	AUD_CS4231_MON_MIN_ATEN		(0)
#define	AUD_CS4231_MON_MAX_ATEN		(63)


/*
 * Values returned by the AUDIO_GETDEV ioctl
 */
#define	CS_DEV_NAME		"SUNW,CS4231"
#define	CS_DEV_VERSION		"a"
#define	CS_DEV_CONFIG_ONBRD1	"onboard1"


/*
 * These are the registers for the Crystal Semiconductor 4231 chip.
 */

struct aud_4231_pioregs {
	u_char iar;		/* Index Address Register */
	u_char pad[3];		/* PAD  */
	u_char idr;		/* Indexed Data Register */
	u_char pad1[3];	/* PAD */
	u_char statr;		/* Status Register */
	u_char pad2[3];	/* PAD */
	u_char piodr;		/* PIO Data Register I/O */
	u_char pad3[3];	/* PAD */
};

/*
 * These are the registers for the APC DMA channel interface to the
 * 4231.
 */

struct apc_dma {
	u_long 	dmacsr;		/* APC CSR */
	u_long		lpad[3];	/* PAD */
	u_long 	dmacva;		/* Captue Virtual Address */
	u_long 	dmacc;		/* Capture Count */
	u_long 	dmacnva;	/* Capture Next VAddress */
	u_long 	dmacnc;		/* Capture next count */
	u_long 	dmapva;		/* Playback Virtual Address */
	u_long 	dmapc;		/* Playback Count */
	u_long 	dmapnva;	/* Playback Next VAddress */
	u_long 	dmapnc;		/* Playback Next Count */
};

struct aud_4231_chip {
	struct aud_4231_pioregs pioregs;
	struct apc_dma dmaregs;
};

#define	DMA_LIST_SIZE 8

typedef struct apc_dma_list {
	aud_cmd_t *cmdp;
	caddr_t buf_dma_handle;
} apc_dma_list_t;

/*
 * Device-dependent audio stream which encapsulates the generic audio
 * stream
 */
typedef struct cs_stream {
	/*
	 * Generic audio stream.  This MUST be the first structure member
	 */
	aud_stream_t as;

	/* DD Audio */
	aud_cmd_t *cmdptr;		/* current command pointer */

	/*
	 * Current statistics
	 */
	unsigned int samples;
	unsigned char active;
	unsigned char error;
} cs_stream_t;


/*
 * This is the control structure used by the CS4231-specific driver
 * routines.
 */
typedef struct {
	/*
	 * Device-independent state--- MUST be first structure member
	 */
	aud_state_t distate;

	/*
	 * Address of the unit's registers
	 */
	struct aud_4231_chip *chip;

	/*
	 * Device-dependent audio stream strucutures
	 */
	cs_stream_t input;
	cs_stream_t output;
	cs_stream_t control;

	/*
	 * Pointers to per-unit allocated memory so we can free it
	 * at detach time
	 */
	caddr_t *allocated_memory;	/* kernel */
	size_t allocated_size;		/* kernel */

	/*
	 * Driver state info.
	 */
	unsigned int playcount;
	unsigned int typ_playlength;
	unsigned int typ_reclength;
	unsigned int recordcount;
	unsigned int recordlastent;
	unsigned int hw_output_inited;
	unsigned int hw_input_inited;

	/*
	 * OS-dependent info
	 */
	struct dev_info	*dip; /* dev info pointer */

	/*
	 * The loopback ioctl sets the device to a funny state, so we need
	 * to know if we have to re-initialize the chip when the user closes
	 * the device.
	 */
	int init_on_close;

} cs_unit_t;

/*
 * Macros to derive control struct and chip addresses from the audio struct
 */
#define	UNITP(as)	((cs_unit_t *)((as)->v->devdata))
#define	CSR(as)		(UNITP(as)->chip)
#define	ASTOCS(as) 	((cs_stream_t *)(as))
#define	CSTOAS(cs) 	((aud_stream_t *)(cs))


/* Shared defineds for gains, muteing etc.. */
#define	GAIN_SET(var, gain)	((var & ~(0x3f)) | gain)
#define	RECGAIN_SET(var, gain)	((var & ~(0x1f)) | gain)
#define	CHANGE_MUTE(var, val)		((var & ~(0x80)) | val)
#define	MUTE_ON(var)			(var | 0x80)
#define	MUTE_OFF(var)			(var & 0x7f)

#define	LINEMUTE_ON		0x80
#define	LINEMUTE_OFF		(~0x80)

/* slot zero of both the record and play is the max attenuation */

#define	CS4231_MAX_DEV_ATEN_SLOT	(0)

/* Index Register values */

#define	IAR_AUTOCAL_BEGIN	0x40
#define	IAR_AUTOCAL_END		~(0x40)
#define	IAR_MCE			0x40 /* Mode change Enable */
#define	IAR_MCD			IAR_AUTOCAL_END
#define	PRDY 			0x02	/* PIO DR Rdy. */
#define	CRDY 			0x20	/* PIO DR Rdy. */
#define	SER 			0x10	/* PIO DR Rdy. */
#define	INTB 			0x01	/* INT bit in Status Reg. */
#define	IAR_NOTREADY		0x80	/* 80h not ready CODEC state */

/*
#define	CHANGE_INPUT(var, val)		((var & ~(0xC0)) | val)
*/
#define	CHANGE_INPUT(var, val)		((var & ~(0x0f)) | val)

/* Left input control (Index 0) M 1&2 */
#define	L_INPUT_CR		0x0
#define	L_INPUTCR_AUX1		0x40
#define	MIC_ENABLE(var)		((var & 0x2f) | 0x80)
#define	LINE_ENABLE(var)	(var & 0x2f)
#define	CDROM_ENABLE(var)	((var & 0x2f) | 0x40)

/*
#define	LINE_ENABLE		(~0xC0)
*/

/* Right Input Control (Index 1) M 1&2 */
#define	R_INPUT_CR		0x1
#define	R_INPUTCR_AUX1		L_INPUTCR_AUX1

/* Left Aux. 1 Input Control (Index 2) M 1&2 */
#define	L_AUX1_INPUT_CR		0x2

/* Right Aux. 1 Input Control (Index 3) M 1&2 */
#define	R_AUX1_INPUT_CR		0x3

/* Left Aux. 2 Input Control (Index 4) M 1&2 */
#define	L_AUX2_INPUT_CR		0x4

/* Right Aux. 1 Input Control (Index 5) M 1&2 */
#define	R_AUX2_INPUT_CR		0x5

/* Left Output Control (Index 6) M 1&2 */
#define	L_OUTPUT_CR		0x6
#define	OUTCR_MUTE		0x80
#define	OUTCR_UNMUTE		~0x80

/* Right Output Control (Index 7) M 1&2 */
#define	R_OUTPUT_CR		0x7

/* Playback Data Fromat Register (Index 8) Mode 2 only */
#define	PLAY_DATA_FR		0x08

#define	CHANGE_DFR(var, val)		((var & ~(0xF)) | val)
#define	CHANGE_ENCODING(var, val)	((var & ~(0xe0)) | val)
#define	DEFAULT_DATA_FMAT		0x20
#define	CS4231_DFR_8000			0x00	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_9600			0x0e	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_11025		0x03	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_16000		0x02	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_18900		0x05	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_22050		0x07	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_32000		0x06	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_37800		0x09	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_44100		0x0b	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_48000		0x0c	/* CSF Bits and XTAL bits */
#define	CS4231_DFR_ULAW 		0x20	/* Mu law 8 bit companded */
#define	CS4231_DFR_ALAW			0x60	/* Alaw 8 bit companded */
#define	CS4231_DFR_ADPCM		0xa0	/* ADPCM 4 bit IMA */
#define	CS4231_DFR_LINEARBE		0xc0	/* Linear 16 bit 2's Big E */

#define	CS4231_STEREO_ON(val)		(val | 0x10)
#define	CS4231_MONO_ON(val)		(val & ~0x10)

/* Interface Configuration Register (Index 9) */
#define	INTERFACE_CR		0x09
#define	CHIP_INACTIVE		0x08
#define	PEN_ENABLE		(0x01) /* ACAL and PEN */
#define	PEN_DISABLE		(~0x01)
#define	CEN_ENABLE		(0x02) /* ACAL and CEN */
#define	CEN_DISABLE		(~0x02)
#define	ACAL_DISABLE		(~0x08)

/*
 * Enable = playback, capture, Dual DMA Channels, Autocal, Playback DMA
 * only, capture DMA only.
 */
#define	ICR_AUTOCAL_INIT	0x01 /* PLAY ONLY FOR NOW XXXXX */

/* Pin Control Register (Index 10) M 1&2  For Interrupt enable */
#define	PIN_CR			0x0a
#define	INTR_ON			0x82
#define	INTR_OFF		0x80
#define	PINCR_LINE_MUTE		0x40
#define	PINCR_HDPH_MUTE		0x80

/* Test and Initialization Register (Index 11) M 1&2 */
#define	TEST_IR			0x0b
#define	DRQ_STAT		0x10
#define	AUTOCAL_INPROGRESS	0x20

/* Misc. Information Register (Index 12) Mode 1&2 */

#define	MISC_IR			0x0c
#define	MISC_IR_MODE2		0x40

/* Loopback Control Register (Index 13) M 1&2 */
#define	LOOPB_CR		0x0d
#define	LOOPB_ON		0x01
#define	LOOPB_OFF		0x00

/* Upper base Register (Index 14) M1 only  Not used */
/* Lower base Register (Index 15) M1 only  Not used */

/* Playback Upper Base Register (Index 14) M2 only */
#define	PLAYB_UBR		0x0e

/* Playback Lower Base Register (Index 15) M2 only */
#define	PLAYB_LBR		0x0f

/* All of the folowing registers only apply to MODE 2 Operations */

/* Alternate Feature Enable I (Index 16) */
#define	ALT_FEA_EN1R		0x10
#define	OLB_ENABLE		0x80

/* Alternate Feature Enable II (Index 17) */
#define	ALT_FEA_EN2R		0x11
#define	HPF_ON			0x01
#define	XTALE_ON		0x20

/* Left Line Input Gain (Index 18) */
#define	L_LINE_INGAIN_R		0x12

/* Right Line Input Gain (Index 19) */
#define	R_LINE_INGAIN_R		0x13

/* Timer Hi Byte (Index 20) */
#define	TIMER_HIB_R		0x14

/* Timer Lo Byte (Index 21) */
#define	TIMER_LOB_R		0x15

/* Index 22 and 23 are reserrved */

/* Alternate Feature Status (Index 24) */
#define	ALT_FEAT_STATR		0x18
#define	CS_PU			0x01	/* playback underrun */
#define	CS_PO			0x20	/* playback Overrun */

/* Version / ID (Index 25) */

#define	VERSION_R	0x19
#define	CS4231A		0x20
#define	CS4231CDE	0x80

/* Mono Input and Output Control (Index 26) */
#define	MONO_IOCR		0x1a
#define	CHANGE_MONO_GAIN(val)	((val & ~(0xFF)) | val)
#define	MONOIOCR_SPKRMUTE	0x40;

/*
 * Capture Data Format Register (Index 28)
 * The bit operations on this are the same as for the PLAY_DATA_FR
 * (Index *).
 */
#define	CAPTURE_DFR		0x1c


/* Capture Base Register Upper for DMA (Index 30) */
#define	CAPTURE_UBR		0x1e

/* Capture Base Register Lower for DMA (Index 30) */
#define	CAPTURE_LBR		0x1f

/*
 * APC CSR Register bit definitions
 */

#define	APC_IP		0x800000	/* Interrupt Pending */
#define	APC_PI		0x400000	/* Playback interrupt */
#define	APC_CI		0x200000	/* Capture interrupt */
#define	APC_EI		0x100000	/* General interrupt */
#define	APC_IE		0x80000		/* General ext int. enable */
#define	APC_PIE		0x40000		/* Playback ext intr */
#define	APC_CIE		0x20000		/* Capture ext intr */
#define	APC_EIE		0x10000		/* Error ext intr */
#define	APC_PMI		0x8000		/* Pipe empty interrupt */
#define	APC_PM		0x4000		/* Play pipe empty */
#define	APC_PD		0x2000		/* Playback NVA dirty */
#define	APC_PMIE	0x1000		/* play pipe empty Int enable */
#define	APC_CM		0x800		/* Cap data dropped on floor */
#define	APC_CD		0x400		/* Capture NVA dirty */
#define	APC_CMI		0x200		/* Capture pipe empty interrupt */
#define	APC_CMIE	0x100		/* Cap. pipe empty int enable */
#define	APC_PPAUSE	0x80		/* Pause the play DMA */
#define	APC_CPAUSE	0x40		/* Pause the capture DMA */
#define	APC_CODEC_PDN   0x20		/* CODEC RESET */
#define	PDMA_GO		0x08
#define	CDMA_GO		0x04		/* bit 2 of the csr */
#define	APC_RESET	0x01		/* Reset the chip */
#define	INITIAL_SETUP	0x00FD000C	/* prime the apc on start */
#define	PLAY_SETUP	(APC_EI | APC_PI | APC_IE | APC_PIE | \
			    APC_PMI | APC_EIE | PDMA_GO | APC_PMIE)
#define	CAP_SETUP	(APC_EI | APC_CI | APC_IE | APC_CIE | \
			    APC_CMI | APC_EIE | CDMA_GO)
