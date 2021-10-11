#ifndef lint
static  char sccsid[] = "@(#)complex_expansion.c 1.1 94/10/31 Copyr 1989 Sun Micro";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "misc.h"
#include "page.h"
#include "iv.h"
#include <stdio.h>

struct exprStack {
    IR_NODE *split;
    IR_NODE *real;
    IR_NODE *imaginary;
};

static struct exprStack exprStackTop;

static TYPE    currentComplexType;
static TYPE    currentFloatType;

static BOOLEAN thereAreComplexOperations = IR_FALSE;
static BOOLEAN complexMultiply; 

typedef enum  {IMAG, CMPLX, CNJG} SPECIAL_FUNCTIONS;
SPECIAL_FUNCTIONS special_call;
 
static void
no_complex_multiplies_have_been_seen_yet()
{
    complexMultiply = IR_FALSE;
}

static BOOLEAN
there_was_a_complex_multiply()
{
    return complexMultiply;
}

static void
clear_expr_stack_top()
{
    exprStackTop.split = (IR_NODE *)NULL;
    exprStackTop.real = (IR_NODE *)NULL;
    exprStackTop.imaginary = (IR_NODE *)NULL;
}

static void
move_expr_stack_top_to(direction)
    struct exprStack *direction;
{
    *direction = exprStackTop;
    clear_expr_stack_top();
}

static TYPE
determine_current_type(use)
    IR_NODE *use;
{
    if (use->operand.tag == ISTRIPLE) {
        return ((TRIPLE *)use)->type;
    }
    else {
        return ((LEAF *)use)->type;
    }
}

static void
determine_complex_type(use)
    IR_NODE *use;
{
    currentComplexType = determine_current_type(use);

    /* determine type of real and imaginary components */
    if (currentComplexType.size == (2 * floattype.size)) {
        currentFloatType = floattype;
    }
    else {
        currentFloatType = doubletype;
    }
}

static BOOLEAN
worth_splitting(expr)
    struct exprStack *expr;
{
    if ((expr->real) &&
        ((expr->real->operand.tag == ISTRIPLE) ||
         (((LEAF *)(expr->real))->class != 
          ((LEAF *)(expr->imaginary))->class))) {
        /* complex subtree of some depth was expanded */
        return IR_TRUE;
    }
    return IR_FALSE;
}

static void
overlap_leaves(complexLeaf, floatLeaf)
    LEAF *complexLeaf;
    LEAF *floatLeaf;
{
    LIST *lp;
    LIST *overlapLP;

    /* overlap complexLeaf with floatLeaf */
    lp = NEWLIST(proc_lq);
    (LEAF*)lp->datap = complexLeaf;
    floatLeaf->overlap = lp;
    lp->next = lp;

    /* overlap floatLeaf with overlapping leaves
       on complexLeaf's overlap list */
    LFOR(overlapLP, complexLeaf->overlap) {
        if (leaves_overlap(
                LCAST(overlapLP, LEAF), 
                floatLeaf)) {
            /* add floatLeaf to overlap list of
               overlapping leaf */
            lp = NEWLIST(proc_lq);
            (LEAF*)lp->datap = floatLeaf;
            LAPPEND(
                LCAST(overlapLP, LEAF)->overlap,
                lp);
         
            /* add overlapping leaf to overlap
               list of floatLeaf */
            lp = NEWLIST(proc_lq);
            (LEAF*)lp->datap = LCAST(overlapLP, LEAF);
            LAPPEND(
                floatLeaf->overlap,
                lp);
        }
    } 

    /* overlap floatLeaf with complexLeaf */
    lp = NEWLIST(proc_lq);
    (LEAF*)lp->datap = floatLeaf;
    if (complexLeaf->overlap) {
        lp->next = complexLeaf->overlap->next;
        complexLeaf->overlap->next = lp;
    }
    else {
        /* first overlapping leaf */
        lp->next = lp;
        complexLeaf->overlap = lp;
    }
}

static LEAF *
build_float_leaf(leafAddress)
    struct address leafAddress;
{
    /* maybe there is already one out there which we
       can share, otherwise create one */ 
    return leaf_lookup(
               VAR_LEAF,
               currentFloatType,
               (LEAF_VALUE *)&leafAddress);    
}

static void
build_real_part(currentLeaf)
    LEAF *currentLeaf;
{
    LEAF *realLeaf = build_float_leaf(currentLeaf->val.addr);

    /* fix up overlap lists */
    overlap_leaves(currentLeaf, realLeaf);

    /* bump up currentLeaf's overlap so that first leaf 
       on overlap list is real leaf */
    currentLeaf->overlap = currentLeaf->overlap->next;
}

static void
get_real_part(currentLeaf)
    LEAF *currentLeaf;
{
    exprStackTop.real = (IR_NODE *) 
        ((LEAF *)(currentLeaf->overlap->datap));
}

static void
build_imaginary_part(currentLeaf)
    LEAF *currentLeaf;
{
    struct address leafAddress;
    LEAF *imaginaryLeaf;

    leafAddress = currentLeaf->val.addr;

    /* reset offset to overlay imaginary part */ 
    leafAddress.offset = leafAddress.offset + currentFloatType.size;

    imaginaryLeaf = build_float_leaf(leafAddress);

    /* fix up overlap lists */
    overlap_leaves(currentLeaf, imaginaryLeaf);
}

static void
get_imaginary_part(currentLeaf)
    LEAF *currentLeaf;
{
    exprStackTop.imaginary = (IR_NODE *) 
        ((LEAF *)(currentLeaf->overlap->next->datap)); 
}

static void
build_real_constant(currentLeaf)
    LEAF *currentLeaf; 
{ 
    struct constant const;

    if (currentLeaf->val.const.isbinary) {
        /* constant is in binary form */
        const.isbinary = IR_TRUE;
        const.c.bytep[0] = currentLeaf->val.const.c.bytep[0];
    }
    else {
        /* constant is in string form */
        const.isbinary = IR_FALSE; 
        const.c.fp[0] = currentLeaf->val.const.c.fp[0];
    }
    exprStackTop.real = (IR_NODE *)leaf_lookup(    
                            CONST_LEAF,            
                            currentFloatType,    
                            (LEAF_VALUE *)&const);
} 
 
static void
build_imaginary_constant(currentLeaf) 
    LEAF *currentLeaf;  
{  
    struct constant const;

    if (currentLeaf->val.const.isbinary) {
        /* constant is in binary form */
        const.isbinary = IR_TRUE;
        const.c.bytep[0] = currentLeaf->val.const.c.bytep[1];
    }
    else {
        /* constant is in string form */
        const.isbinary = IR_FALSE;
        const.c.fp[0] = currentLeaf->val.const.c.fp[1];
    }
    exprStackTop.imaginary = (IR_NODE *)leaf_lookup(
                                 CONST_LEAF,
                                 currentFloatType,
                                 (LEAF_VALUE *)&const);
}  

void
there_are_complex_operations()
{
    thereAreComplexOperations = IR_TRUE;
}

BOOLEAN
worth_optimizing_complex_operations()
{
    BOOLEAN temp = thereAreComplexOperations;
    thereAreComplexOperations = IR_FALSE;
    return temp;
}

static BOOLEAN
complex_reference(currentLeaf)
    LEAF *currentLeaf;
{
    if (ISCOMPLEX(currentLeaf->type.tword)) { 
        return IR_TRUE;
    }
    return IR_FALSE;
}

void
expand_complex_leaves()
{
    LEAF *currentLeaf;

    /* run through leaf table breaking complex
       variables into real and imaginary parts */
    for (currentLeaf = leaf_tab; 
         currentLeaf; 
         currentLeaf = currentLeaf->next_leaf) {
        if (complex_reference(currentLeaf) &&
            currentLeaf->class == VAR_LEAF) {

            /* build imaginary and real leaves */
            determine_complex_type((IR_NODE *)currentLeaf);
            build_imaginary_part(currentLeaf);
            build_real_part(currentLeaf);
        } 
    }
}

static BOOLEAN 
complex_operation(currentTriple)
    TRIPLE *currentTriple;
{
    if (ISCOMPLEX(currentTriple->type.tword)) {
        return IR_TRUE;
    }
    return IR_FALSE;
}

static BOOLEAN
complex_subtree(currentSubtree)
    IR_NODE *currentSubtree;
{
    if (currentSubtree->operand.tag == ISTRIPLE) {
        return complex_operation((TRIPLE *)currentSubtree);
    }
    return complex_reference((LEAF *)currentSubtree);
}

static void
break_up_constant(currentLeaf)
    LEAF *currentLeaf; 
{
    build_real_constant(currentLeaf);
    build_imaginary_constant(currentLeaf);
}

static void
break_up_reference(currentLeaf)
    LEAF *currentLeaf; 
{ 
/*
    if (currentLeaf->is_volatile) {
        warning(
            TNULL, 
            "expansion of volatile complex leaf not yet implemented");    
    }
    else {
*/
        /* case of a regular variable */

        /* first two (float) leaves in the
           overlap list are guaranteed to be
           the split leaves as all complex
           leaves are split and the newly
           created leaves were inserted at
           the beginning of the overlap list */
        get_real_part(currentLeaf);
        get_imaginary_part(currentLeaf);
/*
    }
*/
} 
 
static void
expand_leaf(currentLeaf)
    LEAF *currentLeaf;
{
    if (currentLeaf->class == CONST_LEAF) {
        break_up_constant(currentLeaf);
    }
    else {
        /* case of a variable */  
        break_up_reference(currentLeaf);
    } 
}

static BOOLEAN
bottom_has_been_reached(currentTriple, expandingLeft)
    TRIPLE *currentTriple;
    BOOLEAN expandingLeft;
{
    /* assuming here for binary op if left is
       complex so is right */
    if (currentTriple->op == IR_FCALL ||
        currentTriple->op == IR_IFETCH) {
        return IR_TRUE;
    }
    if (expandingLeft) {
        return ((BOOLEAN)(!complex_subtree(currentTriple->left)));
    }
    else {
        return ((BOOLEAN)(!complex_subtree(currentTriple->right)));
    }
}

static void
expand_subtree(/*currentTriple, currentBlock, ascending, left*/);

static void
expand_left_subtree(currentTriple, currentBlock) 
    TRIPLE *currentTriple; 
    BLOCK  *currentBlock;
{ 
    if (bottom_has_been_reached(currentTriple, IR_TRUE)) {
        determine_complex_type((IR_NODE *)currentTriple);
        clear_expr_stack_top();
        return;
    }
    if (currentTriple->left->operand.tag == ISTRIPLE) {
        /* descend a level */
        expand_subtree(
            ((TRIPLE *)(currentTriple->left)), 
            currentBlock, 
            IR_FALSE, 
            IR_TRUE);
    }  
    else {
        /* we have a leaf */
        determine_complex_type(currentTriple->left);
        expand_leaf(((LEAF *)(currentTriple->left)));
    }
} 
 
static void
expand_right_subtree(currentTriple, currentBlock)
    TRIPLE *currentTriple;
    BLOCK  *currentBlock;
{
    if ((!currentTriple->right) ||
        (currentTriple->op == IR_PARAM)) {
        /* nothing to do on right side */
        clear_expr_stack_top();
        return;
    }
    if (bottom_has_been_reached(currentTriple, IR_FALSE)) {
        determine_complex_type((IR_NODE *)currentTriple);
        clear_expr_stack_top();
        return;
    } 
    if (currentTriple->right->operand.tag == ISTRIPLE) {
        /* descend a level */
        expand_subtree(
            ((TRIPLE *)(currentTriple->right)), 
            currentBlock, 
            IR_FALSE, 
            IR_FALSE);  
    }
    else {
        /* we have a leaf */
        determine_complex_type(currentTriple->right);
        expand_leaf(((LEAF *)(currentTriple->right)));
    }
}

static void
insert_split_triple(expr)
    struct exprStack *expr;
{
/* This routine expects expr->split to be
   set to a triple which returns a complex value */

    LEAF   *temp = new_temp(currentComplexType);
    TRIPLE *tp = append_triple( 
                     (TRIPLE *)expr->split,
                     IR_ASSIGN,
                     (IR_NODE *)temp,
                     expr->split,
                     currentComplexType);

    tp->visited = IR_TRUE;
 
    build_imaginary_part(temp);
    build_real_part(temp);
    get_real_part(temp);
    get_imaginary_part(temp);

    move_expr_stack_top_to(expr);
 
/* The real and imaginary fields of the expr
   have been set to the leaves for the real and
   imaginary parts and the expr->split field has
   been cleared */
}
 
static IR_NODE *
insert_combine_triple(afterTriple, expr)
    TRIPLE  *afterTriple;
    struct exprStack *expr;
{
/* This routine expects the real and imaginary subtrees
   to be combined to be in the expr fields */
 
    TRIPLE  *tp;
    LEAF    *temp = new_temp(currentComplexType);
 
    build_imaginary_part(temp);
    build_real_part(temp);
    get_real_part(temp);
    get_imaginary_part(temp);
 
    tp = append_triple(
             afterTriple,
             IR_ASSIGN,
             exprStackTop.real,
             expr->real,
             currentFloatType);
 
    tp = append_triple(
             tp,
             IR_ASSIGN,
             exprStackTop.imaginary,
             expr->imaginary,
             currentFloatType);
 
    clear_expr_stack_top();
    return (IR_NODE *)temp;
 
/*  The temp representing the combined complex value 
    is returned */
}
 
static void
split_if_necessary(expr)
    struct exprStack *expr;
{
    if (expr->split) {
        insert_split_triple(expr);
    }     
}

static void
complex_minus(currentTriple, left, right)
    TRIPLE  *currentTriple;
    struct exprStack *left;
    struct exprStack *right;
{
    split_if_necessary(left);
    split_if_necessary(right);
    currentTriple->left = left->real;
    currentTriple->right = right->real;
    currentTriple->type = currentFloatType;
    exprStackTop.real = (IR_NODE *)currentTriple;
             
    exprStackTop.imaginary = (IR_NODE *)append_triple(
                                 (TRIPLE *)exprStackTop.real,
                                 IR_MINUS,
                                 left->imaginary,
                                 right->imaginary,
                                 currentFloatType);
}

static void
complex_plus(currentTriple, left, right)
    TRIPLE  *currentTriple;
    struct exprStack *left;
    struct exprStack *right;
{
    split_if_necessary(left);
    split_if_necessary(right);
    currentTriple->left = left->real;
    currentTriple->right = right->real;
    currentTriple->type = currentFloatType;
    exprStackTop.real = (IR_NODE *)currentTriple;
             
    exprStackTop.imaginary = (IR_NODE *)append_triple(
                                 (TRIPLE *)exprStackTop.real,
                                 IR_PLUS,
                                 left->imaginary,
                                 right->imaginary,
                                 currentFloatType);
}

static void
common_if_necessary(expr)
    struct exprStack *expr;
{
    LEAF   *lp;

    /* assuming here that a split is not a possibility */
    if (expr->real->operand.tag == ISTRIPLE) {
        lp = new_temp(currentFloatType);
        (void)append_triple(
                 (TRIPLE *)expr->real,
                 IR_ASSIGN,
                 (IR_NODE *)lp,
                 expr->real,
                 currentFloatType); 
        expr->real = (IR_NODE *)lp;
    }
    if (expr->imaginary->operand.tag == ISTRIPLE) {
        lp = new_temp(currentFloatType); 
        (void)append_triple( 
                 (TRIPLE *)expr->imaginary,
                 IR_ASSIGN, 
                 (IR_NODE *)lp,
                 expr->imaginary,
                 currentFloatType);
        expr->imaginary = (IR_NODE *)lp;
    }
}

static void
complex_cmplx(currentTriple)
    TRIPLE *currentTriple;
{
    currentTriple = currentTriple;
}

static void
complex_cnjg(currentTriple)
    TRIPLE *currentTriple;
{ 
    currentTriple = currentTriple; 

} 
 
static void
complex_imag(currentTriple)
    TRIPLE  *currentTriple;
{
    TRIPLE  *parentTriple;
    IR_NODE *left;
    IR_NODE *addrOfLeft;

    addrOfLeft = (IR_NODE *)append_triple(
        currentTriple->tprev,
        IR_PLUS,
        ((TRIPLE *)(currentTriple->right))->left,
        (IR_NODE *)(ileaf(currentFloatType.size)),
        determine_current_type(((TRIPLE *)(currentTriple->right))->left));
 
    (void)free_triple(
        (TRIPLE *)currentTriple->right,
        (BLOCK *)NULL);

    left = (IR_NODE *)append_triple(
        currentTriple->tprev,
        IR_IFETCH,
        addrOfLeft,
        (IR_NODE *)NULL,
        currentFloatType);
    ((TRIPLE *)(left))->can_access = currentTriple->can_access;
    currentTriple->can_access = LNULL;
 
    parentTriple = find_parent(currentTriple);
    if ((parentTriple->left->operand.tag == ISTRIPLE) &&
        (((TRIPLE *)(parentTriple->left)) == currentTriple)) {
        parentTriple->left = left;
    }
    else {
        parentTriple->right = left;
    }
    (void)free_triple(
        currentTriple,
        (BLOCK *)NULL);
}

static void
complex_mult(currentTriple, left, right)
    TRIPLE  *currentTriple;
    struct exprStack *left;
    struct exprStack *right;
{
    complexMultiply = IR_TRUE;
    split_if_necessary(left);
    split_if_necessary(right);
    common_if_necessary(left);
    common_if_necessary(right);

    exprStackTop.real = 
        (IR_NODE *)append_triple(
            currentTriple->tprev,
            IR_MINUS,
            (IR_NODE *)append_triple(
                currentTriple->tprev,
                IR_MULT,
                left->real,
                right->real,
                currentFloatType),
            (IR_NODE *)append_triple(
                currentTriple->tprev,
                IR_MULT,
                left->imaginary,
                right->imaginary,
                currentFloatType),
            currentFloatType);

    exprStackTop.imaginary =
        (IR_NODE *)append_triple(
            currentTriple,
            IR_PLUS,
            (IR_NODE *)append_triple(
                currentTriple->tprev,
                IR_MULT,
                left->real,
                right->imaginary,
                currentFloatType),
            (IR_NODE *)currentTriple,
            currentFloatType);
             
    currentTriple->left = right->real;
    currentTriple->right = left->imaginary;
    currentTriple->type = currentFloatType;
}

static void
complex_neg(currentTriple, left) 
    TRIPLE  *currentTriple;
    struct exprStack *left; 
{
    split_if_necessary(left);
    currentTriple->left = left->real;
    currentTriple->type = currentFloatType;
    exprStackTop.real = (IR_NODE *)currentTriple;

    exprStackTop.imaginary = (IR_NODE *)append_triple(
                                 (TRIPLE *)exprStackTop.real,
                                 IR_NEG,
                                 left->imaginary,
                                 (IR_NODE *)NULL,
                                 currentFloatType);
} 
 
static void
complex_assign(currentTriple, left, right) 
    TRIPLE  *currentTriple;
    struct exprStack *left; 
    struct exprStack *right; 
{
    TRIPLE *tp;
    LIST   *lp;
    LIST   *listElement;
    LEAF   *temp;

    if (worth_splitting(right)) {
        split_if_necessary(left);

        /* real part */
        currentTriple->type = currentFloatType;
        currentTriple->right = right->real;
        if (there_was_a_complex_multiply()) {
            /* have to save real result in a temp
               until imaginary part is executed as
               there may be a use of the real part
               as an operand of the multiply */
            temp = new_temp(currentFloatType);
            currentTriple->left = (IR_NODE *)temp;
        }
        else { 
            currentTriple->left = left->real;
        }

        /* imaginary part */
        tp = append_triple(
                 currentTriple,
                 IR_ASSIGN,
                 left->imaginary,
                 right->imaginary,
                 currentFloatType);

        /* copy can_access from original */
        LFOR(lp, currentTriple->can_access) {
            listElement = NEWLIST(proc_lq);
            (LEAF *)listElement->datap = LCAST(lp, LEAF);
            LAPPEND(tp->can_access, listElement);
        } 
    
        if (there_was_a_complex_multiply()) {
            tp = append_triple(
                tp,
                IR_ASSIGN,
                left->real,
                (IR_NODE *)temp,
                currentFloatType);

            LFOR(lp, currentTriple->can_access) {
                listElement = NEWLIST(proc_lq);
                (LEAF *)listElement->datap = LCAST(lp, LEAF);
                LAPPEND(tp->can_access, listElement);
            }
        }
    }
    else {
        /* mark visited so we will not visit again */
        currentTriple->visited = IR_TRUE;
    }
} 
 
static BOOLEAN
library_function_cmplx(currentTriple)
    TRIPLE *currentTriple;
{
    if ((((LEAF *)(currentTriple->left))->func_descr == INTR_FUNC) &&
        ((currentComplexType.size == (2 * floattype.size) &&
          (strncmp(
             ((LEAF *)(currentTriple->left))->val.const.c.cp,
             "_c_cmplx",
             8) == 0)) ||
         (strncmp(
             ((LEAF *)(currentTriple->left))->val.const.c.cp,
             "_d_cmplx",
             8) == 0))) {
        return IR_TRUE;
    }
    return IR_FALSE;
}

static void
complex_fcall(currentTriple)
    TRIPLE  *currentTriple;
{
    exprStackTop.split = (IR_NODE *)currentTriple;
    currentTriple->visited = IR_TRUE;
}

BOOLEAN 
constant_is_zero(currentLeaf)
    LEAF *currentLeaf;
{
    return (BOOLEAN)!currentLeaf;
}

static BOOLEAN
converted_float(expr)
    struct exprStack *expr;
{
    if ((expr->imaginary) &&
        (expr->imaginary->operand.tag == ISLEAF) &&
        (((LEAF *)(expr->imaginary))->class == CONST_LEAF) &&
        (constant_is_zero(((LEAF *)(expr->imaginary))))) {
       return IR_TRUE;
    } 
    return IR_FALSE;
}

static void
complex_division(currentTriple, left, right)
    TRIPLE  *currentTriple;
    struct exprStack *left;
    struct exprStack *right;
{
/*
    LEAF    *temp;
    TRIPLE  *tp;
*/

    /* special case division by real */
    if (currentTriple->op == IR_DIV &&
        converted_float(right)) {
        split_if_necessary(left);
        currentTriple->left = left->real;
        currentTriple->right = right->real;
        currentTriple->type = currentFloatType;
        exprStackTop.real = (IR_NODE *)currentTriple;        

        exprStackTop.imaginary = (IR_NODE *)append_triple(
            (TRIPLE *)exprStackTop.real,
            IR_DIV,
            left->imaginary,
            right->real,
            currentFloatType);
        return;
    }

    /* general case */
    if (worth_splitting(left)) {
        currentTriple->left = insert_combine_triple(currentTriple->tprev, left);
    }
    if (worth_splitting(right)) { 
        currentTriple->right = insert_combine_triple(currentTriple->tprev, right);
    }
    exprStackTop.split = (IR_NODE *)currentTriple;
    currentTriple->visited = IR_TRUE;

/* Need check for division by zero in here as well
    split_if_necessary(left);
    split_if_necessary(right);
    common_if_necessary(left);
    common_if_necessary(right);

    temp = new_temp(currentFloatType);
    (void)append_triple(
             currentTriple->tprev,
             IR_ASSIGN,
             (IR_NODE *)temp,
             (IR_NODE *)append_triple(  
                 currentTriple->tprev,  
                 IR_PLUS,
                 (IR_NODE *)append_triple(
                     currentTriple->tprev,  
                     IR_MULT,  
                     right->imaginary, 
                     right->imaginary, 
                     currentFloatType),
                 (IR_NODE *)append_triple(
                     currentTriple->tprev,
                     IR_MULT,
                     right->real,
                     right->real,
                     currentFloatType),
                 currentFloatType),
             currentFloatType);

    tp = append_triple(
             currentTriple->tprev,
             IR_DIV,
             (IR_NODE *)append_triple(
                 currentTriple->tprev,
                 IR_PLUS,
                 (IR_NODE *)append_triple(
                     currentTriple->tprev,
                     IR_MULT,
                     left->real,
                     right->real,
                     currentFloatType),
                 (IR_NODE *)append_triple( 
                     currentTriple->tprev, 
                     IR_MULT,
                     left->imaginary,
                     right->imaginary,
                     currentFloatType),
                 currentFloatType),
             (IR_NODE *)temp,
             currentFloatType);

    exprStackTop.real = (IR_NODE *)tp;

    exprStackTop.imaginary = 
        (IR_NODE *)append_triple(
             currentTriple->tprev,
             IR_DIV,
             (IR_NODE *)append_triple(
                 currentTriple->tprev,
                 IR_MINUS,
                 (IR_NODE *)append_triple(
                     currentTriple->tprev,
                     IR_MULT,
                     right->real,         
                     left->imaginary,
                     currentFloatType),
                 (IR_NODE *)append_triple( 
                     currentTriple->tprev, 
                     IR_MULT, 
                     right->imaginary,
                     left->real,
                     currentFloatType),
                 currentFloatType), 
             (IR_NODE *)temp, 
             currentFloatType); 

    (void)free_triple(
        currentTriple,
        (BLOCK *)NULL);
*/
}

static void
complex_conv(currentTriple, left)
    TRIPLE  *currentTriple; 
    struct exprStack *left;
{
    TYPE currentType;

    if (left->real || left->split) {
        /* case of a conversion between complex types  as we
           have already expanded the left subtree */
        currentType = determine_current_type((IR_NODE *)currentTriple);
        if (currentType.tword == currentComplexType.tword) {
            /* case of unnecessary coversion */
            exprStackTop.real = left->real;
            exprStackTop.imaginary = left->imaginary;
            (void)free_triple(
                currentTriple,
                (BLOCK *)NULL);
        }
        else {
            split_if_necessary(left);
            determine_complex_type((IR_NODE *)currentTriple);
            currentTriple->type = currentFloatType;
            currentTriple->left = left->real;
            exprStackTop.real = (IR_NODE *)currentTriple;
 
            exprStackTop.imaginary = (IR_NODE *)append_triple(
                currentTriple,
                IR_CONV,
                left->imaginary,
                (IR_NODE *)NULL,
                currentFloatType);
        }
        return;
    }

    currentType = determine_current_type(currentTriple->left);
    if (currentType.tword == FLOAT) {
        if (currentFloatType.tword == FLOAT) {
            /* no coversion is necessary */
            exprStackTop.real = currentTriple->left;
            exprStackTop.imaginary = 
                ((IR_NODE *)(fleaf_t(0.0, currentFloatType))); 
            (void)free_triple(
                currentTriple,
                (BLOCK *)NULL);
        }
        else {
            currentTriple->type = currentFloatType; 
            exprStackTop.real = (IR_NODE *)currentTriple;
            exprStackTop.imaginary =
                ((IR_NODE *)(fleaf_t(0.0, currentFloatType)));
        }
        return;
    }
    else {
        if (currentType.tword == DOUBLE) {
            if (currentFloatType.tword == FLOAT) {
                /* conversion necessary */
                currentTriple->type = currentFloatType;   
                exprStackTop.real = (IR_NODE *)currentTriple;  
                exprStackTop.imaginary =
                    ((IR_NODE *)(fleaf_t(0.0, currentFloatType)));
            }
            else { 
                exprStackTop.real = currentTriple->left;  
                exprStackTop.imaginary =  
                    ((IR_NODE *)(fleaf_t(0.0, currentFloatType)));  
                (void)free_triple( 
                    currentTriple,  
                    (BLOCK *)NULL);
            }
            return;
        }
        else {
            if (currentType.tword == INT) {
                /* conversion necessary */ 
                currentTriple->type = currentFloatType;    
                exprStackTop.real = (IR_NODE *)currentTriple;   
                exprStackTop.imaginary = 
                    ((IR_NODE *)(fleaf_t(0.0, currentFloatType)));
                return;
            }
        }
    }

    exprStackTop.split = (IR_NODE *)currentTriple;
    currentTriple->visited = IR_TRUE;
}

static void
non_complex_conv(currentTriple, left)
    TRIPLE  *currentTriple;
    struct exprStack *left;
{
    TRIPLE *parentTriple;
 
    /* could only get here if ascending */
    
    if (same_irtype(
            currentTriple->type,
            currentFloatType)) {
        /* conversion of complex to type of real part */ 
        split_if_necessary(left);
        parentTriple = find_parent(currentTriple);
        if ((parentTriple->left->operand.tag == ISTRIPLE) &&
            (((TRIPLE *)(parentTriple->left)) == currentTriple)) {
            parentTriple->left = left->real;
        }
        else {
            parentTriple->right = left->real;
        }
        (void)free_triple(
            currentTriple,
            (BLOCK *)NULL);

        /* note subtree is left dangling, resequence of triples
           will clean it up, retaining side effects under a FOREFF
           triple */
    }
    else { 
        /* only the real part will be converted */
        split_if_necessary(left);
        currentTriple->left = left->real;
    } 

    /* old way 
    if (worth_splitting(left)) {
        currentTriple->left =
            insert_combine_triple(currentTriple->tprev, left);
    }
       since type of conv is non complex there is no
       chance that we will visit it again */
}

static void
complex_fval(currentTriple, left) 
    TRIPLE  *currentTriple;  
    struct exprStack *left;
{ 
    if (worth_splitting(left)) {
        currentTriple->left = 
            insert_combine_triple(currentTriple->tprev, left);
    }
    currentTriple->visited = IR_TRUE;
} 
 
static void
complex_param(currentTriple, left)  
    TRIPLE  *currentTriple;   
    struct exprStack *left; 
{  
    if (worth_splitting(left)) {
        currentTriple->left =
            insert_combine_triple(currentTriple->tprev, left);
    }
    /* don't need to set visited as we will not be visiting
       params by any other path (ie, this is it, we will not
       see this guy again) */
}  
 
static void
complex_istore(currentTriple, right)   
    TRIPLE  *currentTriple;    
    struct exprStack *right;  
{   
    TYPE    addrType;
    LEAF   *temp;
    TRIPLE *tp;
    LIST   *lp;
    LIST   *listElement;
    LEAF   *newTemp;

/*
    if (worth_splitting(right)) {
        currentTriple->right =
            insert_combine_triple(currentTriple->tprev, right);
    }
*/

     if (worth_splitting(right)) {
        /* common addressing expression under istore if necessary */
        addrType = determine_current_type(currentTriple->left);
        if (currentTriple->left->operand.tag == ISTRIPLE) {
            temp = new_temp(addrType);
            (void)append_triple(
                 currentTriple->tprev,
                 IR_ASSIGN,
                 (IR_NODE *)temp,
                 currentTriple->left,
                 addrType);
        }
        else {
            /* not necessary to common */
            temp = (LEAF *)currentTriple->left;
        }

        /* build real part */
        currentTriple->type = currentFloatType;
        currentTriple->right = right->real;
        if (there_was_a_complex_multiply()) {
            /* need to delay store to real part as the
               real part may be an operand of a multiply
               within the imaginary subtree */
            newTemp = new_temp(currentFloatType);
            currentTriple->op = IR_ASSIGN;
            currentTriple->left = (IR_NODE *)newTemp;
        }
        else {
            currentTriple->left = (IR_NODE *)temp;
        }

        /* build imaginary part */
        tp = append_triple(
                 currentTriple,
                 IR_PLUS,
                 (IR_NODE *)temp,
                 (IR_NODE *)(ileaf(currentFloatType.size)),
                 addrType);

        tp = append_triple(
                 tp,
                 IR_ISTORE,
                 (IR_NODE *)tp,
                 right->imaginary,
                 currentFloatType);

        /* copy can_access from original */
        LFOR(lp, currentTriple->can_access) {
            listElement = NEWLIST(proc_lq);
            (LEAF *)listElement->datap = LCAST(lp, LEAF);
            LAPPEND(tp->can_access, listElement);
        }

        if (there_was_a_complex_multiply()) {
            tp = append_triple(
                tp,
                IR_ISTORE,
                (IR_NODE *)temp,
                (IR_NODE *)newTemp,
                currentFloatType);

            LFOR(lp, currentTriple->can_access) {
                listElement = NEWLIST(proc_lq);
                (LEAF *)listElement->datap = LCAST(lp, LEAF);
                LAPPEND(tp->can_access, listElement);
            }
        }
    }
    else {
        /* mark visited so we will not visit again */
        currentTriple->visited = IR_TRUE;
    }
}   
 
static void
complex_ifetch(currentTriple)   
    TRIPLE  *currentTriple;    
{   
    TYPE    addrType;
    LEAF   *temp;
    TRIPLE *tp;
    LIST   *lp;
    LIST   *listElement;

/*
    exprStackTop.split = (IR_NODE *)currentTriple;
    currentTriple->visited = IR_TRUE;
    return;
*/
    /* common addressing expression under ifetch if necessary */
    addrType = determine_current_type(currentTriple->left);
    if (currentTriple->left->operand.tag == ISTRIPLE) {
        temp = new_temp(addrType);
        (void)append_triple(
             currentTriple->tprev,
             IR_ASSIGN,
             (IR_NODE *)temp,
             currentTriple->left,
             addrType);
    }
    else {
        /* not necessary to common */
        temp = (LEAF *)currentTriple->left;
    }

    /* build real part */
    currentTriple->type = currentFloatType;
    currentTriple->left = (IR_NODE *)temp;
    exprStackTop.real = (IR_NODE *)currentTriple;

    /* build imaginary part */
    tp = append_triple( 
             currentTriple, 
             IR_PLUS,
             (IR_NODE *)temp,
             (IR_NODE *)(ileaf(currentFloatType.size)),
             addrType);

    tp = append_triple(
             tp,
             IR_IFETCH,
             (IR_NODE *)tp,
             (IR_NODE *)NULL,
             currentFloatType);
     
    exprStackTop.imaginary = (IR_NODE *)tp;

    /* copy can_access from original */
    LFOR(lp, currentTriple->can_access) {
        listElement = NEWLIST(proc_lq);
        (LEAF *)listElement->datap = LCAST(lp, LEAF);
        LAPPEND(tp->can_access, listElement);
    }
}   
 
static void
complex_foreff(currentTriple, left)
    TRIPLE  *currentTriple;
    struct exprStack *left;
{
    if (worth_splitting(left)) {
        (void)append_triple(
              currentTriple->tprev,
              IR_FOREFF,
              left->real,
              (IR_NODE *)NULL,
              currentFloatType);
        
        currentTriple->left = left->imaginary;
        currentTriple->type = currentFloatType; 
    }
    currentTriple->visited = IR_TRUE;
}

static void
handle_current_complex_operation(currentTriple, left, right)
    TRIPLE  *currentTriple;
    struct exprStack *left;
    struct exprStack *right;
{
    switch (currentTriple->op) {
        case IR_ASSIGN:
            complex_assign(currentTriple, left, right);
            break;

        case IR_FOREFF:
            complex_foreff(currentTriple, left);
            break;

        case IR_MINUS:
            complex_minus(currentTriple, left, right);
            break;

        case IR_PLUS:
            complex_plus(currentTriple, left, right);
            break;

        case IR_MULT:
            complex_mult(currentTriple, left, right);
            break;

        case IR_NEG:
            complex_neg(currentTriple, left);
            break;

        case IR_IFETCH:
            complex_ifetch(currentTriple);
            break;

        case IR_ISTORE:
            complex_istore(currentTriple, right);
            break;

        case IR_FCALL:
            complex_fcall(currentTriple);
            break;
 
        case IR_DIV:
        case IR_REMAINDER:
            complex_division(currentTriple, left, right); 
            break;
 
        case IR_CONV:
            complex_conv(currentTriple, left);  
            break;
 
        case IR_FVAL:
            complex_fval(currentTriple, left);   
            break;
 
        case IR_PARAM:
            complex_param(currentTriple, left);   
            break;
 
        default:
            warning(
                currentTriple,
                "unexpected complex expression tree");
    }
}

static void
complex_comparison(currentTriple, left, right)
    TRIPLE  *currentTriple;
    struct exprStack *left;
    struct exprStack *right;
{
    /* check left */
    if (worth_splitting(left)) {
        /* left subtree was split up */
        currentTriple->left = insert_combine_triple(currentTriple->tprev,left);
    }
    /* check right */
    if (worth_splitting(right)) { 
        /* right subtree was split up */
        currentTriple->right = insert_combine_triple(currentTriple->tprev,right); 
    }
}

static void
handle_current_non_complex_operation(currentTriple, left, right)
    TRIPLE  *currentTriple; 
    struct exprStack *left;
    struct exprStack *right;
{ 
    switch (currentTriple->op) {
        /* the following three cases are included here as they
           are currently without a type (set to void) */
        case IR_ASSIGN:
            complex_assign(currentTriple, left, right);
            break;
        case IR_ISTORE:
            complex_istore(currentTriple, right);
            break; 
        case IR_FOREFF:
            complex_foreff(currentTriple, left);
            break;

        case IR_CONV:
            non_complex_conv(currentTriple, left);
            break;

        case IR_GE:
        case IR_GT:
        case IR_LT:
        case IR_LE:
        case IR_EQ:
            complex_comparison(currentTriple, left, right);
            break;
 
        default:
            warning(
                currentTriple,
                "unexpected parent of complex expression subtree");
    }
}
 
static BOOLEAN
top_has_been_reached(currentTriple)
    TRIPLE *currentTriple;
{
    if (currentTriple->op == IR_ASSIGN ||
        currentTriple->op == IR_PARAM ||
        currentTriple->op == IR_ISTORE ||
        currentTriple->op == IR_FVAL ||
        currentTriple->op == IR_FOREFF) {
        return IR_TRUE;
    }
    else {
        return IR_FALSE;
    }
}

static void 
expand_subtree(currentTriple, currentBlock, ascending, left)
    TRIPLE  *currentTriple;
    BLOCK   *currentBlock;
    BOOLEAN  ascending;
    BOOLEAN  left;
{
    TRIPLE  *parentTriple;
    BOOLEAN  ascendingLeft;

    struct exprStack leftHandSide;
    struct exprStack rightHandSide;

    /* if ascending, we must save parent so we can trash
       currentTriple if we want */
    if (ascending) {
        if (top_has_been_reached(currentTriple)) {
            ascending = IR_FALSE;
        }
        else {
            /* save parent for later */
            parentTriple = find_parent(currentTriple);
            ascendingLeft = (BOOLEAN)
                ((parentTriple->left->operand.tag == ISTRIPLE) &&
                 (((TRIPLE *)(parentTriple->left)) == currentTriple));
        }

        /* initialize left and right hand sides */
        if (left) {
            move_expr_stack_top_to(&leftHandSide);
            expand_right_subtree(currentTriple, currentBlock);
            move_expr_stack_top_to(&rightHandSide);
        }
        else {
            move_expr_stack_top_to(&rightHandSide);
            expand_left_subtree(currentTriple, currentBlock);
            move_expr_stack_top_to(&leftHandSide);
        }
    }
    else {
        /* descending; no need to save parent as a routine up the
           call stack has it; no left or right hand sides yet */
        expand_left_subtree(currentTriple, currentBlock);
        move_expr_stack_top_to(&leftHandSide);
        expand_right_subtree(currentTriple, currentBlock);
        move_expr_stack_top_to(&rightHandSide);
    }

    if (complex_operation(currentTriple)) {
        handle_current_complex_operation(
            currentTriple,
            &leftHandSide,
            &rightHandSide);

        /* visit parent */
        if (ascending) {
            /* then ascend */
            expand_subtree(
                parentTriple, 
                currentBlock, 
                IR_TRUE, 
                ascendingLeft);
        }
    }
    else {
        handle_current_non_complex_operation(
            currentTriple,
            &leftHandSide,
            &rightHandSide);
    }
}

static BOOLEAN
conversion_from_complex(currentTriple)
    TRIPLE *currentTriple;
{
    if (currentTriple->op == IR_CONV &&
        complex_subtree(currentTriple->left)) {
        return IR_TRUE;
    }
    return IR_FALSE;
}

static BOOLEAN
special_complex_call(/*calledFunction*/)
/*
    LEAF *calledFunction;
*/
{
    /* add code in here when we want to start
       expanding these (maybe we can make them
       ir operators and we won't need this
       routine at all) */
    return (BOOLEAN)IR_FALSE;
}

static void
handle_special_complex_call(currentTriple)
    TRIPLE *currentTriple;
{
    if (special_call == IMAG) {
        complex_imag(currentTriple);
    }
    else {
        if (special_call == CMPLX) {
            complex_cmplx(currentTriple);
        }
        else {
            /* special_call == CNJG */
            complex_cnjg(currentTriple);
        }
    }
}
  
void 
expand_complex_operations()
{
    TRIPLE *currentTriple; 
    BLOCK  *currentBlock;
    TRIPLE *previousTriple;
    struct exprStack left;

    /* run through triples expanding complex expression
       trees into real and imaginary expression trees */
    clear_expr_stack_top();
    for (currentBlock = entry_block;
         currentBlock;
         currentBlock = currentBlock->next) {
        TFOR(currentTriple, currentBlock->last_triple->tnext) {
            currentTriple->visited = IR_FALSE;
        }
        TFOR(currentTriple, currentBlock->last_triple->tnext) {
            /* check for visited triples to avoid 
               retracing previous path */
            if (!(currentTriple->visited)) {
                previousTriple = currentTriple->tprev;
                if (complex_operation(currentTriple)) {
                    no_complex_multiplies_have_been_seen_yet();
                    expand_left_subtree(currentTriple, currentBlock);
                    expand_subtree(
                        currentTriple, 
                        currentBlock, 
                        IR_TRUE, 
                        IR_TRUE);
                    currentTriple = previousTriple;
                }
                else {
                    if (conversion_from_complex(currentTriple)) {
                        expand_left_subtree(currentTriple, currentBlock);
                        move_expr_stack_top_to(&left);
                        non_complex_conv(currentTriple, &left);
                        currentTriple = previousTriple;
                    }
                    else {
                        if (special_complex_call(/*((LEAF *)(currentTriple->left))*/)) {
                            handle_special_complex_call(currentTriple);
                            currentTriple = previousTriple;
                        }
                        else {
                            /* skip as currentTriple is non-complex */
                            currentTriple->visited = IR_TRUE;
                        }
                    }
                }
            }
        }
    }
}

void 
remove_unused_complex_leaves()
{
    /* maybe we blew away the imaginary part so
       we can blow aray the complex variable and
       leave the real part only */

    /* do nothing for now ??? */
    return;
}
