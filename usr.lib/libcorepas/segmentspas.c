/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)segmentspas.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int delete_all_retained_segments();
int delete_retained_segment();
int rename_retained_segment();

int delallretainsegs()
	{
	return(delete_all_retained_segments());
	}

int delretainsegment(segname)
int segname;
	{
	return(delete_retained_segment(segname));
	}

int renameretainseg(segname, newname)
int segname, newname;
	{
	return(rename_retained_segment(segname, newname));
	}
