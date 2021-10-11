/*	@(#)search.h 1.1 94/10/31 SMI; from S5R2 1.2	*/

#ifndef _search_h
#define _search_h

/* HSEARCH(3C) */
typedef struct entry { char *key, *data; } ENTRY;
typedef enum { FIND, ENTER } ACTION;

/* TSEARCH(3C) */
typedef enum { preorder, postorder, endorder, leaf } VISIT;

#endif /*!_search_h*/
