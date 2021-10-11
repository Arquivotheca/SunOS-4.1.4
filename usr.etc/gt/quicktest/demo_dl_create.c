#ifndef lint
static char sccsid[] = "@(#)demo_dl_create.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_dl_create.c
 *
 *	@(#)demo_dl_create.c 1.35 91/06/05 13:37:37
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the routines that create display lists.
 *
 *  3-Apr-90 Scott R. Nelson	  Initial version.
 *  9-May-90 Scott R. Nelson	  Integrated this module with the other code.
 * 25-May-90 Scott R. Nelson	  Added capability to read external .hdl files.
 * 19-Jul-90 Scott R. Nelson	  Changed from immediate mode to R4-relative
 *				  for all attributes.  Commented code.
 * 23-Jul-90 Scott R. Nelson	  Added rotation axis.
 * 22-Aug-90 Scott R. Nelson	  Fixed open to not crash on errors.
 * 21-Sep-90 Scott R. Nelson	  Moved initial display list to demo_templates.c
 * 27-Sep-90 Scott R. Nelson	  Removed initialization code for all bug
 *				  the cube.
 * 11-Oct-90 Scott R. Nelson	  Changes to window bounds instruction.
 * 29-Jan-91 John M. Perry	  XNeWS client direct graphics access (dga) 
 * 31-Jan-91 John M. Perry	  For DGA, don't do WID replace, XNeWS does it.
 * 22-Feb-91 John M. Perry	  For DGA, enable WID clipping.
 * 16-Apr-91 Chris Klein	  Added SunView capability
 * 18-Apr-91 Kevin C. Rushforth	  Added SUSPEND/RESUME ioctl() calls.
 * 30-May-91 John M. Perry        Add Stereo support for DGA QUICKTEST.
 * 06-Jun-91 John M. Perry        Inquire wid partioning wid_clip_mask set
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <sys/fcntl.h>
#include <math.h>
#include "demo.h"
#include "demo_sv.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	build_main_loop
 *	build_dl0
 *	    hk_init_default_context
 *	    init_environment
 *	build_dl
 *	    fopen
 *	    fclose
 *	    hk_read_hdl_header
 *	    hk_load_hdl_file
 *	    hk_init_default_context
 *	    init_environment
 *	init_environment
 *          set_sv_wb
 *
 *==============================================================
 */



/*
 * Individual (simple) objects
 */

/* Color cube (needs processing) */
static unsigned poly_command0[] = {
    ((HK_OP_SET_ATTR_I << HK_OP_POS) | (HK_LINE_GEOM_FORMAT << HK_ATTR_POS)
	| (HK_AM_IMMEDIATE << HK_AM_POS)),
    (HK_LINE_XYZ_RGB),
    ((HK_OP_POLYLINE << HK_OP_POS) | (2 << HK_OFFSET_POS)),
};

static float polyline0[] = {
     0.7,  0.7,  0.7,
    -0.7,  0.7,  0.7,
     0.7,  0.7, -0.7,
    -0.7,  0.7, -0.7,
     0.7, -0.7,  0.7,
    -0.7, -0.7,  0.7,
     0.7, -0.7, -0.7,
    -0.7, -0.7, -0.7,

     0.7,  0.7,  0.7,
     0.7, -0.7,  0.7,
     0.7,  0.7, -0.7,
     0.7, -0.7, -0.7,
    -0.7,  0.7,  0.7,
    -0.7, -0.7,  0.7,
    -0.7,  0.7, -0.7,
    -0.7, -0.7, -0.7,

     0.7,  0.7,  0.7,
     0.7,  0.7, -0.7,
     0.7, -0.7,  0.7,
     0.7, -0.7, -0.7,
    -0.7,  0.7,  0.7,
    -0.7,  0.7, -0.7,
    -0.7, -0.7,  0.7,
    -0.7, -0.7, -0.7,
};

/* Top red, bottom blue, positive corner white, negative corner black */
static float colors0[] = {
     0.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
     0.0,  0.0,  1.0,
     1.0,  0.0,  1.0,
     0.0,  1.0,  0.0,
     1.0,  1.0,  0.0,
     0.0,  1.0,  1.0,
     1.0,  1.0,  1.0,

     0.0,  0.0,  0.0,
     0.0,  1.0,  0.0,
     0.0,  0.0,  1.0,
     0.0,  1.0,  1.0,
     1.0,  0.0,  0.0,
     1.0,  1.0,  0.0,
     1.0,  0.0,  1.0,
     1.0,  1.0,  1.0,

     0.0,  0.0,  0.0,
     0.0,  0.0,  1.0,
     0.0,  1.0,  0.0,
     0.0,  1.0,  1.0,
     1.0,  0.0,  0.0,
     1.0,  0.0,  1.0,
     1.0,  1.0,  0.0,
     1.0,  1.0,  1.0,
};

static unsigned ending_dl[] = {		/* What goes at the end... */
    ((HK_OP_JMPL << HK_OP_POS) | (HK_AM_R6_INDIRECT << HK_AM_POS)
	| (HK_AM_R6_INDIRECT << HK_REG_POS)),
};



/*
 *--------------------------------------------------------------
 *
 * build_main_loop
 *
 *	Build the main loop of the display list.
 *
 *--------------------------------------------------------------
 */

void
build_main_loop()
{
    char *dl;				/* Pointer to display list */

    dl = (char *) vm;
    main_dl = (unsigned) dl;

    memcpy(dl, main_loop, sizeof(main_loop));
    dl += sizeof(main_loop);

    vm = (unsigned *) dl;
} /* End of build_main_loop */



/*
 *--------------------------------------------------------------
 *
 * build_dl0
 *
 *	Build display list 0, including context.
 *
 *--------------------------------------------------------------
 */

void
build_dl0()
{
    unsigned *dl;			/* Pointer to display list */
    int i, j;
    unsigned pc;			/* Polyline command */
    unsigned *v, *c;			/* Source vertex, color pointers */

/* Build display list number 0 */

    dle_ptr = &dle[0];

    dle_ptr->ctx = (Hk_context *) vm;
    vm += sizeof(Hk_context) / sizeof(unsigned);

    dle_ptr->p_dl_data = (dl_data *) vm;
    vm += sizeof(dl_data) / sizeof(unsigned);

    dl = (unsigned *) vm;
    dle_ptr->dl_begin = (unsigned) main_dl; /* Remember display list start */
    dle_ptr->obj_begin = (unsigned) dl;	/* Remember object start */

    memcpy(dl, poly_command0, sizeof(poly_command0));
    dl += sizeof(poly_command0) / sizeof(unsigned);
    pc = *(dl - 1);

    v = (unsigned *) polyline0;
    c = (unsigned *) colors0;
    for (i = 0; i < 12; i++) {
	if (i)
	    *dl++ = pc;
	for (j = 0; j < 3; j++)
	    *dl++ = *v++;		/* Vertex */
	for (j = 0; j < 3; j++)
	    *dl++ = *c++;		/* Color */
	for (j = 0; j < 3; j++)
	    *dl++ = *v++;		/* Vertex */
	for (j = 0; j < 3; j++)
	    *dl++ = *c++;		/* Color */
    }
    memcpy(dl, ending_dl, sizeof(ending_dl));
    dl += sizeof(ending_dl);
    vm = (unsigned *) dl;

    /* Initialize context, display list area, etc. */

    init_environment("cube", 0,
	0.0, 0.0, 1.0,
	0.0, 0.0, 0.0,
	(1 * screen_x) / 8, (1 * screen_y) / 8,
	(2 * screen_x) / 8, (2 * screen_y) / 8);

} /* End of build_dl0 */



/*
 *--------------------------------------------------------------
 *
 * build_dl
 *
 *	Build display list from a .hdl file, including context.
 *	Return 1 of success, 0 if error.
 *
 *--------------------------------------------------------------
 */

int
build_dl(name, num)
    char name[];			/* Display list file name */
    int num;				/* Display list number */
{
    unsigned dl;			/* Display list pointer */
    FILE *hdl_fd;			/* File descriptor */
    int dl_size;			/* Size of display list to read in */
    int hk_read_hdl_header();
    int hk_load_hdl_file();

/* Build display list */

    dle_ptr = &dle[num];

    dle_ptr->ctx = (Hk_context *) vm;
    vm += sizeof(Hk_context) / sizeof(unsigned);

    dle_ptr->p_dl_data = (dl_data *) vm;
    vm += sizeof(dl_data) / sizeof(unsigned);

    hdl_fd = fopen(name, "r");
    if (hdl_fd == NULL) {
	perror("\r\ndemo: Unable to open file");
	return (0);
    }

    dl_size = hk_read_hdl_header(hdl_fd);

    if (dl_size < 0) {
	printw("Couldn't read valid .hdl information for %s\n", name);
	fclose(hdl_fd);
	return (0);
    }
    if (dl_size > (((int) vm_base + (int) vm_size) - (int) vm)) {
	printw("Not enough room to read %s\n", name);
	fclose(hdl_fd);
	return (0);
    }

    dl = (unsigned) vm;

    if (hk_load_hdl_file(hdl_fd, dl_size, vm, vm) < 0) {
	printw("Failed to read .hdl file %s\n", name);
	fclose(hdl_fd);
	return (0);
    }
    fclose(hdl_fd);

    vm = (unsigned *) (dl + dl_size);
    dle_ptr->dl_begin = (unsigned) main_dl; /* Remember display list start */
    dle_ptr->obj_begin = dl;		/* Remember object start */

    init_environment(name, num,
	1.0, 1.0, 1.0,			/* White lines */
	0.0, 0.0, 0.0,			/* On a black background */
	(2 * screen_x) / 8, (2 * screen_y) / 8,
	(4 * screen_x) / 8, (4 * screen_y) / 8);

    return (1);
} /* End of build_dl */



/*
 *--------------------------------------------------------------
 *
 * init_environment
 *
 *	Initialize the display list environment to specified and
 *	standard values.  dle_ptr (global) points to the display list
 *	environment area.
 *
 *--------------------------------------------------------------
 */

/* Macro to take initial value and update dial data record */
#define FIXUP(dldta, tmplt, value) \
    dle_ptr->dldta = tmplt; \
    dle_ptr->dldta.current_value = value; \
    dle_ptr->dldta.current_abs = (int) (value / dle_ptr->dldta.mult_factor);

void
init_environment(name, num, or, og, ob, br, bg, bb, xleft, ytop, width, height)
    char *name;				/* Object name */
    int num;				/* Window number */
    float or, og, ob;			/* Object color */
    float br, bg, bb;			/* Background color */
    int xleft, ytop, width, height; /* Window bounds */
{
    /* Make sure it is ok to write to the context */
    GT_SUSPEND();

    /* Initialize the context area */
    hk_init_default_context(dle_ptr->ctx);

    strcpy(dle_ptr->name, name);

    identity(dle_ptr->mat.matrix);
    dle_ptr->mat.scale = 1.0;

    /* Initialize the stack pointer */
    dle_ptr->ctx->risc_regs[HK_RISC_SP] = (int) vm_base + vm_size;

    /* Initialize the display list data area */
    memcpy(dle_ptr->p_dl_data, dl_data_template, sizeof(dl_data));


/* Initialize all dial structures to proper values */

    dle_ptr->x_rot = rot_template;
    dle_ptr->y_rot = rot_template;
    dle_ptr->z_rot = rot_template;
    dle_ptr->x_rot.attr_update = update_x_rot;
    dle_ptr->y_rot.attr_update = update_y_rot;
    dle_ptr->z_rot.attr_update = update_z_rot;

    dle_ptr->x_rot_axis = rot_template;
    dle_ptr->y_rot_axis = rot_template;
    dle_ptr->z_rot_axis = rot_template;
    dle_ptr->x_rot_axis.attr_update = update_x_rot_axis;
    dle_ptr->y_rot_axis.attr_update = update_y_rot_axis;
    dle_ptr->z_rot_axis.attr_update = update_z_rot_axis;

    dle_ptr->rot_speed = rot_speed_template;

    dle_ptr->x_trans = trans_template;
    dle_ptr->y_trans = trans_template;
    dle_ptr->z_trans = trans_template;

    dle_ptr->scale = scale_template;
    dle_ptr->perspective = persp_template;

    FIXUP(line_r, color_template, or);
    FIXUP(line_g, color_template, og);
    FIXUP(line_b, color_template, ob);
    dle_ptr->p_dl_data->object_color.r = or;
    dle_ptr->p_dl_data->object_color.g = og;
    dle_ptr->p_dl_data->object_color.b = ob;

    dle_ptr->surface_r = color_template;
    dle_ptr->surface_g = color_template;
    dle_ptr->surface_b = color_template;

    FIXUP(bg_r, color_template, br);
    FIXUP(bg_g, color_template, bg);
    FIXUP(bg_b, color_template, bb);
    dle_ptr->p_dl_data->bg_color.r = br;
    dle_ptr->p_dl_data->bg_color.g = bg;
    dle_ptr->p_dl_data->bg_color.b = bb;
    dle_ptr->bg_r.attr_update = update_bg_color;
    dle_ptr->bg_g.attr_update = update_bg_color;
    dle_ptr->bg_b.attr_update = update_bg_color;

    FIXUP(window_x, window_x_template, xleft);
    FIXUP(window_y, window_y_template, ytop);
    FIXUP(window_width, window_width_template, width);
    FIXUP(window_height, window_height_template, height);

    if (dga) {
	int temp;

        dle_ptr->p_dl_data->current_wid = dga->w_wid;
        dle_ptr->p_dl_data->display_a.entry = dga->w_wid;
        dle_ptr->p_dl_data->display_b.entry = dga->w_wid;
	dle_ptr->p_dl_data->wid_write_mask = 0x0;

	if (!dga->w_cdevfd)
	   dga->w_cdevfd = open(dga->w_devname, O_RDWR, 0666);

	ioctl(dga->w_cdevfd, FB_GETWPART, &temp);
	dle_ptr->p_dl_data->wid_clip_mask = (1 << temp) - 1;

	dle_ptr->p_dl_data->wb_ptr = 
		(Hk_window_boundary *) &(dga->w_window_boundary);

	dle_ptr->p_dl_data->fast_clear_set = dga->wx_dbuf.device.hawk.gt_fcs[0];
	dga->wx_dbuf.device.hawk.gt_buf_fcs[0] = 
					dle_ptr->p_dl_data->fast_clear_set;
#ifdef QUICKTEST
	if (dga_stereo)
	    dle_ptr->p_dl_data->tr_flags |= TRF_STEREO; 
#endif
    }
    else if( sv_win.wid ) {
        dle_ptr->p_dl_data->current_wid = sv_win.wid;
        dle_ptr->p_dl_data->display_a.entry = sv_win.wid;
        dle_ptr->p_dl_data->display_b.entry = sv_win.wid;
	dle_ptr->p_dl_data->wid_write_mask = 0x0;
	dle_ptr->p_dl_data->wid_clip_mask = 0x1f;

        dle_ptr->p_dl_data->wb_ptr = &dle_ptr->p_dl_data->window_bounds;
        sv_win.cur_wb_ptr = dle_ptr->p_dl_data->wb_ptr;
	/* Set SunView Window Bounds */
        set_sv_wb();
	dle_ptr->p_dl_data->fast_clear_set = fcs_data.index;
    } else {
        dle_ptr->p_dl_data->current_wid = num;
        dle_ptr->p_dl_data->display_a.entry = num;
        dle_ptr->p_dl_data->display_b.entry = num;

        dle_ptr->p_dl_data->window_bounds.xleft = xleft;
        dle_ptr->p_dl_data->window_bounds.ytop = ytop;
        dle_ptr->p_dl_data->window_bounds.width = width;
        dle_ptr->p_dl_data->window_bounds.height = height;
        dle_ptr->p_dl_data->wb_ptr = &dle_ptr->p_dl_data->window_bounds;
    }
    fix_window_aspect();

    FIXUP(dc_front, dc_range_template, dle_ptr->p_dl_data->dc_param.z_front);
    FIXUP(dc_back, dc_range_template, dle_ptr->p_dl_data->dc_param.z_back);
    FIXUP(dc_max, color_template, dle_ptr->p_dl_data->dc_param.scale_front);
    FIXUP(dc_min, color_template, dle_ptr->p_dl_data->dc_param.scale_back);
    dle_ptr->dc_front.attr_update = update_depth_cue;
    dle_ptr->dc_back.attr_update = update_depth_cue;
    dle_ptr->dc_max.attr_update = update_depth_cue;
    dle_ptr->dc_min.attr_update = update_depth_cue;

    FIXUP(ambient, coeff_template, 0.20);
    FIXUP(diffuse, coeff_template, 0.80);
    FIXUP(specular, coeff_template, 1.00);
    FIXUP(transparency, coeff_template, 1.00);
    FIXUP(spec_r, color_template, 1.00);
    FIXUP(spec_g, color_template, 1.00);
    FIXUP(spec_b, color_template, 1.00);
    FIXUP(spec_exp, exp_template, 20.0);
    dle_ptr->ambient.attr_update = update_surf_prop;
    dle_ptr->diffuse.attr_update = update_surf_prop;
    dle_ptr->specular.attr_update = update_surf_prop;
    dle_ptr->transparency.attr_update = update_surf_prop;
    dle_ptr->spec_r.attr_update = update_surf_prop;
    dle_ptr->spec_g.attr_update = update_surf_prop;
    dle_ptr->spec_b.attr_update = update_surf_prop;
    dle_ptr->spec_exp.attr_update = update_surf_prop;

    /* Done writing to context */
    GT_RESUME();

} /* End of init_environment */

/* End of demo_dl_create.c */
