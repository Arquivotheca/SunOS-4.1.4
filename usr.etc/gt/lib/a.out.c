#ifndef lint
static char sccsid[] = "@(#)a.out.c 1.1 94/10/31 SMI";
#endif

#include <stdio.h>
#include <a.out.h>

char *
aout_gettext(fp, size, laddr)
	FILE *fp;
	int *size, *laddr;
{
	struct exec a;
	char *p;

	rewind(fp);
	fread(&a, 1, sizeof(struct exec), fp);
	fseek(fp, N_TXTOFF(a), 0);

	p = (char *)malloc(a.a_text);

	fread(p, 1, a.a_text, fp);

	if (size)
		*size = a.a_text;

	if (laddr)
		*laddr = a.a_entry;

	return (p);
}


char *
aout_getdata(fp, size, laddr)
	FILE *fp;
	int *size, *laddr;
{
	struct exec a;
	char *p;

	rewind(fp);
	fread(&a, 1, sizeof(struct exec), fp);
	fseek(fp, N_DATOFF(a), 0);

	p = (char *)malloc(a.a_data);

	fread(p, 1, a.a_data, fp);

	if (size)
		*size = a.a_data;

	if (laddr)
		*laddr = a.a_entry + a.a_text;

	return (p);
}


aout_getbss(fp)
	FILE *fp;
{
	struct exec a;

	rewind(fp);
	fread(&a, 1, sizeof(struct exec), fp);

	return (a.a_bss);
}


aout_getentry(fp)
	FILE *fp;
{
	struct exec a;

	rewind(fp);
	fread(&a, 1, sizeof(struct exec), fp);

	return (a.a_entry);
}
