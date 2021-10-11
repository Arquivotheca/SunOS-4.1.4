/* LINTLIBRARY */

/*  @(#)cb_extend.h 1.1 94/10/31 SMI */

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


#ifndef cb_extend_h_INCLUDED
#define cb_extend_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_ex_h_INCLUDED
#include "cb_ex.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#define CB_EX_DEFAULT_DEFAULT_WEIGHT 64
typedef struct Cb_ex_enter_menu_item_tag	*Cb_ex_enter_menu_item,
						 Cb_ex_enter_menu_item_rec;
typedef struct Cb_ex_enter_menu_sub_item_tag	*Cb_ex_enter_menu_sub_item,
						 Cb_ex_enter_menu_sub_item_rec;
typedef struct Cb_ex_enter_focus_unit_tag	*Cb_ex_enter_focus_unit,
						 Cb_ex_enter_focus_unit_rec;
typedef struct Cb_ex_tags_item_tag		*Cb_ex_tags_item,
						 Cb_ex_tags_item_rec;
typedef struct Cb_ex_props_item_tag		*Cb_ex_props_item,
						 Cb_ex_props_item_rec;
typedef struct Cb_ex_equiv_tag			*Cb_ex_equiv, Cb_ex_equiv_rec;
typedef struct Cb_ex_menu_item_tag		*Cb_ex_menu_item,
						 Cb_ex_menu_item_rec;
typedef struct Cb_ex_enter_graph_tag		*Cb_ex_enter_graph,
						 Cb_ex_enter_graph_rec;
typedef struct Cb_ex_enter_sub_graph_tag	*Cb_ex_enter_sub_graph,
						 Cb_ex_enter_sub_graph_rec;
typedef struct Cb_ex_enter_rendering_tag	*Cb_ex_enter_rendering,
						 Cb_ex_enter_rendering_rec;
typedef struct Cb_ex_enter_node_tag		*Cb_ex_enter_node,
						 Cb_ex_enter_node_rec;

enum Cb_ex_start_stop_tag {
	cb_ex_first_start_stop,

	cb_ex_none_start_stop,
	cb_ex_zero_start_stop,
	cb_ex_inf_start_stop,
	cb_ex_regular_start_stop,

	cb_ex_last_start_stop
};
typedef enum Cb_ex_start_stop_tag		*Cb_ex_start_stop_ptr,
						 Cb_ex_start_stop;

/*
 *	When a menu item is entered it is matched with any equivalent
 *	menu items already entered.
 *	Each line in a menu is represented by one Cb_ex_enter_menu_item.
 *	Each such has an array of Cb_ex_enter_menu_sub_item, one for
 *	each equive'd menu item from the .ext file
 */
struct Cb_ex_enter_menu_item_tag {
	Array		sub_items;
	Array		pullright;
	int		offset;
	Array		focus;
};

struct Cb_ex_enter_menu_sub_item_tag {
	char		*menu_label;
	int		invisible;
	char		*help;
	char		*language;
	Array		props;
};

/*
 *	Focusing unit
 */
struct Cb_ex_enter_focus_unit_tag {
	char				*focus_unit_name;
	short				make_absolute;
	Cb_ex_start_stop		start;
	Cb_ex_start_stop		stop;
	int				use_refd;
	char				*search_pattern;
	Cb_ex_enter_menu_item		menu_item;
	char				*tag;
	Cb_ex_props_item		prop;
	Cb_ex_equiv			equiv;
	short				invisible;
};

struct Cb_ex_tags_item_tag {
	char				*name;
	char				*prefix;
	char				*full_name;
	int				weight;
	Array				secondary;
	Hash				props;
	Cb_ex_enter_focus_unit		focus_unit;
	Cb_ex_props_item		focus_prop;
	int				ordinal;
};

struct Cb_ex_props_item_tag {
	char				*language;
	char				*prop;
};

struct Cb_ex_equiv_tag {
	char				*language;
	char				*label;
};

struct Cb_ex_menu_item_tag {
	char				*menu_label;
	int				invisible;
	Cb_ex_equiv			equiv;
	Array				props;
	char				*help;
	Array				pullright;
	Array				focus;
};

struct Cb_ex_enter_graph_tag {
	int				menu_offset;
	char				*graph_name;
	char				*graph_tag;
	Array				menus;
};

struct Cb_ex_enter_sub_graph_tag {
	Array				menu;
	Hash				language;
};

struct Cb_ex_enter_rendering_tag {
	Cb_ex_tags_item			tag;
	Cb_ex_rendering			rendering;
	Cb_ex_color_im			color;
};

struct Cb_ex_enter_node_tag {
	Cb_ex_tags_item			tag;
	Array				menu;
	int				menu_offset;
	Hash				language;
	Array				equiv_list;
	char				*equiv_name;
};


PORT_EXTERN(void		cb_ex_default_equiv_language_statement(
					PORT_0ARG));
PORT_EXTERN(void		cb_ex_graph_statement(PORT_3ARG(char*,
								char*,
								Array)));
PORT_EXTERN(void		cb_ex_invisible_statement(PORT_0ARG));
PORT_EXTERN(void		cb_ex_language_statement(PORT_4ARG(char*,
								   char*,
								   char*,
								   int)));
PORT_EXTERN(Cb_ex_enter_menu_item cb_ex_menu_statement(PORT_1ARG(Array)));
PORT_EXTERN(Cb_ex_menu_item	cb_ex_menu_regular_item(
					PORT_5ARG(char*,
						  Array,
						  int,
						  char*,
						  Cb_ex_equiv)));
PORT_EXTERN(Cb_ex_menu_item	cb_ex_menu_pullright_item(
					PORT_5ARG(char*,
						  Array,
						  int,
						  char*,
						  Cb_ex_equiv)));
PORT_EXTERN(Cb_ex_equiv		cb_ex_menu_equiv_spec(PORT_2ARG(char*,
								char*)));
PORT_EXTERN(void		cb_ex_node_statement(PORT_2ARG(char*, Array)));
PORT_EXTERN(void		cb_ex_properties_statement(PORT_2ARG(char*,
								     Array)));
PORT_EXTERN(Cb_ex_props_item	cb_ex_property(PORT_2ARG(char*, char*)));
PORT_EXTERN(void		cb_ex_tags_statement(PORT_3ARG(Array,
							       char*,
							       int)));
PORT_EXTERN(Cb_ex_tags_item	cb_ex_tags_item(PORT_4ARG(char*,
							  int,
							  Array,
							  Array)));
PORT_EXTERN(void		cb_ex_ref_focus_unit_statement(
					PORT_1ARG(char*)));
PORT_EXTERN(Cb_ex_tags_item	cb_ex_def_focus_unit(
					PORT_8ARG(char*,
						  char*,
						  Array,
						  int,
						  int,
						  int,
						  Cb_ex_start_stop,
						  Cb_ex_start_stop)));
PORT_EXTERN(void		cb_ex_render_statement(PORT_3ARG(char*,
								 Cb_ex_rendering,
								 Cb_ex_color_im)));
PORT_EXTERN(Cb_ex_color_im	cb_ex_color(PORT_3ARG(int, int, int)));
PORT_EXTERN(void		cb_ex_version_statement(PORT_1ARG(int)));
PORT_EXTERN(void		cb_ex_write_files(PORT_1ARG(char*)));
PORT_EXTERN(void		cb_ex_fatal(PORT_2ARG(char*, PORT_MANY_ARG)));
PORT_EXTERN(void		cb_ex_warning(PORT_2ARG(char*,
							PORT_MANY_ARG)));
PORT_EXTERN(void		cb_ex_init(PORT_0ARG));
PORT_EXTERN(Array		cb_ex_first_list_item(PORT_1ARG(int)));
PORT_EXTERN(Array		cb_ex_next_list_item(PORT_2ARG(Array, int)));
PORT_EXTERN(void		cb_ex_insert_menu_statement(PORT_3ARG(char*,
								      Array,
								      Array)));
#endif
