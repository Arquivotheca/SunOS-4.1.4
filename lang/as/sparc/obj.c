static char sccs_id[] = "@(#)obj.c	1.1\t10/31/94";
/*
 *	This file contains functions which interface
 *	with the output object file.
 */

#include <stdio.h>
#include <sun4/a.out.h>

#include "structs.h"
#include "errors.h"
#include "globals.h"
#include "segments.h"
#ifdef SUN3OBJ
#  include "bitmacros.h"	/* for objsym_write_symbols() */
#  include "sym.h"		/* for objsym_write_symbols() & adjust_*() */
#endif
#include "stab.h"
#include "relocs.h"

#define SYM_TMP   0	/* index into objtmpfilename: symbol table */
#define STR_TMP   1	/* index into objtmpfilename: string table */
#define MAX_OBJ_TMP_FILES  2

#ifdef SUN3OBJ
#  define MAX_ALT_DATASEGS 4			/*data, data1, data2, NULL ptr*/
#  define MAX_CONCAT_SEGS  (1+MAX_ALT_DATASEGS)	/* Text, plus the above */
#endif


static S32BITS objsym_write_symbols();	/* forward routine declaration */
static U32BITS tmpfile_to_objfile();	/* forward routine declaration */
static void objsym_cleanup();		/* forward routine declaration */
static U32BITS objsym_sym_to_objfile();	/* forward routine declaration */
static U32BITS objsym_str_to_objfile();	/* forward routine declaration */


/* temporary object filenames in use (so can cleanup) */
static char objtmpfilename[MAX_OBJ_TMP_FILES][20] = { { '\0' } };

/* "objfile_is_incomplete" is used to make sure that if we catch an
 * interrupt after we have written a partial object file, we remove
 * it before we exit.  (Otherwise, the user's LD's and MAKE's would
 * get messed up).
 */
static Bool objfile_is_incomplete = FALSE;


/* When combining data segments (e.g. data, data1, data2) into one data
 * segment, we must adjust the values of the symbols in all but the first
 * one linked in.
 */
#ifdef SUN3OBJ
static void
adjust_concat_symbols(cs_seg_count, cs_seg_offsets, cs_segnos)
	int	cs_seg_count;	/* the # of concatenated segments */
		/* offsets from beginning of the first concatenated segment */
	unsigned long int cs_seg_offsets[MAX_CONCAT_SEGS];
	SEGMENT cs_segnos[MAX_CONCAT_SEGS]; /* segment #s of the seg's*/
{
	register struct symbol *symp;
	register int i;

	for ( symp = sym_first();  symp != NULL;  symp = sym_next() )
	{
		if ( BIT_IS_ON(symp->s_attr, SA_DEFINED) )
		{
			/* Start "i" at 1 here, because there will be no
			 * change for symbols referencing the FIRST
			 * concatenated segment.
			 */
			for (i = 1;  i < cs_seg_count;  i++)
			{
				if (symp->s_segment == cs_segnos[i])
				{
					/* Change the symbol's segment to be
					 * that of the first one in the
					 * concatenation.
					 */
					symp->s_segment = cs_segnos[0];

					/* Adjust the symbol offset, to be from
					 * beginning of the FIRST segment,
					 * instead of from its own beginning.
					 */
					symp->s_value += cs_seg_offsets[i];

					break;	/* ...out of inner "for" loop */
				}
			}
		}
	}
}
#endif

#ifdef SUN3OBJ
static SEGMENT ok_dseg_segnos[MAX_ALT_DATASEGS];
static char *(ok_dseg_names[MAX_ALT_DATASEGS]) = {"data","data1","data2",NULL};
static char *(concat_seg_names[MAX_CONCAT_SEGS]) = { NULL };

Bool
sun3_seg_name_ok(name)
	register char *name;
{
	register int i;

	if ((strcmp(name, "text" ) == 0) ||
#ifdef INTERIM_OBJ_FMT
	    (strcmp(name, "sdata") == 0) ||
#endif /*INTERIM_OBJ_FMT*/
	    (strcmp(name, "bss"  ) == 0))
	{
		return TRUE;
	}

	for (i = 0;   ok_dseg_names[i] != NULL;   i++)
	{
		if( strcmp(name, ok_dseg_names[i]) == 0 )	return TRUE;
	}

	return FALSE;
}
#endif

#ifdef SUN3OBJ
SEGMENT Text_segno = ASSEG_UNKNOWN;
SEGMENT Data_segno = ASSEG_UNKNOWN;
SEGMENT Bss_segno  = ASSEG_UNKNOWN;
U32BITS Text_size  = 0;
U32BITS Data_size  = 0;
U32BITS Bss_size   = 0;
#ifdef INTERIM_OBJ_FMT
SEGMENT Sdata_segno= ASSEG_UNKNOWN;
U32BITS Sdata_size = 0;
#endif /*INTERIM_OBJ_FMT*/
# ifdef DEBUG
Bool Seg_nums_set  = FALSE;	/* *_segno (above) are valid */
Bool Seg_sizes_set = FALSE;	/* all *_size  (above) are valid */
# endif /*DEBUG*/
#endif /*SUN3OBJ*/

unsigned char
get_toolversion()
{
#ifdef CHIPFABS
	unsigned char tv;
	/* iu_fabno & fpu_fabno  are global variables */

	if (TV_SUN4 <= 0x86)
	{
		tv = (iu_fabno == 1) ?(TV_SUN4-1) :TV_SUN4 ;
	}
	else
	{
		static char IuFpuMsg[] = "write_obj_file(): iu%d/fpu%d?";

		switch (iu_fabno)
		{

		case 1:	/* First-fab IU chips */
			switch(fpu_fabno)
			{
			case 1:	/* IU-1, FPU-1 */
				tv = TV_SUN4 - 2;
				break;
			default:
				internal_error(IuFpuMsg, iu_fabno, fpu_fabno);
			}
			break;

		case 0:	/* "Perfect" IU chips */
		case 2:	/* Second-fab IU chips */
			switch (fpu_fabno)
			{
			case 1:	/* IU-2, FPU-1 */
				tv = TV_SUN4 - 1;
				break;
			case 2:	/* IU-2, FPU-2 */
				fprintf(stderr, "[warning: FPU-2 obj file will be indistinguishable (magic#) from FPU-3 obj file]\n");
				/* fall through */
			case 0:	/* IU-2, "perfect" FPU */
			case 3:	/* IU-2, FPU-3 */
				tv = TV_SUN4;
				break;
			default:
				internal_error(IuFpuMsg, iu_fabno, fpu_fabno);
			}
			break;

		default:
			internal_error(IuFpuMsg, iu_fabno, fpu_fabno);
		}
	}

	return tv;
#else /*CHIPFABS*/
	return TV_SUN4;
#endif /*CHIPFABS*/
}

void
write_obj_file()
{
	register FILE *ofp;	/* output object file pointer	 */
#ifdef INTERIM_OBJ_FMT
	struct exec_sun4 objheader;
#else /*INTERIM_OBJ_FMT*/
	struct exec objheader;
#endif /*INTERIM_OBJ_FMT*/
#ifdef SUN3OBJ
	SEGMENT cs_segnos[MAX_CONCAT_SEGS];
		/* offsets from beginning of first concatenated segment */
	unsigned long int cs_seg_offsets[MAX_CONCAT_SEGS];
	int	cs_seg_count;	/* the # of concatenated segments */
#endif /*SUN3OBJ*/

	if (error_count == 0)
	{
		register int i;

		objfile_is_incomplete = TRUE;

		/* open the object file */
		if ( (ofp = fopen(objfilename, "w")) == NULL )
		{
			perror(pgm_name);	/* print the sys err message */
			error(ERR_CANT_OPEN_O, NULL, 0, objfilename);
			return;
		}

		close_all_segments();

		Text_segno = get_seg_number("text");
		Data_segno = get_seg_number("data");
		Bss_segno  = get_seg_number("bss" );
#ifdef INTERIM_OBJ_FMT
		Sdata_segno= get_seg_number("sdata");
#endif /*INTERIM_OBJ_FMT*/

		Text_size = get_seg_size(Text_segno);
		Bss_size  = get_seg_size(Bss_segno);
#ifdef INTERIM_OBJ_FMT
		Sdata_size= get_seg_size(Sdata_segno);
#endif /*INTERIM_OBJ_FMT*/

#ifdef SUN3OBJ
		/* Now, set up the list of segments, if any, to be
		 * concatenated.  Note that order is important -- it is done in
		 * the order which they will be written out to the object file.
		 */
		cs_seg_count = 0;	/* We start with none, of course. */

		if (make_data_readonly)
		{
			cs_segnos[cs_seg_count] = Text_segno;
			cs_seg_offsets[cs_seg_count] = 0;
			cs_seg_count++;
		}

		Data_size = 0;
		for (i = 0;   ok_dseg_names[i] != NULL;   i++)
		{
			/* get its segment #, if it exists */
			ok_dseg_segnos[i] = lookup_seg_number(ok_dseg_names[i]);

			/* If the segment exists,
			 *	add its length onto the total concatenated seg
			 *	length, and hold onto its segment # and offset
			 *	from the beginning of the the FIRST segment
			 *	in the concatenation.
			 */
			if ( ok_dseg_segnos[i] != ASSEG_UNKNOWN )
			{
				cs_segnos[cs_seg_count] = ok_dseg_segnos[i];
				if (make_data_readonly)
				{
					/* Concatenating onto Text segment. */
					cs_seg_offsets[cs_seg_count] =Text_size;
					Text_size += get_seg_size(
						cs_segnos[cs_seg_count]);
				}
				else
				{
					/* Concatenating onto Data segment. */
					cs_seg_offsets[cs_seg_count] =Data_size;
					Data_size += get_seg_size(
						cs_segnos[cs_seg_count]);
				}
				cs_seg_count++;
				if (cs_seg_count > MAX_CONCAT_SEGS)
				{
					internal_error("write_obj_file(): too many concat'd segments");
				}
			}
		}
#else /*SUN3OBJ*/
		Data_size = get_seg_size(Data_segno);
#endif /*SUN3OBJ*/

#ifdef SUN3OBJ
#  ifdef DEBUG
		Seg_nums_set  = TRUE;	/* *_segno (above) are valid */
		Seg_sizes_set = TRUE;	/* *_size  (above) are valid */
#  endif /*DEBUG*/
#endif /*SUN3OBJ*/


		/* Setup the header fields known, so far. */
		objheader.a_dynamic = 0;
		objheader.a_toolversion = get_toolversion();
		objheader.a_machtype = M_SPARC;
		objheader.a_magic    = OMAGIC;
        	objheader.a_text     = Text_size;/* size of text segment      */
        	objheader.a_data     = Data_size;/* size of initialized data  */
        	objheader.a_bss      = Bss_size; /* size of uninitialized data*/
#ifdef INTERIM_OBJ_FMT
        	objheader.a_sdata    = Sdata_size;/*size of small-data seg.   */
#else /*INTERIM_OBJ_FMT*/
		if ( get_seg_size(get_seg_number("sdata")) != 0 )
		{
			/* Something is in small-data segment!   */
			internal_error(
				"write_obj_file(): Small-Data segment present");
		}        		
#endif /*INTERIM_OBJ_FMT*/

		/* -------------------------------------- */
		/* Finally(!), write out the object file. */
		/* -------------------------------------- */

#ifdef SUN3OBJ
#  ifdef INTERIM_OBJ_FMT
		fseek(ofp, (long)(N_TXTOFF_SUN4(objheader)), 0);
#  else /*INTERIM_OBJ_FMT*/
		fseek(ofp, (long)(N_TXTOFF(objheader)), 0);
#  endif /*INTERIM_OBJ_FMT*/
		ABSwrite_seg(Text_segno, ofp);	/* write out the text segment */

#  ifdef INTERIM_OBJ_FMT
		/* append the Small-Data segment to the object file	  */
		ABSwrite_seg(Sdata_segno, ofp);
#  endif /*INTERIM_OBJ_FMT*/

		/* append the data segments, if any */
		for (i = 0;   ok_dseg_names[i] != NULL;   i++)
		{
			ABSwrite_seg(ok_dseg_segnos[i], ofp);
		}

		/* And, of course, there is nothing to actually write out for
		 * the bss segment.
		 */
#else
		fseek(ofp, (long)/*???*/, 0);
		write_segments(ofp);	/* write out the segments of code */
#endif

        	objheader.a_entry = 0;	/* The entry point. */

#ifdef SUN3OBJ
		/* If there were segments to be concatenated, adjust symbols
		 * referring to all but the first of those segments to be
		 * relative to the beginning of the FIRST one, instead of to
		 * their own beginning.
		 *
		 * This must be done before "objsym_write_symbols()" is called,
		 * for the obvious reasons.
		 *
		 * Also, adjust all relocations for the segments to refer to
		 * the new combined segment.
		 *
		 * This must be done BEFORE "write_relocs()" is called,
		 * so that all symbols to which relocations refer will be
		 * marked with the correct segment.
		 */
		if ( cs_seg_count > 1 )
		{
			adjust_concat_relocs(cs_seg_count, cs_seg_offsets,
						cs_segnos);
		}
		adjust_concat_symbols(cs_seg_count, cs_seg_offsets, cs_segnos);
#endif
		/* change relocation types to be pic */
		if (picflag){
			pic_relocs(Text_segno);
		}

		/* write symbol table to temp file */
        	objsym_write_symbols();

		/* write text relocations, and remember how many */
        	objheader.a_trsize   = write_relocs(Text_segno, ofp);

#ifdef INTERIM_OBJ_FMT
		/* write small-data relocations, and remember how many */
        	objheader.a_sdrsize  = write_relocs(Sdata_segno, ofp);

#endif /*INTERIM_OBJ_FMT*/
		/* write data relocations, and remember how many */
        	objheader.a_drsize   = write_relocs(Data_segno, ofp);

		/* append the symbol-table temporary file to the obj file */
        	objheader.a_syms     = objsym_sym_to_objfile(ofp, objfilename);

		/* append the string-table temporary file to the obj file */
#ifdef INTERIM_OBJ_FMT
        	objheader.a_string   = objsym_str_to_objfile(ofp, objfilename);
        	objheader.a_spare1    = 0;
        	objheader.a_spare2    = 0;
        	objheader.a_spare3    = 0;
        	objheader.a_spare4    = 0;
        	objheader.a_spare5    = 0;
#else /*INTERIM_OBJ_FMT*/
        	(void)objsym_str_to_objfile(ofp, objfilename);
#endif /*INTERIM_OBJ_FMT*/
                /* blat out .ident krap, if appropriate */
                (void)ident_to_objfile(ofp, objfilename);

		fseek(ofp, 0L, 0);	/* now come back and write the header */
		if ( fwrite(&objheader, sizeof(objheader), 1, ofp) != 1 )
		{
			perror(pgm_name);	/* print the sys err message */
			error(ERR_WRITE_ERROR, NULL, 0, objfilename);
			return;
		}

		objsym_cleanup();	/* close the /tmp files		*/
		fclose(ofp);		/* close the object file	*/
		objfile_is_incomplete = FALSE;	/* we finished it!	*/
	}
}


static U32BITS		/* Returns the number of bytes copied. */
tmpfile_to_objfile(sfp, ofp, output_filename)
	register FILE *sfp;
	register FILE *ofp;
	register char *output_filename;
{
	register int blocklen;
	register U32BITS nbytes;
#define COPYBUFSZ 8192	/* may not be optimum size, but it should be good */
	char buf[COPYBUFSZ];
	long int ofile_offset;
	extern long int ftell();

	/* flush these files, so can switch to low-level I/O for speed */
	fflush(sfp);
	fflush(ofp);
	ofile_offset = ftell(ofp);
	
	rewind(sfp); /* back up to beginning of source file */
	
	/* copy the file */
	for ( nbytes = 0;
	      ((blocklen = read(fileno(sfp), &buf[0], COPYBUFSZ)) > 0);
	      nbytes += blocklen
	    )
	{
		if ( write(fileno(ofp), &buf[0], blocklen) != blocklen )
		{
			perror(pgm_name);	/* print the sys err message */
			error(ERR_WRITE_ERROR, NULL, 0, output_filename);
			return 0;
		}
		ofile_offset += blocklen;
	}

	/* make sure standard I/O is resynchronized (seek to end of file) */
	fseek(ofp, ofile_offset, 0);

	return nbytes;	/* number of bytes copied */
}


#ifdef SUN3OBJ

FILE *objsym_symfp = NULL;	/* file pointer for /tmp symbol-table file */
FILE *objsym_strfp = NULL;	/* file pointer for /tmp string-table file */
S32BITS objsym_str_offset;
SymbolIndex objsym_symtab_idx;

static U32BITS
objsym_sym_to_objfile(ofp, output_filename)
	register FILE *ofp;
	register char *output_filename;
{
	/* Copy the SYMBOL temporary file onto the object file. */
	return tmpfile_to_objfile(objsym_symfp, ofp, output_filename);
}

static U32BITS
objsym_str_to_objfile(ofp, output_filename)
	register FILE *ofp;
	register char *output_filename;
{
	/* Copy the STRING temporary file onto the object file. */
	return tmpfile_to_objfile(objsym_strfp, ofp, output_filename);
}

void
objsym_init()
{
	int pid;
	extern int getpid();

	pid = getpid();

	/* open the temporary symbol-table file */
	sprintf(objtmpfilename[SYM_TMP], "/tmp/as_sym_%05d", pid);
	if ( (objsym_symfp = fopen(objtmpfilename[SYM_TMP], "w+")) == NULL )
	{
		perror(pgm_name);	/* print the sys err message */
		error(ERR_CANT_OPEN_O, NULL, 0, objtmpfilename[SYM_TMP]);
		return;
	}

	/* open the temporary string-table file */
	sprintf(objtmpfilename[STR_TMP], "/tmp/as_str_%05d", pid);
	if ( (objsym_strfp = fopen(objtmpfilename[STR_TMP], "w+")) == NULL )
	{
		perror(pgm_name);	/* print the sys err message */
		error(ERR_CANT_OPEN_O, NULL, 0, objtmpfilename[STR_TMP]);
		return;
	}

	/* skip over its first 4 bytes of the string table,
	 * into which we will stuff the string table's length, later.
	 */
	fseek(objsym_strfp, 4L, 0);
	objsym_str_offset = 4;

	objsym_symtab_idx = 0;	/* no symbols have been written out yet */
}


SymbolIndex	/* returns index of symbol as written in a.out file */
objsym_write_aout_symbol(asymp)
#ifdef INTERIM_OBJ_FMT
	register struct aout_symbol *asymp;
#else 
	register struct nlist *asymp;
#endif
{
	register char *p;
	register int i;
#ifdef DEBUG
	static Bool asym_initialized = FALSE;
	static char *fcn_name = "objsym_write_aout_symbol()";
#endif /*DEBUG*/

	/* The first time this routine is called objsym_symfp will be NULL,
	 * which will cause all of the temporary files to get opened.
	 */
	if (objsym_symfp == NULL)	objsym_init();

	/* Write the symbol name into the string segment. */
#ifdef INTERIM_OBJ_FMT
	if (asymp->s_string.ptr != NULL)
	{
		p = asymp->s_string.ptr;
		asymp->s_string.strx = objsym_str_offset; /*clobbers .n_name*/
#else /*INTERIM_OBJ_FMT*/
	if (asymp->n_un.n_name != NULL)
	{
		p = asymp->n_un.n_name ;
		asymp->n_un.n_strx = objsym_str_offset; /*clobbers .n_name*/
#endif /*INTERIM_OBJ_FMT*/
		do
		{
			putc(*p, objsym_strfp);

			/* incrementing a counter is "cheaper" than doing an
			 * "ftell(objsym_strfp)" before each symbol is written
			 * out.
			 */
			objsym_str_offset++;

		} while (*p++ != '\0');
	}

#if defined(SUN3OBJ) && !defined(INTERIM_OBJ_FMT)
	/* Have to make kludgy adjustments to Data and BSS symbol values. */
	switch (asymp->n_type & N_TYPE)
	{
#ifdef DEBUG
	case N_DATA:
		if (!Seg_sizes_set)
			internal_error("%s: invalid Data seg size", fcn_name);
		asymp->n_value += Text_size;
		break;
	case N_BSS:
		if (!Seg_sizes_set)
			internal_error("%s: invalid Bss seg size", fcn_name);
		asymp->n_value += Text_size + Data_size;
		break;
	default:	break;	/* no kludge needed. */
#else /*DEBUG*/
	case N_DATA:	asymp->n_value += Text_size;			break;
	case N_BSS:	asymp->n_value += Text_size + Data_size;	break;
	default:	break;	/* no kludge needed. */
#endif /*DEBUG*/
	}
#endif /* defined(SUN3OBJ) && !defined(INTERIM_OBJ_FMT) */

	if ( fwrite(asymp, sizeof(*asymp), 1, objsym_symfp) != 1 )
	{
		perror(pgm_name);	/* print the sys err message */
		error(ERR_WRITE_ERROR, NULL, 0, objtmpfilename[SYM_TMP]);
		return 0;
	}

	return objsym_symtab_idx++;
}


static void
objsym_write_sym(symp)
	register struct symbol *symp;
{
	register char *p;
	register int i;
#ifdef INTERIM_OBJ_FMT
	static struct aout_symbol asym;
#else
	static struct nlist asym;
#endif
	static Bool asym_initialized = FALSE;

	/* The first time this routine is called, "asym" will get initialized.*/
	if (!asym_initialized)
	{
		/* Just-to-be-safe, zero the initial symbol record, so that any
		 * unused fields (e.g. uninitialized bitfields) are zeroed.
		 */
		bzero(&asym, sizeof(asym));
		asym_initialized = TRUE;
	}

#if defined(SUN3OBJ) && defined(DEBUG)
	if (!Seg_nums_set)	internal_error("objsym_write_sym(): invalid Seg values");
#endif /*SUN3OBJ && DEBUG*/

	if (
		/* write out a symbol only if we haven't marked it NOT
		 * to be written out.
		 */
	     BIT_IS_OFF(symp->s_attr, SA_NOWRITE) &&
		/* write out global syms,
		 * and local syms which are defined in this module.
		 */
	     ( BIT_IS_ON(symp->s_attr, SA_GLOBAL)  ||
	       BIT_IS_ON(symp->s_attr, SA_DEFINED) 
	     )
	   )
	{
#ifdef INTERIM_OBJ_FMT
		asym.s_string.ptr = symp->s_symname;
		asym.s_value      = symp->s_value;
#else /*INTERIM_OBJ_FMT*/
		asym.n_un.n_name = symp->s_symname;
		asym.n_value     = symp->s_value;
#endif /*INTERIM_OBJ_FMT*/
		if ( BIT_IS_ON(symp->s_attr, SA_DEFINED) )
		{
			if ( SYMBOL_IS_ABSOLUTE_VALUE(symp) )
			{
#ifdef INTERIM_OBJ_FMT
				asym.s_segment = SEG_ABS;
#else /*INTERIM_OBJ_FMT*/
				asym.n_type = N_ABS;
#endif /*INTERIM_OBJ_FMT*/
			}
			else if (symp->s_segment == Text_segno)
			{
#ifdef INTERIM_OBJ_FMT
				asym.s_segment = SEG_TEXT;
#else /*INTERIM_OBJ_FMT*/
				asym.n_type = N_TEXT;
#endif /*INTERIM_OBJ_FMT*/
			}
			else if (symp->s_segment == Data_segno)
			{
#ifdef INTERIM_OBJ_FMT
				asym.s_segment = SEG_DATA;
#else /*INTERIM_OBJ_FMT*/
				asym.n_type = N_DATA;
#endif /*INTERIM_OBJ_FMT*/
			}
#ifdef INTERIM_OBJ_FMT
			else if (symp->s_segment == Sdata_segno)
			{
				asym.s_segment = SEG_SDATA;
			}
#endif /*INTERIM_OBJ_FMT*/
			else if (symp->s_segment == Bss_segno)
			{
#ifdef INTERIM_OBJ_FMT
				asym.s_segment = SEG_BSS;
#else /*INTERIM_OBJ_FMT*/
				asym.n_type = N_BSS;
#endif /*INTERIM_OBJ_FMT*/
			}
			else if (symp->s_segment == ASSEG_FILENAME)
			{
#ifdef INTERIM_OBJ_FMT
				asym.s_segment = SEG_FILENAME;
#else /*INTERIM_OBJ_FMT*/
				asym.n_type = N_FN;
#endif /*INTERIM_OBJ_FMT*/
			}
			else internal_error("objsym_write_sym(): bad seg#%d",
					symp->s_segment);

#ifdef INTERIM_OBJ_FMT
			asym.s_global = BIT_IS_ON(symp->s_attr, SA_GLOBAL);
#else /*INTERIM_OBJ_FMT*/
			if ( BIT_IS_ON(symp->s_attr, SA_GLOBAL))
				asym.n_type |= N_EXT;
#endif /*INTERIM_OBJ_FMT*/
		}
		else
		{
#ifdef INTERIM_OBJ_FMT
			asym.s_global  = TRUE;

			if (BIT_IS_ON(symp->s_attr, SA_COMMON))
			{
				switch (symp->s_segment)
				{
				case ASSEG_DCOMMON:
					asym.s_segment = SEG_DCOMMON;
					break;
				case ASSEG_SDCOMMON:
					asym.s_segment = SEG_SDCOMMON;
					break;
				default:
					internal_error("objsym_write_sym(): bad common seg#%d",
					symp->s_segment);
				}
			}
			else	asym.s_segment = SEG_UNDEF;
#else /*INTERIM_OBJ_FMT*/
			asym.n_type = N_UNDF | N_EXT;
#endif /*INTERIM_OBJ_FMT*/
		}

		symp->s_symidx = objsym_write_aout_symbol(&asym);

		/* Mark that this symbol has been written into the object
		 * file;  i.e. "don't write it again".
		 */
		SET_BIT(symp->s_attr, SA_NOWRITE);
	}
}

static S32BITS	/* Returns the total bytes of symbols written out. */
objsym_write_symbols()
{
	register struct symbol *symp;
	register int padding;

	/* Try to write all symbols to the object file.
	 * objsym_write_sym() will select which ones are OK to actually write.
	 */
	for ( symp = sym_first();   symp != NULL;   symp=sym_next())
	{
		objsym_write_sym(symp);
	}

        /* Emit all of the STAB symbols. */
        emit_stab_nodes();

	/* The first time objsym_write_sym is called, objsym_symfp should
	 * be NULL which will cause all of the temporary files to get opened.
	 *
	 * But there could be NO symbols;  in that case, make sure that
	 * the files got opened!
	 */
	if (objsym_symfp == NULL)	objsym_init();

	/* align the string table end to word boundary ... for .ident */
	padding = 4 - (objsym_str_offset % 4);
	if (padding  != 4)
		for ( ; padding > 0 ; padding--){
			fputc("\0", objsym_strfp);
			objsym_str_offset++;
		}

	/* write out the string table's final length */
	fseek(objsym_strfp, 0L, 0);
	if ( fwrite(&objsym_str_offset, sizeof(objsym_str_offset), 1,
			objsym_strfp) != 1 )
	{
		perror(pgm_name);	/* print the sys err message */
		error(ERR_WRITE_ERROR, NULL, 0, objtmpfilename[SYM_TMP]);
	}

#ifdef INTERIM_OBJ_FMT
	return (objsym_symtab_idx * sizeof(struct aout_symbol));
#else /*INTERIM_OBJ_FMT*/
	return (objsym_symtab_idx * sizeof(struct nlist));
#endif /*INTERIM_OBJ_FMT*/
}


static void
objsym_cleanup()
{
	/* close the symbol-table file	*/
	if (objsym_symfp != NULL)	fclose(objsym_symfp);

	/* close the string-table file	*/
	if (objsym_strfp != NULL)	fclose(objsym_strfp);
}

#endif /*SUN3OBJ*/

void
unlink_obj_tmp_files()
{
	register int i;

	/* remove object file if we have already opened it, but have not
	 * yet finished writing it.
	 */
	if ( objfile_is_incomplete )
	{
		unlink(objfilename);
	}

	/* remove /tmp file(s) left over from object-file preparation, if any */

	for (i = 0;   i < MAX_OBJ_TMP_FILES;   i++)
	{
		if (objtmpfilename[i][0] != '\0')
		{
			unlink(objtmpfilename[i]);
			objtmpfilename[i][0] = '\0';
		}
	}
}


void
obj_init()
{
	/* nothing */
}

void
obj_cleanup()
{
	objsym_cleanup();
	unlink_obj_tmp_files();
}
