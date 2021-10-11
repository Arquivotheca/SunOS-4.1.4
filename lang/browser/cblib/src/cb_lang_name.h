/* LINTLIBRARY */

/*  @(#)cb_lang_name.h 1.1 94/10/31 SMI */

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


#ifndef cb_lang_name_h_INCLUDED
#define cb_lang_name_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#define	CB_LANG_NAME_SIZE	16

typedef char				Cb_lang_name[CB_LANG_NAME_SIZE];
typedef	struct Cb_lang_tag		*Cb_lang, Cb_lang_rec;

/*
 *	This is used to represent a language.
 */
enum Cb_grepable_tag {
	cb_first_grepable,

	cb_dont_know_grepable,
	cb_is_grepable,
	cb_is_not_grepable,

	cb_last_grepable
};
typedef enum Cb_grepable_tag		*Cb_grepable_ptr, Cb_grepable;

struct Cb_lang_tag {
	char		*name;		/* Unique language name string */
	short		lnum;		/* Language number (or -1) */
	short		major_version;
	short		minor_version;
	short		active;
	Cb_grepable	grepable;
};

extern	Array		cb_lang_list;

PORT_EXTERN(Cb_lang	cb_lang_create(PORT_1ARG(char*)));
PORT_EXTERN(void	cb_lang_flush());

#endif
