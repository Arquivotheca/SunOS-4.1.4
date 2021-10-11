/*      @(#)opt_names.c  1.1     94/10/31     Copyr 1986 Sun Microsystems */

#include <stdio.h>
#include "structs.h"
#include "globals.h"
#include "registers.h"
#include "bitmacros.h"
#include "node.h"
#include "optimize.h"
#include "opcode_ptrs.h"
#include "node.h"
#include "opt_utilities.h"
#include "opt_statistics.h"
#include "opt_progstruct.h"
#include "opt_regsets.h"
#include "opt_context.h"
#include "opt_deldead.h"
#include "set_instruction.h"
#ifdef   sparc /* host==sparc */
# include <alloca.h>
#endif   /*sparc*/ /* host==sparc */

/******************************************************************************/
/***	      name references -- enumerate and eliminate		    ***/
/******************************************************************************/

#define NNAMES	20	/* initial allocation size */
struct namenode *name_array;
static int names_allocated = 0;
#define ALLOCATE_NAMES(i)	if (i >= names_allocated) allocate_names()

static void
allocate_names(){
	if (names_allocated == 0 ){
		name_array = (struct namenode *)as_malloc( NNAMES * sizeof(struct namenode));
		names_allocated = NNAMES;
	} else {
		names_allocated *= 2;
		name_array = (struct namenode *)as_realloc( (char *)name_array, 
				names_allocated * sizeof(struct namenode));
	}
}

Bool
thirteen_bits(v) int v; {
	return ( -0x1000 <= (v) && (v) <= 0xfff );
}

int
magdiff( i, j ){
	if ( i > j ) return i-j;
	return j - i;
}

Bool
set_shifts_addend( np )
	Node * np;
{

	/* this could be a 
	 *     sethi	%hi(number)... 
	 * case, or it could be a 
	 *     sethi	number...
	 * case, or a
	 *     set	xxx...
	 * case. We look at the opcode and the relocation to find out.
	 */
	if (np->n_opcp == opcode_ptr[OP_SETHI] &&
	    np->n_operands.op_reltype != REL_HI22){
		/* sethi  number... */
		return TRUE;
	}
	/* else is %hi() or is set instruction */
	return FALSE;
}

int
sethi_shift(){
#ifdef CHIPFABS
	if (iu_fabno == 1) {
		/* make it 23-bit instead */
		return 9;
	} else
		return 10;
#else /*CHIPFABS*/
	return 10;
#endif /*CHIPFABS*/
}

static int
cmp_names( p1, p2 )
	register struct namenode *p1, *p2;
{
	/* no names are ever equal, since the enumeration is always unique */
	if (p1->name.nm_symbol < p2->name.nm_symbol) return -1;
	else if (p1->name.nm_symbol >p2->name.nm_symbol) return 1;
	else if (p1->enumeration <p2->enumeration) return -1;
	else return 1;
}

int
find_names( tp, xp )
	Node *tp, *xp;
{
	/* make a list of names referenced in this basic block */
	/* it is built in referencing node order */
	register Node *np = tp;
	register int i;
	register_set written;
	int set_val;

	i = 0;
	written = empty_regset;
	while ( np != xp ){
		switch (OP_FUNC(np).f_group){
		case FG_SETHI:
		case FG_SET:
			set_val = np->n_operands.op_addend;
			if (set_shifts_addend( np ) )
				set_val <<= sethi_shift();
			if (np->n_operands.op_symp != 0 
			|| !thirteen_bits(set_val)){
				/* its a "name" */
				ALLOCATE_NAMES(i);
				name_array[i].name.nm_symbol = np->n_operands.op_symp;
				name_array[i].name.nm_addend = set_val;
				name_array[i].np = np;
				name_array[i].enumeration = i;
				i += 1;
			}
			break;
		default:break;
			/* we're not really interested in anything else here */
		}
		written = regset_add( written, REGS_WRITTEN(np) );
		np = np->n_next;
	}
	if ( i != 0 ){
		/* first sort is symbol, second is node order */
		qsort( name_array, i, sizeof(struct namenode), cmp_names );
	}
	totally_dead = regset_excl( regset_sub( window_regset, regset_add( REGS_LIVE(tp), written )), RN_SP );
	return i;
}

static register_set
find_reg_dead_in_range( head, tail )
	Node *head, *tail;
{
	register Node *p;
	register_set written;

	tail = tail->n_next;
	written = empty_regset;
	for (p = head; p != tail; p = p->n_next)
		written = regset_add( written, REGS_WRITTEN(p) );
	return regset_excl( regset_sub( window_regset, regset_add( REGS_LIVE(head), written )), RN_SP );
}

/*
 * determine whether the value of "r" acquired at "begin"
 * persists, in some form, past "end"
 */
static Bool
reg_value_persists( r, begin, end )
	Regno r;
	Node * begin, *end;
{
	register Node * p;
	for ( p = begin->n_next; p != end; p = p->n_next ){
		if (!regset_in( REGS_LIVE(p), r))
			return FALSE;
		if (regset_in( REGS_WRITTEN(p), r)){
			return FALSE;
		}
	}
	return TRUE;
}

static Node *
insert_evaluation_before( np, r, before_here )
	struct namenode *np;
	Regno r;
	Node * before_here;
{
	Node * set;
	set = opt_new_node( 0, opc_lookup("set"), OPT_CONSTPROP );
	set->n_operands.op_symp = np->name.nm_symbol;
	set->n_operands.op_addend = np->name.nm_addend;
	node_references_label( set, np->name.nm_symbol);
	set->n_operands.op_reltype = REL_NONE;
	set->n_operands.op_regs[O_RD] = r;
	set->n_operands.op_imm_used = TRUE;
	set->n_internal = TRUE;
	regset_setnode( set );
	insert_before_node( before_here, set );
	regset_recompute( set );
	return set;
}

static Node *
add_andn_after( np, r )
	Node * np;
	Regno r;
{
	Node * andn;
	andn = opt_new_node( 0, opcode_ptr[OP_ANDN], OPT_CONSTPROP );
	andn->n_operands.op_reltype = REL_13;
#ifdef CHIPFABS
	andn->n_operands.op_addend = (iu_fabno == 1 ) ? MASK(9) : MASK(10);
#else
	andn->n_operands.op_addend = MASK(10);
#endif
	andn->n_operands.op_regs[O_RD] = r;
	andn->n_operands.op_regs[O_RS1] = r;
	andn->n_operands.op_imm_used = TRUE;
	andn->n_internal = TRUE;
	regset_setnode( andn );
	insert_before_node( np->n_next, andn );
	regset_recompute( andn );
	return andn;
}

static void
install_new_name_source( p, r, offset )
	Node *p;
	Regno r;
	int offset;
{
	p->n_operands.op_regs[O_RS1] = r;
	delete_node_reference_to_label( p, p->n_operands.op_symp );
	p->n_operands.op_symp = NULL;
	p->n_operands.op_addend = offset;
	p->n_operands.op_reltype = REL_13;
	regset_setnode(p);
	regset_recompute( p );
}

static Bool
uses_name_source( p, reg, sym, offset )
	register Node *p;
	Regno reg;
	struct symbol *sym;
	int offset;
{
	return (p->n_operands.op_regs[O_RS1] == reg
	        && p->n_operands.op_imm_used
		&& p->n_operands.op_symp == sym
		&& p->n_operands.op_addend == offset);
}

/*
 * we have a number of sethi instructions.
 * they point to same or similar things.
 * if they could be combined, we could eliminate all but one.
 * there are some conditions to test:
 * 1) what happens subsequently. we have to find the add, ld, or st
 *    instruction that will fill in the low-order bits
 * 2) is there a register that will reach? we might have to do some
 *    rewriting.
 * here are some examples:
 *	! a reachability puzzle
 *	sethi	%hi(foo),%o0
 *	ld	[%o0+%lo(foo)],%o0
 *	add	%o0,1,%o0
 *	sethi	%hi(foo),%o1
 *	st	%o0,[%o1+%lo(foo)]
 *	since the ld/st instructions can do arithmetic, we don't want to
 *	form  the full address of  foo using an add instruction, as this
 *	would cost as much as the sethi we're trying to delete.
 *	we also have to rewrite the first sethi/ld to reference %o1.
 *
 *	! a case of close-but-not-equal
 *	sethi	%hi(foo),%o0
 *	ld	[%o0+%lo(foo)],%o0
 *	sethi	%hi(foo+4),%o1
 *	ld	[%o1+%lo(foo+4)],%o1
 *	I'm not sure what the advantage of fully evaluating (foo) into
 *	a register is, except to make it available for later uses.
 *	For now, we'll fully evaluate, and, if appropriate, push the explicit
 *	add back into the ld/st instruction that uses it.
 */
static Bool
comb_uses( ulist, number, bbp )
	struct namenode *ulist;
	int number;
	struct basic_block *bbp;
{
	Regno r, t;
	struct name n;
	int i, addend, new_addend;
	register Node *p;
	int number_changed;
	Node *head_of_range;
	Node * eobl;
	Node * newsethi;
	Node * andn;
	register_set dead_in_range;
	/*
	 * find a register. Experience suggests that the last
	 * register is the best bet.
	 * if that's no good, try to get a new one.
	 * if that's no good, try a smaller set of uses.
	 */
	eobl = bbp->bb_trailer->n_next;
	while ( number >= 1 ){
		dead_in_range = find_reg_dead_in_range( ulist[0].np, ulist[number].np->n_prev);
		i = 0;
		while ( ( i = regset_select( dead_in_range, i+1 ) ) >= 0 ){
			if (! reg_value_persists( i, ulist[number].np, eobl)){
				r = i;
				goto got_a_register;
			}
		}
		number -= 1;
	}
	return 0;
got_a_register:
	/*
	 * have a register. Do this:
	 * a) fully evaluate the name into the register before the current first use.
	 * b) over the full range firstuse..lastuse, rearrange references to the
	 *    name to point to this register, inserting add/sub instructions as
	 *    necessary.
	 * c) call regset_recompute to fix up the register sets.
	 * d) call opt_dead_arithmetic to get rid of the old sethi's
	 */
	n = ulist->name;
	head_of_range = ulist[0].np;
	newsethi = insert_evaluation_before( &n, r, head_of_range );
	optimizer_stats.sethi_eliminated -= 1;
	number_changed = 0;
	if ( head_of_range == bbp->bb_dominator ){
		/* just inserted new dominator */
		bbp->bb_dominator = newsethi;
	}
	for ( i = 0 ; i <= number ; i += 1 ){
		p = ulist[i].np; /* presumably the SETHI */
		if (OP_FUNC(p).f_group != FG_SETHI
		&&  OP_FUNC(p).f_group != FG_SET ){
			/* oops -- deleted by another operation */
			ulist[i].np = NULL;
			continue;
		}
		t = p->n_operands.op_regs[O_RD];
		addend =  p->n_operands.op_addend;
		if ( set_shifts_addend(p))
			addend <<= sethi_shift();
		/*
		 * turn this set or sethi into a mov, add, or worse.
		 * this is to handle the case where we cannot find all the
		 * clients.
		 */
		if ( t == r ){
			new_addend = 0;
		} else {
			new_addend = addend - n.nm_addend;
		}
		p->n_opcp = opcode_ptr[OP_ADD];
		install_new_name_source( p, r, addend - n.nm_addend );
		if (OP_FUNC(p).f_group == FG_SETHI){
			/* insert instruction to reverse effects of OR of %lo()*/
			p = add_andn_after( p, t);
		}
		optimizer_stats.sethi_eliminated += 1;
		while (  (p=p->n_next) != eobl && regset_in( REGS_LIVE( p), t )){
			if ( regset_in( REGS_READ( p ), t )){
				switch( OP_FUNC(p).f_group ){
				case FG_ARITH:
					if ((OP_FUNC(p).f_subgroup == FSG_ADD
				        ||  OP_FUNC(p).f_subgroup == FSG_OR)
					&& (uses_name_source( p, t, n.nm_symbol, addend ))){
						install_new_name_source( p, r, new_addend );
						number_changed += 1;
					}
					break;
				case FG_LD:
				case FG_ST:
					/* a very likely use of the thing */
					if (uses_name_source( p, t, n.nm_symbol, addend )){
						install_new_name_source( p, r, new_addend );
						number_changed += 1;
					}
					break;
				}
			}
			if ( regset_in( REGS_WRITTEN( p ), t ))
				break; /* out of while loop */
		}
		if ( i != 0 )
			ulist[i].np = NULL; /* delete entry */
	}
	opt_dead_arithmetic( head_of_range, p );
	return number_changed > 1; /* if we only changed 1, we did no useful work */
}

Bool
share_names( i, bbp )
	int i;
	struct basic_block *bbp;
{
	register struct namenode * nn1, *nn2;
	struct namenode *end = &name_array[i];
	int k;
	Bool did_something = FALSE;

	for ( nn1 = name_array; nn1 < end; nn1 += 1){
		if ( nn1->np == 0 ) continue; /* deleted entry */
		/* find a run of similar names */
		for (nn2 = nn1+1; nn2 < end; nn2 += 1){
			if (! (nn1->name.nm_symbol == nn2->name.nm_symbol 
				&&  thirteen_bits( magdiff( nn1->name.nm_addend, 
							    nn2->name.nm_addend ))
				&& nn1->np->n_operands.op_reltype ==
					nn2->np->n_operands.op_reltype )
			   )
				break;
		}
		nn2 -= 1;
		if ( nn2 > nn1){
			if ( comb_uses( nn1, nn2 - nn1, bbp ))
				did_something = TRUE;
		}
	}
	return did_something;
}

/*
 * look at every SET instruction in the program.
 * find all the uses of each one. If they're all ld, st, or mov instructions,
 * then we can change the SET to a SETHI, and push the addition into the 
 * client instruction.
 */
static void
pushback_sethi( setp )
	Node * setp;
{
	/*
	 * We'd like to be able to look at and collect all the clients of the
	 * SET in one sweep. For this purpose, we invent our own light-weight
	 * memory management scheme. 
	 */
	struct  nodelist {
		Node *np;
		Bool is_mov_equiv;
		struct nodelist *next;
	} *uselist,  *u;

#	define NEWNODE(p) p=(struct nodelist *)alloca(sizeof(struct nodelist))

	register Node * usep;
	Node *		target;
	Node *		case_branch;
	Regno		r;
	Bool		next_delay_slot, live_in_follows;
	static void deconstruct_set();

	if ( ! do_opt[ OPT_CONSTPROP ] ) {
		if( do_opt[ OPT_SCHEDULING ] ) {
			/* has to translate SET to REAL instruction */
			/* to make instruction scheduling happy */
			deconstruct_set(setp);
		}
		return;
	}
	/* look for all uses of the given set node */
	uselist = NULL;
	r = setp->n_operands.op_regs[O_RD];
	for ( usep = setp->n_next ; usep != NULL; usep=popnode()){
		next_delay_slot = FALSE;
		live_in_follows = FALSE;
		for ( ; regset_in( REGS_LIVE(usep), r) ; usep=usep->n_next){
			if (regset_in( REGS_READ(usep), r)){
				/* examine use, then list it, or bail out */
				switch (OP_FUNC(usep).f_group){
				case FG_LD:
				case FG_ST:
					if (usep->n_operands.op_regs[O_RS1] == r
					    && second_operand_is_zero(usep)) {
						NEWNODE(u);
						u->np = usep;
						u->is_mov_equiv = FALSE;
						u->next = uselist;
						uselist = u;
					}
					else
						goto bail_out;
					break;

				case FG_ARITH:
					if (is_mov_equivalent(usep)) {
						NEWNODE(u);
						u->np = usep;
						u->is_mov_equiv = TRUE;
						u->next = uselist;
						uselist = u;
					}
					else
						goto bail_out;
					break;

				default:
					goto bail_out;
				}
			} else switch(OP_FUNC(usep).f_group){
			case FG_BR:
				if (!BRANCH_IS_CONDITIONAL(usep))
					next_delay_slot = TRUE;
				target = usep->n_operands.op_symp->s_def_node;
				if ( usep->n_operands.op_symp->s_ref_count==1
				     && (! can_fall_into(target)) )
					pushnode(target);
				else {
					live_in_follows |=
						regset_in(REGS_LIVE(target),r );
					if (live_in_follows && !next_delay_slot)
						goto bail_out;
				}
				continue;
			case FG_JMP:
				next_delay_slot = TRUE;
				if (is_jmp_at_top_of_cswitch(usep)){
					/* has many successors */
					case_branch = usep;
					for (usep =usep->n_next->n_next->n_next;
					     OP_FUNC(usep).f_group ==FG_CSWITCH;
					     usep = usep->n_next ){
						if (usep->n_operands.op_symp != NULL){
							target = usep->n_operands.op_symp->s_def_node;
							if ( (usep->n_operands.op_symp
								->s_ref_count==1)
							    && ! can_fall_into(target))
								pushnode(target);
							else
								live_in_follows |=
								    regset_in(REGS_LIVE(target),r );
						}
					}
					usep = case_branch;
				}
				continue;
			case FG_LABEL:
				/* entering another basic block, with r still
				 * live.
				 */
				goto bail_out;
			default:
				break;
			}
			/*
			 * "next_delay_slot" indicates that the instruction
			 * we just examined was the delay slot of an
			 * unconditional control transfer. Quit examining
			 * this thread of execution.
			 * If this instruction WRITES into the register, then
			 * quit examining this path.
			 */
			if (regset_in( REGS_WRITTEN(usep), r)){
				break;
			} else if (next_delay_slot){
				if (live_in_follows)
					goto bail_out;
				else
					break;
			}
		}
	}
	/*
	 * we have collected all the clients of this instruction and
	 * found them all satisfactory. We'll now modify the SET, and
	 * all the clients as well.
	 */
	setp->n_opcp = opcode_ptr[OP_SETHI];
	setp->n_operands.op_reltype = REL_HI22;
	for (u = uselist;  u != NULL;  u = u->next){
		usep = u->np;
		/* see if we haven't already got it by another path */
		if (usep->n_operands.op_symp  == NULL ){
			if (u->is_mov_equiv){
				/* The MOV could have been from either source
				 * operand; make sure it's from RS1.
				 */
				if (first_operand_is_zero(usep)) {
					/* The value to be moved is in the 2nd
					 * operand; i.e. ARITH %0,reg1,regd.
					 * Put it in the 1st operand, instead;
					 * i.e. ARITH reg1,%0,regd.
					 */
					if (usep->n_operands.op_imm_used) {
#ifdef DEBUG
					    if (usep->n_operands.op_addend!=0)
						internal_error("pushback_sethi(): ARITH %%0,CONST,regd?");
#endif /*DEBUG*/
					    /* It was ARITH %0,0,regd. */
					    usep->n_operands.op_regs[O_RS1] =
							RN_G0;
					    usep->n_operands.op_imm_used =FALSE;
					} else {
					    usep->n_operands.op_regs[O_RS1] =
						usep->n_operands.op_regs[O_RS2];
					}
					usep->n_operands.op_regs[O_RS2] = RN_G0;
				}

				/* Turn the MOV-equivalent ARITHmetic
				 * instruction into an ADD.
				 */
				if (NODE_SETSCC(usep)) usep->n_opcp = opcode_ptr[OP_ADDCC]; 
				else usep->n_opcp = opcode_ptr[OP_ADD];
			}
			usep->n_operands.op_symp = setp->n_operands.op_symp ;
			node_references_label( usep, usep->n_operands.op_symp );
			usep->n_operands.op_reltype = REL_LO10;
			usep->n_operands.op_addend = setp->n_operands.op_addend;
			usep->n_operands.op_imm_used = TRUE;
		}

		/* (*u becomes garbage, to be reclaimed on subroutine return) */
	}

	optimizer_stats.dead_arithmetic_deleted += 1;
	return;

bail_out: 		/* flush list */
	while (popnode() != NULL )
		;
	/* has to translate SET to REAL instruction
	 * to make instruction scheduling happy
	 */
	deconstruct_set(setp);
}

void
set_to_sethi( marker_np )
	Node * marker_np;
{

	register Node *	setp;

	for ( setp = marker_np->n_next; setp != marker_np; setp = setp->n_next){
		if (OP_FUNC(setp).f_group == FG_SET){
			pushback_sethi( setp );
		}
	}
}

static void
deconstruct_set( np ) register Node *np;
{
	register Node *new_np;
	
	switch ( minimal_set_instruction(np->n_operands.op_symp,
					 np->n_operands.op_addend) ) {
		case SET_JUST_SETHI:
			/* The source operand is an absolute value,
		 	 * and the 10 low-order bits are zero,
			 * so can use a single instruction:
			 *      SETHI  %hi(VALUE),RD
		 	 */
			np->n_opcp = opcode_ptr[OP_SETHI];
			np->n_operands.op_reltype     = REL_HI22;
			break;

		case SET_JUST_MOV:
			/* The source operand is an absolute value, within the
			 * signed 13-bit Immediate range, so can use a single
			 * instruction:
			 *	MOV const13,RS2
			 * (actually "or  %0,const13,RD")
			 */
			np->n_opcp = opcode_ptr[OP_OR];
			np->n_operands.op_reltype     = REL_13;
			np->n_operands.op_regs[O_RS1] = RN_GP(0);
			break;

		case SET_SETHI_AND_OR:
			/* Turn SET into 2 instructions:
			 *	a SETHI followed by an OR.
			 */
			/* First, modify our given node for the SETHI. */
			np->n_opcp = opcode_ptr[OP_SETHI];
			np->n_operands.op_reltype     = REL_HI22;

			/* Create a new node for the OR. */
			new_np = new_node(np->n_lineno, opcode_ptr[OP_OR]);
			copy_node( np, new_np, OPT_CONSTPROP );
			/* copy_node leaves the following fields set in new_np:
			 *	n_operands.op_imm_used
			 *	n_operands.op_symp
			 *	n_operands.op_addend
			 */
			new_np->n_opcp = opcode_ptr[OP_OR];
			new_np->n_operands.op_reltype     = REL_LO10;
			new_np->n_operands.op_regs[O_RS1] =
						np->n_operands.op_regs[O_RD];

			/* This is an internally-generated node. */
			new_np->n_internal = TRUE;

			insert_after_node(np, new_np);
			regset_setnode(new_np);
			regset_recompute( new_np );
			break;
	}
	return;
}
