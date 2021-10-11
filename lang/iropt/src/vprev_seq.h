/*	@(#)vprev_seq.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __vprev_seq__
#define __vprev_seq__

/*
 * macro to iterate through the VAR_REFs associated with a triple
 */
#define FOR_VAR_REFS(ref, tp) \
	for ((ref) = (tp)->var_refs; (ref) && (ref)->site.tp == (tp); \
	    ref = ref->next)

extern VAR_REF	*init_vprev_seq(/*tp*/);
extern VAR_REF	*vprev_seq();

#endif
