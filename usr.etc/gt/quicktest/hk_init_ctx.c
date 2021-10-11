#ifndef lint
static char sccsid[] = "@(#)hk_init_ctx.c  1.1 94/10/31 SMI";
#endif

/*
 ****************************************************************
 *
 * hk_init_ctx.c
 *
 *	@(#)hk_init_ctx.c 13.1 91/07/22 18:29:57
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Library routine to initialize a hawk context.
 *
 * 20-Feb-90 Kevin C. Rushforth	  Created common init routine.
 * 22-Mar-90 Kevin C. Rushforth	  Added hk_ to routine name.
 *  9-Apr-90 Kevin C. Rushforth	  Initialize CTX magic and version numbers.
 * 12-Apr-90 Kevin C. Rushforth	  Removed "*_stopped" flags from context.
 *  3-Oct-90 Ranjit Oberoi	  Removed parent_needs and added
 *				  need_from_children initialization.
 * 10-Oct-90 Kevin C. Rushforth	  Changes for new window_boundary semantics.
 * 25-Oct-90 Kevin C. Rushforth	  Updated blend program timing values.
 *  9-Jan-91 Kevin C. Rushforth	  Updates for HIS 1.5
 * 24-Mar-91 Kevin C. Rushforth	  Zero out reserved state and ctx fields.
 * 26-Mar-91 Kevin C. Rushforth	  Added hk_clipping_limits
 *
 ****************************************************************
 */

#include "hk_public.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	hk_init_default_context
 *		identity
 *
 *	identity
 *
 *==============================================================
 */



/* Local procedures */
static void identity();



/* Global constants */
#define DEF_SCREEN_SIZE 256		/* Default screen size (X and Y) */



/*
 *--------------------------------------------------------------
 *
 * hk_init_default_context
 *
 *	This routine is called ONCE at the beginning of the main
 *	program to initialize the default context to some "reasonable"
 *	values.
 *
 *--------------------------------------------------------------
 */

void
hk_init_default_context(c)
    Hk_context *c;			/* Pointer to context */
{
    Hk_state *s;			/* Pointer to state values */
    int i;				/* Loop variable */

/* Zero out the entire context */
    bzero((char *) c, sizeof(Hk_context));

/* Initialize the magic number and version number */
    c->magic = HK_CTX_MAGIC;
    c->version = HK_CTX_VERSION;

/*
 *********************************
 * Initialize the state variables
 *********************************
 */

    s = &c->s;				/* Point to state values */

/* State saving information */
    for (i = 0; i < HKST_NUM_WORDS; i++) {
	s->attributes_saved[i] = 0;
	s->need_from_children[i] = ~0;
    }

/* Lines */

    s->line_geom_format = HK_LINE_XYZ;

    s->hki_line_antialiasing = HK_AA_NONE;
    s->hkb_line_antialiasing = HK_AA_NONE;
    s->hka_line_antialiasing = FALSE;

    s->hki_line_color.r = 0.5;
    s->hki_line_color.g = 0.1;
    s->hki_line_color.b = 1.0;
    s->hkb_line_color.r = 0.5;
    s->hkb_line_color.g = 0.1;
    s->hkb_line_color.b = 1.0;
    s->hka_line_color = FALSE;

    s->hki_line_shading_method = HK_LINE_SHADING_INTERPOLATE;
    s->hkb_line_shading_method = HK_LINE_SHADING_INTERPOLATE;
    s->hka_line_shading_method = FALSE;

    s->hki_line_style = HK_LINESTYLE_SOLID;
    s->hkb_line_style = HK_LINESTYLE_SOLID;
    s->hka_line_style = FALSE;

    s->line_off_color.r = 1.0;
    s->line_off_color.g = 0.1;
    s->line_off_color.b = 0.1;

    s->hki_line_pattern = NULL;
    s->hkb_line_pattern = NULL;
    s->hka_line_pattern = FALSE;

    s->hki_line_width = 1.0;
    s->hkb_line_width = 1.0;
    s->hka_line_width = FALSE;

    s->hki_line_cap = HK_CAP_BUTT;
    s->hkb_line_cap = HK_CAP_BUTT;
    s->hka_line_cap = FALSE;

    s->hki_line_join = HK_JOIN_BUTT;
    s->hkb_line_join = HK_JOIN_BUTT;
    s->hka_line_join = FALSE;

    s->hki_line_miter_limit = 0.0;
    s->hkb_line_miter_limit = 0.0;
    s->hka_line_miter_limit = FALSE;

/* Curve approximation */

    s->hki_curve_approx.approx_value = 1.0;
    s->hki_curve_approx.approx_type = HK_CONST_PARAM_SUBDIVISION;
    s->hkb_curve_approx.approx_value = 1.0;
    s->hkb_curve_approx.approx_type = HK_CONST_PARAM_SUBDIVISION;
    s->hka_curve_approx = FALSE;

/* Surface attributes */

    s->tri_geom_format = HK_TRI_XYZ_VNORMAL;

    s->face_culling_mode = HK_CULL_NONE;

    s->use_back_props = FALSE;

    /* Front and back surface color */

    s->hki_front_surface_color.r = 1.00;
    s->hki_front_surface_color.g = 0.10;
    s->hki_front_surface_color.b = 0.05;
    s->hkb_front_surface_color.r = 1.00;
    s->hkb_front_surface_color.g = 0.10;
    s->hkb_front_surface_color.b = 0.05;
    s->hka_front_surface_color = FALSE;

    s->hki_back_surface_color.r = 0.10;
    s->hki_back_surface_color.g = 1.00;
    s->hki_back_surface_color.b = 0.05;
    s->hkb_back_surface_color.r = 0.10;
    s->hkb_back_surface_color.g = 1.00;
    s->hkb_back_surface_color.b = 0.05;
    s->hka_back_surface_color = FALSE;

    /* Front and back material properties */

    s->hki_front_material_properties.ambient_reflection_coefficient = 0.3;
    s->hki_front_material_properties.diffuse_reflection_coefficient = 0.7;
    s->hki_front_material_properties.specular_reflection_coefficient = 1.0;
    s->hki_front_material_properties.specular_exponent = 20.0;
    s->hkb_front_material_properties.ambient_reflection_coefficient = 0.3;
    s->hkb_front_material_properties.diffuse_reflection_coefficient = 0.7;
    s->hkb_front_material_properties.specular_reflection_coefficient = 1.0;
    s->hkb_front_material_properties.specular_exponent = 20.0;
    s->hka_front_material_properties = FALSE;

    s->hki_back_material_properties.ambient_reflection_coefficient = 0.3;
    s->hki_back_material_properties.diffuse_reflection_coefficient = 0.7;
    s->hki_back_material_properties.specular_reflection_coefficient = 1.0;
    s->hki_back_material_properties.specular_exponent = 20.0;
    s->hkb_back_material_properties.ambient_reflection_coefficient = 0.3;
    s->hkb_back_material_properties.diffuse_reflection_coefficient = 0.7;
    s->hkb_back_material_properties.specular_reflection_coefficient = 1.0;
    s->hkb_back_material_properties.specular_exponent = 20.0;
    s->hka_back_material_properties = FALSE;

    /* Front and back specular color */

    s->hki_front_specular_color.r = 1.0;
    s->hki_front_specular_color.g = 1.0;
    s->hki_front_specular_color.b = 1.0;
    s->hkb_front_specular_color.r = 1.0;
    s->hkb_front_specular_color.g = 1.0;
    s->hkb_front_specular_color.b = 1.0;
    s->hka_front_specular_color = FALSE;

    s->hki_back_specular_color.r = 1.0;
    s->hki_back_specular_color.g = 1.0;
    s->hki_back_specular_color.b = 1.0;
    s->hkb_back_specular_color.r = 1.0;
    s->hkb_back_specular_color.g = 1.0;
    s->hkb_back_specular_color.b = 1.0;
    s->hka_back_specular_color = FALSE;

    /* Front and back transparency degree */

    s->hki_front_transparency_degree = 1.0;
    s->hkb_front_transparency_degree = 1.0;
    s->hka_front_transparency_degree = FALSE;

    s->hki_back_transparency_degree = 1.0;
    s->hkb_back_transparency_degree = 1.0;
    s->hka_back_transparency_degree = FALSE;

    /* Front and back shading method */

    s->hki_front_shading_method = HK_SHADING_GOURAUD;
    s->hkb_front_shading_method = HK_SHADING_GOURAUD;
    s->hka_front_shading_method = FALSE;

    s->hki_back_shading_method = HK_SHADING_GOURAUD;
    s->hkb_back_shading_method = HK_SHADING_GOURAUD;
    s->hka_back_shading_method = FALSE;

    /* Front and back lighting degree */

    s->hki_front_lighting_degree = HK_SPECULAR_LIGHTING;
    s->hkb_front_lighting_degree = HK_SPECULAR_LIGHTING;
    s->hka_front_lighting_degree = FALSE;

    s->hki_back_lighting_degree = HK_DIFFUSE_LIGHTING;
    s->hkb_back_lighting_degree = HK_DIFFUSE_LIGHTING;
    s->hka_back_lighting_degree = FALSE;

    /* Front and back interior/general style */

    s->hki_front_interior_style = HK_POLYGON_SOLID;
    s->hkb_front_interior_style = HK_POLYGON_SOLID;
    s->hka_front_interior_style = FALSE;

    s->hki_back_interior_style = HK_POLYGON_SOLID;
    s->hkb_back_interior_style = HK_POLYGON_SOLID;
    s->hka_back_interior_style = FALSE;

    s->hki_front_general_style.real_style = HK_POLYGON_SOLID;
    s->hki_front_general_style.general_info = 0;
    s->hkb_front_general_style.real_style = HK_POLYGON_SOLID;
    s->hkb_front_general_style.general_info = 0;
    s->hka_front_general_style = FALSE;

    s->hki_back_general_style.real_style = HK_POLYGON_SOLID;
    s->hki_back_general_style.general_info = 0;
    s->hkb_back_general_style.real_style = HK_POLYGON_SOLID;
    s->hkb_back_general_style.general_info = 0;
    s->hka_back_general_style = FALSE;

    /* Front and back hatch style */

    for (i = 0; i < HK_SD_MAX; i++) {
	s->hki_front_hatch_style.hatch_pattern[i] = 0x5555;
	s->hkb_front_hatch_style.hatch_pattern[i] = 0x5555;

	s->hki_back_hatch_style.hatch_pattern[i] = 0x5555;
	s->hkb_back_hatch_style.hatch_pattern[i] = 0x5555;
    }

    s->hki_front_hatch_style.hatch_transparency = TRUE;
    s->hkb_front_hatch_style.hatch_transparency = TRUE;
    s->hka_front_hatch_style = FALSE;

    s->hki_back_hatch_style.hatch_transparency = TRUE;
    s->hkb_back_hatch_style.hatch_transparency = TRUE;
    s->hka_back_hatch_style = FALSE;

    /* Surface and trimming curve approximation */

    s->hki_iso_curve_info.flag = FALSE;
    s->hki_iso_curve_info.placement = HK_UNIFORM;
    s->hki_iso_curve_info.u_count = 0;
    s->hki_iso_curve_info.v_count = 0;
    s->hkb_iso_curve_info.flag = FALSE;
    s->hkb_iso_curve_info.placement = HK_UNIFORM;
    s->hkb_iso_curve_info.u_count = 0;
    s->hkb_iso_curve_info.v_count = 0;
    s->hka_iso_curve_info = FALSE;

    s->hki_surf_approx.v_approx_value = 1.0;
    s->hki_surf_approx.v_approx_value = 1.0;
    s->hki_surf_approx.approx_type = HK_SURF_CONST_PARAM_SUBDIVISION;
    s->hkb_surf_approx.v_approx_value = 1.0;
    s->hkb_surf_approx.v_approx_value = 1.0;
    s->hkb_surf_approx.approx_type = HK_SURF_CONST_PARAM_SUBDIVISION;
    s->hka_surf_approx = FALSE;

    s->hki_trim_approx.approx_value = 1.0;
    s->hki_trim_approx.approx_type = HK_CONST_PARAM_SUBDIVISION;
    s->hkb_trim_approx.approx_value = 1.0;
    s->hkb_trim_approx.approx_type = HK_CONST_PARAM_SUBDIVISION;
    s->hka_trim_approx = FALSE;

    /* Hollow antialiasing */

    s->hki_hollow_antialiasing = HK_AA_NONE;
    s->hkb_hollow_antialiasing = HK_AA_NONE;
    s->hka_hollow_antialiasing = FALSE;

/* Edge attributes */

    s->silhouette_edge = FALSE;

    s->hki_edge = HK_EDGE_NONE;
    s->hkb_edge = HK_EDGE_NONE;
    s->hka_edge = FALSE;

    s->edge_z_offset = 0.005;

    s->hki_edge_antialiasing = HK_AA_NONE;
    s->hkb_edge_antialiasing = HK_AA_NONE;
    s->hka_edge_antialiasing = FALSE;

    s->hki_edge_color.r = 0.1;
    s->hki_edge_color.g = 0.8;
    s->hki_edge_color.b = 0.5;
    s->hkb_edge_color.r = 0.1;
    s->hkb_edge_color.g = 0.8;
    s->hkb_edge_color.b = 0.5;
    s->hka_edge_color = FALSE;

    s->hki_edge_style = HK_LINESTYLE_SOLID;
    s->hkb_edge_style = HK_LINESTYLE_SOLID;
    s->hka_edge_style = FALSE;

    s->edge_off_color.r = 1.0;
    s->edge_off_color.g = 0.1;
    s->edge_off_color.b = 0.1;

    s->hki_edge_pattern = NULL;
    s->hkb_edge_pattern = NULL;
    s->hka_edge_pattern = FALSE;

    s->hki_edge_width = 1.0;
    s->hkb_edge_width = 1.0;
    s->hka_edge_width = FALSE;

    s->hki_edge_cap = HK_CAP_BUTT;
    s->hkb_edge_cap = HK_CAP_BUTT;
    s->hka_edge_cap = FALSE;

    s->hki_edge_join = HK_JOIN_BUTT;
    s->hkb_edge_join = HK_JOIN_BUTT;
    s->hka_edge_join = FALSE;

    s->hki_edge_miter_limit = 0.0;
    s->hkb_edge_miter_limit = 0.0;
    s->hka_edge_miter_limit = FALSE;

/* Text attributes */

    s->hki_text_antialiasing = HK_AA_NONE;
    s->hkb_text_antialiasing = HK_AA_NONE;
    s->hka_text_antialiasing = FALSE;

    s->hki_text_color.r = 1.0;
    s->hki_text_color.g = 1.0;
    s->hki_text_color.b = 1.0;
    s->hkb_text_color.r = 1.0;
    s->hkb_text_color.g = 1.0;
    s->hkb_text_color.b = 1.0;
    s->hka_text_color = FALSE;

    s->hki_text_expansion_factor = 1.0;
    s->hkb_text_expansion_factor = 1.0;
    s->hka_text_expansion_factor = FALSE;

    s->hki_text_spacing = 0.0;
    s->hkb_text_spacing = 0.0;
    s->hka_text_spacing = FALSE;

    s->hki_text_line_width = 1.0;
    s->hkb_text_line_width = 1.0;
    s->hka_text_line_width = FALSE;

    s->hki_text_cap = HK_CAP_BUTT;
    s->hkb_text_cap = HK_CAP_BUTT;
    s->hka_text_cap = FALSE;

    s->hki_text_join = HK_JOIN_BUTT;
    s->hkb_text_join = HK_JOIN_BUTT;
    s->hka_text_join = FALSE;

    s->hki_text_miter_limit = 0.0;
    s->hkb_text_miter_limit = 0.0;
    s->hka_text_miter_limit = FALSE;

    for (i = 0; i < 4; i++) {
	s->hki_text_character_set[i] = 0;
	s->hkb_text_character_set[i] = 0;
	s->hki_text_font[i] = 0;
	s->hkb_text_font[i] = 0;
    }
    s->hka_text_character_set = FALSE;
    s->hka_text_font = FALSE;

    s->rtext_up_vector.x = 0.0;
    s->rtext_up_vector.y = 1.0;

    s->rtext_path = HK_TP_RIGHT;

    s->rtext_height = 0.1;

    s->rtext_alignment.halign = HK_AH_NORMAL;
    s->rtext_alignment.valign = HK_AV_NORMAL;

    s->rtext_slant = 0.0;

    s->atext_up_vector.x = 0.0;
    s->atext_up_vector.y = 1.0;

    s->atext_path = HK_TP_RIGHT;

    s->atext_height = 0.1;

    s->atext_alignment.halign = HK_AH_NORMAL;
    s->atext_alignment.valign = HK_AV_NORMAL;

    s->atext_slant = 0.0;

    s->atext_style = HK_ATEXT_NONE;

/* Markers */

    s->marker_geom_format = HK_LINE_XYZ;

    s->hki_marker_antialiasing = HK_AA_NONE;
    s->hkb_marker_antialiasing = HK_AA_NONE;
    s->hka_marker_antialiasing = FALSE;

    s->hki_marker_color.r = 1.0;
    s->hki_marker_color.g = 0.0;
    s->hki_marker_color.b = 0.6;
    s->hkb_marker_color.r = 1.0;
    s->hkb_marker_color.g = 0.0;
    s->hkb_marker_color.b = 0.6;
    s->hka_marker_color = FALSE;

    s->hki_marker_size = 16.0;
    s->hkb_marker_size = 16.0;
    s->hka_marker_size = FALSE;

    s->hki_marker_type = NULL;		/* Dot marker */
    s->hkb_marker_type = NULL;
    s->hka_marker_type = FALSE;

/* Transformations */

    /* Modeling matrices */
    identity(s->lmt.matrix);		/* Local modeling transform */
    s->lmt.scale = 1.0;
    identity(s->gmt.matrix);		/* Global modeling transform */
    s->gmt.scale = 1.0;

    /* Viewing parameters */
    s->view.xcenter = 0.0;		/* Viewport */
    s->view.ycenter = 0.0;
    s->view.xsize = 1.0;
    s->view.ysize = 1.0;
    identity(s->view.vt);		/* Viewing transform */
    identity(s->view.ivt);		/* Inverse viewing transform */
    s->view.scale_xy = 0.0;
    s->view.scale_z = 1.0;

    /* NPC Clipping limits */
    s->clipping_limits.xmin = -1.0;
    s->clipping_limits.ymin = -1.0;
    s->clipping_limits.zmin = -1.0;
    s->clipping_limits.xmax =  1.0;
    s->clipping_limits.ymax =  1.0;
    s->clipping_limits.zmax =  1.0;

    /* Z-buffer range */
    s->z_range.zfront = 0x00010000;
    s->z_range.zback  = 0xFFFE0000;

    s->guard_band = 1.0;

/* Depth attributes */

    s->z_buffer_compare = TRUE;

    s->z_buffer_update = HK_Z_UPDATE_NORMAL;

    s->depth_cue_enable = FALSE;

    s->dc_parameters.z_front = 1.0;
    s->dc_parameters.z_back = -1.0;
    s->dc_parameters.scale_front = 1.0;
    s->dc_parameters.scale_back = 0.1;

    s->dc_color.r = 0.0;
    s->dc_color.g = 0.0;
    s->dc_color.b = 0.0;

/* Light source attributes */

    s->light_mask = 0x00000003;

/* Model clipping attributes */

    s->mcp_mask = 0x0000;

    s->num_mcp = 0;

    /* Model clipping planes are undefined */
    /* s->mp_points[][] */
    /* s->mp_norms[][] */
    /* s->model_planes[][] */

/* PHIGS filter related */

    for (i = 0; i < HK_NUMBER_FILTER_WORDS; i++)
	s->name_set[i] = 0;

/* Picking attributes */

    s->csvn = 0;

    s->upick_id = 0;

/* Frame buffer control attributes */

    s->raster_op = HK_ROP_SRC;

    s->plane_mask = ~0;

    s->current_wid = 0;

    s->wid_clip_mask = 0x000;

    s->wid_write_mask = 0x000;

/* Other attributes */

    s->invisibility = 0x0000;

    s->highlight_color.r = 0.3;
    s->highlight_color.g = 0.0;
    s->highlight_color.b = 1.0;

/* Reserved expansion info (must be zero) */

    for (i = 0; i < HK_SZ_STATE_RES; i++)
	s->state_reserved[i] = 0;

/*
 *********************************
 * Initialize non-state variables
 *********************************
 */

/* Execution values */

    c->dpc = 0;				/* Point to start of display program */
    c->risc_regs[HK_RISC_R0] = 0;	/* RISC registers */
    c->risc_regs[HK_RISC_R1] = 0;
    c->risc_regs[HK_RISC_R2] = 0;
    c->risc_regs[HK_RISC_R3] = 0;
    c->risc_regs[HK_RISC_R4] = 0;
    c->risc_regs[HK_RISC_R5] = 0;
    c->risc_regs[HK_RISC_R6] = 0;
    c->risc_regs[HK_RISC_R7] = 0;	/* Stack must still be initialized */

    c->cpu_time = 0;
    c->elapsed_time = 0;

/* Non-state attribute values */

    c->transparency_quality = HK_SCREEN_DOOR;

    c->surface_normal.nx = 0.0;
    c->surface_normal.ny = 0.0;
    c->surface_normal.nz = 1.0;

    c->text_font_table = NULL;

    c->marker_snap_to_grid = FALSE;

    c->ptr_window_boundary = NULL;

    c->aspect_ratio.flag = HK_ASPECT_NONE;
    c->aspect_ratio.ratio = 1.0;

    /* Initialize light 0 (an ambient light) */
    c->lights[0].ls_type = HK_AMBIENT_LIGHT_SOURCE;
    c->lights[0].lr = 1.0;
    c->lights[0].lg = 1.0;
    c->lights[0].lb = 1.0;

    /* Initialize light 1 (a directional light) */
    c->lights[1].ls_type = HK_DIRECTIONAL_LIGHT_SOURCE;
    c->lights[1].lr = 0.8;
    c->lights[1].lg = 0.8;
    c->lights[1].lb = 1.0;
    c->lights[1].dx = 0.57735;
    c->lights[1].dy = 0.57735;
    c->lights[1].dz = 0.57735;

    for (i = 0; i < HK_NUMBER_FILTER_WORDS; i++) {
	c->invisibility_filter.inclusion[i] = 0;
	c->invisibility_filter.exclusion[i] = 0;
	c->highlighting_filter.inclusion[i] = 0;
	c->highlighting_filter.exclusion[i] = 0;
	c->pick_filter.inclusion[i] = 0;
	c->pick_filter.exclusion[i] = 0;
    }

    c->traversal_mode = HK_RENDER;

    c->pick_results_buffer = NULL;

    c->hsvn = 0;

    c->cen = 0;

    c->pick_box.x_left = 0.0;
    c->pick_box.y_bottom = 0.0;
    c->pick_box.z_back = 0.0;
    c->pick_box.x_right = 0.0;
    c->pick_box.y_top = 0.0;
    c->pick_box.z_front = 0.0;

    c->echo_upick_id = 0;
    c->echo_svn = 0;
    c->echo_en = 0;
    c->echo_end_en = 0;
    c->echo_struct_address = 0;

    c->echo_type = HK_PICK_ECHO_EN;

    c->non_echo_invisible = FALSE;

    c->echo_invisible = FALSE;

    c->force_echo_color = TRUE;

    c->echo_color.r = 1.0;
    c->echo_color.g = 1.0;
    c->echo_color.b = 0.0;

    c->plane_group = HK_24BIT;

    c->draw_buffer = HK_BUFFER_A;

    c->fast_clear_set = -1;

    c->stereo_mode = HK_STEREO_NONE;

    c->window_bg_color.r = 0.0;
    c->window_bg_color.g = 0.0;
    c->window_bg_color.b = 0.0;

    c->transparency_mode = HK_ALL_VISIBLE;

    c->stroke_antialiasing = HK_AA_INDIVIDUAL;

    c->stochastic_enable = FALSE;

    c->stochastic_setup.ss_pass_num = 0;
    c->stochastic_setup.ss_display_flag = FALSE;
    c->stochastic_setup.ss_norm_scale = 1.0;
    c->stochastic_setup.ss_x_offset = 0.0;
    c->stochastic_setup.ss_y_offset = 0.0;
    /* c->stochastic_setup.ss_filter_weights[][] */

    c->stencil_fg_color.r = 1.0;
    c->stencil_fg_color.g = 1.0;
    c->stencil_fg_color.b = 0.1;

    c->stencil_bg_color.r = 0.0;
    c->stencil_bg_color.g = 0.0;
    c->stencil_bg_color.b = 0.3;

    c->stencil_transparent = TRUE;

    c->stack_limit = TRUE;

    c->scratch_buffer = NULL;

/* Reserved expansion info (must be zero) */

    for (i = 0; i < HK_SZ_CTX_RES; i++)
	c->ctx_reserved[i] = 0;

} /* End of hk_init_default_context */



/*
 *--------------------------------------------------------------
 *
 * identity
 *
 *	Turn the specified matrix into an identity matrix.
 *
 *--------------------------------------------------------------
 */

static void
identity(m)
    Hk_dmatrix m;
{
    *m++ = 1.0;			/* We were passed a pointer to double */
    *m++ = 0.0;			/* Use brute force method for speed */
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 1.0;			/* 1,1 */
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 1.0;			/* 2,2 */
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 0.0;
    *m++ = 1.0;			/* 3,3 */
} /* End of identity */

/* End of hk_init_ctx.c */
