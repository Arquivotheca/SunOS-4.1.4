/*	@(#)library.h 1.1 94/10/31 SMI; from UCB X.X XX/XX/XX	*/

#ifndef library_h
#define library_h
#ifndef lint
#endif
#define ERR_IGNORE ((INTFUNC *) 0)
#define ERR_CATCH  ((INTFUNC *) 1)
typedef int INTFUNC();
int call(/* name, in, out, args */);
int back(/* name, in, out, args */);
int callv(/* name, in, out, argv */);
int backv(/* name, in, out, argv */);
shell(/* s */);
ptraced(/* pid */);
unptraced(/* pid */);
pwait(/* pid, statusp */);
syserr(/*  */);
catcherrs(/*  */);
INTFUNC *onsyserr(/* n, f */);
int sys_nsig ;
String sys_siglist[] ;
psig(/* s */);
beginerrmsg(/*  */);
enderrmsg(/*  */);
warning(/* s, a, b, c, d, e, f, g, h, i, j, k, l, m */);
error(/* s, a, b, c, d, e, f, g, h, i, j, k, l, m */);
fatal(/* s, a, b, c, d, e, f, g, h, i, j, k, l, m */);
panic(/* s, a, b, c, d, e, f, g, h, i, j, k, l, m */);
erecover(/*  */);
quit(/* r */);
int cmp(/* s1, s2, n */);
mov(/* src, dest, n */);
char *strdup();
#endif
