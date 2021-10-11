#ifndef lint
static	char sccsid[] = "@(#)help.c 1.18 88/02/22 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Help messages
 */

#include "defs.h"
#include "scanner.h"
#include "names.h"
#include "symbols.h"
#include "pipeout.h"
#include "commands.h"

/*
 * Give the user some help.
 */
public help(t)
Token t;
{
    if (ord(t) == 0) {
	pr_defhelp();
	return;
    }
    switch (t) {
    case (Token) '/':
	printf("/string/               - Search forward for <string> in the current file\n");
	break;
    case (Token) '?':
	printf("?string?               - Search backward for <string> in the current file\n");
	break;
    case ALIAS:
	printf("alias                          - Print the value of all aliases\n");
	printf("alias <newname>                - Print the value of alias <newname>\n");
        printf("alias <newname> \"<cmd>\"        - Create an alias\n");
	break;
    case ASSIGN:
        printf("assign <var> = <exp>   - Assign the value of the <exp> to <var>\n");
        printf("set <var> = <exp>      - Assign the value of the <exp> to <var>\n");
	break;
    case BUTTON:
        printf("button <seltype> <cmd> - Define a software button (dbxtool)\n");
	pr_seltypes();
	break;
    case CALL:
        printf("call <proc>([params])  - Call the procedure\n");
	break;
    case CATCH:
	printf("catch                  - Print a list of the caught signals\n");
        printf("catch <num> <num>...   - Catch signal(s) numbered <num>\n");
        printf("catch <sig> <sig>...   - Catch signal(s) named by <sig>\n");;
	break;
    case CD:
	printf("cd                     - Change to the $HOME directory\n");
	printf("cd <dir>               - Change the current <dir>\n");
	break;
    case CLEAR:
	printf("clear                  - Clear all breakpoints at the current stopping point\n");
	printf("clear <lineno>         - Clear all breakpoints at <lineno>\n");
	break;
    case CONT:
        printf("cont [sig <signo>]     - Continue execution with signal <signo>\n");
        printf("cont at <line> [sig <signo>]\n");
	printf("                       - Continue execution at line <line> with signal <signo>\n");
	break;
    case DBXENV:
        printf("dbxenv                        - Display the dbx changeable variables\n");
	printf("dbxenv case sensitive         - Treat upper and lower case distinctly\n");
	printf("dbxenv case insensitive       - Fold upper and lower case to lower case\n");

#ifdef mc68000
	printf("dbxenv fpaasm <on | off>      - Set whether FPA instruction disassembly is desired\n");
	printf("dbxenv fpabase <a[0-7] | off> - Indicate which A-register is the FPA base\n");
#endif mc68000

	printf("dbxenv makeargs <args>        - Set the arguments passed to make\n");
        printf("dbxenv speed <num>            - Set the speed of tracing execution\n");
        printf("dbxenv stringlen <num>        - Set # of chars printed for `char *'s\n");
	break;
    case DEBUG:
	printf("debug                  - Print the name and args of the program being debugged\n");
        printf("debug prog [core]      - Begin debugging <prog>\n");
	printf("                         An arbitrary process may be debugged by specifying\n");
	printf("                         its process-id instead of <core>\n");
	break;
    case DELETE:
        printf("delete <number> ...    - Remove trace's, when's, or stop's of given number(s)\n");
	printf("delete all             - Remove all trace's, when's, and stop's\n");
	break;
    case DETACH:
	printf("detach                 - Detach the debuggee from the debugger\n");
	break;
    case DISPLAY:
	printf("display                - Print the list of expressions being displayed\n");
        printf("display <exp>, ...     - Display the value of expressions <exp>, ... at every\n");
	printf("                         stopping point\n");
        break;
    case DOWN:
	printf("down                   - Move down the call stack one level\n");
	printf("down <number>          - Move down the call stack <number> levels\n");
	break;
    case DUMP:
	printf("dump                   - Print all variable local to the current procedure\n");
        printf("dump <proc>            - Print all variables local to <proc>\n");
	break;
    case EDIT:
	printf("edit                   - Edit the current file\n");
        printf("edit <filename>        - Edit the specified file <filename>\n");
        printf("edit <proc>            - Edit the file containing function or procedure <proc>\n");
	break;
    case FILE:
	printf("file                   - Print the name of the current file\n");
        printf("file <filename>        - Change the current file\n");
	break;
    case FUNC:
	printf("func                   - Print the name of the current function\n");
        printf("func <proc>            - Change the current function to function\n");
	printf("                         or procedure <proc>\n");
	break;
    case HELP:
	printf("help		       - Print a summary of all commands\n");
        printf("help <cmd>             - Print help about <cmd>\n");
	break;
    case IGNORE:
	printf("ignore                 - Print a list of the ignored signals\n");
        printf("ignore <num> <num>...  - Ignore signal(s) numbered <num>\n");
        printf("ignore <sig> <sig>...  - Ignore signal(s) named by <sig>\n");
	break;
    case LIST:
	printf("list                   - List 10 lines\n");
        printf("list <first>, <last>   - List source lines from <first> to <last>\n");
	printf("list <proc>            - List the source to <proc>\n");
	break;
    case KILL:
        printf("kill	               - Kill the debuggee\n");
	break;
    case MAKE:
	printf("make                   - invokes `make' for the program being debugged\n");
	break;
    case SOURCES:
	printf("modules                           - list object files for which debugging\n"); 
	printf("                                    information is available\n");
	printf("modules select <all|obj> <obj>..  - set or display a selection list\n");
	printf("modules append <obj> <obj>..      - extend a selection list\n");
	break;
    case MENU:
        printf("menu <seltype> <cmd>   - Define an item in the buttons menu (dbxtool)\n");
	pr_seltypes();
	break;
    case NEXT:
        printf("next                   - Step one line (skip OVER calls)\n");
        printf("next <n>               - Step <n> lines (skip OVER calls)\n");
	break;
    case NEXTI:
	printf("nexti                  - Step one machine instruction (skip OVER calls)\n");
	printf("nexti <n>              - Step <n> machine instructions (skip OVER calls)\n");
	break;
    case RERUN:
	printf("rerun                  - Begin execution of the program with no arguments\n");
	printf("rerun <args>           - Begin execution of the program with new arguments\n");
	break;
    case RUN:
	printf("run                    - Begin execution of the program with the current arguments\n");
        printf("run <args>             - Begin execution of the program with new arguments\n");
	break;
    case PRINT:
        printf("print <exp>, ...       - Print the value of the expression(s) <exp>, ...\n");
	break;
    case PROC:
	printf("proc                   - No longer supported in dbx.\n");
	break;

    case PWD:
	printf("pwd                    - Print the current Working Directory\n");
	break;

    case QUIT:
        printf("quit                   - Quit, exit dbx\n");
	break;

    case SET81:
#ifdef mc68000
    printf("set81 <fpreg> = <int> <int> <int>\n");
    printf("                       - Set 68881 register <fpreg> to a bit pattern\n");
#else
    printf("set81 is not available on this architecture.\n"); 
#endif mc68000
    break;

    case SETENV:
	printf("setenv <name> <string> - Set environment variable <name> to <string>\n");
	break;
    case SH:
        printf("sh                     - Get an interactive shell\n");
        printf("sh <shell command>     - Execute a shell command. eg: sh ls -l\n");
	break;
    case SOURCE:
	printf("source <filename>      - Execute dbx commands from file <filename>\n");
	break;
    case STATUS:
        printf("status                 - Print trace's, when's, and stop's in effect\n");
        printf("status > <filename>    - The same, but redirect output into file <filename>\n");
        printf("                         eg: status > foo\n");
	break;
    case STEP:
        printf("step                   - Single step one line (step INTO calls)\n");
        printf("step <n>               - Single step <n> lines (step INTO calls)\n");
	break;
    case STEPI:
	printf("stepi                  - Single step one machine instruction (step INTO calls)\n");
	printf("stepi <n>              - Single step <n> machine instructions (step INTO calls)\n");
	break;
    case STOP:
        printf("stop at <line> [if <cond>]  - Stop execution at the line\n");
        printf("stop in <proc> [if <cond>]  - Stop execution when <proc> is called\n");
        printf("stop <var> [if <cond>]      - Stop when value of <var> changes\n");
	printf("                              NOTE: Optional [if <cond>] causes execution to\n");
	printf("                                    stop only if condition <cond> is true\n");
	printf("                                    when appropriate stopping point is reached\n");
	printf("                              eg:   stop at 100 if i == 5\n");
        printf("stop if <cond>              - Stop if condition true\n");
	break;
    case STOPI:
	printf("stopi at <addr> [if <cond>] - Stop execution at location <addr>\n");
	printf("stopi <var> [if <cond>]     - Stop execution when value of <var> changes\n");
	printf("                              NOTE: Optional [if <cond>] causes execution to\n");
	printf("                                    stop only if condition <cond> is true\n");
	printf("                                    as appropriate stopping point is reached\n");
	printf("                              eg:   stopi at 0x1017 if i == 5\n");
        printf("stopi if <cond>             - Stop if condition true\n");
	break;
    case TOOLENV:
        printf("toolenv                   - Display dbxtool's changeable state\n");
        printf("toolenv font <file>       - Set dbxtool's font\n");
        printf("toolenv width <num>       - Set dbxtool's width in characters\n");
        printf("toolenv srclines <num>    - Set num of lines in source subwindow\n");
        printf("toolenv cmdlines <num>    - Set num of lines in command subwindow\n");
        printf("toolenv displines <num>   - Set num of lines in display subwindow\n");
        printf("toolenv topmargin <num>   - Set top margin in source subwindow\n");
        printf("toolenv botmargin <num>   - Set bottom margin in source subwindow\n");
	break;
    case TRACE:
        printf("trace [if <cond>]                   - Trace each source line\n");
        printf("trace in <proc> [if <cond>]         - Trace each source line while in proc\n");
        printf("trace <line#> [if <cond>]           - Trace execution of the line\n");
        printf("trace <proc> [if <cond>]            - Trace calls to the procedure\n");
        printf("trace <exp> at <line#> [if <cond>]  - Print <exp> when <line> is reached\n");
        printf("trace <var> [in <proc>] [if <cond>] - Trace changes to the variable\n");
	printf("                                    - NOTE: Optional <if cond> causes tracing\n");
	printf("                                            to be performed only when\n");
	printf("                                            condition <cond> is true as trace\n");
	printf("                                            point is reached\n");
	printf("                                      eg:   trace 100 if i == 5\n");
	break;
    case TRACEI:
	printf("tracei at <addr> [if <cond>]        - Trace execution of location <addr>\n");
	printf("tracei <var> [at <addr>] [if <cond>]- Trace changes to <var> at <addr>\n");
	printf("                                    - NOTE: Optional <if cond> causes tracing\n");
	printf("                                            to be performed only when condition\n");
	printf("                                            <cond> is true as trace point is\n");
	printf("                                            reached\n");
	printf("                                      eg:   tracei 0x1017 if i == 5\n");
	break;
    case UNBUTTON:
        printf("unbutton <cmd>         - Remove all buttons containing <cmd> (dbxtool)\n");
	break;
    case UNDISPLAY:
        printf("undisplay <exp>, ...   - Undo the effect of the `display' command\n");
	break;
    case UNMENU:
        printf("unmenu <cmd>           - Remove all menu items containing <cmd> (dbxtool)\n");
	break;
    case UP:
	printf("up                     - Move up the call stack one level\n");
	printf("up <number>            - Move up the call stack <number> levels\n");
	break;
    case USE:
        printf("use                    - Print the directory search path\n");
        printf("use <dir> ...          - Change the directory search path\n");
	break;
    case WHATIS:
        printf("whatis <name>          - Print the declaration of the <name>\n");
	break;
    case WHEN:
        printf("when at <line> { cmd; }- Execute command(s) when <line> reached\n");
        printf("when in <proc> { cmd; }- Execute command(s) when <proc> called\n");
        printf("when <exp> { cmd; }    - Execute command(s) when <exp> true\n");
	break;
    case WHERE:
        printf("where                  - Print a procedure traceback\n");
	printf("where <num>            - Print the <num> top procedure in the traceback\n");
	break;
    case WHEREIS:
        printf("whereis <name>         - Print all declarations of <name>\n");
	break;
    case WHICH:
        printf("which <name>           - Print full qualification of <name>\n");
	break;
    }
}

pr_seltypes()
{
    printf("\n");
    printf("where selection type, <seltype>, is:\n");
    printf("\n");
    printf("  command - Expand to a command from the command subwindow\n");
    printf("  expand  - Expand to longest syntactic identifier\n");
    printf("  ignore  - Ignore the selection\n");
    printf("  lineno  - Lineno from the source subwindow\n");
    printf("  literal - Take the selection literally\n");
}

/*
 * Print the default help message.
 * This is a summary of all the commands.
 */
pr_defhelp()
{
    printf("     Command Summary\n");
    printf("\n");
    printf("Execution and Tracing\n");
    printf("  catch     clear     cont      delete    ignore    next     rerun\n");
    printf("  run       status    step      stop      trace     when\n");
    printf("\n");
    printf("Displaying and Naming Data\n");
    printf("  assign    call      display   down      dump      print     set\n");
    printf("  set81     undisplay up        whatis    where     whereis   which\n");
    printf("\n");
    printf("Accessing Source Files\n");
    printf("  cd        edit      file      func      list      modules   pwd\n");
    printf("  use       /         ?\n");
    printf("\n");
    printf("Miscellaneous\n");
    printf("  alias     dbxenv    debug     detach    help      kill      make\n");
    printf("  quit      setenv    sh        source\n");
    printf("\n");
    printf("Dbxtool\n");
    printf("  button    toolenv   unbutton  unmenu    menu\n");
    printf("\n");
    printf("Machine Level\n");
    printf("  nexti     stepi     stopi     tracei\n");
    printf("\n");
    printf("The command `help <cmdname>' provides additional\n");
    printf("help for each command\n");
}
