#ifndef lint
static char sccsid[] = "@(#)demo_user_interf.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_user_interf.c
 *
 *	@(#)demo_user_interf.c 1.36 91/05/30 14:50:43
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the user interface code.  The output is
 *	tuned for an 80 x 24 CRT.  For more details see chapter 12 of
 *	Programming Utilities and Libraries.
 *
 *	display_* routines put the right stuff up on the screen.
 *
 *  3-Apr-90 Scott R. Nelson	  Initial version.
 *  9-May-90 Scott R. Nelson	  Integrated this module with the other code.
 * 24-May-90 Scott R. Nelson	  Menu stuff split off into demo_menus.c
 * 19-Jul-90 Scott R. Nelson	  Change to consistent dial/keyboard
 *				  interface, added comments.
 * 23-Jul-90 Scott R. Nelson	  Added rotation axis, sub-menu for
 *				  transforms.
 * 27-Sep-90 Scott R. Nelson	  Minor tuning of the code.
 * 27-Sep-90 Scott R. Nelson	  Removed all of the input control
 *				  routines to separate files.
 *  4-Oct-90 Scott R. Nelson	  Added auto_rotate flag.
 * 29-Jan-91 Kevin C. Rushforth	  Fixed matrices for HIS 1.5
 * 16-Apr-91 Chris Klein          Added SunView capability
 * 09-May-91 Chris Klein          Added QUICKTEST pre-processor define.
 *                                Input is only accepted from a script file
 *                                and thereafter ingnored.
 * 16-May-91 John M. Perry	  Convert from Xlib frame to XView
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <curses.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>

#include "demo.h"
#include "demo_sv.h"


/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	execute_dl
 *	    select_object
 *	    (*display_current_screen)()
 *	    notify_dispatch
 *	    check_select
 *	    draw_one_frame
 *	    display_exit_message
 *	    exit_user_interf
 *	draw_one_frame
 *	    hawk_stopped
 *	    start_hawk
 *	select_object
 *	exit_user_interf
 *	push_menu
 *	pop_menu
 *	check_select
 *	    select
 *	    get_dial
 *	    (*get_input)()
 *
 *==============================================================
 */



/*
 *--------------------------------------------------------------
 *
 * execute_dl
 *
 *	Modify and execute the display list.
 *
 *--------------------------------------------------------------
 */

void
execute_dl()
{

/* Do the screen stuff */

    initscr();				/* Allocate working space, etc. */
    scrollok(stdscr, FALSE);		/* Don't scroll */
    leaveok(stdscr, FALSE);		/* Not okay to leave cursor around */

/* Prepare to use single-character mode */

    noecho();				/* Don't echo characters */
    cbreak();
    raw();

/* Initialize all of the variables we'll use here */

    clear();

    /* Specify the routines to use */
    menu_sp = 0;			/* Initialize stack */
    display_current_screen[menu_sp] = display_main_menu;
    get_input[menu_sp] = get_main_loop;
    select_object(0);			/* Start off with display list 0 */

/*
 * There are three things that can make us change the (terminal) screen
 * or the graphics (Hawk) screen:
 *	Keyboard input
 *	Control dial input
 *	Traversal complete in continuous update mode
 */

    (*display_current_screen[menu_sp]) ();
    do {

	(void) notify_dispatch();

#ifdef XVIEW
	XFlush(x_display);
#endif

#ifdef QUICKTEST

        if( input_file == NULL ) {
	    if( Autocycle ) {
	    	if( ! (Autocycle % 50) ) {  /* Switch  */
		    select_object( dl == 3 ? 1 : dl + 1 );
		    Autocycle=1;
		}
	    }
	} else
	    check_select();	/* Get keyboard or dial */
#else
	check_select();	/* Get keyboard or dial */
	if (!no_data)
	    (*display_current_screen[menu_sp]) ();
#endif QUICKTEST

	if ( continuous_draw && hawk_stopped() && 
	     (!sv_win.frame || !window_get(sv_win.frame, FRAME_CLOSED)) ) {
/*struct timeval tp;*/
/*struct timezone tzp;*/
	    /* Update the spin matrix */
	    if (auto_rotate) {
		rot_y(dle_ptr->rot_speed.current_value, dle_ptr->mat.matrix);
		concat_mat(dle_ptr->p_dl_data->spin_matrix.matrix,
		    dle_ptr->mat.matrix,
		    dle_ptr->p_dl_data->spin_matrix.matrix);
	    }
	    draw_one_frame();
#ifdef QUICKTEST
	    if( Autocycle ) Autocycle++;
#endif QUICKTEST
/*gettimeofday(&tp, &tzp);*/
	}

    } while (!done_with_program);

    display_exit_message();

    exit_user_interf();

} /* End of execute_dl */



/*
 *--------------------------------------------------------------
 *
 * draw_one_frame
 *
 *	Update the matrices for the current object and cause it
 *	to be displayed.
 *
 *--------------------------------------------------------------
 */

void
draw_one_frame()
{
    if (dle_ptr->dl_begin == NULL)
	return;
#if 0
fprintf(stderr, "[%d]\r", dle_ptr->p_dl_data->tr_flags);
#endif
    while (!hawk_stopped())
	usleep(100);			/* Make sure Hawk is ready */

    start_hawk();
} /* End of draw_one_frame */



/*
 *--------------------------------------------------------------
 *
 * select_object
 *
 *	Select the specified object and allow modifications to
 *	be made to its appearance.  Don't change anything if the
 *	pointer is null.
 *
 *--------------------------------------------------------------
 */

void
select_object(number)
    int number;
{
    extern char *wb_ptr;
    if (dle[number].dl_begin != NULL) {
	dl = number;
	dle_ptr = &dle[dl];
	dle_ptr->p_dl_data->tr_flags |= TRF_PREPARE_WINDOW | TRF_SET_FCBG;
	if ( sv_win.wid )  {
	    sv_win.cur_wb_ptr = dle_ptr->p_dl_data->wb_ptr;
	    set_sv_wb();
	}

	cur_dials[0] = &dle_ptr->x_rot;
	cur_dials[1] = &dle_ptr->y_rot;
	cur_dials[2] = &dle_ptr->z_rot;
	cur_dials[3] = &dle_ptr->scale;
	cur_dials[4] = &dle_ptr->x_trans;
	cur_dials[5] = &dle_ptr->y_trans;
	cur_dials[6] = &dle_ptr->z_trans;
	cur_dials[7] = &dle_ptr->perspective;
    }
else fprintf(stderr,"%d not valid\r",number);
    /* Else, this one is not yet valid */
} /* End of select_object */



/*
 *--------------------------------------------------------------
 *
 * exit_user_interf
 *
 *	Clean up after using direct screen access.
 *
 *--------------------------------------------------------------
 */

void
exit_user_interf()
{

/* Clean up the screen stuff */

    move(LINES - 1, 0);
    refresh();
    endwin();

/* Go back to normal character input mode */

    noraw();
    nocbreak();
    echo();				/* Echo characters */
} /* End of exit_user_interf */



/*
 *--------------------------------------------------------------
 *
 * push_menu
 *
 *	Push the specified menu and input procedures onto the
 *	menu stack.  The one on the top of stack is used.
 * 
 *	Note: dial_data must only be replaces AFTER a push operation.
 *
 *--------------------------------------------------------------
 */

void
push_menu(menu_proc, input_proc)
    void (*menu_proc)();
    void (*input_proc)();
{
    int i;				/* Iterative counter */

    menu_sp += 1;			/* Pre-increment */

    if (menu_sp == MS_MAX) {
	fprintf(stderr,
	    "\r\nAckkk!  Menu stack overflow!\r\nArrrrrgggggghhhhhhhh!!!\r\n");
	prog_exit(1);
    }
    display_current_screen[menu_sp] = menu_proc;
    get_input[menu_sp] = input_proc;

    for (i = 0; i < 8; i++) {
	dial_stack[i][menu_sp] = cur_dials[i];
    }
} /* End of push_menu */



/*
 *--------------------------------------------------------------
 *
 * pop_menu
 *
 *	Pop back one level on the menu stack.
 *
 *--------------------------------------------------------------
 */

void
pop_menu()
{
    int i;				/* Iterative counter */

    if (menu_sp == 0) {
	fprintf(stderr,
	    "\r\nAckkk!  Menu stack underflow!\r\nArrrrrgggggghhhhhhhh!!!\r\n");
	prog_exit(1);
    }

    for (i = 0; i < 8; i++) {
	cur_dials[i] = dial_stack[i][menu_sp];
    }

    menu_sp -= 1;			/* Post-decrement */
} /* End of pop_menu */



/*
 *--------------------------------------------------------------
 *
 * check_select
 *
 *	Check the current input devices (keyboard and dials) to seed
 *	if they have data.  If they do, call a processing routine to
 *	do something with that data.
 *
 *--------------------------------------------------------------
 */

void
check_select()
{
    struct timeval timeout;		/* Time value */
    fd_set rfds;			/* Read fds */
    int width;				/* Width of file descriptor set */
    int ready;				/* How many are ready */

    if (input_file != NULL) {
	input_ch = getc(input_file);
	if (input_ch == ';') {		/* A comment, ignore rest of line */
	    do {
		input_ch = getc(input_file);
	    } while ((input_ch != EOF) && (input_ch != '\n'));
	}
	if (input_ch == EOF) {
	    input_file = NULL;		/* Done with this file */
#ifdef QUICKTEST
	    /* Close stdin */
	    close(1);
#endif QUICKTEST
	    refresh();
	}
	else {
	    no_data = 0;		/* There is valid data */
	    (*get_input[menu_sp]) ();	/* Process this character */
	    return;			/* Nothing more to do here */
	}
    }

    width = getdtablesize();

    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;		/* 1/1000 second */

    FD_ZERO(&rfds);
    if (use_dials)
	FD_SET(db_fd, &rfds);
    FD_SET(fileno(stdin), &rfds);

    no_data = 1;			/* No input data yet */
    input_ch = 0;			/* No valid input character */

    /* Select an input device */
    ready = select(width, &rfds, NULL, NULL, &timeout);

    if (ready <= 0)
	return;

    if (use_dials && FD_ISSET(db_fd, &rfds)) {	/* Read from the dials */
	no_data = 0;
	get_dial();
    }
    if (FD_ISSET(fileno(stdin), &rfds)) {	/* Read from the keyboard */
	no_data = 0;
	input_ch = getch();
	(*get_input[menu_sp]) ();
    }

} /* End of check_select */

/* End of demo_user_interf.c */
