/*      @(#)wx.h  1.1 94/10/31  SMI    */
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 *
 *-----------------------------------------------------------------------
 * Window grabber minor op codes
 *-----------------------------------------------------------------------
 */

#define  X_WxGrab 		1
#define  X_WxUnGrab 		2
#define  X_WxGrabColormap 	3
#define  X_WxUnGrabColormap 	4
#define  X_WxGrabWids 		5	
#define  X_WxGrabBuffers 	6	
#define  X_WxUnGrabBuffers  	7	
#define  X_WxGrabFCS            8
#define  X_WxGrabZbuf           9
#define  X_WxGrabStereo	       10	
#define  X_WxGrab_New 	       11
#define  X_WxUnGrab_New	       12
#define  X_WxGrabColormapNew   13
#define  X_WxUnGrabColormapNew 13



#define WX_DB_EXISTS    1
#define WX_DB_A         2
#define WX_DB_B         3
#define WX_DB_NONE      4
#define WX_DB_SINGLE    5
#define WX_DB_DOUBLE    6
#define WX_DB_BOTH      7

/*
** OWGX request structure
*/

 typedef struct {
    CARD8 reqType;
    BYTE pad;
    CARD16 length B16;
    CARD32 id B32;  /* a Window, Drawable, Font, GContext, Pixmap, etc. */
    CARD32 number_objects B32;
    } xOWGXReq;


/*
** New requests now tell server what versions they handle.
*/
  typedef struct {
    CARD8 reqType;
    BYTE pad;
    CARD16 length B16;
    CARD32 id B32;  /* a Window, Drawable, Font, GContext, Pixmap, etc. */
    CARD16 minversion B16;
    CARD16 maxversion B16;
    } xOWGXNewReq;
