/*	@(#)pr_planegroups.h 1.1 94/10/31 SMI      */

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#ifndef	pr_planegroups_DEFINED
#define	pr_planegroups_DEFINED

/* Masks for frame buffer planes and plane group number */
#define	PIX_ALL_PLANES		0x00FFFFFF
#define	PIX_GROUP_MASK		0x7F

/* Macros to encode or extract group into or from attribute word */
#define	PIX_GROUP(g)		((g) << 25)
#define	PIX_ATTRGROUP(attr)	((unsigned)(attr) >> 25)

/* Flag bit which inhibits plane mask setting (for setting group only) */
#define	PIX_DONT_SET_PLANES	(1 << 24)
 
/*  Plane groups definitions */
/* !!! NOTE: Be sure to increment LAST_PLUS_ONE as new groups are added !!! */

#define	PIXPG_CURRENT		0
#define PIXPG_MONO		1
#define PIXPG_8BIT_COLOR	2
#define PIXPG_OVERLAY_ENABLE	3
#define PIXPG_OVERLAY	 	4
#define PIXPG_24BIT_COLOR	5
#define PIXPG_VIDEO		6
#define PIXPG_VIDEO_ENABLE	7
#define PIXPG_TRANSPARENT_OVERLAY	8
#define PIXPG_WID		9	/* Window Id */
#define PIXPG_ZBUF		10	/* Z (depth) buffer */
#define PIXPG_CURSOR_ENABLE	11
#define PIXPG_CURSOR	 	12
#define	PIXPG_LAST_PLUS_ONE	13	/* array decls and loop termination */
#define PIXPG_4BIT_OVERLAY	123
#define PIXPG_ALT_COLOR		124
#define PIXPG_A12BIT_COLOR	125
#define PIXPG_B12BIT_COLOR	126
#define	PIXPG_INVALID		127

#define MAKEPLNGRP(pg) (1 << (pg))

/* Plane groups functions */
extern int pr_available_plane_groups();
extern int pr_get_plane_group();
extern void pr_set_plane_group();
extern void pr_set_planes();

#endif	pr_planegroups_DEFINED
