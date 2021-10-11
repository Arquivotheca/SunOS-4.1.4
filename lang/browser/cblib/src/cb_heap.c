#ident "@(#)cb_heap.c 1.1 94/10/31 Copyr 1986 Sun Micro"

#ifndef lint
#ifdef INCLUDE_COPYRIGHT
static char _copyright_notice_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
Sun considers its source code as an unpublished, proprietary \
trade secret, and it is available only under strict license \
provisions.  This copyright notice is placed here only to protect \
Sun in the event the source is deemed a published work.  Dissassembly, \
decompilation, or other means of reducing the object code to human \
readable form is prohibited by the license agreement under which \
this code is provided to the user or company in possession of this \
copy. \
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the \
Government is subject to restrictions as set forth in subparagraph \
(c)(1)(ii) of the Rights in Technical Data and Computer Software \
clause at DFARS 52.227-7013 and in similar clauses in the FAR and \
NASA FAR Supplement.";
#endif

#ifdef INCLUDE_COPYRIGHT_REFERENCE
static char _copyright_notice_reference_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
See copyright notice in cb_copyright.o in the libsb.a library.";
#endif
#endif


#ifndef FILE
#include <stdio.h>
#endif

#include <malloc.h>

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif


/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	double		*cb_malloc();
extern	double		*cb_get_memory();
extern	Cb_heap		cb_init_heap();
extern	void		cb_free_heap();
extern	void		cb_free_heap_on_exit();
static	int		cb_free_heap_on_exit_doit();


/*
 *	cb_malloc()
 *		Allocate memory and make sure we got some.
 */
double *
cb_malloc(size)
	int		size;
{
	register char	*result = malloc((unsigned)size);

	if (result == NULL) {
		cb_fatal("Request for %d bytes of memory failed\n", size);
	}
	return (double *)result;
}

/*
 *	cb_get_memory(size, heap)
 *		This will allocate and return "size" bytes from "heap".  If
 *		"heap" is NULL, cb_malloc() will be used.
 */
double *
cb_get_memory(size, heap)
	register int		size;
	register Cb_heap	heap;
{
	register char		*result;
	register int		allocate;
	register char		*new_chunk;

	if (heap == NULL) {
		return cb_malloc(size);
	}

    try_again:
	result = heap->free;
#ifdef sparc
	size = cb_align(size, sizeof(char *));
#endif
	heap->free += size;

	if (heap->last <= heap->free) {
		if (size > heap->allocation_size - sizeof(Cb_heap_rec)) {
			heap->free -= size;
			allocate = size + sizeof(Cb_heap_rec);
			new_chunk = (char *)cb_malloc(allocate+sizeof(char *));
			heap->total_used += allocate + sizeof(char *);
			*((char **)new_chunk) = *((char **)heap->chain);
			*((char **)heap->chain) = new_chunk;
			return (double *)(new_chunk + sizeof(Cb_heap_rec) +
					  sizeof(char *));
		} else {
			allocate = heap->allocation_size;
			new_chunk = (char *)cb_malloc(allocate+sizeof(char *));
			heap->total_used += allocate + sizeof(char *);
			*((char **)new_chunk) = heap->chain;
			heap->chain = new_chunk;
			heap->free = new_chunk + sizeof(char *);
			heap->last = heap->free + allocate;
			goto try_again;
		}
	}
	return (double *)result;
}

/*
 *	cb_init_heap(size)
 *		This will allocate and return a new heap with allocations
 *		in "size" byte heaps.  If "size" is zero, the system page
 *		size will be used.
 */
Cb_heap
cb_init_heap(size)
	int	size;
{
	Cb_heap		heap;
	char		*new_chunk;
static 	int		page_size = 0;

	if (size == 0) {
		if (page_size == 0) {
			page_size = getpagesize();
		}
		size = page_size;
	}
	new_chunk =
		(char *)cb_malloc(size + sizeof(char *) + sizeof(Cb_heap_rec));
	heap = (Cb_heap)(new_chunk + sizeof(char *));
	heap->free = (char *)heap + sizeof(Cb_heap_rec);
	heap->last = heap->free + size;
	heap->allocation_size = size;
	heap->total_used = 0;
	heap->chain = new_chunk;
	*((char **)new_chunk) = NULL;
	return heap;
}

/*
 *	cb_free_heap(heap)
 *		This will deallocate all of the storage associated with
 *		"heap".
 */
void
cb_free_heap(heap)
	Cb_heap	heap;
{
	char	**f;
	char	**next;

	for (f = (char **)heap->chain; f != NULL; f = next) {
		next = (char **)(*f);
		cb_free((char *)f);
	}
}

/*
 *	cb_free_heap_on_exit(heap)
 *		This will cause "heap" to be freed when exit() is called.
 */
void
cb_free_heap_on_exit(heap)
	Cb_heap		heap;
{
	(void)on_exit(cb_free_heap_on_exit_doit, (char *)heap);
}	

/*
 *	cb_free_heap_on_exit_doit(status, heap)
 *		This will free the storage associated with "heap".
 */
/*ARGSUSED*/
static int
cb_free_heap_on_exit_doit(status, heap)
	int		status;
	Cb_heap		heap;
{
	cb_free_heap(heap);
}

