/* LINTLIBRARY */

/*	@(#)cb_io.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_io_h_INCLUDED
#define cb_io_h_INCLUDED

#ifndef FILE
#include <stdio.h>
#endif

#ifndef _FILE_
#include <sys/file.h>
#endif

#ifndef _PARAM_
#include <sys/param.h>
#endif

#ifndef __STAT_HEADER__
#include <sys/stat.h>
#endif

#ifndef _TIME_
#include <sys/time.h>
#endif

#ifndef _TYPES_
#include <sys/types.h>
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

PORT_EXTERN(void	cb_delete(PORT_1ARG(char*)));
PORT_EXTERN(int		cb_delete_on_exit(PORT_1ARG(char*)));
PORT_EXTERN(FILE	*cb_fopen(PORT_4ARG(char*, char*, char*, int)));
PORT_EXTERN(void	cb_fflush(PORT_1ARG(FILE *)));
PORT_EXTERN(void	cb_fclose(PORT_1ARG(FILE *)));
PORT_EXTERN(void	cb_fseek(PORT_2ARG(FILE *, int)));
PORT_EXTERN(void	cb_fread(PORT_3ARG(char*, int, FILE *)));
PORT_EXTERN(void	cb_fwrite(PORT_3ARG(char*, int, FILE *)));
PORT_EXTERN(int		cb_getc(PORT_1ARG(FILE *)));
PORT_EXTERN(void	cb_gettimeofday(PORT_2ARG(struct timeval*,
						  struct timezone*)));
PORT_EXTERN(int		cb_open(PORT_2ARG(char*, int)));
PORT_EXTERN(void	cb_fstat(PORT_2ARG(int, struct stat*)));
PORT_EXTERN(void	cb_stat(PORT_2ARG(int, struct stat*)));
PORT_EXTERN(void	cb_close(PORT_1ARG(int)));
PORT_EXTERN(void	cb_write(PORT_3ARG(int, char*, int)));
PORT_EXTERN(FILE	*cb_popen(PORT_2ARG(char*, char*)));
PORT_EXTERN(void	cb_pclose(PORT_1ARG(FILE *)));
PORT_EXTERN(char	*cb_fgets(PORT_3ARG(char*, int, FILE *)));
PORT_EXTERN(void	cb_fputc(PORT_2ARG(int, FILE *)));
PORT_EXTERN(void	cb_readlink(PORT_3ARG(char*, char*, int)));
PORT_EXTERN(void	cb_signal(PORT_2ARG(int, void (*)())));
PORT_EXTERN(void	cb_ungetc(PORT_2ARG(int, FILE *)));
PORT_EXTERN(char	*cb_mmap_file_private(PORT_1ARG(int)));
#endif
