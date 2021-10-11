#ifndef lint
static	char sccsid[] = "@(#)kernel.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Support for Unix kernel debugging.
 */

#include "defs.h"
#include "machine.h"
#include "dbxmain.h"
#include "symbols.h"
#include "kernel.h"

#include <a.out.h>

#define BITS_CHAR	8				/* Bits is a char */
#define BITS_SHORT	(BITS_CHAR * sizeof(short))	/* Bits is a short */
#define BITS_INT	(BITS_CHAR * sizeof(int))	/* Bits in an int */

#define btop(addr)	((addr) >> pgshift)	/* Btye to page address */
#define ptob(addr)	((addr) << pgshift)	/* Page to byte address */
#define x_pte(fld)	extract_field(ptep, &pte, ord(fld))
#define x_proc(fld)	extract_field(procp, &proc, ord(fld))

#define n_members(mems)	(sizeof(mems)/sizeof(struct Field))

typedef struct Field *Field;
typedef struct Struct *Struct;

/*
 * Description of a field (member of a structure).
 */
struct Field {
	char *name;			/* Field name */
	int bit_start;			/* Bit offset to start of field */
	int bit_len;			/* Bit length of field */
};

/*
 * Description of a structure.
 */
struct Struct {
	char *name;			/* Structure tag */
	int size;			/* Size of the struct in bytes */
	Field fields;			/* Array of fields to look for */
	int nfields;			/* # of fields to look for */
};

typedef enum User_fields { USER_PCB, USER_COMM } User_fields;
typedef enum Proc_fields { PROC_PID, PROC_ADDR, PROC_FLAG } Proc_fields;
typedef enum Pte_fields { PTE_FOD, PTE_PFNUM, PTE_TYPE, PTE_V } Pte_fields;
typedef enum Pcb_fields { PCB_REGS } Pcb_fields;

private struct Field user_mems[] = {
	{ "u_pcb", 0, 0 },		/* USER_PCB */
	{ "u_comm", 0, 0 },		/* USER_COMM */
};

private struct Field proc_mems[] = {
	{ "p_pid", 0, 0 },		/* PROC_PID */
	{ "p_addr", 0, 0 },		/* PROC_ADDR */
	{ "p_flag", 0, 0 },		/* PROC_FLAG */
};

private struct Field pte_mems[] = {
	{ "pg_fod", 0, 0 },		/* PTE_FOD */
	{ "pg_pfnum", 0, 0 },		/* PTE_PFNUM */
	{ "pg_type", 0, 0 },		/* PTE_TYPE */
	{ "pg_v", 0, 0 },		/* PTE_V */
};

private struct Field pcb_mems[] = {
	{ "pcb_regs", 0, 0 },		/* PCB_REGS */
};

private struct Struct user = { "user", 0, user_mems, n_members(user_mems) };
private struct Struct proc = { "proc", 0, proc_mems, n_members(proc_mems) };
private struct Struct pte  = { "pte",  0, pte_mems,  n_members(pte_mems)  };
private struct Struct pcb  = { "pcb",  0, pcb_mems,  n_members(pcb_mems)  };

/*
 * Special symbols in the kernel that we need for kernel debugging.
 */
private	char Sysmap_nm[]      = "Sysmap";
private	char intstack_nm[]    = "intstack";
private	char eintstack_nm[]   = "eintstack";
private	char physmem_nm[]     = "physmem";
private	char panic_regs_nm[]  = "panic_regs";
private	char nproc_nm[]       = "nproc";
private	char proc_nm[]        = "proc";
private	char masterprocp_nm[] = "masterprocp";
private	char panicstr_nm[]    = "panicstr";

/*
 * Special variables for kernel debugging.
 */
private	int	pgshift;		/* Log2(# bytes/page) */
private int	pgoffset;		/* Mask for offset within a page */
private int	kernelbase;		/* ??? */
private int	nupages;		/* Number of pages within the u-area */
private int	vaddr_mask;		/* Virt addr mask */
private int	slr;			/* Total # of pages */
private int	physmem;		/* Amount of physical memory */
private int	nproc;			/* Number of elements in proc table */
private short	activepid;		/* Pid of the proc currently mapped */
private	int	sload;			/* Bit with proc.p_flag to test */
private int	u_pcb_regs_off;		/* Offset to u.u_pcb.pcb_regs */
private Address	panicstr;		/* Ptr to panic msg (if present) */
private Address intrstack_beg; 		/* Beginning of interrupt stack */
private Address intrstack_end; 		/* End of interrupt stack */
private Address	sbr;			/* Beginning of system page tables */
private Address masterprocp;		/* Ptr to proc struct mapped in */
private Address upage;			/* Page # of beginning of uarea */
private Address	umap;			/* Page maps for alternate uarea */
private Address proc_tbl;		/* Pointer to the proc table */
private Address	procp;			/* Pointer to a proc struct */
private Address	ptep;			/* Pointer to a pte struct */

/*
 * Description of special kernel absolute symbols.
 * Look for these symbols and retrieve their values.
 */
struct Knames {
	char	*name;			/* Name of the symbol */
	int	*valuep;		/* Ptr to where to store its value */
	Boolean	found;			/* Was this symbol found? */
};
typedef	struct	Knames	*Knames;

private	struct	Knames	knames[] = {
	{ "UPAGES_DEBUG", 	&nupages,		false },
	{ "_u", 		(int *) &upage,		false },
	{ "_Syssize", 		&slr,			false },
	{ "PGSHIFT_DEBUG", 	&pgshift,		false },
	{ "KERNELBASE_DEBUG",	&kernelbase,		false },
	{ "VADDR_MASK_DEBUG", 	&vaddr_mask,		false },
        { "SLOAD_DEBUG", 	&sload,			false },
        { NULL,	 		NULL,			false },
};

private Boolean	get_kernelpte();

/*
 * Get useful address, size, offset, etc from the kernel.
 */
public coredump_getkerinfo()
{
	Symbol s;

	check_syms();
	kernel_structs();
	upage = btop(upage & vaddr_mask);
	pgoffset = (1 << pgshift) - 1;
	sbr = (Address) (kernel_lookup(Sysmap_nm) - kernelbase);
	intrstack_beg = (Address) kernel_lookup(intstack_nm);
	intrstack_end = (Address) kernel_lookup(eintstack_nm);

	/* 
	 * Fake physmem so that the address translation to read physmem
	 * will work.  Setting it to the number of pages in 16Meg.
	 */
	physmem = btop(16*1024*1024);
	dread((char *) &physmem, kernel_lookup(physmem_nm), sizeof(physmem));
	dread((char *) &nproc, kernel_lookup(nproc_nm), sizeof(nproc));
	dread((char *) &proc_tbl, kernel_lookup(proc_nm), sizeof(proc_tbl));
	dread((char *) &panicstr, kernel_lookup(panicstr_nm), sizeof(panicstr));

	umap = (Address) malloc(nupages * pte.size);
	if (umap == 0)
		fatal("No memory available. Out of swap space?");

	/*
	 * If `masterprocp' exists in the kernel, use it to
	 * set up the process that was active.  Otherwise
	 * use the registers at `panic_regs'.
	 */
	s = lookup(identname(masterprocp_nm, true));
	if (s != nil) {
		dread((char *) &masterprocp, s->symvalue.offset, 
		    sizeof(masterprocp));
		if (masterprocp >= proc_tbl and 
		    masterprocp < proc_tbl + nproc*proc.size) {
    			dread((char *) procp, masterprocp, proc.size);
	    		proc_pte();
			activepid = x_proc(PROC_PID);
	    		printf("Active process is pid %d\n", activepid);
			pr_cmdname();
		} else {
	    		printf("Masterprocp is not valid - 0x%x\n", 
			    masterprocp);
	    		activepid = -1;
		}
    	} else {
		activepid = -1;
    	}
	if (panicstr != NULL) {
		getpcb((Address) kernel_lookup(panic_regs_nm));
	}
}

/*
 * Get registers from a specific address.
 * The program counter is first, followed by registers d2-d7,
 * then registers a2-a7.
 */
private getpcb(regaddr)
Address	regaddr;
{
    	int reg;

	regaddr = vmap(regaddr);
	fseek(corefile, regaddr, 0);
	setreg(REG_PC, getword(corefile));
#ifdef mc68000
	for (reg = R2; reg <= R7; reg++) {
		setreg(reg, getword(corefile));
	}
	for (reg = AR2; reg <= AR7; reg++) {
		setreg(reg, getword(corefile));
	}
#endif
}

/*
 * Read a long word (32-bits) and return it as an int.
 */
private getword(fp)
File	fp;
{
    	int i;

    	fread((char *) &i, sizeof(i), 1, fp);
    	return(i);
}
	
/*
 * Look up a symbol and check that it is found.
 * Return the value associated with the symbol.
 * Used for kernel debugging only.
 */
private int kernel_lookup(str)
char *str;
{
	Symbol s;
	Name n;

	n = identname(str, true);
	find (s, n) 
		where s->name == n and s->class == VAR
	endfind(s);
	if (s == nil) {
		error("can't find '%s'", str);
	}
	return(s->symvalue.offset);
}

/*
 * Map a kernel virtual address to a physical address
 */
public Address vmap(addr)
Address addr;
{
	int	pg_v;
	int	pg_fod;
	int	pg_pfnum;
	int	pg_type;

	if (get_kernelpte(addr, ptep) == false) {
		error("Invalid kernel address 0x%x", addr);
	}
	pg_v = x_pte(PTE_V);
	pg_fod = x_pte(PTE_FOD);
	pg_pfnum = x_pte(PTE_PFNUM);
	pg_type = x_pte(PTE_TYPE);
	if (pg_v == 0 and pg_fod or pg_pfnum == 0) {
		error("Page not valid/reclaimable, address 0x%x", addr);
	}
	if (pg_type == 0) {
		if (pg_pfnum > physmem) {
			error("Address 0x%x out of segment", addr);
		}
		return((Address) (ptob(pg_pfnum) + (addr & pgoffset)));
    	} else {
		error("Invalid page type, type %d address 0x%x", pg_type, addr);
    	}
    	/* NOTREACHED */
}

/*
 * Given a kernel address fill in a pte entry for it.
 * Return true if the pte entry was found, false otherwise.
 */
private Boolean get_kernelpte(addr, ptep)
Address	addr;
Address	ptep;
{
	int	v;
	int	ix;

	v = btop(addr & vaddr_mask);
	if (v >= upage and (v < (upage + nupages)) and masterprocp != nil) {
		ix = (v - upage) * pte.size;
		bcopy(umap + ix, ptep, pte.size);
	} else {
		if (addr >= kernelbase) {
			v = btop((addr - kernelbase) & vaddr_mask);
		} else {
			return(false);
		}
		if (v >= slr) {
			return(false);
		}
		fseek(corefile, sbr + v*pte.size, 0);
		fread((char *) ptep, pte.size, 1, corefile);
	}
	return(true);
}


/*
 * Setup for a different process.
 * We are given the pid of the process.
 * Find it in the proc table, and find its page tables for
 * the uarea.
 */
setproc(pid)
int pid;
{
	Address p;

	for (p = proc_tbl; p < proc_tbl + nproc*proc.size; p += proc.size) {
		dread((char *) procp, p, proc.size);
		if (x_proc(PROC_PID) == pid) {
 			break;
		}
	}
	if (p >= proc_tbl + nproc*proc.size) {
		error("No process id %d", pid);
	}
	if ((x_proc(PROC_FLAG) & sload) == 0) {
		error("Process %d not in core", pid);
	}
	proc_pte();
	activepid = pid;
	masterprocp = p;
	pr_cmdname();
}

/*
 * Return the pid of the process currently mapped into the uarea.
 */
public int get_proc()
{
    	return(activepid);
}

/*
 * Get the page tables for the uarea from a proc structure
 */
private proc_pte()
{
    	Address pte_ptr;
    	int i;

    	pte_ptr = (Address) x_proc(PROC_ADDR);
    	for (i = 0; i < nupages; i++) {
		dread((char *) umap + i*pte.size, pte_ptr, pte.size);
		pte_ptr += pte.size;
    	}
    	getpcb((Address) ptob(upage) + u_pcb_regs_off);
}

/*
 * Will an address map properly.
 */
public Boolean addr_ok(addr)
Address addr;
{
    	if (get_kernelpte(addr, ptep) == false) {
		return(false);
    	}
    	if (x_pte(PTE_V) == 0 and (x_pte(PTE_FOD) or x_pte(PTE_PFNUM) == 0)) {
		return(false);
    	}
    	if (x_pte(PTE_TYPE) == 0 and x_pte(PTE_PFNUM) <= physmem) {
		return(true);
    	} else {
		return(false);
    	}
}

/*
 * Is an address a legitimate kernel frame pointer.
 * There are two possible kernel frames, the user-level and the
 * interrupt level.  The user-level stack lives in the uarea
 * and the interrupt stack lives between "intrstack_beg" and
 * "intrstack_end".
 */
public Boolean kernel_frame(addr)
Address addr;
{
    	int v;

    	v = btop(addr & vaddr_mask);
    	if (v >= upage and v < (upage + nupages) or
      	    addr >= intrstack_beg and addr < intrstack_end) {
		return(true);
    	}
    	return(false);
}

/*
 * Get the pertinent info about some of the kernel structures.
 * This includes the size of the structure, and the offset and size
 * of some of its members.
 */
public kernel_structs()
{
	fill_in(&user, NULL);
	fill_in(&pcb, NULL);
	fill_in(&proc, &procp);
	fill_in(&pte, &ptep);
	insert_u(upage);
	u_pcb_regs_off = (user.fields[ord(USER_PCB)].bit_start + 
	    pcb.fields[ord(PCB_REGS)].bit_start) / BITS_CHAR;
}

/*
 * Find a description of a structure and search for certain of its
 * members.  Finally, allocate an instance of one.
 */
private fill_in(record, ptr)
Struct record;
Address *ptr;
{
	Symbol s;
	Symbol f;
	Name name;
	Field field;
	int nfields;

	name = identname(record->name, true);
	find (s, name) 
		where s->class == TYPEID
	endfind(s);
	if (s == nil) {
		error("Could not find struct %s - cannot proceed with kernel debugging",
		    idnt(name));
	}
	s = s->type;
	record->size = s->symvalue.offset;
	nfields = record->nfields;
	for (f = s->chain; f != nil; f = f->chain) {
		for (field = record->fields; field < &record->fields[nfields];
		  field++) {
			if (streq(idnt(f->name), field->name)) {
				field->bit_start = f->symvalue.field.offset;
				field->bit_len = f->symvalue.field.length;
				break;
			}
		}
	}
	for (field = record->fields; field < &record->fields[nfields]; 
	  field++) {
		if (field->bit_len == 0) {
			error("Could not find field `%s' in struct `%s'",
			  field->name, record->name);
		}
	}
	if (ptr != NULL) {
		*ptr = (Address) malloc(record->size);
		if (*ptr == 0)
			fatal("No memory available. Out of swap space?");
	}
}

/*
 * Fake up a symbol for 'u' that is of type 'struct user'.
 */
private insert_u(addr)
Address addr;
{
	Symbol	usym;
	Symbol	s;
	Name	uname;
	Name	name;

	uname = identname("u", true);
	usym = insert(uname);
	name = identname(user.name, true);
	find (s, name) 
		where s->class == TYPEID
	endfind(s);
	usym->type = s;
	usym->language = findlanguage(".c");
	usym->class = VAR;
	usym->symvalue.offset = addr;
	usym->level = program->level;
	usym->block = program;
}

/*
 * Look for some kernel debugging symbols.
 * This is just a hard-coded list of names whose values 
 * are assigned to some variables that are local to this file.
 */
public kernel_abs(name, np)
char *name;
struct nlist *np;
{
	Knames	kp;
	Name	n;

	for (kp = knames; kp->name != NULL; kp++) {
		if (streq(name, kp->name)) {
			*kp->valuep = np->n_value;
			kp->found = true;
			return;
		}
	}
	if (name[0] == '_') {
		name = &name[1];
	}
	n = identname(name, true);
	check_var(np, n);
}

/*
 * See that all the necessary absolute kernel symbols where found.
 */
private	check_syms()
{
	Knames kp;

	for (kp = knames; kp->name != NULL; kp++) {
		if (kp->found == false) {
			error("Did not find symbol `%s'", kp->name);
		}
	}
}

/*
 * Given a pointer to structure, a description of the structure,
 * and an index to a particular field, extract and return the 
 * field.  Bitfields complicate things here.
 */
private int extract_field(ptr, record, index)
Address	ptr;
Struct	record;
int	index;
{
	Field	field;
	char	*cp;
	short	*sp;
	int	*ip;
	int	r;


	if (index > record->nfields) {
		error("Index %d too large in extract_field", index);
	}
	r = 0;
	field = &record->fields[index];
	if ((field->bit_start % BITS_CHAR) == 0) {
		cp = (char *) (ptr + field->bit_start / BITS_CHAR);
		switch (field->bit_len) {
		case BITS_CHAR: 
			r = *cp;
			break;

		case BITS_SHORT: 
			sp = (short *) cp;
			r = *sp;
			break;
		case BITS_INT: 
			ip = (int *) cp;
			r = *ip;
			break;
		default:
			r = bitfield(ptr, field);
			break;
		}
	} else {
		r = bitfield(ptr, field);
	}
	return(r);
}

/*
 * Extract a bitfield.
 * Find the word containing the bitfield and read it.
 * Then shift and mask to extract the bitfield.
 */
private int bitfield(ptr, field)
Address ptr;
Field	field;
{
	int	word_ix;
	int	bit_start;
	int	word;
	int	mask;
	int	*ip;

	word_ix = field->bit_start / BITS_INT;
	bit_start = field->bit_start % BITS_INT;
	ip = ((int *) ptr) + word_ix;
	word = *ip;
	mask = (1 << field->bit_len) - 1;
#ifdef i386
	word >>= bit_start;
#else
	word = word >> (BITS_INT - (bit_start + field->bit_len));
#endif
	word &= mask;
	return(word);
}

/*
 * Print the name of a command from the uarea.
 */
pr_cmdname()
{
	Address addr;
	int	stringlen;

	addr = (Address) 
	    (ptob(upage) + user.fields[ord(USER_COMM)].bit_start/BITS_CHAR);
	printf("command name is `");
	stringlen = get_stringlen();
	set_stringlen(100);
	pr_string(addr);
	set_stringlen(stringlen);
	printf("'\n");
}
