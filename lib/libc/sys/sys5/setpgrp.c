#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setpgrp.c 1.1 94/10/31 SMI";
#endif

extern int	setsid();

int
setpgrp()
{

	return (setsid());
}
