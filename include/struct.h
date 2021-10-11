/*	@(#)struct.h 1.1 94/10/31 SMI; from UCB 4.1 83/05/03	*/

/*
 * access to information relating to the fields of a structure
 */

#ifndef _struct_h
#define _struct_h

#define	fldoff(str, fld)	((int)&(((struct str *)0)->fld))
#define	fldsiz(str, fld)	(sizeof(((struct str *)0)->fld))
#define	strbase(str, ptr, fld)	((struct str *)((char *)(ptr)-fldoff(str, fld)))

#endif /*!_struct_h*/
