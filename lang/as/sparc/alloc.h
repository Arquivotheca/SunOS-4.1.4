/*	@(#)alloc.h	1.1	10/31/94	*/

#ifndef _ALLOC_H_

extern U8BITS * as_malloc();
extern U8BITS * as_calloc();
extern U8BITS * as_realloc();

#ifdef DEBUG
   extern void	as_free();
#else /*DEBUG*/
#  define as_free(p)	free(p)
#endif /*DEBUG*/

extern struct value *alloc_value_struct();
extern void	     free_value_struct();

extern struct node  *alloc_node_struct();
extern void	     free_node_struct();

#ifdef DEBUG
   extern void	alloc_statistics();
#endif

#define _ALLOC_H_
#endif
