#ifndef lint
static char sccsid[] = "@(#)pr_io.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1986 by Sun Microsystems, Inc.
 */

/*
 * Extended pixrect operations	-- dump pixrect to file
 *				-- load pixrect from file
 * 
 * Limitations:	Clients (callers) of pr_dump are responsible for locking the
 *		display, as the  pr_rop() operation is not atomic with respect
 *		to display updates by other processes, and there is no
 *		reliable way for pr_dump to do the locking.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>
#include <pixrect/pr_io.h>
#include <rasterfile.h>

extern char *malloc();

#define FD_COPY(p, q) bcopy((char *)(p), (char *)(q), sizeof(*(p)))


/*
 * Dump a pixrect to the specified stream.
 */
int
pr_dump(input_pr, output, colormap, type, copy_flag)
	register Pixrect *input_pr;
	FILE *output;
	colormap_t *colormap;
	int type, copy_flag;
{
	int result = 0;
	Pixrect *dump_pr = 0;
	struct rasterfile rh;
	colormap_t local_colormap;

	if (input_pr == 0 || output == NULL)
		return PIX_ERR;

	/*
	 * If the colormap pointer is NULL and the input pixrect is
	 * a display more than 1 bit deep, try to get its colormap.
	 */
	if (colormap == NULL && 
		input_pr->pr_depth > 1 &&
		(MP_NOTMPR(input_pr) || 
			mpr_d(input_pr)->md_flags & MP_DISPLAY)) {
		register int len;

		local_colormap.type = RMT_EQUAL_RGB;
		local_colormap.length = len =
		    input_pr->pr_depth <= 8 ?
			1 << input_pr->pr_depth :
			256;
		
		if (!(local_colormap.map[0] = 
			(u_char *) malloc((unsigned) len * 3)))
			return PIX_ERR;

		local_colormap.map[1] = local_colormap.map[0] + len;
		local_colormap.map[2] = local_colormap.map[1] + len;

		if( input_pr->pr_depth <= 8 )
		{
		    if (pr_getcolormap(input_pr, 0, len,
				       local_colormap.map[0], 
				       local_colormap.map[1], 
				       local_colormap.map[2]) == 0)
			colormap = &local_colormap;
		}
		else
		{
		    if (pr_getlut(input_pr, 0, len,
				  local_colormap.map[0], 
				  local_colormap.map[1], 
				  local_colormap.map[2]) == 0)
			colormap = &local_colormap;
		}
	}

	/*
	 * Initialize the rasterfile header and copy the input pixrect
	 * if necessary.
	 */
	if (dump_pr = 
		pr_dump_init(input_pr, &rh, colormap, type, copy_flag)) 
		switch (type) {
		case RT_OLD:
		case RT_STANDARD:
		case RT_BYTE_ENCODED:
			if (!(result = pr_dump_header(output, &rh, colormap)))
				result = dump_image(dump_pr, output, &rh);
			break;

		default: 
			rh.ras_type = RT_STANDARD;
			result = dump_filtered(dump_pr, output, 
				&rh, colormap, type);
			break;
		}

	if (colormap == &local_colormap && local_colormap.map[0])
		(void) free((char *) local_colormap.map[0]);

	if (dump_pr && dump_pr != input_pr)
		(void) pr_destroy(dump_pr);

	return result;
}

/*
 * Initialize rasterfile header and output pixrect for pr_dump.
 */
Pixrect *
pr_dump_init(input_pr, rh, colormap, type, copy_flag)
	register Pixrect *input_pr;
	struct rasterfile *rh;
	colormap_t *colormap;
	int type, copy_flag;
{
	Pixrect *copy_pr = 0;
	register struct mpr_data *mprd;

	if (input_pr == 0 || rh == 0)
		return 0;

	if (colormap)
		switch (colormap->type) {
		case RMT_NONE:
		case RMT_EQUAL_RGB:
		case RMT_RAW:
			break;

		default:
			return 0;
	}

	/*
	 * If input pixrect is not a primary memory pixrect, or we are
	 * run length encoding or filtering and it is not 16-bit padded,
	 * we have to copy it regardless of copy_flag.
	 */
	mprd = mpr_d(input_pr);
	if (copy_flag ||
		MP_NOTMPR(input_pr) ||
		mprd->md_offset.x != 0 || mprd->md_offset.y != 0 ||
		mprd->md_flags & MP_REVERSEVIDEO ||
		(type != RT_OLD && type != RT_STANDARD && 
		mpr_prlinebytes(input_pr) != mprd->md_linebytes)) {

		short *image;

		if (!(image = (short *) malloc((unsigned) 
			mpr_prlinebytes(input_pr) * input_pr->pr_size.y)))
			return 0;

		if (!(copy_pr = 
			mem_point(input_pr->pr_size.x, input_pr->pr_size.y,
				input_pr->pr_depth, image))) {
			(void) free((char *) image);
			return 0;
		}

		mpr_d(copy_pr)->md_primary = 1;

		if (pr_rop(copy_pr, 0, 0, 
			input_pr->pr_size.x, input_pr->pr_size.y, 
			PIX_SRC | PIX_DONTCLIP, 
			input_pr, 0, 0)) {
			(void) pr_destroy(copy_pr);
			return 0;
		}

		input_pr = copy_pr;
	}

	/*
	 * Initialize rasterfile header.
	 */
	rh->ras_magic  = RAS_MAGIC;
	rh->ras_width  = input_pr->pr_size.x;
	rh->ras_height = input_pr->pr_size.y;
	rh->ras_depth  = input_pr->pr_depth;

	switch (rh->ras_type = type) {
	case RT_OLD:
		rh->ras_length = 0;
		break;

	case RT_STANDARD:
	default:
		rh->ras_length = 
			mpr_prlinebytes(input_pr) * input_pr->pr_size.y;
		break;

	case RT_BYTE_ENCODED:
		rh->ras_length = dump_encoded(input_pr, (FILE *) NULL);
		break;
	}

	rh->ras_maptype = RMT_NONE;
	rh->ras_maplength = 0;

	if (colormap)
		switch (rh->ras_maptype = colormap->type) {
		case RMT_NONE:
			break;

		case RMT_EQUAL_RGB:
			rh->ras_maplength = colormap->length * 3;
			break;

		case RMT_RAW:
			rh->ras_maplength = colormap->length;
			break;
		}

	return input_pr;
}

/*
 * Dump the header to the specified stream.
 */
int
pr_dump_header(output, rh, colormap)
	FILE *output;
	register struct rasterfile *rh;
	register colormap_t *colormap;
{
	register int len = 0;

	if (output == NULL || rh == 0)
		return PIX_ERR;

	/* validate colormap if present */
	if (colormap) {
		if (colormap->type != rh->ras_maptype)
			return PIX_ERR;

		switch (colormap->type) {
		case RMT_NONE:		len = 0;  break;
		case RMT_EQUAL_RGB:	len = 3;  break;
		case RMT_RAW:		len = 1;  break;
		default:		return PIX_ERR;
		}

		if (colormap->length * len != rh->ras_maplength)
			return PIX_ERR;

		len = colormap->length;
	}

	/* write rasterfile header, colormap if any */
	return fwrite((char *) rh, 1, sizeof *rh, output) != sizeof *rh ||
		len && (fwrite((char *) colormap->map[0], 
				1, len, output) != len ||
			colormap->type == RMT_EQUAL_RGB && (
				fwrite((char *) colormap->map[1], 
					1, len, output) != len ||
				fwrite((char *) colormap->map[2], 
					1, len, output) != len))
		? PIX_ERR : 0;
}

/*
 * Dump the image data to the specified stream.
 */
int
pr_dump_image(input_pr, output, rh)
	Pixrect *input_pr;
	FILE *output;
	struct rasterfile *rh;
{
	Pixrect *dump_pr = 0;
	struct rasterfile dummy_rh;
	int result;

	if (input_pr == 0 || output == NULL || rh == 0)
		return PIX_ERR;

	switch (rh->ras_type) {
	case RT_OLD:
	case RT_STANDARD:
	case RT_BYTE_ENCODED:
		break;

	default:
		return PIX_ERR;
	}

	/* ensure pixrect is directly dumpable */
	if (!(dump_pr = pr_dump_init(input_pr, &dummy_rh, (colormap_t *) 0,
		rh->ras_type, 0)))
		return PIX_ERR;

	result = dump_image(dump_pr, output, rh);

	if (dump_pr != input_pr)
		(void) pr_destroy(dump_pr);

	return result;
}

static int
dump_image(pr, out, rh)
	register Pixrect *pr;
	FILE *out;
	struct rasterfile *rh;
{
	if (rh->ras_type == RT_BYTE_ENCODED) {
		clearerr(out);
		(void) dump_encoded(pr, out);
		return ferror(out) ? PIX_ERR : 0;
	}

	{
		register struct mpr_data *mprd = mpr_d(pr);
		register char *image = (char *) mprd->md_image;
		register int bytes = mpr_prlinebytes(pr);
		register int lines = pr->pr_size.y;

		if (bytes == mprd->md_linebytes) {
			bytes *= lines;
			lines = 1;
		}

		while (--lines >= 0) {
			if (fwrite(image, 1, bytes, out) != bytes)
				return PIX_ERR;
			image += mprd->md_linebytes;
		}
	}

	return 0;
}


/*
 * Load a pixrect from the specified stream.
 */
Pixrect *
pr_load(input, colormap)
	FILE *input;
	colormap_t *colormap;
{
	struct rasterfile rh;

	if (pr_load_header(input, &rh) ||
		pr_load_colormap(input, &rh, colormap))
		return 0;

	return pr_load_image(input, &rh, colormap);
}

/*
 * Load the header from the specified stream.
 */
int
pr_load_header(input, rh)
	FILE *input;
	register struct rasterfile *rh;
{
	return input && rh &&
		fread((char *) rh, 1, sizeof *rh, input) == sizeof *rh &&
		rh->ras_magic == RAS_MAGIC 
		? 0 : PIX_ERR;
}

/*
 * Load the color map from the specified stream.
 */
int
pr_load_colormap(input, rh, user_colormap)
	FILE *input;
	register struct rasterfile *rh;
	colormap_t *user_colormap;
{
	register int len, error = PIX_ERR;
	register colormap_t *colormap;

	if (input == NULL || rh == 0)
		return error;

	if (colormap = user_colormap) {
		colormap_t local_colormap;

		len = colormap->length;

		/* 
		 * If type or length in colormap does not match rasterfile
		 * header, create a new colormap struct.
		 */
		if (rh->ras_maptype != colormap->type ||
			rh->ras_maplength != 
			len * (colormap->type == RMT_EQUAL_RGB ? 3 : 1)) {

			len = rh->ras_maplength;
			colormap = &local_colormap;

			colormap->map[0] = 0;
			if (len && !(colormap->map[0] = 
				(u_char *) malloc((unsigned) len)))
				return error;

			colormap->type = rh->ras_maptype;
			if (colormap->type == RMT_EQUAL_RGB) {
				len /= 3;
				colormap->map[1] = colormap->map[0] + len;
				colormap->map[2] = colormap->map[1] + len;
			}

			colormap->length = len;
		}

		/* read colormap data, if any */
		if (len == 0 || colormap->type == RMT_NONE ||
			fread((char *) colormap->map[0],
				1, len, input) == len && (
				colormap->type != RMT_EQUAL_RGB ||
				fread((char *) colormap->map[1], 
					1, len, input) == len &&
				fread((char *) colormap->map[2], 
					1, len, input) == len))
			error = 0;

		/* copy new colormap over user's */
		if (colormap == &local_colormap) 
			if (error) {
				if (colormap->map[0])
					(void) free((char *) colormap->map[0]);
			}
			else
				*user_colormap = *colormap;
	}
	else {
		/* user doesn't want colormap, discard it */

		len = rh->ras_maplength;
		while (--len >= 0 && getc(input) != EOF)
			/* nothing */ ;

		if (len < 0)
			error = 0;
	}

	return error;
}

/*
 * Load pixrect image from the specified header and stream.
 */
Pixrect *
pr_load_image(input, rh, colormap)
	FILE *input;
	register struct rasterfile *rh;
	colormap_t *colormap;
{
	Pixrect *pr = 0;
	register char *image;
	register int imagesize;
	Pixrect *mem32pr, *image24to32();

	if (input == NULL || rh == 0)
		return 0;

	imagesize = 
		mpr_linebytes(rh->ras_width, rh->ras_depth) * rh->ras_height;

	if (!(image = malloc((unsigned) imagesize)))
		return pr;

	if (!(pr = mem_point(rh->ras_width, rh->ras_height, 
		rh->ras_depth, (short *) image))) {
		(void) free(image);
		return pr;
	}

	mpr_d(pr)->md_primary = 1;

	switch (rh->ras_type) {
	case RT_OLD:
	case RT_STANDARD: 
		if (fread(image, 1, imagesize, input) == imagesize &&
		    rh->ras_depth != 24)
			return pr;
		if ((mem32pr = image24to32(rh, image)) != NULL) {
		    (void) pr_destroy(pr);
		    return mem32pr;
		}
		break;
	    
	case RT_BYTE_ENCODED: 
		if (read_encoded(input, rh->ras_length, 
			(u_char *) image, imagesize) == 0 &&
			rh->ras_depth != 24)
			return pr;
		if ((mem32pr = image24to32(rh, image)) != NULL) {
		    (void) pr_destroy(pr);
		    return mem32pr;
		}
		break;

	default:
		if (read_filter(input, rh, colormap, 
			(u_char *) image, imagesize) == 0 &&
			rh->ras_depth != 24)
			return pr;
		if ((mem32pr = image24to32(rh, image)) != NULL) {
		    (void) pr_destroy(pr);
		    return mem32pr;
		}
		break;
	}

	(void) pr_destroy(pr);

	return 0;
}

/*
 * Load pixrect image from the specified stream, header and colormap.
 * Don't filter nonstandard types.
 */
Pixrect *
pr_load_std_image(input, rh, colormap)
	FILE *input;
	register struct rasterfile *rh;
	register colormap_t *colormap;
{
	switch (rh->ras_type) {
	case RT_OLD:
	case RT_STANDARD:
	case RT_BYTE_ENCODED:
		return pr_load_image(input, rh, colormap);
	}

	return 0;
}

/*
 * Encode/decode functions for RT_BYTE_ENCODED images:
 *
 * The "run-length encoding" is of the form
 *
 *	<byte><byte>...<ESC><0>...<byte><ESC><count><byte>...
 *
 * where the counts are in the range 0..255 and the actual number of
 * instances of <byte> is <count>+1 (i.e. actual is 1..256). One- or
 * two-character sequences are left unencoded; three-or-more character
 * sequences are encoded as <ESC><count><byte>.  <ESC> is the character
 * code 128.  Each single <ESC> in the input data stream is encoded as
 * <ESC><0>, because the <count> in this scheme can never be 0 (actual
 * count can never be 1).  <ESC><ESC> is encoded as <ESC><1><ESC>. 
 *
 * This algorithm will fail (make the "compressed" data bigger than the
 * original data) only if the input stream contains an excessive number of
 * one- and two-character sequences of the <ESC> character.  
 */

#define ESCAPE		128

static int
dump_encoded(pr, out)
	register Pixrect *pr;
	register FILE *out;
{
	register u_char *in, c;
	register int incount;
	register int repeat;	/* repeat count - 1 */
	register int outcount;

	in = (u_char *) mpr_d(pr)->md_image;
	incount = mpr_prlinebytes(pr) * pr->pr_size.y;

	if (--incount < 0)
		return 0;

	c = *in++;
	outcount = 0;
	repeat = 0;

	/* if output file is null, just count output bytes */
	if (out == NULL) {
		while (1) {
			while (--incount >= 0 &&
				c == *in && repeat < 255) {
				repeat++;
				in++;
			}

			if (incount < 0)
				break;

			if (repeat < 2) {
				repeat++;
				outcount += repeat;
				if (c == ESCAPE)
					outcount++;
			}
			else 
				outcount += 3;

			repeat = 0;
			c = *in++;
		}
		if (repeat < 2) {
			repeat++;
			outcount += repeat;
			if (c == ESCAPE)
				outcount++;
		}
		else 
			outcount += 3;
	}
	/* output is non-null, write file */
	else {
		while (1) {
			while (--incount >= 0 &&
				c == *in && repeat < 255) {
				repeat++;
				in++;
			}

			if (incount < 0)
				break;

			if (repeat < 2) {
				if (c == ESCAPE) {
					putc(c, out);
					if (repeat) {
						putc(repeat, out);
						outcount += 2;
						repeat = 0;
					}
					else {
						c = 0;
						outcount++;
					}
				}
				do {
					putc(c, out);
					outcount++;
				} while (--repeat != -1);
			}
			else {
				putc(ESCAPE, out);
				putc(repeat, out);
				putc(c, out);
				outcount += 3;
			}

			repeat = 0;
			c = *in++;
		}
		if (repeat < 2) {
			if (c == ESCAPE) {
				putc(c, out);
				if (repeat) {
					putc(repeat, out);
					outcount += 2;
					repeat = 0;
				}
				else {
					c = 0;
					outcount++;
				}
			}
			do {
				putc(c, out);
				outcount++;
			} while (--repeat != -1);
		}
		else {
			putc(ESCAPE, out);
			putc(repeat, out);
			putc(c, out);
			outcount += 3;
		}
	}

	return outcount;
}

static int
read_encoded(in, incount, out, outcount)
	register FILE *in;
	register int incount;
	register u_char *out;
	register int outcount;
{
	register u_char c;
	register int repeat;

	while (1) {
		while (--incount >= 0 &&
			--outcount >= 0 && (c = getc(in)) != ESCAPE)
			*out++ = c;

		if (outcount < 0 || --incount < 0)
			break;

		if ((repeat = getc(in)) == 0)
			*out++ = c;
		else {
			if ((outcount -= repeat) < 0 || --incount < 0)
				break;

			c = getc(in);
			do {
				*out++ = c;
			} while (--repeat != -1);
		}
	}

	if (outcount < 0)
		incount--;
	else if (incount < -1)
		outcount--;

	if ((incount += 2) > 0)
		return incount;

	return -(++outcount);
}


/*
 * Raster file filter functions.
 */

#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define	PINFD		0	/* input side of pipe */
#define	POUTFD		1	/* output side of pipe */
#define	PIPSIZ		4096	/* size of a pipe (yuck) */

#ifdef SIG_ERR			/* yuck again */
#define	SV_HANDLER_TYPE	void
#else
#define	SV_HANDLER_TYPE	int
#endif

/* SIGPIPE catcher */
static caddr_t sigpipe_env;

static SV_HANDLER_TYPE
sigpipe_catch()
{
	if (sigpipe_env)
		_longjmp((jmp_buf *) sigpipe_env, 1);
}

static struct sigvec sigpipe_vec = {
	sigpipe_catch, 0, 0
};

/*
 * Write specified pixrect through filter then onto specified output.
 * Returns 0 or PIX_ERR.
 */
static int
dump_filtered(pr, out, rh, colormap, type)
	Pixrect *pr;
	FILE *out;
	struct rasterfile *rh;
	colormap_t *colormap;
	int type;
{
	int fds[2];
	FILE *filter = 0;
	int child = -1, pid;
	jmp_buf env;
	struct sigvec oldpipevec;
	int oldmask;
	union wait status;
	register int result = PIX_ERR;

	/* flush output file, create pipe */
	if (fflush(out) || pipe(fds) < 0)
		return result;

	/* create filter stream, start filter */
	if ((filter = fdopen(fds[POUTFD], "w")) != NULL)
		child = fork_filter(type, fds[PINFD], fileno(out));

	/* close unused pipe end */
	(void) close(fds[PINFD]);

	if (child > 0 && _setjmp(env) == 0) {
		/* save environment for SIGPIPE catching */
		sigpipe_env = (caddr_t) env;
		(void) sigvec(SIGPIPE, &sigpipe_vec, &oldpipevec);

		/* block signals in parent */
		oldmask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT));

		/* dump header and image via filter */
		result = (pr_dump_header(filter, rh, colormap) ||
			dump_image(pr, filter, rh));

		result |= fflush(filter);
	}
	sigpipe_env = 0;
	(void) sigvec(SIGPIPE, &oldpipevec, (struct sigvec *) 0);

	if (filter)
		result |= fclose(filter);
	else
		(void) close(fds[POUTFD]);

	if (child > 0) {
		/* wait for child to exit, check status */
		while ((pid = wait(&status)) >= 0) 
			if (pid == child) {
				result |= status.w_status;
				break;
			}

		/* restore signals */
		(void) sigsetmask(oldmask);
	}

	return result ? PIX_ERR : 0;
}

/*
 * Load pixrect image from the specified stream, header and colormap.
 * If the image must be filtered, the header and colormap will be modified.
 * Returns 0 if successful.
 */
static int
read_filter(in, rh, colormap, out, outcount)
	FILE *in;
	register struct rasterfile *rh;
	colormap_t *colormap;
	u_char *out;
	int outcount;
{
	struct {
		int state;	/* filter state */
#define	SINIT		-1	/* not started */
#define	SHEADER		0	/* writing/reading rasterfile header */
#define	SCOLORMAP	1	/* writing/reading colormap data */
#define	SIMAGE		2	/* writing/reading image data */
#define	SDONE		3	/* finished */

		int fd;		/* file descriptor */
		fd_set mask;	/* select() bit mask for fd */
		fd_set ready;	/* working copy of mask */
		char *p;	/* write/read pointer */
		int count;	/* write/read count */
		char *colormap;	/* colormap data array */
	} w, r;

	int incount;		/* image bytes left to be written to filter */
	register int n;		/* write/read result */
	int oldmask;		/* saved signal mask */
	int child = -1, pid;	/* filter pid, wait() result */
	jmp_buf env;		/* SIGPIPE catching environment */
	struct sigvec oldpipevec; /* saved SIGPIPE handler */

	struct rasterfile newrh; /* header read from filter */
	char buf[PIPSIZ];	/* buffer for reading input stream */

	static struct timeval	/* select() timeout value (100ms) */
		timeout = { 0, 100000 };
	struct timeval timeval;	/* working copy of timeout */
	int fdwidth = getdtablesize();


	/* initialize state variables for error recovery */
	w.state = SINIT;
	w.fd = -1;
	w.colormap = 0;
	w.count = 0;
	FD_ZERO(&w.mask);

	r.state = SINIT;
	r.fd = -1;
	r.count = 0;
	r.colormap = 0;
	FD_ZERO(&r.mask);

	/* get input byte count */
	incount = rh->ras_length;

	/* copy colormap data to byte array if necessary */
	if (colormap && 
		rh->ras_maptype != RMT_NONE && rh->ras_maplength > 0) {
		if (!(w.colormap = malloc((unsigned) rh->ras_maplength)))
			goto error;

		convert_colormap(colormap, w.colormap, 
			(struct rasterfile *) 0);
	}

	/* set up pipes and filter */
	{
		int fds[2][2];

		/* create pipe for writing to filter */
		if (pipe(fds[0]) < 0)
			goto error;

		w.fd = fds[0][POUTFD];
		FD_SET(w.fd, &w.mask);

		/* 
		 * create pipe for reading from filter,
		 * make sure pipe fds aren't too large,
		 * make pipe ends non-blocking,
		 * start filter process
		 */
		fds[1][POUTFD] = -1;
		if (pipe(fds[1]) >= 0) {
			r.fd = fds[1][PINFD];
			FD_SET(r.fd, &r.mask);
			if (fcntl(w.fd, F_SETFL, FNDELAY) >= 0 &&
				fcntl(r.fd, F_SETFL, FNDELAY) >= 0)
				child = fork_filter(rh->ras_type, 
					fds[0][PINFD], fds[1][POUTFD]);
		}

		/* close unused pipe ends */
		(void) close(fds[0][PINFD]);
		(void) close(fds[1][POUTFD]);

		/* quit if fork failed */
		if (child < 0)
			goto error;

		/* block signals in parent */
		oldmask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT));

		/* catch SIGPIPEs */
		if (_setjmp(env)) 
			goto error;

		sigpipe_env = (caddr_t) env;
		(void) sigvec(SIGPIPE, &sigpipe_vec, &oldpipevec);
	}

	/*
	 * The big loop.  Whenever possible write to filter, then read
	 * from it.  If it's not ready for either see if it exited.
	 */
	while (1) {
		extern int errno;

		FD_COPY(&w.mask, &w.ready);
		FD_COPY(&r.mask, &r.ready);
		timeval = timeout;

		switch (select(fdwidth,
			&r.ready, &w.ready, (fd_set *) NULL, &timeval)) {
		case -1:
			if (errno != EINTR)
				goto error;
			/* fall through */
		case 0:
			/* 
			 * select was interrupted or timed out; see if
			 * filter has exited
			 */
			while ((pid = wait3((union wait *) 0, WNOHANG, 
				(struct rusage *) 0)) != 0)
				if (pid < 0 || pid == child)
					goto error;
			break;
		}

		if (FD_ISSET(w.fd, &w.ready)) {
			if (w.count == 0)
				switch (++w.state) {
				case SHEADER:
					w.p = (char *) rh;
					w.count = sizeof *rh;
					break;
				case SCOLORMAP:
					if (w.p = w.colormap) {
						w.count = rh->ras_maplength;
						break;
					}
					w.state++;
					/* fall through */
				case SIMAGE:
					w.state++;
					/* fall through */
				case SDONE:
					/*
					 * If we wrote the whole image,
					 * clear mask so we won't bother 
					 * executing this code any more.
					 */
					if ((w.count = incount) == 0) {
						FD_ZERO(&w.mask);
						continue;
					}

					w.state--;
					w.p = buf;
					if (w.count > sizeof buf)
						w.count = sizeof buf;

					/*
					 * Buffer is empty, read something
					 * from the input stream.
					 */
					if (fread(buf, 1, w.count, in) !=
						w.count)
						goto error;

					incount -= w.count;
					break;
				}

			if ((n = write(w.fd, w.p, w.count)) < 0)
				if (errno != EWOULDBLOCK)
					goto error;
				else
					n = 0;

			w.count -= n;
			w.p += n;
		}

		if (FD_ISSET(r.fd, &r.ready)) {
			if (r.count == 0)
				switch (++r.state) {
				case SHEADER:
					r.p = (char *) &newrh;
					r.count = sizeof newrh;
					break;
				case SCOLORMAP:
					if (newrh.ras_maptype != RMT_NONE &&
						(r.count = 
						newrh.ras_maplength) > 0) {
						if (!(r.colormap = malloc(
							(unsigned) r.count)))
							goto error;

						r.p = r.colormap;
						break;
					}
					r.state++;
					/* fall through */
				case SIMAGE:
					r.p = (char *) out;
					r.count = outcount;
					break;
				case SDONE:
					goto done;
				}

			if ((n = read(r.fd, r.p, r.count)) < 0)
				if (errno != EWOULDBLOCK)
					goto error;
				else
					n = 0;

			r.count -= n;
			r.p += n;
		}
	}

	/*
	 * Successful completion -- copy rasterfile header and
	 * colormap struct over user's.
	 */

done:
	*rh = newrh;
	if (colormap) {
		convert_colormap(colormap, r.colormap, rh);
		r.colormap = 0;
	}

	/* error cleanup */
error:
	if (w.colormap)
		(void) free(w.colormap);

	if (r.colormap)
		(void) free(r.colormap);

	if (w.fd)
		(void) close(w.fd);

	if (r.fd)
		(void) close(r.fd);

	if (child >= 0) {
		/* restore SIGPIPE handler */
		sigpipe_env = 0;
		(void) sigvec(SIGPIPE, &oldpipevec, 
			(struct sigvec *) 0);

		/* force filter to exit */
		if (r.state != SDONE) {
			(void) kill(child, SIGTERM);
			(void) kill(child, SIGKILL);
		}

		/* reap it */
		while ((pid = wait((union wait *) 0)) >= 0 && pid != child)
			/* nothing */ ;

		/* restore old signal mask */
		(void) sigsetmask(oldmask);
	}

	return r.state != SDONE;
}

/* Fork a rasterfile filter with given standard input and output. */
static
fork_filter(type, infd, outfd)
int type, infd, outfd;
{
	char fullname[64], *basename;
	int maxfd, fd;
	int child, pid;
	extern char *sprintf(), *rindex();

	/* generate filter name from type code */
	(void) sprintf(fullname, "/usr/lib/rasfilters/convert.%d", type);
	basename = rindex(fullname, '/') + 1;

	maxfd = getdtablesize();

	if ((child = vfork()) == 0) {
		/* child, set up standard in/out/error */
		if (dup2(infd, 0) >= 0 && 
			dup2(outfd, 1) >= 0 &&
			dup2(fileno(stderr), 2)) {

			/* close unneeded file descriptors */
			for (fd = 3; fd < maxfd; fd++)
				(void) close(fd);

			/* try to exec filter using user's path */
			(void) execlp(basename, basename, (char *) 0);

			/* now try standard directory */
			(void) execlp(fullname, fullname, (char *) 0);
		}
		_exit(127);
	}

	/* check for stillborn child */
	if (child > 0) 
		while ((pid = wait3((union wait *) 0, WNOHANG, 
			(struct rusage *) 0)) != 0)
			if (pid < 0 || pid == child) {
				child = -1;
				break;
			}

	return child;
}

/*
 * Convert data in colormap struct to or from byte array.
 * rh = null:     copy data from struct to array
 *      non-null: load struct with pointers into array
 */
static int
convert_colormap(colormap, buf, rh)
	register colormap_t *colormap;
	register char *buf;
	register struct rasterfile *rh;
{
	register int len;

	if (rh == 0) {
		len = colormap->length;

		switch (colormap->type) {
		case RMT_NONE:
			break;

		case RMT_EQUAL_RGB:
			bcopy((char *) colormap->map[0], buf, len);
			buf += len;
			bcopy((char *) colormap->map[1], buf, len);
			buf += len;
			bcopy((char *) colormap->map[2], buf, len);
			break;

		case RMT_RAW:
		default:
			bcopy((char *) colormap->map[0], buf, len);
			break;
		}
	}
	else {
		switch (colormap->type = rh->ras_maptype) {
		case RMT_NONE:
			len = 0;
			break;

		case RMT_EQUAL_RGB:
			len = rh->ras_maplength / 3;
			colormap->map[0] = (u_char *) buf;
			buf += len;
			colormap->map[1] = (u_char *) buf;
			buf += len;
			colormap->map[2] = (u_char *) buf;
			break;

		case RMT_RAW:
		default:
			len = rh->ras_maplength;
			colormap->map[0] = (u_char *) buf;
			break;
		}
		colormap->length = len;
	}
}

/* convert 24-bit images to 32-bit */
static Pixrect        *
image24to32 (rh, image)
    struct rasterfile *rh;
    unsigned char  *image;
{
    Pixrect        *mem32pr;
    int             byte_perline;
    register union fbunit *mem32im;
    register union fbunit *save_im;
    register unsigned char *in_image;
    register int    i,
                    lines;

    /*
     * Use the least effort but inefficient method to convert the pixrect.
     * First we create a 32-bit memory pixrect, then assign each pixel into
     * the image area.  We could have do everything on a per-scanline basis
     * to conserve memory or, better off, use mmap to access the source
     * image.
     */
    mem32pr = mem_create (rh->ras_width, rh->ras_height,
			  32);
    if (mem32pr == NULL)
	return (Pixrect *) NULL;
    save_im = mem32im = (union fbunit *)
	mpr_d (mem32pr)->md_image;
    in_image = image;
    lines = (int) rh->ras_height;
    byte_perline = mpr_linebytes (rh->ras_width,
				  rh->ras_depth);
    while (--lines >= 0) {
	for (i = 0; i < rh->ras_width; i++, mem32im++) {
	    mem32im->channel.B = *in_image++;
	    mem32im->channel.G = *in_image++;
	    mem32im->channel.R = *in_image++;
	}
	save_im += mpr_d (mem32pr)->md_linebytes /
	    sizeof (union fbunit);
	image += byte_perline;
	in_image = image;
	mem32im = save_im;
    }
    return mem32pr;
}
