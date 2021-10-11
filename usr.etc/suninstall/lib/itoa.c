/*      @(#)itoa.c 1.1 94/10/31 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

static char s[25];

char *
itoa(n)
{
        char prbuf[11];
        register char *cp, *str;

	str = s;
        cp = prbuf;
        do {
                *cp++ = "0123456789"[n%10];
                n /= 10;
        } while (n);
        do {
                *str++ = *--cp;
        } while (cp > prbuf);
        return (str);
}
