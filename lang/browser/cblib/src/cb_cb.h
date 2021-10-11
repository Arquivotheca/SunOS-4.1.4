/* LINTLIBRARY */

/*	@(#)cb_cb.h 1.1 94/10/31 SMI	*/

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

/*
 *	.bd file layout.
 *
 *	The .bd file consists of a number of sections. The first section
 *	is always a header section that, among other things, contains
 *	a directory of all the sections in the .bd file. The sections are
 *	not required to be written in any particular order except for:
 *		The header section is always the first section in a .bd file.
 *
 *		The semantic section always follows the symbol table section.
 *
 *	The formats of the various sections are:
 *
 *	The header section:
 *		The header section is fully described by the Cb_header
 *		struct definition.
 *	The src name section:
 *		The src name section contains the name of the source file
 *		the .bd file describes. It is described by the Cb_src_file_if
 *		struct.
 *	The refd files section:
 *		This section contains a list of all files that were included
 *		when compiling the source file. It is only present for .bd
 *		files that describes sources fed directly to the compiler.
 *		Each file is stored using one Cb_refd_file_if struct.
 *	The symtab section:
 *		This section contains a list of all the symbols referenced in
 *		the source file. Each symbol points to the part of the semantic
 *		section that contains the complete description of how the
 *		symbol is used in the source file. Each symbol is stored using
 *		one Cb_symtab_if struct.
 *	The semtab section:
 *		The semantic table contains one segment for each symbol
 *		describing how and where the symbol is referenced.
 *		Each semantic entry is stored using one Cb_semtab_if struct.
 *	The line id section:
 *		This section contains the length and hash value of each line
 *		in the source file. Each line is stored using one Cb_line_id
 *		struct.
 */


#ifndef cb_cb_h_INCLUDED
#define cb_cb_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_lang_name_h_INCLUDED
#include "cb_lang_name.h"
#endif

#ifndef cb_literals_h_INCLUDED
#include "cb_literals.h"
#endif

#ifndef cb_swap_bytes_h_INCLUDED
#include "cb_swap_bytes.h"
#endif

/*
 * Typedefs:
 */
typedef	struct Cb_file_seg_tag		*Cb_file_seg, Cb_file_seg_rec;
typedef	struct Cb_section_tag		*Cb_section, Cb_section_rec;
typedef	struct Cb_header_tag		*Cb_header, Cb_header_rec;
typedef	struct Cb_src_file_if_tag	*Cb_src_file_if, Cb_src_file_if_rec;
typedef	struct Cb_src_file_im_tag	*Cb_src_file_im, Cb_src_file_im_rec;
typedef	struct Cb_refd_file_if_tag	*Cb_refd_file_if, Cb_refd_file_if_rec;
typedef	struct Cb_symtab_if_tag		*Cb_symtab_if, Cb_symtab_if_rec;
typedef	struct Cb_symtab_im_tag		*Cb_symtab_im, Cb_symtab_im_rec;
typedef	union  Cb_semtab_if_tag		*Cb_semtab_if, Cb_semtab_if_rec;


#define CB_MAX_COMPILE_TIME		(1*60*60)	/* 1 hour (sec.) */
#define	CB_MAX_COMPILE_TIME_ENV_VAR	"SB_MAX_COMPILE_TIME"

#define CB_MAGIC_NUMBER			0x3c63623e	/* "<cb>" */
#define CB_MAJOR_VERSION_NUMBER		2
#define CB_MINOR_VERSION_NUMBER		1
#define	CB_HEADER_SECTIONS		6
#define	CB_MAX_MEMORY			500000

/*
 *	Helper declarations used by the various .bd section declarations.
 */
struct Cb_file_seg_tag {
	unsigned int	start;
	unsigned int	length;
};

enum Cb_section_id_tag {
	cb_first_section = 0,

	cb_header_section,
	cb_src_name_section,
	cb_refd_files_section,
	cb_symtab_section,
	cb_semtab_section,
	cb_line_id_section,

	cb_last_section
};
typedef enum Cb_section_id_tag		*Cb_section_id_ptr, Cb_section_id;

/*
 *	This struct describes one section of the .bd file.
 *	The .bd file header contains a vector of Cb_section structs.
 */
struct Cb_section_tag {
	Cb_section_id	tag;
	Cb_file_seg_rec location;
};

/*
 *	The .bd file header section
 */
enum Cb_source_type_tag {
	cb_no_source_type,

	cb_root_source_type,
	cb_refd_source_type,		/* .h */
	cb_bogus_source_type,
	cb_multiple_source_type,	/* ar, ranlib, & ld */

	cb_last_source_type
};
typedef enum Cb_source_type_tag		*Cb_source_type_ptr, Cb_source_type;

struct Cb_header_tag {
	int		magic_number;
	Cb_source_type	source_type;
	Cb_lang_name	language_name;
	short		major_version_number;
	short		minor_version_number;
	short		language_major_version;
	short		language_minor_version;
	short		pound_line_seen;
	short		section_count;
	short		case_was_folded;
	short		unused_filler;
	char		fill[12];	/* For future expansion */
	Cb_section_rec	section[CB_HEADER_SECTIONS];
};

/*
 *	The .bd file src_name section
 *	The name of the source file described by the .bd file.
 */
struct Cb_src_file_if_tag {
	short		relative;
	char		name[2];
};
struct Cb_src_file_im_tag {
	short		relative;
	char		*name;
};

/*
 *	The .bd file refd files section
 *	Used for the list of files included by the source file described
 *	by the .bd file.
 */
struct Cb_refd_file_if_tag {
	unsigned int	hash_value;
	char		refd_name[1];	/* Absolute path to source file */
#ifdef mc68000
	short		fill;		/* Make size the same on sparc & m68k*/
#endif
};

/*
 *	The .bd file symtab section
 *	One entry for each symbol described by the .bd file.
 */
struct Cb_symtab_if_tag {
	unsigned int	semtab;		/* Location of symbols semantic info */
	char		name[1];
#ifdef mc68000
	short		fill;		/* Make size the same on sparc & m68k*/
#endif
};
struct Cb_symtab_im_tag {
	Array		semtab;		/* Semantic info is collected here */
	int		semseg_length;	
};

/*
 *	The .bd file semtab section
 */
enum Cb_semrec_size_tag {
	cb_semrec_usage_width = 12,
	cb_semrec_line_number_width = 18,
	cb_semrec_type_width = 2,

	cb_semrec_non_type_width = 30,	/* word width - cb_semrec_type_width */
};
typedef enum Cb_semrec_size_tag		*Cb_semrec_size_ptr, Cb_semrec_size;

enum Cb_semtab_tag_tag {
	cb_name_ref_semrec = 1,
	cb_end_semrec,
};
typedef enum Cb_semtab_tag_tag		*Cb_semtab_tag_ptr, Cb_semtab_tag;

union Cb_semtab_if_tag {
	struct Name_ref_tag {
#ifdef CB_SWAP
		unsigned int	usage:		cb_semrec_usage_width;
		unsigned int	line_number:	cb_semrec_line_number_width;
		unsigned int	type:		cb_semrec_type_width;
#else
		unsigned int	type:		cb_semrec_type_width;
		unsigned int	line_number:	cb_semrec_line_number_width;
		unsigned int	usage:		cb_semrec_usage_width;
#endif
	} name_ref;
	struct End_rec_tag {
#ifdef CB_SWAP
		unsigned int	unused:		cb_semrec_non_type_width;
		unsigned int	type:		cb_semrec_type_width;
#else
		unsigned int	type:		cb_semrec_type_width;
		unsigned int	unused:		cb_semrec_non_type_width;
#endif
	} end;
	int swap;
};

#endif

