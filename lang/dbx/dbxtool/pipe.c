#ifndef lint
static	char sccsid[] = "@(#)pipe.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/wait.h>
#include <setjmp.h>

#include "defs.h"
#include "pipeout.h"

#include "typedefs.h"
#include "dbxlib.h"
#include "dbxtool.h"
#include "cmd.h"
#include "pipe.h"
#include "src.h"

#define PARENT_READ	0
#define PARENT_WRITE	1
#define CHILD_READ	0
#define CHILD_WRITE	1
#define SLOTS		2		/* # of arguments added */
#define DEBUGGER	"dbx"
#define STRBUF		256

typedef struct	Use	*Use;
struct	Use	{
	char	*dir;
	Use	next;
};

/*
 * There are three types of files and line numbers:
 *	1) current (cur) - this is the name and line number of
 *			   the file currently being displayed in
 *			   the source window.
 *	2) breakpoint (brk) - this is the name and line number of 
 *			   the current stopping point.  If there is
 *			   is source code available, a solid arrow
 *			   will be displayed at 'brkline' in 'brkfile'.
 *      3) hollow - this is the name and line number of the hollow arrow.
 *			   If the hollow and breakpoint files and line
 *			   numbers refer to different locations, then
 *			   a hollow arrow will be displayed at 'hollowline'
 *			   of 'hollowfile'.
 */
private	Boolean	initdone;		/* Have we seen I_INITDONE? */
private Boolean	isstopped;		/* Have we hit a break point? */
private Boolean	istracing;		/* Are we tracing */
private	int	pipein;			/* Pipe from debugger */
private	char	curfile[STRBUF];	/* Current source file, simple name */
private	char	fullcurfile[STRBUF];	/* Current source file, full name */
private	char	curfunc[STRBUF];	/* Current function */
private int	curline;		/* Current source line */
private	char	brkfile[STRBUF];	/* Stopped point file, simple name */
private	char	fullbrkfile[STRBUF];	/* Stopped point file, full name */
private	char	brkfunc[STRBUF];	/* Function of stopped point */
private int	brkline;		/* Line number of stopped point */
private char	hollowfile[STRBUF];	/* Hollow source file, simple name */
private char	fullhollowfile[STRBUF];	/* Hollow source file, full name */
private int	hollowline;		/* Line number of the hollow arrow */
private	Use	sourcepath;		/* List of 'use' directories */
private jmp_buf env;			/* Setjmp buffer for error recovery */

private	Intermed rdtype();
private	Notify_value	wait3_handler();

private	char	*inter_type[] = {
	"I_BADTYPE",
	"I_INITDONE",
	"I_STOPPED",
	"I_STRING",
	"I_INT",
	"I_QUIT",
	"I_PRINTLINES",
	"I_BRKSET",
	"I_BRKDEL",
	"I_RESUME",
	"I_USE",
	"I_REINIT",
	"I_KILL",
	"I_CHDIR",
	"I_EMPHASIZE",
	"I_TRACE",
	"I_DISPLAY",
	"I_VERSION",
	"I_TOOLENV",
	"I_WIDTH",
	"I_SRCLINES",
	"I_CMDLINES",
	"I_DISPLINES",
	"I_FONT",
	"I_TOPMARGIN",
	"I_BOTMARGIN",
	"I_UNBUTTON",
	"I_BUTTON",
	"I_CALLSTACK",
	"I_NEWINITDONE",
	"I_MENU",
	"I_UNMENU",
};

/*
 * The versions specify the version of dbx that this dbxtool
 * is compatible with.  When the interface between dbx and
 * dbxtool is extended in an upwards compatible way, only
 * the high version need be adjusted.  If the interface changes
 * in a non-compatible way the low version should be set to
 * the new incompatible version.
 */
#define DBXTOOL_LOWVERSION	2

private low_version = DBXTOOL_LOWVERSION;
private high_version = DBXTOOL_VERSION;

/*
 * Start up the debugger process.
 * Create a pipe between ourselves and the debugger.
 * Pass the file descriptor of the writable end of the
 * pipe to dbx as "-P nn".
 * Dbx will be responsible for closing the readable end
 * of the pipe.  In fact, dbx will close everything but
 * stdin, stdout, stderr, and the pipe.
 * The argv passed in here points to the first argument
 * not the command name.  Argc has been adjusted accordingly.
 */
public	create_pipe(argc, argv)
int	argc;
char	*argv[];
{
	char	pout[10];
	char	**nargv;
	int	pid;
	int	parent_in[2];
	int	i;
				
	pipe(parent_in);
	sprintf(pout, "%d", parent_in[CHILD_WRITE]);

	/*
	 * Duplicate the argument vector.
	 * Change the command name to that of the debugger and
	 * leave SLOTS empty spots to add arguments.  We add
	 * the "-P xx" argument to tell the debugger where the
	 * pipe is.
	 */
	nargv = (char **) malloc((argc  + 1 + SLOTS + 1) * sizeof(char *));
	nargv[0] = DEBUGGER;
	for (i = 0; i < argc; i++) {
		nargv[i + 1 + SLOTS] = argv[i];
	}
	nargv[i + 1 + SLOTS] = nil;
	nargv[1] = "-P";
	nargv[2] = pout;
		
	ioctl(parent_in[CHILD_READ], FIONCLEX, 0);
	ioctl(parent_in[PARENT_WRITE], FIONCLEX, 0);

	/*
	 * Fork, and exec the child, close the parent's output
	 * side of the pipe and remember the parent's input side.
	 */
	if (debugpipe) {
		for(i = 0; nargv[i] != nil; i++) {
			fprintf(stderr, "%s ", nargv[i]);
		}
		fprintf(stderr, "\n");
	}
	pid = ttysw_start(get_cmd_sw_h(), nargv);
	notify_set_wait3_func(get_cmd_sw_h(), wait3_handler, pid);
	close(parent_in[PARENT_WRITE]);
	pipein = parent_in[PARENT_READ];
}

/*
 * Notify proc for child termination.
 * ARGSUSED
 */
private Notify_value wait3_handler(me, pid, status, rusage)
caddr_t	me;
int	pid;
union	wait	*status;
struct	rusage	*rusage;
{
	if (WIFSIGNALED(*status)) {
		fprintf(stderr, "Child dbx killed by signal %d\n",
		    status->w_termsig);
		exit(1);
	} else if (WIFEXITED(*status)) {
		fprintf(stderr, "Child dbx exited with status %d\n",
		    status->w_retcode);
		exit(1);
	}
}

/*
 * Read from the pipe and process operators until the I_INITDONE
 * operator comes thru.
 */
public	wait_initdone()
{
	union 	wait	status;

	if (setjmp(env) == 0) {
		check_version();
		initdone = false;
		while (not initdone) {
			pipe_input();
		}
	} else {
		wait(&status);
		wait3_handler(nil, 0, &status, nil);
	}
}

/*
 * See if the dbx and this dbxtool are compatible.
 */
public	check_version()
{
	Intermed type;
	int	version;

	type = rdtype();
	if (type == I_VERSION) {
		version = rdint();
		if (version >= low_version  and  version <= high_version) {
			return;
		}
	}
	fprintf(stderr, "Incompatible versions of dbx and dbxtool\n");
	exit(0);
}

/*
 * We have some input from the pipe.
 * Read and process one operator.
 * ARGSUSED
 */
public Notify_value pipe_input(client, fd)
Notify_client client;
int	fd;
{
	Intermed type;

	if (setjmp(env) == 1) {
		initdone = true;
		return;
	}
	type = rdtype();
	istracing = false;
	switch(type) {
	case I_PRINTLINES:
		curline = rd_filefuncline(curfile, fullcurfile, nil, STRBUF);
		rdint();
		dbx_printlines(fullcurfile, curline);
		break;

	case I_INITDONE:
		curline = rd_filefuncline(curfile, fullcurfile, curfunc,STRBUF);
		if (initdone) {
			dbx_display_source(fullcurfile, curline);
		} else {
			initdone = true;
		}
		break;

	case I_NEWINITDONE:
		curline = rd_filefuncline(curfile, fullcurfile, curfunc,STRBUF);
		brkline = rd_filefuncline(brkfile, fullbrkfile, brkfunc,STRBUF);
		set_hollow_to_cur();
		if (initdone) {
			dbx_display_source(fullhollowfile, hollowline);
		} else {
			initdone = true;
		}
		break;
	
	case I_KILL:
		isstopped = false;
		curfile[0] = '\0';
		fullcurfile[0] = '\0';
		curline = 0;

		brkfile[0] = '\0';
		fullbrkfile[0] = '\0';
		brkline = 0;

		hollowfile[0] = '\0';
		fullhollowfile[0] = '\0';
		hollowline = 0;

		dbx_reinit(false);
		break;

	case I_REINIT:
		isstopped = false;
		curfile[0] = '\0';
		fullcurfile[0] = '\0';
		dbx_reinit(true);
		check_version();
		break;

	case I_CHDIR: {
		char	dir[256];

		rdstring(dir, sizeof(dir));
		chdir(dir);
		remember_curdir();
		break;
	}

	case I_QUIT: {
		int	exit_status;

		exit_status = rdint();
		dbx_cleanup();
		exit(exit_status);
		/* NOTREACHED */
	}

	case I_BRKSET: {
		char	filename[256];
		int	lineno;

		rdstring(filename, sizeof(filename));
		lineno = rdint();
		dbx_setbreakpoint(filename, lineno);
		break;
	}

	case I_BRKDEL: {
		char	filename[256];
		int	lineno;

		rdstring(filename, sizeof(filename));
		lineno = rdint();
		dbx_delbreakpoint(filename, lineno);
		break;
	}

	case I_RESUME:
		dbx_resume();
		break;
	
	case I_USE: {
		int	n;
		char	dir[100];

		n = rdint();
		del_uses(sourcepath);
		sourcepath = nil;
		while (n--) {
			rdstring(dir, sizeof(dir));
			add_use(dir);
		}
		strcpy(fullcurfile, findsource(curfile));
		break;
	}

	case I_EMPHASIZE: 
		curline = rd_filefuncline(curfile, fullcurfile, nil, STRBUF);
		dbx_emphasize(fullcurfile, curline);
		break;

	case I_CALLSTACK:
		hollowline = rd_filefuncline(hollowfile, fullhollowfile,
		    nil, STRBUF);
		dbx_display_source(fullhollowfile, hollowline);
		break;

	case I_TRACE:
		isstopped = false;
		istracing = true;
		curline = rd_filefuncline(curfile, fullcurfile, curfunc,STRBUF);
		set_brk_to_cur();
		set_hollow_to_cur();
		dbx_resume();
		dbx_display_source(fullhollowfile, hollowline);
		break;

	case I_FONT: {
		char	filename[256];

		rdstring(filename, sizeof(filename));
		dbx_setfont(filename);
		break;
	}

	case I_TOOLENV:
		dbx_prtoolenv();
		break;

	case I_WIDTH: {
		int	width;

		width = rdint();
		dbx_setwidth(width, true);
		break;
	}

	case I_SRCLINES: {
		int	srclines;

		srclines = rdint();
		dbx_setsrclines(srclines, true);
		break;
	}

	case I_CMDLINES: {
		int	cmdlines;

		cmdlines = rdint();
		dbx_setcmdlines(cmdlines, true);
		break;
	}

	case I_DISPLINES: {
		int	displines;

		displines = rdint();
		dbx_setdisplines(displines, true);
		break;
	}

	case I_TOPMARGIN: {
		int	topmargin;

		topmargin = rdint();
		dbx_settopmargin(topmargin);
		break;
	}

	case I_BOTMARGIN: {
		int	botmargin;

		botmargin = rdint();
		dbx_setbotmargin(botmargin);
		break;
	}

	case I_BUTTON: {
		int	seltype;
		char	str[512];

		seltype = rdint();
		rdstring(str, sizeof(str));
		dbx_new_button((Seltype) seltype, str);
		break;
	}

	case I_UNBUTTON: {
		char	str[512];

		rdstring(str, sizeof(str));
		dbx_unbutton(str);
		break;
	}

	case I_MENU: {
		int	seltype;
		char	str[512];

		seltype = rdint();
		rdstring(str, sizeof(str));
		dbx_new_menu((Seltype) seltype, str);
		break;
	}

	case I_UNMENU: {
		char	str[512];

		rdstring(str, sizeof(str));
		dbx_unmenu(str);
		break;
	}

	case I_STOPPED:
		isstopped = true;
		curline = rd_filefuncline(curfile, fullcurfile, curfunc,STRBUF);
		brkline = rd_filefuncline(brkfile, fullbrkfile, brkfunc,STRBUF);
		set_hollow_to_cur();
		dbx_display_source(fullhollowfile, hollowline);
		break;
	
	case I_DISPLAY: {
		char	display_file[256];

		rdstring(display_file, sizeof(display_file));
		dbx_display(display_file);
		break;
	}


	default:
		fprintf(stderr, "pipe_input: bad type %d\n", type);
		exit(1);
	}
	return(NOTIFY_DONE);
}

/*
 * Read a type descriptor from the pipe.
 */
private	Intermed rdtype()
{
	Intermed type;
	int	r;
	union	wait	status;
	extern  int	errno;

	type = I_BADOP;
	r = myread(pipein, (char *) &type, sizeof(type));

	if (ord(type) < ord(I_FIRST) or ord(type) > ord(I_LAST)) {
		fprintf(stderr, "rdtype: bad type %d\n", ord(type));
		exit(1);
	}
	if (debugpipe) {
		fprintf(stderr, "pipe input: %s\n", inter_type[ord(type)]);
	}
	return(type);
}

/*
 * Read a string from the pipe.
 */
private	rdstring(buf, maxlen)
char	*buf;
int	maxlen;
{
	Intermed type;
	char	dummy[512];
	int	len;
	int	readlen;
	int	extralen;

	type = rdtype();
	if (type != I_STRING) {
		fprintf(stderr, "expected I_STRING but got %s", 
			inter_type[ord(type)]);
		exit(1);
	}
	myread(pipein, (char *) &len, sizeof(len));
	if (maxlen == 0) {
		readlen = 0;
		extralen = len;
	} else if (len > maxlen - 1) {
		readlen = maxlen - 1;
		extralen = len - maxlen + 1;
	} else {
		readlen = len;
		extralen = 0;
	}
	if (readlen > 0) {
		myread(pipein, buf, readlen);
	}
	buf[readlen] = '\0';
	while (extralen > 0) {
		if (extralen > sizeof(dummy)) {
			readlen = sizeof(dummy);
		} else {
			readlen = extralen;
		}
		myread(pipein, dummy, readlen);
		extralen -= readlen;
	}
	if (debugpipe) {
		fprintf(stderr, "\t%s\n", buf);
	}
}

/*
 * Read an integer.  Here we are looking for both the type
 * indicator and the integer value.  Return the value.
 */
private	rdint()
{
	Intermed type;
	int	i;

	type = rdtype();
	if (type != I_INT) {
		fprintf(stderr, "expected I_INT but got %s\n", 
			inter_type[ord(type)]);
		exit(1);
	}
	myread(pipein, (char *) &i, sizeof(i));
	if (debugpipe) {
		fprintf(stderr, "\t%d\n", i);
	}
	return(i);
}

/*
 * Front-end to the read() system call that does
 * error checking.  If we read zero bytes this implies
 * that the pipe was closed by dbx.  This usually indicates
 * that dbx died.  In this case longjmp out of here.
 */
private int myread(fd, buf, nbytes)
int	fd;
char	*buf;
int	nbytes;
{
	int	r;

	r = read(fd, buf, nbytes);
	if (r == 0) {
		longjmp(env, 1);
	}
	if (r == -1) {
		perror("dbxtool read from pipe failed");
		exit(1);
	}
	if (r != nbytes) {
		fprintf( stderr,
		    "dbxtool read from pipe failed: read %d bytes\n", r);
		exit(1);
	}
	return(r);
}

/*
 * Delete a list.
 */
private	del_uses(uses)
Use	uses;
{
	Use 	u;
	Use	next;

	for (u = uses; u != nil; u = next) {
		next = u->next;
		dispose(u->dir);
		dispose(u);
	}
}

/*
 * Add an element to the uses list.
 */
private	add_use(dir)
char	*dir;
{
	Use	u;
	Use	*bpatch;

	bpatch = &sourcepath;
	for (u = sourcepath; u != nil; u = u->next) {
		bpatch = &u->next;
	}
	u = new(Use);
	u->dir = strdup(dir);
	u->next = nil;
	*bpatch = u;
}

/*
 * Look for a file by applying the 'uses' list.
 */
public String findsource(filename)
String filename;
{
	register File f;
	register String src;
	Use	use;
	static	char	filenamebuf[256];

	if (filename == nil || filename[0] == '\0') {
		return("");
	}
	if (filename[0] == '/') {
		src = filename;
	} else {
		src = "";
		for (use = sourcepath; use != nil; use = use->next) {
			sprintf(filenamebuf, "%s/%s", use->dir, filename);
			f = fopen(filenamebuf, "r");
			if (f != nil) {
				fclose(f);
				src = filenamebuf;
				break;
			}
		}
	}
	return src;
}

/*
 * Return whether we are stopped or not
 */
public	Boolean	get_isstopped()
{
	return(isstopped);
}

/*
 * Return whether we are tracing or not.
 */
public	Boolean	get_istracing()
{
	return(istracing);
}

/*
 * Return the current breakpoint line number.
 */
public	get_brkline()
{
	return(brkline);
}

/*
 * Return the current breakpoint function name
 */
public	char	*get_brkfunc()
{
	return(brkfunc);
}

/*
 * Return the current breakpoint file name
 */
public	char	*get_brkfile()
{
	return(findsource(brkfile));
}

/*
 * Read a file, function, and line number
 */
private rd_filefuncline(file, fullfile, func, strbuf)
char	*file;
char	*fullfile;
char	*func;
int	strbuf;
{
	int	line;

	if (file != nil) {
		rdstring(file, strbuf);
		if (fullfile != nil) {
			strcpy(fullfile, findsource(file));
		}
	}
	if (func != nil) {
		rdstring(func, strbuf);
	}
	line = rdint();
	return(line);
}

/*
 * Return the current file name
 */
public	char	*get_curfile()
{
	return(fullcurfile);
}

/*
 * Return the current line number
 */
public	get_curline()
{
	return(curline);
}

/*
 * Return the current file name unmodified
 */
public	char	*get_simple_curfile()
{
	return(curfile);
}

/*
 * Return the pipe input file descriptor.
 */
public	get_pipein()
{
	return(pipein);
}

/*
 * Return the hollow file name.
 */
public	char *get_hollowfile()
{
	return(fullhollowfile);
}

/*
 * Return the hollow line number
 */
public	get_hollowline()
{
	return(hollowline);
}

/*
 * Set the 'hollow' attributes to the same as the 'cur' ones.
 */
private set_hollow_to_cur()
{
	strcpy(hollowfile, curfile);
	strcpy(fullhollowfile, fullcurfile);
	hollowline = curline;
}

/*
 * Set the 'brk' attributes to the same as the 'cur' ones.
 */
private set_brk_to_cur()
{

	strcpy(brkfile, curfile);
	strcpy(fullbrkfile, fullcurfile);
	strcpy(brkfunc, curfunc);
	brkline = curline;
}
