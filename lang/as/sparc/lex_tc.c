static char sccs_id[] = "@(#)lex_tc.c	1.1\t10/31/94";
#include <stdio.h>
#include "structs.h"
#include "y.tab.h"
/** #include "globals.h" **/

#define MAXTOKENS 512
char *(token_name[MAXTOKENS]) = { NULL };
long int tkn_count[MAXTOKENS];

#define MAX_LO_TOKENS 16
char *(lo_token_name[MAX_LO_TOKENS]) = { NULL };
long int lo_tkn_count[MAX_LO_TOKENS];
# define LOT_BAD	0
# define LOT_WHT	1
# define LOT_CMT	2
# define LOT_BOI	3
# define LOT_BOL	4
# define LOT_BOS	5
# define LOT_BOF1	6
# define LOT_BOF2	7
# define LOT_EOF	8

Bool debug = FALSE;


main(argc, argv)
	int argc;
	char *argv[];
{
	FILE	*yfp;
	char	line[100];
	char	ch;
	char	pp_directive[100];
	char	name[100];
	int	token;
	int	max_token;
	long int total_count;
	long int pct;
	extern	char *malloc();

	if ( (argc > 1) && ((*argv[1]=='-') && (*(argv[1]+1)=='D')) )
	{
		debug = TRUE;
		argc--;
		argv++;
	}

	/*
	 * load up the token names
	 */

	/* 32 - 126 is ' ' - '~' */
	for (token = 32;   token < 127;    token++)
	{
		token_name[token] = malloc(4);
		sprintf(token_name[token], "'%c'", (char)token);
	}

	DEBUG("[opening y.tab.h]\n");
	yfp = fopen("y.tab.h", "r");
	for (max_token = 127;   fgets(line, 100, yfp) != NULL;    ) 
	{
		sscanf(line, "%c %s %s %d",
			&ch, &pp_directive[0], &name[0], &token);
		if ( (ch == '#') && (strcmp("define", pp_directive) == 0) )
		{
			if (token > max_token)	max_token = token;
			token_name[token] = malloc(strlen(name)+1);
			strcpy(token_name[token], name);
			DEBUG("\t[adding \"%s\" as %d]\n",
				token_name[token], token);
		}
	}
	fclose(yfp);
	DEBUG("[y.tab.h closed]\n");

	lo_token_name[LOT_BAD]  = "LEXONLY:BAD";
	lo_token_name[LOT_WHT]  = "LEXONLY:WHT";
	lo_token_name[LOT_CMT]  = "LEXONLY:CMT";
	lo_token_name[LOT_BOI]  = "LEXONLY:BOI";
	lo_token_name[LOT_BOL]  = "LEXONLY:BOL";
	lo_token_name[LOT_BOS]  = "LEXONLY:BOS";
	lo_token_name[LOT_BOF1] = "LEXONLY:BOF1";
	lo_token_name[LOT_BOF2] = "LEXONLY:BOF2";
	lo_token_name[LOT_EOF]  = "LEXONLY:EOF";

	/* setup input from stdin */
	if ( argc <= 1 )
	{
		/* no args;  use default file */
		if ( freopen("sras.tkn_count", "r", stdin) == NULL )
		{
			fprintf(stderr, "Can't open file \"sras.tkn_count\"\n");
			exit(1);
		}
	}
	else /* argc > 1 */
	{
		if (strcmp(argv[1], "-") == 0)
		{
			/* use stdin as input */
		}
		else if ( freopen(argv[1], "r", stdin) == NULL )
		{
			fprintf(stderr, "Can't open file \"%s\"\n", argv[1]);
			exit(1);
		}
	}

	/* read it */
	read(fileno(stdin), &tkn_count[0], sizeof(tkn_count));
	read(fileno(stdin), &lo_tkn_count[0], sizeof(lo_tkn_count));

	/* accumulate the total count */
	for(total_count = 0, token = 0;   token < max_token;   token++)
	{
		total_count += tkn_count[token];
	}

	/* print the results */
	for(token = 0;   token < max_token;   token++)
	{
		if ( (token > 256) || (tkn_count[token] > 0) )
		{
			if (total_count == 0)	pct = 0;
			else
			{
				pct = (tkn_count[token] *1000L) / total_count;
				pct = (pct + 5) / 10;	/* round it */
			}

			fprintf(stdout, "%3d\t%-14s\t%4d\t%2d%%\n", token,
				token_name[token], tkn_count[token], pct);
		}
	}

	fprintf(stdout, "\t%-14s\t%4ld\n\n", "--- total ---", total_count);

	/* print the lexonly token counts */
	for(token = 0;   token < MAX_LO_TOKENS;   token++)
	{
		if ( lo_tkn_count[token] > 0 )
		{
			fprintf(stdout, "\t%-14s\t%4d\n",
				lo_token_name[token], lo_tkn_count[token]);
		}
	}
}


DEBUG(fmt, a1, a2, a3, a4)
	char *fmt;
	int a1, a2, a3, a4;
{
	if(!debug)	return;

	fprintf(stdout, fmt, a1, a2, a3, a4);
}
