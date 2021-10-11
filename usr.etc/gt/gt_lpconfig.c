#ifndef lint
static char sccsid[] = "@(#)gt_lpconfig.c 1.1 94/10/31 Copyr 3991 Sun Micro";
#endif

/*
 ****************************************************************
 *
 * Lightpen Calibration Utility for GT.
 *
 *      @(#)gt_lpconfig.c 1.1 94/10/31 21:04:39
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 */

#include <stdio.h>
#include <fcntl.h>


#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_rainbow.h>
#include <sunwindow/win_input.h>
#include <suntool/textsw.h>
#include <sys/ioctl.h>

#include <gtreg.h>
#include <lightpenreg.h>

#include <math.h>

#define FBNAME "/dev/gt0"
short tool_image[256] =
{
#include "gt_lpconfig_icon.h"
};

DEFINE_ICON_FROM_IMAGE(tool_icon, tool_image);

#define PIXEL_TOLERANCE 0
#define TIME_UNDETECT 50000

#define BGCLR_VAL 200
#define COLOUR	5			/* Blue */
#define BGCLR	0			/* Must be 0 */
#define CALIBRATE_BG_COLOUR 0		/* 1 = Black */
#define BLACK	1

#define CURSOR_X 200
#define CURSOR_Y 200

static int Delta_x = 0;
static int Delta_y = 0;
static int Calibrated = FALSE;

Window frame, panel, canvas, help_frame, help_sw;

Pixwin *lightpen_pixwin;

static void lightpen_event_proc(), quit_notify(), Calibrated_notify();
static void draw_b();
static void draw_c();
static void enable_lightpen();
static void setup_lightpen_param();
static void setupcolormap();
static void modify_samples();
static void modify_tolerance();
static void Lightpen_help();
static void done_proc();

static int win_fd;
static Panel_item tolerance;
static Panel_item samples;
static unsigned current_tolerance;
static unsigned current_samples;
static unsigned saved_tolerance = 0;
static unsigned saved_samples = 1;
static int lightpen_fd;


static lightpen_filter_t lf;
static lightpen_calibrate_t lc;

/*
 * main entry.
 */
main(argc, argv)
    int argc;
    char **argv;
{
    if ((frame = window_create(NULL, FRAME,
	FRAME_LABEL, "GT Lightpen Calibration Tool",
	FRAME_ICON, &tool_icon,
	0)) == NULL) {
	fprintf(stderr, "gt_lpconfig: This program has to be run under a Window sytem\n");
	exit(1);
    }
	;
    panel = window_create(frame, PANEL, 0);

    samples =
	panel_create_item(panel, PANEL_SLIDER,
	PANEL_LABEL_STRING, "Samples ",
	PANEL_SLIDER_WIDTH, 200,
	PANEL_MIN_VALUE, 0,
	PANEL_MAX_VALUE, 128,
	PANEL_NOTIFY_PROC, modify_samples,
	0);


    tolerance =
	panel_create_item(panel, PANEL_SLIDER,
	PANEL_LABEL_STRING, "Tolerance",
	PANEL_SLIDER_WIDTH, 200,
	PANEL_MIN_VALUE, 0,
	PANEL_MAX_VALUE, 512,
	PANEL_NOTIFY_PROC, modify_tolerance,
	0);

    panel_create_item(panel, PANEL_BUTTON, PANEL_LABEL_IMAGE,
	panel_button_image(panel, "Calibrate", 10, 0),
	PANEL_ITEM_X, ATTR_COL(58),
	PANEL_ITEM_Y, ATTR_ROW(0) + 5,
	PANEL_NOTIFY_PROC,
	Calibrated_notify,
	0);

    panel_create_item(panel, PANEL_BUTTON, PANEL_LABEL_IMAGE,
	panel_button_image(panel, "Quit", 5, 0),
	PANEL_ITEM_X, ATTR_COL(58),
	PANEL_ITEM_Y, ATTR_ROW(1) + 10,
	PANEL_NOTIFY_PROC,
	quit_notify,
	0);


    panel_create_item(panel, PANEL_BUTTON, PANEL_LABEL_IMAGE,
	panel_button_image(panel, "Help", 5, 0),
	PANEL_ITEM_X, ATTR_COL(72),
	PANEL_ITEM_Y, ATTR_ROW(0) + 5,
	PANEL_NOTIFY_PROC,
	Lightpen_help,
	0);



    window_fit_height(panel);
    win_fd = (int)window_get(frame, WIN_FD);
    if ((lightpen_fd = open("/dev/lightpen", O_RDWR)) == -1)  {
        perror("Open failed for /dev/lightpen:");
        exit(1);
    }
    win_set_input_device(win_fd, lightpen_fd, "/dev/lightpen");

    canvas = window_create(frame, CANVAS,
	CANVAS_WIDTH, 1280,
	CANVAS_HEIGHT, 1024,
	WIN_HEIGHT, 1024,
	WIN_WIDTH, 1280,
	CANVAS_AUTO_CLEAR, TRUE,
	CANVAS_RETAINED, TRUE,
	WIN_EVENT_PROC, lightpen_event_proc,
	0);
    lightpen_pixwin = canvas_pixwin(canvas);

    window_fit(canvas);
    window_fit(frame);

    enable_lightpen(1);
    setup_lightpen_param();
    setupcolormap(lightpen_pixwin);
    draw_b(BGCLR);

    /* Initialize */
    saved_tolerance = 0;
    saved_samples = 1;
    current_tolerance = saved_tolerance;
    current_samples = saved_samples;

    window_main_loop(frame);
}

short oldx, oldy;
short loc_x, loc_y;


/*
 * Receive lightpen events here from the notifier.
 */
static void
lightpen_event_proc(window, event, arg)
    Window window;
    Event *event;
    caddr_t arg;
{

    int event_code;
    short but_state;
    int window_x_origin;
    int window_y_origin;
    int value;
    int button_state;

    if (event_is_lightpen(event)) {
	event_code = LIGHTPEN_EVENT(event->ie_code);
	switch (event_code) {
	case LIGHTPEN_MOVE:
	    if (Calibrated) {
		value = (int) window_get(canvas, WIN_EVENT_STATE, 
				LIGHTPEN_VUID_MOVE);
		loc_x = GET_LIGHTPEN_X(value);
                loc_y = GET_LIGHTPEN_Y(value);

		win_getscreenposition(lightpen_pixwin->pw_clipdata->pwcd_windowfd,
		    &window_x_origin, &window_y_origin);

		loc_x -= window_x_origin;
		loc_y -= window_y_origin;

		/* Undraw previous cursor and draw new cursor */
		if (oldx || oldy)
		    draw_c(oldx, oldy, COLOUR);
		draw_c(loc_x, loc_y, COLOUR);
		oldx = loc_x;
		oldy = loc_y;
	    }
	    break;

	case LIGHTPEN_DRAG:
	    if (Calibrated) {
		value = (int) window_get(canvas, WIN_EVENT_STATE, 
			LIGHTPEN_VUID_DRAG);
		loc_x = GET_LIGHTPEN_X(value);
                loc_y = GET_LIGHTPEN_Y(value);

		win_getscreenposition(lightpen_pixwin->pw_clipdata->pwcd_windowfd,
		    &window_x_origin, &window_y_origin);

		loc_x -= window_x_origin;
		loc_y -= window_y_origin;

		/* Undraw previous cursor and draw new cursor */
		if (oldx || oldy)
		    draw_c(oldx, oldy, COLOUR);
		draw_c(loc_x, loc_y, COLOUR);
		oldx = loc_x;
		oldy = loc_y;
	    }
	    break;
	case LIGHTPEN_BUTTON:
	    button_state = (int) window_get(canvas, WIN_EVENT_STATE,
			 LIGHTPEN_VUID_BUTTON);
	
	    if (!Calibrated && (button_state == LIGHTPEN_BUTTON_DOWN)) {
	    	value = (int) window_get(canvas, WIN_EVENT_STATE, 
			LIGHTPEN_VUID_MOVE);
		loc_x = GET_LIGHTPEN_X(value);
		loc_y = GET_LIGHTPEN_Y(value);

		win_getscreenposition(lightpen_pixwin->pw_clipdata->pwcd_windowfd,
		    &window_x_origin, &window_y_origin);

		Delta_x = CURSOR_X + window_x_origin - loc_x;
		Delta_y = CURSOR_Y + window_y_origin - loc_y;
		/* issue ioctl for offsets */
                lc.x = Delta_x;
                lc.y = Delta_y;
                if ((ioctl(lightpen_fd, LIGHTPEN_CALIBRATE, &lc) == -1)) {
		    perror("LIGHTPEN_CALIBRATE2 ioctl failed");
		    exit(1);
		}

		Calibrated = TRUE;
		draw_c(CURSOR_X, CURSOR_Y, COLOUR);

		oldx = 0;
		oldy = 0;

		/* Issue ioctl with restored samples and tolerance */
		current_samples =  saved_samples;
		current_tolerance =  saved_tolerance;

		/* Issue ioctls for samples = 1; tolerance = delta_x = delta_y = 0 */
		lf.flag = 0;        /* Required */
		lf.tolerance = (unsigned) current_tolerance;
		lf.count = (unsigned) current_samples;       /* Number samples */
		if (ioctl(lightpen_fd, LIGHTPEN_FILTER, &lf) == -1) {
		    perror("LIGHTPEN_FILTER2 ioctl failed");
		    exit(1);
		}
	    
	    }
	    break;
	case LIGHTPEN_UNDETECT:	/* @@@@@ */
	    draw_c(oldx, oldy, COLOUR);
	    break;
	}
    }

}


#define LENGTH 20

/*
 * Clears window to specified background color.
 */
static void
draw_b(color)
    int color;
{
    short y;

    for (y = 0; y < 1023; y++)
	pw_vector(lightpen_pixwin, 0, y, 1279, y, PIX_SRC, color);
}

/*
 * Draw/Undraw cursor.
 */
static void
draw_c(x, y, color)
    short x, y;
    int color;
{
    short left_x;
    short right_x;
    short bot_y;
    short top_y;

    left_x = x - LENGTH;
    right_x = x + LENGTH;
    bot_y = y + LENGTH;
    top_y = y - LENGTH;

    pw_vector(lightpen_pixwin, left_x, y, right_x,
	y, (PIX_SRC ^ PIX_DST), COLOUR);
    pw_vector(lightpen_pixwin, x, top_y, x,
	bot_y, (PIX_SRC ^ PIX_DST), COLOUR);
}

/*
 * Process this when Quit button is selected.
 */
static void
quit_notify()
{
    enable_lightpen(0);
    exit(0);
}

/*
 * Process this when Calibrate button is selected.
 */ 
static void
Calibrated_notify()
{
    short y;

    Calibrated = FALSE;
    saved_samples = (unsigned) current_samples;
    saved_tolerance = (unsigned) current_tolerance;

    /* Issue ioctls for samples = 1; tolerance = delta_x = delta_y = 0 */
    lf.flag = 0;	/* Required */
    lf.tolerance = 0;
    lf.count = 1;	/* Number samples */
    if (ioctl(lightpen_fd, LIGHTPEN_FILTER, &lf) == -1) {
	perror("LIGHTPEN_FILTER3 ioctl failed");
	exit(1);
    }

    lc.x = lc.y = 0;	/* Offsets */
    if (ioctl(lightpen_fd, LIGHTPEN_CALIBRATE, &lc) == -1) {
	perror("LIGHTPEN_CALIBRATE3 ioctl failed");
	exit(1);
    }

    /* Clear canvas and draw_cursor*/
    draw_b(BGCLR);
    draw_c(CURSOR_X, CURSOR_Y, COLOUR);
    oldx = CURSOR_X;
    oldy = CURSOR_Y;
}

/*
 * Enables/Disables lightpen.
 */
static void
enable_lightpen(flag)
    int flag;
{

    int framebuffer_fd;

    if ((framebuffer_fd = open(FBNAME, O_RDWR)) == -1) {
	perror("Open failed for framebuffer:");
	exit(1);
    }
    if (ioctl(framebuffer_fd, FB_LIGHTPENENABLE, &flag) < 0) {
	perror("FB_LIGHTPENENABLE");
	exit(1);
    }
}


/*
 * Sets up initial parameters.
 */
static void
setup_lightpen_param()
{
    int framebuffer_fd;
    struct fb_lightpenparam param;

    if ((framebuffer_fd = open(FBNAME, O_RDWR)) == -1) {
	perror("Open failed for framebuffer:");
	exit(1);
    }

    param.pen_distance = PIXEL_TOLERANCE;
    param.pen_time = TIME_UNDETECT;
    if (ioctl(framebuffer_fd, FB_SETLIGHTPENPARAM, &param) < 0) {
	perror("FB_LIGHTPENENABLE");
	exit(1);
    }
}


/*
 * Sets up color table.
 */
static void
setupcolormap(pw)
    Pixwin *pw;
{
    u_char red[CMS_RAINBOWSIZE], green[CMS_RAINBOWSIZE], blue[CMS_RAINBOWSIZE];

    /* Initialize to rainbow cms. */
    (void) pw_setcmsname(pw, CMS_RAINBOW);
    cms_rainbowsetup(red, green, blue);
    red[0] = green[0] = blue[0] = BGCLR_VAL;
    red[BLACK] = green[BLACK] = blue[BLACK] = 0;
    (void) pw_putcolormap(pw, 0, CMS_RAINBOWSIZE, red, green, blue);
}

/*
 * Responds to the Samples slider.
 */
static void
modify_samples(item, value, event)
    Panel_item 	item;
    int 	value;
    Event	*event;
{
    current_samples = value;

    /* Issue ioctl here */
    lf.flag = 0;
    lf.tolerance = (unsigned) current_tolerance;
    lf.count = (unsigned) current_samples;

    if (lf.count == 0) {
	lf.count = 1;
	current_samples =  (unsigned) lf.count;
    }

    if ((ioctl(lightpen_fd, LIGHTPEN_FILTER, &lf) == -1)) {
	perror("LIGHTPEN_FILTER ioctl failed");
	exit(1);
    }
}


/*
 * Responds to the Tolerance slider.
 */
static void
modify_tolerance(item, value, event)
    Panel_item  item; 
    int         value; 
    Event       *event; 
{
    current_tolerance = value;

    /* Issue ioctl here */
    lf.flag = 0;
    lf.tolerance = (unsigned) current_tolerance;
    lf.count = (unsigned) current_samples;
 
    if (lf.count == 0) {
        lf.count = 1;  
        current_samples =  (unsigned) lf.count;
    }  
 
    if ((ioctl(lightpen_fd, LIGHTPEN_FILTER, &lf) == -1)) {
        perror("LIGHTPEN_FILTER ioctl failed");
        exit(1);
    }  
}

/* * Help function */ static void Lightpen_help() { static unsigned
char help_info[] = {
"                   GT Lightpen Calibration Tool\n\
\n\
This tool is used to set up the calibration and smoothening parameters\n\
in the lightpen driver.  On pressing the Calibrate button, a plus cross\n\
is displyed at a specified location in the canvas. Point to the cursor\n\
intersection point with the lightpen, and press the lightpen button.\n\
This is used to determine the X and Y deltas, with which all future\n\
values measured by the lightpen will be offset to determine their\n\
correct position on the monitor. Now as you move the lightpen on the\n\
screen, it is echoed with a plus cursor.\n\
\n\
The lightpen driver reduces the normal jitter on the screen by averaging\n\
the calibrated values over several samples of light detection. By\n\
adjusting the Samples slider, you select the number of samples for\n\
computing the running average. This averaging is applied only when the\n\
distance between the new and previous locations of the lightpen are\n\
within the tolerance specified, otherwise the running average is\n\
intialized to the new location. This tolerance can be adjusted with\n\
the Tolerance slider.\n\
\n\
After this calibration has been performed, you can observe the behaviour\n\
of the lightpen by moving it across the canvas. And if you are not\n\
satisfied, redo the calibration, starting with the Calibrate button.\n\
\n\
Once the lightpen has been calibrated, the driver maintains the\n\
calibration and smoothening parameters for use by the\n\
application programs.\n"

} ;

        if ( help_frame==NULL)
        {
          help_frame = window_create(frame, FRAME,
                FRAME_DONE_PROC, done_proc, 0) ;
          help_sw = window_create(help_frame, TEXTSW,
                TEXTSW_CONTENTS, help_info,
                TEXTSW_BROWSING, TRUE,
                TEXTSW_DISABLE_CD, TRUE,
                TEXTSW_DISABLE_LOAD, TRUE,
                TEXTSW_FIRST, 0,
                TEXTSW_MENU, window_get(help_frame, WIN_MENU),
                TEXTSW_READ_ONLY, TRUE,
                TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
                0 ) ;
        }
        window_set(help_frame, WIN_SHOW, TRUE, 0 ) ;
}

/*
 * Done on the text window.
 */
static void
done_proc(menu, menu_item)
        Menu    menu ;
        Menu_item       menu_item ;
{
        window_set(help_frame, WIN_SHOW, FALSE, 0 ) ;
}
/* End */
