/* "@(#)sr_general.h 1.1 94/10/31 Copyr 1986 Sun Micro";
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)general.h	3.1 11/11/85
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * general.h renamed to sr_general.h
 */

typedef	unsigned long int Word;	/* Word is 32 bits, and unsigned */
typedef	         long int SWord;/* SWord is 32 bits and signed */

typedef	unsigned int Bool;
#define	FALSE 0
#define	TRUE  1

extern char *strsave();
/* compute positive powers of 2. 2^n */
#define	pow2(n)  (((SWord)1) << (n) )

extern char eBuf[];	/* error message buffer */

#include <malloc.h>
