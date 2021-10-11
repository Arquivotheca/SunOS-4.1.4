#ifndef lint
static char sccsid[] = "@(#)gpconfig.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986-1989 by Sun Microsystems, Inc.
 */

/*
 * gpconfig.c - binds cg2 or cg9 frame buffers to the gp1
 *	      - downloads microcode on cg12
 */

#include <stdio.h>
#include <strings.h>
#include <varargs.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sun/fbio.h>
#include <sun/gpio.h>

#define	UCODE_DIR	"/usr/lib"
#define N_MAXFB_GP2 4
#define N_MAXFB_GP1 2

#define N_MAXFB (isgp2 ? N_MAXFB_GP1 : N_MAXFB_GP2)

extern char *Progname;
char *basename();
char *sexpand();

#ifdef GPCONFIG_DEBUG
static int gpconfig_debug = 1;
#endif

struct cmd_option {
    int             setgb;	       /* which fb binds gb */
    int             setdevfb;	       /* redirect /dev/fb */
    int             unsetdevfb;	       /* un-redirect /dev/fb */
    int             unbindfb;	       /* unbind a bound framebuffer */
    int             ucodedebug;	       /* debug microcode loading */
    int             ucode_nomemclr;    /* do not clear ucode mem b4 loading */
    char           *ufname;	       /* microcode file name */
    int		    u_choice;		/* which ucode sections to use */
    int             n_fb;	       /* number of cg to bind */
    struct dev_descriptor {
	char           *name;
	int             fd;
    }               fb_file[N_MAXFB_GP2], gp_file;
    int             fb_width;	       /* width of framebuffers */
    caddr_t         gp;		       /* gp registers pointer */
};

static	int             isgp2 = 0;
static	int             iscg12 = 0;
static char    *gpfb_names[] = {
    "/dev/gpone0a",
    "/dev/cgtwo0",
    "/dev/cgnine0",
    "/dev/cgtwelve0"
};

#define GP1_NAME gpfb_names[0]
#define CG2_NAME gpfb_names[1]
#define CG9_NAME gpfb_names[2]
#define CG12_NAME gpfb_names[3]

#define CG2_MAJOR 31
#define CG9_MAJOR 68
#define GP1_MAJOR 32
#define CG12_MAJOR 102

main(argc, argv)
    int             argc;
    char           *argv[];

{
    register int    i;
    register struct cmd_option *cmd;
    short           gp_physaddr;
    int             nbound;
    struct fbinfo   fbinfo;
    struct cmd_option cmd_opt;


    Progname = basename(argv[0]);

    cmd = &cmd_opt;
    options(argc, argv, cmd);	       /* cook command line options */

    if (cmd->unsetdevfb) {	       /* -F option has highest precedence */
	int             devfd;

	if ((devfd = open("/dev/fb", O_RDWR, 0)) >= 0) {
	    int             devfbunit = -1;
	    struct fbtype   dummy;

	    devfbunit = -1;
	    if (ioctl(devfd, GP1IO_REDIRECT_DEVFB, &devfbunit))
		error("failed to reset /dev/fb thru GP");
	    close(devfd);
	    if ((devfd = open("/dev/fb", O_RDWR, 0)) < 0)
		error("failed to open /dev/fb");
	    if (ioctl(devfd, FBIOGTYPE, &dummy) < 0)
		error("/dev/fb is not a graphic device");
	    printf("%s: /dev/fb setup as default\n", Progname);
	}
	return 0;		       /* ignore other options */
    }

    /* get physical address of the GP */
    if (ioctl(cmd->gp_file.fd, FBIOGINFO, &fbinfo))
	error("FBIOGINFO ioctl failed for %s", cmd->gp_file.name);

    /* no need to bind frame buffers if it is cg12 */
    if (iscg12) {
	load_microcode(cmd);
	return 0;
    }

    isgp2 = gp2check(cmd->gp);	       /* see if it's a GP2 */

    /*
     * reset GP, size microcode memory and load microcode
     */
    load_microcode(cmd);

    nbound = bind_frame_buffers(cmd, fbinfo.fb_physaddr >> 16);

    if (!nbound)
	error("no frame buffers bound");
    return 0;
}


/*
 * command line option processing function.
 */
static
options(argc, argv, cmd)
    int             argc;
    char           *argv[];
struct cmd_option *cmd;

{
    register char  *arg;
    register char  *fbname,
                   *gpname;
    int             width = -1;
    int             i;
    int             fd;
    int             unit = 0;
    struct fbgattr   fbattr;

    /*
     * initialize everything to zero and collect all option before any sanity
     * check.
     */
    bzero((caddr_t) cmd, sizeof (*cmd));
    cmd->fb_file[0].fd =
	cmd->fb_file[1].fd =
	cmd->fb_file[2].fd =
	cmd->fb_file[3].fd = -1;
    while (--argc > 0 && *++argv != NULL) {
	arg = *argv;
	if (*arg == '-') {
	    switch (*++arg) {
	    case 'u':		       /* microcode file name follows */
		cmd->ufname = *++argv;
		break;
	    case 'f':		       /* redirect /dev/fb to the next
				        * framebuffer */
		cmd->setdevfb = cmd->n_fb + 1;
		break;
	    case 'b':		       /* use GB */
		cmd->setgb = cmd->n_fb + 1;
		break;
	    case 'F':		       /* un-redirect /dev/fb */
		cmd->unsetdevfb++;
		break;
	    case 'U':		       /* un-bind cg */
		cmd->unbindfb++;
		break;
	    case 'g':		       /* debug microcode file loading */
		cmd->ucodedebug++;
		break;
	    case 'n':		       /* don't clear ucode memory b4 loading */
		cmd->ucode_nomemclr++;
		break;
	    default:
		warning("unknown option -%s", *arg);
		break;
	    }
	}
	else {
	    /*
	     * if the argument looks like a gp, assume it is one, else take
	     * it as a framebuffer name.  People are likely to create new
	     * framebuffer and name it something creative.
	     */
	    if ((!strncmp(arg, CG12_NAME, 13) || !strncmp(arg, CG12_NAME+5, 8))
		&& cmd->gp_file.name == NULL)
		cmd->gp_file.name = arg;
	    else if((!strncmp(arg,GP1_NAME,10) || !strncmp(arg, GP1_NAME+5, 5))
		&& cmd->gp_file.name == NULL)
		cmd->gp_file.name = arg;
	    else if (cmd->n_fb++ < N_MAXFB)
		cmd->fb_file[cmd->n_fb - 1].name = arg;
	}
    }

    /*
     * -F turns off all other options
     */
    if (cmd->unsetdevfb) {
	if (cmd->setdevfb || cmd->setgb || cmd->n_fb > 0 ||
	    cmd->ufname != NULL ||
	    cmd->gp_file.name != NULL)
	    warning("-F turns off other options");
	cmd->unsetdevfb = 1;
	return 0;		       /* this is the only task */
    }
    /*
     * Now is the time for sanity check. The gp device must be named and one
     * of the framebuffer must be present.  We try cg9 before cg2.
     */
    if (cmd->gp_file.name == NULL)
	cmd->gp_file.name = GP1_NAME;
    if ((gpname = cmd->gp_file.name)[0] != '/') {
	gpname = sexpand(gpname, 6);
	(void) sprintf(gpname, "/dev/%sa", cmd->gp_file.name);
	cmd->gp_file.name = gpname;
    }

    if ((fd = open_make(cmd->gp_file.name)) >= 0) {	/* try open gp */

	/* check for cg12 */
	if (iscg12 = cg12check(fd)) {
	
	    cmd->gp_file.fd = fd;

	    if (ioctl(cmd->gp_file.fd, FBIOGATTR, &fbattr) < 0 ||
	    	ioctl(cmd->gp_file.fd, FBIOGTYPE, &fbattr.fbtype) < 0)
		error("%s does not seem to be a frame buffer", gpname);

	    if ((fbattr.real_type != FBTYPE_SUN2GP) &&
		(fbattr.real_type != FBTYPE_SUNGP3))
		error("%s is not a GP device", gpname);

	    return 0;

	} else 
	    close(fd);

    } else {

	cmd->gp_file.name = CG12_NAME;

	if (((fd = open_make(cmd->gp_file.name)) >= 0) 
	    && (iscg12 = cg12check(fd))) {

	    cmd->gp_file.fd = fd;

	    if (ioctl(cmd->gp_file.fd, FBIOGATTR, &fbattr) < 0 ||
	    	ioctl(cmd->gp_file.fd, FBIOGTYPE, &fbattr.fbtype) < 0)
		error("%s does not seem to be a frame buffer", gpname);

	    if ((fbattr.real_type != FBTYPE_SUN2GP) &&
		(fbattr.real_type != FBTYPE_SUNGP3))
		error("%s is not a GP device", gpname);

	    return 0;
	} else
	    exit(0);
    }

    /*
     * There has to be at least one frame buffer to be bound. If none entered
     * from the command line, we take the defaults.
     */
    if (cmd->n_fb <= 0) {
	if ((fd = open_make(CG9_NAME)) >= 0)
	    cmd->fb_file[cmd->n_fb++].name = CG9_NAME;
	else if ((fd = open_make(CG2_NAME)) >= 0)
	    cmd->fb_file[cmd->n_fb++].name = CG2_NAME;
	else
	  /* no framebuffer specified, neither found.
	     Job well done, thank you. */
	  exit(0);
	if (fd >= 0)
	    cmd->fb_file[cmd->n_fb - 1].fd = fd;
    }
    /*
     * Try opening each framebuffer and do some minimum checks. We pact the
     * fb_file array to the front and adjust n_fb, setdevfb, setgb
     * accordingly.
     */
    unit = 0;
    for (i = 0; i < cmd->n_fb; i++) {
	fbname = cmd->fb_file[i].name;
	/* first try to open/create it */
	if ((fd = cmd->fb_file[i].fd) < 0) {
	    if (fbname[0] != '/') {
		fbname = sexpand(fbname, 5);
		(void) sprintf(fbname, "/dev/%s", cmd->fb_file[i].name);
	    }
	    if ((fd = open_make(fbname)) < 0) {
		warning("cannot open frame buffer %s", fbname);
	    }
	}
	if (fd >= 0) {		       /* we have a descriptor, is it any
				        * good? */
	    if (ioctl(fd, FBIOGATTR, &fbattr) ||
		(fbattr.real_type != FBTYPE_SUN2COLOR &&
		 fbattr.real_type != FBTYPE_SUNROP_COLOR)) {
		warning("%s does not seem to be a supported frame buffer",
			fbname);
		close(fd);
		fd = -1;
	    }
	    else {
		/* board must be like all previously bound boards */
		if (width < 0)
		    width = fbattr.fbtype.fb_width;
		else if (width != fbattr.fbtype.fb_width) {
		    warning("%s has the wrong width (%d instead of %d)",
			    fbname, fbattr.fbtype.fb_width, width);
		    close(fd);
		    fd = -1;
		}
	    }
	}
	/* OK, we have a legitimate framebuffer, let's record it */
	if (fd >= 0) {
	    cmd->u_choice += (int) (fbattr.real_type == FBTYPE_SUN2COLOR);
	    cmd->fb_file[unit].fd = fd;
	    cmd->fb_file[unit].name = fbname;
	    unit++;
	}
	if (cmd->setdevfb == (i + 1))
	    cmd->setdevfb = fd >= 0 ? unit : 0;
	if (cmd->setgb == (i + 1))
	    cmd->setgb = fd >= 0 ? unit : 0;
	if (cmd->unbindfb == (i + 1))
	    cmd->unbindfb = fd >= 0 ? unit : 0;
    }
    cmd->n_fb = unit;
    cmd->fb_width = width;

    cmd->gp_file.fd = gp1_open(cmd->gp_file.name, (caddr_t *) & cmd->gp);

    /* zap trailing 'a' */
    gpname[strlen(gpname) - 1] = 0;

#   ifdef GPCONFIG_DEBUG
    if (gpconfig_debug) {
	printf("gpconfig options:\n\tgpunit = %s\n",
	       cmd->gp_file.name);
	for (i = 0; i < cmd->n_fb; i++)
	    printf("\t(%d) %s\n", i + 1, cmd->fb_file[i].name);
	printf("\t/dev/fb redirect to %d\n", cmd->setdevfb);
	printf("\tGB bound to %d\n", cmd->setgb);
	printf("\tUse microcode file : %s\n",
	       cmd->ufname ? cmd->ufname : "default");
	if  (gpconfig_debug & 2)
	    exit(0);
    }
#   endif

    if (ioctl(cmd->gp_file.fd, FBIOGATTR, &fbattr) < 0 ||
    ioctl(cmd->gp_file.fd, FBIOGTYPE, &fbattr.fbtype) < 0)
	error("%s does not seem to be a frame buffer", gpname);

    if (fbattr.real_type != FBTYPE_SUN2GP)
	error("%s is not a GP device", gpname);
    return 0;
}


/* mimic /dev/MAKEDEV function */
#define MAKE_DEVICE(nm, maj, min) {                             \
    dev_t           devnum;                                     \
    devnum = makedev ((maj), (min));                            \
    st = !mknod ((nm), S_IFCHR, devnum) && !chmod ((nm), 0666); \
}

/*
 * try open the device file, create one if none-existing
 * return file descriptor.
 */
static int
open_make(devname)
    char           *devname;
{
    int             fd;

    if ((fd = open(devname, O_RDWR | O_NDELAY, 0)) >= 0)
	return fd;

    if (fd < 0 && errno == ENOENT) {
	int             len = strlen(devname) - 1;
	int             st;
	int             unit;	       /* minor device */

	unit = (int) devname[len] - '0';

	if (!strncmp(devname, CG12_NAME, strlen(CG12_NAME) - 1)) {
	    MAKE_DEVICE(devname, CG12_MAJOR, unit);
	    if (st)
		return open(devname, O_RDWR | O_NDELAY, unit);
	}
	if (!strncmp(devname, CG9_NAME, strlen(CG9_NAME) - 1)) {
	    MAKE_DEVICE(devname, CG9_MAJOR, unit);
	    if (st)
		return open(devname, O_RDWR, unit);
	}
	if (!strncmp(devname, CG2_NAME, strlen(CG2_NAME) - 1)) {
	    MAKE_DEVICE(devname, CG2_MAJOR, unit);
	    if (st)
		return open(devname, O_RDWR, unit);
	}
	if (!strncmp(devname, GP1_NAME, strlen(GP1_NAME) - 1)) {
	    char            remember;
	    int             gpminor;

	    remember = devname[len];
	    unit = (int) (devname[len - 1] - '0');
	    unit <<= 2;
	    for (gpminor = 0; gpminor < 4; gpminor++) {
		devname[len] = 'a' + gpminor;
		MAKE_DEVICE(devname, GP1_MAJOR, gpminor + unit);
	    }
	    if (st) {
		devname[len] = remember;
		return open(devname, O_RDWR, 0);
	    }
	}
    }
    return fd;
}


/*
 * Reset GP or CG12 and load microcodes.
 *
 * GP2 has different sections of microcodes for generic fb or for CG9 only.
 * GP+ and GP2 has different memory size and microcode naming convention.
 */
static
load_microcode(cmd)
    struct cmd_option *cmd;
{
    char            ufnamebuf[48];

    if (isgp2 && !cmd->u_choice)
	gp2_usesection((int) 'X', (int) 'R', (int) 'P', (int) 'S');

    if (!isgp2 && !iscg12) {
	int             memsize;

	gp1_reset(cmd->gp);
	memsize = gp1_sizemem(cmd->gp);
	(void) printf(
		      "%s: %s has %dK of microcode memory\n",
		      Progname, cmd->gp_file.name, memsize);

	/* look for appropriate microcode file */
	while (!cmd->ufname && memsize >= 8) {
	    (void) sprintf(cmd->ufname = ufnamebuf,
			   "%s/gp1%s%scg2.%d.ucode",
			   UCODE_DIR,
			   memsize >= 32 ? "+" : "",
			   memsize >= 16 ? "+" : "",
			   cmd->fb_width);

	    if (access(cmd->ufname, R_OK)) {
		cmd->ufname = 0;
		memsize >>= 1;
	    }
	}

	gp1_load(cmd->gp, cmd->ufname);
    }
    else if (!iscg12) {
	gp2_reset(cmd->gp);
	printf("%s: %s is a GP2\n", Progname, cmd->gp_file.name);

	if (!cmd->ufname)
	    (void) sprintf(cmd->ufname = ufnamebuf,
			   "%s/gp2.ucode", UCODE_DIR);
	gp2_load(cmd->gp, cmd->ufname, cmd->ucodedebug);
    }
    else {

	cg12_reset(cmd->gp_file.fd, 1);

	if (iscg12 == 3)
	    printf("%s: %s is a GSXR\n",
	    Progname, cmd->gp_file.name);
	else if (strncmp(cmd->gp_file.name, CG12_NAME, 13) || (iscg12 == 2))
	    printf("%s: %s is a CG12%s\n",
	    Progname, cmd->gp_file.name, (iscg12 == 2) ? "+" : "");

	if (!cmd->ufname)
	    (void) sprintf(cmd->ufname = ufnamebuf,
			   "%s/gs.ucode", UCODE_DIR);

	if(cg12_load(cmd->gp_file.fd, cmd->ufname, cmd->ucodedebug, cmd->ucode_nomemclr)) {

	    (void) printf("%s: microcode file %s not loaded into %s\n",
		Progname, cmd->ufname, cmd->gp_file.name);

		return 0;
	} else {
	    cg12_reset(cmd->gp_file.fd,0);
	    cg12_ucode_ver(cmd->gp_file.fd);
	}
    }

    (void) printf("%s: microcode file %s loaded into %s\n",
		  Progname, cmd->ufname, cmd->gp_file.name);

}

/*
 * at this point, microcode file has been loaded, we have several
 * framebuffers to be bound to the GP.  These framebuffers have been
 * opened and the file descriptors are stored in "cmd" structure.
 */
static
bind_frame_buffers(cmd, gp_physaddr)
    struct cmd_option *cmd;
    short           gp_physaddr;
{
    short           fb_physaddr[4];
    struct fbinfo   fbinfo;
    int             i;
    int             fbunit = -1;
    int             nbound = 0;
    int             bufunit = -1;
    int             devfbunit = -1;

    /* initialize frame buffer physical addresses */
    fb_physaddr[0] = -1;
    fb_physaddr[1] = -1;
    fb_physaddr[2] = -1;
    fb_physaddr[3] = -1;

    /* set the fb unit using the gbuffer (if present) */
    if (cmd->setgb) {
	int             gbufflag = 0;

	if (ioctl(cmd->gp_file.fd, GP1IO_CHK_FOR_GBUFFER, &gbufflag) &&
	    gbufflag) {
	    printf("%s: no graphics buffer present\n", Progname);
	    cmd->setgb = 0;
	}
    }

    for (i = 0; i < cmd->n_fb; i++) {
	/* can only configure 4 frame buffers */
	if (++fbunit >= (isgp2 ? 2 : 4)) {
	    warning(
		    "cannot bind %s; %d frame buffers already bound",
		    cmd->fb_file[i].name, --fbunit);
	    if (cmd->setdevfb == i + 1)
		cmd->setdevfb = 0;
	    if (cmd->setgb == i + 1)
		cmd->setgb = 0;
	    continue;
	}

	/* get frame buffer physical address */
	if (ioctl(cmd->fb_file[i].fd, FBIOGINFO, &fbinfo)) {
	    warning("FBIOGINFO ioctl failed for %s", cmd->fb_file[i].name);
	    if (cmd->setdevfb == i + 1)
		cmd->setdevfb = 0;
	    if (cmd->setgb == i + 1)
		cmd->setgb = 0;
	    continue;
	}

	/* see if fb unit is already bound */
	for (i = 0; i < fbunit; i++)
	    if ((fbinfo.fb_physaddr >> 16) == fb_physaddr[i]) {
		error("cannot bind the same fb (%s) twice",
		      cmd->fb_file[i].name);
		break;
	    }

	if (i < fbunit) {

	    if (cmd->setdevfb == i + 1)
		cmd->setdevfb = 0;
	    if (cmd->setgb == i + 1)
		cmd->setgb = 0;
	    continue;
	}

	/* OK, bind it!  (or un-bind it) */
	if (cmd->unbindfb == (i + 1))
	    fbunit |= GP1_UNBIND_FBUNIT;
	fbinfo.fb_unit = fbunit;
	if (ioctl(cmd->gp_file.fd, GP1IO_PUT_INFO, &fbinfo)) {
	    warning("cannot bind %s to %s",
		    cmd->fb_file[i].name, cmd->gp_file.name);
	    if (cmd->setdevfb == i + 1)
		cmd->setdevfb = 0;
	    if (cmd->setgb == i + 1)
		cmd->setgb = 0;
	    continue;
	}
	fb_physaddr[fbunit] = fbinfo.fb_physaddr >> 16;

	/* another board has been successfully bound */
	(void) printf("%s: %s and %s bound as %s%c\n",
		      Progname, cmd->gp_file.name,
		      cmd->fb_file[i].name, cmd->gp_file.name,
		      'a' + fbunit);

	/* done with the fb unit for now */
	(void) close(cmd->fb_file[i].fd);
	cmd->fb_file[i].fd = -1;

	if (cmd->setdevfb && cmd->setdevfb == i + 1)
	    cmd->setdevfb = fbunit + 1;
	if (cmd->setgb && cmd->setgb == i + 1)
	    cmd->setgb = fbunit + 1;
    }

    bzero((caddr_t) & fbinfo, sizeof (fbinfo));

    /* clean up residue */
    for (; i < N_MAXFB; i++) {
	fbinfo.fb_unit = i | GP1_UNBIND_FBUNIT;
	(void) ioctl(cmd->gp_file.fd, GP1IO_PUT_INFO, &fbinfo);
    }

    /* configure and start the GP */
    fbunit++;
    if (fbunit) {
	if (!isgp2) {
	    gp1_fb_config(cmd->gp, gp_physaddr, fb_physaddr,
			  4, bufunit);
	    gp1_start(cmd->gp);
	}
	else
	    gp2_start(cmd->gp);

	if ((cmd->setgb)-- > 0)	       /* bind GB */
	    if (!ioctl(cmd->gp_file.fd, GP1IO_SET_USING_GBUFFER, &cmd->setgb))
		(void) printf("%s: %s%c will use the graphics buffer\n",
			      Progname, cmd->gp_file.name, 'a' + cmd->setgb);

	if ((cmd->setdevfb)-- > 0)     /* redirect /dev/fb */
	    if (ioctl(cmd->gp_file.fd, GP1IO_REDIRECT_DEVFB, &cmd->setdevfb))
		warning("cannot redirect /dev/fb");
	    else
		(void) printf("%s: /dev/fb redirected to %s%c\n",
			  Progname, cmd->gp_file.name, 'a' + cmd->setdevfb);
    }
    return fbunit;
}
