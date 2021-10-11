static char sccs_id[] = "@(#)relocs.c	1.1\t10/31/94";
/*
 * This file contains routines associated with relocations
 * (performing them, and/or passing them on to the linker).
 */

#include <stdio.h>
#include <sun4/a.out.h>

#ifdef CHIPFABS
   /* Old relocation types (for fab-1 chips).
    * THESE SHOULD DISAPPEAR BEFORE THE REAL RELEASE.
    */
#  define oRELOC_WDISP23  ((enum reloc_type) (15))
#  define oRELOC_HI23	  ((enum reloc_type) (16))
#  define oRELOC_23	  ((enum reloc_type) (17))
#  define oRELOC_LO9	  ((enum reloc_type) (18))
#endif /*CHIPFABS*/

#include "structs.h"
#include "sym.h"
#include "bitmacros.h"
#include "errors.h"
#include "build_instr.h"
#include "globals.h"
#include "segments.h"
#include "alloc.h"
#include "scan_symbols.h"
#include "registers.h"
#include "symbol_expression.h"

struct reloc
{
	RELTYPE	    r_type;	/* Type of relocation			*/
				/* The value to be relcoated:		*/
	struct symbol *r_symp;	/*	symbol value to relocate,	*/
	long int    r_addend;	/*	plus a constant addend.		*/
				/* Where, within this module, the	*/
				/* relocation is to be performed:	*/
	SEGMENT	    r_segment;	/*	which segment,			*/
	LOC_COUNTER r_offset;	/*	offset within the segment.	*/
};

struct rel_list
{
	struct reloc     rl_reloc;
	char		*rl_fnamep;	/* input file  of reloc (for err msgs)*/
	LINENO		 rl_lineno;	/* input line# of reloc (for err msgs)*/
	struct rel_list *rl_next;
};

static struct rel_list *rel_list_head = NULL;
static struct rel_list *rel_list_tail = NULL;

/* forward declaration */
static Bool asm_relocation();
static U32BITS get_obj_word();
static void put_obj_word();


#ifdef DEBUG
static char *
get_reloc_name(reltype)
	RELTYPE reltype;
{
	switch(reltype)
	{
	/*** case REL_ABS:	return "REL_ABS"; ***/
	case REL_LO10:		return "REL_LO10";
	case REL_13:		return "REL_13";
	case REL_22:		return "REL_22";
	case REL_HI22:		return "REL_HI22";
	case REL_DISP22:	return "REL_DISP22";
	case REL_DISP30:	return "REL_DISP30";
	case REL_8:		return "REL_8";
	case REL_16:		return "REL_16";
	case REL_32:		return "REL_32";
	case REL_SFA_BR:	return "REL_SFA_BR";
	case REL_SFA_OF:	return "REL_SFA_OF";
	case REL_SFA:		return "REL_SFA";
	case REL_BASE10:	return "REL_BASE10";
	case REL_BASE13:	return "REL_BASE13";
	case REL_BASE22:	return "REL_BASE22";
	case REL_PC10:		return "REL_PC10";
	case REL_PC22:		return "REL_PC22";
	case REL_JMP_TBL:	return "REL_JMP_TBL";
	default:		return "REL_<error>";
	}
}
#endif


/*
 * Note:
 *	relocate() for a location must not be called until AFTER some data
 *	has been emitted for that location.  Otherwise, when it tries to do
 *	a relocation, it [1] cannot fetch the word from the segment, and even
 *	if it could, [2] the relocation written out (if it can be performed 
 *	immediately, by the assembler) will get clobbered when the data is
 *	emitted.
 */
void
relocate(reltype, symp, segment, offset, addend, filename, lineno)
	RELTYPE		reltype;
	struct symbol  *symp;
	SEGMENT		segment;
	LOC_COUNTER	offset;
	S32BITS		addend;
	char	       *filename;
	LINENO		lineno;
{
	register struct rel_list *rlp;
			/* "avail_rlp" saves a lot of free()s and re-malloc()s
			 * of the same piece of memory (which is expensive).
			 */
	static struct rel_list *avail_rlp = NULL;
#ifdef DEBUG
	char *type, *seg;
	extern char *get_seg_name();

	if(debug)
	{
		type = get_reloc_name(reltype);

		seg = get_seg_name(segment);

		fprintf(stderr, "\t[relocate():");
		fprintf(stderr, " reltype=%s(%d),",	type, reltype);
		if (symp == NULL)	fprintf(stderr, "sym=(none), ");
		else		fprintf(stderr, "sym=\"%s\",",symp->s_symname);
		fprintf(stderr, " segment=%s(%d),",	seg, segment);
		fprintf(stderr, "\n\t\t");
		fprintf(stderr, " offset=%d",		offset);
		if (offset != 0)	fprintf(stderr, "=0x%04x", offset);
		fprintf(stderr, ", addend=%d",		addend);
		if (addend != 0)	fprintf(stderr, "=0x%04x", addend);
		fprintf(stderr, ", lineno=%d",		lineno);
		fprintf(stderr, "]\n");
	}
#endif
	if ( reltype == REL_SFA )
	{
		/* the REL_SFA type is shorthand for doing two relocations:
		 *	REL_SFA_BR (Short-Form Adressing base register)
		 *  and REL_SFA_OF (13-bit Short-Form Adressing offset)
		 */
		relocate(REL_SFA_BR,symp,segment,offset,addend,filename,lineno);
		relocate(REL_SFA_OF,symp,segment,offset,addend,filename,lineno);
		return;
	}

	/* build a relocation list element	*/

	if (avail_rlp == NULL)
	{
		/* allocate a new structure */
		rlp = (struct rel_list *)as_malloc(sizeof(struct rel_list));
	}
	else	rlp = avail_rlp;	/* re-use a previously-allocated one */
	rlp->rl_reloc.r_type    = reltype;
	rlp->rl_reloc.r_symp    = symp;
	rlp->rl_reloc.r_addend  = addend;
	rlp->rl_reloc.r_offset  = offset;
	rlp->rl_reloc.r_segment = segment;
	rlp->rl_fnamep          = filename;
	rlp->rl_lineno          = lineno;


	/*
	 * Actually, ALL of these could be stuffed on the list now, and
	 * the "asm_relocation" attempted at the end of the assembly.
	 * But, to try to keep the size of the list down, let's try it now.
	 *
	 * Do not attempt to resolve relocations involving expressions of
	 * symbols now; always wait until later to handle that.
	 */
	if ( ((symp == NULL) || BIT_IS_OFF(symp->s_attr, SA_EXPRESSION)) &&
	     asm_relocation(rlp)
	   )
	{
		/* then the "relocation" is done; don't put it in the list */
		avail_rlp = rlp;    /* re-use the allocated memory next time */
	}
	else
	{
		avail_rlp = NULL;   /* don't re-use the allocated memory */

		/* save the relocation on the list, to look at
		 * at the end of the assembly.
		 */
		rlp->rl_next = NULL;

		if ( rel_list_head == NULL )	rel_list_head = rlp;
		else				rel_list_tail->rl_next = rlp;

		rel_list_tail = rlp;
	}
}


/* Resolve_relocations() is called after the assembly pass is complete;  it
 * attempts once again to resolve any relocations (e.g. those which were
 * forward references), before leaving the rest for the linker to do.
 */
void
resolve_relocations()
{
	register struct rel_list **rlpp;
	register struct rel_list  *rlp;

#ifdef DEBUG
	if (!did_scan_symbols_after_input) internal_error("resolve_relocations(): didn't do scan_symbols_after_input() first");
#endif

	for ( rlpp = &rel_list_head;  rlp = *rlpp, rlp != NULL;    )
	{
		if ( ( (rlp->rl_reloc.r_symp == NULL) ||
		       evaluate_symbol_expression(rlp->rl_reloc.r_symp,
		       				  rlp->rl_reloc.r_segment,
		       				  rlp->rl_reloc.r_offset )
		     ) &&
		     (!asm_relocation(rlp))
		   )
		{
			/* Couldn't do it here, but not due to expression
			 * problems.
			 * Just leave the relocation on the list, to be
			 * written out for the link editor to perform.
			 */
			rlpp = &(rlp->rl_next);	/* "increment" for loop */
		}
		else
		{
			/* Either there was an error in expression evaluation,
			 * or the "relocation" was successfully done and the
			 * linker won't have to worry about it; in either case,
			 * take it off of the list.
			 */
			*rlpp = rlp->rl_next;	/* is also "incr" for loop */
			as_free(rlp);
		}
	}
}


#ifdef SUN3OBJ
/* If any segments are concatenated (such as the data segments together, or the
 * data segment(s) to the Text segment), then the segment offsets in the
 * relocation records for all but the first such segment have to be adjusted to
 * be relative to the beginning of the FIRST such segment.
 *
 * This routine does just that.
 */
void
adjust_concat_relocs(cs_seg_count, cs_seg_offsets, cs_segnos)
	int	cs_seg_count;	/* the # of concatenated segments */
		/* offsets from beginning of the first concatenated segment */
	unsigned long int cs_seg_offsets[];
	SEGMENT cs_segnos[]; /* segment #s of the seg's*/
{
	register int i;
	register struct rel_list  *rlp;

	for ( rlp= rel_list_head;   rlp != NULL;   rlp = rlp->rl_next)
	{
		/* Start i at 1 here, because there will be no change for
		 * relocations referencing the FIRST of the concatenated
		 * segments.
		 */
		for (i = 1;  i < cs_seg_count;  i++)
		{
			if (rlp->rl_reloc.r_segment == cs_segnos[i])
			{
				rlp->rl_reloc.r_segment = cs_segnos[0];

				/* Adjust the relocation offset, to be from
				 * beginning of the FIRST concatenated segment,
				 * instead of from the first one's beginning.
			 	 */
				rlp->rl_reloc.r_offset += cs_seg_offsets[i];

				break;
			}
		}
	}
}
#endif

#ifdef SUN3OBJ
#ifndef INTERIM_OBJ_FMT
/*
 * pc-relative relocation is really segment-relative, and
 * must be adjusted BOTH by the offset-within-segment,
 * AND the segment's offset-from-0.
 */
static void
adjust_pc_relocation( rlp, segno )
	register SEGMENT segno;
	register struct rel_list  *rlp;
{
	extern SEGMENT Text_segno, Data_segno, Bss_segno;
	extern U32BITS Text_size, Data_size;

	rlp->rl_reloc.r_addend -=rlp->rl_reloc.r_offset;
	if ( segno == Text_segno ) { 
		/* normal case. do nothing. */
	} else if ( segno == Data_segno ) {
		rlp->rl_reloc.r_addend -= Text_size;
	} else 
		internal_error("adjust_pc_relocation(): relocation in bss!");
}
#endif /* not INTERIM_OBJ_FMT */
#endif /* SUN3OBJ */

void
pic_relocs(segno)
	register SEGMENT segno;
{
	/* All of this would better be done in relocate(), but that will have
	 * to be arranged "another day".  Note that the string comparisons on
	 * symbol names won't be possible there, since "symbol names" haven't
	 * yet been assigned to expressions there.  We'll have to use another
	 * method to detect references to PIC relocations.
	 */

	register struct rel_list  *rlp;

	for ( rlp= rel_list_head;  rlp != NULL; )
	{
		if ( rlp->rl_reloc.r_segment == segno )
		{
			register char *namep;	/* pts to 1st symbol name */
			if (rlp->rl_reloc.r_symp == NULL)
			{
				namep = NULL;
			}
			else
			{
				for ( namep = rlp->rl_reloc.r_symp->s_symname;
				      *namep == '(';
				      namep++)
				{
					/* empty body */
				}
			}

			switch ( rlp->rl_reloc.r_type )
			{
			case REL_13:
				CLR_BIT(rlp->rl_reloc.r_symp->s_attr,
					SA_NOWRITE);
				rlp->rl_reloc.r_type = REL_BASE13;
				break;
			case REL_LO10:
				if ( (namep != NULL) &&
				     (strncmp(namep, "__GLOBAL_OFFSET_TABLE_", 22) == 0)
				   )
				{
					rlp->rl_reloc.r_type = REL_PC10;
					rlp->rl_reloc.r_addend +=
						rlp->rl_reloc.r_symp->s_value;
					rlp->rl_reloc.r_symp =
						sym_lookup("__GLOBAL_OFFSET_TABLE_");
				}
				else
				{
					rlp->rl_reloc.r_type = REL_BASE10;
					CLR_BIT(rlp->rl_reloc.r_symp->s_attr,
						SA_NOWRITE);
				}
				break;
			case REL_HI22:
				if ( (namep != NULL) &&
				     (strncmp(namep, "__GLOBAL_OFFSET_TABLE_", 22) == 0)
				   )
				{
					rlp->rl_reloc.r_type = REL_PC22;
					rlp->rl_reloc.r_addend +=
						rlp->rl_reloc.r_symp->s_value;
					rlp->rl_reloc.r_symp =
						sym_lookup("__GLOBAL_OFFSET_TABLE_");
				}
				else
				{
					rlp->rl_reloc.r_type = REL_BASE22;
					CLR_BIT(rlp->rl_reloc.r_symp->s_attr,
						SA_NOWRITE);
				}
				break;
			case REL_DISP30:
				rlp->rl_reloc.r_type = REL_JMP_TBL;
				break;
			}
		}
		rlp = rlp->rl_next;
	}
}

/* write_relocs() writes the relocs for the given segment,
 * and returns how many bytes were written out.
 */
U32BITS
write_relocs(segno, ofp)
	register SEGMENT segno;	/* the seg in which the relocations are made */
	register FILE   *ofp;	/* object file */
{
	register struct rel_list **rlpp;
	register struct rel_list  *rlp;
	register U32BITS bytes_written;
	struct reloc_info_sparc rel;
#ifdef SUN3OBJ
	extern SEGMENT Text_segno, Data_segno, Bss_segno, Sdata_segno;
	extern U32BITS Text_size, Data_size, Sdata_size;
# ifdef DEBUG
	extern Bool Seg_nums_set, Seg_sizes_set;
	if (!Seg_nums_set || !Seg_sizes_set)	internal_error("write_relocs(): invalid Seg values");
# endif DEBUG
#endif SUN3OBJ

	/* Just-to-be-safe, zero the initial relocation record, so that any
	 * unused fields (e.g. uninitialized bitfields) are zeroed.
	 */
	bzero(&rel, sizeof(rel));

	/* Now, create and write out all of the relocations left over for
	 * the linker to do.
	 */
	for ( bytes_written=0,rlpp= &rel_list_head;  rlp = *rlpp, rlp != NULL; )
	{
		if ( rlp->rl_reloc.r_segment == segno )
		{
			/* this relocation is for the right segment. */

			/* pass the relocation (thru the object file)
			 * to the linker to perform.
			 */

			/* the offset within the segment at which to do
			 * the relcation...
			 */
			rel.r_address   = rlp->rl_reloc.r_offset;

			/* the type of relocation to perform */
			switch ( rlp->rl_reloc.r_type )
			{
			case REL_LO10:
#ifdef CHIPFABS
				if (iu_fabno == 1)
				{
					/* make it 9-bit instead */
					rel.r_type = oRELOC_LO9;
				}
				else	rel.r_type = RELOC_LO10;
#else /*CHIPFABS*/
				rel.r_type = RELOC_LO10;
#endif /*CHIPFABS*/
				break;
			case REL_13:	  rel.r_type = RELOC_13;	break;
			case REL_22:
#ifdef CHIPFABS
				if (iu_fabno == 1)
				{
					/* make it 23-bit instead */
					rel.r_type = oRELOC_23;
				}
				else	rel.r_type = RELOC_22;
#else /*CHIPFABS*/
				rel.r_type = RELOC_22;
#endif /*CHIPFABS*/
				break;
			case REL_HI22:
#ifdef CHIPFABS
				if (iu_fabno == 1)
				{
					/* make it 23-bit instead */
					rel.r_type = oRELOC_HI23;
				}
				else	rel.r_type = RELOC_HI22;
#else /*CHIPFABS*/
				rel.r_type = RELOC_HI22;
#endif /*CHIPFABS*/
				break;
			case REL_DISP22:
#ifdef CHIPFABS
				if (iu_fabno == 1)
				{
					/* make it 23-bit instead */
					rel.r_type = oRELOC_WDISP23;
				}
				else	rel.r_type = RELOC_WDISP22;
#else /*CHIPFABS*/
				rel.r_type = RELOC_WDISP22;
#endif /*CHIPFABS*/
#ifndef INTERIM_OBJ_FMT
				/* pc-relative is really segment-relative */
				adjust_pc_relocation( rlp, segno );
#endif /*INTERIM_OBJ_FMT*/
				break;
#ifdef INTERIM_OBJ_FMT
			case REL_DISP30:  rel.r_type = RELOC_WDISP30;	break;
#else /*INTERIM_OBJ_FMT*/
			case REL_DISP30:
				rel.r_type = RELOC_WDISP30;
				/* pc-relative is really segment-relative */
				adjust_pc_relocation( rlp, segno );
				break;
#endif /*INTERIM_OBJ_FMT*/
			case REL_SFA_BR:  rel.r_type = RELOC_SFA_BASE;	break;
			case REL_SFA_OF:  rel.r_type = RELOC_SFA_OFF13;	break;
			case REL_BASE10:  rel.r_type = RELOC_BASE10; break;
			case REL_BASE13:  rel.r_type = RELOC_BASE13; break;
			case REL_BASE22:  rel.r_type = RELOC_BASE22; break;
			case REL_PC10:    rel.r_type = RELOC_PC10; break;
			case REL_PC22:    rel.r_type = RELOC_PC22; break;
			case REL_JMP_TBL: rel.r_type = RELOC_JMP_TBL; 
					  adjust_pc_relocation( rlp, segno );
					 break;
			case REL_8:	  rel.r_type = RELOC_8;		break;
			case REL_16:	  rel.r_type = RELOC_16;	break;
			case REL_32:	  rel.r_type = RELOC_32;	break;
			default:
#ifdef DEBUG
				internal_error(
					"write_relocs(): bad type %d (%s)",
					rlp->rl_reloc.r_type,
					get_reloc_name(rlp->rl_reloc.r_type));
#else
				internal_error("write_relocs(): bad type %d",
					rlp->rl_reloc.r_type);
#endif
			}

			if (rlp->rl_reloc.r_symp == NULL)
			{
				switch ( rlp->rl_reloc.r_type )
				{
				case REL_DISP22:
				case REL_DISP30:
					/* a relocation for a Branch or a Call
					 * to an absolute address.
					 */
					rel.r_extern  = 0;
#ifdef INTERIM_OBJ_FMT
					rel.r_index/*seg*/ = SEG_ABS;
#else /*INTERIM_OBJ_FMT*/
					rel.r_index/*seg*/ = N_ABS;
#endif /*INTERIM_OBJ_FMT*/
					break;
				default:
					/* should NEVER occur except for
					 * DISPLACEMENT relocations (any other
					 * type should have be taken care of in
					 * asm_relocation() ).
					 */
					internal_error("write_relocs(): r_symp==NULL");
				}
			}
			else
			{

#define PIC_REL(r) (r==RELOC_BASE10 || r==RELOC_BASE13 || r==RELOC_BASE22 || \
			r==RELOC_PC10 || r==RELOC_PC22)

				/* if symbol is defined in this module and
				 * it is not a special pic-relocation,
				 * convert reference to it into a
				 * segment-relative relocation
				 */
				if ( BIT_IS_ON(rlp->rl_reloc.r_symp->s_attr,
						SA_DEFINED) &&
				     !(picflag && PIC_REL(rel.r_type))
				   )
#undef PIC_REL
				{
					rel.r_extern = 0;

					if (rlp->rl_reloc.r_symp->s_segment
						== Text_segno)
					{
#ifdef INTERIM_OBJ_FMT
						rel.r_index/*seg*/ = SEG_TEXT;
#else /*INTERIM_OBJ_FMT*/
						rel.r_index/*seg*/ = N_TEXT;
#endif /*INTERIM_OBJ_FMT*/
					}
					else if(rlp->rl_reloc.r_symp->s_segment
						== Data_segno)
					{
#ifdef INTERIM_OBJ_FMT
						rel.r_index/*seg*/ = SEG_DATA;
#else /*INTERIM_OBJ_FMT*/
						rel.r_index/*seg*/ = N_DATA;
						rlp->rl_reloc.r_addend +=
							Text_size;
#endif /*INTERIM_OBJ_FMT*/
					}
#ifdef INTERIM_OBJ_FMT
					else if(rlp->rl_reloc.r_symp->s_segment
						== Sdata_segno)
					{
						rel.r_index/*seg*/ = SEG_SDATA;
					}
#endif /*INTERIM_OBJ_FMT*/
					else if(rlp->rl_reloc.r_symp->s_segment
						== Bss_segno)
					{
#ifdef INTERIM_OBJ_FMT
						rel.r_index/*seg*/ = SEG_BSS;
#else /*INTERIM_OBJ_FMT*/
						rel.r_index/*seg*/ = N_BSS;
						rlp->rl_reloc.r_addend +=
							Text_size + Data_size;
#endif /*INTERIM_OBJ_FMT*/
					}
					else internal_error("write_relocs(): reloc to seg %d?",
								rlp->rl_reloc.r_symp->s_segment);

					/*relative to beginning of the segment*/
					rlp->rl_reloc.r_addend +=
						rlp->rl_reloc.r_symp->s_value;
				}
				else
				{
					/* The symbol was not defined in this
					 * module, therefore must either be
					 * defined in another module
					 * (Externally defined), or it is a PIC
					 * relocation.
					 */
					if (BIT_IS_ON(rlp->rl_reloc.r_symp->s_attr,SA_DEFINED) &&
					    BIT_IS_OFF(rlp->rl_reloc.r_symp->s_attr,SA_GLOBAL))
						rel.r_extern = 0;
					else
						rel.r_extern = 1;
					rel.r_index  =
						rlp->rl_reloc.r_symp->s_symidx;
#ifdef DEBUG
					if (rlp->rl_reloc.r_symp->s_symidx
						==SYMIDX_NONE)
					{
						/* Yikes! We're trying to write
						 * out a relocation against a
						 * symbol which won't even
						 * appear in the object file!
						 */
						internal_error("write_relocs(): s_symidx == <none>");
					}
#endif
				}
			}

			rel.r_addend = rlp->rl_reloc.r_addend;

			if ( fwrite(&rel, sizeof(rel), 1, ofp) != 1 )
			{
				perror(pgm_name);/* print the sys err message */
				error(ERR_WRITE_ERROR, NULL, 0, objfilename);
				return 0;
			}
			bytes_written += sizeof(rel);

#ifdef DEBUG
			if (debug)
			{
				char *symname;
				if (rlp->rl_reloc.r_symp == NULL)
				{
					symname = "<no sym>";
				}
				else
				{
					symname =
						rlp->rl_reloc.r_symp->s_symname;
				}

				fprintf(stderr,
					"\t[ld relocation: %-11s of %s+%d(0x%x) @ \"%s\"+%d(0x%x)]\n",
					get_reloc_name(rlp->rl_reloc.r_type),
					symname,
					rlp->rl_reloc.r_addend,
					rlp->rl_reloc.r_addend,
					get_seg_name(rlp->rl_reloc.r_segment),
					rlp->rl_reloc.r_offset,
					rlp->rl_reloc.r_offset
			       );
			}
#endif

			*rlpp = rlp->rl_next;	/* is also "incr" for loop */
			/** 
			 ** The following call to as_free() is "correct",
			 ** but it is unnecessary, since by the time we are
			 ** writing relocations to the output file, there will
			 ** be few, if any, more malloc()s.  So there is no
			 ** point in doing costly free()s.
			 **
			 ** as_free(rlp);
			 **/
		}
		else
		{
			/* just ignore the relocation, this time thru */
			rlpp = &(rlp->rl_next);	/* "increment" for loop */
		}
	}

	return bytes_written;
}



/* asm_relocation() performs a relocation in the assembler (as opposed to
 * passing it on to the linker), if it can.
 *
 * It returns TRUE if it succeeded.
 */
static Bool
asm_relocation(rlp)
	register struct rel_list *rlp;
{
	register struct symbol *symp;
	register LOC_COUNTER	offset;
	register SEGMENT	segment;
	register S32BITS value;
	register U32BITS objword;
	Bool	 asm_reloc_succeeded;
#ifdef DEBUG
	UINT	reloc_len;
#endif

	asm_reloc_succeeded = FALSE;

	symp    = rlp->rl_reloc.r_symp;
	segment = rlp->rl_reloc.r_segment;
	offset  = rlp->rl_reloc.r_offset;

	if ( (symp != NULL) &&
	     BIT_IS_ON(symp->s_attr, SA_DEFINED) &&
	     SYMBOL_IS_ABSOLUTE_VALUE(symp)
	   )
	{
		value = symp->s_value + rlp->rl_reloc.r_addend;
		symp = NULL;
	}
	else	value = rlp->rl_reloc.r_addend;

	/*
	 * Do any relocations that the Assembler has enough information	
	 * available to perform.
	 *
	 * For most types of relocation, only an absolute value will do.
	 *
	 * For the Displacement types of relocation, the Assembler can do
	 * the relocation if the Displacement is between two locations in
	 * the same segment, in the current module.
	 */
	switch (rlp->rl_reloc.r_type)
	{
	case REL_LO10:		/* low 10 bits of value	*/
		if (symp == NULL)
		{
			objword = get_obj_word(segment, offset);
#ifdef CHIPFABS
			if (iu_fabno == 1)
			{
				/* make it 9-bit instead */
				objword = (objword & (~MASK(9))) |
					  ( value  &   MASK(9) );
			}
			else	objword = (objword & (~MASK(10))) |
					  ( value  &   MASK(10) );
#else /*CHIPFABS*/
			objword = (objword & (~MASK(10))) | (value & MASK(10));
#endif /*CHIPFABS*/
			put_obj_word(segment, offset, objword);
			asm_reloc_succeeded = TRUE;
#ifdef DEBUG
			reloc_len = 4;
#endif
		}
		break;

	case REL_13:		/* a 13-bit value	*/
		if (symp == NULL)
		{
			if ( (value < -0x1000) || (value > 0x0fff) )
				     /*-4096*/            /*4095*/
			{
				error(ERR_VAL_13,rlp->rl_fnamep,rlp->rl_lineno);
			}
			else
			{
				objword = get_obj_word(segment, offset);
				objword = (objword & (~MASK(13))) |
						(value & MASK(13));
				put_obj_word(segment, offset, objword);
				asm_reloc_succeeded = TRUE;
#ifdef DEBUG
				reloc_len = 4;
#endif
			}
		}
		break;

	case REL_22:		/* a 22-bit value	*/
		if (symp == NULL)
		{
#ifdef CHIPFABS
			if (iu_fabno == 1)
			{
				if ( (value < -(MASK(23-1)+1)) ||
				     (value >    MASK(23)    ) )
				{
					error(ERR_VAL_23, rlp->rl_fnamep,
							  rlp->rl_lineno);
				}
				else
				{
					objword = get_obj_word(segment, offset);
					objword = (objword & (~MASK(23)) ) |
						  ( value  &   MASK(23)  );
					put_obj_word(segment, offset, objword);
				}
			}
			else
			{
#endif /*CHIPFABS*/
			if ( (value < -(MASK(22-1)+1)) || (value > MASK(22)) )
			{
				error(ERR_VAL_22,rlp->rl_fnamep,rlp->rl_lineno);
			}
			else
			{
				objword = get_obj_word(segment, offset);
				objword = (objword & (~MASK(22)) ) |
					  ( value  &   MASK(22)  );
				put_obj_word(segment, offset, objword);
			}
#ifdef CHIPFABS
			}
#endif /*CHIPFABS*/

			asm_reloc_succeeded = TRUE;
		}
#ifdef DEBUG
		reloc_len = 4;
#endif
		break;

	case REL_HI22:		/* high 22 bits of value*/
		if (symp == NULL)
		{
			objword = get_obj_word(segment, offset);
#ifdef CHIPFABS
			if (iu_fabno == 1)
			{
				/* make a hi-23 instead */
				value = value >>  9;
				objword = (objword & (~MASK(23))) |
					  ( value  &   MASK(23) );
			}
			else
			{
				value = value >> 10;
				objword = (objword & (~MASK(22))) |
					  ( value  &   MASK(22) );
			}
#else /*CHIPFABS*/
			value = value >> 10;	/* get high 22 bits */
			objword = (objword & (~MASK(22))) | (value & MASK(22));
#endif /*CHIPFABS*/
			put_obj_word(segment, offset, objword);
			asm_reloc_succeeded = TRUE;
#ifdef DEBUG
			reloc_len = 4;
#endif
		}
		break;

	case REL_DISP22:	/* 22-bit signed word-displacement */
		if ( ((symp != NULL) &&
		     /* test for DEFINED is redundant with segment	*/
		     /* comparision, because undefined symbol should	*/
		     /* have segment = ASSEG_UNKNOWN.			*/
		     /* BIT_IS_ON(symp->s_attr, SA_DEFINED) &&		*/
		     (symp->s_segment == segment))
		   )
		{
			register S32BITS disp;

			disp = (symp->s_value + value) - offset;
			if ((disp & 03) != 0)
			{
				/* not a word displacement! */
				error(ERR_NOT_WDISP, rlp->rl_fnamep,
					rlp->rl_lineno);
			}
			else if (
			     ((disp <  0) && ((disp & 0xffa00000)!=0xffa00000))
						||
			     ((disp >= 0) && ((disp & 0xffa00000)!=0))
			        )
			{
				error(ERR_DISP2BIG, rlp->rl_fnamep,
					rlp->rl_lineno);
				disp = 0;
			}
			
			objword = get_obj_word(segment, offset);
#ifdef CHIPFABS
				if (iu_fabno == 1)
				{
					/* make it a 23-bit displac't instead */
					objword = (  objword   & (~MASK(23)) ) |
						  ((disp >> 2) &   MASK(23)  );
				}
				else	objword = (  objword   & (~MASK(22)) ) |
						  ((disp >> 2) &   MASK(22)  );
#else /*CHIPFABS*/
			objword = (  objword   & (~MASK(22)) ) |
				  ((disp >> 2) &   MASK(22)  );
#endif /*CHIPFABS*/
			put_obj_word(segment, offset, objword);

			asm_reloc_succeeded = TRUE;
#ifdef DEBUG
			reloc_len = 4;
#endif
		}
		break;

	case REL_DISP30:	/* 30-bit signed word-displacement */
		if ( ((symp != NULL) &&
		     /* test for DEFINED is redundant with segment	*/
		     /* comparision, because undefined symbol should	*/
		     /* have segment = ASSEG_UNKNOWN.			*/
		     /* BIT_IS_ON(symp->s_attr, SA_DEFINED) &&		*/
		     (symp->s_segment == segment))
		   )
		{
			register S32BITS disp;

			disp = (symp->s_value + value) - offset;
			if ( (disp & 03) != 0 )
			{
				error(ERR_NOT_WDISP, rlp->rl_fnamep,
					rlp->rl_lineno);
			}
			
			/* no test for "displacement too big" here, as for
			 * the 22-bit displacement, because there is no
			 * displacement too big for 30 bits of word offset
			 * to handle (i.e. 30-bit WORD displacement can
			 * reach the entire address space).
			 */
			
			objword = get_obj_word(segment, offset);
			objword = (  objword   & (~MASK(30)) ) |
				  ((disp >> 2) &   MASK(30)  );
			put_obj_word(segment, offset, objword);

			asm_reloc_succeeded = TRUE;
#ifdef DEBUG
			reloc_len = 4;
#endif
		}
		break;

	case REL_SFA_BR:	/* base register for Short-Form Addr'g */
		/* for an absolute address in the proper range, the assembler
		 * can generate a %g0-relative reference.
		 */
		if ( (symp == NULL) && (value >= -0x1000) && (value <= 0x0fff) )
				                 /*-4096*/            /*4095*/
		{
			objword = get_obj_word(segment, offset);
			objword = (objword & (~MASK_RS1)) |
				  (RAW_REGNO(RN_GP(0))<<14);
			put_obj_word(segment, offset, objword);
			asm_reloc_succeeded = TRUE;
# ifdef DEBUG
			reloc_len = 4;
# endif
		}
		else ;/*leave it for the linker to handle*/
		break;

	case REL_SFA_OF: /*offset from base register, for Short-Form Addr'g*/
		/* for an absolute address in the proper range, the assembler
		 * can generate a %g0-relative reference.
		 */
		if ( (symp == NULL) && (value >= -0x1000) && (value <= 0x0fff) )
				                 /*-4096*/            /*4095*/
		{
			objword = get_obj_word(segment, offset);
			objword = (objword & (~MASK(13))) | (value & MASK(13));
			put_obj_word(segment, offset, objword);
			asm_reloc_succeeded = TRUE;
# ifdef DEBUG
			reloc_len = 4;
# endif
		}
		else ;/*leave it for the linker to handle*/
		break;

	case REL_8:		/* 8-bit value		*/
		if (symp == NULL)
		{
			if ( (value < -127) || (value > 255) )
			{
				error(ERR_VAL_NOFIT, rlp->rl_fnamep,
					rlp->rl_lineno, 8);
			}
			else
			{
				*(get_obj_code_ptr(segment, offset)) = value;
				asm_reloc_succeeded = TRUE;
#ifdef DEBUG
				objword = value;
				reloc_len = 1;
#endif
			}
		}
		break;

	case REL_16:		/* 16-bit value		*/
		if (symp == NULL)
		{
					/*-32K*/	      /*+64K*/
			if ( (value < -(0x8000L)) || (value > 0xffffL) )
			{
				error(ERR_VAL_NOFIT, rlp->rl_fnamep,
					rlp->rl_lineno, 16);
			}
			else
			{
				register U16BITS *hwp;	/* half-word pointer */
				hwp = (U16BITS *)
					get_obj_code_ptr(segment, offset&(~01));
				*hwp = value;
				asm_reloc_succeeded = TRUE;
#ifdef DEBUG
				objword = value;
				reloc_len = 2;
#endif
			}
		}
		break;

	case REL_32:		/* 32-bit value		*/
		if (symp == NULL)
		{
			put_obj_word(segment, offset, (U32BITS)value);
			asm_reloc_succeeded = TRUE;
#ifdef DEBUG
			objword = value;
			reloc_len = 4;
#endif
		}
		break;

	case REL_BASE10:
	case REL_BASE13:
	case REL_BASE22:
	case REL_PC10:
	case REL_PC22:
	case REL_JMP_TBL:
		/* Can't do these in the assembler, at all. */
		asm_reloc_succeeded = FALSE;
		break;

	default:
		internal_error("asm_relocation(): reloc type %d?",
				rlp->rl_reloc.r_type);
	}

#ifdef DEBUG
	if (debug)
	{
		if (asm_reloc_succeeded)
		{
			fprintf(stderr,
				"\t[asm_relocation(): %s in \"%s\"+%d(0x%x); new=0x%0*x]\n",
				get_reloc_name(rlp->rl_reloc.r_type),
				get_seg_name(segment),
				offset, offset,
				reloc_len*2, /* #bytes * 2 ==> # hex digits */
				objword
			       );
		}
	}
#endif

	return asm_reloc_succeeded;
}


static U32BITS
get_obj_word(segno, seg_off)
	register SEGMENT segno;
	register U32BITS seg_off;
{
	return ( *((U32BITS *)get_obj_code_ptr(segno, seg_off&(~03))) );
}


static void
put_obj_word(segno, seg_off, word)
	register SEGMENT segno;
	register U32BITS seg_off;
	register U32BITS word;
{
	*((U32BITS *)get_obj_code_ptr(segno, seg_off&(~03))) = word;
}
