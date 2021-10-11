#ifndef lint
static char sccsid[] = "@(#)demo_main.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_main.c
 *
 *	@(#)demo_main.c 1.1 94/10/31 21:05:28
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains a "host" program which opens up hawk (using
 *	shared memory), builds a context in VM, builds a display list in VM,
 *	and kick starts the front end simulator -- hk_fesim.
 *
 * 22-Mar-90 Scott R. Nelson	  Taken from hk_vm_sample.c
 * 27-Mar-90 Scott R. Nelson	  Added four separate contexts.
 *  9-May-90 Scott R. Nelson	  Integrated this module with the other code.
 * 21-Jul-90 Scott R. Nelson	  Added comments to help future generations
 *				  modify this code. :-)
 * 27-Sep-90 Scott R. Nelson	  Removed three of the four object init.
 *				  procedure calls.
 *  4-Oct-90 Scott R. Nelson	  Added auto_rotate flag.
 * 11-Oct-90 Scott R. Nelson	  Changes to window bounds instruction.
 * 16-Oct-90 Scott R. Nelson	  Added input script file.
 * 23-Oct-90 Scott R. Nelson	  Updated the instructions.
 * 12-Feb-91 John M. Perry	  add XNeWS cleanup to prog_exit
 * 16-Apr-91 Kevin C. Rushforth	  DLX mode is now default.
 * 09-May-91 Chris Klein          Added QUICKTEST pre-processor define.
 *                                Input is only accepted from a file and
 *                                then all input is ignored
 * 16-May-91 John M. Perry	  Convert from Xlib from to XView
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include "hk_comm.h"
#include "demo.h"


/*
 *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 * Instructions for adding numeric attributes to the display list:
 *
 *	Numeric attributes are those that can be changed by a control
 *	dial and have many possible values.  Examples include matrix
 *	(rotate, translate, scale, perspective), colors, transparency
 *	coefficient, and so forth.
 *
 * demo.h:
 *
 *	Add an entry to the structure "dl_data" representing the
 *	attribute.  Update the offsets defined immediately below
 *	the structure, being careful to follow the existing form
 *	so that all offsets remain correct.  These defines are
 *	required, since a macro representing the structure directly
 *	can't be stored in a field of a word within a table.
 *	Be careful that the VIEW and MATRIX offsets come out right,
 *	since they get aligned to 64 bits.
 *
 *	Add a "dial_data" entry to the structure "dl_env" for the
 *	new attribute.
 *
 *	Any new procedures added to any other file must be declared in
 *	demo.h under the external procedure definitions.  Any new
 *	variables added to demo_globals.c must also be declared here.
 *
 * demo_templates.c:
 *
 *	Add an entry to "dl_data_template" containing a default value
 *	for the attribute.  If the dial handler data structure
 *	templates do not already have a default template, add one to
 *	the file.  Consider carefully whether the dial should wrap
 *	around or clamp.  The only ones that should not set the
 *	ABSOLUTE flag are cumulative operations like rotations that
 *	are concatenated.  Make sure the multiplication factor is
 *	reasonable for the attribute and map the keyboard equivalent
 *	to a reasonable multiple.
 *
 *	Add an attribute entry to the main_loop array using the macro
 *	ATTR_I_R4_OFFSET which will point indirectly through R4 to
 *	the attribute value.  Consider how the attributes are grouped
 *	and if order might be important.
 *
 * demo_*.c
 *
 *	If a current file, like demo_colors.c or demo_transforms.c is
 *	appropriate, add the update routines there.  If you are adding
 *	to an existing menu, this is easy.  For example, when the specular
 *	exponent was added to the surface properties, an ASCII value
 *	had to be chose for the keyboard (X).  An entry to increase
 *	and decrease the value were added.  All of the work is done
 *	by the standard constraint routine and specified update routine.
 *	Add a menu entry, and a dial entry if necessary.  Add entries to
 *	the display routine to keep the terminal current.  Also make
 *	sure the update routine is current.  One update routine per
 *	menu usually works the best.
 *
 *	If the dials change, be sure to change the previous menu that
 *	calls the new on so that the dials get updated properly.  Once
 *	a menu gets too crowded, go to another level.
 *
 * Don't forget:
 *
 *	At the top of each .c file is a procedure hierarchy comment.
 *	Keep it up to date.
 *
 *	Modify the Makefile if any new files are added.  Run "make depend"
 *	if an include file changes or there is a new file.
 *
 *
 * Data flow when controlling an attribute:
 *
 *	The keyboard specifies the current mode.  Default is the base menu
 *	where no attributes may be modified from the keyboard and the dials
 *	modify rotation, translation, scale and perspective.  Hit a key to
 *	go down a level to modify the desired attribute.  In some cases
 *	two steps are required.
 *
 *	When a dial is turned, check_select recognizes the fact and calls
 *	get_dial.  Get_dial reads the value of the dial, updates the
 *	dial_data structure corresponding to that dial, calls constrain_input
 *	to force wrapping or clamping of the data, then calls the specified
 *	attribute update routine (*dial_ptr->attr_update)().  The next time
 *	the display list is traversed, the change should take effect.
 *
 *	When a key is pressed that affects the value, the current absolute
 *	value (integer) in the dial_data structure is updated appropriately
 *	by the get_* routine, then the constraint code and the attribute
 *	update code are called, the same as when a dial is changed.
 *
 *	The display_* routine makes sure the terminal window is current after
 *	either a dial or keyboard change.
 *
 *
 * Register assignments:
 *
 *	R7 is the stack pointer (inherent).
 *	R6 is used for the return address after calling an object.
 *	R5 points at the object to call.
 *	R4 points to the base of the attribute block.
 *	R3 contains bits that affect the traversal (such as double buffering).
 *
 * hdl file notes:
 *
 *	An hdl file can be loaded using "of".  This file must not destroy
 *	any of the registers, especially R6, and may not execute any
 *	trap instructions.  It really doesn't matter if matrices are
 *	changed or color and such.  These are always set before the
 *	object is called and it doesn't matter what they get changed to.
 *	These files should not touch viewing matrices, perform any
 *	window or viewport clears or perform any matrix load operations.
 *	Loading a matrix destroys the controls provided by the demo program.
 *	Keep in mind that any attribute set by an hdl file can no longer
 *	be controlled by the demo program.
 *
 *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 */



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	main
 *	    init_args
 *	    init_dials
 *	    init_overlay
 *	    init_hawk
 *	    init_screen
 *	    build_main_loop
 *	    build_dl0
 *	    execute_dl
 *	    prog_exit
 *	prog_exit
 *	    hk_disconnect
 * 	    XUnmapWindow
 * 	    XDestroyWindow
 *	    XSync
 * 	    XNoOp
 *	    XCloseDisplay
 *	    exit_user_interf
 *	    hk_close
 *	    exit
 *	init_args
 *
 *==============================================================
 */



/*
 *--------------------------------------------------------------
 *
 * main
 *
 *	Main program for the shared memory sample program.
 *
 *--------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char **argv;
{
    int i;

/*
show_main_loop();
exit(0);
*/
    for (i = 0; i < 10; i++)
	dle[i].dl_begin = NULL;
    continuous_draw = 0;
    auto_rotate = 0;

    init_args(argc, argv);		/* Process cmd line args */
    init_dials();			/* Connect to the dialbox */
    init_overlay();
    init_hawk();			/* Initialize comm to fesim */
    init_screen(argc, argv);		/* Get the screen started up */
    build_main_loop();			/* Build main loop */
    build_dl0();			/* Build one sample object */
    execute_dl();			/* Execute DL (once) */
    prog_exit(0);			/* Clean up and exit */
} /* End of main */



/*
 *--------------------------------------------------------------
 *
 * prog_exit
 *
 *--------------------------------------------------------------
 */

void
prog_exit(n)
    int n;
{
/* Remove X dga window if present */ 

    if (dga) {
	hk_disconnect();

	XUnmapWindow(x_display, x_win);
	XDestroyWindow(x_display, x_win);
	XSync(x_display, FALSE);
	XNoOp(x_display); 

	XCloseDisplay(x_display);
    }

/* Clean up the screen stuff */

    exit_user_interf();


/* Clean up the Hawk stuff */

    hk_close(1);

/* Now you can exit */

    exit(n);
} /* End of prog_exit */



/*
 *--------------------------------------------------------------
 *
 * init_args
 *
 *	Process command line options.
 *
 *--------------------------------------------------------------
 */

void
init_args(argc, argv)
    int argc;
    char **argv;
{
    char *fb_freq;			/* Frame buffer frequency */
#ifdef QUICKTEST
    static char *usage = "Usage: gt_quicktest -f <script file>";
#else 
    static char *usage = "Usage: demo [-d] [-v] [-sock] [-x<width> -y<height>]\
 [-m<VM size>] [-f<script file>]";
#endif QUICKTEST 

    extern char *getenv();
    int pause;

    /* Determine display size before an argument might change it */
    screen_x = 100;
    screen_y = 80;
    if ((fb_freq = getenv("FB_FREQ")) != NULL) {
	if (strncmp(fb_freq, "1152", 4) == 0) {
	    screen_x = 1152;
	    screen_y = 900;
	}
	else if (strncmp(fb_freq, "1280", 4) == 0) {
	    screen_x = 1280;
	    screen_y = 1024;
	}
    }

    vm_size = MAX_VM_BIG;		/* Default memory size */

    prog_name = argv[0];		/* For error messages */

/* Initialize fesim args */
    fesim_argc = 0;
    fesim_argv[fesim_argc++] = FESIM;

/* Parse command line options */
    argv++;
    while ((--argc > 0) && (argv[0][0] == '-')) {
	switch (argv[0][1]) {
	case 'p':
	    pause = TRUE;
	    while (pause)
		;
	    break;

	case 'd':
	    use_dials = TRUE;
	    break;

	case 'v':
	    verbose = TRUE;
	    fesim_argv[fesim_argc++] = argv[0];
	    break;

	case 's':
	    if (strcmp(argv[0], "-sock") == 0) {
		fesim_argv[fesim_argc++] = argv[0];
	    }
	    break;

	case 'x':
	case 'w':
	    if (sscanf(&argv[0][2], "%d", &screen_x) != 1) {
		argc--;
		argv++;
		if ((argc == 0) || (sscanf(argv[0], "%d", &screen_x) != 1)) {
		    fprintf(stderr,
			"%s: Unable to decode screen width value\n",
			prog_name);
		    exit(1);
		}
	    }
	    break;

	case 'y':
	case 'h':
	    if (sscanf(&argv[0][2], "%d", &screen_y) != 1) {
		argc--;
		argv++;
		if ((argc == 0) || (sscanf(argv[0], "%d", &screen_y) != 1)) {
		    fprintf(stderr,
			"%s: Unable to decode screen height value\n",
			prog_name);
		    exit(1);
		}
	    }
	    break;

	case 'M':
	case 'm':
	    if (sscanf(&argv[0][2], "%d", &vm_size) != 1) {
		argc--;
		argv++;
		if ((argc == 0) || (sscanf(argv[0], "%d", &vm_size) != 1)) {
		    fprintf(stderr,
			"%s: Unable to decode maximum VM size\n",
			prog_name);
		    exit(1);
		}
	    }
	    break;

	case 'f':
	    if (argv[0][2] != 0) {
		strcpy(input_file_name, &(argv[0][2]));
	    }
	    else if ((argc > 0) && (argv[1][0] != '-')) {
		argc--;
		argv++;
		strcpy(input_file_name, &(argv[0][0]));
	    }
	    else {
		fprintf(stderr,
		    "%s: Unable to decode include file name\n",
		    prog_name);
		    exit(1);
	    }
	    input_file = fopen(input_file_name, "r");
	    if (input_file == NULL) {
		fprintf(stderr,
		    "%s: Unable to open the file: %s\n",
		    prog_name, input_file_name);
		    exit(1);
	    }
	    break;

	default:
	    fprintf(stderr, "%s: illegal option %s\n", prog_name, argv[0]);
	    fprintf(stderr, "%s\n", usage);
	    exit(1);
	}
	argv++;
    } /* End of argc loop */

    if (argc != 0) {
	fprintf(stderr, "%s\n", usage);
	exit(1);
    }

#ifdef QUICKTEST
    if( strlen( input_file_name ) == 0 ) {
	fprintf(stderr, "%s\n", usage);
	exit(1);
    }
#endif QUICKTEST
} /* End of init_args */

/* End of demo.c */
