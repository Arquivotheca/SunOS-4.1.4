#ifndef lint
static	char sccsid[] = "@(#)output.c 1.1 94/10/31 SMI";
#endif

/*
 *	Microassembler output routines
 */

#include "micro.h"
#include <sys/file.h>
#include <stdio.h>

extern NODE *curnode, n[];
extern NODE *adr[];
extern SYMBOL *shash[];
FILE *binfile;

alpha_out(){
    /* print listing */
    register NODE *nd;
    register int i;
    Boolean any_undef = False;
    SYMBOL *sp;

#ifdef VIEW
    printf("\nSun Microsystems GP-1 Viewing Processor Microassembler -- Version 1.2 (26 October 1984) -- Company Confidential\n");
#endif
#ifdef PAINT
    printf("\nSun Microsystems GP-1 Painting Processor Microassembler -- Version 1.2 (26 October 1984) -- Company Confidential\n");
#endif
    for (i = 0; i < NHASH; i++) {
	sp = shash[i];
	while (sp != 0) {
	    if (!sp->defined) {
		if (!any_undef) {
		    any_undef = True;
		    error("ERROR:  undefined symbols:\n");
		}
		printf("     %s\n",sp->name);
	    }
	    sp = sp->next_hash;
	}
    }
    if (any_undef) {
	printf("\n");
	return;
    }

#ifdef VIEW
    printf("\n   addr  fl mv s b d  alu  var  source line\n\n");
#endif
#ifdef PAINT
    printf("\n   addr  fl mv s b d  alu  gen  source line\n\n");
#endif
    for( nd = n; nd <=curnode; nd++) {
	if (nd->org_pseudo) {
	    printf("                                %s",nd->line);
	} else if (nd->sccsid) {
	    printf("%7x  ",nd->addr);
	    printf("%04x%04x%04x%02x         ",nd->word1,nd->word2,nd->word3,
		(unsigned) nd->word4 >> 8);
	    printf("%s", nd->line != 0 ? nd->line : "\n");
	} else if (nd->filled) {
	    printf("%7x  ",nd->addr);
	    printf("%02x %02x %01x %01x %01x %04x %04x  ",
#ifdef VIEW
		nd->word1 >> 10,
		(nd->word1 >> 4) & 0x3f,
#endif
#ifdef PAINT
		nd->word1 >> 11,
		(nd->word1 >> 4) & 0x7f,
#endif
		nd->word1 & 0xf,
		nd->word2 >> 12,
		(nd->word2 >> 8) & 0xf,
		(unsigned short) (nd->word2 << 8) | (nd->word3 >> 8),
		(unsigned short) (nd->word3 << 8) | (nd->word4 >> 8));
	    printf("%s", nd->line);
	}
    }
    printf("\n\n");
}

FILE *
open_a_file( s )
    char *s;
{
    register int i;
    i = creat( s, O_RDWR );
    if (i<0){
	perror( s );
	fatal("could not create output file");
    }
    return fdopen( i, "w");
}

write_binary()
{
    register NODE *ni, *nj, *nk;
    short word, njd;

    for( ni = n; ni <=curnode; ni++) {
	if (ni->filled) {
	    for (nj = ni + 1; nj <= curnode+1; nj++) {
		if (!nj->filled || nj->addr != (nj-1)->addr + 1) {
		    fwrite(&ni->addr,sizeof(ni->addr),1,binfile);
		    njd = nj - ni;
		    fwrite(&njd,sizeof(njd),1,binfile);
		    for (nk = ni; nk < nj; nk++) {
			word = ((nk->word4 & 0xff) << 8)
			    | ((nk->word1 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
			word = ((nk->word1 & 0xff) << 8)
			    | ((nk->word2 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
			word = ((nk->word2 & 0xff) << 8)
			    | ((nk->word3 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
			word = ((nk->word3 & 0xff) << 8)
			    | ((nk->word4 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
		    }
		    break;
		}
	    }
	    if (nj->filled) {
		ni = nj - 1;
	    } else {
		ni = nj;
	    }
	}
    }
}

/*write_binary()
{
    short i, j, k, word, ji;

    for( i = 0; i < curnode - n; i++){
	if (n[i].filled) {
	    for (j = i+1; j < curnode - n; j++) {
		if (n[j].addr-n[j-1].addr != 1) {
		    fwrite(&n[i].addr,sizeof(n[i].addr),1,binfile);
		    ji = j - i;
		    fwrite(&ji,sizeof(ji),1,binfile);
		    for (k = i; k < j; k++) {
			word = ((n[k].word4 & 0xff) << 8)
			    | ((n[k].word1 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
			word = ((n[k].word1 & 0xff) << 8)
			    | ((n[k].word2 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
			word = ((n[k].word2 & 0xff) << 8)
			    | ((n[k].word3 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
			word = ((n[k].word3 & 0xff) << 8)
			    | ((n[k].word4 & 0xff00) >> 8);
			fwrite(&word,sizeof(word),1,binfile);
		    }
		    break;
		}
	    }
	    i = j;
	}
    }
}
*/

output(out_stream)
FILE * out_stream;
{
    /* there are two parts to the output writing -- the listing and the binary */
    alpha_out();
    if (out_stream == 0) {
	binfile = open_a_file( "m.out" );
    } else {
	binfile = out_stream;
    }
    write_binary();
    fclose( binfile );
}
