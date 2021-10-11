/*	@(#)process.h 1.1 94/10/31 SMI	*/

addr_t	userpc;
struct	pte *sbr;
int	kernel;
int	kcore;
int	slr;
#ifdef vax
int	masterpcbb;
struct	pcb pcb;
#endif
#ifdef sun
int	masterprocp;
struct pte umap[UPAGES];
unsigned upage;
int	physmem;
#endif
int	dot;
int	dotinc;
int	pid;
int	executing;
int	fcor;
int	fsym;
int	signo;
int	sigcode;

struct	exec filhdr;

#ifndef KADB
union {
	struct	user U;
	char	UU[ctob(UPAGES)];
} udot;
#define	u	udot.U
#endif !KADB

char	*corfil, *symfil;

/* file address maps : there are two maps, ? and /. Each consists of a sequence
** of ranges. If mpr_b <= address <= mpr_e the f(address) = file_offset
** where f(x) = x + (mpr_f-mpr_b). mpr_fn and mpr_fd identify the file.
** the first 2 ranges are always present - additional ranges are added
** if inspection of a core file indicates that some of the program text
** is in shared libraries - one range per lib.
*/
struct map_range {
	int					mpr_b,mpr_e,mpr_f;
	char				*mpr_fn;
	int					mpr_fd;
	struct map_range	*mpr_next;
};

struct map {
	struct map_range	*map_head, *map_tail;
} txtmap, datmap;

#define BKPTSET		0x1
#define BKPT_ERR	0x8000		/* breakpoint not successfully set */

#define MAXCOM	64

struct	bkpt {
	addr_t	loc;
	addr_t	ins;
	int	count;
	int	initcnt;
	int	flag;
	char	comm[MAXCOM];
	struct	bkpt *nxtbkpt;
};

struct	bkpt *bkptlookup();
