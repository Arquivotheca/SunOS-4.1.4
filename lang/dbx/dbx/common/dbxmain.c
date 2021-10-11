#ifndef lint
static  char sccsid[] = "@(#)dbxmain.c 1.1 94/10/31 SMI";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

static char release_version[] = "@(#)RELEASE dbx 4.1-FCS 94/10/31";

/*
 * Debugger main routine.
 */

#include "defs.h"
#include <signal.h>
#include <errno.h>
#include "machine.h"
#include "dbxmain.h"
#include "symbols.h"
#include "scanner.h"
#include "process.h"
#include "eval.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "runtime.h"
#include "pipeout.h"
#include "library.h"
#include "getshlib.h"

#ifdef mc68000
#  include "fpa_inst.h"
#endif mc68000

#include <sys/types.h>          /* needed for keybd xlation change */
#include <sys/ioctl.h>          /*              ''                 */
#include <sundev/kbio.h>        /*              ''                 */
#include <sundev/kbd.h>         /*              ''                 */
#include <sys/file.h>           /*              ''                 */
#include <signal.h>
#include <ctype.h>
#ifdef TIMING
#include <sys/time.h>
extern struct timeval timebuf1;
extern struct timeval timebuf2;
#endif

#define FILE_OK	0
#define EXEC_OK 1

#ifndef public

#include <setjmp.h>

#define isterm(file)	(interactive or isatty(fileno(file)))

Boolean multi_stepping;	 		/* "step n" or "next n" where n > 1 */
Boolean save_keybd;			/* Do we worry about keyboard state */
int	keybd_xlation;			/* State of the keyboard */
int	keybd_fd;			/* File descriptor for the keyboard */

#endif

public char *main_routine;		/* starting routine name */
public jmp_buf env;			/* setjmp/longjmp data */
public Boolean coredump;		/* true if using a core dump */
public Boolean runfirst;		/* run program immediately */
public Boolean interactive;		/* standard input IS a terminal */
public Boolean kernel;			/* kernel address mapping */
public Boolean lexdebug;		/* trace yylex return values */
public Boolean tracebpts;		/* trace create/delete breakpoints */
public Boolean traceexec;		/* trace process execution */
public Boolean tracesyms;		/* print symbols as their read */
public Boolean debugpipe;		/* print pipe operators */
public File corefile;			/* File id of core dump */
public Boolean qflag;
public Boolean pascal_xl = true;/* be compatible with "pc -xl" */
private int debuggee_pid = 0;		/* Pid of non-child debuggee */

#define FIRST_TIME 0			/* initial value setjmp returns */

private char outbuf[BUFSIZ];		/* standard output buffer */
private	char prompt[100];		/* cmdline prompt */
private int firstarg;			/* first program argument (for -r) */
private char **argv_gbl;		/* Save a pointer to the args */
private int lastarg;			/* Argv index to program name */
private String initfile = ".dbxinit";	/* Commands file */
private String dbxinit_file;		/* Altername .dbxinit file */
private Boolean dbxinit_filetmp;	/* Is dbxinit_file temporary? */
extern	char	**environ;
extern	int	errno;

private void catchintr();
private void intr_symreading();
private Boolean doinput();
private int sigalarm();
private	Symbol find_func();

extern Boolean nocerror;
private Boolean doCoreDump = false;

#ifdef DEBUGGING
private Boolean do_pause = false;	/* pause before doing anything */
private Boolean echo_pid = false;
#endif

/*
 * Main program.
 */

public main(argc, argv)
int argc;
String argv[];
{
    register Integer i;
    struct sigvec sigv;
    float f1;
    int mask;

    onsyserr(EOPNOTSUPP, ERR_IGNORE);
    onsyserr(ENOTDIR, ERR_IGNORE);
    mask = sigblock(SIGINT_MASK);
#ifdef TIMING
    gettimeofday(&timebuf1, NULL);
#endif
    cmdname = argv[0];
    if( argc > 1  &&  streq(argv[1], "-D") ) {
	doCoreDump = true;
	++argv; --argc;
    }
    argv_gbl = argv;
    onsyserr(EINTR, nil);
    signal(SIGALRM, sigalarm);
    nocerror = true;
#ifdef mc68000
    sunfpapresent() ;
    f1 = 3.0;
    f1 = f1 + 1.0;	/* do a floating point operation */
    mc68881present();
#endif mc68000
    nocerror = false;
    setbuf(stdout, outbuf);
    setlinebuf(stdout); /* synchronise for terminal *and* files */
    scanargs(argc, argv);
#ifdef DEBUGGING
	if (echo_pid)
		printf ("pid is %d\n", getpid());
	if (do_pause)
		{
		char buf[128];
		printf ("Hit return to continue ... "); fflush(stdout);
		gets(buf);
		}
#endif
    if (istool()) {
	strcpy(prompt, "(dbxtool) ");
    } else {
        sprintf(prompt, "(%s) ", cmdname);
    }
    alloc_functab();
    language_init();
    symbols_init();
    source_init();
    if (objname != nil) {
        process_init();
    }
    if (runfirst) {
	if (setjmp(env) == FIRST_TIME) {
	    int oldargc;

	    oldargc = argc;
	    arginit();
	    for (i = firstarg; i < oldargc; i++) {
		newarg(argv[i]);
	    }
		savetty(0);
	    run();
	    /* We arrive here only if run can't find an executable */
	} else {
	    runfirst = false;
	}
    } else {
	/*
	 * Init() may run into a problem and call error() who calls
	 * erecover() who does a longjmp().  Must have the longjmp
	 * buffer initialized.  We really want to get to the setjmp()
	 * in begin_debugging().  This is all rather ugly.
	 */
	if (setjmp(env) == FIRST_TIME) {
	    sigv.sv_handler = intr_symreading;
	    sigv.sv_mask = 0;
	    sigv.sv_onstack = 0;
	    sigvec(SIGINT, &sigv, 0);
	    sigsetmask(mask);
	    init();
        if (coredump && process != nil) {
		printf("program terminated by ");
		psig(errnum(process), errcode(process));
		printf("\n");
		}
	} else {
	    clean_slate();
	}
    }
    begin_debugging();
}

/*
 * This routine enters the command level of dbx.
 * yyparse() accepts commands from the user.
 * If an error occurs, error() is called.  It will call
 * erecrover() which calls longjmp() which returns to the
 * setjmp() here.
 */
private begin_debugging()
{
    int	keybd_flag;
    struct sigvec sigv;

    if (save_keybd) {
        onsyserr(ENODEV, ERR_IGNORE);
        keybd_fd = open("/dev/kbd", O_RDONLY, 0);
        onsyserr(ENODEV, ERR_CATCH);
        keybd_xlation = 1;			/* Normal keyboard operation */
    }
    savetty( 0 );	/* 0 ==> ttyfix's private ttyinfo struct */
    dbx_initdone();

    /*
     * erecover() does a longjmp() that typically returns here.
     */
    setjmp(env);
    sp = &stack[0];	/* reset stack after error */
    restoretty( 0 );	/* 0 ==> ttyfix's private ttyinfo struct */
    if (save_keybd) {
	keybd_flag = 1;
	ioctl(keybd_fd, KIOCTRANS, &keybd_flag);
    }
    sigv.sv_handler = catchintr;
    sigv.sv_mask = 0;
    sigv.sv_onstack = 0;
    sigvec(SIGINT, &sigv, 0);
    batchcmd = false;
    yyparse();
    putchar('\n');
    quit(0);
}

/*
 * Initialize the world, including setting initial input file
 * if the file exists.
 */

public init()
{
    enterkeywords();
    scanner_init();
    dotdbxinit(true);
    object_init();
    dotdbxinit(false);
    if (coredump or debuggee_pid != 0) {
	dbx_stopped();
    }
}

/*
 * Initialize the object file.
 * If there is a coredump set the file, func and linenumber.
 */
public Boolean reading_to_attach;	/* flag to communicate w/ pio */

private object_init()
{
    Address addr;
    Symbol s;
    Boolean fss_flag;   /* need to find some source? */
	int readobjrc;

    if (objname != nil) {
		main_routine = "main";
		if (debuggee_pid != 0)
	    	reading_to_attach = true;

        if (not coredump and not runfirst and debuggee_pid == 0) {
	    start(nil, nil, nil, nil);
        }
		readobjrc =  readobj(objname);
		if(readobjrc) {
	    	bpinit();
		}
        if (not coredump and not runfirst and debuggee_pid == 0) {
	    start(nil, nil, nil, nil);
        }
        setcurfunc(nil);


        if (readobjrc) {
	    objtime(objname);

	    fss_flag = true;
	    if (coredump) {
		if (kernel) {
		    coredump_getkerinfo();
		} else if(use_shlib) {
			getshlib();
		}
	        setcurfunc(whatblock(pc, true));
	        addr = pc;
	    } else if (debuggee_pid != 0) {
		grab_nonchild(debuggee_pid);
		reading_to_attach = false;

#ifndef CONSTANT_FCN_PROLOG
		/*
		 * If the function prolog is not constant, then at this
		 * point we must call findbeginning() for every function.
		 * (See mappings.c.)
		 */
		 find_each_fcn_begin( );
#endif CONSTANT_FCN_PROLOG

	        setcurfunc(whatblock(pc, true));
		addr = pc;
	    } else {
		addr = show_main();
		fss_flag = false;	/* don't find sources */
	    }
	    setsource(srcfilename(addr));
	    brkline = srcline(addr);
	    cursrcline = brkline == 0 ? 1 : brkline;
	    brksource = cursource;
	    brkfunc = curfunc;

	    if( fss_flag ) find_some_source();
	    /* Otherwise, we've already done what's needed. */

	} else {
	    clean_slate();
	    reading_to_attach = false;
	}
    }
}

/*
 * Find the "main" function and see if there is source code
 * associated with it.  If so, set this function as the current
 * one and return it address.  This is used to display the source
 * code associated with the "main" routine ("MAIN" for FORTRAN).
 */
public Address show_main()
{
    Symbol mainsym;
    Symbol MAINsym;
    Symbol s;
    Name n;
    Address addr;

    addr = 0;
    mainsym = find_func(main_routine);
    MAINsym = find_func("MAIN");
    if (mainsym != nil and mainsym->symvalue.funcv.src) {
	s = mainsym;
	addr = firstline(mainsym);
    } else if (MAINsym != nil and MAINsym->symvalue.funcv.src) {
	s = MAINsym;
	addr = firstline(MAINsym);
    } else {
	if (mainsym != nil) {
	    s = mainsym;
	    addr = mainsym->symvalue.funcv.beginaddr;
        } else {
	    s = nil;
	}
	warning("main routine not compiled with the -g option");
    }
    setcurfunc(s);
    return(addr);
}

private Symbol find_func(funcname)
char *funcname;
{
    Symbol s;
    Name n;

    n = identname(funcname, false);
    find (s, n) where
	(s->class == FUNC or s->class == PROC)
    endfind(s);
    return(s);
}

/*
 * Do the .dbxinit file.
 * Look for it in the current directory and, if not found,
 * in the home directory.
 * The -s option specifies an alternate .dbxinit file and
 * the -sr option says to remove the alternate file after
 * processing it.
 *
 * we call this twice, before and after object_init. The second
 * call is a noop unless -sr is set. If set, we look for
 * a secondary file, "tmpfile-2" which contains input to be processed
 * after we have an active process.
 */
private dotdbxinit(before_readobj)
    Boolean before_readobj;
{
    String home;
    char buf[1024];
    extern String getenv();

    if(before_readobj) {
	if (dbxinit_file != nil) {
	    doinput(dbxinit_file);
	    if (dbxinit_filetmp) {
		unlink(dbxinit_file);
	    }
	} else {
	    if (not doinput(initfile)) {
		home = getenv("HOME");
		if (home != nil) {
		    sprintf(buf, "%s/%s", home, initfile);
		    doinput(buf);
		}
	    }
	}
    } else if(dbxinit_file != nil and dbxinit_filetmp) {
	sprintf(buf,"%s-2",dbxinit_file);
	doinput(buf);
	unlink(buf);
    }
}

/*
 * Given a file, open it and read dbx commands from it.
 * If everything is ok, return true, otherwise return false.
 */
private Boolean doinput(file)
char *file;
{
    File f;

    f = fopen(file, "r");
    if (f != nil) {
	fclose(f);
	setinput(file);
	return(true);
    }
    return(false);
}

/*
 * After a non-fatal error we jump back to command parsing.
 */

public erecover()
{
    multi_stepping = false;
    gobble();
    fflush(stdout);
    if (process != nil and reg(REG_PC) != CALL_RETADDR) {
	Boolean doing_call = in_dbxcall();
	if (    (doing_call == false) or 
		(doing_call == true and batchcmd == true) ) {
            longjmp(env, CMD_LEVEL);
	} else {
            restoretty( 0 );
	    longjmp(env, CALL_RETURN);
	}
    } else {
        restoretty( 0 );
	longjmp(env, CALL_RETURN);
    }
}

private String sys_siglist[] = {
    "no signal",
    "hangup",
    "interrupt",
    "quit",
    "illegal instruction",
    "trace/BPT trap",
    "abort",
    "emulator trap",
    "arithmetic exception",
    "kill",
    "bus error",
    "segmentation violation",
    "bad system call",
    "broken pipe",
    "real timer alarm",
    "software termination",
    "urgent I/O condition",
    "stop",
    "stop from tty",
    "continue",
    "child status change",
    "stop (tty input)",
    "stop (tty output)",
    "possible input/output",
    "exceeded CPU time limit",
    "exceeded file size limit",
    "virtual timer alarm",
    "profiling timer alarm",
    "window changed",
    "resource lost",
    "user-defined signal 1",
    "user-defined signal 2",
};

/*
 * These strings were borrowed from the kill command.
 */
private String sys_signames[] = { NULL,
"HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE",	/* 1-8 */
"KILL", "BUS", "SEGV", "SYS", "PIPE", "ALRM", "TERM", "URG",	/* 9-16 */
"STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU",	/* 17-24 */
"XFSZ", "VTALRM", "PROF", "WINCH", "LOST", "USR1", "USR2", 0,	/* 25-32 */
};

private String sys_signames_lc[] = { null,
"hup", "int", "quit", "ill", "trap", "abrt", "emt", "fpe",	/* 1-8 */
"kill", "bus", "segv", "sys", "pipe", "alrm", "term", "urg",	/* 9-16 */
"stop", "tstp", "cont", "chld", "ttin", "ttou", "io", "xcpu",	/* 17-24 */
"xfsz", "vtalrm", "prof", "winch", "lost", "usr1", "usr2", 0,	/* 25-32 */
};

public psig(signo, sigcode)
int signo;
int sigcode;
{
    char *codename;

    if (signo > 0 && signo < NSIG) {
	codename = sigcodename(signo, sigcode);	
	printf("signal %s (%s)", sys_signames[signo],
	  codename != NULL ? codename : sys_siglist[signo]);
    } else {
	printf("signal %d (Unknown signal)", signo);
    }
}

/*
 * Convert a string into a signal number.
 */
public name_to_signo(name)
char *name;
{
    int i;

    for (i = 0; i < NSIG; i++) {
	if (sys_signames[i] != nil) {
	    if ((streq(sys_signames[i], name)) or
		(streq(sys_signames_lc[i], name))){
	        return(i);
	    }
	}
    }
    error("`%s' is not a valid signal name", name);
    /* NOTREACHED */
}

/*
 * Convert a signal number into a string.
 */
public char *signo_to_name(signo)
int signo;
{
    static char message[7+11+1];

    if (signo >= 0 and signo < NSIG) {
	return(sys_signames[signo]);
    } else {
	(void) sprintf(message, "signal %d", signo);
	return(message);
    }
}


/*
 * This routine is called when an interrupt occurs.
 */

private void catchintr()
{
    printf("\n");
    fflush(stdout);
    longjmp(env, CMD_LEVEL);
}

/*
 * Scan the argument list.  This includes setting up the sourcepath
 * list, here and in init_files (below).
 */

private scanargs(argc, argv)
int argc;
String argv[];
{
    register int i, j;
    register Boolean foundfile;

    runfirst = false;
    interactive = false;
    lexdebug = false;
    tracebpts = false;
    traceexec = false;
    tracesyms = false;
    foundfile = false;
    corefile = nil;
    coredump = false;
    number_of_functions = 500;
    sourcepath = list_alloc();

    i = 1;
    while (i < argc and (not foundfile or (corefile == nil and not runfirst))) {
	if (argv[i][0] == '-') {
	    if (streq(argv[i], "-I")) {
			++i;
			if (i >= argc) {
				warning("missing directory for -I");
			}
			list_append_tail(argv[i], sourcepath);
	    } else if (streq(argv[i], "-f")) {
			++i;
			if (i >= argc) {
				warning("missing value for -f");
			}
			number_of_functions = atoi(argv[i]);
	    } else if (streq(argv[i], "-P")) {
			++i;
			if (i >= argc) {
				warning("missing descriptor number for -P option");
			}
			pipeout = atoi(argv[i]);
				for (j = fileno(stderr) + 1; j < getdtablesize(); j++) {
					if (j != pipeout) {
						close(j);
					}
				}
			dbx_version();
	    } else if (streq(argv[i], "-s") || streq(argv[i], "-sr")) {
			++i;
			if (i >= argc) {
				warning("missing file for -s option");
			}
			dbxinit_file = argv[i];
			if (streq(argv[i - 1], "-sr")) {
				dbxinit_filetmp = true;
			}
	    } else if (streq(argv[i], "-kbd")) {
			save_keybd = true;
	    }
#ifdef DEBUGGING
		else if (streq (argv[i], "-p")) {
			echo_pid = true;
		} else if (streq (argv[i], "-pp")) {
			echo_pid = true;
			do_pause = true;
	    } 
#endif
		else {
	        setoptions(argv[i]);
	    }
	} else if (not foundfile) {
	    lastarg = i - 1;
	    objname = argv[i];
	    foundfile = true;
	} else if (corefile == nil) {
	    opencore(argv[i]);
	    if (!corefile && !debuggee_pid) {
		warning("can't read \"%s\", corefile ignored", argv[i]);
	    }
	}
	++i;
    }
    if (i < argc and not runfirst) {
	warning("extraneous argument %s", argv[i]);
    }
    firstarg = i;
    if (foundfile) {
        init_files(corefile);
    } else {
	lastarg = i - 1;
    }
}

/*
 * Open the object and core files, and make sure that the sourcepath
 * list includes the directory where the executable (objname) is really
 * located, whether or not it is a symbolic link.  (That is, follow that
 * link.)
 */
#define PATHLEN 256

private init_files(core)
char	*core;
{
    File f;
    char *tmp, *real_objname;
    char slbuf[ PATHLEN ];
    int cc;

    f = fopen(objname, "r");
    if (f != nil) {
        fclose(f);

	nocerror = true;
	cc= readlink( objname, slbuf, PATHLEN );
	nocerror = false;

	if( cc > 0 ) {
	    /* yes, it was a symbolic link */
	    slbuf[ cc ] = 0;
	    real_objname = slbuf;
	} else {
	    real_objname = objname;
	}

        if (rindex(real_objname, '/') != nil) {
            tmp = strdup(real_objname);
            *(rindex(tmp, '/')) = '\0';
	} else {
	    tmp = export_curdir();;
	}

	list_append_tail(tmp, sourcepath);

        if (core == nil) {
	    if (kernel) {
		opencore("/dev/mem");
	    } else if (debuggee_pid == 0) {
                opencore("core");
	    }
        }
    } else {
        warning("can't read %s", objname);
	objname = nil;
    }
    dbx_use(sourcepath);
}

/*
 * Try to open a core file.
 */
private opencore(file)
String file;
{
    char *cp;

    for (cp = file; isdigit(*cp); cp++)
        ;
    if (*cp != '\0') {
        corefile = fopen(file, "r");
        if (corefile == nil) {
	    coredump = false;
        } else {
	    coredump = true;
        }
    } else {
	debuggee_pid = atoi(file);
	coredump = false;
    }
}

/*
 * Change the program that is being debugged.
 * Check that the file exists and is executable.
 * Redirect the current aliases and dbxenv parameters to
 * a tmp file and add the "-sr tmpfile" option.
 * Then, append the new program name and corefile name to
 * the current argument list.
 * Finally, Exec over top of our selves.
 */
public	debug(p)
Node	p;
{
    char **argv;
    char tmpfile[100];
    char *prog;

    prog = p->value.debug.prog;
    if (prog == nil) {
	printf("Debugging: ");
	pr_cmdline(false, 0);
	return;
    }
    if (access(prog, FILE_OK) == -1) {
	error("File `%s' does not exist", prog);
    }
    if (not p->value.debug.kernel and access(prog, EXEC_OK) == -1) {
	error("File `%s' is not executable", prog);
    }
    strcpy(tmpfile, "/tmp/dbxinit.XXXXXX");
    mktemp(tmpfile);
    setout(tmpfile);
    print_alias(nil);
    dbx_prdbxenv();
    display_select_list();
    unsetout();
    argv = (char **) malloc((lastarg + 1 + 5) * sizeof(char *));
    if (argv == 0)
	fatal("No memory available. Out of swap space?");
    buildargv(argv, tmpfile, p->value.debug.kernel, prog,
      p->value.debug.corefile);
    kill_process();
    dbx_reinit();
    execvp(argv[0], argv, environ);
    error("execvp of %s failed, errno %d", argv[0], errno);
}

/*
 * Begin debugging another program (or the same one over again).
 * Save the state in a temp file.  The state currently consists
 * of the aliases, the dbxenv info, the use, catch, and ignore
 * lists, the breakpoints and the display list.
 * Build an argument list for the new dbx.
 * Use the arguments from the current invokation up to the program
 * name.  Then append the "-sr tmpfile" arg and the program name
 * name and corefile name.  The "-sr tmpfile" arg says to use
 * tmpfile instead of the .dbxinit file.  This allows us to
 * save the state in the tmpfile and reinitialize with it on the
 * next incarnation.
 *
 * commands that require an active process (status, display, run) go 
 * into a separate file, tmpfile-2, which is processed *after* object_init
 */
public	reinit()
{
    char    **argv;
    char    tmpfile[100], tmpfile2[100];
    int    i;

    strcpy(tmpfile, "/tmp/dbxinit.XXXXXX");
    mktemp(tmpfile);
    setout(tmpfile);
    print_alias(nil);
    dbx_prdbxenv();
    pr_use();
    pr_ignore(process);
    pr_catch(process);
    display_select_list();
    unsetout();

    sprintf(tmpfile2, "%s-2", tmpfile);
    unlink(tmpfile2);
    setout(tmpfile2);
    status_cmd();
    pr_display_list();
    printf("run ");
    pr_cmdline(true, 1);
    unsetout();

    argv = (char **) malloc((lastarg + 1 + 4) * sizeof(char *));
    if (argv == 0)
	fatal("No memory available. Out of swap space?");
    buildargv(argv, tmpfile, kernel, objname, nil);
    kill_process();
    dbx_reinit();
    execvp(argv[0], argv, environ);
    exit(1);
}

/*
 * Build an argv list for a new incarnation of dbx.
 * Copy the args from this version up until the program name.
 * Skip any "-sr" options because we will add our own.
 * Also, skip and "-k" options.
 */
private buildargv(argv, tmpfile, kflag, prog, core)
char **argv;
char *tmpfile;
Boolean kflag;
char *prog;
char *core;
{
    int	i, j;

    j = 0;
    for (i = 0; i < lastarg + 1; i++) {
	if (streq(argv_gbl[i], "-sr")) {
	    i++;
	} else {
		argv[j] = argv_gbl[i];
		j++;
	}
    }
    argv[j++] = "-sr";
    argv[j++] = tmpfile;
    argv[j++] = prog;
    argv[j++] = core;
    argv[j] = nil;
}


/*
 * Take appropriate action for recognized command argument.
 */

private setoptions(str)
char *str;
{
    char *cp;

    for (cp = &str[1]; *cp; cp++) {
        switch (*cp) {
    	case 'r':   /* run program before accepting commands */
    	    runfirst = true;
    	    coredump = false;
    	    break;

    	case 'I':
	    list_append_tail(&cp[1], sourcepath);
	    return;

    	case 'f':
	    number_of_functions = atoi(&cp[1]);
	    return;

    	case 'i':
    	    interactive = true;
    	    break;

    	case 'b':
    	    tracebpts = true;
    	    break;

    	case 'e':
	    traceexec = true;
	    break;

	case 's':
	    tracesyms = true;
	    break;

	case 'd':
	    debugpipe = true;
	    break;

case 'q':
	qflag = true;
	break;

	case 'l':
#   	    ifdef LEXDEBUG
		lexdebug = true;
#	    else
		warning("\"-l\" only applicable when compiled with LEXDEBUG");
#	    endif
	    break;

	default:
	    warning("unknown option '%c'", *cp);
	}
    }
}

/*
 * Exit gracefully.
 */

public quit(r)
Integer r;
{
    kill_process();
    dbx_quit(r);
#ifdef TIMING
    gettimeofday(&timebuf2, NULL);
    prtime(&timebuf2, &timebuf1, "After symbols");
#endif

    /*
     * Don't dump unless exit code is an error
     * (E.g., the 'quit' command won't dump.)
     */
    if( r && doCoreDump ) {
	printf( "DBX dumping BIG core file!\n" );
	abort();
    }

    exit(r);
}

/*
 * Return the prompt.
 */
public String get_prompt()
{
    return(prompt);
}

/*
 * Dummy routine to catch SIGALRM's generated from paced
 * traces, setitimer()s and pause()s.
 */
private sigalarm()
{
}

/*
 * Signal catcher for SIGINT while the symbols are being read.
 * If a SIGINT comes in free the symbol table and print a message.
 */
private void intr_symreading()
{
	symbol_free();
	error("Symbol reading interrupted - symbol reading abandoned");
	/* NOTREACHED */
}
