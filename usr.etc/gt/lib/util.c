#ifndef lint
static  char sccsid[] = "@(#)util.c 1.1 94/10/31 SMI";
#endif

#include <stdio.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/types.h>
#include <varargs.h>

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntlcom.h>
#include <fcntl.h>
#include <fbio.h>
#include <gtreg.h>
#include <gtconfig.h>

extern int  errno;
extern int  sys_nerr;
extern char *sys_errlist[];

extern char *progname;
extern char *usage[];
extern int   usageflag;

extern short verbose_flag;
extern short have_mappings;

extern struct gt_ctrl gt_ctrl;


#define OPENBOOT_AS		0x3		/* byte access mode	*/
#define OPENBOOT_BUF_SEL	0x0		/* buffer select	*/

#define	EVEN	(~0x1)


/*
 * This is the minimal context that must be set up before
 * the OpenBoot PROM writes characters to the screen.
 */
struct context {
	unsigned c_as;
	unsigned c_buf_sel;
	unsigned c_pbm_csr;
} context;


/*
 * Must be sure to save/restore state assumed by OpenBoot PROM
 * before chattering away.
 */
verbose(msg, arg0, arg1, arg2, arg3, arg4)
	char *msg;
	unsigned arg0, arg1, arg2, arg3, arg4;
{
	if (verbose_flag) {
		gt_fprintf(stdout, "%s: ", progname);
		gt_fprintf(stdout, msg, arg0, arg1, arg2, arg3, arg4);
	}
}


gt_perror(fmt, x1, x2, x3, x4)
	char *fmt;
	unsigned x1, x2, x3, x4;
{
	gt_fprintf(stderr, "%s: ", progname);
	gt_fprintf(stderr, fmt, x1, x2, x3, x4);

	if (errno) {
		if (errno >= 0 && errno < sys_nerr-1) {
			gt_fprintf(stderr, " (%s)", sys_errlist[errno]);
		}
	}

	fputc('\n', stderr);

	if (usageflag)
		printusage();

	exit(1);
}


gt_compare_err(section, offset, expected, was)
	char *section;
	unsigned offset, expected, was;
{
	gt_fprintf(stderr,
	    "section: %s  offset: 0x%X  expected: 0x%X  was: 0x%X\n",
	    section, offset, expected, was);
}


printusage()
{
	char **msg = usage;

	gt_fprintf(stderr, "usage: %s ", progname);

	while (*msg != (char *)NULL)
		gt_fprintf(stderr, *msg++);

	exit(1);
}


gt_fprintf(fmt, x1, x2, x3, x4, x5, x6, x7, x8)
	char *fmt;
	unsigned x1, x2, x3, x4, x5, x6, x7, x8;
{
	save_context();

	fprintf(fmt, x1, x2, x3, x4, x5, x6, x7, x8);

	restore_context();
}


save_context()
{
	if (have_mappings) {
		context.c_as      = gt_ctrl.rp_host_regs->rp_host_as_reg;
		context.c_buf_sel = gt_ctrl.fbi_regs->fbi_reg_buf_sel;
		context.c_pbm_csr = gt_ctrl.rp_host_regs->rp_host_csr_reg;

		gt_ctrl.rp_host_regs->rp_host_as_reg   = OPENBOOT_AS;
		gt_ctrl.fbi_regs->fbi_reg_buf_sel      = OPENBOOT_BUF_SEL;
		gt_ctrl.rp_host_regs->rp_host_csr_reg &= EVEN;
	}
}


restore_context()
{
	if (have_mappings) {
		gt_ctrl.rp_host_regs->rp_host_as_reg  = context.c_as;
		gt_ctrl.fbi_regs->fbi_reg_buf_sel     = context.c_buf_sel;
		gt_ctrl.rp_host_regs->rp_host_csr_reg = context.c_pbm_csr;
        }
}

