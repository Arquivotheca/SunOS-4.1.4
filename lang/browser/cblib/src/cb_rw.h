/* LINTLIBRARY */

/*	@(#)cb_rw.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_rw_h_INCLUDED
#define cb_rw_h_INCLUDED

#ifndef FILE
#include <stdio.h>
#endif

#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_directory_h_INCLUDED
#include "cb_directory.h"
#endif

#ifndef cb_lang_name_h_INCLUDED
#include "cb_lang_name.h"
#endif

#ifndef cb_line_id_h_INCLUDED
#include "cb_line_id.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

typedef	struct Cb_file_tag		*Cb_file, Cb_file_rec;
typedef struct Cb_read_refd_tag		*Cb_read_refd, Cb_read_refd_rec;
typedef struct Cb_read_symtab_tag	*Cb_read_symtab, Cb_read_symtab_rec;
typedef struct Cb_read_semtab_tag	*Cb_read_semtab, Cb_read_semtab_rec;
typedef struct Cb_read_line_id_tag	*Cb_read_line_id, Cb_read_line_id_rec;
typedef	struct Cb_file_read_tag		*Cb_file_read, Cb_file_read_rec;
typedef	struct Cb_file_write_tag	*Cb_file_write, Cb_file_write_rec;

/*
 *	This struct is used to represent .cb files.
 *	It contains the name of the .cb file and it's associated
 *	object and source files.
 */
struct Cb_file_tag {
	Cb_directory	directory;	/* Directory containing cb file */
	char		*sub_dir;	/* Partition with file */
	unsigned int	hash_value;	/* Hash value for file */
	char		*cb_file_name;	/* The string of the .cb file name */
	time_t		cb_file_timestamp; /* Timestamp of .cb file */
	int		size;		/* Size of the .cb file */
	Cb_src_file_im_rec src;		/* The source file described */
	char		*base_name;	/* Source file wo path */
	Array		focus;		/* Array(Cb_focus) */
	Array		funcs;		/* Array(Cb_func) */
	Array		refd_files;	/* Array of cb_files */
	Cb_lang		lang;		/* Language in .cb file */
	Cb_source_type	source_type;
	Cb_file_read	file_read;	/* Open file read object (or NULL) */
	Cb_heap		heap;
	unsigned int	fnum : 16;	/* File number (used for writing) */
	unsigned int	needed : 1;	/* TRUE => needed in new qref file */
	unsigned int	dir : 1;	/* File is in directory listing */
	unsigned int	quickref : 1 ;	/* File is in quickref file */
	unsigned int	table : 1;	/* File is in quickref table */
	unsigned int	old : 1;	/* File is old */
	unsigned int	refd_focus : 1;	/* Did we do this file yet? */
	unsigned int	local : 1;	/* File is local to index file */
	unsigned int	pending_delete : 1; /* File is .pd file */
};

/*
 * Helper structs for the file read struct
 */
struct Cb_read_refd_tag {
	Cb_refd_file_if	next;
	Cb_refd_file_if	last;
#ifdef CB_SWAP
	int		swapped;
#endif
};

struct Cb_read_symtab_tag {
	Cb_symtab_if	next;
	Cb_symtab_if	last;
#ifdef CB_SWAP
	short		swapped;
#endif
};

struct Cb_read_semtab_tag {
	Cb_semtab_if	first;
	Cb_semtab_if	next;
#ifdef CB_SWAP
	char		*swapped;
#endif
};

struct Cb_read_line_id_tag {
	Cb_line_id	next;
	Cb_line_id	last;
#ifdef CB_SWAP
	int		swapped;
#endif
};

/*
 *	This struct corresponds to FILE from stdio.
 *	It is returned from cb_read_open and passed to the reader routines.
 */
struct Cb_file_read_tag {
	Cb_file			cb_file;
	short			fd;
	char			*buffer;
	Cb_header		header;
	Cb_heap			heap;

	/*
	 * Fields used when stepping thru the various sections
	 */
	Cb_read_refd_rec	refd;
	Cb_read_symtab_rec	symtab;
	Cb_read_semtab_rec	semtab;
	Cb_read_line_id_rec	line_id;
};

/*
 *	This struct corresponds to a .cb file for a source or header file
 *	for writing purposes. Used by the compiler to collect info
 *	about one source file.
 *	We need to save the source file string name until after we
 *	have collected all the data for the file. Only then do we know the
 *	hash value so we can construct the .cb file name.
 */
struct Cb_file_write_tag {
	Cb_file		cb_file;	/* Filled in at write time */
	Cb_src_file_im_rec src;		/* As given by cpp/ccom */
	short		relative;	/* The path was relative */
	unsigned int	hashs[CB_HEADER_SECTIONS+1];
	Cb_heap		heap;		/* To allocate memory from */
	Hash		symtab;		/* Symbol => Cb_symtab_im */
	Array		refd_files;	/* Array of Cb_file */
	Array		inactive_list;	/* (Start, End) pairs */
	Array		line_id_chunks;	/* Array of Cb_line_id_chunk */
	Array		symtab_list;	/* List of symbols to output */
	Array		symrec_list;	/* List of corresponding symrec's */
	int		symtab_size;
	int		stack_depth;
	int		next_semtab_slot;
	short		enter_src_name;
	char		*node_head;	/* Header part of node name string */
	char		*include_node;
	FILE		*fd;
	Cb_header_rec	header;
	int		file_number;	/* Unique file number */
};

#endif
