/*	@(#)sym_fcns.h	1.1	10/31/94	*/
/* forward routine declarations */
static HASHIDX		 FCN_HASH ();
struct STRUCTNAME	*FCN_LOOKUP ();
#ifdef FCN_ADD
struct STRUCTNAME	*FCN_ADD ();
#endif


/*-----------------------------------------------------------------------------
 *	The following hashing function works as follows:
 *
 *		If the given string is 0-length, 0 is returned.
 *
 *		Otherwise, the last character of the string becomes the lower
 *		seven bits of the hash value, the second character of the
 *		string (zero if the string is only one character long) becomes
 *		the next most significant 7 bits of the hash value, and the
 *		bits of the string's length become the upper bits of the hash
 *		value.
 *		
 *		The value returned is the hash value MOD the size of the table.
 *-----------------------------------------------------------------------------
 */

static HASHIDX
FCN_HASH (name)
register char *	name;	/* name to be hashed */
{
	register char		*p;
	register unsigned int	 hvalue;

	p = name;

	if( *p == '\0' )	return 0;
	else
	{
		hvalue = *(p+1) << 7;	/* char #2 into second 7 bits */
		while( *p != '\0' )	p++;

		hvalue |= *(p-1);	/* last char into bottom 7 bits */
		hvalue |= (p - name)<<14;/*bottom length bits into hi 2 bits */

		return ((HASHIDX)(hvalue % TBLLEN));/* MOD hash table size */
	}
}

#ifdef FCN_DELETE
/*-----------------------------------------------------------------------------
 * FCN_DELETE:
 *	Deletes the symbol-table entry for the given symbol.
 *-----------------------------------------------------------------------------
 */

void
FCN_DELETE (key)
register char	*key;		/* key (string) to delete */
{
	register struct STRUCTNAME   **pp;
	register HASHIDX	hidx;
	register struct STRUCTNAME   *new_next_one;
#  ifdef DEBUG
	register Bool found_the_symbol = FALSE;
#  endif /*DEBUG*/

	hidx = FCN_HASH (key);		/* hash the key */

	/*
	 *	Search the chain of symbols which hashed to that key,
	 *	seeing if one of them matches.
	 */

	for (pp = & TABLE [hidx];   (*pp) != NULL;   pp = &((*pp)->CHAIN) )
	{
		if( strcmp(key, (*pp)->SYMNAME) == 0 )
		{
			/* Found the symbol. */

			new_next_one = (*pp)->CHAIN;
			FCN_FREE (*pp);		/* deallocate this one */
			*pp = new_next_one;	/* Remove it from the list */
#  ifdef DEBUG
			found_the_symbol = TRUE;
#  endif /*DEBUG*/

			break;	/* out of the "for" loop */
		}
	}

#  ifdef DEBUG
	if (!found_the_symbol)
	{
		internal_error("???_delete(): couldn't find \"%s\"", key);
	}
#  endif /*DEBUG*/
}
#endif

/*-----------------------------------------------------------------------------
 * FCN_LOOKUP:
 *	The symbol entry is returned when the given symbol is found.
 *	Otherwise, NULL is returned.
 *-----------------------------------------------------------------------------
 */

struct STRUCTNAME *
FCN_LOOKUP (key)
register char	*key;		/* key (string) to search for */
{
	register struct STRUCTNAME   *p;
	register HASHIDX	hidx;

	hidx = FCN_HASH (key);		/* hash the key */

	/*
	 *	Search the chain of symbols which hashed to that key,
	 *	seeing if one of them matches.
	*/

	for ( p = TABLE [hidx];   p != NULL;   p = p->CHAIN )
	{
		if( strcmp(key, p->SYMNAME) == 0 )
		{
			/* Found the symbol.	*/
#ifdef SYNONYM
			/* If it is a synonym for something else, then go look
			 * up the "real" symbol-table entry.
			 */
			if (p->SYNONYM == NULL)	return  p;
			else			return FCN_LOOKUP (p->SYNONYM);
#else /*SYNONYM*/
			return  p;
#endif /*SYNONYM*/
		}
	}

	return (struct STRUCTNAME *) NULL;
}


/*-----------------------------------------------------------------------------
 *	This function takes an initialized array of symbol/opcode
 *	structures, and adds each entry to the appropriate symbol table.
 *-----------------------------------------------------------------------------
 */

void
FCN_INIT (tblp)
register struct STRUCTNAME *tblp;
{
	register struct STRUCTNAME	*p;
	register HASHIDX		hidx;

	/* for every symbol in the table, make a hash table entry	*/
	for (    ;   (tblp->SYMNAME != NULL);   tblp++)
	{
		hidx = FCN_HASH (tblp->SYMNAME);

		/*
		 *	Search the chain of symbols which hashed to the same
		 *	hash index.
		*/

		for ( p = TABLE [hidx];   p != NULL;   p = p->CHAIN )
		{
#ifdef DEBUG
			if( strcmp(tblp->SYMNAME, p->SYMNAME) == 0 )
			{
				/* got a match - item is already in the table */
				internal_error("FCN_INIT (): duplicate initialization for \"%s\"",p->SYMNAME);
			}
#endif
		}

		/*	Here, no match was found (as SHOULD be the case).
		 *	Add entry to the head of the hash chain.
		 */
		tblp->CHAIN = TABLE [hidx];
		TABLE [hidx] = tblp;
	}
}


#ifdef FCN_ADD
struct STRUCTNAME *
FCN_ADD (key)		/* Add an entry to the symbol table */
register char *key;
{
	register struct STRUCTNAME	*p;
	register HASHIDX		hidx;

	hidx = FCN_HASH (key);	/* get hash index */

	/*
		Starting at the entry pointed to by the hash-table entry,
		search the chain of symbols which hashed to that key.
	*/

	for ( p = TABLE [hidx];   p != NULL;   p = p->CHAIN )
	{
		/* Compare the current entry to the key */
		if( strcmp(key, p->SYMNAME) == 0 )
		{
			/* item is already in the table */
			return p;
		}
	}

	/* Here, no match was found;  add entry to the head of the hash chain */

	p = FCN_NEW (key);
	p->CHAIN = TABLE [hidx];
	TABLE [hidx] = p;

	return p;
}
#endif

#ifdef FCN_NEXT
static struct STRUCTNAME  *next_symp;	   /* ptr to next symbol in hash chain*/
static struct STRUCTNAME **next_chain_head; /* next entry from hash table*/
static struct STRUCTNAME **end_chain_head;  /* end entry from hash table*/
/*************************  s y m _ n e x t  *********************************/
struct STRUCTNAME *
FCN_NEXT ()
{
	register struct STRUCTNAME  *p;
	register struct STRUCTNAME **nchpp; /*register copy of next_chain_head*/

	if(next_symp == NULL)
	{
		/* skip empty slots in the hash table */
		for ( nchpp = next_chain_head;
		      (nchpp < end_chain_head) && (*nchpp == NULL);
		      nchpp++
		    ) ;

		next_chain_head = nchpp;	/* (copy new value back) */

		if(next_chain_head == end_chain_head)
		{
			/* past end of hash table, so we've gone through
			 * all of the symbols.
			 */
			return NULL ;
		}
		else
		{
			/* have a chain of symbol(s) off of the hash tbl entry*/
			next_symp = *next_chain_head;
			next_chain_head++;
		}
	}

	p = next_symp;
	next_symp = next_symp->CHAIN;

	return p;
}


/************************  s y m _ f i r s t  ********************************/
struct STRUCTNAME *
FCN_FIRST ()
{
	next_symp = NULL;
	next_chain_head = & TABLE [0];

	/* points to (non-existant) entry just AFTER end of table */
	end_chain_head  = & TABLE [TBLLEN];

	return( FCN_NEXT () );
}
#endif


#ifdef FCN_FREE
/*************************  s y m _ f r e e  *********************************/
FCN_FREE (p)
	struct STRUCTNAME *p;
{
	as_free(p);
}
#endif
