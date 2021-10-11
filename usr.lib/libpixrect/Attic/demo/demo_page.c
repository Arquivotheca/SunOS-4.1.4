#ifndef lint
static	char sccsid[] = "@(#)demo_page.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * demo_page file
 */
#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/pixfont.h>

char	buf[1000000];
int	line;
int	length;

main(argc, argv)
	char **argv;
{
	register int i, fd, len;
	register char *cp;
	register struct pixrect *screen;
	register PIXFONT *pf;

	argc--, argv++;
	if (argc != 1) {
		fprintf(stderr, "Usage: demo_page file\n");
		exit(1);
	}
	screen = (struct pixrect *)bw1_create(1024, 800, 1);
	if (screen == 0) {
		fprintf(stderr, "Can't open frame buffer (/dev/console)\n");
		exit(1);
	}
	fd = open(argv[0], 0);
	if (fd < 0) {
		perror(argv[0]);
		exit(1);
	}
	len = read(fd, buf, 1000000);
	buf[len] = 0;
	pf = pf_open(0);
	pr_rop(screen, 0, 0, screen->pr_size.x, screen->pr_size.y,
	    PIX_DONTCLIP|PIX_CLR, 0, 0, 0);
	cp = buf;
	line = 1;
	while (*cp) {
		char obuf[4096];
		register char *bp = obuf;
		register int leadin = 0, c;

		while (*cp == '\t') {
			leadin += 8;
			cp++;
		}
/*
		pr_rop(screen, 16, line * pf->pf_defaultsize.y - 15,
		    leadin * pf->pf_defaultsize.x,
		    pf->pf_defaultsize.y, PIX_DONTCLIP|PIX_CLR, 0, 0, 0);
*/
		while ((c = *cp) && c != '\n') {
			if (c == '\t') {
				int pos = bp - obuf + 1;
				*bp++ = ' ';
				while (pos & 7)
					*bp++ = ' ', pos++;
			} else
				*bp++ = c;
			cp++;
		}
		if (leadin + bp - obuf > length)
			length = leadin + bp - obuf;
		*bp = 0;
		pf_text(screen,
		    16 + leadin * pf->pf_defaultsize.x,
		    line * pf->pf_defaultsize.y,
		    PIX_DONTCLIP|PIX_SRC, pf, obuf);
		line++;
		if ((line + 3) * pf->pf_defaultsize.y >= screen->pr_size.y) {
			pr_rop(screen, 16, 0,
			    length * pf->pf_defaultsize.x, screen->pr_size.y,
			    PIX_DONTCLIP|PIX_CLR, 0, 0, 0);
			line = 1;
			length = 0;
		}
		if (*cp)
			cp++;
	}
}
