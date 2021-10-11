/*
 * "@(#)netrange.h 1.1 94/10/31 Copyright 1987 Sun Microsystems Inc.
 */
#define	MAX_RANGE	40

typedef struct {
    long	h_start, h_end;
} hostrange;

hostrange *get_hostrange ();
