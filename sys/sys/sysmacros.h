/*	@(#)sysmacros.h 1.1 94/10/31 SMI	*/

/*
 * Major/minor device constructing/busting macros.
 */

#ifndef _sys_sysmacros_h
#define _sys_sysmacros_h

/* major part of a device */
#define	major(x)	((int)(((unsigned)(x)>>8)&0377))

/* minor part of a device */
#define	minor(x)	((int)((x)&0377))

/* make a device number */
#define	makedev(x,y)	((dev_t)(((x)<<8) | (y)))

#endif /*!_sys_sysmacros_h*/
