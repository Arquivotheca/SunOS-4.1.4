/*	@(#)read_ir.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __read_ir__
#define __read_ir__

extern STRING_BUFF *string_buff;
extern STRING_BUFF *first_string_buff;
extern BLOCK	*entry_tab;
extern BLOCK	*entry_top;

extern BOOLEAN	read_ir(/*irfd*/);

#endif
