#ifndef lint
static	char sccsid[] = "@(#)genswitch.c	1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"
#include "ctype.h"
#include "pcc_sw.h"

/* 
 * genswitch.c
 */
# ifdef FORT
int ftlab1, ftlab2;
# endif
/* a lot of the machine dependent parts of the second pass */
#ifdef FORT
#   define fltfun 0
#else
    extern int fltfun;
#endif
# define BITMASK(n) ((1L<<(n))-1)
# define MAXHASHIMM 512



int	 *hashtab; 
unsigned hashkey, hashsize, hashflag;


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 *  do_hash(swtab, num, a, N, hashflag): finds a hash function and
 *	generates the corresponding hash table for a switch
 *	statement.  It tries to find a perfect hash and if it fails 
 *	after "hash table size" trials it re-hashes with the best
 *	trial.  Linear probing is used for collision resolution.
 *      The hash function used in this scheme is:
 *		h(x) = [(x/N)*a + x]% N
 *      where "N" is the size of the hash table and "a" iterates
 *	between 1 and N until it finds a perfect or rehashes with the
 *	"a" that yields the least number of collisions.
 */



int *do_hash(swtab, num, a, N, hashflag)

    register struct sw swtab[];
    unsigned *a, *N, *hashflag;
    int num;

{
    unsigned z, smallest;
    register struct sw *r;
    int *hashtab, flag, *p, hashsize, key;
    int mincoll_num_of, mincoll_key;
   

    mincoll_num_of = 10000000;	
    mincoll_key = 0;
    smallest = swtab[1].sval;
    hashsize = 1;
    key = 1;
    
    while (hashsize <= num)          /* get table size  */
	hashsize *= 2;
    hashtab = (int *) calloc (hashsize, sizeof(int));
    for (; key < hashsize; key++) {
	flag = 0;
	for (p = hashtab; p < hashtab + hashsize; p++) {*p = -1;}  /* flag the
								  table */
	for (r = (swtab + 1); r < swtab + (num + 1); r++) {    /* do the
								  hashing */
	    if (smallest != 0) 
		z = (((r->sval - smallest)/hashsize)*key+(r->sval - smallest)) % hashsize;
	    else
		z = ((r->sval/hashsize)*key + r->sval) % hashsize;
	    if (hashtab[z] != -1) {		/* collision */
		flag += 1;
		while (hashtab[z] != -1){	/* linear probe */
		    z = (z + 1) % hashsize;
		}
	    }
	    hashtab[z] = r - swtab;
	}

	if (flag < mincoll_num_of) {
	    mincoll_num_of = flag;
	    mincoll_key = key;
	}
        if (flag == 0) break;
    }


    if (flag != 0){            /* re-hash with least number of collisions */
	for (p = hashtab; p < hashtab + hashsize; p++) {*p = -1;}
	for (r = (swtab + 1); r < swtab + (num + 1); r++) {
	    if (smallest != 0) 
		z = (((r->sval - smallest)/hashsize)*(mincoll_key)+(r->sval - smallest)) % hashsize;
	    else
		z = ((r->sval/hashsize)*(mincoll_key) + r->sval) % hashsize;
	    if (hashtab[z] != -1) {
		while (hashtab[z] != -1){
		    z = (z + 1) % hashsize;
		}
	    }
	    hashtab[z] = r - swtab;
	}
    }
    *hashflag = mincoll_num_of;
    *N = hashsize;
    *a = (mincoll_num_of) ? mincoll_key : key;
    return(hashtab);
    
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * p2switch(p,swtab,n): generate code for a switch statement.
 *	This routine supports the FSWITCH operator defined in
 *	fort.c and the IR_SWITCH operator in cgrdr.  For
 *	unoptimized C compilations, it is called directly
 *	from genswitch() in code.c.
 */

p2switch(p,swtab,n)
	NODE *p;
	register struct sw swtab[];
	int n;
{
	/*
	 * p is either null or the switch selector expression, whose type
	 * is assumed to be INT. If null, assume the expression has been
	 * compiled into the FORCE register (d0 on the 68000).
	 * 
	 * swtab[] is an array of structures, each consisting of a constant
	 * value and a label.  The first label is the label of the default case
	 * (a fall-through in C and f77; a trap in Pascal).  The first constant
	 * value is always ignored.
	 * 
	 * n is the number of cases, including the default case. 
	 */

	register i,j;

	NODE *q1, *q2, *q3, *q4;

	register dlab, swlab;
	unsigned biggest, smallest, range;

	n--;	/* because everyone else counts the default case */
	if (p != NULL) {
		/* evaluate selector expression */
		myreader(p); /* make me canonical */
		p->in.rall = (MUSTDO|RO0);
		codgen(p, INTAREG);
		reclaim(p, RNULL, 0);
	}
	smallest = swtab[1].sval;
	biggest = swtab[n].sval;
	range = biggest-smallest;
	dlab = swtab[0].slab >= 0 ? swtab[0].slab : getlab();
	swlab = getlab();

	if( range>0 && range <= 3*n && n>=4 ){ /* implement a direct switch */

		if (smallest != 0 ){
			if (IN_IMMEDIATE_RANGE((int)smallest)){
			    printf( "	sub	%%o0,%d,%%o0\n", smallest);
			} else {
			    genimmediate( (int)smallest, RO1);
			    printf( "	sub	%%o0,%%o1,%%o0\n");
			}
		}

		/*
		 * note that this is an unsigned compare; it thus checks
		 * for numbers below range as well as above it.
		 */
		if (IN_IMMEDIATE_RANGE((int)range)){
			printf( "	cmp	%%o0,%d\n", range );
		} else {
			genimmediate( (int)range, RO1);
			printf( "	cmp	%%o0,%%o1\n");
		}
		printf( "	bgu	L%d\n", dlab );
		printf( "	sll	%%o0,2,%%o0\n");
		if (picflag){
			printf( "1:	call	2f\n");
			printf( "	sethi	%%hi(L%d-1b), %%g1\n", swlab);
			printf( "2:	or	%%g1, %%lo(L%d-1b), %%g1\n", swlab);
			printf( "	add	%%o0, %%g1, %%o0\n");
			printf( "	ld	[%%o7+%%o0], %%o0\n");
			printf( "	jmp	%%o7+%%o0\n");
		} else {
			printf( "	set	L%d,%%o1\n", swlab );
			printf( "	ld	[%%o0+%%o1],%%o0\n" );
			printf( "	jmp	%%o0\n" );
		}
		printf( "	nop\n" );

		/* output table */
		printf( "	L%d:  \n", swlab );
		for( i=1,j=swtab[1].sval; i<=n; ++j ){
			if (picflag) {
				printf( "	.word	L%d - 1b\n", 
					(j == swtab[i].sval) ? swtab[i++].slab : dlab);
			} else {
				printf( "	.word	L%d\n", ( j == swtab[i].sval ) ?
					swtab[i++].slab : dlab );
			}
		}
		if( swtab[0].slab< 0 ) deflab( dlab );
		return;

		}


	if( n>8 ) {

		/* hash table method	*/

		if (smallest != 0 ){
			if (IN_IMMEDIATE_RANGE((int)smallest)){
				printf( "	sub	%%o0,%d,%%o0\n", smallest);
			} else {
				genimmediate( (int)smallest, RO1);
				printf( "	sub	%%o0,%%o1,%%o0\n");
			}
		}
		if (IN_IMMEDIATE_RANGE((int)range)){
			printf( "	cmp	%%o0,%d\n", range );
		}else {
			genimmediate( (int)range, RO1);
			printf( "	cmp	%%o0,%%o1\n");
		}
		printf( "	bgu	L%d\n", dlab );

		if (picflag) {
			printf( "	nop\n");
			printf( "4:	call	5f\n");
			printf( "	sethi	%%hi(L%d-4b), %%o2\n", swlab);
			printf( "5:	or	%%o2, %%lo(L%d-4b), %%o2\n", swlab);
			printf( "	add	%%o7, %%o2, %%o2\n");
		} else {
			printf( "	sethi	%%hi(L%d),%%o2\n", swlab);
			printf( "	or	%%o2,%%lo(L%d),%%o2\n", swlab);
		}
		hashtab = do_hash(swtab, n, &hashkey, &hashsize, &hashflag);
		/* set up hashing expression */
	        q1 = build_in(RS, UNSIGNED, 
			       build_tn(REG, UNSIGNED,"", 0, RO0),
			       build_tn(ICON, UNSIGNED,"", ffs(hashsize)-1, 0));
	        q2 = build_in(MUL, UNSIGNED, q1,
			       build_tn(ICON, UNSIGNED, "", hashkey, 0));
	        q3 = build_in(PLUS, UNSIGNED, q2,
			       build_tn(REG, UNSIGNED,"", 0, RO0));
	        q4 = build_in(MOD, UNSIGNED, q3,
			       build_tn(ICON, UNSIGNED,"", hashsize, 0));
	        q4->in.rall = (MUSTDO|RO1);

		/* the case value is in %o0
		 * the value is hashed into %o1
		 * the swtab label is  %o2
		 * the jump-off point is in %o7
		 * the following rbusy calls are necessary to keep these
		 * registers form being used as temporaries. 
		 */

		/* evaluate hashing expression */
		rbusy(RO0, UNSIGNED); 
		rbusy(RO0, UNSIGNED); 
		rbusy(RO0, UNSIGNED); 
		rbusy(RO2, UNSIGNED);
		rbusy(RO7, UNSIGNED);
		order(q4, INTAREG);
		reclaim(q4, RNULL, 0);
		rfree(RO0, UNSIGNED); 
		rfree(RO2, UNSIGNED);
		rfree(RO7, UNSIGNED);

		/* here's the initial compare with hashed value */
		printf( "	sll	%%o1,3,%%o4\n" );
		printf( "1:	ld	[%%o4+%%o2],%%o3\n" );
		printf( "	cmp	%%o3,%%o0\n");
		if (!(hashflag)) { /* perfect hash; no need for linear probe code */
		    printf( "	bne	L%d\n", dlab);
		    printf( "	nop\n");		    
		}
		else {
     		    printf( "	be	2f\n");
		    printf( "	nop\n");
		/* here's the LOOSE case and linear probing */
		    printf( "3:	cmp	%%o3,-1\n");
		    printf( "	be	L%d\n", dlab);
		    printf( "	nop\n");
		    /* (hashsize-1)*8 too big for immediate */
		    if (hashsize <= MAXHASHIMM) { 
			printf( "	add	%%o4,8,%%o4\n" );
			printf( "	and	%%o4,%d,%%o4\n", (hashsize-1)*8);
		    }
		    else {
			printf( "	add	%%o1,1,%%o1\n");
			printf( "	and	%%o1,%d,%%o1\n", hashsize-1);
			printf( "	sll	%%o1,3,%%o4\n");
		    }
		    printf( "	ld	[%%o4+%%o2],%%o3\n" );
		    printf( "	cmp	%%o3,%%o0\n");
		    printf( "	be	2f\n");
		    printf( "	nop\n");
		    printf( "	b	3b\n");
		    printf( "	nop\n");
		}

		/* here's the WIN case */
		printf( "2:	add     %%o4,%%o2,%%o4\n");
		printf( "	ld	[%%o4+4], %%o0\n");
		if (picflag) {
			printf( "	jmp	%%o7+%%o0\n");
		} else {
			printf( "	jmp	%%o0\n");
		}
		printf( "	nop\n");



		/* print the switch table  */
		printf( "L%d:\n", swlab );
		for( i=0; i<(hashsize); i++ ){
		    if (hashtab[i] != -1){
				if (picflag) {
					printf( "	.word	%d,L%d-4b\n",
			    			swtab[hashtab[i]].sval-smallest,
			    			swtab[hashtab[i]].slab );
				} else {
					printf( "	.word	%d,L%d\n", 
			    			swtab[hashtab[i]].sval-smallest,
			    			swtab[hashtab[i]].slab );
				}
		    } else {
				if (picflag) {
					printf("        .word   -1,L%d-4b\n", dlab);
				} else {
					printf("        .word   -1,L%d\n", dlab);
				}
		    }
		}
		/* and finally ... */
		if( swtab[0].slab< 0 ) deflab( dlab );
		free(hashtab);
		return;
	} 



	/* simple switch code */

	for( i=1; i<=n; ++i ){
		/* already in o0 */
		if (IN_IMMEDIATE_RANGE((int)swtab[i].sval)){
			printf( "	cmp	%%o0,%d\n", swtab[i].sval );
		} else {
			genimmediate((int)swtab[i].sval, RO1);
			printf( "	cmp	%%o0,%%o1\n" );
		}
		printf( "	be	L%d\n", swtab[i].slab );
	}
	printf( "	nop\n");

	/* branch to default case, if there is one */
	if (swtab->slab > 0) branch( swtab->slab );
}

