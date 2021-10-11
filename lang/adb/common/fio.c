#ifndef lint
static	char sccsid[] = "@(#)fio.c 1.1 94/10/31 SMI";
#endif

/*
 * adb - source file access routines
 */

#include "adb.h"
#include <stdio.h>
#include "fio.h"

fenter(name)
	char *name;
{
	register struct file *fi;

	if (fi = fget(name))
		return (fi - file + 1);
	if (NFILE == 0) {
		NFILE = 10;
		file = (struct file *)malloc(NFILE * sizeof (struct file));
		filenfile = file+nfile;
	}
	if (nfile == NFILE) {
		NFILE *= 2;
		file = (struct file *)
		    realloc((char *)file, NFILE*sizeof (struct file));
		filenfile = file+nfile;
	}
	fi = &file[nfile++];
	filenfile++;
	fi->f_name = savestr(name);
	fi->f_flags = 0;
	fi->f_nlines = 0;
	fi->f_lines = 0;
	return (nfile);
}

struct file *
fget(name)
	char *name;
{
	register struct file *fi;

	for (fi = file; fi < filenfile; fi++)
		if (!strcmp(fi->f_name, name))
			return (fi);
	return (0);
}

#ifdef notdef
static
findex(name)
	char *name;
{
	register struct file *fi;
	int index = 0;

	for (fi = file, index = 1; fi < filenfile; fi++, index++)
		if (!strcmp(fi->f_name, name))
			return (index);
	return (0);
}
#endif

struct file *
indexf(index)
	unsigned index;
{

	index--;
	if (index >= 0 && index < nfile)
		return (&file[index]);
	return (NULL);
}

finit(fi)
	struct file *fi;
{
#ifndef KADB
	char buf[BUFSIZ];
	register off_t *p;

	if (openfile == fi && OPENFILE)
		return;
	/* IT WOULD BE BETTER TO HAVE A COUPLE OF FILE DESCRIPTORS, LRU */
	if (OPENFILE) {
		(void) fclose(OPENFILE);
		OPENFILE = NULL;
	}
	if ((OPENFILE = fopen(fi->f_name, "r")) == NULL)
		error("can't open file");
	openfile = fi;
	setbuf(OPENFILE, filebuf);
	openfoff = -BUFSIZ;		/* putatively illegal */
	if (fi->f_flags & F_INDEXED)
		return;
	fi->f_flags |= F_INDEXED;
	p = fi->f_lines = (off_t *)malloc(101 * sizeof (off_t));
	*p++ = 0;		/* line 1 starts at 0 ... */
	fi->f_nlines = 0;
	/* IT WOULD PROBABLY BE FASTER TO JUST USE GETC AND LOOK FOR \n */
	while (fgets(buf, sizeof buf, OPENFILE)) {
		if ((++fi->f_nlines % 100) == 0)
			openfile->f_lines =
			    (off_t *)realloc((char *)openfile->f_lines,
			     (openfile->f_nlines+101) * sizeof (off_t));
		p[0] = p[-1] + strlen(buf);
		p++;
	}
#else
	error("can't open file");
#endif KADB
}

printfiles()
{
	/* implement $f command */
	register struct file *f;
	register int i;

	for (f = file, i = 1; f < filenfile; f++, i++) {
		printf("%d	%s\n", i, f->f_name);
	}
}

getline(file, line)
	int file, line;
{
	register struct file *fi;
	register off_t *op;
	int o, n;

	fi = indexf(file);
	finit(fi);
#ifndef KADB
	if (line == 0) {
		(void) sprintf(linebuf, "%s has %ld lines\n",
		    fi->f_name, fi->f_nlines);
		return;
	}
	if (line < 0 || line > fi->f_nlines)
		errflg = "line number out of range";
	op = &fi->f_lines[line-1];
	n = op[1] - op[0];
	linebuf[n] = 0;
	if (*op >= openfoff && *op < openfoff + BUFSIZ) {
case1:
		o = op[0] - openfoff;
		if (o + n > BUFSIZ) {
			strncpy(linebuf, filebuf+o, BUFSIZ-o);
			openfoff += BUFSIZ;
			(void) read(fileno(OPENFILE), filebuf, BUFSIZ);
			strncpy(linebuf+BUFSIZ-o, filebuf, n-(BUFSIZ-o));
		} else
			strncpy(linebuf, filebuf+o, n);
		return;
	}
	if (op[1]-1 >= openfoff && op[1] <= openfoff+BUFSIZ) {
		o = op[1] - openfoff;
		strncpy(linebuf+n-o, filebuf, o);
		(void) lseek(fileno(OPENFILE), (long)openfoff, 0);
		(void) read(fileno(OPENFILE), filebuf, BUFSIZ);
		strncpy(linebuf, filebuf+op[0]-openfoff, n-o);
		return;
	}
	openfoff = (op[0] / BUFSIZ) * BUFSIZ;
	(void) lseek(fileno(OPENFILE), (long)openfoff, 0);
	(void) read(fileno(OPENFILE), filebuf, BUFSIZ);
	goto case1;
#endif !KADB
}

static char *
savestr(cp)
	char *cp;
{
	char *dp = (char *)malloc(strlen(cp) + 1);

	(void) strcpy(dp, cp);
	return (dp);
}
