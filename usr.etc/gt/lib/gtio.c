#ifndef lint
static char sccsid[] = "@(#)gtio.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <mon/idprom.h>
#include <nlist.h>

#include <sys/ioctl.h>

char *coff_gettext();
char *coff_getdata();
char *aout_gettext();
char *aout_getdata();


char *
gt_gettext(fp, ts, tl)
	FILE *fp;
	int *ts, *tl;
{
	if (coff_iscoff(fp))
		return (coff_gettext(fp, ts, tl));
	else
		return (aout_gettext(fp, ts, tl));
}


char *
gt_getdata(fp, ds, dl)
	FILE *fp;
	int *ds, *dl;
{
	if (coff_iscoff(fp))
		return (coff_getdata(fp, ds, dl));
	else
		return (aout_getdata(fp, ds, dl));
}


gt_getbss(fp)
	FILE *fp;
{
	if (coff_iscoff(fp))
		return (coff_getbss(fp));
	else
		return (aout_getbss(fp));
}


gt_getentry(fp)
	FILE *fp;
{
	if (coff_iscoff(fp))
		return (coff_getentry(fp));
	else
		return (aout_getentry(fp));
}


nlist(filename, nl)
	char *filename;
	struct nlist *nl;
{
	FILE *fp;
	int error = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return (-1);

	if (coff_iscoff(fp))
		error = coff__nlist(fp, nl);
	else
		error = _nlist(fileno(fp), nl);

	fclose(fp);

	return (error);
}
