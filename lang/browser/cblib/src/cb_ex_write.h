/* LINTLIBRARY */

/*  @(#)cb_ex_write.h 1.1 94/10/31 SMI */

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


#ifndef cb_ex_write_h_INCLUDED
#define cb_ex_write_h_INCLUDED

#ifndef FILE
#include <stdio.h>
#endif

#ifndef cb_ex_h_INCLUDED
#include "cb_ex.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

typedef struct Cb_ex_unique_string_tag	*Cb_ex_unique_string,
						Cb_ex_unique_string_rec;
typedef struct Cb_ex_file_write_tag	*Cb_ex_file_write,Cb_ex_file_write_rec;

struct Cb_ex_unique_string_tag {
	char		*string;
	int		offset;
};

struct Cb_ex_file_write_tag {
	Cb_ex_file	file;
	FILE		*fd;
	Hash		strings;
	char		*language;
	Hash		languages;
	Hash		language_tags;
	Array		menu_item_path;
	int		next_offset;
};

PORT_EXTERN(void	cb_ex_write_ex_file(PORT_2ARG(Cb_ex_file, Hash)));

#endif
