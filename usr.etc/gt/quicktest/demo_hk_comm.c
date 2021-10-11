#ifndef lint
static char sccsid[] = "@(#)demo_hk_comm.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_hk_comm.c
 *
 *	@(#)demo_hk_comm.c 1.35 91/06/27 15:23:42
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the routines that communicate with
 *	the Hawk hardware.
 *
 *  3-Apr-90 Scott R. Nelson	  Initial version.
 *  9-May-90 Scott R. Nelson	  Integrated this module with the other code.
 * 19-Jul-90 Scott R. Nelson	  Added comments, minor tuning.
 * 20-Aug-90 Scott R. Nelson	  Changes for gtmcb file and hk_connect.
 * 29-Jan-91 John M. Perry	  XNeWS client DGA (direct graphics access) 
 * 12-Feb-91 John M. Perry	  XNeWS display and window now globals 
 * 22-Feb-91 John M. Perry	  XNeWS add window & icon title
 * 16-Apr-91 Chris Klein          Added SunView capability
 * 16-Apr-91 Kevin C. Rushforth	  DLX mode is now default.
 * 18-Apr-91 Kevin C. Rushforth	  Added SUSPEND/RESUME ioctl() calls.
 * 16-May-91 John M. Perry	  Convert from Xlib from to XView
 * 29-May-91 John M. Perry	  Add XGrabStereo call
 * 30-May-91 John M. Perry	  Add Stereo support for XNeWS QUICKTEST 
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>	/*   pr_ioctl   */
#include <pixrect/gt_fbi.h>	/*   HKPP_FBW_960STEREO   */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <fcntl.h>
#include <sys/time.h>

#include "hk_comm.h"
#include "demo.h"
#include "demo_sv.h"

/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	init_hawk
 *	    getenv
 *	    system
 *	    hk_open
 *	    hk_mmap
 *	    signal
 *	    hk_connect
 *	init_screen
 *	    hk_init_default_context
 *	    XMatchVisualInfo
 *	    cr_x_window
 *          DefaultVisual
 *          DefaultColormap
 *	    XCreateColormap
 *	    BlackPixel
 *	    WhitePixel
 *	    XCreateWindow
 *	    XStringListToTextProperty
 *	    XSetWMName
 *	    XSetIconName
 *	    XMapWindow
 *	    XSync
 *	    XGrabNewWindow
 *	    wx_grab
 *	    XGrabWids
 *	    XGrabZbuf
 *	    XGrabBuffers
 *	    XGrabStereo
 *          win_screenget
 *          cr_sv_frame
 *          set_sv_wb
 *          pr_ioctl(...FBIO_WID_ALLOC...)
 *          pr_ioctl(...FB_FCSALLOC...)
 *          sv_win_show
 *	    wait_for_hawk_ready
 *	wait_for_hawk_ready
 *	start_hawk
 *	hawk_stopped
 *	user_signal_handler
 *	    prog_exit
 *	    sleep
 *
 *==============================================================
 */



/*
 *--------------------------------------------------------------
 *
 * init_hawk
 *
 *	Code to initialize everything to use Hawk.
 *
 *--------------------------------------------------------------
 */

void
init_hawk()
{
    int child_pid;			/* Process ID of child */
    unsigned *ptr;			/* Memory pointer */
    char *name;				/* Value of HK_COMM_MODE env variable */
    char *tmpstr;			/* Temporary string */
    char *exec_string;			/* Execution string */
    char exec_buffer[256];		/* Execution string build area */
    struct gt_connect conn;		/* Struct used by hk_connect */
    int i;				/* Iterative counter */
    extern char *getenv();

/* Now do the Hawk stuff */

    /* Set up the default program to launch */
    if ((name = getenv("HK_COMM_MODE")) == NULL)
	name = "DLX";

    if (strcmp(name, "SIM") == 0) {
	exec_string =
	    "hk_fesim -sock | hk_su | hk_ew | hk_si | hk_fb -sock | hk_display";
    }
    else if (strcmp(name, "DLX") == 0) {
	exec_string = NULL;
    }
    else {
	fprintf(stderr, "%s: I don't understand HK_COMM_MODE=%s\n",
	    prog_name, name);
	exit (1);
    }

    if ((tmpstr = getenv("HK_SIM_EXEC")) != NULL) {
	exec_string = tmpstr;
    }

    if (exec_string != NULL) {
	sprintf(exec_buffer, "(%s)&", exec_string);
	if( system(exec_buffer) != 0 ) {
		fprintf( stderr, "demo: Could not find programs to run \"%s\"\n", exec_string);
		prog_exit(1);
	} else	
		fprintf(stderr, "demo: launched %s\n", exec_buffer);
    }

/* Open Hawk (get the kernel connections) */

    if (hk_open() < 0) {
	fprintf(stderr, "%s: ", prog_name);
	perror("hk_open failed");
	exit(1);
    }

/* Allocate the required virtual memory */

    /* vm_size must include everything we put in the memory */
    vm_base = (unsigned) hk_mmap((char *) 0, vm_size);
    if (vm_base == NULL) {
	fprintf(stderr, "%s: hk_mmap failed.\n", prog_name);
	exit(1);
    }
    vm = (unsigned *) vm_base;

/* Set up an MCB */

    user_mcb_ptr = (Hkvc_umcb *) vm;
    vm += sizeof(Hkvc_umcb) / sizeof(unsigned);

    ptr = (unsigned *) user_mcb_ptr;
    for (i = 0; i < (sizeof(Hkvc_umcb) / sizeof(unsigned)); i++)
	*ptr++ = 0;			/* Clear out all words */
    user_mcb_ptr->magic = HK_UMCB_MAGIC;
    user_mcb_ptr->version = HK_UMCB_VERSION;

/* Set up a signal handler (must come before hk_connect) */

    signal(SIGXCPU, user_signal_handler);

/* Get the MCB connected to Hawk */

    conn.gt_pmsgblk = user_mcb_ptr;
    if (hk_connect(&conn) < 0) {
	fprintf(stderr, "%s: hk_connect failed.\n", prog_name);
	exit(1);
    }
    vcom_host_u = conn.gt_puvcr;
    vcom_host_s = conn.gt_psvcr;

} /* End of init_hawk */



/*
 *--------------------------------------------------------------
 *
 * init_screen
 *
 *	Get the screen cleared and set up the simulator to use it;
 *
 *--------------------------------------------------------------
 */

void
init_screen(argc, argv)
int argc;
char **argv;
{
    Hk_context *ctx;			/* Context for initialization */
    void hk_init_default_context();	/* Context init routine */
    Hk_window_boundary *wb;		/* Window bounds area */

    int screen;				/* X screen */
    int cookie;				/* cookie for XNeWS DGA connection */
    int rval;				/* return status from X... calls */
    Colormap cmap;			/* X color map */
    Visual *vis;			/* X visual */
    XVisualInfo vinfo_ret;		/* X visual information */
    XTextProperty win_name;
    char* win_name_str = "GT QuickTest";
    WXCLIENT *cinfop;

    GT_SUSPEND();

    ctx = (Hk_context *) vm;		/* Allocate a context */
    vm += sizeof(Hk_context) / sizeof(unsigned);
    hk_init_default_context(ctx);	/* Clear it out */

/* Make that little window boundary data area */
    wb = (Hk_window_boundary *) vm;
    vm += sizeof(Hk_window_boundary) / sizeof(unsigned);

/*
 * If in a X-window system, find the 24-bit visual. 
 */
    dga = NULL;
    x_display = XOpenDisplay(NULL);

    if(x_display) {
	screen = DefaultScreen(x_display);
	rval = XMatchVisualInfo(x_display, screen, 24, TrueColor, &vinfo_ret); 
	if (!rval) {
	    fprintf(stderr, "24-bit visual not found.\n");
	    fprintf(stderr, "Will try to running outside of window system.\n");
	    XCloseDisplay(x_display);
	    sleep(5);
	}
    } else {
	struct screen sv_screen;    /* SunView Display   */
	char *sv_win_dev;
	rval = 0;
	
	/* No X server, try SunView */
	sv_win_dev = getenv( "WINDOW_ME" );	
	if( sv_win_dev )  {      /*   Looks like SunView tool ran me   */
	    if( (sv_win.wid = open( sv_win_dev, O_RDONLY )) == -1) {
		perror(sv_win_dev);
	    } else {
		win_screenget( sv_win.wid, &sv_screen );  close( sv_win.wid );
		if(( sv_win.wid = open( sv_screen.scr_fbname, O_RDWR )) == -1 )
			perror("sv_screen.scr_fbname");
		else {
		    struct fbtype fb;
	    
		    if( ioctl( sv_win.wid, FBIOGTYPE, &fb ) == -1 )
		    	perror("on sv_screen.scr_fbname");
		    if( ioctl( sv_win.wid, FB_GETMONITOR, &stereo ) == -1 )
		    	perror("on sv_screen.scr_fbname");
		    close( sv_win.wid );
		    if( fb.fb_type != FBTYPE_SUNGT )	 {
		    	sv_win.wid = 0;
			fprintf(stderr, "SunView not on GT device\n");
			fprintf(stderr, "Will try running directly on device.\n");
	    	    }
		}
	    }
	}
    }

/*
 * If 24-bit visual was found, create a window and setup a DGA page.
 */

    if (rval) {
#ifdef XVIEW 
	x_win = cr_x_window( win_name_str, argc, argv, 
			     DEF_WIN_WIDTH, DEF_WIN_HEIGHT );
#else
#ifdef QUICKTEST
	fprintf(stderr, "SV version of quicktest will not run in OW.\n");
	exit(1);
#else
	XSetWindowAttributes attributes;
        int black, white;
 
        vis = vinfo_ret.visual;
 
        if (vis == DefaultVisual(x_display, screen))
            cmap = DefaultColormap(x_display, screen);
        else
            cmap = XCreateColormap( x_display,
                                DefaultRootWindow(x_display), vis, AllocNone );
 
        black = BlackPixel(x_display, screen);
        white = WhitePixel(x_display, screen);
 
        attributes.background_pixel = black;
        attributes.border_pixel = black;
        attributes.colormap = cmap;
 
        x_win = XCreateWindow(x_display, DefaultRootWindow(x_display),
                100,100, DEF_WIN_WIDTH, DEF_WIN_HEIGHT, 5, 24, CopyFromParent, vis,               
                CWBackPixel|CWBorderPixel|CWColormap, &attributes);
 
        XStringListToTextProperty(&win_name_str, 1, &win_name);
        XSetWMName(x_display, x_win, win_name);
        XSetIconName(x_display, x_win, "GT Demo");
 
        XMapWindow(x_display, x_win);
        XSync(x_display, TRUE);
#endif QUICKTEST
#endif XVIEW 

	cookie = XGrabWindowNew(x_display, x_win, 1);

	if (cookie) {
	   cinfop = wx_grab(-1, cookie);
	   dga = wx_infop(cinfop);
	}
	else {
	   XDestroyWindow(x_display, x_win);
	   XCloseDisplay(x_display);
	}
    } else if ( sv_win.wid ) {
#ifdef XVIEW    	
	fprintf(stderr, "OW version of quicktest will not run in SV.\n");
	exit(1);
#else
    	cr_sv_frame();
#endif XVIEW
  
    }
    	
    if (dga) {
    	wb = (Hk_window_boundary *) &(dga->w_window_boundary);
    }
    else if (sv_win.wid) {
        set_sv_wb();
    } else {
        wb->xleft = 0;
        wb->ytop = 0;
        wb->width = screen_x;
        wb->height = screen_y;
    }


/* Prepare to clear the full screen and draw a dot on it */

    ctx->dpc = (unsigned) vm;		/* Point at display list */
    ctx->s.marker_geom_format = HK_LINE_XYZ;
    ctx->s.hki_marker_color.r = BG_R;
    ctx->s.hki_marker_color.g = BG_G;
    ctx->s.hki_marker_color.b = BG_B;
    ctx->ptr_window_boundary = wb;
    if (wb->height > wb->width)
	ctx->s.view.vt[HKM_1_1] = (float) wb->width / (float) wb->height;
    else
	ctx->s.view.vt[HKM_0_0] = (float) wb->height / (float) wb->width;
    ctx->window_bg_color.r = BG_R;
    ctx->window_bg_color.g = BG_G;
    ctx->window_bg_color.b = BG_B;
    ctx->s.wid_clip_mask = 0x000;

    if (dga) {
        rval = XGrabWids(x_display, x_win, 1);

	if (!rval) {
	    fprintf(stderr, "Could not allocate WID.\n");
	    prog_exit(1);
	}

	ctx->s.current_wid = dga->w_wid;
        ctx->s.wid_write_mask = 0;

	rval = XGrabZbuf(x_display, x_win, 1);

	if (!rval) {
	    fprintf(stderr, "Could not allocate Z-buffer.\n");
	    prog_exit(1);
	}

	rval = XGrabBuffers(x_display, x_win, 2);

	if (!rval) {
	    fprintf(stderr, "Could not allocate 2nd buffer.\n");
	    prog_exit(1);
	}

	rval = XGrabFCS(x_display, x_win, 1);

	if (!rval) {
	    fprintf(stderr, "Could not allocate FCS.\n");
	    prog_exit(1);
	}

	dga_stereo = XGrabStereo(x_display, x_win, 1);

    } else if ( sv_win.wid ) {   /*  SunView Window System   */
	int     win_fd;
	struct  fb_wid_alloc winfo;          /* see /usr/include/sun/fbio.h  */
        struct  pixwin_clipdata *pwcd;
	struct	pixwin_prlist *prl;

	if( stereo == HKPP_FBW_960STEREO )   /*  Set up for stereo   */
		stereo = FB_WIN_STEREO;
	else
		stereo = ~FB_WIN_STEREO;

	winfo.wa_type = FB_WID_DBL_24;
	winfo.wa_count = 1;
	winfo.wa_index = -1;

	win_fd = sv_win.pw->pw_windowfd;
	pr_ioctl(sv_win.pw->pw_pixrect, FBIOSWINFD, &win_fd);
	if(pr_ioctl(sv_win.pw->pw_pixrect, FBIO_WID_ALLOC, &winfo) == -1) {
	    printf("could not alloc a wid\n");
	    prog_exit(1);
	}
	if(pr_ioctl(sv_win.pw->pw_pixrect, FBIO_STEREO, &stereo) == -1) {
	    printf("could not set stereo mode\n");
	    prog_exit(1);
	}
    
	pr_ioctl(sv_win.pw->pw_clipdata->pwcd_prmulti, FBIOSWINFD, &win_fd);
	pr_ioctl(sv_win.pw->pw_clipdata->pwcd_prmulti, FBIO_STEREO, &stereo);
	pr_ioctl(sv_win.pw->pw_clipdata->pwcd_prmulti, FBIO_ASSIGNWID, &winfo);
	pr_ioctl(sv_win.pw->pw_clipdata->pwcd_prmulti, FBIO_STEREO, &stereo);

	pwcd = sv_win.pw->pw_clipdata;
	for (prl = pwcd->pwcd_prl;prl;prl = prl->prl_next) {
	    pr_ioctl(sv_win.pw->pw_pixrect, FBIOSWINFD, &win_fd);
	    pr_ioctl(prl->prl_pixrect, FBIO_ASSIGNWID, &winfo);
	    pr_ioctl(prl->prl_pixrect, FBIO_STEREO, &stereo);
	}
     
        /*  Allocate Fast Clear Plane   */
	if( pr_ioctl(sv_win.pw->pw_pixrect, FB_FCSALLOC, &fcs_data) == -1) {
	    printf("could not alloc a FCS\n");
	    prog_exit(1);
	}

	/*  and the fast clear plane allocated to you will be in fcs_data.index. 
	    The definition of "struct fb_fcsalloc"  is in  sys/sbusdev/gtreg.h.
	*/  
	     
	sv_win.wid = ctx->s.current_wid = winfo.wa_index;
        ctx->s.wid_write_mask = 0;
       } else {
        ctx->s.current_wid = DEF_WID;
        ctx->s.wid_write_mask = 0x3ff;
    }

    *vm++ = ((HK_OP_POLYMARKER << HK_OP_POS) | 1);
    *vm++ = 0;				/* Note: floating-point same as int */
    *vm++ = 0;
    *vm++ = 0;
    *vm++ = (HK_OP_WINDOW_CLEAR << HK_OP_POS);

    *vm++ = ((HK_OP_UPDATE_LUT << HK_OP_POS) | (HK_UPDATE_WLUT << HK_SO_POS) |
	(HK_AM_IMMEDIATE << HK_AM_POS));

    if (dga) {
        *vm++ = dga->w_wid;		/* Entry */
    	*vm++ = 0x3f;			/* Mask */
    } else if( sv_win.wid ) {
    	*vm++ = sv_win.wid;		/* Entry */
    	*vm++ = 0x1f;			/* Mask */
    } else {
    	*vm++ = DEF_WID;		/* Entry */
    	*vm++ = 0x1f;			/* Mask */
    }

    *vm++ = HK_BUFFER_A;		/* Image buffer */
    *vm++ = HK_BUFFER_A;		/* Overlay buffer */
    *vm++ = HK_24BIT;			/* Plane_group */
    *vm++ = DEF_CLUT;			/* CLUT */
    *vm++ = 4;				/* Fast clear set */

    *vm++ = (HK_OP_FLUSH_RENDERING_PIPE << HK_OP_POS);
    *vm++ = (HK_OP_FLUSH_CONTEXT << HK_OP_POS);
    *vm++ = (HK_OP_WAIT << HK_OP_POS);

    GT_RESUME();

    if ( sv_win.wid ) sv_win_show();

/* Cause Hawk to go once */

    wait_for_hawk_ready();

    GT_SUSPEND();
    user_mcb_ptr->context = (unsigned *) ctx;
    user_mcb_ptr->command = HKUVC_LOAD_CONTEXT;	/* Don't pause == go */
    GT_RESUME();

    *vcom_host_u = 1;
    while (*vcom_host_u == 1)
	;				/* Make sure Hawk is ready */

} /* End of init_screen */



/*
 *--------------------------------------------------------------
 *
 * wait_for_hawk_ready
 *
 *--------------------------------------------------------------
 */

void
wait_for_hawk_ready()
{
    while (*vcom_host_u == 1)
	;				/* Make sure Hawk is ready */
    while (!user_mcb_ptr->gt_stopped)
	;				/* Make sure it is stopped */
} /* End of wait_for_hawk_ready */



/*
 *--------------------------------------------------------------
 *
 * start_hawk
 *
 *	Set the DPC to point to the main display list loop, point
 *	R5 at the object to draw, and point R4 at the data area.
 *	Then set up the MCB to run and start Hawk.
 *
 *--------------------------------------------------------------
 */

void
start_hawk()
{
    GT_SUSPEND();
    dle_ptr->ctx->dpc = dle_ptr->dl_begin;
    dle_ptr->ctx->risc_regs[HK_RISC_R5] = dle_ptr->obj_begin;
    dle_ptr->ctx->risc_regs[HK_RISC_R4] = (int) dle_ptr->p_dl_data;

    user_mcb_ptr->context = (unsigned *) dle_ptr->ctx;
    user_mcb_ptr->command = HKUVC_LOAD_CONTEXT;	/* Don't pause == go */
    GT_RESUME();

    *vcom_host_u = 1;
} /* End of start_hawk */



/*
 *--------------------------------------------------------------
 *
 * hawk_stopped
 *
 *	Return true if Hawk is stopped with no requests pending.
 *
 *--------------------------------------------------------------
 */

int
hawk_stopped()
{
    if ((*vcom_host_u == 1) || (!user_mcb_ptr->gt_stopped))
	return (0);
    else
	return (1);
} /* End of hawk_stopped */



/*
 *--------------------------------------------------------------
 *
 * user_signal_handler
 *
 *	This represents the code a user would need for handling
 *	a signal caused by an interrupt from Hawk.
 *
 *--------------------------------------------------------------
 */

void
user_signal_handler()
{
    unsigned status;
    unsigned dpc;
    unsigned error;
    char *err_string;
    extern char *hk_error_string();

    status = user_mcb_ptr->status;
    dpc = user_mcb_ptr->dpc;
    error = user_mcb_ptr->errorcode;

    if (verbose)
	fprintf(stderr,
	    "%s: User interrupt received, cmd: 0x%.8X dpc: 0x%.8x err: 0x%.8X\n",
	    prog_name, status, dpc, error);

    if (status & HKUVS_ERROR_CONDITION) {

	err_string = hk_error_string(error);
	fprintf(stderr, "%s: Error: %d (%s) at: 0x%.8X\n",
	    prog_name, error, err_string, dpc);
	if ((error != HKERR_ILLEGAL_OP_CODE)
	    && (error != HKERR_ILLEGAL_ATTRIBUTE))
		prog_exit(1);
    }
    if (status & HKUVS_INSTRUCTION_TRAP) {

	/* If "trap 0", exit, else continue */

	if (user_mcb_ptr->trap_instruction == (HK_OP_TRAP << HK_OP_POS))
	    prog_exit(0);
    }
    else if (status & HKUVS_DONE_WITH_REQUEST) {
	/* Now what did I ask it to do?  (return from interrupt) */
    }

/* Tell it to "return from interrupt" */

    user_mcb_ptr->command = 0;		/* Implied "go" */
    *vcom_host_s = 1;			/* Restart Front End processor */
} /* End of user_signal_handler */

/* End of demo_hk_comm.c */
