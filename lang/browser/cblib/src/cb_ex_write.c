#ident "@(#)cb_ex_write.c 1.1 94/10/31 SMI"

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

#ifndef cb_ex_h_INCLUDED
#include "cb_ex.h"
#endif

#ifndef cb_ex_write_h_INCLUDED
#include "cb_ex_write.h"
#endif

#ifndef cb_extend_h_INCLUDED
#include "cb_extend.h"
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

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef cb_swap_bytes_h_INCLUDED
#include "cb_swap_bytes.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

#include "cb_ex_parse.h"

extern	Hash			cb_ex_language;
extern	int			cb_ex_extra_menu;

	Hash			semantic_widths;

#define ALL_LANGUAGES		"All languages"

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void			cb_ex_write_ex_file();
extern	Cb_ex_file_write	cb_ex_open_write();
extern	void			cb_ex_close_write();
extern	void			cb_ex_write_header_section();
extern	void			cb_ex_write_languages();
extern	void			cb_ex_write_menu_spec();
extern	void			cb_ex_write_menu_eliminate();
extern	int			cb_ex_write_menu_eliminate_fn1();
extern	int			cb_ex_write_menu_eliminate_fn2();
extern	void			cb_ex_write_menu_layout();
extern	void			cb_ex_write_menu_write();
extern	void			cb_ex_write_semantics();
extern	void			cb_ex_get_pullright_semvec();
extern	void			cb_ex_get_pullright_help();
extern	void			cb_ex_write_menu_strings();
extern	int			cb_ex_write_get_strings();
extern	int			cb_ex_write_sort_strings();
extern	Cb_ex_unique_string	cb_ex_unique_menu_string();
extern	int			cb_ex_unique_equal();
extern	int			cb_ex_unique_hash();
extern	void			cb_ex_write_node_menus();
extern	int			cb_ex_write_node_menus_collect();
extern	int			cb_ex_write_node_menus_sort();
extern	void			cb_ex_write_graph_menus();
extern	void			cb_ex_write_focus_unit();
extern	void			cb_ex_write_tags();
extern	void			cb_ex_write_tags_section();
extern	void			cb_ex_write_comment_delimiters();
extern	int			cb_ex_write_comment_delimiters_section();
extern	int			cb_ex_write_comment_delimiters_sort();
extern	void			cb_ex_write_renderings();
extern	int			cb_ex_write_renderings_section();
extern	int			cb_ex_write_renderings_sort();
extern	void			cb_ex_write_nodes();
extern	int			cb_ex_write_nodes_section();
extern	void			cb_ex_write_graph_section();
extern	void			cb_ex_write_weight();
extern	int			cb_ex_write_weight_sort();
extern	void			cb_ex_write_weight_section();

typedef struct Node_menu_help_tag	*Node_menu_help, Node_menu_help_rec;
struct Node_menu_help_tag {
	char	*name;
	Hash	table;
};

/*
 *	cb_ex_write_ex_file(ex_enter_file, languages)
 */
void
cb_ex_write_ex_file(ex_enter_file, languages)
	Cb_ex_file	ex_enter_file;
	Hash		languages;
{
	Cb_ex_file_write	ex_file_write;
	Array			node_menus;
	Node_menu_help		help;
	int			n;

	ex_file_write = cb_ex_open_write(ex_enter_file, languages);

	cb_ex_write_languages(ex_file_write);
	cb_ex_write_weight(ex_file_write);
	cb_ex_write_menu_spec(ex_file_write);
	node_menus = array_create((Cb_heap)NULL);
	if (ex_file_write->language_tags == NULL) {
		cb_ex_fatal("Can not write empty .ex file\n");
	}
	(void)hsh_iterate(ex_file_write->language_tags,
			  cb_ex_write_node_menus_collect,
			  (int)node_menus);
	array_sort(node_menus, cb_ex_write_node_menus_sort, 0);
	for (n = array_size(node_menus)-1; n >= 0; n--) {
		help = (Node_menu_help)ARRAY_FETCH(node_menus, n);
		cb_ex_write_node_menus(ex_file_write, help);
	}
	cb_ex_write_graph_menus(ex_file_write);
	cb_ex_write_focus_unit(ex_file_write);
	cb_ex_write_renderings(ex_file_write);
	cb_ex_write_nodes(ex_file_write);
	cb_ex_write_graph_section(ex_file_write);
	cb_ex_write_tags(ex_file_write);
	cb_ex_write_comment_delimiters(ex_file_write);

	/*
	 * And Last of all we write the header
	 */
	cb_ex_write_header_section(ex_file_write);

	cb_ex_close_write(ex_file_write);
}

/*
 *	cb_ex_open_write(ex_file, language_tags)
 */
static Cb_ex_file_write
cb_ex_open_write(ex_file, language_tags)
	Cb_ex_file		ex_file;
	Hash			language_tags;
{
	Cb_ex_file_write	ex_file_write;

	/*
	 * Create the file objects and open the file
	 */
	ex_file_write = cb_allocate(Cb_ex_file_write, (Cb_heap)NULL);
	(void)bzero((char *)ex_file_write, sizeof(Cb_ex_file_write_rec));
	ex_file_write->file = ex_file;
	ex_file_write->file->header.section_count = 1;
	ex_file_write->fd = cb_fopen(CB_EX_FILENAME, "w", (char *)NULL, 0);
	ex_file_write->language_tags = language_tags;
	ex_file_write->menu_item_path = array_create((Cb_heap)NULL);

	ex_file_write->file->header.section[0].location.start = 0;
	ex_file_write->file->header.section[0].location.length =
		sizeof(ex_file->header.section[0]) * (ex_file->sections - 1) +
			sizeof(Cb_ex_file_rec);

	return ex_file_write;
}

/*
 *	cb_ex_close_write(ex_file_write)
 */
static void
cb_ex_close_write(ex_file_write)
	Cb_ex_file_write	ex_file_write;
{
	cb_fclose(ex_file_write->fd);
}

/*
 *	cb_ex_write_header_section(ex_file)
 */
static void
cb_ex_write_header_section(ex_file)
	Cb_ex_file_write	ex_file;
{
	int			length;
#ifdef CB_SWAP
	int			i;
#endif

	if (ex_file->file->header.section_count != ex_file->file->sections) {
		cb_ex_fatal("Internal error: section count mismatch\n");
	}

	ex_file->file->header.magic_number = CB_EX_MAGIC_NUMBER;
	ex_file->file->header.version_number = CB_EX_VERSION_NUMBER;
	ex_file->file->header.section[0].tag = cb_ex_header_section;
	(void)bzero((char *)ex_file->file->header.section[0].language,
			sizeof(ex_file->file->header.section[0].language));
	(void)strcpy((char *)ex_file->file->header.section[0].language,
			ALL_LANGUAGES);

	cb_fseek(ex_file->fd,
		ex_file->file->header.section[0].location.start);
	length = ex_file->file->header.section[0].location.length;

#ifdef CB_SWAP
	i = ex_file->file->header.section_count-1;
	cb_swap(ex_file->file->header.magic_number);
	cb_swap(ex_file->file->header.version_number);
	cb_swap(ex_file->file->header.section_count);
	for (; i >= 0 ; i--) {
		cb_swap(ex_file->file->header.section[i].tag);
		cb_swap(ex_file->file->header.section[i].swapped);
		cb_swap(ex_file->file->header.section[i].location.start);
		cb_swap(ex_file->file->header.section[i].location.length);
	}
#endif

	cb_fwrite((char *)&ex_file->file->header, length, ex_file->fd);
}

/*
 *	cb_ex_write_languages(ex_file)
 */
static void
cb_ex_write_languages(ex_file)
	Cb_ex_file_write	ex_file;
{
	Cb_ex_section		section;
	Cb_ex_language_rec	rec;
	int			n;
	int			i;
	char			*language;
	Hash			lang;
	int			semantics_width;

	if (ex_file->file->languages == NULL) {
		return;
	}

	section = &ex_file->file->header.section
					[ex_file->file->header.section_count];

	ex_file->file->header.section_count++;
	section->tag = cb_ex_languages_section;
	(void)bzero((char *)section->language, sizeof(section->language));
	(void)strcpy((char *)section->language, ALL_LANGUAGES);
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	section->location.length = array_size(ex_file->file->languages) *
					sizeof(Cb_ex_language_rec);

	array_sort(ex_file->file->languages, strcmp, 0);

	ex_file->languages = hsh_create(100,
					hsh_int_equal,
					hsh_int_hash,
					(Hash_value)-1,
					(Cb_heap)NULL);

	cb_fseek(ex_file->fd, section->location.start);
	for (i = 0, n = array_size(ex_file->file->languages)-1;
	     n >= 0 ;
	     i++, n--) {
		language = (char *)ARRAY_FETCH(ex_file->file->languages, n);
		(void)bzero((char *)&rec, sizeof(rec));
		rec.major_version =
		  hsh_lookup(ex_file->language_tags, (Hash_value)EX_VERSION);
		lang = (Hash)hsh_lookup(ex_file->language_tags,
					(Hash_value)language);
		if (lang != NULL) {
			rec.minor_version = hsh_lookup(lang,
						       (Hash_value)EX_VERSION);
		} else {
			rec.minor_version = 0;
		}

		rec.grep_able = 0;
		if ((lang != NULL) &&
		    (hsh_lookup(lang, (Hash_value)EX_GREP_ABLE) != NULL)) {
			rec.grep_able = 1;
		}
		
		(void)strcpy(rec.name, language);
		(void)hsh_insert(ex_file->languages,
				 (Hash_value)language,
				 (Hash_value)i);

		semantics_width = (int)hsh_lookup(semantic_widths,
						  (Hash_value)language);
		semantics_width += 2;
		semantics_width /= CB_EX_FIELDS_PER_BYTE;
		semantics_width = cb_align(semantics_width, sizeof(int));
		(void)hsh_replace(semantic_widths,
				  (Hash_value)language,
				  (Hash_value)semantics_width);

#ifdef CB_SWAP
		cb_swap(rec.major_version);
		cb_swap(rec.minor_version);
		cb_swap(rec.grep_able);
#endif

		cb_fwrite((char *)&rec, sizeof(rec), ex_file->fd);
		}
}

/*
 *	cb_ex_write_menu_spec(ex_file)
 */
static void
cb_ex_write_menu_spec(ex_file)
	Cb_ex_file_write	ex_file;
{
	Cb_ex_section		section;

	if ((ex_file->file->menu == NULL) || (ex_file->languages == NULL)) {
		return;
	}

	section = &ex_file->file->header.section
					[ex_file->file->header.section_count];

	ex_file->file->header.section_count++;
	section->tag = cb_ex_menu_section;
	(void)bzero((char *)section->language, sizeof(section->language));
	(void)strcpy((char *)section->language, ALL_LANGUAGES);
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	cb_fseek(ex_file->fd, section->location.start);
	section->location.length = 0;
	ex_file->next_offset = section->location.start;

	cb_ex_write_menu_eliminate(ex_file, ex_file->file->menu);
	cb_ex_write_menu_layout(ex_file, ex_file->file->menu);
	cb_ex_write_menu_write(ex_file, ex_file->file->menu);
	cb_ex_write_menu_strings(ex_file);

	section->location.length = ex_file->next_offset -
						section->location.start;
}

/*
 *	cb_ex_write_menu_eliminate(ex_file, menu)
 *		Eliminate all entries that should be invisible
 */
static void
cb_ex_write_menu_eliminate(ex_file, menu)
	Cb_ex_file_write		ex_file;
	Array				menu;
{
	Cb_ex_enter_menu_item		item;
	int				i;

	if ((menu == NULL) || (ex_file->languages == NULL)) {
		return;
	}

	for (i = 0; i < array_size(menu); i++) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);
		(void)array_eliminate(item->sub_items,
				      cb_ex_write_menu_eliminate_fn1,
				      (int)ex_file);
	}

	(void)array_eliminate(menu, cb_ex_write_menu_eliminate_fn2, 0);

	for (i = 0; i < array_size(menu); i++) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);
		cb_ex_write_menu_eliminate(ex_file, item->pullright);
	}
}

/*
 *	cb_ex_write_menu_eliminate_fn1(subitem, ex_file)
 */
static int
cb_ex_write_menu_eliminate_fn1(subitem, ex_file)
	Cb_ex_enter_menu_sub_item	subitem;
	Cb_ex_file_write		ex_file;
{
	return hsh_lookup(ex_file->languages,
			  (Hash_value)subitem->language) == -1;
}

/*
 *	cb_ex_write_menu_eliminate_fn2(item)
 */
static int
cb_ex_write_menu_eliminate_fn2(item)
	Cb_ex_enter_menu_item	item;
{
	return !array_size(item->sub_items);
}

/*
 *	cb_ex_write_menu_layout(ex_file, menu)
 *		First layout the top level menu then recurse to all
 *		the pullrights
 */
static void
cb_ex_write_menu_layout(ex_file, menu)
	Cb_ex_file_write		ex_file;
	Array				menu;
{
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_sub_item	subitem;
	int				size;
	int				i;
	int				j;

	if (menu == NULL) {
		return;
	}

	for (i = 0; i < array_size(menu); i++) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);
		item->offset = ex_file->next_offset;
		/*
		 * Then compute the size of this menu item
		 */
		size = sizeof(Cb_ex_menu_item_if_rec) -
		  sizeof(Cb_ex_menu_sub_item_if_rec);
		for (j = 0; j < array_size(item->sub_items); j++) {
			subitem = (Cb_ex_enter_menu_sub_item)
			  ARRAY_FETCH(item->sub_items, j);
			(void)cb_ex_unique_menu_string(ex_file,
						       subitem->menu_label);
			if (subitem->help == NULL) {
				cb_ex_get_pullright_help(item, subitem);
			}
			if (subitem->help != NULL) {
				(void)cb_ex_unique_menu_string(ex_file,
							       subitem->help);
			}
			size += sizeof(Cb_ex_menu_sub_item_if_rec) -
			  sizeof(((Cb_ex_menu_sub_item_if)0)->semantics) +
			    (int)hsh_lookup(semantic_widths,
					    (Hash_value)subitem->language);
		}
		ex_file->next_offset += size;
	}
	ex_file->next_offset +=
	  sizeof(Cb_ex_menu_if_rec) - sizeof(Cb_ex_menu_item_if_rec);

	for (i = 0; i < array_size(menu); i++) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);
		cb_ex_write_menu_layout(ex_file, item->pullright);
	}
}

/*
 *	cb_ex_write_menu_write(ex_file, menu)
 *		First write the top level menu then recurse to all
 *		the pullrights
 */
static void
cb_ex_write_menu_write(ex_file, menu)
	Cb_ex_file_write		ex_file;
	Array				menu;
{
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_sub_item	subitem;
	int				i;
	int				j;
	Cb_ex_unique_string		string;
	Cb_ex_menu_if_rec		menu_rec;
	Cb_ex_menu_item_if_rec		menu_item_rec;
	Cb_ex_menu_sub_item_if_rec	menu_sub_item_rec;

	if ((menu == NULL) || (array_size(menu) == 0)) {
		return;
	}

	menu_rec.items = array_size(menu);
	menu_rec.swapped = 0;
#ifdef CB_SWAP
	cb_swap(menu_rec.items);
	cb_swap(menu_rec.swapped);
#endif
	cb_fwrite((char *)&menu_rec,
			sizeof(menu_rec) - sizeof(menu_rec.item),
			ex_file->fd);
	for (i = 0; i < array_size(menu); i++) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);

		menu_item_rec.pullright = 0;
		if ((item->pullright != NULL) &&
		    (array_size(item->pullright) > 0)) {
			Cb_ex_enter_menu_item	pr_item;

			pr_item = (Cb_ex_enter_menu_item)
				array_fetch(item->pullright, 0);
			menu_item_rec.pullright = pr_item->offset;
		}
		menu_item_rec.sub_items = array_size(item->sub_items);
#ifdef CB_SWAP
		cb_swap(menu_item_rec.pullright);
		cb_swap(menu_item_rec.sub_items);
#endif
		cb_fwrite((char *)&menu_item_rec,
			sizeof(menu_item_rec) - sizeof(menu_item_rec.sub_item),
			ex_file->fd);
		/*
		 * Write all the subitems (one per language)
		 */
		for (j = 0; j < array_size(item->sub_items); j++) {
			subitem = (Cb_ex_enter_menu_sub_item)
					ARRAY_FETCH(item->sub_items, j);
			menu_sub_item_rec.language =
				hsh_lookup(ex_file->languages,
					   (Hash_value)subitem->language);
			menu_sub_item_rec.invisible = subitem->invisible;
			string = cb_ex_unique_menu_string(ex_file,
							subitem->menu_label);
			if (string->offset < 0) {
				string->offset = ex_file->next_offset;
				ex_file->next_offset +=
						strlen(string->string) + 1;
			}
			menu_sub_item_rec.menu_string = string->offset;
			if (subitem->help != NULL) {
				string = cb_ex_unique_menu_string(ex_file,
								subitem->help);
				if (string->offset < 0) {
					string->offset = ex_file->next_offset;
					ex_file->next_offset +=
						strlen(string->string) + 1;
				}
				menu_sub_item_rec.help = string->offset;
			} else {
				menu_sub_item_rec.help = 0;
			}
			menu_sub_item_rec.rec_size =
			  (int)hsh_lookup(semantic_widths,
					  (Hash_value)subitem->language) +
			    sizeof(menu_sub_item_rec) -
			      sizeof(menu_sub_item_rec.semantics);
#ifdef CB_SWAP
			cb_swap(menu_sub_item_rec.language);
			cb_swap(menu_sub_item_rec.invisible);
			cb_swap(menu_sub_item_rec.menu_string);
			cb_swap(menu_sub_item_rec.help);
			cb_swap(menu_sub_item_rec.rec_size);
#endif
			cb_fwrite((char *)&menu_sub_item_rec,
				sizeof(menu_sub_item_rec) -
					sizeof(menu_sub_item_rec.semantics),
				ex_file->fd);
			array_append(ex_file->menu_item_path,
				     (Array_value)item);
			cb_ex_write_semantics(ex_file, item, subitem);
			(void)array_delete(ex_file->menu_item_path,
					array_size(ex_file->menu_item_path)-1);
		}
	}

	for (i = 0; i < array_size(menu); i++) {
		item = (Cb_ex_enter_menu_item)ARRAY_FETCH(menu, i);
		array_append(ex_file->menu_item_path,
			     (Array_value)item);
		cb_ex_write_menu_write(ex_file, item->pullright);
		(void)array_delete(ex_file->menu_item_path,
				array_size(ex_file->menu_item_path)-1);
	}
}

/*
 *	cb_ex_write_semantics(ex_file, item, subitem)
 */
static void
cb_ex_write_semantics(ex_file, item, subitem)
	Cb_ex_file_write		ex_file;
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_sub_item	subitem;
{
	char				*semantics;
	Hash				language;
	Hash				htags;
	Array				tags;
	Cb_ex_tags_item			tag;
	Cb_ex_tags_item			matched_tag;
	int				i;
	int				j;
	int				k;
	int				l;
	int				*ip;
	Cb_ex_props_item		qitem;
	char				*ctag;
	char				*buffer;
	int				found_tag = 0;
	Array				subitems;
	Hash				secondary;
	Array				menu_item_path;
	int				semantics_width;

	semantics_width = (int)hsh_lookup(semantic_widths,
					  (Hash_value)subitem->language);
	semantics = alloca(semantics_width);
	(void)bzero(semantics, semantics_width);

	/*
	 *	Find the vector of tags for this language
	 */
	language = (Hash)hsh_lookup(ex_file->language_tags,
				    (Hash_value)subitem->language);
	tags = (Array)hsh_lookup(language, (Hash_value)EX_TAGS);
	if (tags == NULL) {
		cb_ex_fatal("No tags for language `%s'\n", subitem->language);
	}
	htags = (Hash)hsh_lookup(language, (Hash_value)EX_TAGS_SET);
	if (htags == NULL) {
		htags = hsh_create(100,
				   hsh_int_equal,
				   hsh_int_hash,
				   (Hash_value)-1,
				   (Cb_heap)NULL);
		(void)hsh_insert(language,
				 (Hash_value)EX_TAGS_SET,
				 (Hash_value)htags);
		for (i = array_size(tags)-1; i >= 0; i--) {
			(void)hsh_insert(htags,
				(Hash_value)((Cb_ex_tags_item)ARRAY_FETCH(tags, i))->
								full_name,
				(Hash_value)i);
		}
	}
	if (subitem->props == NULL) {
		cb_ex_get_pullright_semvec(item, subitem);
	}

	/*
	 *	For each tag we check if it would match this query.
	 *	If it would we set the bit in the semantics vector.
	 *	The query matched iff all props it carries are found
	 *	for the tag.
	 *	The query has an array of props lists, one for each ORd
	 *	props list. Each such is an array of props.
	 */
	for (l = array_size(subitem->props)-1; l >= 0; l--) {
	    subitems = (Array)ARRAY_FETCH(subitem->props, l);
	    for (i = array_size(tags)-1; i >= 0; i--) {
		tag = (Cb_ex_tags_item)ARRAY_FETCH(tags, i);
		/*
		 * We scan thru each tag for the language. For each tag
		 * we check if this query would match by searching for
		 * all props the query needs on the tag props list.
		 */
		for (j = array_size(subitems)-1; j >= 0; j--) {
			qitem = (Cb_ex_props_item)ARRAY_FETCH(subitems, j);
			if (hsh_lookup(tag->props, (Hash_value)qitem) != NULL){
				continue;
			}
			goto no_match;
		}
		/*
		 * We now know that all props for the query are defined
		 * by this tag.
		 */
		matched_tag = tag;
		if (tag->secondary != NULL) {
		    /*
		     * Check if there are any secondary tags for this tag
		     */
		    for (j = array_size(tag->secondary)-1; j >= 0; j--) {
			ctag = (char *)ARRAY_FETCH(tag->secondary, j);
			k = hsh_lookup(htags, (Hash_value)ctag);
			if (k != -1) {
			    if (cb_ex_get_sem_field(semantics, k+1) != CB_EX_MATCH) {
				cb_ex_set_sem_field(semantics,
					k+1,
					CB_EX_SECONDARY_MATCH);
			    }
			    goto found_secondary_tag;
			}
			cb_ex_fatal("Undefined tag `%s' in secondary statement for `%s'. Language = `%s', item = `%s'\n",
				ctag,
				tag->name,
				subitem->language,
				subitem->menu_label);
		found_secondary_tag:;
		    }
		}
		found_tag++;
		cb_ex_set_sem_field(semantics, i+1, CB_EX_MATCH);
	    no_match:;
	    }
	}

	switch (found_tag) {
	case 0:
		cb_ex_warning("Menu item `%s' (language `%s') did not match any tags\n",
			subitem->menu_label,
			subitem->language);
		break;
	case 1:
		if (array_size(ex_file->menu_item_path) == 0) {
			cb_ex_fatal("No path for secondary tag\n");
		}
		if (item->focus != NULL) {
			for (i = array_size(item->focus)-1; i >= 0; i--) {
				ip = (int *)ARRAY_FETCH(item->focus, i);
				*ip = item->offset;
			}
			break;
		}
		secondary = (Hash)hsh_lookup(language,
					     (Hash_value)EX_SECONDARY);
		if (secondary == NULL) {
			break;
		}
		buffer = alloca(strlen(matched_tag->prefix) +
				strlen(matched_tag->name) + 32);
		(void)sprintf(buffer, "%s_%s",
			matched_tag->prefix,
			matched_tag->name);
		buffer = cb_string_unique(buffer);
		menu_item_path = (Array)hsh_lookup(secondary,
						   (Hash_value)buffer);
						
		if ((menu_item_path != NULL) &&
		    (array_size(menu_item_path) <
					array_size(ex_file->menu_item_path))) {
			menu_item_path = array_create((Cb_heap)NULL);
			(void)hsh_replace(secondary,
					  (Hash_value)buffer,
					  (Hash_value)menu_item_path);
			for (i = 0;
			     i < array_size(ex_file->menu_item_path);
			     i++) {
				array_append(menu_item_path,
					(Array_value)ARRAY_FETCH(ex_file->menu_item_path,i));
			}
		}
		break;
	default:
		if (item->focus != NULL) {
			for (i = array_size(item->focus)-1; i >= 0; i--) {
				ip = (int *)ARRAY_FETCH(item->focus, i);
				*ip = item->offset;
			}
		}
		break;
	}
	cb_fwrite(semantics, semantics_width, ex_file->fd);
}

/*
 *	cb_ex_get_pullright_semvec(item, subitem)
 *		Fetch the semantic vector for a menu item with a pullright.
 *		Get the vector from the first menu item of the pullright,
 *		recursively.
 */
static void
cb_ex_get_pullright_semvec(item, subitem)
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_sub_item	subitem;
{
	Cb_ex_enter_menu_item		pr_item;
	Cb_ex_enter_menu_sub_item	pr_sitem;
	int				i;
	int				j;

	if (item->pullright == NULL) {
		cb_ex_fatal("Pullright props not specified for menu item `%s' language `%s'\n",
				subitem->menu_label, subitem->language);
	}

	for (i = 0; i < array_size(item->pullright); i++) {
		pr_item = (Cb_ex_enter_menu_item)ARRAY_FETCH(item->pullright,
								i);
		for (j = 0; j < array_size(pr_item->sub_items); j++) {
			pr_sitem = (Cb_ex_enter_menu_sub_item)
					ARRAY_FETCH(pr_item->sub_items, j);
			if (pr_sitem->language == subitem->language) {
				if (pr_sitem->props == NULL) {
					cb_ex_get_pullright_semvec(pr_item,
								pr_sitem);
				}
				subitem->props = pr_sitem->props;
				return;
			}
		}
	}
	cb_ex_fatal("No props found for pullright\n");
}

/*
 *	cb_ex_get_pullright_help(item, subitem)
 *		Fetch the help message for a menu item with a pullright.
 *		Get it from the first menu item of the pullright,
 *		recursively.
 */
static void
cb_ex_get_pullright_help(item, subitem)
	Cb_ex_enter_menu_item		item;
	Cb_ex_enter_menu_sub_item	subitem;
{
	Cb_ex_enter_menu_item		pr_item;
	Cb_ex_enter_menu_sub_item	pr_sitem;
	int				i;
	int				j;

	if (item->pullright == NULL) {
		return;
	}

	for (i = 0; i < array_size(item->pullright); i++) {
		pr_item = (Cb_ex_enter_menu_item)ARRAY_FETCH(item->pullright,
								i);
		for (j = 0; j < array_size(pr_item->sub_items); j++) {
			pr_sitem = (Cb_ex_enter_menu_sub_item)
					ARRAY_FETCH(pr_item->sub_items, j);
			if (pr_sitem->language == subitem->language) {
				if (pr_sitem->help == NULL) {
					cb_ex_get_pullright_help(pr_item,
								pr_sitem);
				}
				subitem->help = pr_sitem->help;
				return;
			}
		}
	}
}

/*
 *	cb_ex_write_menu_strings(ex_file)
 */
static void
cb_ex_write_menu_strings(ex_file)
	Cb_ex_file_write	ex_file;
{
	Array			strings;
	Cb_ex_unique_string	string;
	int			i;

	if (ex_file->strings == NULL) {
		return;
	}
	strings = array_create((Cb_heap)NULL);
	(void)hsh_iterate(ex_file->strings,
			  cb_ex_write_get_strings,
			  (int)strings);
	array_sort(strings, cb_ex_write_sort_strings, 0);

	for (i = array_size(strings)-1; i >= 0; i--) {
		string = (Cb_ex_unique_string)ARRAY_FETCH(strings, i);
		cb_fwrite(string->string,
			  strlen(string->string) + 1,
			  ex_file->fd);
	}
}

/*
 *	cb_ex_write_sort_strings(a, b)
 */
static int
cb_ex_write_sort_strings(a, b)
	Cb_ex_unique_string	a;
	Cb_ex_unique_string	b;
{
	return a->offset - b->offset;
}


/*
 *	cb_ex_write_get_strings(string, ignore, strings)
 */
/*ARGSUSED*/
static int
cb_ex_write_get_strings(string, ignore, strings)
	char	*string;
	char	*ignore;
	Array	strings;
{
	array_append(strings, (Array_value)string);
}

/*
 *	cb_ex_unique_menu_string(ex_file, string)
 */
static Cb_ex_unique_string
cb_ex_unique_menu_string(ex_file, string)
	Cb_ex_file_write	ex_file;
	char			*string;
{
	Cb_ex_unique_string_rec	rec;
	Cb_ex_unique_string	value;

	if (ex_file->strings == NULL) {
		ex_file->strings = hsh_create(100,
				cb_ex_unique_equal,
				cb_ex_unique_hash,
				(Hash_value)NULL,
				(Cb_heap)NULL);
	}
	rec.string = string;
	value = (Cb_ex_unique_string)hsh_lookup(ex_file->strings,
						(Hash_value)&rec);
	if (value == NULL) {
		value = cb_allocate(Cb_ex_unique_string, (Cb_heap)NULL);
		value->string = string;
		value->offset = -1;
		(void)hsh_insert(ex_file->strings,
				 (Hash_value)value,
				 (Hash_value)value);
	}
	return value;
}

/*
 *	cb_ex_unique_equal(a, b)
 */
static int
cb_ex_unique_equal(a, b)
	Cb_ex_unique_string	a;
	Cb_ex_unique_string	b;
{
	return a->string == b->string;
}

/*
 *	cb_ex_unique_hash(a)
 */
static int
cb_ex_unique_hash(a)
	Cb_ex_unique_string	a;
{
	return (int)a->string;
}

/*
 *	cb_ex_write_node_menus(ex_file, help)
 *		Write a menu section for the node menu.
 */
/*ARGSUSED*/
static void
cb_ex_write_node_menus(ex_file, help)
	Cb_ex_file_write	ex_file;
	Node_menu_help		help;
{
	Array			nodes;
	Cb_ex_enter_node	node_menu;
	int			i;
	int			j;
	Array			list;
	int			menu_offset;

	nodes = (Array)hsh_lookup(help->table, (Hash_value)EX_NODE);
	if (nodes != NULL) {
		for (i = array_size(nodes)-1; i >= 0; i--) {
			node_menu = (Cb_ex_enter_node)ARRAY_FETCH(nodes, i);
			if (node_menu->menu_offset != -1) {
				continue;
			}
			ex_file->file->menu = NULL;
			ex_file->strings = NULL;
			ex_file->menu_item_path = array_create((Cb_heap)NULL);
			cb_ex_extra_menu = 1;

			list = node_menu->equiv_list;
			for (j = array_size(list)-1; j >= 0; j--) {
				node_menu = (Cb_ex_enter_node)ARRAY_FETCH(list,
									  j);
				cb_ex_language = node_menu->language;
				(void)cb_ex_menu_statement(node_menu->menu);
				ex_file->file->sections--;
			}
			cb_ex_write_menu_spec(ex_file);

			menu_offset =
			  ex_file->file->header.section
			    [ex_file->file->header.section_count-1].
			      location.start;
			for (j = array_size(list)-1; j >= 0; j--) {
				node_menu = (Cb_ex_enter_node)ARRAY_FETCH(list,
									  j);
				node_menu->menu_offset = menu_offset;
			}
		}
	}
}

/*
 *	cb_ex_write_node_menus_collect(language_name, language, list)
 */
static int
cb_ex_write_node_menus_collect(language_name, language, list)
	char			*language_name;
	Hash			language;
	Array			list;
{
	Node_menu_help		help;

	if (language_name <= (char *)EX_LAST_TOKEN) {
		return;
	}

	help = cb_allocate(Node_menu_help, (Cb_heap)NULL);
	help->name = language_name;
	help->table = language;
	array_append(list, (Array_value)help);
}

/*
 *	cb_ex_write_node_menus_sort(a, b)
 */
static int
cb_ex_write_node_menus_sort(a, b)
	Node_menu_help		a;
	Node_menu_help		b;
{
	return strcmp(a->name, b->name);
}

/*
 *	cb_ex_write_graph_menus(ex_file)
 *		Write a menu section for the graph menu.
 */
/*ARGSUSED*/
static void
cb_ex_write_graph_menus(ex_file)
	Cb_ex_file_write	ex_file;
{
	Cb_ex_enter_graph	graph;
	Cb_ex_enter_sub_graph	sub_graph;
	int			i;
	int			j;

	if (ex_file->file->graphs == NULL) {
		return;
	}

	for (i = array_size(ex_file->file->graphs)-1; i >= 0; i--) {
		graph = (Cb_ex_enter_graph)ARRAY_FETCH(ex_file->file->graphs,
						       i);
		if ((graph->menus == NULL) || (array_size(graph->menus) == 0)){
			cb_ex_fatal("graph statement with no menu\n");
		}
		ex_file->file->menu = NULL;
		ex_file->strings = NULL;
		ex_file->menu_item_path = array_create((Cb_heap)NULL);
		ex_file->file->sections--;
		for (j = array_size(graph->menus)-1; j >= 0; j--) {
			sub_graph =
			  (Cb_ex_enter_sub_graph)ARRAY_FETCH(graph->menus, j);
			cb_ex_language = sub_graph->language;
			cb_ex_extra_menu = 1;
			(void)cb_ex_menu_statement(sub_graph->menu);
		}
		cb_ex_write_menu_spec(ex_file);
		graph->menu_offset =
		  ex_file->file->header.section
		    [ex_file->file->header.section_count-1].
		      location.start;
	}
}

typedef struct Secondary_help_tag	*Secondary_help, Secondary_help_rec;
struct Secondary_help_tag {
	char	*name;
	Hash	table;
};

/*
 *	cb_ex_write_focus_unit(ex_file)
 */
static void
cb_ex_write_focus_unit(ex_file)
	Cb_ex_file_write	ex_file;
{
	Cb_ex_section		section;
	int			i;
	Cb_ex_focus_if_rec	rec;
	Cb_ex_enter_focus_unit	unit;
	int			next_offset;
	char			*buffer;
	int			length;
	int			units_count = 0;

	if (ex_file->file->focus_units == NULL) {
		return;
	}
	section = &ex_file->file->header.section
					[ex_file->file->header.section_count];

	ex_file->file->header.section_count++;
	section->tag = cb_ex_focus_section;
	(void)bzero((char *)section->language, sizeof(section->language));
	(void)strcpy((char *)section->language, ALL_LANGUAGES);
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	section->location.length = 0;

	cb_fseek(ex_file->fd, section->location.start);

	for (i = 0; i < array_size(ex_file->file->focus_units); i++) {
		unit = (Cb_ex_enter_focus_unit)
				ARRAY_FETCH(ex_file->file->focus_units, i);
		if (unit->invisible) {
			continue;
		}
		units_count++;
	}

	next_offset = section->location.start + (units_count+1) * sizeof(rec);
	for (i = 0; i < array_size(ex_file->file->focus_units); i++) {
		unit = (Cb_ex_enter_focus_unit)
				ARRAY_FETCH(ex_file->file->focus_units, i);
		if (unit->invisible) {
			continue;
		}
		rec.focus_unit_name = next_offset;
		next_offset += strlen(unit->focus_unit_name)+1;

		rec.unit_search_pattern = next_offset;
		buffer = alloca(strlen(unit->tag) + 32);
		buffer[0] = CB_EX_FUNNY_HEADER_BYTE;
		buffer[1] = CB_EX_FOCUS_HEADER_BYTE;
		(void)sprintf(buffer+2, " %s %%s *", unit->tag);
		unit->search_pattern = cb_string_unique(buffer);
		next_offset += strlen(unit->search_pattern)+1;
		rec.unit_menu_item = unit->menu_item->offset + 4;


		rec.make_absolute = unit->make_absolute;
		rec.use_refd = unit->use_refd;

		section->location.length += sizeof(rec);

#ifdef CB_SWAP
		cb_swap(rec.focus_unit_name);
		cb_swap(rec.unit_search_pattern);
		cb_swap(rec.unit_menu_item);
		cb_swap(rec.use_refd);
		cb_swap(rec.make_absolute);
#endif
		cb_fwrite((char *)&rec, sizeof(rec), ex_file->fd);
	}
	(void)bzero((char *)&rec, sizeof(rec));
	section->location.length += sizeof(rec);
	cb_fwrite((char *)&rec, sizeof(rec), ex_file->fd);

	for (i = 0; i < array_size(ex_file->file->focus_units); i++) {
		unit = (Cb_ex_enter_focus_unit)
				ARRAY_FETCH(ex_file->file->focus_units, i);

		if (unit->invisible) {
			continue;
		}

		cb_fwrite(unit->focus_unit_name,
			length = strlen(unit->focus_unit_name)+1,
			ex_file->fd);
		section->location.length += length;

		cb_fwrite(unit->search_pattern,
			length = strlen(unit->search_pattern)+1,
			ex_file->fd);
		section->location.length += length;

	}
}

/*
 *	cb_ex_write_tags(ex_file)
 */
static void
cb_ex_write_tags(ex_file)
	Cb_ex_file_write	ex_file;
{
	int			n;

	if (ex_file->file->tags == NULL) {
		return;
	}
	for (n = array_size(ex_file->file->tags)-1; n >= 0; n--) {
		cb_ex_write_tags_section(ex_file,
			(Cb_ex_tags_im)ARRAY_FETCH(ex_file->file->tags, n));
	}
}

/*
 *	cb_ex_write_tags_section(ex_file, item)
 */
static void
cb_ex_write_tags_section(ex_file, item)
	Cb_ex_file_write	ex_file;
	Cb_ex_tags_im		item;
{
	Cb_ex_section		section;
	int			i;
	Cb_ex_tags_item		tag_item;
	int			length;

	if (item->tags == NULL) {
		return;
	}
	section = &ex_file->file->header.section
					[ex_file->file->header.section_count];

	ex_file->file->header.section_count++;
	section->tag = cb_ex_tags_section;
	(void)bzero((char *)section->language, sizeof(section->language));
	(void)strcpy((char *)section->language, item->language);
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	section->location.length = 0;

	cb_fseek(ex_file->fd, section->location.start);
	(void)putc(0, ex_file->fd);
	section->location.length++;
	for (i = 0; i < array_size(item->tags); i++) {
		char *tag;

		tag_item = (Cb_ex_tags_item)ARRAY_FETCH(item->tags, i);
		tag = alloca(strlen(tag_item->prefix) +
			     strlen(tag_item->name) + 32);
		(void)sprintf(tag, "cb_%s_%s",
				tag_item->prefix, tag_item->name);
		length = strlen(tag) + 1;
		section->location.length += length;
		cb_fwrite(tag, length, ex_file->fd);
	}
}

typedef struct Comment_help_tag		*Comment_help, Comment_help_rec;
struct Comment_help_tag {
	char	*name;
	Hash	table;
};

/*
 *	cb_ex_write_comment_delimiters(ex_file)
 */
static void
cb_ex_write_comment_delimiters(ex_file)
	Cb_ex_file_write	ex_file;
{
	Array			list;
	Cb_ex_section		section;
	int			length;
	char			*start;
	char			*end;
	Comment_help		help;
	int			n;

	list = array_create((Cb_heap)NULL);
	(void)hsh_iterate(ex_file->language_tags,
			  cb_ex_write_comment_delimiters_section,
			  (int)list);
	array_sort(list, cb_ex_write_comment_delimiters_sort, 0);

	for (n = array_size(list)-1; n >= 0; n--) {
		help = (Comment_help)ARRAY_FETCH(list, n);

		start = (char *)hsh_lookup(help->table,
					   (Hash_value)EX_LANGUAGE_START_COMMENT);
		end = (char *)hsh_lookup(help->table,
					 (Hash_value)EX_LANGUAGE_END_COMMENT);
		if (start == NULL) {
			continue;
		}

		section = &ex_file->file->header.section
		  [ex_file->file->header.section_count];

		ex_file->file->header.section_count++;
		section->tag = cb_ex_comment_delimiter_section;
		(void)bzero((char *)section->language,
			    sizeof(section->language));
		(void)strcpy((char *)section->language,
			     (char *)hsh_lookup(help->table,
						(Hash_value)EX_LANGUAGE));
		section->location.start =
		  cb_align((section-1)->location.start +
			   (section-1)->location.length,
			   sizeof(int));
		section->location.length = 0;

		cb_fseek(ex_file->fd, section->location.start);

		length = strlen(start) + 1;
		section->location.length += length;
		cb_fwrite(start, length, ex_file->fd);

		if (end == NULL) {
			end = "";
		}
		length = strlen(end) + 1;
		section->location.length += length;
		cb_fwrite(end, length, ex_file->fd);

		section->location.length = cb_align(section->location.length,
						    sizeof(int));
	}
}

/*
 *	cb_ex_write_comment_delimiters_section(language_name,language, list)
 */
static int
cb_ex_write_comment_delimiters_section(language_name, language, list)
	char			*language_name;
	Hash			language;
	Array			list;
{
	Comment_help		help;

	if (language_name <= (char *)EX_LAST_TOKEN) {
		return;
	}

	help = cb_allocate(Comment_help, (Cb_heap)NULL);
	help->name = language_name;
	help->table = language;
	array_append(list, (Array_value)help);
}

/*
 *	cb_ex_write_comment_delimiters_sort(a, b)
 */
static int
cb_ex_write_comment_delimiters_sort(a, b)
	Comment_help	a;
	Comment_help	b;
{
	return strcmp(a->name, b->name);
}

typedef struct Rendering_help_tag	*Rendering_help, Rendering_help_rec;
struct Rendering_help_tag {
	char		*name;
	Hash		table;
};

/*
 *	cb_ex_write_renderings(ex_file)
 */
static void
cb_ex_write_renderings(ex_file)
	Cb_ex_file_write	ex_file;
{
	Cb_ex_section		section;
	Array			renderings;
	Cb_ex_enter_rendering	rendering;
	int			i;
	Cb_ex_rendering_if_rec	rec;
	Array			rendering_list;
	Rendering_help		help;
	int			n;

	rendering_list = array_create((Cb_heap)NULL);
	(void)hsh_iterate(ex_file->language_tags,
			  cb_ex_write_renderings_section,
			  (int)rendering_list);

	array_sort(rendering_list, cb_ex_write_renderings_sort, 0);

	for (n = array_size(rendering_list)-1; n >= 0; n--) {
		help = (Rendering_help)ARRAY_FETCH(rendering_list, n);

		renderings = (Array)hsh_lookup(help->table,
					       (Hash_value)EX_RENDER);
		section = &ex_file->file->header.section
		  [ex_file->file->header.section_count];

		ex_file->file->header.section_count++;
		section->tag = cb_ex_rendering_section;
		(void)bzero((char *)section->language,
			    sizeof(section->language));
		(void)strcpy((char *)section->language,
			     (char *)hsh_lookup(help->table,
						(Hash_value)EX_LANGUAGE));
		section->location.start =
		  cb_align((section-1)->location.start +
			   (section-1)->location.length,
			   sizeof(int));
		section->location.length = 0;

		cb_fseek(ex_file->fd, section->location.start);

		for (i = array_size(renderings)-1; i >= 0; i --) {
			rendering =
			  (Cb_ex_enter_rendering)ARRAY_FETCH(renderings, i);

			rec.tag = rendering->tag->ordinal + 1;
			rec.rendering = (int)rendering->rendering;
			if (rendering->color != NULL) {
				rec.color.red = rendering->color->red;
				rec.color.green = rendering->color->green;
				rec.color.blue = rendering->color->blue;
			} else {
				rec.color.red = -1;
				rec.color.green = -1;
				rec.color.blue = -1;
			}
#ifdef CB_SWAP
			cb_swap(rec.tag);
			cb_swap(rec.rendering);
			cb_swap(rec.color.red);
			cb_swap(rec.color.green);
			cb_swap(rec.color.blue);
#endif
			section->location.length += sizeof(rec);
			cb_fwrite((char *)&rec, sizeof(rec), ex_file->fd);
		}
		section->location.length = cb_align(section->location.length,
						    sizeof(int));
	}
}

/*
 *	cb_ex_write_renderings_section(language_name, language, list)
 */
/*ARGSUSED*/
static int
cb_ex_write_renderings_section(language_name, language, list)
	char			*language_name;
	Hash			language;
	Array			list;
{
	Rendering_help		help;
	Array			renderings;

	if (language_name <= (char *)EX_LAST_TOKEN) {
		return;
	}
	renderings = (Array)hsh_lookup(language, (Hash_value)EX_RENDER);
	if (renderings == NULL) {
		return;
	}

	help = cb_allocate(Rendering_help, (Cb_heap)NULL);
	help->name = language_name;
	help->table = language;
	array_append(list, (Array_value)help);
}

/*
 *	cb_ex_write_renderings_sort(a, b)
 */
static int
cb_ex_write_renderings_sort(a, b)
	Rendering_help	a;
	Rendering_help	b;
{
	return strcmp(a->name, b->name);
}

/*
 *	cb_ex_write_nodes(ex_file)
 */
static void
cb_ex_write_nodes(ex_file)
	Cb_ex_file_write	ex_file;
{
	(void)hsh_iterate(ex_file->language_tags,
			  cb_ex_write_nodes_section,
			  (int)ex_file);
}

/*
 *	cb_ex_write_nodes_section(language_name, language, ex_file)
 */
/*ARGSUSED*/
static int
cb_ex_write_nodes_section(language_name, language, ex_file)
	char			*language_name;
	Hash			language;
	Cb_ex_file_write	ex_file;
{
	Cb_ex_section		section;
	Array			nodes;
	Cb_ex_enter_node	node;
	int			i;
	Cb_ex_node_menu_if_rec	rec;

	if (language_name <= (char *)EX_LAST_TOKEN) {
		return 0;
	}

	nodes = (Array)hsh_lookup(language, (Hash_value)EX_NODE);
	if (nodes == NULL) {
		return 0;
	}

	section = &ex_file->file->header.section
					[ex_file->file->header.section_count];

	ex_file->file->header.section_count++;
	section->tag = cb_ex_node_menu_section;
	(void)bzero((char *)section->language, sizeof(section->language));
	(void)strcpy((char *)section->language,
		     (char *)hsh_lookup(language,
					(Hash_value)EX_LANGUAGE));
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	section->location.length = 0;

	cb_fseek(ex_file->fd, section->location.start);

	for (i = array_size(nodes)-1; i >= 0; i --) {
		node = (Cb_ex_enter_node)ARRAY_FETCH(nodes, i);

		rec.tag = node->tag->ordinal + 1;
		rec.menu = node->menu_offset;
#ifdef CB_SWAP
		cb_swap(rec.tag);
		cb_swap(rec.menu);
#endif
		section->location.length += sizeof(rec);
		cb_fwrite((char *)&rec, sizeof(rec), ex_file->fd);
	}
	section->location.length = cb_align(section->location.length,
					    sizeof(int));
	return 0;
}

/*
 *	cb_ex_write_graph_section(ex_file)
 */
static void
cb_ex_write_graph_section(ex_file)
	Cb_ex_file_write	ex_file;
{
	Cb_ex_section		section;
	Cb_ex_enter_graph	graph;
	int			i;
	Cb_ex_graph_if		rec;
	int			length;

	if (ex_file->file->graphs == NULL) {
		return;
	}

	section = &ex_file->file->header.section
					[ex_file->file->header.section_count];

	ex_file->file->header.section_count++;
	section->tag = cb_ex_graph_section;
	(void)bzero((char *)section->language, sizeof(section->language));
	(void)strcpy((char *)section->language, ALL_LANGUAGES);
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	section->location.length = 0;

	cb_fseek(ex_file->fd, section->location.start);

	for (i = 0; i < array_size(ex_file->file->graphs); i++) {
		graph = (Cb_ex_enter_graph)ARRAY_FETCH(ex_file->file->graphs,
						       i);

		length = cb_align(sizeof(*rec) + strlen(graph->graph_name),
				  sizeof(int));
		rec = (Cb_ex_graph_if)alloca(length);
		rec->rec_length = length;
		rec->menu = graph->menu_offset;
		rec->tag = graph->graph_tag[0];
		(void)strcpy(rec->name, graph->graph_name);
#ifdef CB_SWAP
		cb_swap(rec->rec_length);
		cb_swap(rec->menu);
		cb_swap(rec->tag);
#endif
		section->location.length += length;
		cb_fwrite((char *)rec, length, ex_file->fd);
	}
	section->location.length = cb_align(section->location.length,
					    sizeof(int));
}

/*
 *	cb_ex_write_weight(ex_file)
 */
static void
cb_ex_write_weight(ex_file)
	Cb_ex_file_write	ex_file;
{
	int			n;

	if (ex_file->file->tags == NULL) {
		return;
	}
	array_sort(ex_file->file->tags, cb_ex_write_weight_sort, 0);
	for (n = array_size(ex_file->file->tags)-1; n >= 0; n--) {
		cb_ex_write_weight_section(ex_file,
			(Cb_ex_tags_im)ARRAY_FETCH(ex_file->file->tags, n));
	}
}

/*
 *	cb_ex_write_weight_sort(a, b)
 */
static int
cb_ex_write_weight_sort(a, b)
	Cb_ex_tags_im	a;
	Cb_ex_tags_im	b;
{
	return strcmp(a->language, b->language);
}

/*
 *	cb_ex_write_weight_section(ex_file, item)
 */
static void
cb_ex_write_weight_section(ex_file, item)
	Cb_ex_file_write	ex_file;
	Cb_ex_tags_im		item;
{
	Cb_ex_section		section;
	int			i;
	Cb_ex_tags_item		tag_item;
	int			limit;

	if (item->tags == NULL) {
		return;
	}
	section = &ex_file->file->header.section
					[ex_file->file->header.section_count];

	ex_file->file->header.section_count++;
	section->tag = cb_ex_weight_section;
	(void)bzero((char *)section->language, sizeof(section->language));
	(void)strcpy((char *)section->language, item->language);
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	section->location.length = 0;

	cb_fseek(ex_file->fd, section->location.start);
	(void)putc(0, ex_file->fd);
	section->location.length++;
	for (i = 0; i < array_size(item->tags); i++) {
		tag_item = (Cb_ex_tags_item)ARRAY_FETCH(item->tags, i);
		section->location.length++;
		(void)putc(tag_item->weight, ex_file->fd);
	}

	limit = cb_align(section->location.length, sizeof(int));
	while (limit > section->location.length) {
		section->location.length++;
		(void)putc(0, ex_file->fd);
	}
}

