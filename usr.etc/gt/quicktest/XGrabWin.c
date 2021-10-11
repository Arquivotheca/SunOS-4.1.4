#ifndef line
static char sccsid[] = "@(#)XGrabWin.c  1.1 94/10/31 GT object demo SMI";
#endif
/*-
 *-----------------------------------------------------------------------
 * XGrabWin.c - X11 Client Side interface to Window Grabber.
 * 
 * This code uses the standard R3 extension mechanism for sending the grab or
 * ungrab window requests. If the extension isn't present, it uses the un-used
 * protocol if the server is from Sun.
 *
 *-----------------------------------------------------------------------
 */

#define	NEED_REPLIES
#define NEED_EVENTS

#include <X11/Xlibint.h>	/* from usr.lib/libX11 */
#include <X11/Xproto.h>		/* from usr.lib/libX11/include */
#include "wx.h"
#include "win_grab.h"

#ifdef SVR4

#include <netinet/in_systm.h>
#include <sys/time.h>
#include <sys/resource.h>

#else

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>  
#include <netinet/tcp.h>
#include <sys/types.h>

#endif

#ifdef __STDC__
static int WxError (Display *dpy, int mc);
#else
static int WxError ();
#endif
				/* Unused requests	*/
#define X_GrabWindow	125	/* should be in Xproto.h */
#define X_UnGrabWindow	126	/* just before X_NoOperation */

#define BadCookie	0
static int X_WxExtensionCode;

#define sz_xOWGXReq 12
#define X_OWGX X_WxExtensionCode

static enum _wx_state {
  NOT_INITIALIZED=0,	/* Assumption: bzero() places this value */
  USE_EXTENSION,
  USE_EXTRA_PROTOCOL,
  NOT_LOCAL_HOST
  } ;


static enum _wx_state *WxInitTable = 0;	/* No table to start with */

static int WxInitTableSize=0;
  
static
Initialize(dpy)
     register Display *dpy;
{
  char hostname[128];
  char *colan;
  int tmp, size, hostlen, phostlen, namelen;
  struct hostent *phost;
  union genericaddr {
    struct sockaddr	generic;
    struct sockaddr_in	internet;
    struct sockaddr_un	nuxi;
  } addr;
  struct hostent *h;
  
  if (!WxInitTable || WxInitTableSize < dpy->fd) {
    /* We've got to allocate or grow the table */

#ifdef SVR4
    struct rlimit	rip;
    size = (getrlimit(RLIMIT_NOFILE, &rop) < 0) ? -1 : rip.rlim_cur;
#else
    size = getdtablesize();
#endif

    WxInitTable = (enum _wx_state*)
      ((WxInitTableSize) ?
	realloc(WxInitTable,sizeof(enum _wx_state) * size) :
        malloc(sizeof(enum _wx_state) * size));
    /* We don't zero because 0 is a valid fd */
    bzero(WxInitTable, sizeof(enum _wx_state)*size);
    WxInitTableSize = size;
  }
    
  if (dpy->display_name[0] != ':') {
    colan = index(dpy->display_name,':');
    phostlen = strlen(dpy->display_name) - (colan ? strlen(colan) : 0);

    if (strncmp("unix",dpy->display_name,MIN(phostlen,4)) &&
	strncmp("localhost",dpy->display_name,MIN(phostlen,9))) {

      /* We're not a unix domain connection so what are we ? */

      namelen = sizeof addr;
      bzero((char *) &addr, sizeof addr);

      if (getpeername(dpy->fd, (struct sockaddr *) &addr, &namelen) < 0)
	goto bad_access;
      else {

	if ((addr.generic.sa_family !=  AF_INET) ||
	    ((h = gethostbyaddr(&addr.internet.sin_addr,
				sizeof addr.internet.sin_addr,
				addr.internet.sin_family)) == 0))
	  goto bad_access;

	/* Find real host name on TCPIP */
	phost = gethostbyname(h->h_name);
	phostlen = strlen(phost->h_name);
	gethostname(hostname,128);
	hostlen = strlen(hostname);
      
	if (strncmp(hostname,phost->h_name,MIN(phostlen,hostlen))) {
bad_access:	
	  WxInitTable[dpy->fd] = NOT_LOCAL_HOST;
	  return;
	}
      }
    }
  }
  if (XQueryExtension(dpy, "SunWindowGrabber",&X_WxExtensionCode,
		      &tmp, &tmp))
    WxInitTable[dpy->fd] = USE_EXTENSION;
  else if (!strcmp(dpy->vendor,"X11/NeWS - Sun Microsystems Inc."))
    WxInitTable[dpy->fd] = USE_EXTRA_PROTOCOL;
}


static	int	(*old_handler)() ;
static	Display	*grabreq_dpy ;
static	int	grabreq_seq ;
static	int	grab_failed ;
static	int	installed = 0 ;

static int
grab_handler(dpy,err)
    Display	*dpy ;
    XErrorEvent	*err ;
{
	if( dpy == grabreq_dpy  &&  err->request_code == X_WxExtensionCode ) {
	  if( err->serial == grabreq_seq )
	    grab_failed = 1 ;
	  return 0 ;
	} else
	  return old_handler(dpy,err) ;
}


static
install_handler(dpy)
	Display	*dpy ;
{
	grabreq_dpy = dpy ;
	grabreq_seq = NextRequest(dpy) ;
	grab_failed = 0 ;
	if( !installed ) {
	  old_handler = XSetErrorHandler(grab_handler) ;
	  installed = 1 ;
	}
}


/* IFF the current handler is grab_handler(), replace it with
 * old_handler.  If not, someone has over-ridden us.  Just ignore it.
 */

static
uninstall_handler()
{
	int	(*current_handler)() ;

	if( !installed )
	  return ;

	current_handler = XSetErrorHandler(old_handler) ;
	if( current_handler != grab_handler )	/* oops, put it back */
	  (void) XSetErrorHandler(current_handler) ;
	else
	  installed = 0 ;
}


int 
XGrabWindowNew(dpy, win, version)
     register Display *dpy;
     register Window win;
     int      version ;
{
  register xResourceReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:
    LockDisplay(dpy);
    install_handler(dpy) ;
    GetResReq(WxExtensionCode, win, req);
    req->pad = version == 1 ? X_WxGrab_New : X_WxGrab ;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    if( !grab_failed )
      uninstall_handler() ;
    UnlockDisplay(dpy);
    SyncHandle();
    return grab_failed ? 0 : rep.data00;
  case USE_EXTRA_PROTOCOL:
    LockDisplay(dpy);
    GetResReq(GrabWindow, win, req);
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.data00);	/* GrabToken */
  case NOT_INITIALIZED:
     WxError(dpy,X_WxGrab);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

int
XUnGrabWindowNew(dpy, win, version)
    register Display *dpy;
    register Window win;
    int      version ;
{
  register xResourceReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:
    LockDisplay(dpy);
    GetResReq(WxExtensionCode, win, req);
    req->pad = version == 1 ? X_WxUnGrab_New : X_WxUnGrab;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();
    uninstall_handler() ;
    return rep.data00;		/* Status */
  case USE_EXTRA_PROTOCOL:
    LockDisplay(dpy);
    GetResReq(UnGrabWindow, win, req);
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.data00);	/* Status */
  case NOT_INITIALIZED:
    WxError(dpy,X_WxUnGrab);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}


int 
XGrabWindow(dpy, win)
     Display *dpy;
     Window win;
{
	return XGrabWindowNew(dpy,win,0) ;
}

int
XUnGrabWindow(dpy, win)
    Display *dpy;
    Window win;
{
	return XUnGrabWindowNew(dpy,win,0) ;
}


int 
XGrabColormapNew(dpy, cmap, version)
     register Display *dpy;
     register Colormap cmap;
     int	version ;
{
  register xResourceReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:
    LockDisplay(dpy);
    install_handler(dpy) ;
    GetResReq(WxExtensionCode, cmap, req);
    req->pad = version == 1 ? X_WxGrabColormapNew : X_WxGrabColormap;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    if( !grab_failed )
      uninstall_handler() ;
    UnlockDisplay(dpy);
    SyncHandle();
    return grab_failed ? 0 : rep.data00;
  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxGrabColormap);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

int
XUnGrabColormapNew(dpy, cmap, version)
     register	Display *dpy;
     register	Colormap cmap;
     int	version ;
{
  register xResourceReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:
    LockDisplay(dpy);
    GetResReq(WxExtensionCode, cmap, req);
    req->pad = version == 1 ? X_WxUnGrabColormapNew : X_WxUnGrabColormap;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();
    uninstall_handler() ;
    return rep.data00;		/* Status */
  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxGrabColormap);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

int
XGrabColormap(dpy, win)
    Display	*dpy ;
    Window	win ;
{
	return XGrabColormapNew(dpy,win,0) ;
}


int
XUnGrabColormap(dpy, win)
    Display	*dpy ;
    Window	win ;
{
	return XUnGrabColormapNew(dpy,win,0) ;
}


static int
WxError (dpy,mc)
     Display *dpy;
     int mc;
{
  XErrorEvent event;
  extern int (*_XErrorFunction)();

  event.display = dpy;
  event.type = X_Error;
  event.error_code = BadImplementation;
  event.request_code = 0xff;	/* Means that we were requesting an extension*/
  event.minor_code = mc;
  event.serial = dpy->request;
  if (_XErrorFunction != NULL) {
    return ((*_XErrorFunction)(dpy, &event));
  }
  exit(1);
  /*NOTREACHED*/
}

int 
XGrabWids(dpy,win,nwids)
     register Display *dpy;
     register Window  win;
     int	      nwids;
{
  register xOWGXReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:

    LockDisplay(dpy);
    GetReqExtra(OWGX,0,req);
    req->pad = X_WxGrabWids;
    req->id = win;
    req->number_objects = nwids;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();

    return(rep.data00);

  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxGrabWids);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

int 
XGrabFCS(dpy,win,nfcs)
     register Display *dpy;
     register Window  win;
     int	      nfcs;
{
  register xOWGXReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:

    LockDisplay(dpy);
    GetReqExtra(OWGX,0,req);
    req->pad = X_WxGrabFCS;
    req->id = win;
    req->number_objects = nfcs;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();

    return(rep.data00);

  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxGrabFCS);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

int 
XGrabZbuf(dpy,win,nzbuftype)
     register Display *dpy;
     register Window  win;
     int	      nzbuftype;
{
  register xOWGXReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:

    LockDisplay(dpy);
    GetReqExtra(OWGX,0,req);
    req->pad = X_WxGrabZbuf;
    req->id = win;
    req->number_objects = nzbuftype;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();

    return(rep.data00);

  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxGrabZbuf);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

int 
XGrabStereo(dpy,win,st_mode)
     register Display *dpy;
     register Window  win;
     int	      st_mode;
{
  register xOWGXReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:

    LockDisplay(dpy);
    GetReqExtra(OWGX,0,req);
    req->pad = X_WxGrabStereo;
    req->id = win;
    req->number_objects = st_mode;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();

    return(rep.data00);

  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxGrabStereo);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

int 
XGrabBuffers(dpy,win,nbuffers)
     register Display *dpy;
     register Window  win;
     int	      nbuffers;
{
  register xOWGXReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:

    LockDisplay(dpy);
    GetReqExtra(OWGX,0,req);
    req->pad = X_WxGrabBuffers;
    req->id = win;
    req->number_objects = nbuffers;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();

    return(rep.data00);

  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxGrabBuffers);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}


int
XUnGrabBuffers(dpy,win,clientp)
     register Display *dpy;
     register Window win;
     WXCLIENT *clientp;
{
  register xResourceReq *req;
  xGenericReply rep;

  if (!WxInitTable || WxInitTableSize <= dpy->fd ||
      WxInitTable[dpy->fd] == NOT_INITIALIZED)
    Initialize(dpy);

  switch (WxInitTable[dpy->fd]) {
  case USE_EXTENSION:

/*  Generate protocol request to server to release  its token
    from the counter page.
*/
    LockDisplay(dpy);
    GetResReq(WxExtensionCode,win,req);
    req->pad = X_WxUnGrabBuffers;
    (void) _XReply(dpy, (xReply *) &rep, 0, xFalse);
    UnlockDisplay(dpy);
    SyncHandle();

    return(rep.data00);

  case USE_EXTRA_PROTOCOL:
  case NOT_INITIALIZED:
    WxError(dpy,X_WxUnGrabBuffers);
  case NOT_LOCAL_HOST:
    return BadCookie;
  }
}

