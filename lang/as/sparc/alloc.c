static char sccs_id[] = "@(#)alloc.c	1.1\t10/31/94";
#include <stdio.h>	/* for definition of NULL */
#include "structs.h"
#include "alloc.h"

#ifdef DEBUG
static long int as_malloc_calls= 0, as_free_calls  = 0;
static long int total_allocd   = 0, total_freed  = 0;
static long int max_net_allocd = 0;
#  ifdef sparc
#     define  ALIGNMENT 8
#  else
#     define  ALIGNMENT 4
#  endif
#  define  ALIGN_MASK (ALIGNMENT-1)
	/* ROUNDUP only works for "r" being a power of 2. */
#  define  ROUNDUP(value,r)	( ((value) + ((r)-1)) & (~((r)-1)) )
static void
tally_allocd(memp)
	register U8BITS * memp;
{
	/* malloc()'s magic prefix cookie is in the "int" before "memp".  */
	total_allocd += *( (unsigned int *) ((char*)memp - ALIGNMENT)) ;
	if ( (total_allocd - total_freed) > max_net_allocd )
	{
		max_net_allocd = (total_allocd - total_freed);
	}
}
static void
tally_freed(memp)
	register U8BITS * memp;
{
	/* malloc()'s magic prefix cookie is in the "int" before "memp".  */
	total_freed += *( (unsigned int *) ((char*)memp - ALIGNMENT)) ;
}
static long int values_allocd  = 0, values_freed = 0;
static long int nodes_allocd   = 0, nodes_freed  = 0;
#endif /*DEBUG*/


struct alloc_value
{
	struct value	     the_struct;	/* MUST be the first element. */
	struct alloc_value * next;
};

struct alloc_node
{
	struct node	    the_struct;	/* MUST be the first element. */
	struct alloc_node * next;
};

struct alloc_value *free_value_structs = NULL;
struct alloc_node  *free_node_structs = NULL;

#define VALUE_ALLOC_BLKSZ	100
#define NODE_ALLOC_BLKSZ	100

struct value *
alloc_value_struct()
{
	/* Return a pointer to an avaiable "struct value".  That "sturct value"
	 * can (only) be returned to the free pool by using free_value_struct().
	 */

	register struct alloc_value *avp;

	if (free_value_structs == NULL)
	{
		/* We are out of available "struct value"s.  Get some more. */

		register int i;
		register struct alloc_value *p;

		free_value_structs =
			(struct alloc_value *)
			as_malloc(sizeof(struct alloc_value)*VALUE_ALLOC_BLKSZ);

		/* Chain all of them together, except the last one, which
		 * gets NULL for a chain pointer.
		 */
		for ( i = 0, p = free_value_structs;
		      i < (VALUE_ALLOC_BLKSZ-1);
		      i++, p++)
		{
			p->next = p + 1;	/* note: pointer arithmetic! */
		}
		p->next = NULL;
	}

	avp = free_value_structs;
	free_value_structs = avp->next;
	avp->next = NULL;	/* ...just to be safe. */

#ifdef DEBUG
	values_allocd++;
#endif /*DEBUG*/

	return &(avp->the_struct);
}


void
free_value_struct(memp)
	register struct value *memp;
{
	/* Return a "struct value" (which must have been allocated with
	 * alloc_value_struct()) to the available pool.
	 */

	/* This routine should ONLY be called after all sub-components of
	 * a value structure have been freed; i.e. it should only be called
	 * from free_scratch_value().
	 */

	register struct alloc_value *avp;

#ifdef DEBUG
	values_freed++;
#endif /*DEBUG*/

	/* Get the pointer to the beginning of the alloc_value structure.
	 * This assumes that "the_struct" is the first element of alloc_value!
	 */
	avp = (struct alloc_value *)memp;

	/* Insert this onto the beginning of the free list. */
	avp->next = free_value_structs;
	free_value_structs = avp;
}


struct node *
alloc_node_struct()
{
	/* Return a pointer to an avaiable "struct node".  That "sturct node"
	 * can (only) be returned to the free pool by using free_node_struct().
	 */

	register struct alloc_node *anp;

	if (free_node_structs == NULL)
	{
		/* We are out of available "struct node"s.  Get some more. */

		register int i;
		register struct alloc_node *p;

		free_node_structs =
			(struct alloc_node *)
			as_malloc(sizeof(struct alloc_node)*NODE_ALLOC_BLKSZ);

		/* Chain all of them together, except the last one, which
		 * gets NULL for a chain pointer.
		 */
		for ( i = 0, p = free_node_structs;
		      i < (NODE_ALLOC_BLKSZ-1);
		      i++, p++)
		{
			p->next = p + 1;	/* note: pointer arithmetic! */
		}
		p->next = NULL;
	}

	anp = free_node_structs;
	free_node_structs = anp->next;
	anp->next = NULL;	/* ...just to be safe. */

#ifdef DEBUG
	nodes_allocd++;
#endif /*DEBUG*/

	return &(anp->the_struct);
}


void
free_node_struct(memp)
	register struct node *memp;
{
	/* Return a "struct node" (which must have been allocated with
	 * alloc_node_struct()) to the available pool.
	 */

	/* This routine should ONLY be called after all sub-components of
	 * a node structure have been freed; i.e. it should only be called
	 * from free_node().
	 */

	register struct alloc_node *avp;

#ifdef DEBUG
	nodes_freed++;
#endif /*DEBUG*/

	/* Get the pointer to the beginning of the alloc_node structure.
	 * This assumes that "the_struct" is the first element of alloc_node!
	 */
	avp = (struct alloc_node *)memp;

	/* Insert this onto the beginning of the free list. */
	avp->next = free_node_structs;
	free_node_structs = avp;
}


U8BITS *
as_malloc(size)
	unsigned int size;
{
	register U8BITS *memp;
	extern   char	*malloc();

	if ( (memp = (U8BITS *)malloc(size)) == NULL )
	{
		internal_error("out of memory");
		return (U8BITS *)NULL;/*stmt just here to keep lint happy*/
	}
	else
	{
#ifdef DEBUG
		as_malloc_calls++;
		tally_allocd(memp);
#endif /*DEBUG*/
		return memp;
	}
}


U8BITS *
as_calloc(count, size)
	unsigned int count;
	unsigned int size;
{
	register U8BITS *memp;

	memp = as_malloc(count*size);
	bzero(memp, count*size);
	return memp;
}


U8BITS *
as_realloc(memp, size)
	U8BITS	     *memp;
	unsigned int  size;
{
	register U8BITS *new_memp;
	extern   char	*realloc();

#ifdef DEBUG
	tally_freed(memp);
#endif /*DEBUG*/
	if ( (new_memp = (U8BITS *)realloc(memp,size)) == NULL )
	{
		internal_error("out of memory");
		return (U8BITS *)NULL;/*stmt just here to keep lint happy*/
	}
	else
	{
#ifdef DEBUG
		tally_allocd(new_memp);
#endif /*DEBUG*/
		return new_memp;
	}
}


#ifdef DEBUG
void
as_free(memp)
	register U8BITS *memp;
{
	as_free_calls++;
	tally_freed(memp);
	free(memp);
}
#endif /*DEBUG*/


#ifdef DEBUG
void
alloc_statistics()
{
	fprintf(stderr,
		"[alloc/free:       total calls:%7ld alloc,  %7ld free]\n",
		as_malloc_calls, as_free_calls);
	fprintf(stderr,
		"[alloc/free:       total bytes:%7ld alloc'd,%7ld freed, %ld max net]\n",
		total_allocd, total_freed, max_net_allocd);
	fprintf(stderr,
		"[alloc/free: values @%3ld bytes:%7ld alloc'd,%7ld freed]\n",
		sizeof(unsigned int *)+ROUNDUP(sizeof(struct value),ALIGNMENT),
		values_allocd, values_freed);
	fprintf(stderr,
		"[alloc/free: nodes  @%3ld bytes:%7ld alloc'd,%7ld freed]\n",
		sizeof(unsigned int *)+ROUNDUP(sizeof(struct node),ALIGNMENT),
		nodes_allocd, nodes_freed);
}
#endif /*DEBUG*/
