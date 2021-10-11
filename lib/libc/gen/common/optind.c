#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)optind.c 1.1 94/10/31 SMI"; 
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*LINTLIBRARY*/
int	optind = 1;
int	opterr = 1;
char	*optarg;
