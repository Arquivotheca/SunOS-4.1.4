#ifndef lint
static  char sccsid[] = "@(#)coff.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <filehdr.h>
#include <n10aouth.h>
#include <scnhdr.h>
#include <reloc.h>
#include <linenum.h>
#include <storclass.h>
#include <syms.h>


/*
 * XXX: problem with symbol table if -g is on
 */
char *
coff_gettext(fp, size, laddr)
	FILE *fp;
	int *size, *laddr;
{
	struct filehdr hc;
	struct scnhdr ts;
	N10AOUTHDR aouthdr;
	char *p;
	int cc;

	rewind(fp);
	fread(&hc, 1, sizeof(struct filehdr), fp);
	fread(&aouthdr, 1, sizeof(N10AOUTHDR), fp);
	fread(&ts, 1, sizeof(struct scnhdr), fp);

	fseek(fp, ts.s_scnptr, 0);

	p = (char *)valloc(ts.s_size);

	if ((cc = fread(p, 1, ts.s_size, fp)) != ts.s_size)
		return (NULL);

	if (size)
		*size = ts.s_size;
	if (laddr)
		*laddr = aouthdr.text_start;

	return (p);
}


char *
coff_getdata(fp, size, laddr)
	FILE *fp;
	int *size, *laddr;
{
	struct filehdr hc;
	struct scnhdr ts,ds;
	N10AOUTHDR aouthdr;
	char *p;
	int cc;

	rewind(fp);
	fread(&hc, 1, sizeof(struct filehdr), fp);
	fread(&aouthdr, 1, sizeof(N10AOUTHDR), fp);
	fread(&ts, 1, sizeof(struct scnhdr), fp);
	fread(&ds, 1, sizeof(struct scnhdr), fp);

	fseek(fp, ds.s_scnptr, 0);

	p = (char *)valloc(ds.s_size);

	if ((cc = fread(p, 1, ds.s_size, fp)) != ds.s_size)
		return (NULL);

	if (size)
		*size = ds.s_size;
	if (laddr)
		*laddr = aouthdr.data_start;

	return (p);
}


coff_getbss(fp)
	FILE *fp;
{
	struct filehdr hc;
	N10AOUTHDR aouthdr;

	rewind(fp);
	fread(&hc, 1, sizeof(struct filehdr), fp);
	fread(&aouthdr, 1, sizeof(N10AOUTHDR), fp);

	return (aouthdr.bsize);
}


coff_getentry(fp)
	FILE *fp;
{
	struct filehdr hc;
	N10AOUTHDR aouthdr;

	rewind(fp);
	fread(&hc, 1, sizeof(struct filehdr), fp);
	fread(&aouthdr, 1, sizeof(N10AOUTHDR), fp);

	return (aouthdr.entry);
}


/*
 * same as an nlist but name changed to avoid conflicts with
 * coff include files
 */
struct	alist {
	char	*a_name;	/* for use when in-core */
	unsigned char a_type;	/* type flag, i.e. N_TEXT etc; see below */
	char	a_other;	/* unused */
	short	a_desc;		/* see <stab.h> */
	unsigned long a_value;	/* value of this symbol (or sdb offset) */
};


coff_iscoff(fp)
	FILE *fp;
{
	struct filehdr hc;

	rewind(fp);
	fread(&hc,1,sizeof(struct filehdr),fp);

	if (hc.f_magic == 0515)
		return (1);

	return (0);
}


coff__nlist(fp, nl)
	FILE *fp; struct alist *nl;
{
	struct filehdr hc;
	N10AOUTHDR aouthdr;
	struct scnhdr ts, ds;
	struct syment *syms, *sm;
	char *strx, *p;
	int i, cc;

	rewind(fp);
	fread(&hc, 1, sizeof(struct filehdr), fp);
	fread(&aouthdr, 1, sizeof(N10AOUTHDR), fp);
	fread(&ts, 1, sizeof(struct scnhdr), fp);
	fread(&ds, 1, sizeof(struct scnhdr), fp);

	syms = (struct syment *)calloc(hc.f_nsyms, sizeof(struct syment));
	fseek(fp, hc.f_symptr, 0);

	for (i=0; i<hc.f_nsyms; i++) {
		fread(&syms[i]._n._n_n._n_zeroes, sizeof(long),	1, fp);
		fread(&syms[i]._n._n_n._n_offset, sizeof(long), 1, fp);
		fread(&syms[i].n_value, sizeof(long), 1, fp);
		fread(&syms[i].n_scnum, sizeof(short), 1, fp);
		fread(&syms[i].n_type, sizeof(unsigned short), 1, fp);
		fread(&syms[i].n_sclass, sizeof(char), 1, fp);
		fread(&syms[i].n_numaux, sizeof(char), 1, fp);
	}

	cc = 0;
	fseek(fp, hc.f_symptr + hc.f_nsyms * SYMESZ, 0);
	while ((int)getc(fp) != EOF)
		cc++;

	strx = (char *)malloc(cc);
	fseek(fp, hc.f_symptr + hc.f_nsyms * SYMESZ, 0);
	fread(strx, 1, cc, fp);

	while (nl->a_name) {

		/* nlist(3) says it sets type to 0, we're setting value to
		 * zero as well. Could be a problem over in exec if we stop
		 * setting value to zero
		 */
		nl->a_value = 0;
		nl->a_type  = 0;

		for (i=0,sm=syms; i<hc.f_nsyms; i++,sm++) {
			if (sm->n_value == 0)
			continue;

			if (sm->n_zeroes == 0 && sm->n_offset > cc)
				continue;

			if (sm->n_zeroes == 0)
				p = (char *)&strx[sm->n_offset];
			else
				p = sm->n_name;

			if (!strcmp(p,nl->a_name)) {
				nl->a_value = sm->n_value;
			}
		}
		nl++;
	}
}
