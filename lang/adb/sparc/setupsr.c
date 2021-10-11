#ifndef lint
static	char sccsid[] = "@(#)setupsr.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * adb - routines to read a.out+core at startup
 */

#include "adb.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vmmac.h>
#include <vm/seg_u.h>
#include "symtab.h"

off_t	datbas;			/* offset of the base of the data segment */
off_t	stksiz;			/* stack size in the core image */
off_t   textaddr();		/* address of the text segment */

#ifndef KADB
#include <kvm.h>

char	*symfil	= "a.out";
char	*corfil	= "core";

extern kvm_t *kvmd;		/* see main.c */
struct user *uarea;		/* a ptr to the currently mapped uarea */
struct asym *trampsym, *funcsym;
extern int use_shlib;   /* non-zero __DYNAMIC ==> extended maps for text */
#endif !KADB

setsym()
{
	off_t loc;
	unsigned long txtbas;
	struct map_range *fsym_rng1, *fsym_rng2;

	fsym_rng1 = (struct map_range*) calloc(1, sizeof (struct map_range));
	fsym_rng2 = (struct map_range*) calloc(1, sizeof (struct map_range));
	txtmap.map_head = fsym_rng1;
	txtmap.map_tail = fsym_rng2;
	fsym_rng1->mpr_next = fsym_rng2;

#ifndef KADB
	fsym_rng1->mpr_fn = fsym_rng2->mpr_fn  = symfil;
	fsym = getfile(symfil, 1);
	fsym_rng1->mpr_fd  = fsym_rng2->mpr_fd = fsym;
	if (read(fsym, (char *)&filhdr, sizeof filhdr) != sizeof filhdr ||
			N_BADMAG(filhdr))
	{
			fsym_rng1->mpr_e = MAXFILE;
			return;
	}

#endif !KADB
    loc = filhdr.a_text+filhdr.a_data;
	fsym_rng1->mpr_f = fsym_rng2->mpr_f =  N_TXTOFF(filhdr);
	fsym_rng1->mpr_b = 0;
	/* check for unknown machine types */
	switch (filhdr.a_machtype) {
	case M_SPARC:
		break;
	case M_OLDSUN2:
	case M_68010:
	case M_68020:
		break;
	default:
		printf("warning: unknown machine type %d\n", filhdr.a_machtype);
		break;
	}
	switch (filhdr.a_magic) {

	case OMAGIC:
		txtbas  = textaddr(&filhdr);
	fsym_rng1->mpr_b = txtbas;
		fsym_rng1->mpr_e =  txtbas + filhdr.a_text;
		fsym_rng2->mpr_b = datbas = txtbas;
		fsym_rng2->mpr_e =  txtbas + loc;
		break;

	case ZMAGIC:
	case NMAGIC:
		fsym_rng1->mpr_b = textaddr(&filhdr);
		fsym_rng1->mpr_e = filhdr.a_text+N_TXTADDR(filhdr);
		fsym_rng2->mpr_b = datbas = roundup((int)fsym_rng1->mpr_e, N_SEGSIZ(filhdr));
		fsym_rng2->mpr_e = datbas + filhdr.a_data;
		fsym_rng2->mpr_f += filhdr.a_text;
	}
	stinit(fsym, &filhdr, AOUT);
#ifndef KADB
	(void) lookup( "__sigtramp");
	if (trampsym = cursym) {
		(void) lookup( "__sigfunc" );
		funcsym = cursym;
	} else
		funcsym = 0;
#endif !KADB
#ifdef DEBUG
  { extern int adb_debug;

	if ( adb_debug )
	   printf("__sigtramp at 0x%X, __sigfunc at 0x%X\n",
		trampsym? trampsym->s_value : 0,
		funcsym? funcsym->s_value : 0);
  }
#endif
}

/*
 * address of the text segment.  Normally this is given
 * by (x).a_entry, but an exception is made for demand
 * paged (ZMAGIC == 0413) files, in which the exec structure
 * is at the beginning of text address space, and the entry
 * point immediately follows.
 */
static off_t
textaddr(fhdr)
	struct exec *fhdr;
{

	if (fhdr->a_magic == ZMAGIC) {
		return (N_TXTADDR(*fhdr));
	}
	return (fhdr->a_entry);
}

#ifndef KADB
static
ksetcor(readpanicregs)
	int readpanicregs;
{
	char *looking;

	kcore = 1;
	masterprocp = 0; /* don't override Sysmap mapping */
	/*
	 * Pretend we have 16 meg until we read in the
	 * value of 'physmem'
	 */
	physmem = 16 * 1024 *1024;
	(void) lookup(looking = "_physmem");
	if (cursym == 0)
		goto nosym;
	if (kvm_read(kvmd, (long)cursym->s_value, (char *)&physmem,
	    sizeof physmem) != sizeof physmem)
		goto nosym;
	printf("physmem %X\n", physmem);
    datmap.map_head->mpr_e = MAXFILE;
	(void) lookup(looking = "_masterprocp");
	if (cursym == 0)
		goto nosym;
	if (kvm_read(kvmd, (long)cursym->s_value, (char *)&masterprocp,
	    sizeof masterprocp) != sizeof masterprocp)
		goto nosym;
	getproc();
	if (readpanicregs) {
		label_t pregs;

		(void) lookup(looking = "_panic_regs");
		if (cursym == 0)
			goto nosym;

		db_printf( "ksetcor:  if ( readpanicregs ) getpcb( 0x%X )\n",
		    cursym->s_value );

		if (kvm_read(kvmd, (long)cursym->s_value, (char *)&pregs,
		    sizeof pregs) != sizeof pregs)
			goto nosym;
		getpcb(&pregs);
	}
	return;

nosym:
	printf("Cannot adb -k: %s missing symbol %s\n",
	    corfil, looking);
	exit(2);
}


setcor()
{
	struct stat stb;
	struct exec ex;
    struct map_range *fcor_rng1, *fcor_rng2;

	fcor_rng1 = (struct map_range*) calloc(1, sizeof (struct map_range));
	fcor_rng2 = (struct map_range*) calloc(1, sizeof (struct map_range));
	datmap.map_head = fcor_rng1;
	datmap.map_tail = fcor_rng2;
	fcor_rng1->mpr_next = fcor_rng2;

	fcor_rng1->mpr_fn = fcor_rng2->mpr_fn  = corfil;
	fcor = getfile(corfil, 2);
	fcor_rng1->mpr_fd  = fcor_rng2->mpr_fd = fcor;
	if (fcor == -1) {
		  fcor_rng1->mpr_e = MAXFILE;
		  return;
	}
	if (kernel) {
		ksetcor(1);
		return;
	}
	fstat(fcor, &stb);

	if (((stb.st_mode&S_IFMT) == S_IFREG) &&
	    read(fcor, (char *)&core, sizeof (core)) == sizeof (core) &&
	    !N_BADMAG(core.c_aouthdr) &&
	    core.c_magic == CORE_MAGIC &&
	    core.c_len >= sizeof (core)) {
		ex = core.c_aouthdr;
		printf("core file = %s -- program ``%s''\n", corfil,
		    core.c_cmdname);

		stksiz = core.c_ssize;

		switch (ex.a_magic) {

		case OMAGIC:
			fcor_rng1->mpr_b = N_TXTADDR(ex) + core.c_tsize;
			fcor_rng1->mpr_e = fcor_rng1->mpr_b + core.c_dsize;
			fcor_rng2->mpr_f = sizeof (core) + core.c_tsize + core.c_dsize;
			break;

		case NMAGIC:
		case ZMAGIC:
			fcor_rng1->mpr_b = roundup(N_TXTADDR(ex) + core.c_tsize,
				N_SEGSIZ(ex));
			fcor_rng1->mpr_e = fcor_rng1->mpr_b + core.c_dsize;
			fcor_rng2->mpr_f = sizeof (core) + core.c_dsize;
			break;
		}

	datbas = fcor_rng1->mpr_b;
		fcor_rng1->mpr_f = sizeof (core);
		fcor_rng2->mpr_e = USRSTACK;
		fcor_rng2->mpr_b = fcor_rng2->mpr_e - stksiz;

		if (filhdr.a_magic && ex.a_magic &&
		    filhdr.a_magic != ex.a_magic) {
				fcor_rng1->mpr_b = 0;
				fcor_rng1->mpr_e = MAXFILE;
				fcor_rng1->mpr_f = 0;
			printf(" - warning: bad magic number\n");
		} else {
			/*
			 * On sparc, the u.u_pcb contains most of the
			 * registers (all but the G and status regs).
			 * So we sorta gotta read it in.  Could read just
			 * the pcb, but...
			 */
			read_ustruct(fcor);

			signo = core.c_signo;
			sigprint(signo);
			printf("\n");
		}
		core_to_regs( );
	} else {
		/* either not a regular file, or core struct not convincing */
		printf("not core file = %s\n", corfil);
		fcor_rng1->mpr_e = MAXFILE;
	}
	if (use_shlib) {
		extend_text_map();
	}
}

#else KADB

/*
 * For kadb's sake, we pretend we can always find
 * the registers on the stack.
 */
regs_not_on_stack ( ) {
	return 0;
}

#endif KADB



#include <sys/file.h>

/*
 * Get the whole u structure.  We need the pcb for stack tracing.
 * Called by setcor only if we got what looks like a convincing core file.
 */
read_ustruct ( core_fd )
int core_fd;
{
	off_t pos_was, pos_ustruct, err, lseek( );

#ifndef KADB
	pos_ustruct = core.c_len + core.c_dsize + core.c_ssize;
	pos_was = lseek( core_fd, 0L, L_INCR );		/* = tell(core_fd); */
	err = lseek( core_fd, pos_ustruct, L_SET );	/* <sys/file.h> */

	if (err != -1  &&  pos_was != -1 &&		/* ok pos'ns */
	    read( core_fd, (char *)& u, sizeof u ) == sizeof u ) {
		/* Got it ok. */

		/* go back whence you came */
		(void) lseek( core_fd, pos_was, L_SET );
	} else {
		printf("core file (\"%s\") appears to be truncated.\n", corfil);
    	datmap.map_head->mpr_e = MAXFILE;
	}
#endif !KADB
}

/*
 * Only called if kernel debugging (-k or KADB).  Read in (or map)
 * the u-area for the appropriate process and get the registers.
 */
getproc()
{
	struct proc proc;
#ifdef KADB
#define foffset  ((char *)&proc.p_flag  - (char *)&proc)
#define uoffset ((char *)&proc.p_uarea - (char *)&proc)
	struct user *uptr;
	int flags;
#endif KADB

	if (masterprocp == 0)
		return;
#ifdef KADB
	flags = rwmap('r', (int)masterprocp + foffset, DSP, 0);
	if (errflg) {
		errflg = "cannot find proc";
		return;
	}
	if ((flags & SLOAD) == 0) {
		errflg = "process not in core";
		return;
	}
	uptr = (struct user *)rwmap('r', (int)masterprocp + uoffset, DSP, 0);
	if (errflg) {
		errflg = "cannot get u area";
		return;
	}
	(void) rwmap('r', (int)&uptr->u_pcb.pcb_regs, DSP, 0);
	if (errflg) {
		errflg = "cannot read pcb";
		return;
	}
	getpcb(&uptr->u_pcb.pcb_regs);
#else !KADB
	if (kvm_read(kvmd, (long)masterprocp, (char *)&proc, sizeof proc) !=
	    sizeof proc) {
		errflg = "cannot find proc";
		return;
	}
	if (proc.p_stat == SZOMB) {
		errflg = "zombie process - no u-area";
		return;
	}
	if ((uarea = kvm_getu(kvmd, &proc)) == NULL) {
		errflg = "cannot find u-area";
		return;
	}
	getpcb((label_t *)&uarea->u_pcb.pcb_regs);
#endif !KADB
}

/*
 * getpcb is called from ksetcor() and from getproc() when debugging in
 * kernel mode (-k), we read the pcb structure (and as many
 * registers as we can find) into the place where adb keeps the
 * appropriate registers.  The first time getpcb is called, the
 * argument is a pointer to the register list in the pcb structure
 * in the u-area.  The second time getpcb is called, the argument
 * is a pointer to the registers pulled out of panic_regs.  If these
 * are all zero, assume we are looking at a live kernel and ignore them.
 * Later, getpcb is called to read the registers of various mapped processes.
 *
 * The registers come from the label_t (see types.h) that begins a pcb.
 * The order of those registers is not published anywhere (a label_t is just
 * an array), but I've been told that the two registers are a PC and an SP.
 *
 * Unlike the 68k (whose label_t includes all of the registers), we must
 * then go find where the rest of the registers were saved.
 */
getpcb(regaddr)
	label_t *regaddr;
{
	register int i;
	register int *rp;
	int pcval, spval;


	rp = &(regaddr->val[0]);
	pcval = *rp++;
	spval = *rp++;
	if (pcval == 0 && spval == 0)
		return;
	setreg(REG_PC, pcval);
	setreg(REG_SP, spval);


	/*
	 * We have PC & SP; now we have to find the rest of the registers.
	 *  If in kernel mode (are we always?), the globals and current
	 *   out-registers are unreachable.
	 */
#ifndef KADB
	if ( kernel == 0 ) {
		errflg = "internal error -- getpcb not in kernel mode.";
	}
#endif KADB
	for (i = REG_G1; i <= REG_G7; ++i ) setreg( i, 0 );
	for (i = REG_O1; i <= REG_O5; ++i ) setreg( i, 0 );
	setreg( REG_O7, pcval);

	for (i = REG_L0; i <= REG_L7; ++i ) {
		setreg( i, rwmap( 'r', (spval + FR_LREG(i)), DSP, 0 ) );
	}
	for (i = REG_I0; i <= REG_I7; ++i ) {
		setreg( i, rwmap( 'r', (spval + FR_IREG(i)), DSP, 0 ) );
	}
}

#ifndef KADB

create(f)
	char *f;
{
	int fd;

	if ((fd = creat(f, 0644)) >= 0) {
		close(fd);
		return (open(f, wtflag));
	} else
		return (-1);
}

getfile(filnam, cnt)
	char *filnam;
{
	register int fsym;

	if (!strcmp(filnam, "-"))
	    return (-1);
	fsym = open(filnam, wtflag);
	if (fsym < 0 && xargc > cnt) {
	    if (wtflag) {
		fsym = create(filnam);
		if (fsym < 0) {
		    /* try reading only */
		    fsym = open(filnam, 0);
		    if (fsym >=0)
			printf("warning: `%s' read-only\n", filnam);
		}
	    }
	    if (fsym < 0)
		printf("cannot open `%s'\n", filnam);
	}
	return (fsym);
}
#endif !KADB

setvar()
{

	var[varchk('b')] = datbas;
	var[varchk('d')] = filhdr.a_data;
	var[varchk('e')] = filhdr.a_magic;
	var[varchk('m')] = filhdr.a_magic;
	var[varchk('s')] = stksiz;
	var[varchk('t')] = filhdr.a_text;
	var[varchk('F')] = 0;	/* There's never an fpa on a sparc */
}
