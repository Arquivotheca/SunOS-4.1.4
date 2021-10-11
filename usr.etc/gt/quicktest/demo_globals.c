#ifndef lint
static char sccsid[] = "@(#)demo_globals.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_globals.c
 *
 *	@(#)demo_globals.c 1.26 91/06/27 15:23:40
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	Data structures, constants, external procedure definitions
 *	and global variables for the Hawk demo program.
 *
 * 24-May-90 Scott R. Nelson	  Initial file.
 * 12-Jul-90 Scott R. Nelson	  Made the dials optional.
 * 19-Jul-90 Scott R. Nelson	  Added display list data field, changed a
 *				  few modes.
 * 23-Jul-90 Scott R. Nelson	  Added rotation axis, stack.
 * 20-Aug-90 Scott R. Nelson	  Changes for dlxmcb file.
 * 21-Sep-90 Scott R. Nelson	  Move the templates to demo_templates.c
 *  4-Oct-90 Scott R. Nelson	  Added auto_rotate flag.
 * 29-Jan-91 John M. Perry	  Added XNeWS direct graphics access (dga) 
 * 12-Feb-91 John M. Perry	  Added XNeWS display and window
 * 16-Apr-91 Chris Klein          Added SunView capability
 * 18-Apr-91 Kevin C. Rushforth	  Added SUSPEND/RESUME ioctl() calls.
 * 30-May-91 John M. Perry        Add Stereo support for XNeWS QUICKTEST
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include "demo.h"

#include <X11/Xlib.h>

#include "demo_sv.h"

/* Global variables */

char *fesim_argv[MAX_ARGC];		/* Arguments to hk_fesim */
int fesim_argc = 0;			/* Number of arguments to hk_fesim */
FILE *input_file = NULL;		/* Input file, if specified */
char input_file_name[256];		/* Name of input file */
int verbose = FALSE;			/* Indicates verbose messages */
int use_dials = 0;			/* Default to not use dials */
int use_cg9_overlay = 1;		/* Assume cg9 present */
char *prog_name;			/* Name of program for error msgs */
int screen_x, screen_y;			/* Size of screen to use */
int input_ch;				/* Last character input from keyboard */

struct fb_vmctl vmctl_suspend = { FBV_SUSPEND };
struct fb_vmctl vmctl_resume = { FBV_RESUME };

WXINFO *dga;				/* XNeWS Direct Graphics Access page*/  
Display *x_display;			/* XNeWS display */  
Window x_win;				/* XNeWS window */  
int	dga_stereo;			/* XNeWS DGA stereo mode */

struct sv_win sv_win;                   /* SunView Window data */
struct  fb_fcsalloc fcs_data;           /* FAST CLEAR SET data  */

int continuous_draw;			/* Don't wait for command to draw */
int auto_rotate;			/* Automatically rotate the object */
u_long stereo;                          /*   Stereo flag  */
#ifdef QUICKTEST
int Autocycle = 1;				/* Automatically cycle through objects */
#endif QUICKTEST

int done_with_program = 0;		/* Exit flag */
int no_data = 1;			/* Data input flag */

/* Menus and input routines get stacked to return to previous levels */
int menu_sp;				/* Menu stack pointer */
void (*display_current_screen[MS_MAX])(); /* Procedure to update the screen */
void (*get_input[MS_MAX])();		/* Procedure to get input */
dial_array cur_dials;			/* Current dial records */
dial_array dial_stack[MS_MAX];		/* Dial data stack */

int db_fd;				/* Dial box file descriptor */

dl_env *dle_ptr;			/* Display list environment pointer */
int dl;					/* Which display list is current */

dl_env dle[10];				/* The various display list environs */

unsigned main_dl;			/* Main display list */

/* Virtual memory pointers and size */
unsigned vm_base;			/* Base addr. of virtual memory area */
unsigned vm_size;			/* Size of virtual memory (in bytes) */
unsigned *vm;				/* Pointer to next available VM */

/* Pointers to Hawk flag words */
unsigned *vcom_host_s;			/* Flag: Host to FE (signal handler) */
unsigned *vcom_host_u;			/* Flag: Host to FE (user) */

/* Pointer to communication message control block */
Hkvc_umcb *user_mcb_ptr;

/* End of demo_globals.c */
