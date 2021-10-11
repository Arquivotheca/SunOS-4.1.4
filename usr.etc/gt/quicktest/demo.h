#ifndef _DEMO_
#define _DEMO_

/*
 ***********************************************************************
 *
 * demo.h
 *
 *      @(#)demo.h  1.1 94/10/31  SMI 
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	Data structures, constants, external procedure definitions
 *	and global variables for the Hawk demo program.
 *
 *  2-Apr-90 Scott R. Nelson	  Initial file.
 *  9-May-90 Scott R. Nelson	  Integration of separate pieces.
 * 19-Jul-90 Scott R. Nelson	  Switch to data in data record for HDL.
 * 23-Jul-90 Scott R. Nelson	  Added rotation axis, stacks.
 * 20-Aug-90 Scott R. Nelson	  Changes for gtmcb file.
 * 27-Sep-90 Scott R. Nelson	  Added more attributes and lights
 *  4-Oct-90 Scott R. Nelson	  Bump version to 0.6, minor tuning.
 * 11-Oct-90 Scott R. Nelson	  Changes to window bounds instruction.
 * 15-Jan-91 Scott R. Nelson	  Changes to make HIS 1.5 work.
 * 29-Jan-91 John M. Perry	  XNeWS port, add direct graphics access (dga)
 * 29-Jan-91 Kevin Rushforth	  minor HIS 1.5 name changes.
 * 12-Feb-91 John M. Perry  	  add XNews display and window 
 * 22-Feb-91 John M. Perry  	  Bump version to 1.0 for XNeWS support 
 * 18-Apr-91 Kevin C. Rushforth	  Added SUSPEND/RESUME ioctl() calls.
 * 10-May-91 Chris  Klein	  Added Auto-cycle under QUICKTEST
 * 30-May-91 John M. Perry 	  Add Stereo support for XNeWS QUICKTEST 
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <sys/types.h> 
#include <sys/param.h> 
#include <sys/ioctl.h> 

#include <sbusdev/gtmcb.h>
#include <sbusdev/gtreg.h>
#include "hk_public.h"

#include <X11/Xlib.h>
#include "win_grab.h"



/* Some defined constants */
#define FESIM		"hk_fesim"	/* Hawk front end simulator */
#define DEMO_VERSION	"1.1 (18-Apr-91)" /* Version of the code */
#define MAX_ARGC	10		/* Maximum argc for hk_fesim */
/* Max size for DL + CTX + Stack */
#define MAX_VM_SMALL	0x170000	/* For HFE environment (1.5 Mb limit) */
#define MAX_VM_BIG	0x800000	/* For SIM and GT */

#define CTRL_C		0x03
#define CTRL_D		0x04
#define BS		0x08
#define LF		0x0A
#define NL		0x0A
#define CR		0x0D
#define CTRL_L		0x0C
#define CTRL_U		0x15
#define CTRL_W		0x17
#define ESC		0x1B
#define DEL		0x7F

#define BG_R		0.15		/* Default background color */
#define BG_G		0.15
#define BG_B		0.15

/* Local macros */
#define RAD(a)	((a) * M_PI / 180.0)
#define DEG(a)	((a) * 180.0 / M_PI)

/* Macro to stop screen update while loading a file (be careful here) */
#define REFRESH if (input_file == NULL) refresh()

/* ANSI standard macro to get the offset of a structure element */
#define offsetof(type, member) ((unsigned) (&((type *)0)->member))

/* Macros to suspend/resume display list execution */
#ifdef DISALLOW_SUSPEND
#define GT_SUSPEND()
#define GT_RESUME()
#else DISALLOW_SUSPEND
#define GT_SUSPEND()						\
    {								\
	if (hk_ioctl(FB_VMCTL, &vmctl_suspend) == -1)		\
	    perror("GT_SUSPEND");				\
    }

#define GT_RESUME()						\
    {								\
	if (hk_ioctl(FB_VMCTL, &vmctl_resume) == -1)		\
	    perror("GT_RESUME");				\
    }
#endif DISALLOW_SUSPEND

/* Maximum number of objects this can keep track of */
#define OBJ_MAX		10

/* Values for Hawk window */
#define DEF_WID		15		/* Default window ID */
#define DEF_CLUT	14		/* Default CLUT to use */

/* Macro to cycle through enumerated types */
#define cycle_forward(v, max) \
	do {(v) += 1; if ((v) > (max)) (v) = 0; } while (0)
#define cycle_back(v, max) \
	do {(v) -= 1; if ((int) (v) < 0) (v) = (max); } while (0)




#define DIAL_ONE_ROTATION 23040		/* 256 * 90 */
#define DIAL_ONE_DEGREE	64
/* Dial update structure */

typedef struct dial_data dial_data;
struct dial_data {
    float current_value;		/* Current dial value */
    int current_abs;			/* Current absolute value */
    float mult_factor;			/* Multiplication factor */
    int min_abs;			/* Minimum absolute value */
    int max_abs;			/* Maximum absolute value */
    int flag;				/* Flag, see description below */
    void (*dial_constraints)();		/* Constraint checking routine */
    void (*attr_update)();		/* Attribute update routine */
    int key_step;			/* What one key-stroke is worth */
};

/* Flag definitions */
#define WRAP		0x0000
#define CLAMP		0x0001		/* 0 = wrap, 1 = clamp */

#define ABSOLUTE	0x0000
#define RELATIVE	0x0002		/* 0 = absolute, 1 = relative */

#define ON_OFF		0x0004		/* The value is 1, 0 only */



/* Display list data accessed by the attribute instructions */

typedef struct dl_data dl_data;
struct dl_data {
    unsigned tr_flags;			/* Traversal flags */
    unsigned fast_clear_set;		/* Fast clear set */
    int z_buffer_update;		/* Z-buffer update */
    float guard_band;			/* Guard band */
    Hk_fcbg_update fcbg_color;		/* Fast clear background color */

    Hk_plane_group plane_group;		/* Plane group */
    int plane_mask;			/* Plane mask */
    int current_wid;			/* Current window ID */
    int wid_clip_mask;			/* WID clip mask */
    int wid_write_mask;			/* WID write_mask */

    Hk_rgb object_color;		/* Object color */
    Hk_rgb bg_color;			/* Background color */
    Hk_rgb edge_color;			/* Edge color */
    Hk_rgb line_off_color;		/* Line off color */

    int antialias;			/* Flag to enable antialiasing */
    Hk_line_shading_method line_shad_meth; /* Line shading method */
    Hk_line_style line_style;		/* Line style */
    Hk_pattern *pattern_ptr;		/* Pattern pointer */
    float line_width;			/* Line width */
    Hk_line_cap line_cap;		/* Line cap */
    Hk_line_join line_join;		/* Line join */
    float line_miter_limit;		/* Line miter limit */

    Hk_edge_control edge;		/* Edge control */
    float edge_z_offset;		/* Edge Z offset */

    Hk_marker_type *marker;		/* Marker pointer */
    float marker_size;			/* Marker size */

    int use_back_props;			/* Use back surface properties */
    int shading_method;			/* Shading method */
    int lighting_degree;		/* Lighting degree */
    Hk_material_properties mat_prop;	/* Material properties */
    Hk_rgb spec_color;			/* Specular color */
    float transp_degree;		/* Transparency degree */
    Hk_interior_style interior_style;	/* Front/back interior style */
    Hk_hatch_style hatch_style;		/* Front/back hatch style */
    int culling_mode;			/* Culling mode */
    Hk_transparency_quality trans_qual;	/* Transparency quality */

    int depth_cue;			/* Enable depth-cuing */
    Hk_depth_cue_parameters dc_param;	/* Depth-cue parameters */

    Hk_light_mask_update light_mask;	/* Light mask */
    Hk_light_source light_0;		/* Light 0 */
    Hk_light_source light_1;		/* Light 1 */
    Hk_light_source light_2;		/* Light 2 */
    Hk_light_source light_3;		/* Light 3 */
    Hk_light_source light_4;		/* Light 4 */
    Hk_light_source light_5;		/* Light 5 */
    Hk_light_source light_6;		/* Light 6 */
    Hk_light_source light_7;		/* Light 7 */
    Hk_light_source light_8;		/* Light 8 */

    int draw_a;				/* Draw to buffer A */
    int draw_b;				/* Draw to buffer B */
    Hk_wlut_update display_a;		/* Display from buffer A */
    Hk_wlut_update display_b;		/* Display from buffer B */

    /* Warning: Objects with doubles get lined up on 64-bit boundaries */
    Hk_view view;			/* Current view */
    Hk_window_boundary window_bounds;	/* Current window position */
    Hk_model_transform global_matrix;	/* Global modelling transform matrix */
    Hk_model_transform trans_matrix;	/* Current translation matrix */
    Hk_model_transform rot_matrix;	/* Current rotation matrix */
    Hk_model_transform scale_matrix;	/* Current scale matrix */
    Hk_model_transform rot_axis_matrix;	/* Rotation axis matrix */
    Hk_model_transform spin_matrix;	/* Current spin matrix */
    Hk_window_boundary *wb_ptr;		/* Pointer to window boundary */

    int z_buf_stereo;			/* Filler: 0 */
    Hk_view left_view;			/* Stereo left eye view */
    Hk_view right_view;			/* Stereo right eye view */
    Hk_rgb left_color;			/* Left color, etc. */
    Hk_rgb right_color;
/* End of objects with doubles in them */
};

/* Note: The following offsets must match the above structure exactly */
#define TR_FLAGS	0
#define FCS		(TR_FLAGS + (sizeof(unsigned) / 4))
#define Z_BUFFER_UPDATE	(FCS + (sizeof(int) / 4))
#define GUARD_BAND	(Z_BUFFER_UPDATE + (sizeof(int *) / 4))
#define FCBG_COLOR	(GUARD_BAND + (sizeof(float) / 4))

#define PLANE_GROUP	(FCBG_COLOR + (sizeof(Hk_fcbg_update) / 4))
#define PLANE_MASK	(PLANE_GROUP + (sizeof(Hk_plane_group) / 4))
#define CURRENT_WID	(PLANE_MASK + (sizeof(int) / 4))
#define WID_CLIP_MASK	(CURRENT_WID + (sizeof(int) / 4))
#define WID_WRITE_MASK	(WID_CLIP_MASK + (sizeof(int) / 4))

#define OBJECT_COLOR	(WID_WRITE_MASK + (sizeof(int) / 4))
#define BG_COLOR	(OBJECT_COLOR + (sizeof(Hk_rgb) / 4))
#define EDGE_COLOR	(BG_COLOR + (sizeof(Hk_rgb) / 4))
#define LINE_OFF_COLOR	(EDGE_COLOR + (sizeof(Hk_rgb) / 4))

#define ANTIALIAS	(LINE_OFF_COLOR + (sizeof(Hk_rgb) / 4))
#define LINE_SHAD_METH	(ANTIALIAS + (sizeof(int) / 4))
#define LINE_STYLE	(LINE_SHAD_METH + (sizeof(Hk_line_shading_method) / 4))
#define PATTERN_PTR	(LINE_STYLE + (sizeof(Hk_line_style) / 4))
#define LINE_WIDTH	(PATTERN_PTR + (sizeof(Hk_pattern *) / 4))
#define LINE_CAP	(LINE_WIDTH + (sizeof(float) / 4))
#define LINE_JOIN	(LINE_CAP + (sizeof(Hk_line_cap) / 4))
#define LINE_MITER_LIMIT (LINE_JOIN + (sizeof(Hk_line_join) / 4))

#define EDGE		(LINE_MITER_LIMIT + (sizeof(float) / 4))
#define EDGE_Z_OFFSET	(EDGE + (sizeof(Hk_edge_control) / 4))

#define MARKER		(EDGE_Z_OFFSET + (sizeof(float) / 4))
#define MARKER_SIZE	(MARKER + (sizeof(Hk_marker_type *) / 4))

#define USE_BACK_PROPS	(MARKER_SIZE + (sizeof(float) / 4))
#define SHADING_METHOD	(USE_BACK_PROPS + (sizeof(int) / 4))
#define LIGHTING_DEGREE	(SHADING_METHOD + (sizeof(int) / 4))
#define MAT_PROP	(LIGHTING_DEGREE + (sizeof(int) / 4))
#define SPEC_COLOR	(MAT_PROP + (sizeof(Hk_material_properties) / 4))
#define TRANSP_DEGREE	(SPEC_COLOR + (sizeof(Hk_rgb) / 4))
#define INTERIOR_STYLE	(TRANSP_DEGREE + (sizeof(float) / 4))
#define HATCH_STYLE	(INTERIOR_STYLE + (sizeof(Hk_interior_style) / 4))
#define CULLING_MODE	(HATCH_STYLE + (sizeof(Hk_hatch_style) / 4))
#define TRANS_QUAL	(CULLING_MODE + (sizeof(int) / 4))

#define DEPTH_CUE	(TRANS_QUAL + (sizeof(Hk_transparency_quality) / 4))
#define DC_PARAM	(DEPTH_CUE + (sizeof(int) / 4))

#define LIGHT_MASK	(DC_PARAM + (sizeof(Hk_depth_cue_parameters) / 4))
#define LIGHT_0		(LIGHT_MASK + (sizeof(Hk_light_mask_update) / 4))
#define LIGHT_1		(LIGHT_0 + (sizeof(Hk_light_source) / 4))
#define LIGHT_2		(LIGHT_1 + (sizeof(Hk_light_source) / 4))
#define LIGHT_3		(LIGHT_2 + (sizeof(Hk_light_source) / 4))
#define LIGHT_4		(LIGHT_3 + (sizeof(Hk_light_source) / 4))
#define LIGHT_5		(LIGHT_4 + (sizeof(Hk_light_source) / 4))
#define LIGHT_6		(LIGHT_5 + (sizeof(Hk_light_source) / 4))
#define LIGHT_7		(LIGHT_6 + (sizeof(Hk_light_source) / 4))
#define LIGHT_8		(LIGHT_7 + (sizeof(Hk_light_source) / 4))

#define DRAW_A		(LIGHT_8 + (sizeof(Hk_light_source) / 4))
#define DRAW_B		(DRAW_A + (sizeof(int) / 4))
#define DISPLAY_A	(DRAW_B + (sizeof(int) / 4))
#define DISPLAY_B	(DISPLAY_A + (sizeof(Hk_wlut_update) / 4))

#define VIEW		(DISPLAY_B + (sizeof(Hk_wlut_update) / 4))
#define WINDOW_BOUNDS	(VIEW + (sizeof(Hk_view) / 4))
#define GLOBAL_MATRIX	(WINDOW_BOUNDS + (sizeof(Hk_window_boundary) / 4))
#define TRANS_MATRIX	(GLOBAL_MATRIX + (sizeof(Hk_model_transform) / 4))
#define ROT_MATRIX	(TRANS_MATRIX + (sizeof(Hk_model_transform) / 4))
#define SCALE_MATRIX	(ROT_MATRIX + (sizeof(Hk_model_transform) / 4))
#define ROT_AXIS_MATRIX	(SCALE_MATRIX + (sizeof(Hk_model_transform) / 4))
#define SPIN_MATRIX	(ROT_AXIS_MATRIX + (sizeof(Hk_model_transform) / 4))
#define WB_PTR		(SPIN_MATRIX + (sizeof(Hk_model_transform) / 4))

#define Z_BUF_STEREO	(WB_PTR + (sizeof(Hk_window_boundary *) / 4))
#define LEFT_VIEW	(Z_BUF_STEREO + (sizeof(int *) / 4))
#define RIGHT_VIEW	(LEFT_VIEW + (sizeof(Hk_view) / 4))
#define LEFT_COLOR	(RIGHT_VIEW + (sizeof(Hk_view) / 4))
#define RIGHT_COLOR	(LEFT_COLOR + (sizeof(Hk_rgb) / 4))
#define NEXT_ITEM	(RIGHT_COLOR + (sizeof(Hk_rgb) / 4))


/* Traversal flag bit definitions */
#define TRF_BUFFER_B	0x00000001	/* Currently drawing to buffer B */
#define TRF_SET_FCBG	0x00000002	/* Set fast clear background color */
#define TRF_PREPARE_WINDOW 0x00000004	/* Prepare all pixels of window */
#define TRF_PREPARE_SURFACE 0x00000008	/* Prepare all pixels of screen */
#define TRF_STEREO	0x00000010	/* Go into stereo mode */



/* Display list environment for multiple display lists */

typedef struct dl_env dl_env;
struct dl_env {
    Hk_context *ctx;			/* The context for this display list */
    unsigned dl_begin;			/* Where the display list starts */
    unsigned obj_begin;			/* Where the object starts */
    dl_data *p_dl_data;			/* Pointer to display list data */
    char name[256];			/* Name of the object */
    Hk_model_transform mat;		/* Matrix build area */

/* Transformation stuff */

    dial_data x_rot;
    dial_data y_rot;
    dial_data z_rot;
    dial_data x_trans;
    dial_data y_trans;
    dial_data z_trans;
    dial_data scale;
    dial_data perspective;
    dial_data line_r;
    dial_data line_g;
    dial_data line_b;
    dial_data surface_r;
    dial_data surface_g;
    dial_data surface_b;
    dial_data bg_r;
    dial_data bg_g;
    dial_data bg_b;
    dial_data window_x;
    dial_data window_y;
    dial_data window_width;
    dial_data window_height;
    dial_data dc_front;
    dial_data dc_back;
    dial_data dc_max;
    dial_data dc_min;
    dial_data x_rot_axis;
    dial_data y_rot_axis;
    dial_data z_rot_axis;
    dial_data rot_speed;
    dial_data ambient;
    dial_data diffuse;
    dial_data specular;
    dial_data transparency;
    dial_data spec_r;
    dial_data spec_g;
    dial_data spec_b;
    dial_data spec_exp;
};


/* Array of pointers to dial data */

typedef dial_data *dial_array[8];


/* Menu text */

typedef struct Menu Menu;
struct Menu {
    char *highlight;			/* Highlighted portion of text string */
    char *text_string;			/* Main text string */
};



/*
 * Global variables
 */

/* From demo_globals.c */

extern char *fesim_argv[MAX_ARGC];	/* Arguments to hk_fesim */
extern int fesim_argc;			/* Number of arguments to hk_fesim */
extern FILE *input_file;		/* Input file, if specified */
extern char input_file_name[256];	/* Name of input file */
extern int verbose;			/* Indicates verbose messages */
extern int use_dials;			/* Indicates to use control dials */
extern int use_cg9_overlay;		/* Use CG9 overlay planes */
extern char *prog_name;			/* Name of program for error msgs */
extern int screen_x, screen_y;		/* Screen size */
extern int input_ch;			/* Last character input from keyboard */

extern struct fb_vmctl vmctl_suspend;	/* Struct used for suspend */
extern struct fb_vmctl vmctl_resume;	/* Struct used for resume */

extern WXINFO *dga;			/* XNeWS Direct Graphics Access.*/  
extern Display *x_display;		/* XNeWS display */ 
extern Window x_win;			/* XNeWS window */  
extern int dga_stereo;			/* XNeWS DGA stereo mode */

extern int continuous_draw;		/* Don't wait for command to draw */
extern int auto_rotate;			/* Automatically rotate the object */
#ifdef QUICKTEST
extern int Autocycle;			/* Automatically cycles through objects */
#endif QUICKTEST

extern int done_with_program;		/* Exit flag */ 
extern int no_data;			/* Data input flag */ 

#define MS_MAX 20			/* Maximum menu stack depth */
extern int menu_sp;			/* Menu stack pointer */
extern void (*display_current_screen[MS_MAX])(); /* Procedure to update the screen */
extern void (*get_input[MS_MAX])();	/* Procedure to get input */

extern dial_array cur_dials;		/* Current dial records */
extern dial_array dial_stack[MS_MAX];	/* Dial data stack */

extern int db_fd;			/* Dial box file descriptor */

extern dl_env *dle_ptr;			/* Current display list environment */
extern int dl;				/* Which display list is current */

extern dl_env dle[OBJ_MAX];		/* The various display list environs */

extern unsigned main_dl;		/* Main display list */

/* Virtual memory pointers and size */
extern unsigned vm_base;		/* Base addr. of virtual memory area */
extern unsigned vm_size;		/* Size of virtual memory (in bytes) */
extern unsigned *vm;			/* Pointer to next available VM */

/* Pointers to Hawk flag words */
extern unsigned *vcom_host_s;		/* Flag: Host to FE (signal handler) */
extern unsigned *vcom_host_u;		/* Flag: Host to FE (user) */

/* Pointer to communication message control block */
extern Hkvc_umcb *user_mcb_ptr;		/* User MCB */


/* From demo_templates.c */
extern dl_data dl_data_template;
extern dial_data rot_template;
extern dial_data rot_speed_template;
extern dial_data trans_template;
extern dial_data scale_template;
extern dial_data persp_template;
extern dial_data color_template;
extern dial_data exp_template;
extern dial_data dc_range_template;
extern dial_data coeff_template;
extern dial_data window_x_template;
extern dial_data window_y_template;
extern dial_data window_width_template;
extern dial_data window_height_template;
/* Note: This is larger than required.  If it becomes too small, enlarge it */
#define MAIN_LOOP_SIZE 500
extern unsigned main_loop[500];



/* External procedures */

/* demo_dials.c */
extern void init_dials();
extern void get_dial();
extern void constrain_input();
extern void constrain_window_x();
extern void constrain_window_y();
extern void constrain_window_width();
extern void constrain_window_height();

/* demo_dl_create.c */
extern void build_main_loop();
extern void build_dl0();
extern int build_dl();
extern void init_environment();

/* demo_mat_util.c */
extern void identity();
extern void concat_mat();
extern void copy_mat();
extern void scale_xyz();
extern void translate();
extern void rot_x();
extern void rot_y();
extern void rot_z();
extern int invert();

/* demo_hk_comm.c */
extern void init_hawk();
extern void init_screen();
extern void wait_for_hawk_ready();
extern void start_hawk();
extern int hawk_stopped();
extern void user_signal_handler();

/* demo_main.c */
extern void intr_handler();
extern void init_args();
extern void prog_exit();

/* demo_menus.c */
extern void display_main_screen();
extern void display_help_menu();
extern void display_menu();
extern void display_exit_message();
extern void display_data();
extern void wait_for_space();

/* demo_rubber_band.c */
extern void init_overlay();
extern void draw_box();

/* demo_user_interf.c */
extern void execute_dl();
extern void draw_one_frame();
extern void select_object();
extern void exit_user_interf();
extern void push_menu();
extern void pop_menu();
extern void check_select();

/* demo_transforms.c */
extern void get_transforms();
extern void display_transforms();
extern void get_rot();
extern void display_rot();
extern void update_x_rot();
extern void update_y_rot();
extern void update_z_rot();
extern void get_scale();
extern void display_scale();
extern void update_scale();
extern void get_trans();
extern void display_trans();
extern void update_trans();
extern void get_persp();
extern void display_persp();
extern void update_persp();
extern void get_axis();
extern void display_axis();
extern void update_x_rot_axis();
extern void update_y_rot_axis();
extern void update_z_rot_axis();
extern void update_rot_speed();
extern void reset_scale();
extern void reset_persp();
extern void reset_trans();
extern void reset_rot();
extern void reset_axis();

/* demo_colors.c */
extern void get_bg_color();
extern void display_bg_color();
extern void update_bg_color();
extern void get_line_color();
extern void display_line_color();
extern void update_obj_color();
extern void get_surf_props();
extern void display_surf_prop();
extern void update_surf_prop();
extern void get_depth_cue();
extern void display_depth_cue();
extern void update_depth_cue();

/* demo_windows.c */
extern void get_window();
extern void display_window();
extern void update_window();
extern void fix_window_aspect();

/* demo_obj_control.c */
extern void get_main_loop();
extern void display_main_menu();
extern void get_object();
extern void display_object();
extern void get_filename();
extern void display_filename();
extern void get_keyboard_toggle();
extern void display_keyboard_toggle();

/* End of demo.h */
#endif _DEMO_
