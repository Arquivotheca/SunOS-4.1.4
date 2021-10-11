/*      @(#)win_grab.h  1.1 94/10/31  SMI    */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/****
 *
 * WIN_GRAB.H Window Grabber
 *
 * This file describes the shared memory info page used by the window
 * grabber.  Many of the structures are shapes specific, and their
 * definitions are copied from shapes #include files.  If the shapes
 * structures are changed, then this file would be messed up; but
 * if that happens, a lot of other things would get messed up too, so
 * let's hope that it never happens.
 *
 * This structure is defined in such a way as to allow expansion.  In
 * particular, the field 'w_clipoff' contains the offset, in bytes, of
 * the clip data relative to the start of the window information area.
 * Do not attempt to refer directly to the 'w_shape_hdr' element of
 * this structure, as that can cause old executables to break with new
 * servers.
 *
 * It is anticipated that other "offset" fields will be added to this
 * structure as the information in the window-info area expands.
 *
 * MACROS: These return info from the client page.
 *
 *	wx_infop(clientp)	the info page.
 *	wx_devname_c(clientp)	returns the w_devname field.
 *	wx_devfd(clientp)	device fd
 *	wx_version_c(clientp)	the version
 *	wx_cinfo(clientp)	the cursor-info page.
 *	wx_lock_c(clientp)	locks the info page.
 *	wx_unlock_c(clientp)	unlocks the info page.
 *	wx_modif_c(clientp)	returns nonzero if the cliplist has
 *				been modified.
 *	wx_seen_c(clientp)	clears the "modified" flag.
 *
 *	wx_sh_clipinfo_c(clientp)	address of the start of the clip list.
 *	wx_shape_flags_c(clientp)	Shapes flag
 *	wx_shape_size_c(clientp)	the shape size field
 *	wx_shape_ymin_c(clientp)	xmax,xmin,ymax,ymin
 *	wx_shape_ymax_c(clientp)
 *	wx_shape_xmin_c(clientp)
 *	wx_shape_xmax_c(clientp)
 *	wx_cookie_c(clientp)	The filename cookie (from XGrabWindow)
 *
 * These return info from the info page.
 *
 *	wx_devname(infop)	returns the w_devname field.
 *	wx_version(infop)	the version
 *
 *	wx_shape_flags(infop)	Shapes flag
 *	wx_shape_size(infop)	the shape size field
 *	wx_shape_ymin(infop)	xmax,xmin,ymax,ymin
 *	wx_shape_ymax(infop)
 *	wx_shape_xmin(infop)
 *	wx_shape_xmax(infop)
 *
 * Here is a brief description of the WXINFO structure elements
 * which are relavent to the client:
 *
 *	w_flag		set to WMODIF by the server whenever any field
 *			in the window info area is changed.  Set back to
 *			WSEEN by the client once the info has been read.
 *			The window-info area should be locked while examining
 *			this flag and copying information from the
 *			window-info area.
 *	w_magic		Set to 0x47524142 ("GRAB").
 *	w_version	version of window-info area.  This header file
 *			currently describes version 1.	Programs should
 *			check the version number and either refuse to run
 *			with older versions or at least be careful not to
 *			use fields not defined in older versions.  The
 *			comments in the WXINFO structure definition will
 *			show you which these are.
 *	w_devname	Ascii device name.  Let's hope there's never a
 *			framebuffer with a filename larger than 20 characters.
 *	w_cinfofd	File descriptor of wininfo file.  This file has
 *			been mmap'ed, so there is no reason for the
 *			application to need to use this.
 *	w_cdevfd	File descriptor of the framebuffer named in w_devname.
 *	w_crefcnt	Lock count for nested locking.
 *	w_cookie	Internal use only.  NOT related to the window cookie
 *			that was returned by X_GrabWindow().
 *	w_clockp	pointer to lockpage, used by wx_lock().
 *	w_clipoff	Byte offset from start of info page to start of
 *			clip list.
 *	w_clipsize	size of cliplist.
 *	w_org, w_dim	window dimensions? cliplist bounding box?
 *			In 16-bit signed fract format.
 *	w_shape		cliplist info
 *
 *
 * Interpreting the clip info:
 *
 *  The client is interested in the following structures:
 *    w_shape_hdr	pointed to via the wx_clipinfo macro.
 *    w_shape		contains flags and a bounding-box for the cliplist.
 *    cliparray		follows w_shape.  Contains the clip list.
 *
 *  w_shape.SHAPE_FLAGS contains flags that describe the cliplist.  If
 *  SH_WX_EMPTY is set, then the clip list is empty (window obscured or
 *  something).
 *
 *  If SH_RECT_FLAG is set, the clip list is a single rectangle
 *  described by w_shape->SHAPE_{XMIN,YMIN,XMAX,YMAX}.
 *
 *  If SH_SPECIAL_FLAG is set, the clip list is made up of rectangles
 *  that are 1 scanline high, exactly one per y value.
 *
 *  For non-empty cliplists, use the wx_sh_clipinfo(shape) macro to get
 *  a pointer to the cliplist.
 *
 *  "Normal" cliplists are a sequence of signed shorts which describes
 *  a sequence of rectangles.  The data consists of a sequence of one
 *  or more ymin,ymax pairs, each of which is followed by a sequence of
 *  one or more xmin,xmax pairs.  xmin,xmax sequences are terminated by
 *  a single value of X_EOL.  ymin,ymax sequences are terminated by a
 *  single value of Y_EOL.  This is best described with some sample
 *  code:
 *
 *	short x0,y0,x1,y1 ;
 *
 *	ptr = wx_sh_clipinfo(shape) ;
 *	while( (y0=*ptr++) != Y_EOL )
 *	{
 *	  y1 = *ptr++ ;
 *	  while( (x0=*ptr++) != X_EOL )
 *	  {
 *	    x1 = *ptr++ ;
 *	    printf("rectangle from (%d,%d) to (%d,%d)\n",x0,y0,x1-1,y1-1) ;
 *	  }
 *	}
 *
 *  Note that the xmax,ymax values are actually one pixel too high.  This
 *  may be a bug or a feature, I don't know.
 *
 ****/

#ifndef	_WXGRAB

#ifndef	WMODIF			/* cliplist flag, one of: */
#define	WMODIF	1		/* server has set a new cliplist */
#define	WEXTEND	2		/* extended cliplist present	*/
#define	WXMAGIC	0x47524142	/* "GRAB" */
#endif	WMODIF

#define	WX_PAGESZ	(8*1024)

#ifndef	_SH_POINT		/* 2d-point structure from Shapes */
typedef	struct { unsigned int t; int   x, y; } POINT_B2D;
typedef	struct { short   x, y; } COORD_2D;
#endif	_SH_POINT

#ifndef	_SH_CLASS
typedef	void	*OBJID ;
typedef	OBJID	(*CLASS_OPER)() ;

typedef	struct {
	  unsigned char	type ;
	  unsigned char	id ;
	} CLASS ;

struct	obj_hdr {
	  unsigned int	obj_flags : 8 ;
	  unsigned int	obj_size : 24 ;
	  CLASS		obj_class ;
	  unsigned short obj_use ;
	  CLASS_OPER	*obj_func ;
	} ;
#endif	_SH_CLASS

#ifndef	_SH_SHAPE_INT
struct	class_SHAPE_vn {
	  unsigned char	SHAPE_FLAGS ;
	  unsigned char	SHAPE_SIZE ;	/* new with 1.1 */
	  short		SHAPE_YMIN ;
	  short		SHAPE_YMAX ;
	  short		SHAPE_XMIN ;
	  short		SHAPE_XMAX ;
	  short		SHAPE_X_EOL ;
	  union {
	    short	SHAPE_Y_EOL ;
	    short	*obj ;		/* new with 1.1 */
	  } u ;
	} ;

struct	class_SHAPE_v0 {
	  unsigned char	SHAPE_FLAGS ;
	  short		SHAPE_YMIN ;
	  short		SHAPE_YMAX ;
	  short		SHAPE_XMIN ;
	  short		SHAPE_XMAX ;
	  short		SHAPE_X_EOL ;
	  short		SHAPE_Y_EOL ;
};

#define	SH_RECT_FLAG	1
#define	SH_SPECIAL_FLAG	2
#define	SH_FLIP_Y	4
#define	SH_WX_EMPTY	128
#define	X_EOL		(-32767)
#define	Y_EOL		(-32768)
#endif	_SH_SHAPE_INT

typedef	unsigned long	*WX_LOCKP_T;

#define MAX_FB		16	    /* same as max number of lock pages */
#define CURSOR_DOWN	0
#define CURSOR_UP	1
/* Big time kludge - i assume max cursor size 32x32x24 */
#define CURG_SAVE_UNDER_SZ	(sizeof(CURG_MPR) + 3072)
#define CURG_MAGIC		0x43555247

typedef struct curg_mpr {
    u_long		curg_linebytes;
    COORD_2D		curg_dim;
    u_char		curg_depth;
/* image data floats under here */
} CURG_MPR;

typedef struct curg_info
{
    long        c_magic;		/* magic no, "CURG"=0x43555247 */
    int		*c_screen;		/* screen structure for fb */
    int		*c_scr_ras;		/* screen raster ass. w/cursor*/
    int		c_scurgfd;		/* file descriptor for server */
    int		c_ref_cnt;		/* total number of cur-grabbers*/
    int		c_state_flag;		/* client/server can set this */
    int		c_chng_cnt;		/* Change count */
    int		c_index;		/* entry no. in global array */
    COORD_2D	c_org;			/* top left */
    COORD_2D	c_hot_spot;		/* why bother with this? */
    int		c_offset;		/* offset in bytes from top of
					   page to save_under info */
    CURG_MPR	*c_sptr;		/* server's ptr to save_unders*/
    CURG_MPR    c_saved_under;		/* this gets puts back up */

} CURGINFO;


#ifndef	Unsgn32
#define Unsgn32 unsigned int
#endif	Unsgn32

#ifndef Unsgn8
#define Unsgn8 unsigned char
#endif Unsgn32

/* Defines for double buffering information state. Used
** in shared memory and in the multiple plane group
** information structures.
*/

#define WX_DB_EXISTS    1
#define WX_DB_A		2
#define WX_DB_B		3
#define WX_DB_NONE      4
#define WX_DB_SINGLE    5
#define WX_DB_DOUBLE    6
#define WX_DB_BOTH      7
 
/* Planegroup ID's used in the Hawk specific portion of
** the double buffer information structure DBINFO.
*/

#define GT_RED_A        0  /* Red buffer A */
#define GT_RED_B        1  /* Red buffer B */
#define GT_GREEN_A      2  /* Green buffer A */
#define GT_GREEN_B      3  /* Green buffer B */
#define GT_BLUE_A       4  /* Blue buffer A */
#define GT_BLUE_B       5  /* Blue buffer B */
#define GT_OVL_A        6  /* Overlay buffer A */
#define GT_OVL_B        7  /* Overlay buffer B */
#define GT_IMAGE_A      8  /* Image buffer A */
#define GT_IMAGE_B      9  /* Image buffer B */
 
#define GT_MAX_BUF 8    /* up to 8 8 bit indexed buffers available */
#define GT_MAX_BUF_32 2 /* 2 32 bit direct color buffers available */
#define GT_MAX_FCS 4    /* up to 4 sets of fast clear planes */
 


/* Planegroup ID's used in the Campus2 specific portion of
** the double buffer information structure DBINFO.
*/

#define	CAMPUS2_RED	0  /* Red channel */
#define	CAMPUS2_GREEN	1  /* Green channel */
#define	CAMPUS2_BLUE	2  /* Blue channel */
#define	CAMPUS2_ALL	3  /* All RGB channels */


typedef struct wx_gt {
        char    gt_buf_pg[GT_MAX_BUF];  /* planegroup id for each buffer */
        char    gt_nfcs;                /* number fast clear sets allocated */
        char    gt_fcs[GT_MAX_FCS];     /* ids of fast clear sets allocated */
        char    gt_buf_fcs[GT_MAX_BUF]; /* fcs to use for each buffer */
        union {
            Unsgn32     c_32[GT_MAX_BUF_32];
            Unsgn8      c_8[GT_MAX_BUF];
        } gt_fc_col;
} WX_GT;
 

typedef struct wx_egret
{
	char	buf_gids[2];		/* A12 or B12, use this to get masks */
} EGRET;

typedef struct wx_quadro 
{	
	short legocb;    	/* Single or multiple db clients */
	caddr_t	bufaddr[2];	/* client virtual address of buffer n */
} QUADRO;

typedef struct wx_campus2 
{	
	short campus2cb;  		/* Copy mode multibuffering */
	short xvalue;			/* Our Current xvalue */
	char	buf_pgids[4];		/* client virtual address of buffer n */
} CAMPUS2;

typedef struct wx_dbinfo
{
	short   number_buffers;
	short   read_buffer;
	short   write_buffer;
	short   display_buffer;
	off_t   wx_vrtctroff;	/* device file offset to VRT counter */
	Unsgn32 wx_vert_ctr;	/* Client's VRT counter */
	Unsgn32 *wx_cvrtcntr;	/* Client's ptr to VRT counter */
	Unsgn32 *wx_svrtcntr;	/* Window system's ptr to VRT counter */
	Unsgn32	wx_db_token;    /* Dbuf token, changed to ptr w/wid's */
	Unsgn32 WID;		/* Mpg window id. */
	Unsgn32 UNIQUE;		/* = 0; unique, =1, shared */
	union {
		struct wx_gt	hawk;
		struct wx_egret egret;
		struct wx_quadro quadro;
		struct wx_campus2 campus2;
		char		pad[128] ;	/* try to keep union size from
						   changing in future */
	} device;
	Unsgn32	wx_db_swapint; 
} DBINFO;



typedef	struct	wx_winfo
{
  /* VERSION 0 INFO STARTS HERE */
	  unsigned long	w_flag;		/* cliplist flag */
	  long		w_magic;	/* magic number, "GRAB"=0x47524142 */
	  long		w_version;	/* version, currently 2 */
	  WX_LOCKP_T	w_cunlockp;	/* used to be w_list */
					/* points to unlock page */

	  /* server info, meaningless to client */
	  int		w_sinfofd;	/* fd of wxinfo file */
	  int		w_sdevfd;	/* fd of framebuffer */
	  WX_LOCKP_T	w_slockp;	/* pointer to lockpage */
	  int		w_srefcnt;	/* lock count for nested locks */

	  /* client info */
	  char		w_devname[20];	/* framebuffer device name */
	  int		w_cinfofd;	/* fd of wxinfo file */
	  int		w_cdevfd;	/* fd of framebuffer */
	  int		w_crefcnt;	/* lock count for nested locks */

	  unsigned long	w_cookie;	/* "cookie" for lock pages */
	  unsigned long	w_filesuffix;	/* "cookie" for info file */
	  WX_LOCKP_T	w_clockp;	/* pointer to lock page */

	  /* clipping info */
	  short		*w_cclipptr;	/* client virtual pointer to	*/
					/* clip array.			*/
	  unsigned int	w_scliplen;	/* server's size of cliplist	*/
					/* in bytes.			*/
	  /* shapes stuff */
	  POINT_B2D	w_org;		/* origin of window */
	  POINT_B2D	w_dim;		/* size of window */

	  /* VERSION 1 INFO STARTS HERE */

	  union {
	    struct {
		long	w_clipoff;	/* byte offset from wx_info to	*/
					/* w_cliparray.			*/
		long	w_shapeoff;	/* byte offset from wx_info to	*/
					/* w_shape.			*/
		long	w_clipseq;	/* clip sequence number		*/
		long	w_ccliplen;	/* client's size of clip list	*/
					/* in bytes.			*/
		int	*w_sclipptr;	/* server's pointer to clip array */
	    } vn;
	    struct {
		struct	obj_hdr w_shape_hdr ;	/* not useful to client */
		struct	class_SHAPE_v0 w_shape ;
		short	w_cliparray;
	    } v0;
	  } u;


	/* VERSION 2 INFO STARTS HERE */

	/* cursor grabber info */
	int	 *c_scr_addr;	/* info derive shared cursor file from */
	CURGINFO *c_sinfo;	/* pointer to server's cursor info */
	CURGINFO *c_cinfo;	/* pointer to client's cursor info */
	int	  c_cfd;	/* Client side file descriptor */

	/* window id info */
	short	w_number_wids ;	/* number of contiguous wids alloc */
	short	w_start_wid ;	/* starting wid */
	short	w_wid ;		/* current wid */

	/* integer version of window bounds */
	struct {
	  int	xleft, ytop ;		/* upper-left corner, in pixels */
	  int	width, height ;		/* dimensions, in pixels */
	} w_window_boundary ;

	struct wx_dbinfo wx_dbuf;	/* double buffer info structure */

	int	w_auxdevfd;		/* Aux fd for asynch devices */
	char	w_auxdevname[20] ;	/* Asynch device name */
	short	w_auxdevid ;		/* ID number for aux dev type */
	short	w_auxserveronly;	/* To keep SERVER_ONLY's in sync */

	WX_LOCKP_T	w_sunlockp;	/* pointer to unlockpage */
	int	w_extnd_seq ;		/* cliplist extension counter */
	int	w_clientcnt ;		/* # of clients using new protocol */
	int	w_oldproto ;		/* client used old protocol */
	int	w_locktype ;		/* what type of locking */


  /* FLOATING INFO STARTS HERE, DO NOT REFER DIRECTLY
     TO ANYTHING BELOW THIS LINE */

	  struct obj_hdr w_shape_hdr ;	/* not useful to client */
	  struct class_SHAPE_vn w_shape ;
	  short	w_cliparray;
} WXINFO ;


#define	WG_LOCKDEV	0	/* use GRABPAGEALLOC from fb */
#define	WG_WINLOCK	1	/* use winlock functions */



	/* client-private data.  By using this structure instead of
	 * the wx_info structure above, multiple clients can grab the
	 * same window.  There will be a new protocol request to let
	 * the server know that we will be using this private structure.
	 * The server can only permit one client to grab using the old
	 * protocol.
	 */

typedef	struct wx_client
{
	WXINFO		*w_info ;	/* pointer to shared memory */
	long		w_version;	/* version, copied from w_info */
	struct wx_client *w_next ;	/* for client use */
	caddr_t		w_client ;	/* for client use */
	int		w_clip_seq ;	/* last recorded change count */
	int		w_extnd_seq ;	/* last recorded extension count */
	int		w_cinfofd;	/* fd of wxinfo file */
	int		w_cdevfd;	/* fd of framebuffer */
	int		w_crefcnt;	/* lock count for nested locks */
	WX_LOCKP_T	w_clockp;	/* pointer to lock page */
	WX_LOCKP_T	w_cunlockp;	/* points to unlock page */
	short		*w_cclipptr;	/* client virtual pointer to	*/
					/* clip array.			*/
	long		w_ccliplen;	/* client's size of clip list	*/
					/* in bytes.			*/
	int		w_proto_vers ;	/* which version of protocol worked? */

	/* cursor grabber info */
	CURGINFO *c_cinfo;	/* pointer to client's cursor info */
	int	  c_cfd;	/* Client side file descriptor */

	int		w_grab_count ;	/* how many times we've grabbed it */
} WXCLIENT ;




#define WX_LOCK(x)	(*((x)->w_clockp)) = 1
#define WX_UNLOCK(x)	(*((x)->w_cunlockp)) = 0

#define	wx_infop(clientp)	((clientp)->w_info)

#define	wx_devname(infop)	((infop)->w_devname)
#define	wx_devname_c(clientp)	(wx_devname((clientp)->w_info))

#define	wx_version(infop)	((infop)->w_version)
#define	wx_version_c(clientp)	((clientp)->w_version)

#define	wx_cookie_c(clientp)	((clientp)->w_version >= 2 ?		\
				 (clientp)->w_info->w_filesuffix :	\
				 (clientp)->w_info->w_cookie )


#define	wx_cinfo(clientp)	((clientp)->c_cinfo)

#define	wx_lock_c(clientp)		\
    do {				\
    if ((((WXCLIENT*)(clientp))->w_crefcnt)++ == 0) { \
	WX_LOCK(clientp);		\
    }					\
    } while (0)

#define wx_unlock_c(clientp)		\
    do {				\
    if (--(((WXCLIENT*)(clientp))->w_crefcnt) == 0)	\
	WX_UNLOCK(clientp);		\
    } while (0)

#define	wx_modif_c(clientp)				\
	( (clientp)->w_version == 0			\
	    ? (clientp)->w_info->w_flag & WMODIF	\
	    : (clientp)->w_clip_seq != (clientp)->w_info->u.vn.w_clipseq )

#define	wx_seen_c(clientp)				\
	( ((clientp)->w_version == 0)			\
	    ? ((clientp)->w_info->w_flag &= ~WMODIF)	\
	    : ((clientp)->w_clip_seq = (clientp)->w_info->u.vn.w_clipseq) )

#define	wx_sh_clipinfo_c(clientp)					\
    ( ((clientp)->w_version != 0)					\
	? ((short *) wx_clipinfo(clientp))				\
	: ( (wx_shape_flags_c(clientp) & SH_RECT_FLAG)			\
	    ? ((short *) &((clientp)->w_info->u.v0.w_shape.SHAPE_YMIN))	\
	    : ((short *) &((clientp)->w_info->u.v0.w_cliparray)) ) )

#define	wx_devfd(clientp)	((clientp)->w_cdevfd)


/* Jennifer: this thing horses the shape flag field out of the info struct */

#define wx_shape_flags_c(clientp)			\
	((clientp)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(clientp)->w_info +			\
		     (clientp)->w_info->u.vn.w_shapeoff))->SHAPE_FLAGS	\
	    : (clientp)->w_info->u.v0.w_shape.SHAPE_FLAGS)

#define wx_shape_flags(infop)				\
	((infop)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(infop) +			\
		     (infop)->u.vn.w_shapeoff))->SHAPE_FLAGS	\
	    : (infop)->u.v0.w_shape.SHAPE_FLAGS)

/* ..and the shape size field... not sure what its use is */

#define wx_shape_size_c(clientp)			\
	((clientp)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(clientp)->w_info +	\
		     (clientp)->w_info->u.vn.w_shapeoff))->SHAPE_SIZE	\
	    : -1)

#define wx_shape_size(infop)				\
	((infop)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(infop) +			\
		     (infop)->u.vn.w_shapeoff))->SHAPE_SIZE	\
	    : -1)

/* these things get xmax,xmin,ymax,ymin */

#define wx_shape_ymin_c(clientp)			\
	((clientp)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(clientp)->w_info +			\
		     (clientp)->w_info->u.vn.w_shapeoff))->SHAPE_YMIN	\
	    : (clientp)->w_info->u.v0.w_shape.SHAPE_YMIN)

#define wx_shape_ymin(infop)			\
	((infop)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(infop) +			\
		     (infop)->u.vn.w_shapeoff))->SHAPE_YMIN	\
	    : (infop)->u.v0.w_shape.SHAPE_YMIN)

#define wx_shape_ymax_c(clientp)			\
	((clientp)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(clientp)->w_info +			\
		     (clientp)->w_info->u.vn.w_shapeoff))->SHAPE_YMAX	\
	    : (clientp)->w_info->u.v0.w_shape.SHAPE_YMAX)

#define wx_shape_ymax(infop)			\
	((infop)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(infop) +			\
		     (infop)->u.vn.w_shapeoff))->SHAPE_YMAX	\
	    : (infop)->u.v0.w_shape.SHAPE_YMAX)

#define wx_shape_xmin_c(clientp)			\
	((clientp)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(clientp)->w_info +			\
		     (clientp)->w_info->u.vn.w_shapeoff))->SHAPE_XMIN	\
	    : (clientp)->w_info->u.v0.w_shape.SHAPE_XMIN)

#define wx_shape_xmin(infop)			\
	((infop)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(infop) +			\
		     (infop)->u.vn.w_shapeoff))->SHAPE_XMIN	\
	    : (infop)->u.v0.w_shape.SHAPE_XMIN)

#define wx_shape_xmax_c(clientp)			\
	((clientp)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(clientp)->w_info +			\
		     (clientp)->w_info->u.vn.w_shapeoff))->SHAPE_XMAX	\
	    : (clientp)->w_info->u.v0.w_shape.SHAPE_XMAX)

#define wx_shape_xmax(infop)			\
	((infop)->w_version != 0			\
	    ? ((struct class_SHAPE_vn *)		\
		    ((char *)(infop) +			\
		     (infop)->u.vn.w_shapeoff))->SHAPE_XMAX	\
	    : (infop)->u.v0.w_shape.SHAPE_XMAX)

extern	WXCLIENT	*wx_grab() ;
extern	WXCLIENT	*wx_win_grab() ;
extern	short		*wx_clipinfo() ;

#define	_WXGRAB
#endif	_WXGRAB
