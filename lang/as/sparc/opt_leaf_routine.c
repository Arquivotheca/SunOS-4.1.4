static char sccs_id[] = "@(#)opt_leaf_routine.c	1.1\t10/31/94";
/*
 * This file contains code which turns a SPARC function (routine) into a
 * "leaf routine" when possible.
 */

#include <stdio.h>
#include "structs.h"
#include "registers.h"
#include "opcode_ptrs.h"
#include "optimize.h"
#include "node.h"
#include "opt_utilities.h"
#include "opt_regsets.h"
#include "opt_statistics.h"
#include "pcc_types.h"


struct reg_map
{
	unsigned int ref_count;	/* > 0 if reg is ever referenced in function.
				 * (If 0, there is no need to map it).
				 */

	Bool	is_mapped;	/* TRUE if reg has been mapped;
				 * i.e. TRUE if "reg_mapped_into" is valid.
				 */
	Bool    is_pair;        /* TRUE if this is really a register pair as in
				 * ldd/std, ld2/st2
				 */

	Regno	reg_mapped_into;/* Register to which this reg is mapped;
				 * i.e. the register which will be
				 * substituted for this one in the code.
				 */
};

static struct reg_map regs[32];

	/* TRUE if a register hasn't been mapped into, yet. */
static Bool reg_is_available[32];

#ifdef DEBUG	/* useful with DBX */
extern char *regnames[32];	/* from disassemble.c */
static void
display_reg_maps()
{
	register int c,r;

	for (c = 0;  c <= 31;  c+=8)
	{
		printf("reg ref? map2 avl?");
		if (c <= 23)	printf("    ");
		else		putchar('\n');
	}
	for (c = 0;  c <= 31;  c+=8)
	{
		printf("--- ---- ---- ----");
		if (c <= 23)	printf("    ");
		else		putchar('\n');
	}

	for (r = 0;   r <= 7;   r++)
	{
		for (c = 0;  c <= 31;  c+=8)
		{
			printf("%-3s ",  regnames[c+r]);
			printf(" %3d ",  regs[c+r].ref_count);
			if (regs[c+r].is_mapped)
			{
				printf("%-3s  ", regnames[RAW_REGNO(regs[c+r]
							.reg_mapped_into)]);
			}
			else	printf("     ");

			printf(" %c  ",   reg_is_available[c+r] ? 'y' : ' ' );
			if (c <= 23)	printf("    ");
			else		putchar('\n');
		}
	}
}
#endif /*DEBUG*/

static void
clear_reg_maps()
{
	register int i;

	for (i = 0;   i <= 31;   i++)
	{
		regs[i].ref_count  = 0;
		regs[i].reg_mapped_into = RN_NONE;
		regs[i].is_mapped   = FALSE;
		regs[i].is_pair = FALSE;

		reg_is_available[i] = TRUE;
	}
}


static void
tally_a_register(regno)
	register Regno regno;
{
	if (IS_A_GP_REG(regno))
	{
		regs[RAW_REGNO(regno)].ref_count++;
	}
}


static void
untally_a_register(regno)
	register Regno regno;
{
	if (IS_A_GP_REG(regno))
	{
		regs[RAW_REGNO(regno)].ref_count--;
	}
}


static void
tally_node_register_usage(np)
	register Node *np;
{
	if ( OP_FUNC(np).f_node_union == NU_OPS )
	{
		tally_a_register(np->n_operands.op_regs[O_RS1]);
		if ( !np->n_operands.op_imm_used )
		{
			tally_a_register(np->n_operands.op_regs[O_RS2]);
		}
		if ( (NODE_IS_LOAD(np) && OP_FUNC(np).f_rd_width == 8) ||
		     (NODE_IS_STORE(np) && OP_FUNC(np).f_rd_width == 8) )
		{
			tally_a_register(np->n_operands.op_regs[O_RD]);
			tally_a_register(np->n_operands.op_regs[O_RD]+1);
			if (IS_A_GP_REG(np->n_operands.op_regs[O_RD]))
			        regs[RAW_REGNO(np->n_operands.op_regs[O_RD])].is_pair = TRUE;
		} else {
			tally_a_register(np->n_operands.op_regs[O_RD]);
		}
	}
}


static void
untally_node_register_usage(np)
	register Node *np;
{
	if ( OP_FUNC(np).f_node_union == NU_OPS )
	{
		untally_a_register(np->n_operands.op_regs[O_RS1]);
		if ( !np->n_operands.op_imm_used )
		{
			untally_a_register(np->n_operands.op_regs[O_RS2]);
		}
		untally_a_register(np->n_operands.op_regs[O_RD]);
	}
}


static int
number_of_leaf_registers_needed()
{
	register int i;
	register int regs_used = 0;

	/* Counts the number of window registers which are referenced in this
	 * routine, which are therefore the number of leaf (OUT+GLOBAL)
	 * registers which would be needed if they were all mapped into
	 * regsiters for a leaf routine.
	 * Exceptions:
	 *
	 *	-- %fp (%i6) gets mapped to itself instead of to an
	 *		OUT-register, so it is NEVER counted.
	 *
	 *	-- %sp (%o6) gets mapped to itself and is always reserved, so
	 *		it ALWAYS gets counted.
	 *	-- %g0, ditto.
	 *	-- %g6, ditto.
	 *	-- %g7, ditto.
	 */

	for ( i = 0;   i <= 31;   i++ )
	{
		switch ( i )
		{
		case RAW_REGNO(RN_GP_IN(6)):
			/* Never count %i6 (%fp). */
			break;
		case RAW_REGNO(RN_GP_OUT(6)):	/* Always count %o6 (%sp). */
		case RAW_REGNO(RN_GP_GBL(0)):	/* Always count %g0. */
		case RAW_REGNO(RN_GP_GBL(6)):	/* Always count %g6. */
		case RAW_REGNO(RN_GP_GBL(7)):	/* Always count %g7. */
			regs_used++;
			break;
		default:
			if ( regs[i].ref_count > 0 )
			{
				regs_used++;
			}
		}
	}

	return regs_used;
}
static int
number_of_leaf_registers_available()
{
	register int i;
	register int regs_avail = 0;

	/* Counts the number of globals and outs that are available for 
	 * mapping.  This number should be 16 unless we have "pre_mapped"
	 * %o0 onto itself  (see make_leaf_routine) in which case it should
	 * 15.
	 */
	for ( i = 0;   i <= 7;   i++ ){
		if ( reg_is_available[RAW_REGNO(RN_GP_GBL(i))] )
			regs_avail++;
		if ( reg_is_available[RAW_REGNO(RN_GP_OUT(i))] )
			regs_avail++;
	}
	return regs_avail;
}


static Regno
get_free_leaf_regsiter()
{
	register int i;

	/* Try first to put it into an OUT register. */ 
	for ( i = 0;   i <= 7;   i++ )
	{
		if ( reg_is_available[RAW_REGNO(RN_GP_OUT(i))] )
		{
			reg_is_available[RAW_REGNO(RN_GP_OUT(i))] = FALSE;
			return RN_GP_OUT(i);
		}
		
	}

	/* The OUT registers are all taken up... Try for one of the available
	 * GLOBAL registers, %g1 - %g5.
	 */
	for ( i = 1;   i <= 5;   i++ )
	{
		if ( reg_is_available[RAW_REGNO(RN_GP_GBL(i))] )
		{
			reg_is_available[RAW_REGNO(RN_GP_GBL(i))] = FALSE;
			return RN_GP_GBL(i);
		}
	}

	internal_error("get_free_leaf_regsiter(): not enuf OUT+GBL regs?");
	/*NOTREACHED*/
}


/* If this function returns 0 (map to %g0!!!) it means it failed to
 * find a pair and we can't make_leaf_routine().
 */
static Regno
get_free_leaf_regsiter_pair()
{
	register int i;

	/* Try first to put it into an OUT register. */ 
	for ( i = 0;   i <= 6;   i = i + 2 )
	{
		if ( reg_is_available[RAW_REGNO(RN_GP_OUT(i))] &&
		     reg_is_available[RAW_REGNO(RN_GP_OUT(i+1))] )
		{
			reg_is_available[RAW_REGNO(RN_GP_OUT(i))] = FALSE;
			reg_is_available[RAW_REGNO(RN_GP_OUT(i+1))] = FALSE;
			return RN_GP_OUT(i);
		}
		
	}

	/* The OUT registers are all taken up... Try for one of the available
	 * *EVEN* GLOBAL registers, %g2 - %g4.
	 */
	for ( i = 2;   i <= 4;   i = i + 2 )
	{
		if ( reg_is_available[RAW_REGNO(RN_GP_GBL(i))] && 
		     reg_is_available[RAW_REGNO(RN_GP_GBL(i+1))] )
		{
			reg_is_available[RAW_REGNO(RN_GP_GBL(i))] = FALSE;
			reg_is_available[RAW_REGNO(RN_GP_GBL(i+1))] = FALSE;
			return RN_GP_GBL(i);
		}
	}
	return 0;  
}


static void
map_into_given_register(regno, target_regno)
	register Regno regno;
	register Regno target_regno;
{
	/* Forcibly map the given register "regno" into the register
	 * "target_regno".
	 */
#ifdef DEBUG
	if ( !reg_is_available[RAW_REGNO(target_regno)] )
	{
		internal_error("map_into_given_register(): target not available?");
	}
	if ( !IS_A_GP_REG(regno) || !IS_A_GP_REG(target_regno) )
	{
		internal_error("map_into_given_register(): %d|%d not GP reg?",
			regno, target_regno);
	}
#endif /*DEBUG*/
	regs[RAW_REGNO(regno)].reg_mapped_into = target_regno;
	reg_is_available[RAW_REGNO(target_regno)] = FALSE;

	regs[RAW_REGNO(regno)].is_mapped = TRUE;
}


static void
map_into_free_leaf_register(regno)
	register Regno regno;
{
	/* Map it into any unused OUT or GLOBAL register. */
	regs[RAW_REGNO(regno)].reg_mapped_into = get_free_leaf_regsiter();

	regs[RAW_REGNO(regno)].is_mapped = TRUE;
}

static Bool
map_into_free_leaf_register_pair(regno)
	register Regno regno;
{
	Regno mapped_reg =  get_free_leaf_regsiter_pair();
	if ( mapped_reg == 0 ) return FALSE;

	/* Map it into any unused OUT or GLOBAL register. */
	regs[RAW_REGNO(regno)].reg_mapped_into = mapped_reg++;
	regs[RAW_REGNO(regno+1)].reg_mapped_into = mapped_reg;

	regs[RAW_REGNO(regno)].is_mapped = TRUE;
	regs[RAW_REGNO(regno+1)].is_mapped = TRUE;
	return TRUE;
}


static Bool
create_register_mapping()
{
	register int i;

	/* Always map the 0, 6, and 7 Global registers to themselves,
	 * as they are reserved for use as "constant 0" and two base registers
	 * for the Small-Data segment.
	 */ 
	map_into_given_register(RN_GP_GBL(0), RN_GP_GBL(0));
	map_into_given_register(RN_GP_GBL(6), RN_GP_GBL(6));
	map_into_given_register(RN_GP_GBL(7), RN_GP_GBL(7));
	/* Map the first 6 IN registers into the correspondingly-numbered OUT
	 * registers.  This is necessary, so that input arguments will be
	 * correctly referenced!
	 */
	for (i = 0;   i <= 5;   i++)
	{
		if ( regs[RAW_REGNO(RN_GP_IN(i))].ref_count > 0 )
		{
			map_into_given_register(RN_GP_IN(i), RN_GP_OUT(i));
		}
	}
	/* Ditto for %i7 (the return address), which has to be referenced in
	 * the RET instruction.
	 */
	map_into_given_register(RN_GP_IN(7), RN_GP_OUT(7));  

	/* Since %i6 has a SPECIAL meaning (%fp, the frame pointer), make sure
	 * that it is forcibly mapped to itself.  make_leaf_routine() may not
	 * even try to make a leaf routine out of one which references %fp, but
	 * this is the safe thing to do anyway.  If nothing else, it makes sure
	 * that nothing ELSE ends up mapped to the Frame Pointer.
	 * This mapping is harmless even if %i6 is never referenced (since the
	 * mapping doesn't eat up a precious OUT register).
	 */
	map_into_given_register(RN_GP_IN(6), RN_GP_IN(6));

	/* Since %o6 has a SPECIAL meaning (%sp, the stack pointer), make sure
	 * that it is forcibly mapped to itself.  This must be done, even if
	 * it is never referenced (although it SHOULD have been, by SAVE!),
	 * to make sure that nothing ELSE ends up mapped to it.
	 */
	map_into_given_register(RN_GP_OUT(6), RN_GP_OUT(6));

	/* Do register pairs (for ldd/std/ld2/st2 first) to make best use of 
	 * available register pairs.
	 */
	for (i = 0;   i <= 30;   i = i + 2)
	{
		if ( (regs[i].is_pair) && (regs[i].ref_count > 0)
		     && (!regs[i].is_mapped) )
		     
		{
			if ( !map_into_free_leaf_register_pair(RN_GP(i)) )
			        return FALSE;
		}
	}
	/* Do the remaining GLOBAL registers separately, so that we can
	 * attempt to map them to themselves, when possible.
	 * (This is not necessary, but is a nice touch).
	 */
	for (i = 0;  i <= 5;   i++)
	{
		if ( (regs[RAW_REGNO(RN_GP_GBL(i))].ref_count > 0) &&
		     (!regs[RAW_REGNO(RN_GP_GBL(i))].is_mapped) )
		{
			if ( reg_is_available[RAW_REGNO(RN_GP_GBL(i))] )
			{
				map_into_given_register( RN_GP_GBL(i),
							 RN_GP_GBL(i) );
			}
		}
	}

	/* Do the remaining OUT-registers separately from the LOCAL registers,
	 * so that we can attempt to map them to themselves, when possible.
	 * (This is not necessary, but is a nice touch).
	 */
	for (i = 0;   i <= 5;   i++)
	{
		if ( (regs[RAW_REGNO(RN_GP_OUT(i))].ref_count > 0) &&
 		     (!regs[RAW_REGNO(RN_GP_OUT(i))].is_mapped) )
		{
			if ( reg_is_available[RAW_REGNO(RN_GP_OUT(i))] )
			{
				map_into_given_register( RN_GP_OUT(i),
							 RN_GP_OUT(i) );
			}
		}
	}

	/* Now make sure that every single register which is referenced in the
	 * routine, but wasn't already mapped above as a "special case", gets
	 * mapped into SOMEthing.
	 */
	for (i = 0;   i <= 31;   i++)
	{
		if ( (regs[i].ref_count > 0) && (!regs[i].is_mapped) )
		{
			map_into_free_leaf_register(RN_GP(i));
		}
	}
	return TRUE;
}


static void
remap_register(regno_p)
	register Regno *regno_p;
{
	/* Remap the registers according to its mapping.
	 * Change ONLY general-purpose regs.
	 */

	if ( IS_A_GP_REG(*regno_p) )
	{
#ifdef DEBUG
		/* If there is no mapping for this register, we really goofed
		 * somehwere.
		 */
		if (!regs[RAW_REGNO(*regno_p)].is_mapped)
		{
			internal_error("remap_register(): no map for %%%d?",
				RAW_REGNO(*regno_p));
		}
#endif
		/* Yes, it's a G.P. reg; go ahead and map it! */
		*regno_p = regs[RAW_REGNO(*regno_p)].reg_mapped_into;
	}
}


static void
remap_referenced_registers(np)
	register Node *np;
{
	/* Within given instruction, change any remapped registers according
	 * to their mappings.
	 */

	if ( OP_FUNC(np).f_node_union == NU_OPS )
	{
		/* Map each of RS1, RS2, and RD in the instruction to the new
		 * name for the register.
		 */
		remap_register( &(np->n_operands.op_regs[O_RS1]) );
		if ( !np->n_operands.op_imm_used )
		{
			remap_register( &(np->n_operands.op_regs[O_RS2]) );
		}
		remap_register( &(np->n_operands.op_regs[O_RD ]) );
		regset_setnode(np);  /* restore live/dead info */
	}
}


static void
delete_node_if_prologue_comment(np)
	register Node *np;
{
	if ( (OP_FUNC(np).f_group == FG_COMMENT) &&
	     (strncmp(np->n_string, "#PROLOGUE#", 10) == 0)
	   )
	{
		delete_node(np);
	}
}

static void
tally_exit_registers(t)
	PCC_TWORD t;
{
	switch (t) {
	    case PCC_VOID:
		break;  /* no return value to worry about */
	    case PCC_FLOAT:
		tally_a_register(RN_FP(0));
		break;
	    case PCC_DOUBLE:
		tally_a_register(RN_FP(0)); 
		tally_a_register(RN_FP(1));
                break; 
	    default:
		tally_a_register(RN_I0);
		break;
	}
}	
	
Bool
make_leaf_routine(marker_np)
	register Node *marker_np;
{
	/* If a function:
	 *	(a) contains no CALLs or JMPLs to other functions,
	 *	(b) contains no references to %sp or %fp,
	 *	(c) refers to <= 8 different registers (excluding global
	 *		registers and %i6 (%sp)), 
	 *  and (d) contains only one each SAVE, RET, and RESTORE instruction
	 *		(we could theoretically handle more, but let's keep
	 *		 things simple for now),
	 * then we can make it into a leaf routine, i.e. one which doesn't
	 * create a new register window to use.
	 *
	 * The advantage of so doing is to reduce the number of (expensive)
	 * register window overflows/underflows.
	 *
	 * Making it into a leaf routine entails:
	 *	-- changing the SAVE instruction into a ADD instruction.
	 *	-- changing the RESTORE instruction into a SUB instruction,
	 *		using the same three operands as did the SAVE.
	 *	-- mapping all references to IN and LOCAL registers in the
	 *		function body into references to OUT registers.
	 *	-- changing the "ret" to a "retl" (return from leaf routine).
	 *
	 * If we can produce a leaf routine, we take this:
	 *
	 *	.proc	<n>			<-- proc_np
	 *	!#PROLOGUE# 0		    !(maybe)
	 *	sethi	%hi(LF<nn>),%g1     !(maybe)
	 *	add	%g1,%lo(LF<nn>),%g1 !(maybe)
	 *	save	%sp,<xx>,%sp		<-- save_np
	 *	!#PROLOGUE# 1		    !(maybe)
	 *	<function body>
	 *	ret				<-- ret_np
	 *	restore	<ARGS>			<-- restore_np
	 *
	 * and, if "restore <ARGS>" is "restore %g0,%g0,%g0", we turn it into:
	 *
	 *	.proc	<n>			<-- proc_np
	 *	<function body>
	 *	retl				<-- ret_np
	 *	nop
	 *
	 * or, if "restore <ARGS>" is "restore ...,...,%i<n>", we turn it into:
	 *
	 *	.proc	<n>			<-- proc_np
	 *	<function body>
	 *	retl				<-- ret_np
	 *	restore	...,...,%o<n>		<-- restore_np
	 *
	 * (with all registers [except %i6/%sp and Global registers) turned
	 *  into OUT registers).
	 *
	 * make_leaf_routine() returns TRUE if was successful and changed
	 * the code, otherwise FALSE.
	 */

	register Node *np;
	register Node *save_np, *ret_np, *restore_np;
	register Node *proc_np;
	Bool convert_restore_RD_to_IN_register = FALSE;

#ifdef DEBUG
	if((RAW_REGNO(RN_GP_GBL(0)) ==  0) || (RAW_REGNO(RN_GP_OUT(0)) ==  8) ||
	   (RAW_REGNO(RN_GP_LCL(0)) == 16) || (RAW_REGNO(RN_GP_IN(0) ) == 24) )
	{
		/* Then using 0..31 as array indices into one array for all of
		 * the regsiters is OK.  The alternative (yuck) would be to
		 * keep 4 different arrays.
		 */
	}
	else	internal_error("make_leaf_routine(): need > 1 array!");
#endif

	if ( !do_opt[OPT_LEAF] )	return FALSE;

	proc_np = marker_np->n_next;

	/* If the beginning node isn't a ".proc" node, then this isn't a
	 * regular procedure, so skip it.
	 */
	if ( !NODE_IS_PROC_PSEUDO(proc_np) )	return FALSE;

	save_np    = NULL;
	ret_np     = NULL;
	restore_np = NULL;

	/* Scan the routine:
	 *	-- remembering where the 3 nodes of most interest (SAVE, RET,
	 *		and RESTORE) are,
	 *	-- looking for CALLs and JMPLs,
	 *      -- tallying up register usage.
	 */

	clear_reg_maps();

	tally_exit_registers(proc_np->n_operands.op_addend);

	for (np = proc_np->n_next;  !NODE_IS_MARKER_NODE(np);  np = np->n_next)
	{
		switch ( OP_FUNC(np).f_group )
		{
		case FG_JMPL:
		case FG_CALL:
			/* We handle pic calls as a special case so we can still
			 * do leaf routine optimization on pic code.  If the pic
			 * call is useless (%o7 is never used) then we delete it.
			 */
			if (NODE_IS_PIC_CALL(np)) {
				if (!regset_in(following_liveset(np->n_next->n_next),
					       RN_CALLPTR)) {
					np = np->n_prev;
					delete_node(np->n_next); /* call */
					delete_node(np->n_next); /* nop */
					regset_recompute(np->n_next);
				} else  {
					return FALSE;
				}
			}
			else return FALSE;
			break;

		case FG_RET:
			switch ( OP_FUNC(np).f_subgroup )
			{
			case FSG_RET:
				/* If we already saw a "ret", forget it. */
				if (ret_np == NULL)
				{
					ret_np = np;
					/* Only one register to tally. */
					tally_a_register(
						np->n_operands.op_regs[O_RS1]);
				}
				else	return FALSE;
				break;
			case FSG_RETL:
				/* Must ALREADY be a leaf routine! */
				return FALSE;
			}
			break;

		case FG_ARITH:
			switch ( OP_FUNC(np).f_subgroup )
			{
			case FSG_SAVE:
				/* If we already saw a "save", forget it. */
				if (save_np == NULL)
				{
					save_np = np;
					/* Don't bother to tally the SAVE's
					 * registers, as it will be deleted if
					 * we're successful, anyway.
					 */
				}
				else	return FALSE;
				break;
			case FSG_REST:
                                /* If we already saw a "restore", forget it. */
				if (restore_np == NULL)
				{
					restore_np = np;
					tally_a_register(
						np->n_operands.op_regs[O_RS1]);
					tally_a_register(
						np->n_operands.op_regs[O_RS2]);
#ifdef DEBUG
					if ( IS_GP_IN_REG(np->n_operands.op_regs[O_RD]) ||
					     IS_GP_LCL_REG(np->n_operands.op_regs[O_RD]) )
					{
						/* If the destination operand
						 * isn't an OUT reg or GBL reg,
						 * something funny is going on.
						 */
						internal_error("make_leaf_routine(): RESTORE ,,IN|LCL?");
					}
#endif /*DEBUG*/

					/*Treat RESTORE's RD specially, later.*/
				}
				else	return FALSE;
				break;
			default:
				if ( save_np != NULL ) /*(see comment below)*/
				{
					tally_node_register_usage(np);
				}
			}
			break;

		default:
			/* Make note of the registers referenced by this
			 * instruction, IF we've already passed the SAVE
			 * instruction.  (Ignore the instructions preceeding
			 * the SAVE, since they will be deleted if we succeed).
			 */
			if ( save_np != NULL )	tally_node_register_usage(np);
		}
	}

	/* If the Frame Pointer was referenced, then don't bother to try
	 * making this into a leaf routine.  It probably could be done, but
	 * the work wouldn't be worth it.
	 */
	if ( (regs[RAW_REGNO(RN_FPTR)].ref_count > 0) ||
	     (regs[RAW_REGNO(RN_SP)].ref_count > 0) )	return FALSE;

	/* We're counting on having found one each of a SAVE, a RET, and a
	 * RESTORE instruction!
	 * If they're not here, then we sure can't do this optimization.
	 * Example procedure in which this happens:
	 *	extern int q;
	 *	foo(){ while(1) q++; }
	 */
	if ( (save_np == NULL) || (restore_np == NULL) || (ret_np == NULL) )
	{
		return FALSE;
	}
	/* We expect the RET and RESTORE, if present, to have come as a pair. */
	if ( ret_np->n_next != restore_np )
	{
		internal_error("make_leaf_routine(): not RET/RESTORE?");
		/*NOTREACHED*/
	}

	/* One last thing to do to finish up the register-reference tallying:
	 * handle the RESTORE's destination register.
	 *
	 * After function-epilogue optimization, the RESTORE instruction often
	 * ends up putting its result in one of the OUT regsiters, typically
	 * %o0 (%o0 of the CALLER's window, i.e. %i0 of the subroutine's
	 * window).
	 *
	 * If there are any other references to %i0, then we will later change
	 * %o0 to %i0 in the RESTORE's RD (if there are any any references to
	 * an IN register %i0 - %i5, it is a parameter and therefore is
	 * guaranteed to get mapped to the corresponding OUT register).
	 *
	 * If there are NO other references to %o0 (%i0), then don't suck up a
	 * mapping from %i0 --> %o0; just leave it %o0 and make sure that ALL
	 * references to %o0 get mapped back to itself, by pre-mapping it.
	 *
	 * Believe it or not, all of this is to possibly free up ONE leaf
	 * register...
	 */
	if ( IS_GP_OUT_REG(restore_np->n_operands.op_regs[O_RD]) )
	{
		/* It's an OUT regsiter. */

		Regno restore_rd_as_IN_reg = 
			restore_np->n_operands.op_regs[O_RD]
				     - RN_GP_OUT(0) + RN_GP_IN(0) ;

		if (regs[ RAW_REGNO(restore_rd_as_IN_reg) ].ref_count == 0)
		{
			/* There are no other references to this register.
			 * Here's the tricky part (see above): pre-map the OUT
			 * register to itself. This will save one leaf regsiter.
			 */
			map_into_given_register(
				restore_np->n_operands.op_regs[O_RD],
				restore_np->n_operands.op_regs[O_RD] );
		}
		else
		{
			/* No other references:	tally RD as if it were an IN
			 * register, and remember to change RD to an IN, later.
			 */
			tally_a_register( restore_rd_as_IN_reg );
			convert_restore_RD_to_IN_register = TRUE;
		}
	}
	else	tally_a_register(restore_np->n_operands.op_regs[O_RD]);

	/* If we would need more than 16 "leaf-usable" regsiters in order to
	 * make this into a leaf routine, then trying to make this routine
	 * into a leaf routine is a lost cause (we only have 8 OUT plus
	 * 8 GLOBAL registers to work with).
	 */
	if ( number_of_leaf_registers_needed() > number_of_leaf_registers_available())
		return FALSE;

	/* Create the register mapping. Since the number of registers is there
	 * per above test this can only fail if there are not enough register pairs.
	 */
	if (!create_register_mapping()) return FALSE;

	/* -------------------------------------------------------------------
	 * At this point, we are certain that we can make the routine into a
	 * leaf routine.
	 * -------------------------------------------------------------------
	 */

	/* Delete the SAVE instruction -- and the preceeding SETHI and ADD of
	 * the function prologue, if they haven't already been deleted by
	 * function-prologue compaction.  Also, while we're at it, delete the
	 * two "!#PROLOGUE#" comments, if they're present.
	 */ 
	if ( (OP_FUNC(save_np->n_prev->n_prev).f_group == FG_SETHI) &&
	     ( (OP_FUNC(save_np->n_prev).f_group    == FG_ARITH) &&
	       (OP_FUNC(save_np->n_prev).f_subgroup == FSG_ADD)
	     )
	   )
	{
		delete_node(save_np->n_prev->n_prev);
		delete_node(save_np->n_prev);
	}
	delete_node_if_prologue_comment(save_np->n_prev);
	delete_node_if_prologue_comment(save_np->n_prev);
	delete_node_if_prologue_comment(save_np->n_next);
	if (opt_node_is_in_delay_slot(save_np))
	{
		/* The SAVE node is in a delay slot, which can happen in
		 * Instruction Scheduling (which shouldn't have yet been done,
		 * but let's play it really safe).  We must leave something
		 * "known" in the delay slot, so replace the SAVE with a NOP.
		 */
		insert_before_node(save_np,
			new_optimizer_NOP_node(save_np->n_lineno, OPT_LEAF));
	}
	delete_node(save_np);

	/* If we are to convert the RESTORE's RD register from an OUT register
	 * to an IN register, do it now. 
	 * This is done so that remap_referenced_registers() doesn't have to
	 * know about the peculiarity of RESTORE's RD.  (we'd have to do
	 * something similar with the SAVE's RS1 & RS2, but fortunately we just
	 * deleted the SAVE).
	 */
	if ( convert_restore_RD_to_IN_register )
	{
		restore_np->n_operands.op_regs[O_RD] +=
			(RN_GP_IN(0) - RN_GP_OUT(0));
	}
		

	/* Now, go back through the routine and make the register substitutions
	 * using the mapping to OUT registers which we just created.
	 */
	for (np = proc_np->n_next;  !NODE_IS_MARKER_NODE(np);  np = np->n_next)
	{
		remap_referenced_registers(np);
	}

	/* Change the RET into a RETL. */
	ret_np->n_opcp = opcode_ptr[OP_RETL];
	/* RET's RS1 was not changed into %o7 by remap_referenced_registers()
	 * because its use of RS1 is only implied
	 * (i.e., OP_FUNC(ret_np).f_node_union == NU_NONE, not NU_OPS).
	 * Be considerate, and fix it here.
	 */
	ret_np->n_operands.op_regs[O_RS1] = RN_GP_OUT(7); /* %o7 */
	regset_setnode(ret_np);
#ifdef DEBUG
	ret_np->n_optno = OPT_LEAF;
#endif /*DEBUG*/

	/* The destination register of the RESTORE is an OUT register has
	 * already been changed to the equivalent current-window register
	 * i.e. the corresponding IN register.
	 */

	/* Now, does this instruction do any useful work?
	 * If it does do useful work, then change it into an ADD.
	 * If not, then just change it into a NOP.
	 *
	 * Note that the RESTORE's RD regsiter was already changed to refer to
	 * a CURRENT-window relative register above.  Therefore, (1) the RS1/RS2
	 * and RD regs can be directly compared, and (2) RD won't have to be
	 * changed again if we convert it into an ADD instruction.
	 */

	if ( ( !restore_np->n_operands.op_imm_used &&
	       ( restore_np->n_operands.op_regs[O_RS2] ==
	           restore_np->n_operands.op_regs[O_RD] ) &&
	       ( restore_np->n_operands.op_regs[O_RS1] == RN_GP(0) )
	     ) ||
	     ( ( restore_np->n_operands.op_regs[O_RS1] ==
	         restore_np->n_operands.op_regs[O_RD] ) &&
	       ( restore_np->n_operands.op_imm_used &&
	         (restore_np->n_operands.op_symp == NULL) &&
	         (restore_np->n_operands.op_addend == 0)
	       ) ||
	       ( ( restore_np->n_operands.op_regs[O_RS1] ==
	         restore_np->n_operands.op_regs[O_RD] ) &&
		 !restore_np->n_operands.op_imm_used &&
	         ( restore_np->n_operands.op_regs[O_RS2] == RN_GP(0) )
	       )
	     )
	   )
	{
		/* The RESTORE does nothing useful besides popping the register
		 * window.  At most, it copies a register to itself (big deal).
		 * Replace the RESTORE with a NOP, untallying the RESTORE's
		 * register references (if it contained the only references to
		 * some register, then it might free one up for the re-mapping).
		 * We don't need to tally the NOP node's register usage, since
		 * it doesn't use any registers.
		 */
		Node *nop_np;
		insert_before_node( restore_np,
				   	(nop_np = new_optimizer_NOP_node(
							restore_np->n_lineno,
						  	OPT_LEAF)) );
		/**untally_node_register_usage(restore_np);**/
		delete_node(restore_np);
		restore_np = nop_np;
	}
	else
	{
		/* The RESTORE has been given operands which do "useful" work
		 * (probably by the function-epilogue optimization).
		 * Therefore, we can't just turn it into a NOP.  We must leave
		 * an equivalently useful instruction in its place (an ADD).
		 */
		restore_np->n_opcp = opcode_ptr[OP_ADD];
#ifdef DEBUG
		restore_np->n_optno = OPT_LEAF;
#endif /*DEBUG*/
	}

	/* Make sure register-usage information is left correct. */
	regset_make_leaf_exit();
	regset_procedure_exit_regs_read(restore_np);
	regset_recompute(restore_np);

	optimizer_stats.leaf_routines_made++;

	return TRUE;
}
