static char sccs_id[] = "@(#)localsyms.c	1.1\t10/31/94";

/* Following is array of definition counts for each of the 10 local labels.
 * Every reference to "Nb" is to the previous (backward) definition of the
 * label, while a reference to "Nf" is to the next (forward) definition.
 */

static unsigned int local_label_count[10] = {0,0,0,0,0,0,0,0,0,0};



/**********************  l o c a l _ s y m _ n a m e  ************************/
char *
local_sym_name(n, count_offset, bufp)
	register long int   n;		/* "n" MUST be 0 .. 9 */
	register int   count_offset;
	register char *bufp;
{
	register unsigned int lblno;	/* must be unsigned for ">>" to work */

#ifdef DEBUG
	if ((n < 0) || (n > 9))	internal_error("local_sym_name(): n=%d?", n);
#endif
	/*
	 * The following is much cheaper/faster than doing a "sprintf" or
	 * something similar...
	 */
	for ( *bufp++ = n + '0', lblno = local_label_count[n] + count_offset;
	       lblno != 0;
	       lblno >>= 4
	    )
	{
		*bufp++ = 'A' + (lblno & 0x0f);
	}
	*bufp++ = '\0';	/* null-terminate the symbol name! */

	return bufp;
}


/***********************  n e w _ l o c a l _ s y m  *************************/
void
new_local_sym(n)
	register long int n;		/* "n" MUST be 0 .. 9 */
{
#ifdef DEBUG
	if ((n < 0) || (n > 9))	internal_error("new_local_sym(): n=%d", n);
#endif
	local_label_count[n]++;
}
