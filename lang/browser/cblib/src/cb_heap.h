/* LINTLIBRARY */

/*	@(#)cb_heap.h 1.1 94/10/31 SMI	*/

/*
 *	Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved.
 *	Sun considers its source code as an unpublished, proprietary
 *	trade secret, and it is available only under strict license
 *	provisions.  This copyright notice is placed here only to protect
 *	Sun in the event the source is deemed a published work.  Dissassembly,
 *	decompilation, or other means of reducing the object code to human
 *	readable form is prohibited by the license agreement under which
 *	this code is provided to the user or company in possession of this
 *	copy.
 *	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *	Government is subject to restrictions as set forth in subparagraph
 *	(c)(1)(ii) of the Rights in Technical Data and Computer Software
 *	clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *	NASA FAR Supplement.
 */

#ifndef cb_heap_h_INCLUDED
#define cb_heap_h_INCLUDED

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

/*
 *	Utility macros
 */
#define cb_allocate(type, heap) \
	(type)cb_get_memory(sizeof(*((type)0)), (heap))
#define cb_nallocate(type, n, heap) \
	(type)cb_get_memory(sizeof(*((type)0)) * (n), (heap))

#ifndef TRUE
#define TRUE		1
#endif

#ifndef FALSE
#define FALSE		0
#endif

typedef struct Cb_heap_tag			*Cb_heap, Cb_heap_rec;

/*
 *	The Cb_heap struct is used to allocate memory.
 *	The general idea is to allocate all memory that should be
 *	freed at the same time (for one .cb file) from the same
 *	heap. By freeing the heap all allocated memory is freed.
 *	The chain field is used to chain all the heaps we have consumed
 *	so far together. It points to the first word of a chnk that we
 *	got from malloc(). That first word then points to the next heap
 *	and so on.
 */
struct Cb_heap_tag {
	char		*free;
	char		*last;
	char		*chain;
	int		allocation_size;
	int		total_used;
};

PORT_EXTERN(double	*cb_malloc(PORT_1ARG(int)));
PORT_EXTERN(void	cb_free_heap(PORT_1ARG(Cb_heap)));
PORT_EXTERN(Cb_heap	cb_init_heap(PORT_1ARG(int)));
PORT_EXTERN(double	*cb_get_memory(PORT_2ARG(int, Cb_heap)));
PORT_EXTERN(void	cb_free_heap_on_exit(PORT_1ARG(Cb_heap)));

#endif
