#ident "@(#)cb_swap_bytes.c 1.1 94/10/31 Copyr 1986 Sun Micro"

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


#ifndef cb_swap_bytes_h_INCLUDED
#include "cb_swap_bytes.h"
#endif

#ifdef CB_SWAP
/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		cb_swap_int_func();
extern	void		cb_swap_short_func();

void
cb_swap_int_func(address)
	register char	*address;
{
	cb_swap_int(*address);
}

void
cb_swap_short_func(address)
	register char	*address;
{
	cb_swap_short(*address);
}

#else
	char	cb_dummy_symbol_to_make_ranlib_happy;
#endif

