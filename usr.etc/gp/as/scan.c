#ifndef lint
static	char sccsid[] = "@(#)scan.c 1.1 94/10/31 SMI";
#endif

/*
 *	Microassembler scanner for GP/1
 *		scan.c	1.1	1 January 1985
 */

#include "micro.h"
#include <stdio.h>
#include <ctype.h>


#define scanb(c)  while(isspace(c = *curpos)) curpos++
#define nextc()     *++curpos
#define peekc()     *curpos
#define LP	'['
#define RP	']'
#define ELSE	':'
#define MAXSYMBOL 256
#define MAXLINE 1024
#define SHAREDMEMPTR 1

int curlineno;
char *curfilename;
char *curpos;
char *curline;
char inputline[MAXLINE];

/* The following 2 lines added by PWC to count lines of ucode. */
extern NODE n[NNODE];
extern NODE *curnode;
extern int curaddr;

extern Boolean asrcdest();
extern Boolean aseq();
extern Boolean acontrol();
#ifdef VIEW
extern Boolean almode();
extern Boolean aflt();
#endif

Boolean
newline(){
    /* glom a new input line.
     * clobber comments. do
     * continuation-line collection, as well.
     */
    register char *  maxpos = inputline + sizeof inputline - 1;
    register char *cp;
    char *end;
    register c;
    Boolean barseen = False;
    int len;

    cp = inputline;
restart:
    curlineno++;
    do {
	c = getchar();
	if (c == '|') barseen = True;
	*cp++ = c;
    } while ( c != '\n' && c != EOF && cp < maxpos );
    if ( c == EOF ) return False;
    end = cp;
    *cp = '\0';
    if (cp == maxpos){
	error("Sorry, line(s) too long");
	goto process;
    }
    if (!barseen) {
	/* line may be continued -- search backwards for continuation */
	cp -= 1; /* back up over \0 */
	while (isspace(*cp) && cp >= inputline) cp--;
	if (*cp == '&')
	    /* splice on new line right here */
	    goto restart;
    }
process:
    curpos = inputline;
    len = end-inputline;
    curline = (char*)malloc(len+2);
    if (curline == 0) {
	error("unable to get sufficient storage\n");
	return False;
    }
    strcpy( curline, inputline );
    return True;
}

/*	instruction fields are collected here  */
# define NOPR 5
OPERAND oprs[ NOPR ];
int noprs;

char realc;

char *
scansym( )
{
    static char stuff[MAXSYMBOL];
    register char *cp;
    register char c;
    Boolean ok = True;

    cp = stuff;
    curpos -= 1; /*back up to get a running start*/
    while( ok ){
	while (isalnum(*cp++ = c = nextc()));
	switch(c){
	case '.':
	case '_':
	case '\\':
	    continue;
	default:
	    ok = False;
	}
    }
    *--cp = '\0';
    return( stuff );
}


int
getnum( )
{
    register char c = *curpos;
    register int val, base;
    int sign;

    sign = 1;
    if (c == '-') {
	sign = -1;
	c = nextc();
    } else if (c == '+') {
	c = nextc();
    }
    val = c - '0';
    base = (val==0)? 010 : 10;
    c = nextc();
    if ( val == 0 && c == 'x' ){
	base = 0x10;
	c = nextc();
    }
    while(isxdigit(c)){
	val *= base;
	if( isdigit(c) )
	    val += c -'0';
	else if (islower(c))
	    val += c -'a' + 10;
	else
	    val += c -'A' + 10;
	c = nextc();
    }
    if (sign == -1) {
	val = -val;
    }
    return( val );
}

/* parse an operand list */
Boolean
operands()
{
    register char c;
    char *s;
    static char oprch[MAXSYMBOL];

    scanb( c );
    for(noprs = 0; noprs < NOPR; noprs++){
	switch(c){ 
	case ';':
	    nextc();
	case '\0':
	    return True;
	}
	if (c == '=') {
	    c = nextc();
	    scanb(c);
	    strcpy(oprch,scansym());
	    oprs[noprs].r = SYM;
	    oprs[noprs].v = (long) oprch;
	} else if (isdigit(c) || c == '-') {
	    /* it's a number */
	    oprs[noprs].r = Z;
	    oprs[noprs].v = getnum( );
	    if (oprs[noprs].v != 0) {
		oprs[noprs].r = IMM;
	    }
	} else if ( isalpha(c)) {
	    /* it's a reserved word */
	    RESERVED *rp;

	    rp =  resw_lookup( s=scansym( ));
	    oprs[noprs].v = 0;
	    if (rp == 0 || rp->type != Operand) {
		error("found \"%s\" while looking for an operand", s);
		return False;
	    } else if (rp->value1 == RAM) {
		/* pick up subscript */
		scanb(c);
		if( c != LP ) goto yeucch;
		nextc();
		scanb(c);
		oprs[noprs].v = getnum(  );
		scanb(c);
		if(c != RP ) goto yeucch;
		c = nextc();
	    }
	    oprs[noprs].r = rp->value1;
	} else {
yeucch:
	    error("operand syntax -- choked on a %c", c);
	    return False;
	}
	scanb(c);
	switch(c){
	case ',':
	    nextc(); /*skip the comma*/
	    scanb(c);
	case ';':
	case '\0':
	    break;
	default:
	    error("unexpected character (%c) in  operands", c);
	    return False;
	}
    }
    error("too many operands");
    return False;
}
    
Boolean
alupart(name)
    char *name;
{
    static char *nopname = "nop";
    register RESERVED *rp, *snp;
    char c;
    int sc;
    
    if (name == 0) goto mtalu;
    scanb(c);
    if (curnode->imm_29116) {
	error("instruction slot following 29116 immediate must be empty",0);
    }
    rp = resw_lookup(name);
    if (rp == 0 || rp->type != Aluop) {
	error("found \"%s\" while looking for an Am29116 opcode", name );
	return False;
    }
    scanb(c);
    if (c == ',') {
	c = nextc();
	scanb(c);
	snp = resw_lookup(name = scansym());
	sc = 0;
	if (snp == 0 || snp->type != Completer) {
	    error("found \"%s\" while looking for an Am29116 completer",
		name );
	} else {
	    sc = snp->value1;
	}
    }
    if ( operands() == False ) {
	/* there was an error -- propagate */
	return False;
    }
    assemble(rp,sc,noprs,oprs);
    return True;

mtalu:
    c = nextc();
    if (curnode->imm_29116) {
	return True;
    } else {
	assemble(resw_lookup(nopname),0,0,0);
	return True;
    }
}


Boolean
srcdestpart(  )
{
    static char srcch[MAXSYMBOL];
    register RESERVED *rp;
    register char c;
    char *cp, *name;
    SYMTYPE gentype;
    int source, dest, num;

    /* get srcdest part, if any */
    /* syntax is:
	;     (empty field)
	source->destination;
    */
    scanb(c);
    switch(c){
    case ';':
	nextc();
    case '\0':
	return asrcdest(-1,-1,NEITHER,0,0);
    }
    c = peekc();
    if (c == '=') {
	c = nextc();
	scanb(c);
	strcpy(srcch,scansym());
	source = GENERAL;
	gentype = ALPHA;
    } else if (isdigit(c) || c == '-') {
	num = getnum();
	source = GENERAL;
	gentype = NUMBER;
    } else {
	rp = resw_lookup(cp=scansym( ));
	if (rp == 0 || rp->type != Srcdest || rp->value2 == 2) {
	    error("found \"%s\" while looking for source", cp);
	    return False;
	} else {
	    source = rp->value1;
	}
    }
    scanb(c);
    /* found source -- now find -> */
    if (c != '-') goto botch;
    c = nextc();
    if (c != '>') goto botch;
    /* get destination */
    nextc();
    scanb(c);
    rp = resw_lookup( cp=scansym( ) );
    if ( rp == 0 || rp->type != Srcdest  || rp->value2 == 1){
	error("found \"%s\" while looking for destination", cp);
	return False;
    }
    dest = rp->value1;
    scanb(c);
    if (c == ';') {
	nextc();
    } else {
	goto botch;
    }
    return asrcdest(source,dest,gentype,num,srcch);

botch:
    error("source/destination syntax error -- choked on character %c", c);
    return False;
}

Boolean
seqpart()
{
    char *name;
    Boolean notflag;
    register RESERVED *rp;
    register RESERVED *ccp;
    char c;
    SYMTYPE which;
    int num, offset;
    char *sym;
    
    /* scan for an operator and optional branch condition, then scan the
       operand */

    scanb(c);
    switch(c){
    case ';':
	nextc();
    case '\0':
	return aseq(0,0,0,0,0,0);
    }
    rp = resw_lookup(name = scansym());
    if (rp == 0 || rp->type != Seqop){
	error("found \"%s\" while looking for an Am2910 operator", name );
	return False;
    }
    notflag = False;
    ccp = (RESERVED *)0;
    scanb(c);
    if (c == ',') {
	if ((rp->value2 % 2) == 0) {
	    error("2910 operator %s does not allow condition",name);
	    return False;
	}
	c = nextc();
	scanb(c);
	if (c == '~') {
	    nextc();
	    notflag = True;
	}
	ccp = resw_lookup(name = scansym());
	if (ccp == 0 || ccp->type != Cc){
	    error("found \"%s\" while looking for a condition code", name );
	    return False;
	}
	if (notflag && ccp->value2 != 0) {
	    error("condition \"%s\" cannot be negated",name);
	    return False;
	}
    }
    scanb(c);
    which = NEITHER;
    if (rp->value2 >= 2) {
	if (isdigit(c) || c == '-') {
	    which = NUMBER;
	    num = getnum( );
	    scanb(c);
	} else if (isalpha(c)) {
	    which = ALPHA;
	    sym = scansym( );
	    scanb(c);
	} else if (c == '.') {
	    which = NUMBER;
	    offset = 0;
	    c = nextc();
	    scanb(c);
	    if (c == '+') {
		c = nextc();
		scanb(c);
		offset = getnum();
		scanb(c);
	    } else if (c == '-') {
		c = nextc();
		scanb(c);
		offset = -getnum();
		scanb(c);
	    }
    	    num = curaddr - 1 + offset;
	}
    }
    if (c == ';') {
	nextc();
    } else {
	goto botch;
    }
    return aseq(rp,notflag,ccp,which,num,sym);

botch:
    error("Am2910 syntax error -- choked on character %c", c);
    return False;
}

#ifdef VIEW
Boolean
lmode_operands( )
{
    RESERVED *lop;
    char c;
    char *s, *name;
    int round, inf, mode, code;

    scanb( c );
    lop = resw_lookup(name = scansym());
    if (lop == 0 || lop->type != Wround) {
	goto wbotch;
    }
    round = lop->value1;
    scanb(c);
    if (c != ',') {
	goto botch;
    }
    c = nextc();
    scanb(c);
    lop = resw_lookup(name = scansym());
    if (lop == 0 || lop->type != Winf) {
	goto wbotch;
    }
    inf = lop->value1;
    scanb(c);
    if (c != ',') {
	goto botch;
    }
    c = nextc();
    scanb(c);
    lop = resw_lookup(name = scansym());
    if (lop == 0 || lop->type != Wmode) {
	goto wbotch;
    }
    mode = lop->value1;
    scanb(c);
    if (c != ',') {
	goto botch;
    }
    c = nextc();
    scanb(c);
    lop = resw_lookup(name = scansym());
    if (lop == 0 || lop->type != Wcode) {
	goto wbotch;
    }
    code = lop->value1;
    scanb(c);
    if (c != ';') {
	goto botch;
    }
    c = nextc();
    noprs = 3;
    return almode(round,inf,mode,code);
botch:
    error("lmode operands syntax error -- choked on character %c", c);
    return False;
wbotch:
    error("found \"%s\" while looking for an lmode operand",name);
    return False;
}

Boolean
fltpart(name)
    char *name;
{
    register RESERVED *rp, *comp;
    int load, unld, store, select;
    char c;

    /* scan for an operator and optional completers */

    scanb(c);
    switch(c){
    case ';':
	nextc();
    case '\0':
	return aflt(0,0,0,0,0);
    }
    rp = resw_lookup(name = scansym());
    if (rp == 0 || rp->type != Wop){
	error("found \"%s\" while looking for a Weitek operator", name );
	return False;
    }
    if (rp->value1 == -1) {
	return lmode_operands( );
    }  else {
	load = 0;
	unld = 0;
	store = 0;
	select = 0;
	scanb(c);
	if (c == ',') {
	    c = nextc();
	    scanb(c);
	    comp = resw_lookup(name = scansym());
	    if (comp == 0) goto wbotch;
	    if (comp->type == Wload) {
		load = comp->value1;
	    } else {
		goto tryunld;
	    }
	}
	scanb(c);
	if (c == ',') {
	    c = nextc();
	    scanb(c);
	    comp = resw_lookup(name = scansym());
	    if (comp == 0) goto wbotch;
tryunld:    if (comp->type == Wunld) {
		unld = comp->value1;
	    } else {
		goto trystr;
	    }
	}
	scanb(c);
	if (c == ',') {
	    c = nextc();
	    scanb(c);
	    comp = resw_lookup(name = scansym());
	    if (comp == 0) goto wbotch;
trystr:	    if (comp->type == Wstore) {
		store = comp->value1;
	    } else {
		goto trysel;
	    }
	}
	scanb(c);
	if (c == ',') {
	    c = nextc();
	    scanb(c);
	    comp = resw_lookup(name = scansym());
	    if (comp == 0) goto wbotch;
trysel:	    if (comp->type == Wselect) {
		select = comp->value1;
	    } else {
		goto wbotch;
	    }
	} else {
	    error("hi/lo specifier is required");
	    return False;
	}
	scanb(c);
	if (c != ';') goto botch;
	c = nextc();
    }
    return aflt(rp,load,unld,store,select);

botch:
    error("floating point syntax error -- choked on character %c", c);
    return False;
wbotch:
    error("found \"%s\" while looking for Weitek completer",name);
    return False;
}
#endif

#ifdef VIEW
Boolean
controlpart(  )
{
    register RESERVED *rp;
    register char c;
    char *cp;
    int ctrl, op, ctrl2;

    /* get control part, if any */
    /* syntax is:
	;     (empty field)
	name operator;
    */
    scanb(c);
    switch(c){
    case ';':
	c = nextc();
	scanb(c);
    case '\0':
    case '|':
	return acontrol(-1,0,0);
    }
    rp = resw_lookup(cp=scansym( ));
    if ( rp == 0 || rp->type != Control) {
	error("found \"%s\" while looking for control operand", cp);
	return False;
    } else {
	ctrl = rp->value1;
    }
    scanb(c);
    if (ctrl == 0)
    {	ctrl = 1;
	op = 0;
	goto getctrl2;
    }
    switch (c) {
    case '0':
	op = 0;
	break;
    case '+':
	op = 1;
	break;
    case '-':
	op = 2;
	break;
    default:
	goto botch;
    }
getctrl2:
    ctrl2 = -1;
    if (ctrl == SHAREDMEMPTR) {
	c = nextc();
	scanb(c);
	if (c == ',') {
	    if (op == 0) {
		goto combotch;
	    } else {
		c = nextc();
		scanb(c);
		rp = resw_lookup(cp = scansym());
		if (rp == 0 || rp->type != Control) {
		    goto wbotch;
		} else {
		    ctrl2 = rp->value2;
		    if (ctrl2 == 0) {
			goto combotch;
		    }
		    scanb(c);
		    if (c != '+') {
			goto botch;
		    }
		}
	    }
	}
    }
    if (ctrl != SHAREDMEMPTR && op != 1) {
	error("incompatible control operand and operator");
	return False;
    }
    return acontrol(ctrl,op,ctrl2);
botch:
    error("control syntax error -- choked on character %c", c);
    return False;
wbotch:
    error("found \"%s\" while looking for a control operand",cp);
    return False;
combotch:
    error("illegal combination of controls");
    return False;
}
#endif
#ifdef PAINT
Boolean
controlpart(  )
{
    register RESERVED *rp;
    register char c;
    char *cp;
    int ctrl;

    /* get control part, if any */
    /* syntax is:
	;     (empty field)
	name operator;
    */
    scanb(c);
    switch(c){
    case ';':
	c = nextc();
	scanb(c);
    case '\0':
    case '|':
	return acontrol(-1);
    }
    rp = resw_lookup(cp=scansym( ));
    if ( rp == 0 || rp->type != Control) {
	error("found \"%s\" while looking for control operand", cp);
	return False;
    } else {
	ctrl = rp->value1;
    }
    switch (ctrl) {
    case 1:
	scanb(c);
	switch (c) {
	case '+':
	    break;
	case '-':
	    ctrl = 2;
	    break;
	default:
	    error("found \"%c\" while looking for control operator", c);
	}
	break;
    case 3:
	scanb(c);
	switch (c) {
	case '0':
	    break;
	case '+':
	    ctrl = 4;
	    break;
	default:
	    error("found \"%c\" while looking for control operator", c);
	}
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    default:
	error("found %c while looking for control operator", c);
	return False;
    }
    return acontrol(ctrl);
}
#endif

extern NODE *curnode;

label(sp)
    SYMBOL *sp;
{
    if (sp->defined){
	error("label %s already defined", sp->name );
	return;
    }
    sp->defined = True;
    sp->node = curnode+1;
    DEBUG("Label: defining %s\n", sp->name);
}

int instrcnt = -1;	/* Added 9/25 PWC. */
Boolean
cpp(){
    /* the scanner just saw a '#' -- treat as cpp leaving */
    register char c ;
    char *sp;
    int leng;
    int foo, cln;

    nextc();
    scanb(c);
    cln = curlineno;
    curlineno = getnum( ) - 1;
    scanb(c);
    if (c != '"') {
	curlineno = cln;
	error("#-line not of form output by C preprocessor");
	return False;
    }
    /* just saw the " */
    c = nextc(); /* skip it */
    sp =  curpos;
    while( (c=nextc()) != '"' );
    *curpos = '\0';
    leng = strlen( sp );
    curfilename = (char *)malloc( leng+1 );
    if (curfilename == 0) {
	error("unable to get sufficient storage\n");
	return False;
    }
    strcpy(  curfilename, sp );
    DEBUG("# %d \"%s\"\n", curlineno+1, curfilename);
    return True;
}

    
scanprog()
{
    /* 
      for each line in the program text:
	if it is blank, continue.
	if it begins with a '#', its a cpp dropping
	look at the first symbol on the line.
	    if it is followed by a :, its a label,
		so define the label and scan out the next symbol
	assemble the alu (Am29116) part
	assemble the source/destination part
	assemble the sequencer (Am2910) part
	assemble the float (Weitek) part
	assemble the control part

    */
    char c;
    char *name;
    SYMBOL *sp;
    Boolean stopsw = False;
    Boolean bkptsw = False;
    int debugsw = 0;
    char *save;
    int foo;
    register RESERVED *rp;

    while (newline()){
	if (*curpos == '#'){
	    cpp();
	    continue;
	}
restart:
	scanb(c);
	if( c == '\0') {
	    continue; /* blank line */
	}
	if (c == ';') {
	    name = 0;
	    anext();
	    goto doalu;
	}
	if (!isalnum(c)) {
	    if (c != '|') {
		error("character %c unexpected", c);
	    }
	    anext();
	    curnode->org_pseudo = True;
	    (curnode+1)->word1 = curnode->word1;
	    (curnode+1)->word2 = curnode->word2;
	    (curnode+1)->word3 = curnode->word3;
	    (curnode+1)->imm_29116 = curnode->imm_29116;
	    (curnode+1)->status = curnode->status;
#ifdef VIEW
	    (curnode+1)->no_fpstore = curnode->no_fpstore;
	    (curnode+1)->no_shmread = curnode->no_shmread;
	    (curnode+1)->fphilo = curnode->fphilo;
	    (curnode+1)->no_shminc = curnode->no_shminc;
	    (curnode+1)->no_fpinc = curnode->no_fpinc;
#endif
#ifdef PAINT
	    (curnode+1)->no_scrmread = curnode->no_scrmread;
	    (curnode+1)->no_scrminc = curnode->no_scrminc;
#endif
	    (curnode+2)->no_promread = (curnode+1)->no_promread;
	    continue;
	}
	save = curpos;
	name = scansym( );
	scanb(c);
	if(c== ':') {
	    /* what we have here is a label */
	    sp = lookup( name );
	    if (sp == 0)
		sp = enter( name );
	    label( sp );
	    nextc();
	    scanb(c);
	    (curnode+1)->addr = curaddr;
	    if (c == '\0') {
		anext();
		curnode->org_pseudo = True;
	    }
	    goto restart;
	}
	anext();
	rp = resw_lookup( name );
	if (rp != 0 && rp->type == Pseudo) {
	    /* it's a pseudo-op */
	    switch (rp->value1) {
	    case 1:	/* even */
		if (curaddr % 2 != 0) {
		    curnode->addr = curaddr++;
		    anop();
		    astatus();
	        } else {
		    curnode->org_pseudo = True;
		}
		break;
	    case 2:	/* odd */
		if (curaddr % 2 == 0) {
		    curnode->addr = curaddr++;
		    anop();
		    astatus();
	        } else {
		    curnode->org_pseudo = True;
	    	}
	        break;
	    case 3:	/*org */
	        scanb(c);
	        if (!isdigit(c)) {
		    error("expected digit; found %c", c);
		    continue;
		}
		curnode->org_pseudo = True;
		curaddr = getnum();
		break;
	    case 4:	/* debug */
		scanb(c);
		rp = resw_lookup(name = scansym());
		if (rp == 0 || rp->type != Switch) {
		    error("illegal debug switch value %s",name);
		    continue;
		}
		debugsw = rp->value1;
		curnode->org_pseudo = True;
		break;
	    case 5:	/* stop */
		stopsw = True;
		curnode->org_pseudo = True;
		break;
	    case 6:	/* bkpt */
		bkptsw = True;
		curnode->org_pseudo = True;
		break;
	    case 7:	/* sccsid */
		scanb(c);
		asccs(curpos);
		curnode->addr = curaddr++;
		break;
	    }
	    continue;
	}
doalu:	if (debugsw) {
	    adebug(debugsw);
	    debugsw = 0;
	}
	if (stopsw) {
	    astop();
	    stopsw = False;
	}
	if (bkptsw) {
	    abkpt();
	    bkptsw = False;
	}
	curnode->addr = curaddr++;
	if (alupart(name) == False) goto stat;
	if (srcdestpart( ) == False) goto stat;
	if (seqpart( ) == False) goto stat;
#ifdef VIEW
	if (fltpart( ) == False) goto stat;
#endif
	if (controlpart( ) == False) goto stat;
stat:	astatus();
    }
}
