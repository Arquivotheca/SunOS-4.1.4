/*	@(#)fiodefs.h 1.1 94/10/31 SMI; from UCB 1.3 */

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *
 * fortran file i/o type definitions
 */

#include <stdio.h>
#include "f_errno.h"

extern	int	MXUNIT;	/* variable-size unit tables under 4.3 */
#define GLITCH '\2'	/* special quote for Stu, generated in f77pass1 */

#define LISTDIRECTED  -1
#define FORMATTED      1

#define ERROR	1
#define OK	0
#define YES	1
#define NO	0

#define STDERR	0
#define STDIN	5
#define STDOUT	6

#define WRITE	1
#define READ	2
#define SEQ	3
#define DIR	4
#define FMT	5
#define UNF	6
#define EXT	7
#define INT	8

typedef char ioflag;
typedef long ftnint;
typedef ftnint flag;
typedef long ftnlen;

typedef struct		/*external read, write*/
{	flag cierr;
	ftnint ciunit;
	flag ciend;
	char *cifmt;
	ftnint cirec;
} cilist;

typedef struct		/*internal read, write*/
{	flag icierr;
	char *iciunit;
	flag iciend;
	char *icifmt;
	ftnint icirlen;
	ftnint icirnum;
	ftnint icirec;
} icilist;

typedef struct		/*open*/
{	flag oerr;	/* err =     */
	ftnint ounit;	/* unit =    */
	char *ofnm;	/* file =    */
	ftnlen ofnmlen;
	char *osta;	/* status =  */
	char *oacc;	/* access =  */
	char *ofm;	/* form =    */
	ftnint orl;	/* recl =    */
	char *oblnk;	/* blank =   */
#ifdef FILEOPT
	char  *ofopt;	/* fileopt = */
	ftnint ofoptlen;
#endif FILEOPT
} olist;

typedef struct		/*close*/
{	flag cerr;
	ftnint cunit;
	char *csta;
} cllist;

typedef struct		/*rewind, backspace, endfile*/
{	flag aerr;
	ftnint aunit;
} alist;

typedef struct		/* unique file identifier = devno + inode */
{	short udev;	/* type is a dev_t */
	long uinode;	/* type is a ino_t */
} ufid;
typedef struct		/*units*/
{	FILE *ufd;	/*0=unconnected*/
	char *ufnm;
	ufid uid;
	int url;	/*0=sequential*/
	int	unitno;	/* Fortran unit number */
	flag useek;	/*true=can backspace, use dir, ...*/
	flag ufmt;
	flag upad;	/* true=pad input records infinitely with blanks (formatted only */
	flag uprnt;
	flag ublnk;
	flag uend;
	flag uwrt;	/*last io was write*/
	flag uscrtch;
	flag utape;	/*true=appears to be a magtape drive */
	flag ueof;	/* opened at eof */
} unit;

typedef struct		/* inquire */
{	flag     inerr;		/* err=		*/
	ftnint   inunit;	/* unit=	*/
	char    *infile;	/* file=	*/
	ftnlen   infilen;
	ftnint	*inex;		/* exist=	*/
	ftnint	*inopen;	/* opened=	*/
	ftnint	*innum;		/* number=	*/
	ftnint	*innamed;	/* named=	*/
	char	*inname;	/* name=	*/
	ftnlen	 innamlen;
	char	*inacc;		/* access=	*/
	ftnlen	 inacclen;
	char	*inseq;		/* sequential=	*/
	ftnlen	 inseqlen;
	char 	*indir;		/* direct=	*/
	ftnlen	 indirlen;
	char	*inform;	/* form=	*/
	ftnlen	 informlen;
	char	*infmt;		/* formatted=	*/
	ftnint	 infmtlen;
	char	*inunf;		/* unformatted=	*/
	ftnlen	 inunflen;
	ftnint	*inrecl;	/* recl=	*/
	ftnint	*innrec;	/* nextrec=	*/
	char	*inblank;	/* blank=	*/
	ftnlen	 inblanklen;
#ifdef FILEOPT
	char    *infopt;	/* fileopt = */
	ftnint   infoptlen;
#endif FILEOPT
} inlist;

struct ioiflg {
	short if_oeof;
	short if_ctrl;
	short if_bzro;
};
#define	opneof	ioiflg_.if_oeof
#define	ccntrl	ioiflg_.if_ctrl
#define	blzero	ioiflg_.if_bzro

unit	*newunit();
unit	*mapunit();
unit	*findunit();

