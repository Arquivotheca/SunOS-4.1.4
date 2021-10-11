#ifndef lint
static char sccsid[] = "@(#)pr_texpat.c 1.1 94/10/31";
#endif

/* 
 * Copyright 1986 by Sun Microsystems, Inc.
 */

 
/*
 * Default patterns for pr_line.  Patterns used by CGI.
 */
 
short pr_tex_dotted[] = {1,5,1,5,1,5,1,5,0};
short pr_tex_dashed[] = {7,7,7,7,0};
short pr_tex_dashdot[] = {7,3,1,3,7,3,1,3,0};
short pr_tex_dashdotdotted[] = {9,3,1,3,1,3,1,3,0};
short pr_tex_longdashed[] = {13,7,0};
	
