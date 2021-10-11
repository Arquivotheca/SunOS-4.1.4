/*
 ***********************************************************************
 *
 * @(#)gtprobe.h 1.1 94/10/31 21:05:09
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Comment:
 *
 * REVISION HISTORY
 *
 * 03/31/91     Roger Pham      Originated
 *
 ***********************************************************************
 */
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <kvm.h>
#include <nlist.h>
#if !defined(_HKFEIO_)
#define _HKFEIO_
/*
 ***********************************************************************
 *
 *
 * @(#)gtprobe.h 1.1 94/10/31 21:05:09
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Header files for Jet/Hawk Front End board and simulator.
 *
 * ??-???-88 Paul Ramsey        Initial version created.
 *  2-Apr-90 Kevin C. Rushforth Updates for HFE hardware.
 *  3-May-90 Tom Bowman         yet another version of hawk2
 *
 ***********************************************************************
 */

/*
 ***********************************************************************
 *
 * @(#)interprocess.h 1.3 90/05/03 15:33:15
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Header files for Jet/Hawk/Hawk2 Front End board and simulator.
 *
 * ??-???-?? ???l ??????        Initial version created.
 *  3-May-90 Tom Bowman         yet another version for hawk2
 *
 ***********************************************************************
 */
#define BUFFERSIZE 1024 /* MUST BE AT LEAST 3 * MAXBPS */
#define MAXBP 256

typedef struct {
        long address;
        long type;
        long data;
} BREAKPOINT;

typedef struct {
        long rcregs[65];
        long rdregs[65];
        long lr;
        long lt;
        long lu;
        long fcxh,fcxl;
        long fcyh,fcyl;
        long fcxt,fcyt;
        long fch,fcl;
        long fcsh,fcsl;
        long fcph,fcpl;
        long fcch, fccl;
        long fdxh,fdxl;
        long fdyh,fdyl;
        long fdxt,fdyt;
        long fdh,fdl;
        long fdsh,fdsl;
        long fdph,fdpl;
        long fdch,fdcl;
        long mo;
        long sf;
        long sqpc;
        long sqstack[65];
        long sqra;
        long sqrb;
        long sqsp;
        long dr,dw,sdr;
        long ac1,ac2,ar;
        long icount;
} STATE;

/* types of breakpoints */
#define NORMALBP 0

typedef struct {
        union {
                long raw[BUFFERSIZE];
                STATE state;
                BREAKPOINT breakpoints[MAXBP];
        } data;
        long command;
        long param1;
        long param2;
        long param3;
        long param4;
        char strparam1[100];
} FESIM_COMM;

/* these are the commands */
#define GETSTATE   1
#define PUTSTATE   2
#define GETMEMORY  3
#define PUTMEMORY  4
#define GETZERO    5
#define PUTZERO    6
#define GETSTOP    7
#define PUTSTOP    8
#define GETJAM     9
#define PUTJAM     10
#define GETIVR     11
#define PUTIVR     12
#define CLRSCRN    13
#define PUTBP      14
#define GETBP      15
#define CONTINUE   16
#define GETBPSTAT  17
#define STEP       18
#define BREAK      19
/*
 ***************************************
 * Check for valid compiler flag
 ***************************************
 */

/* Check for either type of hardware */
#if !defined(HFE_HARDWARE)
#define HFE_SIMULATOR
#endif /* not HFE_HARDWARE */



/*
 ***************************************
 * Define TA_HANDLE
 ***************************************
 */

#if defined(HFE_HARDWARE)
typedef struct {
        int *mapped_mem;
        int *slave_mode;
        char filename[40];
        int status;
        int jetfefilehandle;
        unsigned vmsize;
        unsigned *vaddr;
        unsigned *paddr;
        unsigned erraddr;
} TA_HANDLE;
#endif /* HFE_HARDWARE */

#if defined(HFE_SIMULATOR)
typedef FESIM_COMM TA_HANDLE;
#endif HFE_SIMULATOR


/*
 ***************************************
 * Address definitions
 ***************************************
 */

/* HFE hardware (and simulator) addresses */
#if defined(HFE_HARDWARE) || defined(HFE_SIMULATOR)
#define HK_HOSTBASEADDR         0x00000000

/* Local bus addresses */
#define FE_LBBASEADDR           0x00000000
#define REGBASEADDR             0x00200000
#define REGSIZE                 0x00020000

/* Host interface registers */
#define MIA_HCR_ADDR            0x00200001      /* Host only */
#define MIA_IMR_ADDR            0x00200002      /* FE only */
#define MIA_HOST_INT_ADDR       0x00200003      /* Host only */
#define MIA_FE_INT_ADDR         0x00200004      /* FE only */
#define MIA_ATU_SYNC_ADDR       0x00200005      /* Host only */
#define MIA_MDB_SYNC_ADDR       0x00200006
#define MIA_HOST_FLAG0_ADDR     0x00200007
#define MIA_HAWK_FLAG0_ADDR     0x00200008
#define MIA_TEST_ADDR           0x0020000B

#define MIA_VBPT_ADDR           0x00200100      /* FE only */
#define MIA_LBARC_ADDR          0x00200200      /* FE only */
#define MIA_MBARC_ADDR          0x00200300      /* FE only */
#define MIA_PHYSICAL_ADDR       0x00200400      /* Host only */
#define MIA_VIRTUAL_ADDR        0x00200440      /* Host only */
#define MIA_ROOT_PTP_ADDR       0x00200480      /* Host only */
#define MIA_LVL1_PTP_ADDR       0x002004C0      /* Host only */
/* Add TLB entries here */

#define MIA_HOST_FLAG1_ADDR     0x00204009
#define MIA_HAWK_FLAG1_ADDR     0x0020400A
#define MIA_HOST_FLAG2_ADDR     0x0020800C
#define MIA_HAWK_FLAG2_ADDR     0x0020800D
#define MIA_HOST_FLAG3_ADDR     0x0020C00E
#define MIA_HAWK_FLAG3_ADDR     0x0020C00F

/* Front end registers */
#define FE_HOLD                 0x00210000
#define FE_INT_STAT             0x00210002
#define FE_INT_MSK              0x00210004
#define FE_HOINT_STAT           0x00210010
#define FE_ERR_ADDR             0x00210016
#define FE_LDM_BASEADDR         0x00380000
#define FE_LDM_SIZE             0x00080000

/* HSA diag registers */
#define HSA_MODE_REG            0x0084F000
#define HSA_ADDR_REG            0x0084F004

#endif /* HFE_HARDWARE || HFE_SIMULATOR */

/*
 ***************************************
 * HCR bit definitions
 ***************************************
 */

#if defined(HFE_HARDWARE)
/* bits in the host interface register */
#define FE_RESET                0x01
#define FE_GO                   0x02
#define BE_RESET                0x10
/* #define FE_BREAK             0x04 */
/* #define FE_STEP                      0x08 */
/* #define FE_JAM                       0x10 */
/* #define FE_ZERO                      0x20 */
#define HK_RESET                0x40
#endif HFE_HARDWARE



/*
 ***************************************
 * Common definitions (e.g. for jsim)
 ***************************************
 */

#define LDMBASEADDR             FE_LDM_BASEADDR
#define LDMSIZE                 FE_LDM_SIZE

#define FE_OPEN                 0xFEFE
#define FE_FAILURE              (-1)

TA_HANDLE *ta_open();
#endif _HKFEIO_

#ifdef HFE_HARDWARE
#include <sys/ioctl.h>
#endif HFE_HARDWARE
extern int errno;
/* define sbus slot environment variables */
#define NULL            0
#define GT_SLOT_ENV     "GT_SLOT"
#define GT_SLOT_DEFAULT 2

#ifdef HFE_HARDWARE
#define HK_DEVICE1_NAME		"/dev/sbus1"
#define HK_DEVICE2_NAME		"/dev/sbus2"
#define HK_DEVICE3_NAME		"/dev/sbus3"
#define HK_DEVICE_BASE		0
#define HK_DEVICE_SIZE		(32 * 0x100000)  /* size = 0x0020 0000 */
#else
#define HK_DEVICE_NAME		"/dev/mem"
#define HK_DEVICE_BASE		0x30000000
#define HK_DEVICE_SIZE		(16 * 0x100000)
#endif HFE_HARDWARE

#ifdef DEBUG
#define PERROR(x) printf(x)
#else
#define PERROR(x)
#endif

#define MODE_MASK		0x00030003

#define PRINT_LINE	printf ("\n")

typedef char	Slot_name [100];

typedef struct {
	int slot1, slot2, slot3;
	Slot_name device1, device2, device3;
	} SBus;

#define	TEST_REG	0x0080002c
