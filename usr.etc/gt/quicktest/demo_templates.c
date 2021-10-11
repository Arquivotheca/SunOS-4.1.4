#ifndef lint
static char sccsid[] = "@(#)demo_templates.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_templates.c
 *
 *	@(#)demo_templates.c 1.10 91/01/29 13:48:25
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This file contains all of the initialized structure examples,
 *	and display list code used in creating each instance of an
 *	object for the Hawk demo program.
 *
 * 21-Sep-90 Scott R. Nelson	  Split out from demo_globals.c and
 *				  demo_dl_create.c
 * 27-Sep-90 Scott R. Nelson	  Added more attributes.
 *  4-Oct-90 Scott R. Nelson	  Fixed lights to have unit direction vector.
 * 11-Oct-90 Scott R. Nelson	  Changes to window bounds instruction.
 * 14-Oct-90 Kevin C. Rushforth	  Make line attributes apply to text.
 * 12-Nov-90 Scott R. Nelson	  Switch to fast clear for default.
 * 15-Jan-91 Scott R. Nelson	  Changes to make HIS 1.5 work.
 * 29-Jan-91 Kevin C. Rushforth	  Fixed matrices for HIS 1.5
 *
 ***********************************************************************
 */

#include "demo.h"



/*
 *--------------------------------------------------------------
 *
 * Template of the attribute data area, referenced by the display list.
 * This must match the structure dl_data exactly.  It is especially
 * important for sub-structures to put in enough elements rather than
 * default to zero.
 *
 *--------------------------------------------------------------
 */

dl_data dl_data_template = {
	/* Traversal control flags */
	0x0000,
	/* Fast clear set */
	0,
	/* Z-buffer update */
	HK_Z_UPDATE_ALL,
	/* Guard band */
	1.1,
	/* Fast clear background color (for updates) */
	{0, 0x00000000},

	/* Plane group */
	HK_24BIT,
	/* Plane mask */
	0x00ffffff,
	/* Current WID */
	DEF_WID,
	/* WID clip mask */
	0,
	/* WID write mask */
	0x3ff,

	/* Object color */
	{0.2, 1.0, 0.1},
	/* Background color */
	{0.0, 0.0, 0.0},
	/* Edge color */
	{0.0, 0.0, 0.2},
	/* Line off color */
	{1.0, 0.0, 0.0},

	/* Flag to enable antialiasing */
	FALSE,
	/* Line shading method */
	HK_LINE_SHADING_INTERPOLATE,
	/* Line style */
	HK_LINESTYLE_SOLID,
	/* Line pattern */
	NULL,
	/* Line width */
	1.0,
	/* Line cap */
	HK_CAP_BUTT,
	/* Line join */
	HK_JOIN_BUTT,
	/* Line miter limit */
	0.0,

	/* Edge control */
	HK_EDGE_NONE,
	/* Edge Z offset */
	0.005,

	/* Marker pointer */
	NULL,
	/* Marker size */
	20.0,

	/* Use back surface properties */
	FALSE,
	/* Shading method */
	HK_SHADING_GOURAUD,
	/* Lighting degree */ 
	HK_SPECULAR_LIGHTING,
	/* Material properties */
	{0.2, 0.8, 1.0, 20.0},
	/* Specular color */
	{1.0, 1.0, 1.0},
	/* Transparency degree */
	1.0,
	/* Front/back interior style */
	HK_POLYGON_SOLID,
	/* Front/back hatch style */
	{TRUE, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000},
	/* Culling mode */
	HK_CULL_NONE,
	/* Transparency quality */
	HK_SCREEN_DOOR,

	/* Enable depth-cuing */
	FALSE,
	/* Depth-cue parameters */
	{1.0, -1.0, 1.0, 0.1},

	/* Light mask */
	{0xffffffff, 0x00000007},
	/* Light 0 (ambient) */
	{0,				/* Light number */
	    {HK_AMBIENT_LIGHT_SOURCE,	/* Light type */
	    1.0, 1.0, 1.0,		/* Light color */
	    0.0, 0.0, 0.0,		/* Position */
	    0.0, 0.0, 0.0,		/* Direction */
	    0.0,			/* Spot light power */
	    0.0, 0.0,			/* Attenuation (spot, point) */
	    0.0}},			/* Spot angle */
	/* Light 1 (directional) */
	{1, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    1.0, 0.8, 0.8,		/* Light color */
	    0.0, 0.0, 0.0,		/* Position */
	    0.398, 0.199, 0.896,	/* Direction */
	    0.0, 0.0, 0.0, 0.0}},
	/* Light 2 (directional) */
	{2, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    0.8, 0.8, 1.0,
	    0.0, 0.0, 0.0,
	    -0.623, 0.778, -0.078,
	    0.0, 0.0, 0.0, 0.0}},
/* Normalize the directions on these! */
	/* Light 3 (directional) */
	{3, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    0.6, 0.5, 0.6,
	    0.0, 0.0, 0.0,
	    -1.00, 1.00, -0.25,
	    0.0, 0.0, 0.0, 0.0}},
	/* Light 4 (directional) */
	{4, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    0.45, 0.4, 0.4,
	    0.0, 0.0, 0.0,
	    -10.00, 1.00, 1.00,
	    0.0, 0.0, 0.0, 0.0}},
	/* Light 5 (directional) */
	{5, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    0.45, 0.45, 0.4,
	    0.0, 0.0, 0.0,
	    -1.40, 1.00, 1.00,
	    0.0, 0.0, 0.0, 0.0}},
	/* Light 6 (directional) */
	{6, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    0.4, 0.45, 0.4,
	    0.0, 0.0, 0.0,
	    0.00, 1.00, 1.00,
	    0.0, 0.0, 0.0, 0.0}},
	/* Light 7 (directional) */
	{7, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    0.4, 0.4, 0.45,
	    0.0, 0.0, 0.0,
	    1.40, 1.00, 1.00,
	    0.0, 0.0, 0.0, 0.0}},
	/* Light 8 (directional) */
	{8, {HK_DIRECTIONAL_LIGHT_SOURCE,
	    0.45, 0.4, 0.45,
	    0.0, 0.0, 0.0,
	    10.00, 1.00, 1.00,
	    0.0, 0.0, 0.0, 0.0}},

	/* Draw to buffer A */
	HK_BUFFER_A,
	/* Draw to buffer B */
	HK_BUFFER_B,
	/* Display from buffer A */
	{DEF_WID, 0x1f, HK_BUFFER_A, HK_BUFFER_A, HK_24BIT, DEF_CLUT, 0},
	/* Display from buffer B */
	{DEF_WID, 0x1f, HK_BUFFER_B, HK_BUFFER_B, HK_24BIT, DEF_CLUT, 0},

	/* Current view (includes perspective) */ 
	{0.0, 0.0, 1.0, 1.0,
	{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 0.2, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	/* Note: When point and spot light sources are implemented, */
	/*       we'll have to compute the inverse matrix here. */
	{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 5.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 0.0, 1.0},
	/* Current window position */
	{0, 0, 50, 40},
	/* Global modeling transform matrix */
	{{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 1.0},
	/* Current translation matrix */
	{{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 1.0},
	/* Current rotation matrix */
	{{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 1.0},
	/* Current scale matrix */
	{{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 1.0},
	/* Rotation axis matrix */
	{{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 1.0},
	/* Current spin matrix */
	{{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 1.0},
	/* Pointer to window boundary information */
	NULL,

	0,				/* Filler, z-buffer for stereo */
	/* Left stereo view */ 
	{0.0, 0.0, 1.0, 1.0,
	{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 0.2, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	/* Note: When point and spot light sources are implemented, */
	/*       we'll have to compute the inverse matrix here. */
	{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 5.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 0.0, 1.0},
	/* Right stereo view */ 
	{0.0, 0.0, 1.0, 1.0,
	{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 0.2, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	/* Note: When point and spot light sources are implemented, */
	/*       we'll have to compute the inverse matrix here. */
	{1.0, 0.0, 0.0, 0.0,
	 0.0, 1.0, 0.0, 0.0,
	 0.0, 0.0, 5.0, 0.0,
	 0.0, 0.0, 0.0, 1.0},
	 0.0, 1.0},
	/* Left stereo color */
	{0.0, 0.0, 1.0},
	/* Right stereo color */
	{1.0, 0.0, 0.0},
};



/*
 *--------------------------------------------------------------
 *
 * Templates for the dial_data structure.  Each template has the correct
 * default settings for one or more of the control dials.  It may be
 * necessary to initialize the "current values" in some cases.  For
 * templates used multiple times, it may also be necessary to set the
 * constraint checking routine to something else.
 *
 *--------------------------------------------------------------
 */

/* The following definition is only valid in this module: */
/* One rotation of the dial corresponds to x */
#define ONE_ROT(x) ((x) / (float) DIAL_ONE_ROTATION)

/* For rotations */
dial_data rot_template = {
    0.0,				/* Current dial value */
    0,					/* Current absolute value */
    ONE_ROT(360.0),			/* Multiplication factor */
    0,					/* Minimum absolute */
    DIAL_ONE_ROTATION - 1,		/* Maximum absolute */
    WRAP | RELATIVE,			/* clamp, relative flag */
    NULL,				/* Constraint checking routine */
    update_x_rot,			/* Attribute update routine */
    DIAL_ONE_ROTATION / 90,
};

/* For rotation speed */
dial_data rot_speed_template = {
    0.0,
    0,
    ONE_ROT(10.0),			/* 10 degrees per frame each rotation */
    -DIAL_ONE_ROTATION * 9,
    DIAL_ONE_ROTATION * 9,
    CLAMP | ABSOLUTE,
    NULL,
    update_rot_speed,
    DIAL_ONE_ROTATION / 90,
};

/* For translations */
dial_data trans_template = {
    0.0,
    0,
    ONE_ROT(1.0),
    -1000000000,			/* Effectively no limits */
    1000000000,
    CLAMP | ABSOLUTE,
    NULL,
    update_trans,
    DIAL_ONE_ROTATION / 90,
};

/* For scale operations */
dial_data scale_template = {
    1.0,
    DIAL_ONE_ROTATION * 2,
    ONE_ROT(0.5),
    0,
    1000000000,				/* No real upper limit */
    CLAMP | ABSOLUTE,
    NULL,
    update_scale,
    DIAL_ONE_ROTATION / 64,
};

/* For perspective */
dial_data persp_template = {
    0.0,
    0,
    ONE_ROT(0.2),
    0,
    DIAL_ONE_ROTATION * 5,
    CLAMP | ABSOLUTE,
    NULL,
    update_persp,
    DIAL_ONE_ROTATION / 64,
};

/* For color, depth-cue intensity */
dial_data color_template = {
    0.0,
    0,					/* This one must be initialized */
    ONE_ROT(1.0),
    0,
    DIAL_ONE_ROTATION,
    CLAMP | ABSOLUTE,
    NULL,
    update_obj_color,
    DIAL_ONE_ROTATION / 256,
};

/* For specular exponent */
dial_data exp_template = {
    20.0,
    0,
    ONE_ROT(10.0),
    0,
    DIAL_ONE_ROTATION * 26,
    CLAMP | ABSOLUTE,
    NULL,
    update_surf_prop,
    DIAL_ONE_ROTATION / 20,
};

/* For depth-cue range */
dial_data dc_range_template = {
    0.0,
    0,
    ONE_ROT(1.0),
    -DIAL_ONE_ROTATION,
    DIAL_ONE_ROTATION,
    CLAMP | ABSOLUTE,
    NULL,
    update_obj_color,
    DIAL_ONE_ROTATION / 256,
};

/* For surface property coefficients */
dial_data coeff_template = {
    0.0,
    0,
    ONE_ROT(1.0),
    0,
    DIAL_ONE_ROTATION,
    CLAMP | ABSOLUTE,
    NULL,
    update_surf_prop,
    DIAL_ONE_ROTATION / 90,
};

/* For window X movement */
dial_data window_x_template = {
    0.0,
    0,
    ONE_ROT(256.0),
    0,
    DIAL_ONE_ROTATION * 5,
    CLAMP | ABSOLUTE,
    constrain_window_x,
    update_window,
    DIAL_ONE_ROTATION / 256,
};

/* For window Y movement */
dial_data window_y_template = {
    0.0,
    0,
    ONE_ROT(256.0),
    0,
    DIAL_ONE_ROTATION * 4,
    CLAMP | ABSOLUTE,
    constrain_window_y,
    update_window,
    DIAL_ONE_ROTATION / 256,
};

/* For window width adjustments */
dial_data window_width_template = {
    256.0,
    DIAL_ONE_ROTATION,
    ONE_ROT(256.0),
    10 * DIAL_ONE_ROTATION / 256,		/* 10 pixels */
    DIAL_ONE_ROTATION * 5,
    CLAMP | ABSOLUTE,
    constrain_window_width,
    update_window,
    DIAL_ONE_ROTATION / 256,
};

/* For window height adjustments */
dial_data window_height_template = {
    256.0,
    DIAL_ONE_ROTATION,
    ONE_ROT(256.0),
    10 * DIAL_ONE_ROTATION / 256,		/* 10 pixels */
    DIAL_ONE_ROTATION * 4,
    CLAMP | ABSOLUTE,
    constrain_window_height,
    update_window,
    DIAL_ONE_ROTATION / 256,
};



/*
 *--------------------------------------------------------------
 *
 * The main display list loop for all "windows"
 *
 * Note: The sections of code with assembly language listings above them
 * have been run through hasm, then dasm to get branch offsets correct.
 * CHANGE THEM WITH CAUTION!
 *
 *--------------------------------------------------------------
 */

/*
 * Macro to specify an individual attribute indexed by R4 into the
 * display list data record.
 */

#define ATTR_I_R4_OFFSET(attr, data) \
	((HK_OP_SET_ATTR_I << HK_OP_POS) | ((attr) << HK_ATTR_POS) \
	| (HK_AM_R4_INDIRECT << HK_AM_POS) \
	| (data))
/*	| (((int)offsetof(dl_data, data)) >> 2))*/

unsigned main_loop[MAIN_LOOP_SIZE] = {

/* Check for special case of fast clear background color changes */
    /*-
     *    ld #TRF_SET_FCBG, r0        ; Flag bit for fcbg updates
     *    move r0, r2                 ; Make an extra working copy
     *    ld (r4 + TR_FLAGS), r1      ; Flag word
     *    and r1, r0                  ; See if the bit is set
     *    bleg r0, update_fcbg, dont_update_fcbg, update_fcbg
     *update_fcbg:
     *    xor r2, r1                  ; Turn off the bit
     *    st r1, (r4 + TR_FLAGS)      ; Save the updated bit
     *    update_lut hk_update_fcbg, (r4 + FCBG_COLOR)
     *dont_update_fcbg:
     */
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)), (TRF_SET_FCBG),
    ((HK_OP_MOVE << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)
	| (HK_RISC_R2 << HK_REG_POS)),
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    ((HK_OP_AND << HK_OP_POS) | (HK_RISC_R1 << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)),
    ((HK_OP_BLEG << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)),
	(0x00000000), (0x0000000c), (0x00000000),
    ((HK_OP_XOR << HK_OP_POS) | (HK_RISC_R2 << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS)),
    ((HK_OP_ST << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    ((HK_OP_UPDATE_LUT << HK_OP_POS) | (HK_UPDATE_FCBG << HK_SO_POS)
	| (HK_AM_R4_INDIRECT << HK_AM_POS) | FCBG_COLOR),

/* Window attributes */
    ATTR_I_R4_OFFSET(HK_WINDOW_BOUNDARY, WB_PTR),

    ((HK_OP_SET_ATTR_I << HK_OP_POS) | ((HK_STEREO_MODE) << HK_ATTR_POS)
	| (HK_AM_IMMEDIATE << HK_AM_POS)),
    (HK_STEREO_NONE),

    ATTR_I_R4_OFFSET(HK_WINDOW_BG_COLOR, BG_COLOR),
    ATTR_I_R4_OFFSET(HK_DEPTH_CUE_COLOR, BG_COLOR),	/* No choice here */

    ATTR_I_R4_OFFSET(HK_FAST_CLEAR_SET, FCS),
    ATTR_I_R4_OFFSET(HK_PLANE_GROUP, PLANE_GROUP),
    ATTR_I_R4_OFFSET(HK_PLANE_MASK, PLANE_MASK),
    ATTR_I_R4_OFFSET(HK_CURRENT_WID, CURRENT_WID),
    ATTR_I_R4_OFFSET(HK_WID_CLIP_MASK, WID_CLIP_MASK),
    ATTR_I_R4_OFFSET(HK_WID_WRITE_MASK, WID_WRITE_MASK),

/* View */
    ATTR_I_R4_OFFSET(HK_LOAD_GMT, GLOBAL_MATRIX),
    ATTR_I_R4_OFFSET(HK_LOAD_LMT, ROT_AXIS_MATRIX),
    ATTR_I_R4_OFFSET(HK_PRE_CONCAT_LMT, SPIN_MATRIX),
    ATTR_I_R4_OFFSET(HK_PRE_CONCAT_LMT, TRANS_MATRIX),
    ATTR_I_R4_OFFSET(HK_PRE_CONCAT_LMT, ROT_MATRIX),
    ATTR_I_R4_OFFSET(HK_PRE_CONCAT_LMT, SCALE_MATRIX),

/* Lights */
    ATTR_I_R4_OFFSET(HK_LIGHT_MASK, LIGHT_MASK),
    ATTR_I_R4_OFFSET(HK_LIGHT_SOURCE, LIGHT_0),
    ATTR_I_R4_OFFSET(HK_LIGHT_SOURCE, LIGHT_1),
    ATTR_I_R4_OFFSET(HK_LIGHT_SOURCE, LIGHT_2),

/* Object attributes */
    ATTR_I_R4_OFFSET(HK_LINE_COLOR, OBJECT_COLOR),
    ATTR_I_R4_OFFSET(HK_LINE_ANTIALIASING, ANTIALIAS),
    ATTR_I_R4_OFFSET(HK_LINE_SHADING_METHOD, LINE_SHAD_METH),
 
    ATTR_I_R4_OFFSET(HK_MARKER_COLOR, OBJECT_COLOR),
    ATTR_I_R4_OFFSET(HK_MARKER_ANTIALIASING, ANTIALIAS),

    ATTR_I_R4_OFFSET(HK_TEXT_COLOR, OBJECT_COLOR),
    ATTR_I_R4_OFFSET(HK_TEXT_ANTIALIASING, ANTIALIAS),

    ATTR_I_R4_OFFSET(HK_FRONT_SURFACE_COLOR, OBJECT_COLOR),
    ATTR_I_R4_OFFSET(HK_FRONT_SHADING_METHOD, SHADING_METHOD),
    ATTR_I_R4_OFFSET(HK_FRONT_LIGHTING_DEGREE, LIGHTING_DEGREE),
    ATTR_I_R4_OFFSET(HK_FRONT_MATERIAL_PROPERTIES, MAT_PROP),
    ATTR_I_R4_OFFSET(HK_FRONT_SPECULAR_COLOR, SPEC_COLOR),
    ATTR_I_R4_OFFSET(HK_FRONT_TRANSPARENCY_DEGREE, TRANSP_DEGREE),
    ATTR_I_R4_OFFSET(HK_BACK_SURFACE_COLOR, OBJECT_COLOR),
    ATTR_I_R4_OFFSET(HK_BACK_SHADING_METHOD, SHADING_METHOD),
    ATTR_I_R4_OFFSET(HK_BACK_LIGHTING_DEGREE, LIGHTING_DEGREE),
    ATTR_I_R4_OFFSET(HK_BACK_MATERIAL_PROPERTIES, MAT_PROP),
    ATTR_I_R4_OFFSET(HK_BACK_SPECULAR_COLOR, SPEC_COLOR),
    ATTR_I_R4_OFFSET(HK_BACK_TRANSPARENCY_DEGREE, TRANSP_DEGREE),
    ATTR_I_R4_OFFSET(HK_USE_BACK_PROPS, USE_BACK_PROPS),
    ATTR_I_R4_OFFSET(HK_FACE_CULLING_MODE, CULLING_MODE),
    ATTR_I_R4_OFFSET(HK_TRANSPARENCY_QUALITY, TRANS_QUAL),
    ATTR_I_R4_OFFSET(HK_EDGE, EDGE),
    ATTR_I_R4_OFFSET(HK_EDGE_COLOR, EDGE_COLOR),
    ATTR_I_R4_OFFSET(HK_EDGE_Z_OFFSET, EDGE_Z_OFFSET),

    ATTR_I_R4_OFFSET(HK_Z_BUFFER_UPDATE, Z_BUFFER_UPDATE),
    ATTR_I_R4_OFFSET(HK_DEPTH_CUE_ENABLE, DEPTH_CUE),
    ATTR_I_R4_OFFSET(HK_DEPTH_CUE_PARAMETERS, DC_PARAM),

/* Determine which buffer to draw to */
    /*-
     *    ld #TRF_BUFFER_B, r0        ; Flag bit for buffer B
     *    ld (r4 + TR_FLAGS), r1      ; Flag word
     *    and r1, r0                  ; r0: 0 == A, 1 == B 
     *    bleg r0, draw_to_b, draw_to_a, draw_to_b    ; set->B, clear->A
     *draw_to_a:
     *    set_attribute hk_draw_buffer, (r4 + DRAW_A)
     *    jmpr        go_draw_it
     *draw_to_b:
     *    set_attribute hk_draw_buffer, (r4 + DRAW_B)
     *go_draw_it:
     */
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)), (TRF_BUFFER_B),
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    ((HK_OP_AND << HK_OP_POS) | (HK_RISC_R1 << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)),
    ((HK_OP_BLEG << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)),
	(0x0000000c), (0x00000000), (0x0000000c),
    ATTR_I_R4_OFFSET(HK_DRAW_BUFFER, DRAW_A),
    ((HK_OP_JMPR << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)),
	(0x00000004),
    ATTR_I_R4_OFFSET(HK_DRAW_BUFFER, DRAW_B),

/* Check if we need to set the WID on every pixel */
    /*-
     *    ld #TRF_PREPARE_WINDOW, r0  ; Flag bit for fcbg updates
     *    move r0, r2                 ; Make an extra working copy
     *    ld (r4 + TR_FLAGS), r1      ; Flag word
     *    and r1, r0                  ; See if the bit is set
     *    bleg r0, clear_viewport, clear_window, clear_viewport
     *clear_viewport:
     *    xor r2, r1                  ; Turn off the bit
     *    st r1, (r4 + TR_FLAGS)      ; Save the updated bit
     *    viewport_clear              ; Clear viewport, write all pixels
     *    jmpr window_done
     *clear_window:
     *    window_clear                ; Clear window (use FCS)
     *window_done:
     */
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)), (TRF_PREPARE_WINDOW),
    ((HK_OP_MOVE << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)
	| (HK_RISC_R2 << HK_REG_POS)),
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    ((HK_OP_AND << HK_OP_POS) | (HK_RISC_R1 << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)),
    ((HK_OP_BLEG << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)),
	(0x00000000), (0x00000014), (0x00000000),
    ((HK_OP_XOR << HK_OP_POS) | (HK_RISC_R2 << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS)),
    ((HK_OP_ST << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    (HK_OP_VIEWPORT_CLEAR << HK_OP_POS),
    ((HK_OP_JMPR << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)),
	(0x00000004),
    (HK_OP_WINDOW_CLEAR << HK_OP_POS),

/* Check if we need to draw in stereo mode */
    /*-
     *    ld #TRF_STEREO, r0          ; Flag bit for fcbg updates
     *    ld (r4 + TR_FLAGS), r1      ; Flag word
     *    and r1, r0                  ; See if the bit is set
     *    bleg r0, stereo_mode, mono_mode, stereo_mode
     *stereo_mode:
     *    set_attribute hk_line_color, (r4 + LEFT_COLOR)
     *    set_attribute hk_marker_color, (r4 + LEFT_COLOR)
     *    set_attribute hk_text_color, (r4 + LEFT_COLOR)
     *    set_attribute hk_front_surface_color, (r4 + LEFT_COLOR)
     *    set_attribute hk_back_surface_color, (r4 + LEFT_COLOR)
     *    set_attribute hk_view, (r4 + LEFT_VIEW)
     *    jmpl (r5 + 0), r6           ; Draw the object
     *    set_attribute hk_line_color, (r4 + RIGHT_COLOR)
     *    set_attribute hk_marker_color, (r4 + RIGHT_COLOR)
     *    set_attribute hk_text_color, (r4 + RIGHT_COLOR)
     *    set_attribute hk_front_surface_color, (r4 + RIGHT_COLOR)
     *    set_attribute hk_back_surface_color, (r4 + RIGHT_COLOR)
     *    set_attribute hk_view, (r4 + RIGHT_VIEW)
     *    jmpl (r5 + 0), r6           ; Draw the object
     *    jmpr stereo_done
     *mono_mode:
     *    set_attribute hk_view, (r4 + VIEW)
     *    jmpl (r5 + 0), r6           ; Draw the object
     *stereo_done:
     */
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)), (TRF_STEREO),
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    ((HK_OP_AND << HK_OP_POS) | (HK_RISC_R1 << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)),
#ifdef PSEUDO_STEREO
    ((HK_OP_BLEG << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)),
	(0x00000000), (0x00000044), (0x00000000),
    ATTR_I_R4_OFFSET(HK_Z_BUFFER_UPDATE, Z_BUF_STEREO),
#else
    ((HK_OP_BLEG << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)),
	(0x00000000), (0x00000028), (0x00000000),
#endif

/* Draw the object for the left view */
#ifdef PSEUDO_STEREO
    ATTR_I_R4_OFFSET(HK_LINE_COLOR, LEFT_COLOR),
    ATTR_I_R4_OFFSET(HK_MARKER_COLOR, LEFT_COLOR),
    ATTR_I_R4_OFFSET(HK_TEXT_COLOR, LEFT_COLOR),
    ATTR_I_R4_OFFSET(HK_FRONT_SURFACE_COLOR, LEFT_COLOR),
    ATTR_I_R4_OFFSET(HK_BACK_SURFACE_COLOR, LEFT_COLOR),
#else
    ((HK_OP_SET_ATTR_I << HK_OP_POS) | ((HK_STEREO_MODE) << HK_ATTR_POS)
	| (HK_AM_IMMEDIATE << HK_AM_POS)),
    (HK_STEREO_LEFT),
#endif

    ATTR_I_R4_OFFSET(HK_VIEW, LEFT_VIEW),
    ((HK_OP_JMPL << HK_OP_POS) | (HK_AM_R5_INDIRECT << HK_AM_POS)
	| (HK_AM_R6_INDIRECT << HK_REG_POS)),

/* Draw the object for the right view */
#ifdef PSEUDO_STEREO
    ATTR_I_R4_OFFSET(HK_LINE_COLOR, RIGHT_COLOR),
    ATTR_I_R4_OFFSET(HK_MARKER_COLOR, RIGHT_COLOR),
    ATTR_I_R4_OFFSET(HK_TEXT_COLOR, RIGHT_COLOR),
    ATTR_I_R4_OFFSET(HK_FRONT_SURFACE_COLOR, RIGHT_COLOR),
    ATTR_I_R4_OFFSET(HK_BACK_SURFACE_COLOR, RIGHT_COLOR),
#else
    ((HK_OP_SET_ATTR_I << HK_OP_POS) | ((HK_STEREO_MODE) << HK_ATTR_POS)
	| (HK_AM_IMMEDIATE << HK_AM_POS)),
    (HK_STEREO_RIGHT),
#endif

    ATTR_I_R4_OFFSET(HK_VIEW, RIGHT_VIEW),
    ((HK_OP_JMPL << HK_OP_POS) | (HK_AM_R5_INDIRECT << HK_AM_POS)
	| (HK_AM_R6_INDIRECT << HK_REG_POS)),

    ((HK_OP_JMPR << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)),
	(0x00000008),

/* Draw the object for the regular view */
    ATTR_I_R4_OFFSET(HK_VIEW, VIEW),
    ((HK_OP_JMPL << HK_OP_POS) | (HK_AM_R5_INDIRECT << HK_AM_POS)
	| (HK_AM_R6_INDIRECT << HK_REG_POS)),

/* Determine which buffer to display */
    /*-
     *    ld  #TRF_BUFFER_B, r0       ; Check bit for buffer A/B
     *    move r0, r2                 ; Make an extra working copy
     *    ld (r4 + TR_FLAGS), r1      ; Flag word
     *    and r1, r0                  ; See if the bit is set
     *    xor r2, r1                  ; Complement the bit for next time
     *    st r1, (r4 + TR_FLAGS)      ; Save the updated bit
     *    bleg r0, display_b, display_a, display_b    ; set->B, clear->A
     *display_a:
     *    update_lut hk_update_wlut, (r4 + DISPLAY_A)
     *    jmpr        display_it
     *display_b:
     *    update_lut hk_update_wlut, (r4 + DISPLAY_B)
     *display_it:
     */
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)), (TRF_BUFFER_B),
    ((HK_OP_MOVE << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)
	| (HK_RISC_R2 << HK_REG_POS)),
    ((HK_OP_LD << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    ((HK_OP_AND << HK_OP_POS) | (HK_RISC_R1 << HK_AM_POS)
	| (HK_RISC_R0 << HK_REG_POS)),
    ((HK_OP_XOR << HK_OP_POS) | (HK_RISC_R2 << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS)),
    ((HK_OP_ST << HK_OP_POS) | (HK_AM_R4_INDIRECT << HK_AM_POS)
	| (HK_RISC_R1 << HK_REG_POS) | TR_FLAGS),
    ((HK_OP_BLEG << HK_OP_POS) | (HK_RISC_R0 << HK_AM_POS)),
	(0x0000000c), (0x00000000), (0x0000000c),
    ((HK_OP_UPDATE_LUT << HK_OP_POS) | (HK_UPDATE_WLUT << HK_SO_POS)
	| (HK_AM_R4_INDIRECT << HK_AM_POS) | DISPLAY_A),
    ((HK_OP_JMPR << HK_OP_POS) | (HK_AM_IMMEDIATE << HK_AM_POS)),
	(0x00000004),
    ((HK_OP_UPDATE_LUT << HK_OP_POS) | (HK_UPDATE_WLUT << HK_SO_POS)
	| (HK_AM_R4_INDIRECT << HK_AM_POS) | DISPLAY_B),

/* Clean up afterwards and wait for further instruction */
    (HK_OP_FLUSH_RENDERING_PIPE << HK_OP_POS),
    (HK_OP_FLUSH_CONTEXT << HK_OP_POS),
    (HK_OP_WAIT << HK_OP_POS),
};

/* End of demo_templates.c */
