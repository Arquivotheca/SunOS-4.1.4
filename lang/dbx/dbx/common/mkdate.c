
#ifndef lint
static char sccsid[] = "@(#)mkdate.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

#include <stdio.h>
#include <sys/time.h>

main()
{
    struct tm *t;
    long clock;
    char name[100];
    int namelen;

    printf("char *date = \"VERSION @(#) ");
    clock = time(0);
    t = localtime(&clock);
    printf("%d/%d/%d ", t->tm_mon + 1, t->tm_mday, t->tm_year % 100);
    printf("%d:%02d", t->tm_hour, t->tm_min);
    gethostname(name, &namelen);
    printf(" (%s)", name);
    printf("\";\n");
    exit(0);
}
