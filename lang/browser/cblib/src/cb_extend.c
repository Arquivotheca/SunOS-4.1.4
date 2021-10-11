#ident "@(#)cb_extend.c 1.1 94/10/31"

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
#endif


#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef CB_BOOT
#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif
#endif

#ifndef cb_ex_write_h_INCLUDED
#include "cb_ex_write.h"
#endif

#ifndef cb_extend_h_INCLUDED
#include "cb_extend.h"
#endif

#ifndef CB_BOOT
#ifndef cb_extend_tags_h_INCLUDED
#include "cb_extend_tags.h"
#endif
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_menu_strings_h_INCLUDED
#include "cb_menu_strings.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

#include "cb_ex_parse.h"

static	Hash		languages = NULL;
static	int		version = -1;
	Hash		cb_ex_language = NULL;
static	Hash		tag_check = NULL;
static	Cb_ex_file	ex_enter_file;
static	char		*write_h_files;
static	Hash		focus_units = NULL;
static	Hash		unit_tags = NULL;
static	Array		focus_tags = NULL;
static	int		focus_size = -1;
	int		cb_ex_extra_menu;
static	int		cb_ex_fatal_warning = 0;
extern	Hash		semantic_widths;

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void			cb_ex_default_equiv_language_statement();
extern	void			cb_ex_graph_statement();
extern	void			cb_ex_invisible_statement();
extern	void			cb_ex_language_statement();
extern	void			cb_ex_insert_menu_statement();
extern	void			cb_ex_insert_menu_item();
extern	Cb_ex_enter_menu_item	cb_ex_menu_statement();
extern	Cb_ex_enter_menu_item	cb_ex_enter_menu_item();
extern	Cb_ex_menu_item		cb_ex_menu_regular_item();
extern	Cb_ex_menu_item		cb_ex_menu_pullright_item();
extern	Cb_ex_equiv		cb_ex_get_equiv_default();
extern	Cb_ex_equiv		cb_ex_menu_equiv_spec();
extern	void			cb_ex_node_statement();
extern	void			cb_ex_properties_statement();
extern	Cb_ex_props_item	cb_ex_property();
extern	void			cb_ex_add_focus_tags();
extern	int			cb_ex_focus_tag_sort();
extern	void			cb_ex_add_fill_tag();
extern	void			cb_ex_tags_statement();
extern	void			cb_ex_check_tag_duplicated();
extern	Cb_ex_tags_item		cb_ex_tags_item();
extern	void			cb_ex_ref_focus_unit_statement();
extern	int			cb_ex_ref_focus_unit_make_list();
extern	Cb_ex_tags_item		cb_ex_def_focus_unit();
extern	void			cb_ex_render_statement();
extern	Cb_ex_color_im		cb_ex_color();
extern	void			cb_ex_version_statement();
extern	int			cb_ex_prop_equal();
extern	int			cb_ex_prop_hash();
extern	void			cb_ex_write_files();
extern	int			cb_ex_write_tags_file();
extern	void			cb_ex_write_focus_unit_pattern();
extern	char			*cb_ex_lineno_pattern();
extern	int			cb_ex_different();
extern	void			cb_ex_fatal();
extern	void			cb_ex_warning();
extern	void			cb_ex_init();
extern	void			cb_ex_check_language();
extern	Array			cb_ex_first_list_item();
extern	Array			cb_ex_next_list_item();

/*
 *	cb_ex_default_equiv_language_statement()
 */
void
cb_ex_default_equiv_language_statement()
{
	char		*language_name;

	cb_ex_check_language();

	language_name = (char *)hsh_lookup(cb_ex_language,
					   (Hash_value)EX_LANGUAGE);

	if (hsh_lookup(languages, (Hash_value)EX_DEFAULT_EQUIV_LANGUAGE)) {
		cb_ex_fatal("Redeclaration of default-equiv-language\n");
	} else {
		(void)hsh_insert(languages,
				(Hash_value)EX_DEFAULT_EQUIV_LANGUAGE,
				(Hash_value)language_name);
	}
}

/*
 *	cb_ex_graph_statement(graph_name, tag, menu)
 */
void
cb_ex_graph_statement(graph_name, tag, menu)
	char		*graph_name;
	char		*tag;
	Array		menu;
{
	int			i;
	Cb_ex_enter_graph	graph;
	Cb_ex_enter_sub_graph	sub_graph;
	Cb_ex_enter_graph	duplicate = NULL;

	cb_ex_check_language();

	if (strlen(tag) != 1) {
		cb_ex_fatal("Tag for graph statement must be one char long `%s'\n",
			    tag);
	}

	if (ex_enter_file->graphs == NULL) {
		ex_enter_file->sections++;
		ex_enter_file->graphs = array_create((Cb_heap)NULL);
	}

	for (i = array_size(ex_enter_file->graphs)-1; i >= 0; i--) {
		graph = (Cb_ex_enter_graph)ARRAY_FETCH(ex_enter_file->graphs,
						       i);

		if (graph->graph_name == graph_name) {
			duplicate = graph;
			break;
		}
	}

	if (duplicate == NULL) {
		graph = cb_allocate(Cb_ex_enter_graph, (Cb_heap)NULL);
		ex_enter_file->sections++;
		graph->menus = array_create((Cb_heap)NULL);
		graph->menu_offset = -1;
		graph->graph_name = graph_name;
		graph->graph_tag = tag;
		array_append(ex_enter_file->graphs, (Array_value)graph);
	} else {
		graph = duplicate;
		if (strcmp(graph->graph_tag, tag)) {
			cb_ex_fatal("graph statement tag clash `%s' vs. `%s'\n",
				    tag,
				    graph->graph_tag);
		}
	}
	sub_graph = cb_allocate(Cb_ex_enter_sub_graph, (Cb_heap)NULL);
	array_append(graph->menus, (Array_value)sub_graph);
	sub_graph->language = cb_ex_language;
	sub_graph->menu = menu;
}

/*
 *	cb_ex_invisible_statement()
 *		Do not write a tags file or menu spec for this language
 */
void
cb_ex_invisible_statement()
{
	cb_ex_check_language();

	(void)hsh_insert(cb_ex_language,
			 (Hash_value)EX_INVISIBLE,
			 (Hash_value)EX_INVISIBLE);
}

/*
 *	cb_ex_language_statement(language,start_comment,end_comment,grep_able)
 *		languages is a hash table that contains hash tables.
 *		Each such table contains info about that language.
 */
void
cb_ex_language_statement(language_name, start_comment, end_comment, grep_able)
	char	*language_name;
	char	*start_comment;
	char	*end_comment;
	int	grep_able;
{
	focus_tags = NULL;

	if (strlen(language_name) >= CB_LANG_NAME_SIZE) {
		cb_ex_fatal("Language name `%s' is too long. (Max = %d)\n",
			    language_name,
			    CB_LANG_NAME_SIZE);
	}
	if (languages == NULL) {
		languages = hsh_create(100,
				       hsh_int_equal,
				       hsh_int_hash,
				       (Hash_value)NULL,
				       (Cb_heap)NULL);
		if (version > 0) {
			(void)hsh_insert(languages,
					 (Hash_value)EX_VERSION,
					 (Hash_value)version);
		}
	}
	cb_ex_language = (Hash)hsh_lookup(languages,
					  (Hash_value)language_name);
	if (cb_ex_language != NULL) {
		cb_ex_fatal("Language `%s' redefined\n",
			    language_name);
	}
	cb_ex_language = hsh_create(100,
				    hsh_int_equal,
				    hsh_int_hash,
				    (Hash_value)NULL,
				    (Cb_heap)NULL);
	(void)hsh_insert(cb_ex_language,
			 (Hash_value)EX_LANGUAGE,
			 (Hash_value)language_name);
	if (start_comment != NULL) {
		ex_enter_file->sections++;
	}
	(void)hsh_insert(cb_ex_language,
			 (Hash_value)EX_LANGUAGE_START_COMMENT,
			 (Hash_value)start_comment);
	(void)hsh_insert(cb_ex_language,
			 (Hash_value)EX_LANGUAGE_END_COMMENT,
			 (Hash_value)end_comment);
	if (grep_able) {
		(void)hsh_insert(cb_ex_language,
				 (Hash_value)EX_GREP_ABLE,
				 (Hash_value)EX_GREP_ABLE);
	}
	(void)hsh_insert(languages,
			 (Hash_value)language_name,
			 (Hash_value)cb_ex_language);
}

/*
 *	cb_ex_insert_menu_statement(language, path, menu)
 */
void
cb_ex_insert_menu_statement(language, path, menu)
	char		*language;
	Array		path;
	Array		menu;
{
	Hash		save_language = cb_ex_language;
	int		i;
	Cb_ex_menu_item	menu_item;
	Array		props;

	cb_ex_language = (Hash)hsh_lookup(languages,
					  (Hash_value)language);
	if (cb_ex_language == NULL) {
		cb_ex_fatal("Language `%s' not defined\n", language);
	}

	props = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_PROPERTIES);
	for (i = 0; i < array_size(menu); i++) {
		menu_item = (Cb_ex_menu_item)ARRAY_FETCH(menu, i);
		cb_ex_insert_menu_item(path,
				       language,
				       props,
				       menu_item);
	}
	cb_ex_language = save_language;
}

/*
 *	cb_ex_insert_menu_item(path, language, props, menu_item)
 */
static void
cb_ex_insert_menu_item(path, language, props, menu_item)
	Array				path;
	char				*language;
	Array				props;
	Cb_ex_menu_item			menu_item;
{
	Array				menu;
	int				i;
	int				j;
	int				k;
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_item		ritem;
	Cb_ex_enter_menu_sub_item	sub_item;
	Cb_ex_enter_menu_sub_item	rsub_item;
	Cb_ex_props_item		prop;
	Cb_ex_props_item		rprop;
	Array				props_list;
	char				*label;

	/*
	 * Traverse the path to find the menu we want to insert things into
	 */
	menu = ex_enter_file->menu;
	for (i = 0; i < array_size(path)-1; i++) {
		label = (char *)ARRAY_FETCH(path, i);
		if (menu == NULL) {
			cb_ex_fatal("insert-menu-items path invalid\n");
		}
		for (j = 0; j < array_size(menu); j++) {
			ritem = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, j);
			for (k = 0; k < array_size(ritem->sub_items); k++) {
				rsub_item = (Cb_ex_enter_menu_sub_item)
				  ARRAY_FETCH(ritem->sub_items, k);
				if ((rsub_item->language == language) &&
				    (rsub_item->menu_label == label)) {
					menu = ritem->pullright;
					goto found_label1;
				}
			}
		}
		cb_ex_fatal("insert-menu-items path invalid\n");
	found_label1:;
	}

	if (menu == NULL) {
		ritem->pullright = menu = array_create((Cb_heap)NULL);
	}

	/*
	 * Check if this item is equivalent with anything
	 */
	if (menu_item->equiv != NULL) {
	    for (i = array_size(menu)-1; i >= 0; i--) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);
		for (j = array_size(item->sub_items)-1; j >= 0; j--) {
			sub_item = (Cb_ex_enter_menu_sub_item)
					ARRAY_FETCH(item->sub_items, j);
			if ((sub_item->menu_label == menu_item->equiv->label)&&
			    (sub_item->language == menu_item->equiv->language)){
				/*
				 * The new menu item has equiv that matched
				 */
				sub_item = cb_allocate(Cb_ex_enter_menu_sub_item,
							(Cb_heap)NULL);
				sub_item->menu_label = menu_item->menu_label;
				sub_item->invisible = menu_item->invisible;
				sub_item->help = menu_item->help;
				sub_item->language = language;
				sub_item->props = menu_item->props;
				array_append(item->sub_items,
					     (Array_value)sub_item);
				goto pullrights;
			}
		}
	    }
	    cb_ex_fatal("Equiv statement did not match anything. Item = %s, language = %s\n",
			menu_item->menu_label, language);
	}

	/*
	 * Check for duplicate menu entries
	 */
	for (i = array_size(menu)-1; i >= 0; i--) {
		ritem = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);
		if (ritem->sub_items == NULL) {
			continue;
		}
		for (j = array_size(ritem->sub_items)-1; j >= 0; j--) {
			rsub_item = (Cb_ex_enter_menu_sub_item)
					ARRAY_FETCH(ritem->sub_items, j);

			if (rsub_item->menu_label == menu_item->menu_label) {
				if (cb_ex_extra_menu) {
					/*
					 * The new menu item item has a sister
					 */
					sub_item = cb_allocate(Cb_ex_enter_menu_sub_item,
							       (Cb_heap)NULL);
					sub_item->menu_label =
					  menu_item->menu_label;
					sub_item->invisible =
					  menu_item->invisible;
					sub_item->help = menu_item->help;
					sub_item->language = language;
					sub_item->props = menu_item->props;
					array_append(item->sub_items,
						     (Array_value)sub_item);
				}
				item = ritem;
				goto pullrights;
			}
		}
	}

	/*
	 * No equiv statement. New entry.
	 */
	item = cb_allocate(Cb_ex_enter_menu_item, (Cb_heap)NULL);
	item->sub_items = array_create((Cb_heap)NULL);
	sub_item = cb_allocate(Cb_ex_enter_menu_sub_item,(Cb_heap)NULL);
	array_append(item->sub_items, (Array_value)sub_item);
	sub_item->menu_label = menu_item->menu_label;
	sub_item->invisible = menu_item->invisible;
	sub_item->help = menu_item->help;
	sub_item->language = language;
	sub_item->props = menu_item->props;
	item->pullright = NULL;
	item->offset = -1;

	/*
	 * Figure out where to insert the item
	 */
	label = (char *)ARRAY_FETCH(path, array_size(path)-1);
	if (label[0] == 0) {
		array_append(menu, (Array_value)item);
	} else {
		for (j = 0; j < array_size(menu); j++) {
			ritem = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, j);
			for (k = 0; k < array_size(ritem->sub_items); k++) {
				rsub_item = (Cb_ex_enter_menu_sub_item)
				  ARRAY_FETCH(ritem->sub_items, k);
				if ((rsub_item->language == language) &&
				    (rsub_item->menu_label == label)) {
					goto found_label2;
				}
			}
		}
		cb_ex_fatal("insert-menu-items path invalid\n");
	found_label2:
		array_insert(menu, j+1, (Array_value)item);
	}

 pullrights:
	item->focus = menu_item->focus;
	if (menu_item->props != NULL) {
	  for (k = array_size(menu_item->props)-1; k >= 0; k--) {
	    props_list = (Array)ARRAY_FETCH(menu_item->props, k);
	    /*
	     * Make sure all props have a language assigned to them
	     */
	    for (i = array_size(props_list)-1; i >= 0; i--) {
		prop = (Cb_ex_props_item)ARRAY_FETCH(props_list, i);
		if (prop->language == NULL) {
			prop->language = language;
		}
		/*
		 * Make sure the prop is defined for the language
		 */
		for (j = array_size(props)-1; j >= 0; j--) {
			rprop = (Cb_ex_props_item)ARRAY_FETCH(props, j);
			if ((rprop->language == prop->language) &&
			    (rprop->prop == prop->prop)) {
				goto found_it;
			}
		}
		cb_ex_fatal("Property `%s' in menu item `%s' for language `%s' not defined\n",
			prop->prop,
			menu_item->menu_label,
			language);
	    found_it:;
	    }
	  }
	}

	ritem = item;
	/*
	 * And do the sub menus
	 */
	if (menu_item->pullright == NULL) {
		return;
	}

	array_store(path,
		    array_size(path)-1,
		    (Array_value)menu_item->menu_label);
	for (i = 0; i < array_size(menu_item->pullright); i++) {
		array_append(path, (Array_value)"");
		cb_ex_insert_menu_statement(language,
					    path,
					    menu_item->pullright);
		array_trim(path, array_size(path)-1);
	}
}

/*
 *	cb_ex_menu_statement(menu)
 */
Cb_ex_enter_menu_item
cb_ex_menu_statement(menu)
	Array				menu;
{
	int				i;
	Cb_ex_enter_menu_item		item;

	cb_ex_add_focus_tags();

	cb_ex_check_language();

	for (i = 0; i < array_size(menu); i++) {
		if (ex_enter_file->menu == NULL) {
			ex_enter_file->sections++;
			if (ex_enter_file->sections >= CB_EX_MAX_SECTION_COUNT) {
				cb_ex_fatal(".ex file section overflow\n");
			}
		}
		item = cb_ex_enter_menu_item(&(ex_enter_file->menu),
					     (char *)hsh_lookup(cb_ex_language,
								(Hash_value)EX_LANGUAGE),
					     (Array)hsh_lookup(cb_ex_language,
							       (Hash_value)EX_PROPERTIES),
					     (Cb_ex_menu_item)ARRAY_FETCH(menu,
									  i));
	}
	return item;
}

/*
 *	cb_ex_enter_menu_item(ex_file, language, props, menu_item)
 */
static Cb_ex_enter_menu_item
cb_ex_enter_menu_item(menu, language, props, menu_item)
	Array				*menu;
	char				*language;
	Array				props;
	Cb_ex_menu_item			menu_item;
{
	int				i;
	int				j;
	int				k;
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_item		ritem;
	Cb_ex_enter_menu_sub_item	sub_item;
	Cb_ex_enter_menu_sub_item	rsub_item;
	Cb_ex_props_item		prop;
	Cb_ex_props_item		rprop;
	Array				props_list;

	if (*menu == NULL) {
		*menu = array_create((Cb_heap)NULL);
		
	}

	/*
	 * Check if this item is equivalent with anything
	 */
	if (menu_item->equiv != NULL) {
	    for (i = array_size(*menu)-1; i >= 0; i--) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(*menu, i);
		for (j = array_size(item->sub_items)-1; j >= 0; j--) {
			sub_item = (Cb_ex_enter_menu_sub_item)
					ARRAY_FETCH(item->sub_items, j);
			if ((sub_item->menu_label == menu_item->equiv->label)&&
			    (sub_item->language == menu_item->equiv->language)){
				/*
				 * The new menu item has equiv that matched
				 */
				sub_item = cb_allocate(Cb_ex_enter_menu_sub_item,
							(Cb_heap)NULL);
				sub_item->menu_label = menu_item->menu_label;
				sub_item->invisible = menu_item->invisible;
				sub_item->help = menu_item->help;
				sub_item->language = language;
				sub_item->props = menu_item->props;
				array_append(item->sub_items,
					     (Array_value)sub_item);
				goto pullrights;
			}
		}
	    }
	    cb_ex_fatal("Equiv statement did not match anything. Item = %s, language = %s\n",
			menu_item->menu_label, language);
	}

	/*
	 * Check for duplicate menu entries
	 */
	for (i = array_size(*menu)-1; i >= 0; i--) {
		ritem = (Cb_ex_enter_menu_item)ARRAY_FETCH(*menu, i);
		if (ritem->sub_items == NULL) {
			continue;
		}
		for (j = array_size(ritem->sub_items)-1; j >= 0; j--) {
			rsub_item = (Cb_ex_enter_menu_sub_item)
					ARRAY_FETCH(ritem->sub_items, j);

			if (rsub_item->menu_label == menu_item->menu_label) {
				if (cb_ex_extra_menu) {
					/*
					 * The new menu item item has a sister
					 */
					sub_item = cb_allocate(Cb_ex_enter_menu_sub_item,
							       (Cb_heap)NULL);
					sub_item->menu_label =
					  menu_item->menu_label;
					sub_item->invisible =
					  menu_item->invisible;
					sub_item->help = menu_item->help;
					sub_item->language = language;
					sub_item->props = menu_item->props;
					item = ritem;
					array_append(item->sub_items,
						     (Array_value)sub_item);
					goto pullrights;
				}
				cb_ex_warning("Duplicate menu item `%s' for languages `%s'/`%s'\n",
					rsub_item->menu_label,
					rsub_item->language,
					language);
				cb_ex_fatal_warning = 1;
			}
		}
	}

	/*
	 * No equiv statement. New entry.
	 */
	item = cb_allocate(Cb_ex_enter_menu_item, (Cb_heap)NULL);
	item->sub_items = array_create((Cb_heap)NULL);
	sub_item = cb_allocate(Cb_ex_enter_menu_sub_item,(Cb_heap)NULL);
	array_append(item->sub_items, (Array_value)sub_item);
	sub_item->menu_label = menu_item->menu_label;
	sub_item->invisible = menu_item->invisible;
	sub_item->help = menu_item->help;
	sub_item->language = language;
	sub_item->props = menu_item->props;
	item->pullright = NULL;
	item->offset = -1;

	array_append(*menu, (Array_value)item);

    pullrights:
	item->focus = menu_item->focus;
	if (menu_item->props != NULL) {
	  for (k = array_size(menu_item->props)-1; k >= 0; k--) {
	    props_list = (Array)ARRAY_FETCH(menu_item->props, k);
	    /*
	     * Make sure all props have a language assigned to them
	     */
	    for (i = array_size(props_list)-1; i >= 0; i--) {
		prop = (Cb_ex_props_item)ARRAY_FETCH(props_list, i);
		if (prop->language == NULL) {
			prop->language = language;
		}
		/*
		 * Make sure the prop is defined for the language
		 */
		for (j = array_size(props)-1; j >= 0; j--) {
			rprop = (Cb_ex_props_item)ARRAY_FETCH(props, j);
			if ((rprop->language == prop->language) &&
			    (rprop->prop == prop->prop)) {
				goto found_it;
			}
		}
		cb_ex_fatal("Property `%s' in menu item `%s' for language `%s' not defined\n",
			prop->prop,
			menu_item->menu_label,
			language);
	    found_it:;
	    }
	  }
	}

	ritem = item;
	/*
	 * And do the sub menus
	 */
	if (menu_item->pullright == NULL) {
		return ritem;
	}

	for (i = 0; i < array_size(menu_item->pullright); i++) {
		ritem = cb_ex_enter_menu_item(&(item->pullright),
					     language,
					     props,
					     (Cb_ex_menu_item)
					     ARRAY_FETCH(menu_item->pullright,
							 i));
	}
	return ritem;
}

/*
 *	cb_ex_menu_regular_item(menu_label, props_list, invisible, help, equiv)
 */
Cb_ex_menu_item
cb_ex_menu_regular_item(menu_label, props_list, invisible, help, equiv)
	char			*menu_label;
	Array			props_list;
	int			invisible;
	char			*help;
	Cb_ex_equiv		equiv;
{
	Cb_ex_menu_item		item;

	item = cb_allocate(Cb_ex_menu_item, (Cb_heap)NULL);
	item->menu_label = menu_label;
	item->invisible = invisible;
	item->equiv = cb_ex_get_equiv_default(equiv, menu_label);
	item->props = props_list;
	item->help = help;
	item->pullright = NULL;
	item->focus = NULL;
	return item;
}

/*
 *	cb_ex_menu_pullright_item(menu_label, pullright, invisible, help,equiv)
 */
Cb_ex_menu_item
cb_ex_menu_pullright_item(menu_label, pullright, invisible, help, equiv)
	char			*menu_label;
	Array			pullright;
	int			invisible;
	char			*help;
	Cb_ex_equiv		equiv;
{
	Cb_ex_menu_item		item;

	item = cb_allocate(Cb_ex_menu_item, (Cb_heap)NULL);
	item->menu_label = menu_label;
	item->invisible = invisible;
	item->equiv = cb_ex_get_equiv_default(equiv, menu_label);
	item->props = NULL;
	item->help = help;
	item->pullright = pullright;
	item->focus = NULL;
	return item;
}

/*
 *	cb_ex_get_equiv_default(equiv, menu_label)
 *		Supply default values for equiv statement
 */
static Cb_ex_equiv
cb_ex_get_equiv_default(equiv, menu_label)
	Cb_ex_equiv	equiv;
	char		*menu_label;
{
	if (equiv == NULL) {
		return equiv;
	}
	if (equiv->label == NULL) {
		equiv->label = menu_label;
	}
	if (equiv->language == NULL) {
		equiv->language =
			(char *)hsh_lookup(languages,
					   (Hash_value)EX_DEFAULT_EQUIV_LANGUAGE);
		if (equiv->language == NULL) {
			cb_ex_fatal("No language for equiv `%s'\n",
				equiv->label);
		}
	}
	return equiv;
}

/*
 *	cb_ex_menu_equiv_spec(language, label)
 */
Cb_ex_equiv
cb_ex_menu_equiv_spec(language, label)
	char		*language;
	char		*label;
{
	Cb_ex_equiv	item;

	if ((language != NULL) && (strlen(language) >= CB_LANG_NAME_SIZE)) {
		cb_ex_fatal("Language name `%s' too long. (Max = %d)\n",
			language,
			CB_LANG_NAME_SIZE);
	}
	item = cb_allocate(Cb_ex_equiv, (Cb_heap)NULL);
	item->language = language;
	item->label = label;
	return item;
}

/*
 *	cb_ex_node_statement(tag_name, equiv_name, menu)
 */
void
cb_ex_node_statement(tag_name, equiv_name, menu)
	char		*tag_name;
	char		*equiv_name;
	Array		menu;
{
	Array			master;
	Array			tags;
	Cb_ex_tags_item		tag;
	int			i;
	Cb_ex_enter_node	node;
	char			*buffer;

	cb_ex_check_language();

	ex_enter_file->sections++;
	master = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_NODE);
	if (master == NULL) {
		ex_enter_file->sections++;
		(void)hsh_insert(cb_ex_language,
				 (Hash_value)EX_NODE,
				 (Hash_value)(master =
					      array_create((Cb_heap)NULL)));
	}

	tags = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_TAGS);
	if (tags == NULL) {
		cb_ex_fatal("No tags defined when parsing node statement\n");
	}

	for (i = array_size(tags)-1; i >= 0; i--) {
		tag = (Cb_ex_tags_item)ARRAY_FETCH(tags, i);

		if (tag->full_name == NULL) {
			buffer = alloca(strlen(tag->prefix) +
					strlen(tag->name) + 32);
			(void)sprintf(buffer, "%s_%s", tag->prefix, tag->name);
			tag->full_name = cb_string_unique(buffer);
		}
		if (tag->full_name == tag_name) {
			goto found_it;
		}
	}
	cb_ex_fatal("Could not find tag `%s'\n", tag_name);
 found_it:
	for (i = array_size(master)-1; i >= 0; i--) {
		node = (Cb_ex_enter_node)ARRAY_FETCH(master, i);

		if (node->tag == tag) {
			cb_ex_fatal("Tag `%s' has more than one node menu\n",
				    tag->full_name);
		}
	}

	node = cb_allocate(Cb_ex_enter_node, (Cb_heap)NULL);
	node->tag = tag;
	node->menu = menu;
	node->menu_offset = -1;
	node->language = cb_ex_language;
	node->equiv_name = equiv_name;
	array_append(master, (Array_value)node);

	if (ex_enter_file->node_menus == NULL) {
		ex_enter_file->node_menus = hsh_create(10,
						       hsh_int_equal,
						       hsh_int_hash,
						       (Hash_value)NULL,
						       (Cb_heap)NULL);
	}
	master = (Array)hsh_lookup(ex_enter_file->node_menus,
				   (Hash_value)equiv_name);
	if (master == NULL) {
		(void)hsh_insert(ex_enter_file->node_menus,
				 (Hash_value)equiv_name,
				 (Hash_value)
				 (master = array_create((Cb_heap)NULL)));
	}
	array_append(master, (Array_value)node);
	node->equiv_list = master;
}

/*
 *	cb_ex_properties_statement(opt_tag, props_list)
 *		Append the props to the master prop list for this language
 */
void
cb_ex_properties_statement(opt_tag, props_list)
	char			*opt_tag;
	Array			props_list;
{
	int			i;
	int			j;
	Array			master;
	Cb_ex_props_item	prop;
	Cb_ex_props_item	defined_prop;

	cb_ex_check_language();

	if (opt_tag == NULL) {
		opt_tag = (char *)hsh_lookup(cb_ex_language,
					     (Hash_value)EX_LANGUAGE);
	}
	master = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_PROPERTIES);
	if  (master == NULL) {
		master = array_create((Cb_heap)NULL);
		(void)hsh_insert(cb_ex_language,
				 (Hash_value)EX_PROPERTIES,
				 (Hash_value)master);
	}
	for (i = 0; i < array_size(props_list); i++) {
		prop = (Cb_ex_props_item)ARRAY_FETCH(props_list, i);
		if (prop->language == NULL) {
			prop->language = opt_tag;
		}
		for (j = array_size(master)-1; j >= 0; j--) {
			defined_prop = (Cb_ex_props_item)ARRAY_FETCH(master, j);
			if ((prop->prop == defined_prop->prop) &&
			    (prop->language == defined_prop->language)) {
				cb_ex_warning("Duplicate prop `%s.%s' for language `%s'\n",
					prop->language,
					prop->prop,
					hsh_lookup(cb_ex_language,
						   (Hash_value)EX_LANGUAGE));
				cb_ex_fatal_warning = 1;
			}
		}
		array_append(master, (Array_value)prop);
	}
}

/*
 *	cb_ex_property(language, prop)
 */
Cb_ex_props_item
cb_ex_property(language, prop)
	char			*language;
	char			*prop;
{
	Cb_ex_props_item	item;

	if ((language != NULL) && (strlen(language) >= CB_LANG_NAME_SIZE)) {
		cb_ex_fatal("Language name `%s' too long. (Max = %d)\n",
			language,
			CB_LANG_NAME_SIZE);
	}
	item = cb_allocate(Cb_ex_props_item, (Cb_heap)NULL);
	item->language = language;
	item->prop = prop;
	return item;
}

/*
 *	cb_ex_add_focus_tags()
 */
static void
cb_ex_add_focus_tags()
{
	Array			current_tags;
	Array			tags;
	Cb_ex_tags_item		tag;
	int			n;
	int			m;
	int			last;

	if (focus_tags == NULL) {
		return;
	}

	cb_ex_check_language();
	current_tags = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_TAGS);
	if (current_tags == NULL) {
		(void)hsh_insert(cb_ex_language,
				 (Hash_value)EX_TAGS,
				 (Hash_value)(current_tags =
					      array_create((Cb_heap)NULL)));
	}
	last = array_size(current_tags);

	array_sort(focus_tags, cb_ex_focus_tag_sort, 0);

	tags = array_create(current_tags->heap);

	/*
	 * Now we need to iterate over the tags to add here.
	 * We might need to add fillers here and there.
	 */
	for (n = array_size(focus_tags)-1; n >= 0; n--) {
		tag = (Cb_ex_tags_item)ARRAY_FETCH(focus_tags, n);
		for (m = tag->ordinal - last; m > 0 ; m--) {
			cb_ex_add_fill_tag(tags, CB_MENU_STRING_FOCUS);
		}
		array_append(tags, (Array_value)tag);
		last = tag->ordinal + 1;
	}

	focus_tags = NULL;
	cb_ex_tags_statement(tags, CB_MENU_STRING_FOCUS, focus_size);
}

/*
 *	cb_ex_focus_tag_sort(a, b)
 */
static int
cb_ex_focus_tag_sort(a, b)
	Cb_ex_tags_item		a;
	Cb_ex_tags_item		b;
{
	return a->ordinal - b->ordinal;
}

/*
 *	cb_ex_add_fill_tag(tags_list, prefix)
 */
static void
cb_ex_add_fill_tag(tags_list, prefix)
	Array			tags_list;
	char			*prefix;
{
	char			name[1000];
	Cb_ex_tags_item		item;
static	Hash			fill_prop = NULL;
	Hash_value		element;

	if (fill_prop == NULL) {
		fill_prop = hsh_create(100,
				       cb_ex_prop_equal,
				       cb_ex_prop_hash,
				       (Hash_value)NULL,
				       (Cb_heap)NULL);
		element = (Hash_value)
		  cb_ex_property(cb_string_unique(CB_MENU_STRING_BASE),
				 cb_string_unique(CB_MENU_STRING_HIDDEN));
		(void)hsh_replace(fill_prop, element, element);
	}

	item = cb_allocate(Cb_ex_tags_item, (Cb_heap)NULL);
	(void)sprintf(name,
		      "%s_fill_%d",
		      prefix,
		      array_size(tags_list) + 1);
	item->name = cb_string_unique(name);
	item->prefix = prefix;
	item->full_name = NULL;
	item->weight = 64;
	item->secondary = NULL;
	item->props = fill_prop;
	item->focus_unit = NULL;
	item->focus_prop = NULL;
	array_append(tags_list, (Array_value)item);
}

/*
 *	cb_ex_tags_statement(tags_list, prefix, fill_count)
 */
void
cb_ex_tags_statement(tags_list, prefix, fill_count)
	Array	tags_list;
	char	*prefix;
	int	fill_count;
{
	int			i;
	int			j;
	Array			master;
	Cb_ex_tags_item		item;
	char			*language_name;

	for (i = 0; i < array_size(tags_list); i++) {
		item = (Cb_ex_tags_item)ARRAY_FETCH(tags_list, i);
		if (item->focus_unit != NULL) {
			if ((focus_size != -1) && (focus_size != fill_count)) {
				cb_ex_fatal("Focus tag set size changed %d -> %d\n",
					    focus_size,
					    fill_count);
			}
			focus_size = fill_count;
		}
	}

	cb_ex_add_focus_tags();

	cb_ex_check_language();

	language_name = (char *)hsh_lookup(cb_ex_language,
					   (Hash_value)EX_LANGUAGE);
	if (prefix == NULL) {
		prefix = language_name;
	}

	master = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_TAGS);
	if (master == NULL) {
		(void)hsh_insert(cb_ex_language,
				 (Hash_value)EX_TAGS,
				 (Hash_value)(master =
					      array_create((Cb_heap)NULL)));
	}
	if ((i = fill_count - array_size(tags_list)) < 0) {
		cb_ex_fatal("Tags with prefix `%s' overflow allocated size\n",
			    prefix);
	}
	for (i = fill_count - array_size(tags_list); i > 0; i--) {
		cb_ex_add_fill_tag(tags_list, prefix);
	}
	for (i = 0; i < array_size(tags_list); i++) {
		item = (Cb_ex_tags_item)ARRAY_FETCH(tags_list, i);
		for (j = array_size(master)-1; j >= 0; j--) {
			if (item->name ==
				((Cb_ex_tags_item)ARRAY_FETCH(master, j))->name) {
				cb_ex_warning("Duplicate tag item `%s' for language `%s'\n",
					item->name, hsh_lookup(cb_ex_language,
							       (Hash_value)EX_LANGUAGE));
				cb_ex_fatal_warning = 1;
			}
		}
		item->prefix = prefix;
		item->ordinal = array_size(master);
		cb_ex_check_tag_duplicated(item,
					   &tag_check,
					   prefix,
					   language_name);
		array_append(master, (Array_value)item);
	}
}

/*
 *	cb_ex_check_tag_duplicated(item, table, prefix, language_name)
 *		Check if this tag/prop has been entered before and
 *		complain if the ordinal number is different if it has
 */
static void
cb_ex_check_tag_duplicated(item, table, prefix, language_name)
	Cb_ex_tags_item		item;
	Hash			*table;
	char			*prefix;
	char			*language_name;
{
	char			*buffer;
	int			item_number;

	if (*table == NULL) {
		*table = hsh_create(100,
				    hsh_int_equal,
				    hsh_int_hash,
				    (Hash_value)0,
				    (Cb_heap)NULL);
	}
	if (item->full_name == NULL) {
		buffer = alloca(strlen(item->prefix) + strlen(item->name) + 32);
		(void)sprintf(buffer, "%s_%s", item->prefix, item->name);
		item->full_name = cb_string_unique(buffer);
	}
	item_number = (int)hsh_lookup(*table, (Hash_value)item->full_name);
	if (item_number == 0) {
		(void)hsh_insert(*table,
				 (Hash_value)item->full_name,
				 (Hash_value)item->ordinal);
		return;
	}

	if (item_number != item->ordinal) {
		cb_ex_warning("Tag `cb_%s_%s' defined with new ordinal for language `%s' (New=%d Old=%d)\n",
			    prefix,
			    item->name,
			    language_name,
			    item_number,
			    item->ordinal);
		cb_ex_fatal_warning = 1;
	}
	return;
}

/*
 *	cb_ex_tags_item(name, opt_weight, props_list, secondary_list)
 *		handle one "decl = ( decl )" line
 */
Cb_ex_tags_item
cb_ex_tags_item(name, opt_weight, props_list, secondary_list)
	char			*name;
	int			opt_weight;
	Array			props_list;
	Array			secondary_list;
{
	Cb_ex_tags_item		item;
	Array			defined_props;
	int			i;
	int			j;
	Cb_ex_props_item	prop;
	Cb_ex_props_item	defined_prop;
	char			*language_name;
	int			weight;
	Hash			secondary;
	char			*ctag;
	Hash_value		element;

	cb_ex_check_language();

	if (secondary_list != NULL) {
		secondary = (Hash)hsh_lookup(cb_ex_language,
					     (Hash_value)EX_SECONDARY);
		if (secondary == NULL) {
			secondary = hsh_create(10,
					       hsh_int_equal,
					       hsh_int_hash,
					       (Hash_value)NULL,
					       (Cb_heap)NULL);
			(void)hsh_insert(cb_ex_language,
					 (Hash_value)EX_SECONDARY,
					 (Hash_value)secondary);
		}
		for (i = array_size(secondary_list)-1; i >= 0; i--) {
			ctag = (char *)ARRAY_FETCH(secondary_list, i);
			if (hsh_lookup(secondary,
				       (Hash_value)ctag) == NULL) {
				(void)hsh_insert(secondary,
						 (Hash_value)ctag,
						 (Hash_value)array_create((Cb_heap)NULL));
			}
		}
	}

	if (opt_weight == -1) {
		weight = CB_EX_DEFAULT_DEFAULT_WEIGHT;
	} else {
		weight = opt_weight;
		if ((weight < 1) || (weight > 127)) {
			cb_ex_fatal("Weight (%d) out of range (1..127)\n",
				weight);
		}
	}

	language_name = (char *)hsh_lookup(cb_ex_language,
					   (Hash_value)EX_LANGUAGE);
	/*
	 * Verify that all the props mentioned have been defined in a
	 * props statement
	 */
	defined_props = (Array)hsh_lookup(cb_ex_language,
					  (Hash_value)EX_PROPERTIES);
	if (defined_props == NULL) {
		cb_ex_fatal("No properties have been defined for language `%s'\n",
				hsh_lookup(cb_ex_language,
					   (Hash_value)EX_LANGUAGE));
	}
	for (i = array_size(props_list)-1; i >= 0; i--) {
		prop = (Cb_ex_props_item)ARRAY_FETCH(props_list, i);
		if (prop->language == NULL) {
			prop->language = language_name;
		}
		for (j = array_size(defined_props)-1; j >= 0; j--) {
			defined_prop = (Cb_ex_props_item)ARRAY_FETCH(defined_props, j);
			if ((prop->language == defined_prop->language) &&
			    (prop->prop == defined_prop->prop)) {
				goto found_it;
			}
		}
		cb_ex_fatal("Undefined property `%s.%s' for tag `%s', language `%s'\n",
				prop->language,
				prop->prop,
				name,
				hsh_lookup(cb_ex_language,
					   (Hash_value)EX_LANGUAGE));
	    found_it:;
	}

	item = cb_allocate(Cb_ex_tags_item, (Cb_heap)NULL);
	item->name = name;
	item->prefix = NULL;
	item->full_name = NULL;
	item->weight = weight;
	item->secondary = secondary_list;
	item->props = hsh_create(100,
				 cb_ex_prop_equal,
				 cb_ex_prop_hash,
				 (Hash_value)NULL,
				 (Cb_heap)NULL);
	item->focus_unit = NULL;
	item->focus_prop = NULL;
	for (i = array_size(props_list)-1; i>= 0; i--) {
		element = (Array_value)ARRAY_FETCH(props_list, i);
		(void)hsh_replace(item->props, element, element);
	}

	return item;
}

/*
 *	cb_ex_ref_focus_unit_statement(name)
 */
void
cb_ex_ref_focus_unit_statement(name)
	char				*name;
{
	Cb_ex_tags_item			tag_item;
	Cb_ex_tags_item			defd_tag_item = NULL;
	Array				props_list;
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_sub_item	subitem;
	Cb_ex_enter_menu_sub_item	subitem2;
	char				*language_name;

	if (focus_units != NULL) {
		defd_tag_item = (Cb_ex_tags_item)hsh_lookup(focus_units,
							    (Hash_value)name);
	}
	if (defd_tag_item == NULL) {
		cb_ex_fatal("No focus unit named `%s'\n", name);
	}

	/*
	 * Expand the menu items with this language
	 */
	cb_ex_check_language();
	language_name = (char *)hsh_lookup(cb_ex_language,
					   (Hash_value)EX_LANGUAGE);
	item = defd_tag_item->focus_unit->menu_item;
	subitem = (Cb_ex_enter_menu_sub_item)array_fetch(item->sub_items, 0);
	subitem2 = cb_allocate(Cb_ex_enter_menu_sub_item,
			       item->sub_items->heap);
	*subitem2 = *subitem;
	subitem2->language = language_name;
	array_append(item->sub_items, (Array_value)subitem2);

	props_list = array_create((Cb_heap)NULL);
	array_append(props_list, (Array_value)defd_tag_item->focus_prop);
	cb_ex_properties_statement((char *)NULL, props_list);

	props_list = array_create((Cb_heap)NULL);
	(void)hsh_iterate(defd_tag_item->props,
			  cb_ex_ref_focus_unit_make_list,
			  (int)&props_list);
	tag_item = cb_ex_tags_item(defd_tag_item->name,
				   -1,
				   props_list,
				   (Array)NULL);

	tag_item->focus_unit = defd_tag_item->focus_unit;
	tag_item->ordinal = defd_tag_item->ordinal;

	if (focus_tags == NULL) {
		focus_tags = array_create(item->sub_items->heap);
	}
	array_append(focus_tags, (Array_value)tag_item);
}

/*
 *	cb_ex_ref_focus_unit_make_list(key, value, props_list)
 */
/*ARGSUSED*/
static int
cb_ex_ref_focus_unit_make_list(key, value, props_list)
	Array_value	key;
	Array_value	value;
	Array		*props_list;
{
	array_append(*props_list, value);
}

/*
 *	cb_ex_def_focus_unit(name, unit_tag, props, use_refd,
 *				make_absolute, start, stop, invisible)
 */
Cb_ex_tags_item
cb_ex_def_focus_unit(name, unit_tag, props, use_refd,
			make_absolute, start, stop, invisible)
	char			*name;
	char			*unit_tag;
	Array			props;
	int			use_refd;
	int			make_absolute;
	Cb_ex_start_stop	start;
	Cb_ex_start_stop	stop;
	int			invisible;
{
	char			*label;
	char			*p;
	char			*q;
	Array			props_list;
	Array			menu_list;
	Array			menu_props_list;
	Cb_ex_tags_item		tag_item;
	Cb_ex_enter_focus_unit	focus_unit;
	Cb_ex_props_item	new_prop;

	if (focus_units == NULL) {
		focus_units = hsh_create(100,
					 hsh_int_equal,
					 hsh_int_hash,
					 (Hash_value)NULL,
					 (Cb_heap)NULL);
	}

	if (unit_tags == NULL) {
		unit_tags = hsh_create(10,
				       hsh_int_equal,
				       hsh_int_hash,
				       (Hash_value)NULL,
				       (Cb_heap)NULL);
	}
	if (hsh_lookup(unit_tags, (Hash_value)unit_tag) != NULL) {
		cb_ex_fatal("Focus unit tag `%s' redefined in unit `%s'\n",
			    unit_tag,
			    name);
	}
	(void)hsh_insert(unit_tags,
			 (Hash_value)unit_tag,
			 (Hash_value)unit_tag);

	label = (char *)alloca(strlen(name) + 16);
	for (p = name, q = label; *p != 0; p++) {
		if (isalpha(*p) && isupper(*p)) {
			*q++ = tolower(*p);
		} else {
			*q++ = *p;
		}
	}
	(void)strcpy(q, "_unit");
	label = cb_string_unique(label);

	if (ex_enter_file->focus_units == NULL) {
		ex_enter_file->sections++;
		if (ex_enter_file->sections >= CB_EX_MAX_SECTION_COUNT) {
			cb_ex_fatal(".ex file section overflow\n");
		}
		ex_enter_file->focus_units = array_create((Cb_heap)NULL);
	}

	if (strlen(unit_tag) != 1) {
		cb_ex_fatal("The unit tag (%s) for focusing unit %s is too long. Must be one char long.\n",
			    name,
			    unit_tag);
	}

	/*
	 * We need to connect units with include tags with their unit
	 */
	focus_unit = cb_allocate(Cb_ex_enter_focus_unit, (Cb_heap)NULL);
	(void)bzero((char *)focus_unit, sizeof(*focus_unit));
	array_append(ex_enter_file->focus_units,
			   (Array_value)focus_unit);
	focus_unit->focus_unit_name = name;
	focus_unit->make_absolute = make_absolute;
	focus_unit->start = start;
	focus_unit->stop = stop;
	focus_unit->use_refd = use_refd;
	focus_unit->tag = unit_tag;
	focus_unit->prop = NULL;
	focus_unit->equiv = NULL;
	focus_unit->invisible = invisible;

	cb_ex_check_language();

	props_list = array_create((Cb_heap)NULL);
	new_prop = cb_ex_property((char *)NULL, label);
	array_append(props_list, (Array_value)new_prop);
	cb_ex_properties_statement((char *)hsh_lookup(cb_ex_language,
						      (Hash_value)EX_LANGUAGE),
				   props_list);

	menu_props_list = array_create((Cb_heap)NULL);
	array_append(menu_props_list, (Array_value)props_list);

	menu_list = array_create((Cb_heap)NULL);
	array_append(menu_list,
		     (Array_value)cb_ex_menu_regular_item(label,
							  menu_props_list,
							  1,
							  (char *)NULL,
							  (Cb_ex_equiv)NULL));
	focus_unit->menu_item = cb_ex_menu_statement(menu_list);

	array_append(props, (Array_value)new_prop);
	tag_item = cb_ex_tags_item(label,
				   -1,
				   props,
				   (Array)NULL);
	tag_item->focus_unit = focus_unit;
	tag_item->focus_prop = new_prop;
	(void)hsh_insert(focus_units, (Hash_value)name, (Hash_value)tag_item);
	return tag_item;
}

/*
 *	cb_ex_render_statement(tag_name, rendering, color)
 */
void
cb_ex_render_statement(tag_name, rendering, color)
	char			*tag_name;
	Cb_ex_rendering		rendering;
	Cb_ex_color_im		color;
{
	Array			master;
	Array			tags;
	Cb_ex_tags_item		tag;
	int			i;
	Cb_ex_enter_rendering	render;
	char			*buffer;

	cb_ex_check_language();

	master = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_RENDER);
	if (master == NULL) {
		ex_enter_file->sections++;
		(void)hsh_insert(cb_ex_language,
				 (Hash_value)EX_RENDER,
				 (Hash_value)(master =
					      array_create((Cb_heap)NULL)));
	}

	tags = (Array)hsh_lookup(cb_ex_language, (Hash_value)EX_TAGS);
	if (tags == NULL) {
		cb_ex_fatal("No tags defined when parsing render statement\n");
	}

	for (i = array_size(tags)-1; i >= 0; i--) {
		tag = (Cb_ex_tags_item)ARRAY_FETCH(tags, i);

		if (tag->full_name == NULL) {
			buffer = alloca(strlen(tag->prefix) +
					strlen(tag->name) + 32);
			(void)sprintf(buffer, "%s_%s", tag->prefix, tag->name);
			tag->full_name = cb_string_unique(buffer);
		}
		if (tag->full_name == tag_name) {
			goto found_it;
		}
	}
	cb_ex_fatal("Could not find tag `%s'\n", tag_name);
 found_it:
	for (i = array_size(master)-1; i >= 0; i--) {
		render = (Cb_ex_enter_rendering)ARRAY_FETCH(master, i);

		if (render->tag == tag) {
			cb_ex_fatal("Tag `%s' has more than one rendering\n",
				    tag->full_name);
		}
	}

	render = cb_allocate(Cb_ex_enter_rendering, (Cb_heap)NULL);
	render->tag = tag;
	render->rendering = rendering;
	render->color = color;
	array_append(master, (Array_value)render);
}

/*
 *	cb_ex_color(red, green, blue)
 */
Cb_ex_color_im
cb_ex_color(red, green, blue)
	int		red;
	int		green;
	int		blue;
{
	Cb_ex_color_im	color;

	if ((0 > red) || (red > 255)) {
		cb_ex_fatal("Color red out of range 0..255 `%d'\n", red);
	}
	if ((0 > green) || (green > 255)) {
		cb_ex_fatal("Color green out of range 0..255 `%d'\n", green);
	}
	if ((0 > blue) || (blue > 255)) {
		cb_ex_fatal("Color blue out of range 0..255 `%d'\n", blue);
	}

	color = cb_allocate(Cb_ex_color_im, (Cb_heap)NULL);
	color->red = red;
	color->green = green;
	color->blue = blue;
	return color;
}

/*
 *	cb_ex_version_statement(number)
 */
void
cb_ex_version_statement(number)
	int		number;
{
	if ((number < 1) || (number > 32767)) {
		cb_ex_fatal("Version number must be in the range 1..32767\n");
	}
	if (cb_ex_language != NULL) {
		(void)hsh_insert(cb_ex_language,
				 (Hash_value)EX_VERSION,
				 (Hash_value)number);
	} else {
		if (languages != NULL) {
			(void)hsh_insert(languages,
					 (Hash_value)EX_VERSION,
					 (Hash_value)version);
		} else {
			version = number;
		}
	}
}

/*
 *	cb_ex_prop_equal(a, b)
 */
static int
cb_ex_prop_equal(a, b)
	Cb_ex_props_item	a;
	Cb_ex_props_item	b;
{
	return (a->language == b->language) && (a->prop == b->prop);
}

/*
 *	cb_ex_prop_hash(a)
 */
static int
cb_ex_prop_hash(a)
	Cb_ex_props_item	a;
{
	return (int)a->language + (int)a->prop;
}


/*
 *	cb_ex_write_files(write_h)
 */
void
cb_ex_write_files(write_h)
	char			*write_h;
{
	write_h_files = write_h;

	if (cb_ex_fatal_warning) {
		cb_ex_fatal("No files written due to warnings\n");
	}
	if (languages != NULL) {
		(void)hsh_iterate(languages,
				  cb_ex_write_tags_file,
				  (int)ex_enter_file);
	}
	cb_ex_write_ex_file(ex_enter_file, languages);
}

/*
 *	cb_ex_write_tags_file(language_name, language, ex_file)
 */
static int
cb_ex_write_tags_file(language_name, language, ex_file)
	char			*language_name;
	Hash			language;
	Cb_ex_file		ex_file;
{
	int			i;
	Array			tags;
	Cb_ex_tags_item		item;
	FILE			*out;
	char			tempname[MAXPATHLEN];
	char			filename[MAXPATHLEN];
	Cb_ex_tags_im		im_item;
	Cb_ex_enter_graph	graph;
	char			*uname;
	char			*pu;
	char			*q;

	if (language_name <= (char *)EX_LAST_TOKEN) {
		return;
	}

	tags = (Array)hsh_lookup(language, (Hash_value)EX_TAGS);

	if (hsh_lookup(language, (Hash_value)EX_INVISIBLE) == NULL) {
		if (ex_file->languages == NULL) {
			ex_file->languages = array_create((Cb_heap)NULL);
			ex_file->sections++;
			if (ex_file->sections >= CB_EX_MAX_SECTION_COUNT) {
				cb_ex_fatal(".ex file section overflow\n");
			}
		}
		array_append(ex_file->languages, (Array_value)language_name);

		if (ex_file->tags == NULL) {
			ex_file->tags = array_create((Cb_heap)NULL);
		}
		if (tags != NULL)  {
			if (semantic_widths == NULL) {
				semantic_widths = hsh_create(10,
							     hsh_int_equal,
							     hsh_int_hash,
							     (Hash_value)0,
							     (Cb_heap)NULL);
			}
			(void)hsh_insert(semantic_widths,
					 (Hash_value)language_name,
					 (Hash_value)array_size(tags));
		}
		im_item = cb_allocate(Cb_ex_tags_im, (Cb_heap)NULL);
		im_item->language = language_name;
		im_item->tags = tags;
		array_append(ex_file->tags, (Array_value)im_item);
		if (tags != NULL) {
			ex_file->sections += 2;
			if (ex_file->sections >= CB_EX_MAX_SECTION_COUNT) {
				cb_ex_fatal(".ex file section overflow\n");
			}
		}
	}

	if ((hsh_lookup(language, (Hash_value)EX_INVISIBLE) != NULL) ||
	    (write_h_files == NULL)) {
		return;
	}

	(void)sprintf(tempname,
		      "%s/cb_%s_tags.h.temp",
		      write_h_files,
		      language_name);
	(void)sprintf(filename,
		      "%s/cb_%s_tags.h",
		      write_h_files,
		      language_name);
	out = cb_fopen(tempname, "w", (char *)NULL, 0);

	(void)fprintf(out, "/*\n");
	(void)fprintf(out,
		      " * SunSourceBrowser tags file for %s language\n",
		      language_name);
	(void)fprintf(out,
		      " * This file was generated by the sbextend program.\n");
	if (hsh_lookup(languages, (Hash_value)EX_VERSION) != NULL) {
		(void)fprintf(out, " * Major version number is %d\n",
			hsh_lookup(languages, (Hash_value)EX_VERSION));
	}
	if (hsh_lookup(language, (Hash_value)EX_VERSION) != NULL) {
		(void)fprintf(out,
			      " * Version number for the language is %d\n",
			      hsh_lookup(language, (Hash_value)EX_VERSION));
	}
	(void)fprintf(out, " */\n\n\n");

	(void)fprintf(out, "#ifndef cb_%s_tags_h_INCLUDED\n",
			language_name);
	(void)fprintf(out, "#define cb_%s_tags_h_INCLUDED\n\n",
			language_name);
	(void)fprintf(out, "#define CB_CURRENT_LANGUAGE \"%s\"\n",
			language_name);
	(void)fprintf(out,
		      "#define CB_CURRENT_MAJOR_VERSION %d\n",
		      hsh_lookup(languages, (Hash_value)EX_VERSION));
	(void)fprintf(out,
		      "#define CB_CURRENT_MINOR_VERSION %d\n\n",
		      hsh_lookup(language, (Hash_value)EX_VERSION));

	
	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_ARC_TAG '%c'\n",
		      CB_EX_ARC_HEADER_BYTE);
	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_NODE_TAG '%c'\n",
		      CB_EX_NODE_HEADER_BYTE);
	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_PROP_TAG '%c'\n\n",
		      CB_EX_PROP_HEADER_BYTE);

	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_ARC_PATTERN1 \"\\%o%c%%c %%s\"\n",
		      CB_EX_FUNNY_HEADER_BYTE,
		      CB_EX_ARC_HEADER_BYTE);
	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_ARC_PATTERN2 \"\\%o%c%%c %%s %%s\"\n",
		      CB_EX_FUNNY_HEADER_BYTE,
		      CB_EX_ARC_HEADER_BYTE);
	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_ARC_PATTERN3 \"\\%o%c%%c %%s %%s %%s\"\n",
		      CB_EX_FUNNY_HEADER_BYTE,
		      CB_EX_ARC_HEADER_BYTE);
	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_NODE_PATTERN \"\\%o%c %%s\"\n",
		      CB_EX_FUNNY_HEADER_BYTE,
		      CB_EX_NODE_HEADER_BYTE);
	(void)fprintf(out,
		      "#define CB_EX_GRAPHER_PROP_PATTERN \"\\%o%c %%s\"\n\n",
		      CB_EX_FUNNY_HEADER_BYTE,
		      CB_EX_PROP_HEADER_BYTE);

	if ((ex_enter_file->graphs != NULL) &&
	    (array_size(ex_enter_file->graphs) > 0)) {
		(void)fprintf(out, "\n/*\n * Graph tags\n */\n");
		for (i = array_size(ex_enter_file->graphs)-1; i >= 0; i--) {
			graph = (Cb_ex_enter_graph)
			  ARRAY_FETCH(ex_enter_file->graphs, i);
			uname = alloca(strlen(graph->graph_name) + 32);
			for (pu = uname, q = graph->graph_name; *q != 0; q++) {
				if (isalpha(*q) && islower(*q)) {
					*pu++ = toupper(*q);
				} else if (isspace(*q)) {
					*pu++ = '_';
				} else {
					*pu++ = *q;
				}
			}
			*pu = 0;

			(void)fprintf(out,
				      "#define CB_EX_GRAPHER_%s_TAG '%c'\n",
				      uname,
				      graph->graph_tag[0]);
		}
	}

	(void)fprintf(out, "\n/*\n * Generic pattern\n */\n");
	(void)fprintf(out,
		      "#define CB_EX_FOCUS_UNIT_FORMAT \"\\%o%c %%s %%s %%x %%x\"\n\n",
		      CB_EX_FUNNY_HEADER_BYTE,
		      CB_EX_FOCUS_HEADER_BYTE);

	if (tags != NULL) {
		for (i = 0; i < array_size(tags); i++) {
			item = (Cb_ex_tags_item)ARRAY_FETCH(tags, i);
			if (item->focus_unit != NULL) {
				cb_ex_write_focus_unit_pattern(out,
							       item->focus_unit);
			}
		}

		(void)fprintf(out, "\n");

		(void)fprintf(out, "#ifndef CB_EX_NO_TAGS\n");
		(void)fprintf(out, "enum Cb_%s_tags_tag {\n", language_name);
		(void)fprintf(out, "\tcb_%s_first_tag,\n\n", language_name);
		for (i = 0; i < array_size(tags); i++) {
			item = (Cb_ex_tags_item)ARRAY_FETCH(tags, i);
			(void)fprintf(out,
				      "\tcb_%s_%s,\n",
				      item->prefix,
				      item->name);
		}
		(void)fprintf(out, "\n\tcb_%s_last_tag\n", language_name);
		(void)fprintf(out, "};\n");
		(void)fprintf(out, "typedef enum Cb_%s_tags_tag\t*Cb_%s_tags_ptr, Cb_%s_tags;\n",
			      language_name,
			      language_name,
			      language_name);
		(void)fprintf(out, "#endif\n");
	}

	(void)fprintf(out, "\n#endif\n");
	cb_fclose(out);
	if (cb_ex_different(tempname, filename)) {
		(void)unlink(filename);
		(void)rename(tempname, filename);
	} else {
		(void)unlink(tempname);
	}
}

/*
 *	cb_ex_write_focus_unit_pattern(out, focus_unit)
 */
static void
cb_ex_write_focus_unit_pattern(out, focus_unit)
	FILE			*out;
	Cb_ex_enter_focus_unit	focus_unit;
{
	char			*uname;
	char			*pu;
	char			*q;

	uname = alloca(strlen(focus_unit->focus_unit_name) + 32);
	for (pu = uname, q = focus_unit->focus_unit_name;
	     *q != 0;
	     q++) {
		if (isalpha(*q) && islower(*q)) {
			*pu++ = toupper(*q);
		} else {
			*pu++ = *q;
		}
	}
	*pu = 0;
	(void)fprintf(out,
		      "#define CB_EX_FOCUS_UNIT_%s_FORMAT \"\\%o%c %s %%s%s%s\"\n",
		      uname,
		      CB_EX_FUNNY_HEADER_BYTE,
		      CB_EX_FOCUS_HEADER_BYTE,
		      focus_unit->tag,
		      cb_ex_lineno_pattern(focus_unit->start),
		      cb_ex_lineno_pattern(focus_unit->stop));
	(void)fprintf(out,
		      "#define CB_EX_FOCUS_UNIT_%s_KEY '%s'\n",
		      uname,
		      focus_unit->tag);
	(void)fprintf(out, "\n");
}

/*
 *	cb_ex_lineno_pattern(pattern)
 */
static char *
cb_ex_lineno_pattern(pattern)
	Cb_ex_start_stop	pattern;
{
	switch (pattern) {
		case cb_ex_none_start_stop:	return "";
		case cb_ex_zero_start_stop:	return " 0";
		case cb_ex_regular_start_stop:	return " %x";
		case cb_ex_inf_start_stop:	return " .";
		default: cb_abort("Illegal lineno pattern type %d\n", pattern);
	}
	/*NOTREACHED*/
}

/*
 *	cb_ex_different(file_name1, file_name2)
 *		This will return FALSE if the contents "file_name1" is
 *		equal to the contents of "file_name2".  In all other cases,
 *		TRUE is returned.
 */
static int
cb_ex_different(file_name1, file_name2)
	char		*file_name1;
	char		*file_name2;
{
	int		chr1;
	int		chr2;
	FILE		*file1;
	FILE		*file2;

	file1 = fopen(file_name1, "r");
	if (file1 == NULL) {
		return TRUE;
	}
	file2 = fopen(file_name2, "r");
	if (file2 == NULL) {
		return TRUE;
	}
	do {
		chr1 = getc(file1);
		chr2 = getc(file2);
	} while ((chr1 == chr2) && (chr1 != EOF));
	cb_fclose(file1);
	cb_fclose(file2);
	return ((chr1 != EOF) || (chr2 != EOF));
}

/*
 *	cb_ex_fatal(template, args...)
 */
/*VARARGS0*/
void
cb_ex_fatal(va_alist)
	va_dcl
{
	va_list		args;
	char		*format;
	char		message[1024];

	cb_fflush(stdout);
	va_start(args);
	format = va_arg(args, char *);
	(void)vsprintf(message, format, args);
	va_end(args);
	yyerror(message);
	cb_fflush(stderr);
	exit(1);
}

/*
 *	cb_ex_warning(template, args...)
 */
/*VARARGS0*/
void
cb_ex_warning(va_alist)
	va_dcl
{
	va_list		args;
	char		*format;
	char		message[1024];
extern  int     	yylineno;
extern  char    	yyfile[];

	cb_fflush(stdout);
	va_start(args);
	format = va_arg(args, char *);
	(void)vsprintf(message, format, args);
	va_end(args);
        (void)fprintf(stderr,
		      "%s, line %d: Warning: %s\n",
		      yyfile,
		      yylineno,
		      message);
	cb_fflush(stderr);
}

/*
 *	cb_ex_init()
 */
void
cb_ex_init()
{
	int		length;
extern	int		yylineno;

	length = sizeof(ex_enter_file->header.section[0]) *
	  CB_EX_MAX_SECTION_COUNT;
	length += sizeof(Cb_ex_file_rec);
	ex_enter_file = (Cb_ex_file)cb_get_memory(length, (Cb_heap)NULL);
	(void)bzero((char *)ex_enter_file, length);
	ex_enter_file->sections = 1;

#ifndef CB_BOOT
	cb_enter_start(&yylineno,
		       CB_CURRENT_LANGUAGE,
		       CB_CURRENT_MAJOR_VERSION,
		       CB_CURRENT_MINOR_VERSION);
#endif
}

/*
 *	cb_ex_check_language()
 *		Make sure a language has been set.
 */
static void
cb_ex_check_language()
{
	if (cb_ex_language == NULL) {
		cb_ex_fatal("No language defined yet");
	}
}

/*
 *	cb_ex_first_list_item(name)
 */
Array
cb_ex_first_list_item(item)
	int	item;
{
	Array	array;

	array = array_create((Cb_heap)NULL);
	array_append(array, (Array_value)item);
	return array;
}

/*
 *	cb_ex_next_list_item(array, item)
 */
Array
cb_ex_next_list_item(array, item)
	Array	array;
	int	item;
{
	array_append(array, (Array_value)item);
	return array;
}

