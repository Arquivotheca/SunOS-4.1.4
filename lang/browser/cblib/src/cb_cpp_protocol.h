/* LINTLIBRARY */

/*	@(#)cb_cpp_protocol.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_cpp_protocol_h_INCLUDED
#define cb_cpp_protocol_h_INCLUDED

#ifndef FILE
#include <stdio.h>
#endif

#ifndef cb_line_id_h_INCLUDED
#include "cb_line_id.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

/*
 * Special character definitions for communicating cpp information to
 * the compilers.
 */

#define	CB_CPP_PROTOCOL_VERSION		1

#define CB_CHR(chr)			chr
#define CB_CHR_PREFIX			CB_CHR('\001')
#define CB_CHR_END_ID			CB_CHR('\0')	/* Terminates string
							 * args of protocol*/
#define CB_CHR_IF			CB_CHR('A')	/* Parsing #if */
#define CB_CHR_IF_END_ACTIVE		CB_CHR('B')	/* Stop #if TRUE */
#define CB_CHR_IF_END_INACTIVE		CB_CHR('b')	/* Stop #if FALSE */
#define CB_CHR_IFDEF_DEFD		CB_CHR('C')	/* #ifdef <sym> */
#define CB_CHR_IFDEF_UNDEFD		CB_CHR('c')	/* #ifdef <sym> */
#define CB_CHR_IFNDEF_DEFD		CB_CHR('D')	/* #ifndef <sym> */
#define CB_CHR_IFNDEF_UNDEFD		CB_CHR('d')	/* #ifndef <sym> */
#define	CB_CHR_IDENT			CB_CHR('E')	/* #ident <sym> */
#define	CB_CHR_CONTROL_A		CB_CHR('F')	/* Control-A */
#define CB_CHR_LINE_ID			CB_CHR('G')	/* Line id block */
#define CB_CHR_MACRO_DEF_W_FORMALS	CB_CHR('H')	/* #define <sym>() */
#define CB_CHR_MACRO_DEF_WO_FORMALS	CB_CHR('I')	/* #define <sym> */
#define	CB_CHR_MACRO_DEF_END		CB_CHR('J')	/* End macro def */
#define CB_CHR_MACRO_REF_W_FORMALS_DEFD	CB_CHR('K')	/* <sym>() */
#define CB_CHR_MACRO_REF_WO_FORMALS_DEFD CB_CHR('L')	/* <sym> */
#define CB_CHR_MACRO_REF_WO_FORMALS_UNDEFD CB_CHR('l')	/* <sym> */
#define CB_CHR_MACRO_REF_END		CB_CHR('M')	/* Turn visible */
#define CB_CHR_MACRO_FORMAL_DEF		CB_CHR('N')	/* #define (<sym>) */
#define CB_CHR_MACRO_FORMAL_REF		CB_CHR('O')	/* <sym> */
#define CB_CHR_MACRO_ACTUAL_REF		CB_CHR('P')	/* <sym> */
#define CB_CHR_MACRO_NON_FORMAL_REF	CB_CHR('Q')	/* <sym> */
#define CB_CHR_UNDEF			CB_CHR('R')	/* #undef <sym> */
#define CB_CHR_START_INACTIVE		CB_CHR('S')	/* Start of inactive
							 * block from #if* */
#define CB_CHR_END_INACTIVE		CB_CHR('T')
#define CB_CHR_SAYLINE			CB_CHR('U')	/* Prefix for cpp */
							/* filename line */
#define CB_CHR_LINENO			CB_CHR('V')	/* Set linenumber */
#define CB_CHR_MACRO_STRING_IN_DEF	CB_CHR('W')
#define CB_CHR_MACRO_STRING_ACTUAL	CB_CHR('X')
#define CB_CHR_POUND_LINE		CB_CHR('Y')	/* #line in src */
#define CB_CHR_INCLUDE_LOCAL		CB_CHR('Z')	/* "filename" */
#define CB_CHR_INCLUDE_SYSTEM		CB_CHR('z')	/* <filename> */
#define	CB_CHR_VERSION			CB_CHR('v')	/* Version <num> */

/* Apollo(tm) extensions for Sun Pascal 2.0 */
#define	CB_CHR_XIF_TRUE			CB_CHR('!')	/* %if TRUE */
#define	CB_CHR_XIF_FALSE		CB_CHR('@')	/* %if FASLE */
#define	CB_CHR_XIFDEF_TRUE		CB_CHR('#')	/* %ifdef TRUE */
#define	CB_CHR_XIFDEF_FALSE		CB_CHR('$')	/* %ifdef FALSE */
#define	CB_CHR_XELSEIF_TRUE		CB_CHR('%')	/* %elseif TRUE */
#define	CB_CHR_XELSEIF_FALSE		CB_CHR('^')	/* %elseif FALSE */
#define	CB_CHR_XELSEIFDEF_TRUE		CB_CHR('&')	/* %elseifdef TRUE */
#define	CB_CHR_XELSEIFDEF_FALSE		CB_CHR('*')	/* %elseifdef FALSE*/
#define	CB_CHR_XVAR			CB_CHR('(')	/* %var <sym> */
#define	CB_CHR_XENABLE			CB_CHR(')')	/* %enable <sym> */
#define	CB_CHR_XERROR			CB_CHR('_')	/* %error <sym> */
#define	CB_CHR_XWARNING			CB_CHR('-')	/* %warning <sym> */
#define	CB_CHR_XINCLUDE			CB_CHR('+')	/* %include <sym> */
#define	CB_CHR_XSLIBRARY		CB_CHR('=')	/* %slibrary <sym> */
#define	CB_CHR_XEXIT			CB_CHR('{')	/* %exit */
#define	CB_CHR_XVAR_REF_UNDEF		CB_CHR('[')	/* ref <sym> undef */
#define	CB_CHR_XVAR_REF_TRUE		CB_CHR('}')	/* ref <sym> TRUE */
#define	CB_CHR_XVAR_REF_FALSE		CB_CHR(']')	/* ref <sym> FALSE */
#define	CB_CHR_XDEBUG_TRUE		CB_CHR(':')	/* %debug TRUE */
#define	CB_CHR_XDEBUG_FALSE		CB_CHR(';')	/* %debug FALSE */
#define	CB_CHR_XELSE_TRUE		CB_CHR('<')	/* %else TRUE */
#define	CB_CHR_XELSE_FALSE		CB_CHR(',')	/* %else FALSE */

#define CB_CHR_MAX_LINES		8192

#define cbputid(f, chr, id) { \
		(void)putc(CB_CHR_PREFIX, f); \
		(void)putc(chr, f); \
		fputs(id, f); \
		(void)putc(CB_CHR_END_ID, f); \
		}

#define cbgetid(getc, f, result) { \
		static char	*buffer = NULL; \
		static char	*buffer_end = NULL; \
		register char	*cp = buffer; \
		register char	*be = buffer_end; \
		register char	c; \
		PORT_EXTERN(char *malloc(PORT_1ARG(unsigned))); \
		while ( (c = getc(f) ) != CB_CHR_END_ID) { \
			if (cp >= be) { \
				unsigned length = (be-buffer+100)*2; \
				char	*new_buffer = malloc(length); \
				if (buffer != NULL) { \
					*cp = 0; \
					(void)strcpy(new_buffer, buffer); \
				} \
				cp = new_buffer + (be-buffer); \
				buffer = new_buffer; \
				be = buffer_end = buffer + length - 10; \
			} \
			*cp++ = c; \
		} \
		*cp = '\0'; \
		(result) = buffer; \
		}

#define cbputlineno(f, lineno) \
		    if (lineno < 0xffff) { \
			(void)putc(lineno, f); \
			(void)putc(lineno >> 8, f); \
		    } else { \
			(void)putc(0xff, f); \
			(void)putc(0xff, f); \
			(void)putc(lineno, f); \
			(void)putc(lineno >> 8, f); \
			(void)putc(lineno >> 16, f); \
			(void)putc(lineno >> 24, f); \
		    }

#define cbgetlineno(getc, f, lineno) { \
		    (lineno) = getc(f) & 0xff; \
		    (lineno) |= (getc(f) & 0xff) << 8; \
		    if ((lineno) == 0xffff) { \
			(lineno) = getc(f) & 0xff; \
			(lineno) |= (getc(f) & 0xff) << 8; \
			(lineno) |= (getc(f) & 0xff) << 16; \
			(lineno) |= (getc(f) & 0xff) << 24; \
		    } }


extern	int		cb_hidden;

typedef char		(*Cb_cpp_get_char_fn)(PORT_1ARG(FILE *));
typedef char		*(*Cb_cpp_get_id_fn)(PORT_1ARG(FILE *));
typedef void		(*Cb_cpp_get_block_fn)(PORT_3ARG(Cb_line_id,
							 int,
							 FILE *));

PORT_EXTERN(int		cb_cpp_protocol(PORT_5ARG(char,
						  Cb_cpp_get_char_fn,
						  Cb_cpp_get_id_fn,
						  Cb_cpp_get_block_fn,
						  FILE *)));

#endif
