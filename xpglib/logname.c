#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)logname.c 1.1 94/10/31 SMI";
#endif

#define NAME "LOGNAME"

extern char *getenv();


char *logname()
{
	return (getenv(NAME));
}
