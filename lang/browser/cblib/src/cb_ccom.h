/* LINTLIBRARY */

/*	@(#)cb_ccom.h 1.1 94/10/31 SMI	*/

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

#ifndef cb_ccom_h_INCLUDED
#define cb_ccom_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif

#define cb_enter_symbol_hidden(symbol, lineno, usage, hidden_usage) \
	cb_enter_symbol((symbol), \
			(lineno) > 0 ? (lineno):-(lineno), \
			(lineno) > 0 ? (usage):(hidden_usage))

#define cb_enter_name(symbol, lineno, usage) \
	{    if (cb_flag && ((lineno) > 0)) \
		(void)cb_enter_symbol( \
				  (symbol)->sname, \
				  (lineno), \
				  (usage)); }
#define cb_enter_name_mark(symbol, lineno, usage, mark) \
	{   mark = NULL; \
	    if (cb_flag && ((lineno) > 0)) \
		mark = cb_enter_symbol( \
				  (symbol)->sname, \
				  (lineno), \
				  (usage)); }

#define cb_enter_name_hidden(symbol, lineno, usage, hidden_usage) \
	{    if (cb_flag) \
		(void)cb_enter_symbol( \
				  (symbol)->sname, \
				  (lineno) > 0 ? (lineno):-(lineno), \
				  (lineno) > 0 ? (usage):(hidden_usage)); }
#define cb_enter_name_mark_hidden(symbol, lineno, usage, hidden_usage, mark) \
	{   mark = NULL; \
	    if (cb_flag) \
		mark = cb_enter_symbol( \
				  (symbol)->sname, \
				  (lineno) > 0 ? (lineno):-(lineno), \
				  (lineno) > 0 ? (usage):(hidden_usage)); }

#define cb_ccom_adjust_tag(rec, tag) \
	{ \
	cb_enter_get_cb_file()->hashs[(int)cb_semtab_section] -= \
		(unsigned int)(rec)->name_ref.usage; \
	(rec)->name_ref.usage = (unsigned int)(tag); \
	cb_enter_get_cb_file()->hashs[(int)cb_semtab_section] += \
	  (unsigned int)(tag); \
	}


typedef struct Cb_expr_tag	*Cb_expr, Cb_expr_rec;

extern	Array		cb_ccom_in_struct;
extern	Cb_semtab_if	cb_ccom_func;
extern	Cb_semtab_if	cb_ccom_maybe_write;
extern	Cb_semtab_if	cb_ccom_maybe_cast[3];
extern	char		*cb_ccom_string;
extern	int		cb_ccom_string_length;
extern	int		cb_ccom_line_no;
extern	Cb_expr		cb_ccom_expr;

enum Cb_expr_op_tag {
	cb_first_expr_op,

	cb_unary,
	cb_binary,
	cb_name,
	cb_name_vector,
	cb_name_point,
	cb_name_dot,

	cb_last_expr_op
};
typedef enum Cb_expr_op_tag	*Cb_expr_op_pointer, Cb_expr_op;

struct Cb_expr_tag {
	Cb_expr_op	op;
	Cb_expr		e1;
	Cb_expr		e2;
};

PORT_EXTERN(int		cb_ccom_cpp_protocol(PORT_0ARG));
PORT_EXTERN(void	cb_ccom_enter_string(PORT_3ARG(unsigned, int, char*)));
PORT_EXTERN(Cb_expr	cb_ccom_alloc(PORT_3ARG(Cb_expr_op,
						Cb_expr,
						Cb_expr)));
PORT_EXTERN(void	cb_ccom_dealloc(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_read2write(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_read2read_write(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_read2addrof(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_read2sizeof(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_cast2sizeof(PORT_1ARG(Cb_semtab_if)));
PORT_EXTERN(void	cb_ccom_decl2cast(PORT_1ARG(Cb_semtab_if)));
PORT_EXTERN(void	cb_ccom_read2vector_addrof(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_vector_name2name(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_name2other(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_wo_body2w_body(PORT_1ARG(Cb_semtab_if)));
PORT_EXTERN(void	cb_ccom_func2proc(PORT_1ARG(Cb_expr)));
PORT_EXTERN(void	cb_ccom_block_start(PORT_2ARG(char, int)));
PORT_EXTERN(void	cb_ccom_block_end(PORT_2ARG(int, int)));
PORT_EXTERN(void	cb_ccom_push_maybe_write(PORT_0ARG));
PORT_EXTERN(void	cb_ccom_pop_maybe_write(PORT_0ARG));
PORT_EXTERN(void	cb_ccom_regular_call(PORT_1ARG(char *)));
PORT_EXTERN(void	cb_ccom_var_call(PORT_1ARG(char *)));
PORT_EXTERN(void	cb_ccom_func_assignment(PORT_3ARG(char *,char *,int)));

#endif
