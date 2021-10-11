#ifndef _DEMO_SV_
#define _DEMO_SV_

/*
 ***********************************************************************
 *
 * demo_sv.h
 *
 *      @(#)demo_sv.h  1.1 94/10/31  SMI
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	Data structures, constants, external procedure definitions
 *	and global variables for the Hawk demo program running in SunView.
 *      Cannot include window.h because the typedef window is also in Xlib.h
 *      All functions needing window.h are placed in demo_sv.c
 *
 * 13-Apr-91 Chris Klein	  Initial file.
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <sys/types.h> 
#include <sys/time.h> 

#include "hk_public.h"

/*  Cannot include window.h   */
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/pixwin.h>
#include <suntool/frame.h>
#include <suntool/canvas.h>
#include <suntool/panel.h>

#define DEF_WIN_HEIGHT 600
#define DEF_WIN_WIDTH 740

/* Structure for holding SunView screen data  */
struct sv_win {
     int   wid;               /*  Window ID */
     Frame frame;             /*  Frame object */
     Canvas canvas;           /*  Canvas object */
     Panel panel;             /*  Canvas object */
     Pixwin *pw;              /*  Canvas pixwin */
     Hk_window_boundary *cur_wb_ptr;  /*  HAWK window bounds pointer */
};

extern struct sv_win sv_win;         /* SunView window */  
extern int done_with_program;
extern int Autocycle;
extern struct  fb_fcsalloc	fcs_data;
extern u_long stereo;

extern void get_sv_wb();
extern void cr_sv_win();

#endif _DEMO_SV_
