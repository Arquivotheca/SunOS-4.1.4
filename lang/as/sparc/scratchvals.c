static char sccs_id[] = "@(#)scratchvals.c	1.1\t10/31/94";
#include <stdio.h>
#include "structs.h"
#include "errors.h"
#include "alloc.h"


/*****************  g e t _ s c r a t c h _ v a l u e  ***********************/

struct value *
get_scratch_value(sp, length)
	register char *sp;		/* pointer to string */
	register unsigned int length;	/* length of string  */
{
	register struct value *vp;

	vp = alloc_value_struct();
	/* Zeroing the whole structure with bzero() accomplishes the following:
	 *	vp->v_symp = NULL;  // no symbol associated with value (yet) //
	 *	vp->v_next = NULL;  // not a list of Values (yet)	     //
	 *	vp->v_relmode = REL_NONE; // no relocation mode (yet)	     //
	 *	vp->v_value   = 0;  // no value (yet)			     //
	 */
	bzero(vp, sizeof(struct value));

	if (sp == NULL)
	{
		vp->v_strptr = NULL;
	}
	else
	{
		vp->v_strptr = (char *)as_malloc(length + 1);
		bcopy(sp, vp->v_strptr, length);
		*(vp->v_strptr + length) = '\0';
	}

	vp->v_strlen = length;

	return vp;
}


/****************  f r e e _ s c r a t c h _ v a l u e  **********************/

void
free_scratch_value(vp)
	register struct value *vp;
{
	if (vp != NULL)
	{
		if (vp->v_strptr != NULL)
		{
			as_free(vp->v_strptr);
		}
		free_value_struct(vp);
	}
}


/*****************  n e w _ v a l _ l i s t _ h e a d  ***********************/

struct val_list_head *
new_val_list_head()
{
	register struct val_list_head *vlhp;

	vlhp = (struct val_list_head *) as_malloc(sizeof(struct val_list_head));
	bzero(vlhp, sizeof(struct val_list_head));
	return  vlhp;
}


/*******************  f r e e _ v a l u e _ l i s t  *************************/

void
free_value_list(vlhp)
	register struct val_list_head *vlhp;
{
	register struct value *vp;
	register struct value *next_vp;

	/* deallocate the individual list elements */
	for (vp = vlhp->vlh_head;   vp != NULL;   vp = next_vp)
	{
		next_vp = vp->v_next;
		free_scratch_value(vp);
	}

	/* now, deallocate the list-head structure */
	as_free(vlhp);
}
