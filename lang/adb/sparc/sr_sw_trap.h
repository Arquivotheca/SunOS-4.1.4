/* "@(#)sr_sw_trap.h 1.1 94/10/31 Copyr 1986 Sun Micro";
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)sw_trap.h	3.2 12/11/85
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* sw_trap.h renamed to sr_sw_trap.h */
/* reserved software trap types (subject to change by software folks)
	There are 128 software trap types, numbered 128 to 255. */

#define ST_BREAKPOINT	255 /* breakPoint trap */
#define ST_SYSCALL	128

#define ST_PUTCONSOLE	253
#define ST_GETCONSOLE	252

#define ST_BOOTENTRY	251 /* used in machine simulation as bootstrap entry */

#define ST_PAUSE	200 /* (temporary) SAS-pause trap		*/
#define ST_DIV0		190 /* (temporary) Division-by-Zero trap	*/
#define ST_SYSCALL_MACH	210 /* (temporary) Kernel system call,
						for device driver emulation */
#define ST_DUP_CTXT0	209 /* (temporary) Machiner mode only,
				duplicate context 0 mapping in all contexts */
