/*	@(#)text.h 1.1 94/10/31 SMI	*/

/***********************************************************************/
/*                            text.h                                    */
/*              Copyright (c) 1985 by Sun Microsystems, Inc.           */
/***********************************************************************/

/* Compatibility only -- THIS IS GOING AWAY */

#ifndef text_DEFINED
#define text_DEFINED

#include <suntool/textsw.h>

#define TEXT_TYPE ATTR_PKG_TEXTSW
#define TEXT textsw_window_object, WIN_COMPATIBILITY
typedef Textsw Text;

#endif ~text_DEFINED 
