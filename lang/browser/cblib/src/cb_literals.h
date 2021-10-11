/* LINTLIBRARY */

/*	@(#)cb_literals.h 1.1 94/10/31 SMI	*/

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

#ifndef cb_literals_h_INCLUDED
#define cb_literals_h_INCLUDED

/*
 * Some constants used by everyone:
 */
#define CB_SUFFIX			".bd"
#define CB_SUFFIX_LEN			3
#define CB_SUFFIX_PENDING_DELETE	".pd"
#define CB_SUFFIX_PENDING_DELETE_LEN	3
#define CB_SUFFIX_IN_PROGRESS		".ip"
#define CB_SUFFIX_IN_PROGRESS_LEN	3

#define CB_OPTION			"-sb"
#define	CB_DIRECTORY			".sb"
#define CB_DIRECTORY_LEN		3
#define CB_NEW_DIR			"NewRoot"
#define CB_NEW_DIR_ID			1
#define CB_OLD_DIR			"OldRoot"
#define CB_OLD_DIR_ID			2
#define CB_REFD_DIR			"Refd"
#define CB_REFD_DIR_ID			3
#define CB_LOCKED_DIR			"Locked"
#define CB_LOCKED_DIR_ID		4

#define CB_MARK_FILES_HEADER		'.'
#define CB_SPLIT_HAPPENED		".Split.Happened"
#define CB_NO_GC_REFD			".No.GC.Refd"
#define CB_NO_GC_ROOT			".No.GC.Root"

#define CB_INIT_DEFAULT_FILE_NAME	".sbinit"

#endif
