#ifndef lint
static char sccsid[] = "@(#)demo_mat_util.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_mat_util.c
 *
 *	@(#)demo_mat_util.c 1.1 90/09/27 10:44:13
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the matrix utility routines.
 *
 *  3-Apr-90 Scott R. Nelson	  Initial version.
 *  9-May-90 Scott R. Nelson	  Integration with the rest of the code.
 * 19-Jul-90 Scott R. Nelson	  Added comments.  Optimized rotation code.
 * 21-Sep-90 Scott R. Nelson	  Added invert routine and changed the
 *				  name from demo_dl_mod to demo_mat_util
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <math.h>
#include "demo.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	identity
 *	concat_mat
 *	copy_mat
 *	scale_xyz
 *	    identity
 *	translate
 *	    identity
 *	rot_x
 *	    identity
 *	    sin
 *	    cos
 *	rot_y
 *	    identity
 *	    sin
 *	    cos
 *	rot_z
 *	    identity
 *	    sin
 *	    cos
 *	invert
 *	    copy_mat
 *	    identity
 *
 *==============================================================
 */



#define TOLERANCE	(1.0e-5)	/* Floating point tolerance */
#define N		4		/* The order of the matrix */
#define MAX(a,b) ((a) >= (b) ? (a) : (b))

/* Compute the 1D index from the row and column indices and the order */
#define INDX(r,c) (iperm[(r)] * N + (c))

/* Globals */
static int iperm[N];			/* Permutation vector */



/*
 *--------------------------------------------------------------
 *
 * identity
 *
 *	Turn the specified matrix into an identity matrix.
 *
 *--------------------------------------------------------------
 */

void
identity(m)
    Hk_dmatrix m;
{
    *m++ = 1.0;				/* We were passed a pointer to double */
    *m++ = 0.0;				/* Use brute force method for speed */
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 1.0;				/* 1,1 */
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 1.0;				/* 2,2 */
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 1.0;				/* 3,3 */
} /* End of identity */



/*
 *--------------------------------------------------------------
 *
 * concat_mat
 *
 *	Multiply the first two matrices and store the result in
 *	the third.  Note that the result matrix may be the same
 *	matrix as either of the source matrices.
 *
 *--------------------------------------------------------------
 */

void
concat_mat(m1, m2, m3)
    Hk_dmatrix m1, m2;			/* The matrices to multiply */
    Hk_dmatrix m3;			/* Matrix to put the result in */
{
    Hk_dmatrix mtmp;			/* Temporary result matrix */
    double *src;			/* Pointer for final copy */

/* Multiply the two matrices */

    mtmp[HKM_0_0] = m1[HKM_0_0] * m2[HKM_0_0] + m1[HKM_0_1] * m2[HKM_1_0]
		  + m1[HKM_0_2] * m2[HKM_2_0] + m1[HKM_0_3] * m2[HKM_3_0];
    mtmp[HKM_0_1] = m1[HKM_0_0] * m2[HKM_0_1] + m1[HKM_0_1] * m2[HKM_1_1]
		  + m1[HKM_0_2] * m2[HKM_2_1] + m1[HKM_0_3] * m2[HKM_3_1];
    mtmp[HKM_0_2] = m1[HKM_0_0] * m2[HKM_0_2] + m1[HKM_0_1] * m2[HKM_1_2]
		  + m1[HKM_0_2] * m2[HKM_2_2] + m1[HKM_0_3] * m2[HKM_3_2];
    mtmp[HKM_0_3] = m1[HKM_0_0] * m2[HKM_0_3] + m1[HKM_0_1] * m2[HKM_1_3]
		  + m1[HKM_0_2] * m2[HKM_2_3] + m1[HKM_0_3] * m2[HKM_3_3];
    mtmp[HKM_1_0] = m1[HKM_1_0] * m2[HKM_0_0] + m1[HKM_1_1] * m2[HKM_1_0]
		  + m1[HKM_1_2] * m2[HKM_2_0] + m1[HKM_1_3] * m2[HKM_3_0];
    mtmp[HKM_1_1] = m1[HKM_1_0] * m2[HKM_0_1] + m1[HKM_1_1] * m2[HKM_1_1]
		  + m1[HKM_1_2] * m2[HKM_2_1] + m1[HKM_1_3] * m2[HKM_3_1];
    mtmp[HKM_1_2] = m1[HKM_1_0] * m2[HKM_0_2] + m1[HKM_1_1] * m2[HKM_1_2]
		  + m1[HKM_1_2] * m2[HKM_2_2] + m1[HKM_1_3] * m2[HKM_3_2];
    mtmp[HKM_1_3] = m1[HKM_1_0] * m2[HKM_0_3] + m1[HKM_1_1] * m2[HKM_1_3]
		  + m1[HKM_1_2] * m2[HKM_2_3] + m1[HKM_1_3] * m2[HKM_3_3];
    mtmp[HKM_2_0] = m1[HKM_2_0] * m2[HKM_0_0] + m1[HKM_2_1] * m2[HKM_1_0]
		  + m1[HKM_2_2] * m2[HKM_2_0] + m1[HKM_2_3] * m2[HKM_3_0];
    mtmp[HKM_2_1] = m1[HKM_2_0] * m2[HKM_0_1] + m1[HKM_2_1] * m2[HKM_1_1]
		  + m1[HKM_2_2] * m2[HKM_2_1] + m1[HKM_2_3] * m2[HKM_3_1];
    mtmp[HKM_2_2] = m1[HKM_2_0] * m2[HKM_0_2] + m1[HKM_2_1] * m2[HKM_1_2]
		  + m1[HKM_2_2] * m2[HKM_2_2] + m1[HKM_2_3] * m2[HKM_3_2];
    mtmp[HKM_2_3] = m1[HKM_2_0] * m2[HKM_0_3] + m1[HKM_2_1] * m2[HKM_1_3]
		  + m1[HKM_2_2] * m2[HKM_2_3] + m1[HKM_2_3] * m2[HKM_3_3];
    mtmp[HKM_3_0] = m1[HKM_3_0] * m2[HKM_0_0] + m1[HKM_3_1] * m2[HKM_1_0]
		  + m1[HKM_3_2] * m2[HKM_2_0] + m1[HKM_3_3] * m2[HKM_3_0];
    mtmp[HKM_3_1] = m1[HKM_3_0] * m2[HKM_0_1] + m1[HKM_3_1] * m2[HKM_1_1]
		  + m1[HKM_3_2] * m2[HKM_2_1] + m1[HKM_3_3] * m2[HKM_3_1];
    mtmp[HKM_3_2] = m1[HKM_3_0] * m2[HKM_0_2] + m1[HKM_3_1] * m2[HKM_1_2]
		  + m1[HKM_3_2] * m2[HKM_2_2] + m1[HKM_3_3] * m2[HKM_3_2];
    mtmp[HKM_3_3] = m1[HKM_3_0] * m2[HKM_0_3] + m1[HKM_3_1] * m2[HKM_1_3]
		  + m1[HKM_3_2] * m2[HKM_2_3] + m1[HKM_3_3] * m2[HKM_3_3];

/* Copy the result to the specified destination */

    src = mtmp;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
    *m3++ = *src++;
} /* End of concat_mat */



/*
 *--------------------------------------------------------------
 *
 * copy_mat
 *
 *	Multiply the first two matrices and store the result in
 *	the third.  Note that the result matrix may be the same
 *	matrix as either of the source matrices.
 *
 *--------------------------------------------------------------
 */

static void
copy_mat(m1, m2)
    Hk_dmatrix m1;			/* Source matrix */
    Hk_dmatrix m2;			/* Destination matrix */
{
    int i;

    for (i = 0; i < 16; i++)
	*m2++ = *m1++;

} /* End of copy_mat */



/*
 *--------------------------------------------------------------
 *
 * scale_xyz
 *
 *	Create a scale matrix.
 *
 *--------------------------------------------------------------
 */

void
scale_xyz(x, y, z, m)
    float x, y, z;			/* Scale amounts */
    Hk_dmatrix m;			/* The resulting matrix */
{
    identity(m);
    m[HKM_0_0] = x;
    m[HKM_1_1] = y;
    m[HKM_2_2] = z;
} /* End of scale_xyz */



/*
 *--------------------------------------------------------------
 *
 * translate
 *
 *	Create a translation matrix then pre-concatenate it to the
 *	transformation matrix.
 *
 *--------------------------------------------------------------
 */

void
translate(x, y, z, m)
    float x, y, z;			/* Translation amounts */
    Hk_dmatrix m;			/* The resulting matrix */
{
    identity(m);
    m[HKM_3_0] = x;
    m[HKM_3_1] = y;
    m[HKM_3_2] = z;
} /* End of trans */



/*
 *--------------------------------------------------------------
 *
 * rot_x
 *
 *	Create a rotation matrix to rotate clockwise about
 *	the X axis by "a" degrees.  (Left-handed coordinate system).
 *	Then pre-concatenate it to the transformation matrix.
 *
 *--------------------------------------------------------------
 */

void
rot_x(a, m)
    float a;				/* Angle in degrees */
    Hk_dmatrix m;			/* The resulting matrix */
{
    double c, s;			/* Sine and cosine */

    c = cos(RAD(a));
    s = sin(RAD(a));

    identity(m);
    m[HKM_1_1] = c;
    m[HKM_2_2] = c;
    m[HKM_1_2] = -s;
    m[HKM_2_1] = s;
} /* End of rot_x */



/*
 *--------------------------------------------------------------
 *
 * rot_y
 *
 *	Create a rotation matrix to rotate clockwise about
 *	the Y axis by "a" degrees.  (Left-handed coordinate system).
 *	Then pre-concatenate it to the transformation matrix.
 *
 *--------------------------------------------------------------
 */

void
rot_y(a, m)
    float a;				/* Angle in degrees */
    Hk_dmatrix m;			/* The resulting matrix */
{
    double c, s;			/* Sine and cosine */

    c = cos(RAD(a));
    s = sin(RAD(a));

    identity(m);
    m[HKM_0_0] = c;
    m[HKM_2_2] = c;
    m[HKM_0_2] = s;
    m[HKM_2_0] = -s;
} /* End of rot_y */



/*
 *--------------------------------------------------------------
 *
 * rot_z
 *
 *	Create a rotation matrix to rotate clockwise about
 *	the Z axis by "a" degrees.  (Left-handed coordinate system).
 *	Then pre-concatenate it to the transformation matrix.
 *
 *--------------------------------------------------------------
 */

void
rot_z(a, m)
    float a;				/* Angle in degrees */
    Hk_dmatrix m;			/* The resulting matrix */
{
    double c, s;			/* Sine and cosine */

    c = cos(RAD(a));
    s = sin(RAD(a));

    identity(m);
    m[HKM_0_0] = c;
    m[HKM_1_1] = c;
    m[HKM_0_1] = -s;
    m[HKM_1_0] = s;
} /* End of rot_z */



/*
 *--------------------------------------------------------------
 *
 * invert
 *
 *	This subroutine inverts a four by four matrix (m1) and
 *	returns the inverse (m2).
 *
 * RETURN VALUE:
 *	0 = no error
 *	1 = Input matrix is singular or Ill-conditioned
 *
 *--------------------------------------------------------------
 */

int
invert(m1, m2)
    Hk_dmatrix m1;		/* 4x4 matrix to be inverted */
    Hk_dmatrix m2;		/* Resultant matrix */
{
    Hk_dmatrix lu;		/* Copy of the original to work on */
    Hk_dmatrix inv;		/* Working copy of output matrix */
    int i, j, k;		/* Temporary loop counters */
    int itemp;			/* Temporary */
    int maxrow;			/* Row number of maximum value */
    double colmax;		/* column max value for current row */
    double pivtmx;		/* DAMNFINO */
    double divisr;		/* Ditto */
    double scale;		/* Current/pivot */

/* Make a copy of input matrix and initialize the output matrix */
    copy_mat(m1, lu);
    identity(inv);

/* Initialize the permutation vector (used implicitly by the INDX macro) */
    for (k = 0; k < N; k++)
	iperm[k] = k;

/*
 * Next we begin the Gaussian Elimination.  For each column we
 * will use scaled-partial pivoting to find the maximum value
 * with respect to the rest of the members in the row.
 */
    for (k = 0; k < N - 1; k++) {
	pivtmx = 0.0;
	maxrow = -1;
	for (i = k; i < N; i++) {
	    if (fabs(lu[INDX(i, k)]) >= TOLERANCE) {
		colmax = fabs(lu[INDX(i, k)]);
		for (j = k + 1; j < N; j++)
		    colmax = MAX(colmax, fabs(lu[INDX(i, j)]));
/*
 * Now we have the largest entry for row I --- we need to
 * Compute A(i,k) / colmax and find the largest of these for
 * every value of I >= K
 */
		divisr = fabs(lu[INDX(i, k)] / colmax);
		if (divisr > pivtmx) {
		    /* we found a better pivot...record that fact */
		    pivtmx = divisr;
		    maxrow = i;
		}
	    } /* End if lu[i,k] != 0 */
	} /* End for i */
/*
 * if MAXROW is non-negative then it is the row number of the
 * best choice for the pivot and should be interchanged with
 * row K.  If MAXROW is negative we have an error.
 */
	if (maxrow < 0)
	    return (1);

	itemp = iperm[k];
	iperm[k] = iperm[maxrow];
	iperm[maxrow] = itemp;

/* We got a good pivot...let's run with it! */

	for (i = k + 1; i < N; i++) {
	    scale = lu[INDX(i, k)] / lu[INDX(k, k)];
	    /* lu[INDX(i, k)] = scale; */
	    for (j = k + 1; j < N; j++)
		lu[INDX(i, j)] -= scale * lu[INDX(k, j)];

	    for (j = 0; j < N; j++)
		inv[INDX(i, j)] -= scale * inv[INDX(k, j)];
	}
    } /* End for k (outer row loop) */

/*
 * We're done with the triangulation.  "A" contains the upper triangular
 * matrix U and the lower triangular matrix L(?).  iperm contains the row
 * permutations necessary for solving a linear system.
 *
 * Check the final row to ensure that we don't have a singularity.
 */
    if (fabs(lu[INDX(N-1, N-1)]) < TOLERANCE)
	return (1);

/*
 * Now we need to factor out the upper portion of the matrix
 */
    for (j = 0; j < N; j++) {			/* For each column in the inv */
	for (k = N - 1; k >= 0; k--) {		/* For each diagonal in LU */
	    for (i = N - 1; i > k; i--)		/* For each entry right of K */
		inv[INDX(k,j)] -= inv[INDX(i,j)] * lu[INDX(k, i)];

	    inv[INDX(k,j)] /= lu[INDX(k, k)];
	}
    }
/*
 * The next section of code copies the working version of
 * the inverse matrix to the output matrix, physically
 * switching the rows.
 */
    for (i = 0; i < N; i++)
	for (j = 0; j < N; j++)
	    *m2++ = inv[INDX(i,j)];

    return(0);
} /* End of invert */

/* End of demo_mat_util.c */
