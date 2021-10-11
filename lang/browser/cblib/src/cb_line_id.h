/* LINTLIBRARY */

/*	@(#)cb_line_id.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_line_id_h_INCLUDED
#define cb_line_id_h_INCLUDED

#ifndef cb_swap_bytes_h_INCLUDED
#include "cb_swap_bytes.h"
#endif

typedef	struct Cb_line_id_tag		*Cb_line_id, Cb_line_id_rec;
typedef	struct Cb_line_id_chunk_tag	*Cb_line_id_chunk, Cb_line_id_chunk_rec;

/*
 *	The .cb file line_id section
 */
struct Cb_line_id_tag {
#ifdef CB_SWAP
	unsigned int	hash : 19;	/* Line hash value */
	unsigned int	is_inactive : 1; /* Is ifdefd out */
	unsigned int	length : 12;	/* Line length (exclude '\n') */
#else
	unsigned int	length : 12;	/* Line length (exclude '\n') */
	unsigned int	is_inactive : 1; /* Is ifdefd out */
	unsigned int	hash : 19;	/* Line hash value */
#endif
};

/*
 *	The compiler builds an array of Cb_line_id_chunk.
 */
struct Cb_line_id_chunk_tag {
	Cb_line_id	chunk;
	int		size;
};

#endif
