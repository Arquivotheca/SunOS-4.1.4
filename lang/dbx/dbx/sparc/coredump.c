#ifndef lint
static	char sccsid[] = "@(#)coredump.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Deal with the core dump anachronism.
 */

#include "defs.h"
#include "coredump.h"
#include "kernel.h"
#include "machine.h"
#include "symbols.h"
#include "object.h"
#include "dbxmain.h"
#include "getshlib.h"
#include "process.h"
#include <sys/param.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <sys/core.h>
#include <sys/user.h>
#include <sys/vm.h>
#include <sys/ptrace.h>
#include <a.out.h>

#define	SEGRND(x)	((N_SEGSIZ(x))-1)

#ifndef public

#include "machine.h"
#endif

public unsigned int	maxstkaddr = 0;

typedef struct map{
    Address begin;
    Address end;
    Address seekaddr;
	File    file;
	struct map *next;
} Map;

private Map datamap, stkmap, txtmap;
private File objfile;
private struct exec hdr;
private Boolean core_matches();

/*
 * Read the user area information from the core dump.
 */

#include <unistd.h>
#define pcb_offset(ptr) ( (char *)&(ptr->u_pcb) - (char *)ptr )


public coredump_readin(reg, signo, sigcode, ppcb, pfpu)
struct regs *reg;
short *signo, *sigcode;
struct pcb *ppcb;
struct fpu *pfpu;
{
    struct core core;
    struct user *up;
    unsigned long pcbloc, goback;

    if (maxstkaddr == 0) {
	/*
	 * core files contain the length of the
	 * stack segment, but no indication of
	 * the starting or ending address, and
	 * this addres might change from system
	 * to system.
	 */
	extern char **environ;
	char **epp = environ;
	char *les = (char *)environ;
	char *ep;
	while ((ep = *epp++) != (char *)0) {
	    if (ep > les) {
		les = ep;
	    }
	}
	while (*les != '\0') {
	    les ++;
	}
	maxstkaddr = (((unsigned)les)|(getpagesize()-1)) + 1;
    }

    objfile = fopen(objname, "r");
    if (objfile == nil) {
	fatal("can't read \"%s\"", objname);
    }
    get(objfile, hdr);
    if (kernel) {
	datamap.begin = 0;
	datamap.end = 0xffffffff;
	stkmap.begin = 0xffffffff;
	stkmap.end = 0xffffffff;
    } else {
        fread((char *) &core, sizeof(core), 1, corefile);
        if (not core_matches(&hdr, &core.c_aouthdr)) {
    	    warning("core file does not match object file:\ncore file ignored");
	    coredump = false;
	    fclose(corefile);
	    fclose(objfile);
	    return;
	}
	*reg = core.c_regs;
    *signo = core.c_signo;
    *sigcode = core.c_ucode;
	/* seek & read in ppcb after verifying the core file */
	*pfpu = core.c_fpu;

        datamap.begin = 0;
        datamap.seekaddr = core.c_len;
        stkmap.begin = maxstkaddr - core.c_ssize;
        stkmap.end = maxstkaddr;
        stkmap.seekaddr = datamap.seekaddr + core.c_dsize;
        txtmap.begin = N_TXTADDR(hdr);
		txtmap.end = N_TXTADDR(hdr) + core.c_tsize;
		txtmap.seekaddr = N_TXTOFF(hdr);
		txtmap.file = objfile;
        switch (hdr.a_magic) {
        case OMAGIC:
#ifdef sun
	    datamap.begin = N_TXTADDR(hdr);
	    datamap.end = datamap.begin + core.c_tsize + core.c_dsize;
#else ifdef vax
	    datamap.begin = 0;
	    datamap.end = core.c_tsize + core.c_dsize;
#endif
	    break;

	case NMAGIC:
	case ZMAGIC:
#ifdef sun
	    datamap.begin =
	      (Address) ((N_TXTADDR(hdr) + core.c_tsize + SEGRND(hdr)) & ~SEGRND(hdr));
#else ifdef vax
	    datamap.begin = (Address) ptob(btop(core.c_tsize) - 1) + 1);
#endif
	    datamap.end = datamap.begin + core.c_dsize;
	    break;

	default:
	    fatal("bad magic number 0x%x", hdr.a_magic);
        }
        /*
         * Core dump not from this object file?
         */
        if (hdr.a_magic != 0 and core.c_aouthdr.a_magic != 0 and
          hdr.a_magic != core.c_aouthdr.a_magic) {
	    warning("core dump ignored");
	    coredump = false;
	    fclose(corefile);
	    fclose(objfile);
	    start(nil, nil, nil, nil);
        }

	pcbloc = core.c_len + core.c_dsize + core.c_ssize + pcb_offset( up );

	goback = fseek( corefile, pcbloc, SEEK_SET ) ;
	if(   goback == -1
	   || fread((char *) ppcb, sizeof(*ppcb), 1, corefile) != 1 ) {
    	    warning("cannot read pcb in core file:  registers' values may be wrong");
	}

	/*
	 * For safety's sake, leave file ptr at start of data segment
	 * (end of core structure), just like it used to be.
	 */
	fseek( corefile, goback, SEEK_SET );
    }
}

/*
 * See if the core dump matches the object file.
 * Do some funky comparisions of the magic numbers, text size
 * and data size.  If the magic number is 407 (impure text)
 * then the text size in the core file is zero, because the kernel
 * thinks the whole process is data space.
 */
private Boolean core_matches(progp, corep)
struct exec *progp;
struct exec *corep;
{
    long data407;

    if (progp->a_magic != corep->a_magic) {
	return(false);
    }
    if (progp->a_magic == OMAGIC) {
	data407 = progp->a_text + progp->a_data;
	if (corep->a_text != 0 or corep->a_data != data407) {
	    return(false);
	}
    } else {
	if (corep->a_text != progp->a_text or corep->a_data != progp->a_data) {
	    return(false);
	}
    }
    return(true);
}

public coredump_close()
{
	 Map *p, *tmp;
	  
	fclose(objfile);
	for(p=txtmap.next; p; p=tmp) {
		fclose(p->file);
		tmp = p->next;
		free(p);
	}
}

private Address ok_text_addr(addr)
Address addr;
{
	Map *map, *p;
	for(p=&txtmap;p;p=p->next) {
		if(addr >= p->begin && addr < p->end) {
    	    if(fseek(p->file, p->seekaddr + addr - p->begin, 0) ==0 ) {
				return 1;
			} else {
				return 0;
			}
		}
	}
	return 0;
}

public add_txt_map(fn,b,e,f)
char *fn;
int b,e,f;
{
	Map *map, *p;

	map = (Map *)  malloc(sizeof(struct map));
	if (map == 0)
		fatal("No memory available. Out of swap space?");
	if( (map->file = fopen(fn,"r")) == NULL) {
		error("cannot open shared library %s", fn);
	}
	map->begin = b;
	map->end = e;
	map->seekaddr = f;
	map->next = 0;

	for(p=&txtmap;p->next;p=p->next);
	p->next = map;
}

public coredump_readtext(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    if (hdr.a_magic == OMAGIC or kernel) {
	coredump_readdata(buff, addr, nbytes);
    } else {
#ifdef sun
	if(ok_text_addr(addr)) {
		fread(buff, nbytes, sizeof(Byte), objfile);
	} else {
		*(int *) buff = 0;
		read_err_flag = 1;
		warning("object file read error: text address not found");
	}
#else ifdef vax
	fseek(objfile, N_TXTOFF(hdr) + addr, 0);
	fread(buff, nbytes, sizeof(Byte), objfile);
#endif
    }
}

public coredump_readdata(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    if (addr < datamap.begin) {
	coredump_readtext(buff, addr, nbytes);
    } else if (addr >= stkmap.end) {
	warning("core file read error: data space address too high");
	read_err_flag = 1;
        buff[0] = '\0';
    } else {
	if (kernel) {
	    addr = vmap(addr);
	}
	if (addr >= stkmap.begin) {
    	    fseek(corefile, stkmap.seekaddr + addr - stkmap.begin, 0);
    	    fread(buff, nbytes, sizeof(Byte), corefile);
        } else if (addr < datamap.end) {
    	    fseek(corefile, datamap.seekaddr + addr - datamap.begin, 0);
    	    fread(buff, nbytes, sizeof(Byte), corefile);
        } else {
	    warning("core file read error: address not in data space");
	    read_err_flag = 1;
	    buff[0] = '\0';
	}
    }
}

/*
 * See if an address is within the address space.
 * This really only applies to corefiles.  This check
 * is to prevent the "text/data space address too high/low"
 * messages when trying to print "char *" vars that have bogus
 * values in them.
 */
public Boolean in_address_space(addr)
Address addr;
{
    if (coredump) {
	if (addr < N_TXTADDR(hdr)) {
	    return(false);
	} else if ((addr > datamap.end) and (addr < stkmap.begin)) {
	    return(false);
	} else if (addr > stkmap.end) {
	    return(false);
	} else {
	    return(true);
	}
    } else {
	return(true);
    }
}
