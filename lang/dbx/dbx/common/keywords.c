#ifndef lint
static	char sccsid[] = "@(#)keywords.c 1.1 94/10/31 SMI"; /* from UCB X.X XX/XX/XX */
#endif

/*
 * Keyword management.
 */

#include "defs.h"
#include "keywords.h"
#include "scanner.h"
#include "names.h"
#include "symbols.h"
#include "tree.h"
#include "pipeout.h"
#include "commands.h"

#ifndef public
#include "scanner.h"
#endif

private String reserved[] ={
    "alias", "all", "and", "assign", "at", 
    "botmargin", "button", 
    "call", "case", "catch", "cd", "clear", "cmdlines", "command", "cont",
    "dbxdebug", "dbxenv", "debug", "delete", "detach",
    "display", "displines", "div", "down", "dump", 
    "edit", "enum", "expand", 
    "file", "font", "fpaasm", "fpabase", "func",
    "help", 
    "if", "ignore", "in",
    "kill", "lineno", "list", "literal", 
    "make", "makeargs", "menu", "mod", 
    "next", "nexti", 
    "or",
    "print", "psym", "pwd",
    "quit", 
    "rerun", "run",
    "set81", "setenv", "sh", "sig", "sizeof", 
    "source", "modules", "speed", "srclines", 
    "status", "step", "stepi", "stringlen", "struct",
    "stop", "stopi", 
    "toolenv", "topmargin", "trace", "tracei",
    "unbutton", "undisplay", "union", "unmenu", "up", "use", 
    "whatis", "when", "where", "whereis", "which", "width",
};

/*
 * The keyword table is a traditional hash table with collisions
 * resolved by chaining.
 */

#define KW_HASHTABLESIZE 128		/* Keyword hash table size */
#define AL_HASHTABLESIZE 32		/* Alias hash table size */

typedef struct Keyword {
    Name name;
    Token toknum : 16;
    Boolean isalias : 16;
    struct Keyword *chain;
} *Keyword;

typedef struct Alias {
    Name newname;			/* New name for old string */
    Name oldname;			/* Old name */
    struct Alias *chain;		/* Linked list */
} *Alias;

typedef unsigned int Hashvalue;

private Keyword kw_hashtab[KW_HASHTABLESIZE];
private Alias   al_hashtab[AL_HASHTABLESIZE];

#define hash(n)    ((((unsigned) n) >> 2) mod KW_HASHTABLESIZE)
#define al_hash(n) ((((unsigned) n) >> 2) mod AL_HASHTABLESIZE)

private Alias find_alias_struct();

/*
 * Enter all the reserved words into the keyword table.
 */

public enterkeywords()
{
    register Integer i;

    for (i = ALIAS; i <= WIDTH; i++) {
	keyword(reserved[ord(i) - ord(ALIAS)], i, false);
    }
    keyword("set", ASSIGN, false);
}

/*
 * Deallocate the keyword table.
 */

public keywords_free()
{
    register Integer i;
    register Keyword k, nextk;

    for (i = 0; i < KW_HASHTABLESIZE; i++) {
	k = kw_hashtab[i];
	while (k != nil) {
	    nextk = k->chain;
	    dispose(k);
	    k = nextk;
	}
	kw_hashtab[i] = nil;
    }
}

/*
 * Enter a keyword into the name table.  It is assumed to not be there already.
 * The string is assumed to be statically allocated.
 */

private keyword(s, t, isalias)
String s;
Token t;
Boolean isalias;
{
    register Hashvalue h;
    register Keyword k;
    Name n;

    n = identname(s, true);
    h = hash(n);
    k = new(Keyword);
    if (k == 0)
	fatal("No memory available. Out of swap space?");
    k->name = n;
    k->toknum = t;
    k->isalias = isalias;
    k->chain = kw_hashtab[h];
    kw_hashtab[h] = k;
}

/*
 * Return the string associated with a token corresponding to a keyword.
 */

public String keywdstring(t)
Token t;
{
    return reserved[ord(t) - ord(ALIAS)];
}

/*
 * Find a keyword in the keyword table.
 * We assume that tokens cannot legitimately be nil (0).
 */

public Token findkeyword(n)
Name n;
{
    register Hashvalue h;
    register Keyword k;
    Token t;

    h = hash(n);
    k = kw_hashtab[h];
    while (k != nil and k->name != n) {
	k = k->chain;
    }
    if (k == nil) {
	t = nil;
    } else {
	t = k->toknum;
    }
    return t;
}

/*
 * Create an alias.
 */
public enter_alias(newstr, oldstr)
String newstr;
String oldstr;
{
    Alias alias;
    Name newname;
    Name oldname;
    Hashvalue h;

    newname = identname(newstr, false);
    oldname = identname(oldstr, true);
    alias = find_alias_struct(newname);
    if (alias == nil) {
        h = al_hash(newname);
        alias = new(Alias);
	if (alias == 0)
		fatal("No memory available. Out of swap space?");
        alias->newname = newname;
        alias->oldname = oldname;
        alias->chain = al_hashtab[h];
        al_hashtab[h] = alias;
    } else {
         alias->oldname = oldname;
    }
}

/*
 * See if a name is an alias.
 * Return the new name to the outside world.
 */
public Boolean find_alias(name, buf)
Name name;
char *buf;
{
    Alias alias;
    char args[512];

    alias = find_alias_struct(name);
    if (alias != nil) {
	get_restofline(args);
	expand_alias(idnt(name), args, idnt(alias->oldname), buf);
	return(true);
    }
    return(false);
}

/*
 * See if a name is an alias.
 */
private Alias find_alias_struct(name)
Name name;
{
   Alias alias;
   Hashvalue h;
	       
   h = al_hash(name);
   alias = al_hashtab[h];
   while (alias != nil) {
       if (name == alias->newname) {
	   return(alias);
       }
       alias = alias->chain;
   }   
   return(nil);
}

/*
 * Print out an alias.
 */

public print_alias(str)
String str;
{
    Alias alias;
    Name n;
    Integer i;

    if (str != nil) {
	n = identname(str, false);
	alias = find_alias_struct(n);
	if (alias != nil) {
	    escape_string(idnt(alias->oldname));
	    printf("\n");
	} else {
	    error("Alias `%s' not found", str);
	}
    } else {
        for (i = 0; i < AL_HASHTABLESIZE; i++) {
	    for (alias = al_hashtab[i]; alias != nil; alias = alias->chain) {
	        if (isredirected()) {
  	            printf("alias ");
    	        }
	        printf("%s\t\"", idnt(alias->newname));
		escape_string(idnt(alias->oldname));
		printf("\"\n");
	    }
	}
    }
}

/*
 * Print the value of an alias and escape any double quotes.
 */
private escape_string(str)
char	*str;
{
	char	*cp;

	for (cp = str; *cp; cp++) {
		if (*cp == '"') {
			putchar('\\');
		}
		putchar(*cp);
	}
}
