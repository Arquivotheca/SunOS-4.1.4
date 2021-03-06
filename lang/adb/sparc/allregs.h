/*	@(#)allregs.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * adb keeps its own idea of the current value of most of the
 * processor registers, in an "adb_regs" structure.  This is used
 * in different ways for kadb, adb -k, and normal adb.
 *
 * For kadb, this is really the machine state -- so kadb must
 * get the current window pointer (CWP) field of the psr and
 * use it to index into the windows to find the currently
 * relevant register set.
 *
 * For a normal adb, the TBR and WIM registers aren't present in the
 * struct regs that we get (either within a core file, or from
 * PTRACE_GETREGS); so I might use those two fields for something.
 * In this case, we always ignore the "locals" half of register
 * window zero.  Its "ins" half is used to hold the current "outs",
 * and window one has the current locals and "ins".
 *
 * For adb -k (post-mortem examination of kernel crash dumps), there
 * is no way to find the current globals or outs, but the stack frame
 * that sp points to will tell us the current locals and ins.  Because
 * we have no current outs, I suppose that we could use window zero for
 * the locals and ins, but I'd prefer to make it the same as normal adb.
 * Also, if the kernel crash-dumper is changed to make these available
 * somehow, I'd have to change things again.
 */

#ifndef rw_fp
#include <machine/reg.h>
#endif

#ifndef LOCORE
#ifndef _ALLREGS_
#define _ALLREGS_
#include <machine/pcb.h>
struct allregs {
	int		r_psr;
	int		r_pc;
	int		r_npc;
	int		r_tbr;
	int		r_wim;
	int		r_y;
	int		r_globals[7];
#ifdef KADB
	struct rwindow	r_window[MAXWIN];	/* locals, then ins */
#else !KADB
	int		r_outs[8];
	int		r_locals[8];
	int		r_ins[8];
#endif !KADB
};
#endif !_ALLREGS_
#endif !LOCORE
