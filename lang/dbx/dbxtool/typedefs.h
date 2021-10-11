/*	@(#)typedefs.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef typedefs_h
/*
 * Typedefs of window system entities.
 */
typedef Pixwin		 *Pixwinp;
typedef	Pixrect		 *Pixrectp;
typedef	Pixfont		 *Pixfontp;
typedef Rectlist	 *Rectlistp;
typedef Rectnode	 *Rectnodep;
typedef	Rect		 *Rectp;
typedef Event		 *Eventp;
typedef struct selection *Selectp;
typedef	struct menuitem  *Menuitemp;
typedef Ttysubwindow	  Cmdsw;

typedef	char	*(*Strfunc)();		/* Ptr to a func returning a string */

#define baseline_y(font)	(-(font)->pf_char['a'].pc_home.y)
#define font_height(font)	((font)->pf_defaultsize.y)
#define font_width(font)	((font)->pf_defaultsize.x)
#define text_getfont(textsw)	(Pixfontp) window_get(textsw, WIN_FONT);

#define typedefs_h
#endif
