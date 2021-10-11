#ident "@(#)cb_ccom.c 1.1 94/10/31 SMI"

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

#ifndef cb_ccom_h_INCLUDED
#include "cb_ccom.h"
#endif

#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
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

#ifndef cb_sun_c_tags_h_INCLUDED
#include "cb_sun_c_tags.h"
#endif

#define cb_ccom_get_tag() \
	(Cb_sun_c_tags)((Cb_semtab_if)(expr->e1))->name_ref.usage
#define cb_ccom_set_tag(tag) \
	cb_ccom_adjust_tag(((Cb_semtab_if)(expr->e1)), (tag))


	Array		cb_ccom_in_struct = 0;
	Cb_semtab_if	cb_ccom_func = NULL;
	Cb_semtab_if	cb_ccom_maybe_write = NULL;
	Cb_semtab_if	cb_ccom_maybe_cast[3] = {NULL, NULL, NULL };
	char		*cb_ccom_string = NULL;
	int		cb_ccom_string_length = 0;
	int		cb_ccom_line_no;
	Cb_expr		cb_ccom_expr;

static	Cb_expr		cb_ccom_free_list[] = {NULL, NULL };
static	Array		cb_block_names = NULL;
static	Array		cb_block_starts = NULL;
static	Array		cb_maybe_write_stack = NULL;

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	int		cb_ccom_cpp_protocol();
extern	void		cb_ccom_enter_string();
extern	char		*cb_ccom_get_cpp_id();
extern	Cb_expr		cb_ccom_alloc();
extern	void		cb_ccom_dealloc();
extern	void		cb_ccom_read2write();
extern	void		cb_ccom_read2read_write();
extern	void		cb_ccom_read2addrof();
extern	void		cb_ccom_read2sizeof();
extern	void		cb_ccom_cast2sizeof();
extern	void		cb_ccom_decl2cast();
extern	void		cb_ccom_read2vector_addrof();
extern	void		cb_ccom_vector_name2name();
extern	void		cb_ccom_name2other();
extern	void		cb_ccom_wo_body2w_body();
extern	void		cb_ccom_func2proc();
extern	void		cb_ccom_block_start();
extern	void		cb_ccom_block_end();
extern	void		cb_ccom_push_maybe_write();
extern	void		cb_ccom_pop_maybe_write();
extern	void		cb_ccom_regular_call();
extern	void		cb_ccom_var_call();
extern	void		cb_ccom_func_assignment();

/*
 *	cb_ccom_cpp_protocol()
 *		This function is called from ccom to handle cpp
 *		protocol information.  TRUE is returned whenever
 *		a control-A was in the input stream.
 */
int
cb_ccom_cpp_protocol()
{
	char		chr;
extern	char		cb_ccom_get_char();

	chr = cb_ccom_get_char();
	switch (chr) {
	case CB_CHR_SAYLINE:
		lxtitle(TRUE);
		return FALSE;
	case CB_CHR_POUND_LINE:
		lxtitle(FALSE);
		/* Fall thru */
	default:
		return cb_cpp_protocol(chr,
				       cb_ccom_get_char,
				       cb_ccom_get_cpp_id,
				       cb_fread,
				       stdin);
	}
}

/*
 *	cb_ccom_enter_string(tag, lineno, str)
 *		Enter one string from ccom
 */
void
cb_ccom_enter_string(tag, lineno, str)
	Cb_sun_c_tags	tag;
	int		lineno;
	char		*str;
{
	char		*string;

	if (!cb_flag || (cb_hidden > 0)) {
		return;
	}
	string = (str == NULL) ? "\"\"" : str;
	(void)cb_enter_symbol(string, lineno, (unsigned int)tag);
}

/*
 *	cb_ccom_get_cpp_id()
 *		Read one identifier from the cpp byte stream
 */
static char *
cb_ccom_get_cpp_id()
{
	char	*result;

	cbgetid(getc, stdin, result);
	return result;
}

/*
 *	cb_ccom_alloc(op, e1, e2)
 *		Allocate one node for the cb parse tree
 */
Cb_expr
cb_ccom_alloc(op, e1, e2)
	Cb_expr_op	op;
	Cb_expr		e1;
	Cb_expr		e2;
{
	register int		nodes;
	register int		size;
	register Cb_expr	expr;
	static	Cb_heap		heap = NULL;

	if (!cb_flag) {
		return NULL;
	}

	nodes = op == cb_binary ? 2 : 1;
	if ((expr = cb_ccom_free_list[nodes-1]) != NULL) {
		cb_ccom_free_list[nodes-1] = *((Cb_expr *)expr);
	} else {
		size =  sizeof(Cb_expr_op) + nodes * sizeof(Cb_expr_rec);
		if (heap == NULL) {
			heap = cb_init_heap(10000);
		}
		expr = (Cb_expr)cb_get_memory(size, heap);
	}
	expr->op = op;

	switch (nodes) {
		case 2:
			expr->e2 = e2;
		case 1:
			expr->e1 = e1;
	}
	return expr;
}

/*
 *	cb_ccom_dealloc(expr)
 */
void
cb_ccom_dealloc(expr)
	Cb_expr		expr;
{
	if (!cb_flag || (expr == NULL)) {
		return;
	}
	switch (expr->op) {
	case cb_name:
	case cb_name_vector:
	case cb_name_point:
	case cb_name_dot:
		*((Cb_expr *)expr) = cb_ccom_free_list[0];
		cb_ccom_free_list[0] = expr;
		break;

	case cb_binary:
		cb_ccom_dealloc(expr->e1);
		cb_ccom_dealloc(expr->e2);
		*((Cb_expr *)expr) = cb_ccom_free_list[1];
		cb_ccom_free_list[1] = expr;
		break;

	case cb_unary:
		cb_ccom_dealloc(expr->e1);
		*((Cb_expr *)expr) = cb_ccom_free_list[0];
		cb_ccom_free_list[0] = expr;
		break;
	}
}

/*
 *	cb_ccom_read2write(expr)
 *		Set write bit for names of expression.
 */
void
cb_ccom_read2write(expr)
	Cb_expr		expr;
{
	register Cb_sun_c_tags tag;

	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name_point:
		break;
	case cb_name:
	case cb_name_vector:
	case cb_name_dot:
		tag = cb_ccom_get_tag();
		switch (tag) {
		case cb_c_def_enum_member:
			tag = cb_c_def_enum_member_init;
			break;
		case cb_c_def_enum_member_hidden:
			tag = cb_c_def_enum_member_init_hidden;
			break;
		case cb_c_ref_struct_field_read:
			tag = cb_c_ref_struct_field_write;
			break;
		case cb_c_ref_struct_bitfield_read:
			tag = cb_c_ref_struct_bitfield_write;
			break;
		case cb_c_ref_union_field_read:
			tag = cb_c_ref_union_field_write;
			break;
		case cb_c_def_var_global:
			tag = cb_c_def_var_global_init;
			break;
		case cb_c_def_var_global_hidden:
			tag = cb_c_def_var_global_init_hidden;
			break;
		case cb_c_def_var_toplevel_static:
			tag = cb_c_def_var_toplevel_static_init;
			break;
		case cb_c_def_var_toplevel_static_hidden:
			tag = cb_c_def_var_toplevel_static_init_hidden;
			break;
		case cb_c_def_var_local:
			tag = cb_c_def_var_local_init;
			break;
		case cb_c_def_var_local_hidden:
			tag = cb_c_def_var_local_init_hidden;
			break;
		case cb_c_def_var_local_register:
			tag = cb_c_def_var_local_register_init;
			break;
		case cb_c_def_var_local_register_hidden:
			tag = cb_c_def_var_local_register_init_hidden;
			break;
		case cb_c_def_var_local_static:
			tag = cb_c_def_var_local_static_init;
			break;
		case cb_c_def_var_local_static_hidden:
			tag = cb_c_def_var_local_static_init_hidden;
			break;
		case cb_c_ref_var_read:
			tag = cb_c_ref_var_write;
			break;
		}
		cb_ccom_set_tag(tag);
		break;
	case cb_binary:
		cb_ccom_read2write(expr->e2);
	case cb_unary:
		cb_ccom_read2write(expr->e1);
		break;

	default:
		cb_abort("Illegal op for cb_ccom_read2write() = %d\n", expr->op);
	}
	cb_ccom_dealloc(expr);
}

/*
 *	cb_ccom_read2read_write(expr)
 *		Set write bit for names of expression.
 */
void
cb_ccom_read2read_write(expr)
	Cb_expr		expr;

{
	register Cb_sun_c_tags tag;

	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name_point:
		break;
	case cb_name:
	case cb_name_vector:
	case cb_name_dot:
		tag = cb_ccom_get_tag();
		switch (tag) {
		case cb_c_ref_struct_field_read:
			tag = cb_c_ref_struct_field_read_write;
			break;
		case cb_c_ref_struct_bitfield_read:
			tag = cb_c_ref_struct_bitfield_read_write;
			break;
		case cb_c_ref_union_field_read:
			tag = cb_c_ref_union_field_read_write;
			break;
		case cb_c_ref_var_read:
			tag = cb_c_ref_var_read_write;
			break;
		}
		cb_ccom_set_tag(tag);
		break;
	case cb_binary:
		cb_ccom_read2read_write(expr->e2);
	case cb_unary:
		cb_ccom_read2read_write(expr->e1);
		break;

	default:
		cb_abort("Illegal op for cb_ccom_read2read_write() = %d\n", expr->op);
	}
	cb_ccom_dealloc(expr);
}

/*
 *	cb_ccom_read2addrof(expr)
 *		Set write bit for names of expression.
 */
void
cb_ccom_read2addrof(expr)
	Cb_expr		expr;
{
	register Cb_sun_c_tags tag;

	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name_point:
		break;
	case cb_name:
	case cb_name_vector:
	case cb_name_dot:
		tag = cb_ccom_get_tag();
		switch (tag) {
		case cb_c_ref_struct_field_read:
			tag = cb_c_ref_struct_field_addrof;
			break;
		case cb_c_ref_union_field_read:
			tag = cb_c_ref_union_field_addrof;
			break;
		case cb_c_ref_var_read:
			tag = cb_c_ref_var_addrof;
			break;
		}
		cb_ccom_set_tag(tag);
		break;
	case cb_binary:
		cb_ccom_read2addrof(expr->e2);
	case cb_unary:
		cb_ccom_read2addrof(expr->e1);
		break;

	default:
		cb_abort("Illegal op for cb_ccom_read2addrof() = %d\n", expr->op);
	}
}

/*
 *	cb_ccom_read2sizeof(expr)
 *		Set write bit for names of expression.
 */
void
cb_ccom_read2sizeof(expr)
	Cb_expr		expr;
{
	register Cb_sun_c_tags tag;

	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name:
	case cb_name_vector:
	case cb_name_point:
	case cb_name_dot:
		tag = cb_ccom_get_tag();
		switch (tag) {
		case cb_c_ref_enum_member:
			tag = cb_c_ref_enum_member_sizeof;
			break;
		case cb_c_ref_struct_field_read:
			tag = cb_c_ref_struct_field_sizeof;
			break;
		case cb_c_ref_struct_bitfield_read:
			tag = cb_c_ref_struct_bitfield_sizeof;
			break;
		case cb_c_ref_union_field_read:
			tag = cb_c_ref_union_field_sizeof;
			break;
		case cb_c_ref_var_read:
			tag = cb_c_ref_var_sizeof;
			break;

		case cb_c_ref_struct_field_addrof:
			tag = cb_c_ref_struct_field_sizeof;
			break;
		case cb_c_ref_union_field_addrof:
			tag = cb_c_ref_union_field_sizeof;
			break;
		case cb_c_ref_var_addrof:
			tag = cb_c_ref_var_sizeof;
			break;
		}
		cb_ccom_set_tag(tag);
		break;
	case cb_binary:
		cb_ccom_read2sizeof(expr->e2);
	case cb_unary:
		cb_ccom_read2sizeof(expr->e1);
		break;

	default:
		cb_abort("Illegal op for cb_ccom_read2sizeof() = %d\n", expr->op);
	}
	cb_ccom_dealloc(expr);
}

/*
/*
 *	cb_ccom_cast2sizeof(semrec)
 */
void
cb_ccom_cast2sizeof(semrec)
	Cb_semtab_if	semrec;
{
	Cb_sun_c_tags	tag = cb_sun_c_bogus_entry;

	if (!cb_flag || (semrec == NULL)) {
		return;
	}

	switch (tag = (Cb_sun_c_tags)semrec->name_ref.usage) {
	case cb_c_ref_builtin_type_cast:
	case cb_c_ref_builtin_type_decl:
		tag = cb_c_ref_builtin_type_sizeof;
		break;
	case cb_c_ref_typedef_cast:
	case cb_c_ref_typedef_decl:
		tag = cb_c_ref_typedef_sizeof;
		break;
	case cb_c_ref_struct_tag_cast:
	case cb_c_ref_struct_tag_decl:
		tag = cb_c_ref_struct_tag_sizeof;
		break;
	case cb_c_ref_union_tag_cast:
	case cb_c_ref_union_tag_decl:
		tag = cb_c_ref_union_tag_sizeof;
		break;
	case cb_c_ref_enum_tag_cast:
	case cb_c_ref_enum_tag_decl:
		tag = cb_c_ref_enum_tag_sizeof;
		break;
	case cb_c_ref_typedef_decl_hidden:
	case cb_c_ref_struct_tag_decl_hidden:
	case cb_c_ref_union_tag_decl_hidden:
	case cb_c_ref_enum_tag_decl_hidden:
	case cb_sun_c_bogus_entry:
		tag = cb_sun_c_bogus_entry;
		break;
	}
	cb_ccom_adjust_tag(semrec, tag);
}

/*
 *	cb_ccom_decl2cast(semrec)
 */
void
cb_ccom_decl2cast(semrec)
	Cb_semtab_if	semrec;
{
	Cb_sun_c_tags	tag;

	if (!cb_flag || (semrec == NULL)) {
		return;
	}

	switch (tag = (Cb_sun_c_tags)semrec->name_ref.usage) {
	case cb_c_ref_typedef_decl:
	case cb_c_ref_typedef_sizeof:
		tag = cb_c_ref_typedef_cast;
		break;
	case cb_c_ref_builtin_type_decl:
	case cb_c_ref_builtin_type_sizeof:
		tag = cb_c_ref_builtin_type_cast;
		break;
	case cb_c_ref_struct_tag_decl:
	case cb_c_ref_struct_tag_sizeof:
		tag = cb_c_ref_struct_tag_cast;
		break;
	case cb_c_ref_union_tag_decl:
	case cb_c_ref_union_tag_sizeof:
		tag = cb_c_ref_union_tag_cast;
		break;
	case cb_c_ref_enum_tag_decl:
	case cb_c_ref_enum_tag_sizeof:
		tag = cb_c_ref_enum_tag_cast;
		break;
	case cb_c_ref_typedef_decl_hidden:
	case cb_c_ref_builtin_type_decl_hidden:
	case cb_c_ref_struct_tag_decl_hidden:
	case cb_c_ref_union_tag_decl_hidden:
	case cb_c_ref_enum_tag_decl_hidden:
	case cb_sun_c_bogus_entry:
		tag = cb_sun_c_bogus_entry;
		break;
	}
	cb_ccom_adjust_tag(semrec, tag);
}

/*
 *	cb_ccom_read2vector_addrof(expr)
 *		Set write bit for names of expression.
 */
void
cb_ccom_read2vector_addrof(expr)
	Cb_expr		expr;
{
	register Cb_sun_c_tags tag;

	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name:
	case cb_name_point:
	case cb_name_dot:
		break;
	case cb_name_vector:
		tag = cb_ccom_get_tag();
		switch (tag) {
		case cb_c_ref_struct_field_read:
			tag = cb_c_ref_struct_field_addrof;
			break;
		case cb_c_ref_union_field_read:
			tag = cb_c_ref_union_field_addrof;
			break;
		case cb_c_ref_var_read:
			tag = cb_c_ref_var_addrof;
			break;
		}
		cb_ccom_set_tag(tag);
		break;
	case cb_binary:
		cb_ccom_read2vector_addrof(expr->e2);
	case cb_unary:
		cb_ccom_read2vector_addrof(expr->e1);
		break;

	default:
		cb_abort("Illegal op for cb_ccom_read2vector_addrof() = %d\n", expr->op);
	}
}

/*
 *	cb_ccom_vector_name2name(expr)
 *		Set write bit for names of expression.
 */
void
cb_ccom_vector_name2name(expr)
	Cb_expr		expr;
{
	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name:
	case cb_name_point:
	case cb_name_dot:
		break;

	case cb_name_vector:
		expr->op = cb_name;
		break;

	case cb_binary:
		cb_ccom_vector_name2name(expr->e2);

	case cb_unary:
		cb_ccom_vector_name2name(expr->e1);
		break;

	default:
		cb_abort("Illegal op for cb_ccom_vector_name2name() = %d\n", expr->op);
	}
}

/*
 *	cb_ccom_name2other(expr, other)
 *		Set write bit for names of expression.
 */
void
cb_ccom_name2other(expr, other)
	Cb_expr		expr;
	Cb_expr_op	other;
{
	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name_point:
		break;

	case cb_name_dot:
	case cb_name_vector:
	case cb_name:
		expr->op = other;
		break;

	case cb_binary:
		cb_ccom_name2other(expr->e2, other);

	case cb_unary:
		cb_ccom_name2other(expr->e1, other);
		break;

	default:
		cb_abort("Illegal op for cb_ccom_name2other() = %d\n", expr->op);
	}
}

/*
 *	cb_ccom_wo_body2w_body(semrec)
 */
void
cb_ccom_wo_body2w_body(semrec)
	Cb_semtab_if	semrec;
{
	Cb_sun_c_tags	tag;

	if (!cb_flag || (semrec == NULL)) {
		return;
	}

	switch (tag = (Cb_sun_c_tags)semrec->name_ref.usage) {
	case cb_c_def_global_func_wo_body:
		tag = cb_c_def_global_func_w_body;
		break;
	case cb_c_def_global_func_wo_body_hidden:
		tag = cb_c_def_global_func_w_body_hidden;
		break;
	case cb_c_def_toplevel_static_func_wo_body:
		tag = cb_c_def_toplevel_static_func_w_body;
		break;
	case cb_c_def_toplevel_static_func_wo_body_hidden:
		tag = cb_c_def_toplevel_static_func_w_body_hidden;
		break;
	case cb_sun_c_def_global_fortran_func_wo_body:
		tag = cb_sun_c_def_global_fortran_func_w_body;
		break;
	case cb_sun_c_def_global_fortran_func_wo_body_hidden:
		tag = cb_sun_c_def_global_fortran_func_w_body_hidden;
		break;
	case cb_c_def_toplevel_extern_func_wo_body:
		tag = cb_c_def_global_extern_func_w_body;
		break;
	case cb_c_def_toplevel_extern_func_wo_body_hidden:
		tag = cb_c_def_global_extern_func_w_body_hidden;
		break;
	}

	cb_ccom_adjust_tag(cb_ccom_func, tag);
}

/*
 *	cb_ccom_func2proc(expr)
 *		Set write bit for names of expression.
 */
void
cb_ccom_func2proc(expr)
	Cb_expr		expr;
{
	register Cb_sun_c_tags tag;

	if (!cb_flag || (expr == NULL)) {
		return;
	}

	switch (expr->op) {
	case cb_name:
		tag = cb_ccom_get_tag();
		switch (tag) {
		case cb_c_def_global_func_implicit:
			tag = cb_c_def_global_proc_implicit;
			break;
		case cb_c_def_global_func_implicit_hidden:
			tag = cb_c_def_global_proc_implicit_hidden;
			break;
		case cb_c_ref_func_call_using_var:
			tag = cb_c_ref_proc_call_using_var;
			break;
		case cb_c_ref_func_call_using_var_hidden:
			tag = cb_c_ref_proc_call_using_var_hidden;
			break;
		case cb_c_ref_func_call:
			tag = cb_c_ref_proc_call;
			break;
		case cb_c_ref_func_call_hidden:
			tag = cb_c_ref_proc_call_hidden;
			break;
		}
		cb_ccom_set_tag(tag);
		break;
	case cb_name_point:
	case cb_name_dot:
	case cb_name_vector:
	case cb_binary:
	case cb_unary:
		break;

	default:
		cb_abort("Illegal op for cb_ccom_func2proc() = %d\n", expr->op);
	}
	cb_ccom_dealloc(expr);
}

/*
 *	cb_ccom_block_start(type, linenumber)
 */
void
cb_ccom_block_start(type, linenumber)
	char		type;
	int		linenumber;
{
	if (!cb_flag) {
		return;
	}
	if (cb_block_names == NULL) {
		cb_block_names = array_create((Cb_heap)NULL);
		cb_block_starts = array_create((Cb_heap)NULL);
	}

	array_append(cb_block_names, (Array_value)type);
	array_append(cb_block_starts, (Array_value)linenumber);
}

/*
 *	cb_ccom_block_end(linenumber)
 */
void
cb_ccom_block_end(linenumber, use_this_block)
	int		linenumber;
{
	char		type;
	int		start;
	char		symbol[MAXPATHLEN];
	char		fn[2];
	char		*format;
	unsigned int	tag;

	if (!cb_flag) {
		return;
	}
	type = (char)array_pop(cb_block_names);
	start = array_pop(cb_block_starts);
	if (!use_this_block) {
		return;
	}
	switch (type) {
	case CB_EX_FOCUS_UNIT_BLOCK_KEY:
		format = CB_EX_FOCUS_UNIT_BLOCK_FORMAT;
		tag = (unsigned int)cb_focus_block_unit;
		break;
	case CB_EX_FOCUS_UNIT_ENUM_KEY:
		format = CB_EX_FOCUS_UNIT_ENUM_FORMAT;
		tag = (unsigned int)cb_focus_enum_unit;
		break;
	case CB_EX_FOCUS_UNIT_STRUCT_KEY:
		format = CB_EX_FOCUS_UNIT_STRUCT_FORMAT;
		tag = (unsigned int)cb_focus_struct_unit;
		break;
	case CB_EX_FOCUS_UNIT_UNION_KEY:
		format = CB_EX_FOCUS_UNIT_UNION_FORMAT;
		tag = (unsigned int)cb_focus_union_unit;
		break;
	default:
		cb_abort("Illegal focus unit tag `%c'\n", type);
	}
	fn[0] = type;
	fn[1] = 0;
	(void)sprintf(symbol, format, fn, start, linenumber);
	(void)cb_enter_symbol(symbol, start, tag);
}

/*
 *	cb_ccom_push_maybe_write()
 */
void
cb_ccom_push_maybe_write()
{
	if (cb_maybe_write_stack == NULL) {
		cb_maybe_write_stack = array_create((Cb_heap)NULL);
	}
	array_append(cb_maybe_write_stack, (Array_value)cb_ccom_maybe_write);
	cb_ccom_maybe_write = NULL;
}

/*
 *	cb_ccom_pop_maybe_write()
 */
void
cb_ccom_pop_maybe_write()
{
	if ((cb_maybe_write_stack == NULL) ||
	    (array_size(cb_maybe_write_stack) == 0)) {
		cb_abort("maybe_write stack underflow\n");
	}
	cb_ccom_maybe_write =
	  (Cb_semtab_if)array_delete(cb_maybe_write_stack,
				     array_size(cb_maybe_write_stack) - 1);
}

/*
 *	cb_ccom_regular_call(callee, lineno)
 */
void
cb_ccom_regular_call(callee, lineno)
	char		*callee;
	int		lineno;
{
	char		name[MAXPATHLEN*2];

	if (!cb_flag ||
	    (cb_enter_function_names == NULL) ||
	    (array_size(cb_enter_function_names) == 0)) {
		return;
	}

	if (lineno < 0) {
		lineno = -lineno;
	}

	(void)sprintf(name,
		      CB_EX_GRAPHER_ARC_PATTERN2,
		      CB_EX_GRAPHER_FUNCTION_CALLS_TAG,
		      array_fetch(cb_enter_function_names,
				  array_size(cb_enter_function_names)-1),
		      callee);
	(void)cb_enter_symbol(name,
			      lineno,
			      (unsigned)cb_grapher_function_call_arc);
}

/*
 *	cb_ccom_var_call(callee, lineno)
 */
void
cb_ccom_var_call(callee, lineno)
	char		*callee;
	int		lineno;
{
	char		name1[MAXPATHLEN*2];
	char		name2[MAXPATHLEN];

	if (!cb_flag ||
	    (cb_enter_function_names == NULL) ||
	    (array_size(cb_enter_function_names) == 0)) {
		return;
	}

	if (lineno < 0) {
		lineno = -lineno;
	}

	(void)sprintf(name2, "(*%s)()", callee);
	(void)sprintf(name1,
		      CB_EX_GRAPHER_ARC_PATTERN2,
		      CB_EX_GRAPHER_FUNCTION_CALLS_TAG,
		      array_fetch(cb_enter_function_names,
				  array_size(cb_enter_function_names)-1),
		      name2);
	(void)cb_enter_symbol(name1,
			      lineno,
			      (unsigned)cb_grapher_function_call_arc);
	(void)sprintf(name1, CB_EX_GRAPHER_NODE_PATTERN, name2);
	(void)cb_enter_symbol(name1,
			      lineno,
			      (unsigned)cb_grapher_function_variable_call_node);
}

/*
 *	cb_ccom_func_assignment(callee, caller, lineno)
 */
void
cb_ccom_func_assignment(callee, caller, lineno)
	char		*callee;
	char		*caller;
	int		lineno;
{
	char		name1[MAXPATHLEN*2];
	char		name2[MAXPATHLEN];

	if ((callee == NULL)|| (caller == NULL)) {
		return;
	}

	if (lineno < 0) {
		lineno = -lineno;
	}

	(void)sprintf(name2, "%s=", callee);
	(void)sprintf(name1,
		      CB_EX_GRAPHER_ARC_PATTERN2,
		      CB_EX_GRAPHER_FUNCTION_CALLS_TAG,
		      name2,
		      caller);
	(void)cb_enter_symbol(name1,
			      lineno,
			      (unsigned)cb_grapher_function_call_arc);
	(void)sprintf(name1, CB_EX_GRAPHER_NODE_PATTERN, name2);
	(void)cb_enter_symbol(name1,
			      lineno,
			      (unsigned)cb_grapher_function_variable_call_node);
}

