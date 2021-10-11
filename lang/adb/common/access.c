#ifndef lint
static  char sccsid[] = "@(#)access.c 1.1 94/10/31 SMI"; /* from UCB X.X XX/XX/XX */
#endif
/*
 * adb: rwmap data in file/process address space.
 */

#include "adb.h"
#include <sys/ptrace.h>

put(addr, space, value) 
	addr_t addr;
	int space, value;
{

	(void) rwmap('w', addr, space, value);
}

get(addr, space)
	addr_t addr;
	int space;
{

	return (rwmap('r', addr, space, 0));
}

chkget(addr, space)
	addr_t addr;
	int space;
{
int w;
	w = get(addr, space);

	chkerr();
	return (w);
}

bchkget(addr, space) 
	addr_t addr;
	int space;
{
	long val = chkget(addr, space);

	/* try to be machine independent -- big end and little end machines */
	return (*(char *)&val);
}

rwmap(mode, addr, space, value)
	char mode;
	addr_t addr;
	int space, value;
{
	int file, w;

	if (space == NSP) {
		return (0);
	}
	if (pid) {
		if (mode == 'r') {
			if (readproc(addr, (char *)&value, 4) != 4) {
				rwerr(space);
			}
			return (value);
		}
		if (writeproc(addr, (char *)&value, 4) != 4) {
			rwerr(space);
		}
		return (value);
	}
	w = 0;
	if (mode == 'w' && wtflag == 0)
		error("not in write mode");
	if (!chkmap(&addr, space, &file)) {
		return (0);
	}

#ifndef KADB
	if (kernel && space == DSP) {
		if ((mode == 'r') ? kread(addr, &w) : kwrite(addr, &value))
			rwerr(space);
		return (w);
	}
#endif !KADB
	if (rwphys(file, addr, mode == 'r' ? &w : &value, mode) < 0) {
		rwerr(space);
	}
	return (w);
}


static
rwerr(space)
	int space;
{

	if (space & DSP)
		errflg = "data address not found";
	else
		errflg = "text address not found";
}

rwphys(file, addr, aw, rw)
	int file;
	unsigned addr;
	int *aw, rw;
{
	int rc;

#ifdef notdef
	if (kernel)
		addr = KVTOPH(addr);
#endif notdef
	if (fileseek(file, addr)==0)
		return (-1);
	if (rw == 'r') {
		rc = read(file, (char *)aw, sizeof (int));
	} else {
		rc = write(file, (char *)aw, sizeof (int));
	}
	if (rc != sizeof (int)) {
		return (-1);
	}
	return (0);
}

/* check that addr can be mapped to a file offset and if so set the
** offset and fd;  the first map range that includes the address is
** used - except that if the SPACE is ?* or /* the first (map_head) 
** range is bypassed - this continues support for using ?* or /*
** to explicitly specify use of the (b2,e2,f2) triple for mapping
*/
static
chkmap(addr, space, fdp)
	register addr_t *addr;
	int space;
	int *fdp;
{
	register struct map *amap;
	register struct map_range *mpr;

	amap = (space&DSP) ? &datmap : &txtmap;
	for(mpr = amap->map_head; mpr; mpr= mpr->mpr_next){
		if(mpr == amap->map_head && space&STAR)
			continue;
		if(within(*addr, mpr->mpr_b, mpr->mpr_e)) {
			*addr += mpr->mpr_f - mpr->mpr_b;
			*fdp = mpr->mpr_fd; 
			return (1);
		}
	}

	rwerr(space);
	return (0);
}

static
within(addr, lbd, ubd)
	addr_t addr, lbd, ubd;
{

	return (addr >= lbd && addr < ubd);
}

static
fileseek(f, a)
	int f;
	addr_t a;
{

	return (lseek(f, (long)a, 0) != -1);
}


/*
 * VAX UNIX can read and write data from the user process at odd addresses.
 * 68000 UNIX does not have this capability, so we must attempt to ensure
 * even addresses.  The 68000 has the related problem of not being able
 * to do word accesses to memory at odd addresses, so we must be careful.
 *
 * On a Sparc, it's even worse.  All accesses must be on a 32-bit
 * boundary.  These routines sort of allow 68000s and sparcs to
 * pretend that they're vaxen (only as far as word/long boundaries
 * are concerned, not w.r.t. byte order).
 */

writeproc(a, buf, len0)
	int a;
	char *buf;
	int len0;
{
	int len = len0;
	int  count;
	unsigned long val;
	char *writeval;

	errno = 0;

	if (len0 <= 0) {
		return len0;
	}

#ifdef sparc
	if (a & 03) {
	  int  ard32;		/* a rounded down to 32-bit boundary */
	  unsigned short *sh;

		/* We must align to a four-byte boundary */
		ard32 = a & ~3;
		(void) readproc( ard32, &val, 4);
		switch( a&3 ) {
		 case 1: /* Must insert a byte and a short */
			*((char *)&val + 1) = *buf++;	/* insert byte */
			--len;  ++a;
			/* FALL THROUGH to insert short */

		 case 2: /* insert short-word */
			*((char *)&val + 2) = *buf++;	/* insert byte */
			*((char *)&val + 3) = *buf++;	/* insert byte */
			len -= 2; a += 2;
			break;

		 case 3: /* insert byte -- big or little end machines */
			*((char *)&val +3) = *buf++;
			--len;  ++a;
			break;
		}
		/*
		 * to do shared library, we are not allowed to
		 * change the permit of data, never POKEDATA
		 * (void) ptrace(a < txtmap.e1 ? PTRACE_POKETEXT : 
		 * PTRACE_POKEDATA, pid, ard32, val);
		 */
		(void) ptrace(PTRACE_POKETEXT, pid, ard32, val);
	}
#else !sparc
        if (a & 01) {
		/* Only need to align to two bytes */
                /* here for stupid version of ptrace */
                (void) readproc(a & ~01, &val, 4);
                /* insert byte -- big or little end machines */
                *((char *)&val + 1) = *buf++;

		/*
		 * to do shared library, we are not allowed to
		 * change the permit of data, never POKEDATA
                 * (void) ptrace(a < txtmap.e1 ? PTRACE_POKETEXT :
                 * PTRACE_POKEDATA, pid, a & ~01, val);
		 */
		(void) ptrace(PTRACE_POKETEXT, pid, a & ~01, val);
		--len;  ++a;
        }

#endif !sparc

	while (len > 0) {
		if (len < 4) {
			(void) readproc(a, (char *)&val, 4);
			count = len;
		} else {
			count = 4;
		}
		writeval = (char *)&val;
		while (count--) {
			*writeval++ = *buf++;
		}
		/*
		 * to do shared library, we are not allowed to
		 * change the permit of data, never POKEDATA
		 * (void) ptrace(a < txtmap.e1 ? PTRACE_POKETEXT : 
		 * PTRACE_POKEDATA, pid, a, val);
		 */
		(void) ptrace(PTRACE_POKETEXT, pid, a, val);
		len -= 4; a += 4;
	}
	if (errno) {
		return (0);
	}
	return (len0);
}

readproc(a, buf, len0)
	int a;
	char *buf;
	int len0;
{
	int len = len0;
	int count;
	char *readchar;
	unsigned val;

	errno = 0;

	if (len0 <= 0) {
		return len0;
	}

#ifdef sparc
	if (a & 03) {
	  unsigned short *sh;
		/* We must align to a four-byte boundary */
		val = ptrace(a < txtmap.map_head->mpr_e ? PTRACE_PEEKTEXT : PTRACE_PEEKDATA,
		    pid, a & ~3, 0);
		switch( a&3 ) {
		 case 1: /* Must insert a byte and a short */
			*buf++ = *((char *)&val + 1);
			--len;  ++a;

			/* Fall through to handle the short */

		 case 2: /* insert two bytes */
			*buf++ = *((char *)&val + 2);
			*buf++ = *((char *)&val + 3);
			len -= 2; a += 2;
			break;

		 case 3: /* insert byte -- big or little end machines */
			*buf++ = *((char *)&val + 3);
			--len; ++a;
			break;
		}
	}
#else !sparc

	/* Only need to align to two bytes */
	if (a & 01) {
		/* here for stupid ptrace that cannot grot odd addresses */
		val = ptrace(a < txtmap.map_head->mpr_e ? PTRACE_PEEKTEXT : PTRACE_PEEKDATA,
		    pid, a & ~01, 0);
		/* technically, should handle big & little end machines */
		*(buf++) = *((char*)&val + 1);
		--len; ++a;
	}
#endif !sparc

	while (len > 0) {
		val = ptrace(a < txtmap.map_head->mpr_e ? PTRACE_PEEKTEXT : PTRACE_PEEKDATA,
		    pid, a, 0);
		readchar = (char *)&val;
		if (len < 4) {
			count = len;
		} else {
			count = 4;
		}
		while (count--) {
			*buf++ = *readchar++;
		}
		len -= 4; a += 4;
	}
	if (errno) {
		return (0);
	}
	return (len0);
}

exitproc()
{

#ifndef KADB
	(void) ptrace(PTRACE_KILL, pid, 0, 0);
	pid = 0;
#endif !KADB
}

#ifndef KADB
/* allocate a new map_range; fill in fields and add it to the map so that
** "ranges are stored in increasing starting value (mpr_b)" remains true
*/
#define INFINITE 0x7fffffff
struct map_range *
add_map_range(map, start, length, offset, file_name)
struct map *map;
int start, length, offset;
char *file_name;
{
	struct map_range *mpr;

	mpr = (struct map_range*) calloc(1,sizeof(struct map_range));
	mpr->mpr_fd = getfile(file_name, INFINITE);
	mpr->mpr_fn = file_name;
	mpr->mpr_b = start;
	mpr->mpr_e = length;
	mpr->mpr_f = offset;
	map->map_tail->mpr_next = mpr;
	map->map_tail = mpr;
}

/* free any ranges which may refer to shared libraries
**  -the first two ranges are tied to corfil and symfil
*/
free_shlib_map_ranges(map)
struct map *map;
{
	struct map_range *mpr, *tmp;

	if( map->map_head && map->map_head->mpr_next) {
		map->map_tail = map->map_head->mpr_next;
		for(mpr = map->map_tail->mpr_next; mpr; mpr =  tmp) {
			if(mpr->mpr_fd)
				close(mpr->mpr_fd);
			tmp = mpr->mpr_next;
			free(mpr);
		}
		map->map_tail->mpr_next = 0;
	} else { /* corrupt range list..*/
		map->map_tail = map->map_head;
	}
}
#endif !KADB
