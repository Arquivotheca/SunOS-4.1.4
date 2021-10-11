#ifndef lint
static	char sccsid[] = "@(#)confirm.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/ttysw.h>
#include <suntool/textsw.h>

#include "defs.h"

#include "typedefs.h"
#include "src.h"
#include "dbxtool.h"

private Frame	init_confirmer();
private int	confirm();
private void	yes_no();


int
confirm_yes(message)
char	*message;
{
    return confirm(message);
}


private int
confirm(message)
char	*message;
{
    Frame	confirmer;
    int		answer;

    /* create the confirmer */
    confirmer = init_confirmer(message);

    /* make the user answer */
    answer = (int) window_loop(confirmer);

    /* destroy the confirmer */
    window_set(confirmer, FRAME_NO_CONFIRM, TRUE, 0);
    window_destroy(confirmer);

    return answer;
}


private Frame
init_confirmer(message)
char	*message;
{
    Frame		confirmer, base_frame;
    Panel		panel;
    Panel_item		message_item;
    Textsw		src_sw_h;       /* handle for source subwindow */
    int			left, top, width, height;
    Rect		bf_rect;
    Rectp		r, src_rp;
    struct pixrect	*pr;

    /*
     * The confirmer is not a child of the base frame, because it
     * normally shows up in the middle of the *screen*, not necessarily
     * anywhere near the dbxtool window.
     */
    confirmer = window_create(0, FRAME, FRAME_SHOW_LABEL, FALSE, 0);

    panel = window_create(confirmer, PANEL, 0);

    message_item = panel_create_item(panel, PANEL_MESSAGE, 
	PANEL_LABEL_STRING, message,
	0);

    pr = panel_button_image(panel, "NO", 3, 0);
    width = 2 * pr->pr_width + 10;

    r = (Rect *) panel_get(message_item, PANEL_ITEM_RECT);
 
    /* center the yes/no or ok buttons under the message */
    left = (r->r_width - width) / 2;
    if (left < 0)
	left = 0;
    top = rect_bottom(r) + 5;

    panel_create_item(panel, PANEL_BUTTON, 
        PANEL_ITEM_X, left, PANEL_ITEM_Y, top,
        PANEL_LABEL_IMAGE, pr,
        PANEL_CLIENT_DATA, 0,
        PANEL_NOTIFY_PROC, yes_no,
        0);
    
    panel_create_item(panel, PANEL_BUTTON, 
        PANEL_LABEL_IMAGE, panel_button_image(panel, "YES", 3, 0),
        PANEL_CLIENT_DATA, 1,
        PANEL_NOTIFY_PROC, yes_no,
        0);

    window_fit(panel);
    window_fit(confirmer);

    /*
     * Center the confirmer in the src window.
     */
    base_frame = get_base_frame( );

    r = (Rect *)window_get( base_frame, WIN_RECT );
    bf_rect = *r;       /* copy the structure */

    src_sw_h = get_srctext();
    src_rp = (Rect *)window_get( src_sw_h, WIN_RECT );

    src_rp->r_top += bf_rect.r_top;
    src_rp->r_left += bf_rect.r_left;

    width = (int) window_get(confirmer, WIN_WIDTH);
    height = (int) window_get(confirmer, WIN_HEIGHT);

    left = src_rp->r_left + (src_rp->r_width - width) / 2;
    top = src_rp->r_top + (src_rp->r_height - height) / 2;

    if (left < 0)
	left = 0;
    if (top < 0)
	top = 0;

    window_set(confirmer, WIN_X, left, WIN_Y, top, 0);

    return confirmer;
}


/* yes/no/ok notify proc */
private void
yes_no(item, event)
Panel_item	item;
Event		*event;
{
    window_return(panel_get(item, PANEL_CLIENT_DATA));
}
