#ident "@(#)cb_cpp_proto.c 1.1 94/10/31 SMI"

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


#ifndef cb_cpp_protocol_h_INCLUDED
#include "cb_cpp_protocol.h"
#endif

#ifndef cb_cpp_tags_h_INCLUDED
#include "cb_cpp_tags.h"
#endif

#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_line_id_h_INCLUDED
#include "cb_line_id.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif


	int			cb_hidden = 0;
static	Cb_cpp_get_char_fn	cb_cpp_get_chr;
extern	Cb_file_write		cb_enter_cb_file;
/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	int	cb_cpp_protocol();
static	char	*cb_cpp_get_id();

/*
 *	cb_cpp_protocol(chr, get_chr, get_id, get_block, infile)
 *		This will process a message from cpp that is prefixed by
 *		CB_CHR_PREFIX.  "get_chr(infile)" is called to get the
 *		next character.  "get_id(infile)" is called to get an
 *		identifier that is terminated by CB_CHR_END_ID.  If "get_id",
 *		is NULL, a generic (i.e. slow) routine that calls "get_chr"
 *		will be used to obtain an id.  "get_block(...) is called to
 *		read a block of characters.  "chr" is the first character
 *		after CB_CHR_PREFIX.  TRUE is returned if a control-A
 *		was encountered in the source.
 */
int
cb_cpp_protocol(chr, get_chr, get_id, get_block, infile)
	char				chr;
	register Cb_cpp_get_char_fn	get_chr;
	register Cb_cpp_get_id_fn	get_id;
	Cb_cpp_get_block_fn		get_block;
	register FILE			*infile;
{
static	char				*macro_name;
static	int				macro_save_name = FALSE;
static	int				macro_start_line = -1;
static	int				cb_in_if = 0;
static	int				cb_start_inactive_line = -1;
	register int			tag;
	char				*symbol;

	if (get_id == NULL) {
		cb_cpp_get_chr = get_chr;
		get_id = cb_cpp_get_id;
	}
	switch (chr) {
	case CB_CHR_IF:
		cb_in_if = 1;
		return FALSE;
	case CB_CHR_IF_END_ACTIVE:
		(void)cb_enter_symbol("#if",
				      *cb_lineno_addr,
				      (unsigned int)cb_cpp_if_block_active);
		cb_in_if = 0;
		return FALSE;
	case CB_CHR_IF_END_INACTIVE:
		(void)cb_enter_symbol("#if",
				      *cb_lineno_addr,
				      (unsigned int)cb_cpp_if_block_inactive);
		cb_in_if = 0;
		return FALSE;
	case CB_CHR_IFDEF_DEFD:
		tag = (int)cb_cpp_ref_macro_from_ifdef_defd;
		break;
	case CB_CHR_IFDEF_UNDEFD:
		tag = (int)cb_cpp_ref_macro_from_ifdef_undefd;
		break;
	case CB_CHR_IFNDEF_DEFD:
		tag = (int)cb_cpp_ref_macro_from_ifndef_defd;
		break;
	case CB_CHR_IFNDEF_UNDEFD:
		tag = (int)cb_cpp_ref_macro_from_ifndef_undefd;
		break;
	case CB_CHR_IDENT:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_CONTROL_A:
		return TRUE;
	case CB_CHR_LINE_ID:
		{
			register int	count;
			Cb_line_id	line_ids;
			register int	size;
			register char	*pointer;

			count = ((*get_chr)(infile) & 0xff);
			count |= ((*get_chr)(infile) & 0xff) << 8;

			size = count * sizeof(Cb_line_id_rec);
			line_ids =
			  (Cb_line_id)cb_get_memory(size,
						 cb_enter_get_cb_file()->heap);
			if (get_block == NULL) {
				pointer = (char *)line_ids;
				while (size-- > 0) {
					*pointer++ = (*get_chr)(infile);
				}
			} else {
				(*get_block)(line_ids, size, infile);
			}
			cb_enter_line_id_block(line_ids, count);
			return FALSE;
		}
	case CB_CHR_MACRO_DEF_W_FORMALS:
		tag = (int)cb_cpp_def_macro_w_formal;
		macro_start_line = *cb_lineno_addr;
		macro_save_name = TRUE;
		break;
	case CB_CHR_MACRO_DEF_WO_FORMALS:
		tag = (int)cb_cpp_def_macro_wo_formal;
		macro_start_line = *cb_lineno_addr;
		macro_save_name = TRUE;
		break;
	case CB_CHR_MACRO_DEF_END:
		if ((*cb_lineno_addr != macro_start_line) &&
		    (macro_start_line > 0)) {
			char	*buffer;

			buffer = alloca(strlen(CB_EX_FOCUS_UNIT_MACRO_FORMAT) +
					strlen(macro_name) +
					100);
			(void)sprintf(buffer, CB_EX_FOCUS_UNIT_MACRO_FORMAT,
				      macro_name,
				      macro_start_line,
				      *cb_lineno_addr);
			(void)cb_enter_symbol(buffer,
					      macro_start_line,
					      (int)cb_focus_macro_unit);
			macro_start_line = -1;
		}
		return FALSE;
	case CB_CHR_MACRO_REF_W_FORMALS_DEFD:
		if (cb_hidden != 0) {
			(void)(*get_id)(infile);
			cb_hidden = 1;
			return FALSE;
		}
		tag = (int)(cb_in_if ?
			    cb_cpp_ref_macro_from_if_defd :
			    cb_cpp_ref_macro_w_formal_from_source);
		cb_hidden = 1;
		break;
	case CB_CHR_MACRO_REF_WO_FORMALS_DEFD:
		if (cb_hidden != 0) {
			(void)(*get_id)(infile);
			cb_hidden = 1;
			return FALSE;
		}
		tag = (int)(cb_in_if ?
			    cb_cpp_ref_macro_from_if_defd :
			    cb_cpp_ref_macro_wo_formal_from_source);
		cb_hidden = 1;
		break;
	case CB_CHR_MACRO_REF_WO_FORMALS_UNDEFD:
		if (cb_hidden != 0) {
			(void)(*get_id)(infile);
			cb_hidden = 1;
			return FALSE;
		}
		tag = (int)(cb_in_if ?
			    cb_cpp_ref_macro_from_if_undefd :
			    cb_cpp_ref_macro_wo_formal_from_source);
		cb_hidden = 1;
		break;
	case CB_CHR_MACRO_REF_END:
		cb_hidden = 0;
		return FALSE;
	case CB_CHR_MACRO_FORMAL_DEF:
		tag = (int)cb_cpp_def_macro_formal;
		break;
	case CB_CHR_MACRO_FORMAL_REF:
		tag = (int)cb_cpp_ref_macro_formal;
		break;
	case CB_CHR_MACRO_ACTUAL_REF:
		tag = (int)cb_cpp_ref_macro_actual;
		break;
	case CB_CHR_MACRO_NON_FORMAL_REF:
		tag = (int)cb_cpp_ref_from_macro_body;
		break;
	case CB_CHR_UNDEF:
		tag = (int)cb_cpp_def_macro_undef;
		break;
	case CB_CHR_START_INACTIVE:
		cb_start_inactive_line = *cb_lineno_addr;
		return FALSE;
	case CB_CHR_END_INACTIVE:
		cb_enter_inactive_lines(cb_start_inactive_line,
					*cb_lineno_addr);
		return FALSE;
	case CB_CHR_SAYLINE:
		return FALSE;
	case CB_CHR_LINENO:
		cbgetlineno((*get_chr), infile, *cb_lineno_addr);
		return FALSE;
	case CB_CHR_MACRO_STRING_IN_DEF:
		tag = (int)cb_cpp_ref_string_from_macro_body;
		break;
	case CB_CHR_MACRO_STRING_ACTUAL:
		if (cb_hidden != 0) {
			(void)(*get_id)(infile);
			return FALSE;
		}
		tag = (int)cb_cpp_ref_string_macro_actual;
		break;
	case CB_CHR_POUND_LINE:
		cb_enter_pound_line_seen();
		return FALSE;
	case CB_CHR_INCLUDE_LOCAL:
		tag = (int)cb_cpp_included_file_name_local;
		break;
	case CB_CHR_INCLUDE_SYSTEM:
		tag = (int)cb_cpp_included_file_name_system;
		break;
	case CB_CHR_VERSION:
		{
			int	version = atoi((*get_id)(infile));

			if (version > CB_CPP_PROTOCOL_VERSION) {
				cb_fatal("cpp/ccom %s option version mismatch %d!=%d\n",
					 CB_OPTION,
					 version,
					 CB_CPP_PROTOCOL_VERSION);
			}
			if (version < CB_CPP_PROTOCOL_VERSION) {
				cb_message("Warning: cpp/ccom %s option version mismatch %d!=%d\n",
					      CB_OPTION,
					      version,
					      CB_CPP_PROTOCOL_VERSION);
			}
			return FALSE;
		}
	case CB_CHR_XIF_TRUE:
		return FALSE;
	case CB_CHR_XIF_FALSE:
		return FALSE;
	case CB_CHR_XIFDEF_TRUE:
		return FALSE;
	case CB_CHR_XIFDEF_FALSE:
		return FALSE;
	case CB_CHR_XELSEIF_TRUE:
		return FALSE;
	case CB_CHR_XELSEIF_FALSE:
		return FALSE;
	case CB_CHR_XELSEIFDEF_TRUE:
		return FALSE;
	case CB_CHR_XELSEIFDEF_FALSE:
		return FALSE;
	case CB_CHR_XVAR:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XENABLE:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XERROR:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XWARNING:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XINCLUDE:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XSLIBRARY:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XEXIT:
		return FALSE;
	case CB_CHR_XVAR_REF_UNDEF:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XVAR_REF_TRUE:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XVAR_REF_FALSE:
		tag = (int)cb_cpp_ident;
		break;
	case CB_CHR_XDEBUG_TRUE:
		return FALSE;
	case CB_CHR_XDEBUG_FALSE:
		return FALSE;
	case CB_CHR_XELSE_TRUE:
		return FALSE;
	case CB_CHR_XELSE_FALSE:
		return FALSE;
	default:
		cb_fatal("cpp sent unknown %s message %d (or ^A in source)\n",
			 CB_OPTION,
			 chr);
	}

	symbol = (*get_id)(infile);
	if (cb_enter_cb_file->header.case_was_folded) {
		register char	*symptr;

		switch (chr) {
		case CB_CHR_MACRO_DEF_W_FORMALS:
		case CB_CHR_MACRO_DEF_WO_FORMALS:
		case CB_CHR_MACRO_REF_W_FORMALS_DEFD:
		case CB_CHR_MACRO_REF_WO_FORMALS_DEFD:
		case CB_CHR_MACRO_REF_WO_FORMALS_UNDEFD:
		case CB_CHR_MACRO_FORMAL_DEF:
		case CB_CHR_MACRO_FORMAL_REF:
		case CB_CHR_MACRO_ACTUAL_REF:
		case CB_CHR_MACRO_NON_FORMAL_REF:
		case CB_CHR_UNDEF:
		case CB_CHR_XVAR:
		case CB_CHR_XENABLE:
		case CB_CHR_XVAR_REF_UNDEF:
		case CB_CHR_XVAR_REF_TRUE:
		case CB_CHR_XVAR_REF_FALSE:
			symptr = symbol;
			while ((chr = *symptr++) != '\0') {
				if (isupper(chr)) {
					symptr[-1] = tolower(chr);
				}
			}
			break;
		}
	}
	if (macro_save_name) {
		/*
		 * If we are going to enter a macro focus unit later
		 * we save the name of the macro
		 */
		macro_save_name = FALSE;
		macro_name = cb_string_unique(symbol);
		(void)cb_enter_symbol(macro_name,
				      *cb_lineno_addr,
				      (unsigned int)tag);
	} else {
		(void)cb_enter_symbol(symbol, *cb_lineno_addr,
				      (unsigned int)tag);
	}
	return FALSE;
}

/*
 *	cb_cpp_get_id(infile)
 *		This will get obtain and return identifier terminated by
 *		CB_CHR_END by repeated calls to "cb_cpp_get_chr(infile)".
 */
static char *
cb_cpp_get_id(infile)
	register FILE	*infile;
{
	char		*result;

	cbgetid(cb_cpp_get_chr, infile, result);
	return result;
}

