/* LINTLIBRARY */

/*  @(#)cb_ex.h 1.1 94/10/31 SMI */

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


#ifndef cb_ex_h_INCLUDED
#define cb_ex_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_lang_name_h_INCLUDED
#include "cb_lang_name.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

typedef	struct Cb_ex_file_seg_tag	*Cb_ex_file_seg, Cb_ex_file_seg_rec;
typedef	struct Cb_ex_section_tag	*Cb_ex_section, Cb_ex_section_rec;
typedef	struct Cb_ex_header_tag		*Cb_ex_header, Cb_ex_header_rec;
typedef struct Cb_ex_language_tag	*Cb_ex_language, Cb_ex_language_rec;
typedef union Cb_ex_string_tag		*Cb_ex_string, Cb_ex_string_rec;
typedef struct Cb_ex_menu_sub_item_if_tag *Cb_ex_menu_sub_item_if,
						Cb_ex_menu_sub_item_if_rec;
typedef struct Cb_ex_menu_item_if_tag	*Cb_ex_menu_item_if,
						Cb_ex_menu_item_if_rec;
typedef struct Cb_ex_menu_if_tag	*Cb_ex_menu_if, Cb_ex_menu_if_rec;
typedef struct Cb_ex_focus_if_tag	*Cb_ex_focus_if, Cb_ex_focus_if_rec;
typedef struct Cb_ex_tags_im_tag	*Cb_ex_tags_im, Cb_ex_tags_im_rec;
typedef struct Cb_ex_graph_if_tag	*Cb_ex_graph_if, Cb_ex_graph_if_rec;
typedef struct Cb_ex_color_im_tag	*Cb_ex_color_im, Cb_ex_color_im_rec;
typedef struct Cb_ex_node_menu_if_tag	*Cb_ex_node_menu_if,
						Cb_ex_node_menu_if_rec;
typedef struct Cb_ex_rendering_if_tag	*Cb_ex_rendering_if,
						Cb_ex_rendering_if_rec;
typedef struct Cb_ex_file_tag		*Cb_ex_file, Cb_ex_file_rec;


#define CB_EX_FILENAME			"sun_source_browser.ex"
#define CB_EX_FILENAME_ENV_VAR		"SUN_SOURCE_BROWSER_EX_FILE"

#define CB_EX_MAGIC_NUMBER		0x3c65783e	/* "<ex>" */
#define CB_EX_VERSION_NUMBER		11
#define CB_EX_MAX_SECTION_COUNT		256

#define CB_EX_FUNNY_HEADER_BYTE		'\177'
#define CB_EX_ARC_HEADER_BYTE		'A'
#define CB_EX_FOCUS_HEADER_BYTE		'F'
#define CB_EX_NODE_HEADER_BYTE		'N'
#define CB_EX_PROP_HEADER_BYTE		'P'

/*
 * Some macros for accessing the semantic vector in the .ex file
 */
#define cb_ex_set_sem_field(v, b, f) \
	((v)[(b) / CB_EX_FIELDS_PER_BYTE] &= \
			~(0x3 << (((b) % CB_EX_FIELDS_PER_BYTE) * CB_EX_FIELD_WIDTH))); \
	((v)[(b) / CB_EX_FIELDS_PER_BYTE] |= \
			((f) << (((b) % CB_EX_FIELDS_PER_BYTE) * CB_EX_FIELD_WIDTH)))
#define cb_ex_get_sem_field(v, b) \
	((((v)[(b) / CB_EX_FIELDS_PER_BYTE]) >> \
			(((b) % CB_EX_FIELDS_PER_BYTE) * CB_EX_FIELD_WIDTH)) & 0x3)

#define CB_EX_MATCH		1
#define CB_EX_SECONDARY_MATCH	2

#define CB_EX_FIELDS_PER_BYTE	4
#define CB_EX_FIELD_WIDTH	2

struct Cb_ex_file_seg_tag {
	int		start;
	int		length;
};

/*
 *	.ex file header section
 */
enum Cb_ex_section_id_tag {
	cb_ex_first_section = 0,

	cb_ex_header_section,
	cb_ex_languages_section,
	cb_ex_weight_section,
	cb_ex_menu_section,
	cb_ex_focus_section,
	cb_ex_tags_section,
	cb_ex_comment_delimiter_section,
	cb_ex_graph_section,
	cb_ex_node_menu_section,
	cb_ex_rendering_section,

	cb_ex_last_section
};
typedef enum Cb_ex_section_id_tag	*Cb_ex_section_id_ptr,Cb_ex_section_id;

/*
 *	This struct describes one section of the .ex file.
 *	The .ex file header contains a vector of Cb_ex_section structs.
 */
struct Cb_ex_section_tag {
	Cb_ex_section_id	tag;
	int			swapped;
	Cb_lang_name		language;
	Cb_ex_file_seg_rec	location;
};

struct Cb_ex_header_tag {
	int			magic_number;
	int			version_number;
	int			section_count;
	Cb_ex_section_rec	section[1];
};

/*
 *	.ex file languages section
 *		An ordered list of all the languages known to the .ex file
 */
struct Cb_ex_language_tag {
	Cb_lang_name	name;
	short		major_version;
	short		minor_version;
	short		grep_able;
};


/*
 *	.ex file weight section
 *		A vector of weights for matches sorting per language
 */

/*
 *	.ex file menu section
 *		Declaration of the menu structure
 */
struct Cb_ex_menu_sub_item_if_tag {
	short				rec_size;
	short				language;
	short				invisible;
	short				fill;
	int				menu_string;	/* Offset */
	int				help;		/* Offset */
	char				semantics[4];
};

struct Cb_ex_menu_item_if_tag {
	int				pullright;	/* Offset */
	int				sub_items;	/* Count of items */
	Cb_ex_menu_sub_item_if_rec	sub_item[1];	/* One per language */
};

struct Cb_ex_menu_if_tag {
	short				items;		/* Count of items */
	short				swapped;	/* For 386i */
	Cb_ex_menu_item_if_rec		item[1];	/* One per menu line */
};

/*
 *	.ex file focus section.
 *		This section describes the focusing units
 */
struct Cb_ex_focus_if_tag {
	int			focus_unit_name;	/* Offset */
	int			unit_search_pattern;	/* Offset */
	int			unit_menu_item;
	short			use_refd;
	short			make_absolute;
};

/*
 *	.ex file tags section.
 *		This is just one list of tags strings per language.
 */
struct Cb_ex_tags_im_tag {
	char	*language;
	Array	tags;
};

/*
 *	.ex file comment delimiters section.
 *		This is just two nul terminated strings
 */

/*
 *	.ex file graph section.
 *		This section defines the menu used to build the menu that
 *		starts the grapher.
 */
struct Cb_ex_graph_if_tag {
	int			rec_length;
	int			menu;			/* Offset */
	int			tag;
	char			name[1];		/* Open ended string */
#ifdef mc68000
	short		fill;		/* Make size the same on sparc & m68k*/
#endif
};

/*
 *	.ex file node menu section.
 *		This is just a list of menu offset structs.
 *		Each entry contains the number of the tag is defines.
 */
struct Cb_ex_node_menu_if_tag {
	int			tag;
	int			menu;			/* Offset */
};


/*
 *	.ex file rendering section.
 *		This is just a list of rendering structs.
 *		Each entry contains the number of the tag is defines.
 */
enum Cb_ex_rendering_tag {
	cb_ex_first_rendering = 0,

	cb_ex_solid_rendering,
	cb_ex_dashed_rendering,
	cb_ex_dotted_rendering,
	cb_ex_dash_dotted_rendering,
	cb_ex_no_border_rendering,
	cb_ex_square_rendering,
	cb_ex_oval_rendering,

	cb_ex_last_rendering
};
typedef enum Cb_ex_rendering_tag	*Cb_ex_rendering_ptr, Cb_ex_rendering;

struct Cb_ex_color_im_tag {
	int	red;
	int	green;
	int	blue;
};

struct Cb_ex_rendering_if_tag {
	int			tag;
	int			rendering;	/* Should be Cb_ex_rendering */
	Cb_ex_color_im_rec	color;
};

/*
 *	An .ex file object
 */
struct Cb_ex_file_tag {
	Array			languages;
	Array			tags;
	Array			menu;
	int			sections;
	Array			focus_units;
	Array			graphs;
	Hash			node_menus;
	Cb_ex_header_rec	header;		/* must be the last record */
};

#endif
