static char sccs_id[] = "@(#)segments.c	1.1\t10/31/94";
/*
 * This file contains routines associated with reading/writing in the
 * user's target segments (e.g. text, data, bss, etc).
 */

#include <stdio.h>
#include "structs.h"
#include "sym.h"
#include "bitmacros.h"
#include "errors.h"
#include "actions.h"
#include "globals.h"
#include "segments.h"
#include "alloc.h"
#include "node.h"
#ifdef DEBUG
# include "opcode_ptrs.h"
#endif

#define MAXSEGMENTS	16	/*maximum of 16 different segments in an ass'y*/

# define LOG2DATACHUNKSZ	10	/* for DATACHUNKSZ of 1024	*/
# define DATAARRINCR		16

# define DATACHUNKSZ		(01<<LOG2DATACHUNKSZ)
# define CHUNKMASK		(DATACHUNKSZ-1)	/* gives a mask of 1's */
# define CHUNKNO(offset)	( (offset)  >> LOG2DATACHUNKSZ )
# define CHUNKOFFSET(offset)	( (offset) &   CHUNKMASK  )

/*
 *	For each Segment which the user references, a "struct segment" is
 *	set up.  It contains the segment name and number, and refers to
 *	all of the data associated with the segment.
 *
 *	The size of a segment (the amount of data which it will contain)
 *	is (naturally) unknown in advance.  So, the data is organized into
 *	a series of "chunks", each of size DATACHUNKSZ.  Each time one is
 *	filled up, a new (additional) one is allocated.
 *
 *	The "chunks" are kept track of by an array of pointers to chunks.
 *	When the array gets filled up, it is reallocated to make room for
 *	more pointers.
 *	
 */

typedef	enum { SP_RO, SP_RW } SegPerms;	/* R/W permissions on segment */

struct segment
{
	SEGMENT	seg_number;	/* segment number			*/
	char   *seg_name;	/* ptr to Ascii name of segment		*/

	struct symbol *seg_symp;/* ptr to internal "beginning-of-segment"
				 *	symbol, from which an offset is taken
				 *	when the "." symbol is referenced.
				 */

	SegPerms seg_perms  :8;	/* R/W permissions for segment		*/
	BITBOOL seg_do_opt  :1;	/* if TRUE, attempt optimizations on this seg */
	BITINT	/*<unused>*/:7;
	Node	*seg_node_markp;/* ptr to marker node for this segment's node
				 *	list; i.e. the special node which is at
				 *	the beginning and end of the doubly-
				 *	linked list.  This field is NULL and
				 *	unused, UNLESS:
				 *	assembly_mode == ASSY_MODE_BUILD_LIST.
				 */

	U8BITS **seg_chunkpp;	/* ptr to array of ptrs to segment chunks    */
	UINT	seg_chunkslots;	/* # of elements currently in the array      */
	UINT	seg_chunksused;	/* # of elements used in the array	     */

	U8BITS *seg_datap;	/* ptr to next free byte of data in the last */
				/*	segment chunk allocated.	     */
	U32BITS	seg_dataleft;	/* # of bytes of data empty in last chunk    */

	U32BITS	seg_log_length;	/* actual (logical) length of segment; bytes */
	U32BITS	seg_phys_length;/* stored (physical) length of segment; bytes*/
				/* Note that:
				 *	(seg_log_length != seg_phys_length)
				 *   when there is a gap of uninitialized data
				 *   at the end of the segment.  As an extreme
				 *   case, note that BSS's seg_phys_length is
				 *   always 0.
				 */
};


/* array of segments, indexed by segment number, all initialized to 0 */
static struct segment segments[MAXSEGMENTS] = { { 0 } };
static struct segment *curr_segp;	/* pointer to current segment */


/* forward declarations */
static void	get_new_chunk();
static void	init_the_seg();

void
segments_init()
{
	/* this routine must be called AFTER lex_init(), because it references
	 * symbol dot (".").
	 */
	register struct segment *segp;
	register int		 i;
#ifdef DEBUG
	unsigned char *p;
	U32BITS word;
	/* make sure that the byte-order of this host and the target	*/
	/* processor (SPARC) are the same, or funny things could	*/
	/* happen (e.g. in emit_word()).				*/
	word = 0x11223344;
	p = ( unsigned char *)(&word);
	if ( (*p++ != 0x11) || (*p++ != 0x22) ||
	     (*p++ != 0x33) || (*p   != 0x44)
	   )
	{
		internal_error("host and target byte-orders differ");
	}

	/* code in this module (e.g. emit_word() and align()) assumes	*/
	/* that every chunk is aligned on AT LEAST a doubleword (8-byte)*/
	/* boundary;  here, let's ensure that.				*/
	if ( (DATACHUNKSZ % 8) != 0 )
	{
		internal_error("segments_init(): chunk size not multiple of wordsize!");
	}
#endif DEBUG

	for ( i = 0, segp = &segments[0];   i < MAXSEGMENTS;    i++,segp++)
	{
		segp->seg_number = i;
	}
}

/* following used by segname_to_segp() and open_segment() */
static struct segment * next_avail_segp;

static struct segment *
segname_to_segp(name)
	register char *name;
{
	register int i;
	register struct segment *segp;

	/* look up segment name */
	for( i = 0, segp = &segments[0];
	     (i < MAXSEGMENTS) && (segp->seg_name != NULL);
	     i++, segp++
	   )
	{
		/* if it is found, we are done */
		if (strcmp(name, segp->seg_name) == 0)	return segp;
	}

	/* here, it was not found */
	next_avail_segp = segp;	/* for change_named_segment() to use */
	return NULL;
}

static struct segment *
open_segment(name)
	register char *name;
{
	register struct segment *segp;

	if ( (segp = segname_to_segp(name)) == NULL )
	{
		/* didn't find the segment; open up a new one */
		if ((segp = next_avail_segp) >= &segments[MAXSEGMENTS])
		{
			internal_error("open_segment(): >%d segments", MAXSEGMENTS);
		}
		else
		{
			char bos_symname[100];

			/* create an internal "beginning-of-segment" symbol
			 * name (use the same name as the segment, but with
			 * '&' prepended).
			 */
			bos_symname[0] = '&';
			strcpy(&bos_symname[1], name);

			/* now create the symbol entry to go with the
			 * "beginning-of-segment" symbol name.
			 */
#    ifdef DEBUG
			segp->seg_symp = sym_lookup(bos_symname);
			if ( SYM_FOUND(segp->seg_symp) )
			{
				internal_error("open_segment(): dup seg syms!");
			}
#    endif
			segp->seg_symp = sym_add(bos_symname);

			/* Mark that we've seen the definition of this symbol */
			SET_BIT(segp->seg_symp->s_attr, SA_DEFN_SEEN);

			/* now actually define it */
			define_symbol(segp->seg_symp, segp->seg_number, 0);

			/* now set up the ptr to the segment name itself
			 * (pt to just after the '&' prefix in the symbol name)
			 */
			segp->seg_name = segp->seg_symp->s_symname + 1;

			/* set flags for the segment */
			if ( strcmp(name, "text") == 0 )
			{
				segp->seg_perms  = SP_RO;	/* Read-only */
				segp->seg_do_opt = (optimization_level != 0);
			}
			else
			{
				segp->seg_perms  = SP_RW;    /* Read/Write */
				segp->seg_do_opt = FALSE;    /* don't opt  */
			}

			switch (assembly_mode)
			{
			case ASSY_MODE_BUILD_LIST:
				/* We're building a list of the nodes */
				/* Initialize its marker node */
				segp->seg_node_markp = new_marker_node(name);
				break;
			
			case ASSY_MODE_QUICK:
				/* We're not building a list of the nodes;
				 * we'll emit each one as it is read in.
				 */
				segp->seg_node_markp = NULL;
				break;
#ifdef DEBUG
			default:
				internal_error("open_segment(): assymode=%d?",
						assembly_mode);
#endif /*DEBUG*/
			}

			/* Give it an initial array of ptrs to 0 data chunks
			 * (which will be expanded as needed).
			 *
			 * Since System-V malloc() barfs (returns NULL) when
			 * allocating an actual 0-size'd block of memory (what
			 * a stupid idea) we must pass it "1" instead, to keep
			 * malloc() content -- even though we'll never use that
			 * byte.
			 */
			segp->seg_chunkpp =
				(U8BITS**)as_malloc(1 /* 0*sizeof(U8BITS*) */);

			/* Everything else is already initialized to 0,	so will
			 * be taken care of the first time bytes are "emit"ed
			 * to the segment.
			 */
		}
	}

	return segp;	/* return ptr to new segment */
}


void
change_segment(segno)
	register SEGMENT segno;
{
	curr_segp = &segments[segno];

	/* Some (most) segments are just assembled directly.  Those
	 * which are to be optimized (e.g. "text") must be put into a list
	 * structure so optimization can be done, then will later be emitted
	 * from that list.
	 *
	 * The call to set_node_info() passes the information to the list-
	 * building module on whether to build a list or just directly emit
	 * the instructions as they come.  And if it should build the list, 
	 * it has to know on which segment's list to put them.
	 */
	set_node_info(curr_segp->seg_node_markp);
}


void
change_named_segment(name)
	char *name;
{
	register Node *np;

#ifdef SUN3OBJ 
        if ( sun3_seg_name_ok(name) )
	{ 
		change_segment( open_segment(name)->seg_number );
	}
	else	error(ERR_SEGNAME, input_filename, input_line_no, name);
#else 
	change_segment( open_segment(name)->seg_number );
#endif 
}


static void
round_up_segment(segp, bdy)
	register struct segment *segp;
	register unsigned int   bdy;
{
	/* for the given segment, round its length up to the given boundary */
	while ( (segp->seg_log_length % bdy) != 0 )
	{
		segp->seg_log_length++;
	}
}


void
close_all_segments()
{
	register int i;

	/* for each segment, round its length up to a word or doubleword
	 * boundary.
	 */
	for( i = 0, curr_segp = &segments[0];
	     (i < MAXSEGMENTS) && (curr_segp->seg_name != NULL);
	     i++, curr_segp++
	   )
	{
		/* Round all segments up to a doubleword boundary */
		round_up_segment(curr_segp, 8);
	}
}


void
do_for_each_segment(function)
	register void (*function)();	/* pointer to function to call */
{
	register int i;

	/* For each segment, call the given function, passing it a pointer
	 * to the marker node for that segment.
	 */

	for( i = 0, curr_segp = &segments[0];
	     (i < MAXSEGMENTS) && (curr_segp->seg_name != NULL);
	     i++, curr_segp++
	   )
	{
		/* emit this segment's list of nodes, if there are any */
		(*function)(curr_segp->seg_node_markp);
	}
}


void
optimize_all_segments()
{
	register int i;

	/* for each segment, perform optimizations, if applicable */

	for( i = 0, curr_segp = &segments[0];
	     (i < MAXSEGMENTS) && (curr_segp->seg_name != NULL);
	     i++, curr_segp++
	   )
	{
		if ( curr_segp->seg_do_opt )
		{
			optimize_segment(curr_segp->seg_node_markp);
		}
	}
}


void
emit_byte(byte)
	U8BITS byte;
{
	register struct segment *csp = curr_segp; /* for speed */
#ifdef DEBUG
	if(debug) fprintf(stderr,"\t[emit_byte(0x%02x)]\n", byte);
#endif DEBUG

	if ( csp->seg_log_length > csp->seg_phys_length )
	{
		init_the_seg(csp);
	}

	/*** output the given byte into the appropriate segment ***/
	if (csp->seg_dataleft == 0)
	{
		/* have used up last byte in data chunk for segment;	*/
		/* get a new chunk.					*/
		get_new_chunk(csp);
	}

	*((csp->seg_datap)++) = byte;
	--(csp->seg_dataleft);
	++(csp->seg_phys_length);
	++(csp->seg_log_length);
}


void
emit_halfword(halfword)
	U16BITS halfword;
{
	register struct segment *csp = curr_segp; /* for speed */
#ifdef DEBUG
	if(debug) fprintf(stderr,"\t[emit_halfword(0x%04x)]\n", halfword);
#endif DEBUG

	align(2, TRUE);

	if ( csp->seg_log_length > csp->seg_phys_length )
	{
		init_the_seg(csp);
	}

	/*** output the given halfword into the appropriate segment ***/
	if (csp->seg_dataleft == 0)
	{
		/* have used up last byte in data chunk for segment;	*/
		/* get a new chunk.					*/
		get_new_chunk(csp);
	}

#ifdef DEBUG
	if ( ((UINT)(csp->seg_datap) & 0x01) != 0)   internal_error("emit_halfword(): alignment");
#endif DEBUG

	*( (U16BITS *)(csp->seg_datap) ) = halfword;
	(csp->seg_datap)       += sizeof(U16BITS);
	(csp->seg_dataleft)    -= sizeof(U16BITS);
	(csp->seg_phys_length) += sizeof(U16BITS);
	(csp->seg_log_length)  += sizeof(U16BITS);
}


void
emit_word(word)
	U32BITS word;
{
	register struct segment *csp = curr_segp; /* for speed */
#ifdef DEBUG
	if(debug) fprintf(stderr,"\t[emit_word(): 0x%08x]\n", word);
#endif DEBUG

	align(4, TRUE);

	if ( csp->seg_log_length > csp->seg_phys_length )
	{
		init_the_seg(csp);
	}

	/*** output the given word into the appropriate segment ***/
	if (csp->seg_dataleft == 0)
	{
		/* have used up last byte in data chunk for segment;	*/
		/* get a new chunk.					*/
		get_new_chunk(csp);
	}

	*( (U32BITS *)(csp->seg_datap) ) = word;
	(csp->seg_datap)       += sizeof(U32BITS);
	(csp->seg_dataleft)    -= sizeof(U32BITS);
	(csp->seg_phys_length) += sizeof(U32BITS);
	(csp->seg_log_length)  += sizeof(U32BITS);
}


void
align(bdy, err_if_off)
	register UINT bdy;	/* boundary to align to: 1, 2, 4, or 8 */
	register Bool err_if_off;/*TRUE if to give err msg if alignment is off*/
{
	register UINT            modval;

	switch (bdy)
	{
	case 1:	/* "byte" boundary	*/
		break;	/* nothing to do */
	case 2:	/* "halfword" boundary	*/
	case 4:	/* "word" boundary	*/
	case 8:	/* "doubleword" boundary*/
		/* ASSUMEs that the segment is on a doubleword boundary: */
		modval = curr_segp->seg_log_length % bdy;
		if ( modval == 0 )
		{
			/* already on requested boundary;  do nothing */
		}
		else	/* round up to requested boundary, using zero-fill */
		{
			register UINT incr;
			for (incr = bdy - modval;  incr > 0;  incr--)
			{
				emit_byte( (U8BITS)0 );
			}
			if (err_if_off)
			{
				error(ERR_ALIGN, input_filename,
					input_line_no,
					((bdy==2) ?"halfword"
						  :((bdy==4) ?"word"
							     :"doubleword")
					)
				       );
			}
		}
		break;
	default:
		error(ERR_ALIGN_BDY, input_filename, input_line_no);
	}
}


static void
init_the_seg(segp)
	register struct segment *segp;
{
	/* the tail end of the given segment is uninitialized data (e.g. via
	 * ".skip" pseudo), so has been left unwritten.  we are about
	 * to write actual (logical) data onto the end, so must play "catch up"
	 * and fill in the hole at the end of the segment with zeroes.
	 */
	register long int nbytes;

	for ( nbytes = segp->seg_log_length - segp->seg_phys_length;
	      nbytes > 0;    nbytes--
	    )
	{
		/*** output a zero byte into the appropriate segment ***/

		if (segp->seg_dataleft == 0)
		{
			/* have used up last byte in data chunk for segment;*/
			/* get a new chunk.				    */
			get_new_chunk(segp);
		}

		*((segp->seg_datap)++) = 0;
		(segp->seg_dataleft)--;
	}

	segp->seg_phys_length = segp->seg_log_length;
}


void
emit_skipbytes(segno, nskip)
	SEGMENT segno;	/* segment # of segment in which to skip bytes	*/
	U32BITS nskip;	/* number of bytes to skip in the segment	*/
{
#ifdef DEBUG
	if (segno == ASSEG_UNKNOWN)    internal_error("emit_skipbytes(): seg#");
#endif DEBUG
	/* Increment the Logical length of the segment.
	 * Note that the Physical length to be stored in the object file
	 * (seg_phys_length) is not affected (i.e. what's the point of storing
	 * Uninitialized data in the obj file, if we can get away without it?).
	 */
	segments[segno].seg_log_length += nskip;
}


static void
get_new_chunk(segp)
	register struct segment *segp;
{
	if (segp->seg_chunkslots == segp->seg_chunksused)
	{
		/* am out of array elements; reallocate to get more */
		segp->seg_chunkslots += DATAARRINCR;
		segp->seg_chunkpp = (U8BITS **) as_realloc(
					segp->seg_chunkpp,
					segp->seg_chunkslots*sizeof(U8BITS*));
	}

	/* now allocate the newest "chunk" */
	segp->seg_chunkpp[segp->seg_chunksused++] = segp->seg_datap =
			as_malloc(DATACHUNKSZ);
	segp->seg_dataleft = DATACHUNKSZ;
}


#ifdef DEBUG
void
emit_instr(instr)
	register INSTR instr;
{
	long int disp;
#  include "instructions.h"
#  define OP(i)  ( ((i)>>30) & MASK(2) ) 
#  ifdef CHIPFABS
#    define OP2(i) ( (iu_fabno == 1)		  \
			? (((i)>>22) & MASK(3))	  \
			: (((i)>>23) & MASK(2)) ) 
#  else /*CHIPFABS*/
#    define OP2(i) ( ((i)>>23) & MASK(2) ) 
#  endif /*CHIPFABS*/
#  define OP3(i) ( ((i)>>19) & MASK(6) ) 
	if(debug)
	{
		fprintf(stderr, "\t[emit_instr(0x%08x): op=%d, ", instr, OP(instr));
		switch( OP(instr) )
		{
		case 01:	/* CALL */
			disp = instr & MASK(30);
			if(debug)fprintf(stderr, "disp30=%d", disp);
			if (disp != 0)	if(debug)fprintf(stderr,"=0x%04x",disp);
			break;
		case 00:	/* SETHI, BRANCHES */
			if( OP3(instr) == OP3(opcode_ptr[OP_SETHI]->o_instr) )
			{
				fprintf(stderr, "rd=%d", (instr>>25)&MASK(5) );
			}
			else	fprintf(stderr, "a=%d, cond=0x%x",
					(instr>>29)&1, (instr>>25)&MASK(5) );
#ifdef CHIPFABS
			disp = instr & ((iu_fabno == 1) ? MASK(23) : MASK(22));
			fprintf(stderr,", op2=%d, disp%d=%d", OP2(instr),
					(iu_fabno == 1)?23:22, disp);
#else /*CHIPFABS*/
			disp = instr & MASK(22);
			fprintf(stderr,", op2=%d, disp22=%d", OP2(instr), disp);
#endif /*CHIPFABS*/
			if (disp != 0)	fprintf(stderr, "=0x%04x", disp);
			break;
		default:
			fprintf(stderr, "rd=%2d, op3=0x%02x, rs1=%2d, i=%d, ",
				/*rd*/(instr>>25)&MASK(5), /*op3*/OP3(instr),
				/*rs1*/(instr>>14)&MASK(5),
				/*i*/(instr>>13)&MASK(1) );
			if ( (instr&(1<<13)) != 0)
			{
				long int i = instr; /* int gets sign-ext */
				fprintf(stderr, "imm=%d", ((i<<19)>>19));
			}
			else	fprintf(stderr, "rs2=%2d", instr&MASK(5));
		}
		fprintf(stderr, "]\n");
	}

	/*** output the word into the appropriate segment ***/
	emit_word(instr);
}
#endif DEBUG


LOC_COUNTER
get_dot()
{
	/* The location counter of the current segment == its total length */
	return  curr_segp->seg_log_length;
}


SEGMENT
get_dot_seg()
{
	return  curr_segp->seg_number;
}


char *
get_dot_seg_name()
{
	return  curr_segp->seg_name;
}


struct symbol *
get_dot_seg_symp()
{
	return  curr_segp->seg_symp;
}


U8BITS *
get_obj_code_ptr(segno, seg_off)
	register SEGMENT     segno;	/* segment number	 */
	register LOC_COUNTER seg_off;	/* offset within segment */
{
	register struct segment *segp;
	register U8BITS		*chunkp;
	register UINT		 chunkno;

	if ( (segp = &segments[segno]) == NULL )
	{
		internal_error("get_obj_code_ptr(): bad seg# (%d)", segno);
		/* internal_error never returns */
		return (U8BITS *)NULL;/*stmt just here to keep lint happy*/
	}
	else if ( ((chunkno = CHUNKNO(seg_off)) >= segp->seg_chunksused) ||
		  ((chunkp = segp->seg_chunkpp[chunkno]) == NULL)
		)
	{
		internal_error("get_obj_code_ptr(): bad chunk# (%d)", chunkno);
		/* internal_error never returns */
		return (U8BITS *)NULL;/*stmt just here to keep lint happy*/
	}
	else
	{
		return (U8BITS *)(chunkp + CHUNKOFFSET(seg_off));
	}
}


#ifdef DEBUG
char *
get_seg_name(segno)		/* used by relocate()	*/
	SEGMENT segno;
{
	     if (segno == ASSEG_ABS)		return "<ASSEG_ABS>";
	else if (segno == ASSEG_UNKNOWN)	return "<ASSEG_UNKNOWN>";
	else if (segno < MAXSEGMENTS)		return segments[segno].seg_name;
	else					return  "<ASSEG_error>";
}
#endif DEBUG

/*--------------------------------------------------------------------------*/

U32BITS
get_seg_size(segno)
	register SEGMENT segno;
{
	if (segno == ASSEG_UNKNOWN)	return 0;
	else				return segments[segno].seg_log_length;
}

SEGMENT
get_seg_number(name)
	register char *name;
{
	/* Note that get_seg_number() is not passive; it CREATES the segment
	 * if it didn't already exist.
	 */
	return  (open_segment(name))->seg_number;
}

SEGMENT
lookup_seg_number(name)
	register char *name;
{
	register struct segment *segp;

	/* Note that lookup_seg_number() is PASSIVE;
	 * it just returns ASSEG_UNKNOWN if the segment didn't already exist.
	 */

	if ( (segp = segname_to_segp(name)) == NULL )
	{
		/* didn't find the segment */
		return ASSEG_UNKNOWN;
	}
	else	return segp->seg_number;   /* return the segment's number */
}


#ifdef SUN3OBJ

static void
ABSwrite_chunk(fp, p, nbytes)
	FILE *fp;
	U8BITS *p;
	unsigned int   nbytes;
{
	if ( fwrite(p, 1, nbytes, fp) != nbytes )
	{
		perror(pgm_name);	/* print the sys err message */
		error(ERR_WRITE_ERROR, NULL, 0, objfilename);
	}
}

static void
ABSwrite_segment(fp, sp)
	FILE *fp;
	register struct segment *sp;
{
	unsigned long int i;
	long int nbytes;
	static U8BITS zeroes[512];	/* init. to zeroes, by default */

	for ( i = 0;   i < sp->seg_chunksused;   i++)
	{
		nbytes = DATACHUNKSZ;
		if (i==(sp->seg_chunksused-1))	nbytes -= sp->seg_dataleft;
		ABSwrite_chunk(fp, sp->seg_chunkpp[i], nbytes);
	}

	/* in SUN3 obj file, store zero bytes from end of segment, as well */
	for ( nbytes = sp->seg_log_length - sp->seg_phys_length;
	      nbytes > 0;    nbytes-=512
	    )
	{
		ABSwrite_chunk(fp, &zeroes[0], (nbytes>512)?512:nbytes);
	}
}

void
ABSwrite_seg(segno, ofp)
	register SEGMENT segno;
	register FILE *ofp;
{
	if (segno == ASSEG_UNKNOWN)
	{
		/*** do nothing;
		/*** not all 3 of "text"/"data"/"bss" HAVE to be present.
		/***internal_error("ABSwrite_seg(): unknown segment \"%s\"", name);***/
	}
	else	ABSwrite_segment(ofp, &segments[segno]);
}

#endif SUN3OBJ
