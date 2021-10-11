/*      @(#)complex_expansion.h 1.1 94/10/31 SMI      */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef __complex_expansion__
#define __complex_expansion__

/* call the following routine to expand
   complex variables into a real part
   and a imaginary part */
void expand_complex_leaves();

/* the following routine will return 
   IR_TRUE iff at least one complex leaf
   was encountered during last call of
   expand_complex_leaves */
BOOLEAN worth_optimizing_complex_operations();

/* use the following to indicate the presence 
   of complex operations */
void there_are_complex_operations();
 
/* call the following routine to expand
   and lay bare for optimization complex
   operations */
void expand_complex_operations();

/* call the following routine after
   optimization has occurred to remove
   complex variables which have been
   patially or fully optimized away */
void remove_unused_complex_leaves();


/* FURTHER ENHANCEMENTS

   1.  Code could be added to expand 
       the calls the fortran routines:
       imag, cmplx and cnjg (prefixed
       with type e.g., r_imag for the
       function returning real).  This
       process was begun but was not 
       completed as it is desirable
       that a future fortran frontend
       not create the routine call and
       accompanying tree but, instead,
       use a new operator.

   2.  ABS could be implemented if
       there was a square root operator 
       in ir.
*/

#endif
