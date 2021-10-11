/*	@(#)loop.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef __loop__
#define __loop__

#include "bit_util.h"

typedef enum {
	RETREATING,
	RETREATING_BACK,
	ADVANCING,
	ADVANCING_TREE,
	CROSS,
	RETREATORCROSS,
} EDGETYPE;

#	define INLOOP(blockno,loop) test_bit((loop)->in_blocks,(blockno))
#	define LOOP_WEIGHT_FACTOR 8

typedef struct edge {
	EDGETYPE edgetype;
	BLOCK *from, *to;
} EDGE;

typedef struct loop {
	LIST *back_edges;
	int 	loopno;
	BLOCK	*preheader;
	LIST	*blocks;
	LIST	*exit_blocks;
	LIST	*invariant_triples;
	SET	in_blocks;
} LOOP;

typedef struct loop_tree {
	struct loop_tree *parent;
	char *children;
	LOOP *loop;
} LOOP_TREE;

extern LIST	*loops;
extern int	nloops;

extern void	find_dominators();
extern void	find_loops();
extern BOOLEAN	dominates(/*bn1,bn2*/);
extern BOOLEAN	is_exit_dominator(/*bp, loop*/);
extern void	loop_cleanup();
extern void	dump_loops();
extern void	print_loop(/*loop*/);
extern void	dump_edges();
#endif
