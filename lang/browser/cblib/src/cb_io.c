#ident "@(#)cb_io.c 1.1 94/10/31 Copyr 1986 Sun Micro"

#ifndef lint
#ifdef INCLUDE_COPYRIGHT
static char _copyright_notice_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
Sun considers its source code as an unpublished, proprietary \
trade secret, and it is available only under strict license \
provisions.  This copyright notice is placed here only to protect \
Sun in the event the source is deemed a published work.  Dissassembly, \
decompilation, or other means of reducing the object code to human \
readable form is prohibited by the license agreement under which \
this code is provided to the user or company in possession of this \
copy. \
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the \
Government is subject to restrictions as set forth in subparagraph \
(c)(1)(ii) of the Rights in Technical Data and Computer Software \
clause at DFARS 52.227-7013 and in similar clauses in the FAR and \
NASA FAR Supplement.";
#endif

#ifdef INCLUDE_COPYRIGHT_REFERENCE
static char _copyright_notice_reference_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
See copyright notice in cb_copyright.o in the libsb.a library.";
#endif
#endif


/*
 * This file implements cover routines for io with error checking
 */

#include <sys/mman.h>

#ifndef _sys_signal_h
#include <sys/signal.h>
#endif

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

static	struct {
	char		*start;
	int		length;
}			cb_file_mmap[NOFILE];

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		cb_delete();
extern	int		cb_delete_on_exit();
extern	FILE		*cb_fopen();
extern	void		cb_fflush();
extern	void		cb_fclose();
extern	void		cb_fseek();
extern	void		cb_fread();
extern	void		cb_fwrite();
extern	int		cb_getc();
extern	void		cb_gettimeofday();
extern	int		cb_open();
extern	void		cb_fstat();
extern	void		cb_stat();
extern	void		cb_close();
extern	void		cb_write();
extern	FILE		*cb_popen();
extern	void		cb_pclose();
extern	char		*cb_fgets();
extern	void		cb_fputc();
extern	void		cb_readlink();
extern	void		cb_signal();
extern	void		cb_ungetc();
extern	char		*cb_mmap_file_private();

/*
 *	cb_delete(file_name)
 *		This will delete "file_name".  No error occurs if "file_name"
 *		does not exist.
 */
void
cb_delete(file_name)
	char		*file_name;
{
	(void)unlink(file_name);
}

/*
 *	cb_delete_on_exit(file_name)
 *		This will delete "file_name".  No error occurs if "file_name"
 *		does not exist. Made for on_exit().
 */
/*ARGSUSED*/
int
cb_delete_on_exit(status, file_name)
	int		status;
	char		*file_name;
{
	(void)unlink(file_name);
}

/*
 *	cb_fopen(file_name, rw, buffer, length)
 *		This will open "file_name" and return the
 *		associated FILE object.
 */
FILE *
cb_fopen(file_name, rw, buffer, length)
	char		*file_name;
	char		*rw;
	char		*buffer;
	int		length;
{
	register FILE	*file;

	file = fopen(file_name, rw);
	if (file == NULL) {
		cb_abort("fopen(%s, %s) failed\n", file_name, rw);
		/* NOTREACHED */
	}
	if (buffer != NULL) {
		(void)setvbuf(file, buffer, _IOFBF, length);
	}
	return file;
}

/*
 *	cb_fflush(file)
 */
void
cb_fflush(file)
	FILE	*file;
{
	if (fflush(file) != 0)  {
		cb_abort("fflush() failed\n");
	}
}

/*
 *	cb_fclose(fd)
 *		This will close the file and complain if the close fails
 */
void
cb_fclose(fd)
	FILE	*fd;		/* File descriptor */
{
	if (cb_file_mmap[fileno(fd)].start != NULL) {
		if (munmap(cb_file_mmap[fileno(fd)].start,
			   cb_file_mmap[fileno(fd)].length) != 0) {
			cb_abort("munmap failed\n");
		}
		cb_file_mmap[fileno(fd)].start = NULL;
	}
	if (fclose(fd) != 0) {
		cb_abort("Could not fclose\n");
		/* NOTREACHED */
	}
}

/*
 *	cb_fseek(file, offset)
 *		This will seek to "offset" in "file".
 */
void
cb_fseek(file, offset)
	FILE		*file;
	int		offset;
{
	if (ftell(file) == (long)offset) {
		return;
	}
	if (fseek(file, (long)offset, 0) == -1) {
		cb_abort("Could not fseek to %d\n", offset);
	}
}

#ifndef _CBQUERY
/*
 *	cb_fread(data, size, in_file)
 *		Read "size" bytes from "in_file" to "data"
 *		or die trying.
 */
void
cb_fread(data, size, in_file)
	char		*data;		/* Place to store read data */
	int		size;		/* Number of bytes to read */
	FILE		*in_file;	/* Input file */
{
	if (fread(data, 1, size, in_file) != size) {
		cb_abort("fread could not read in %d bytes\n", size);
	}
}
#endif

/*
 *	cb_fwrite(data, size, out_file)
 *		Write "size" bytes of "data" to "out_file" or dies trying.
 */
void
cb_fwrite(data, size, out_file)
	char		*data;		/* Data to write */
	int		size;		/* Number of bytes to write */
	FILE		*out_file;	/* Output file */
{
	if (fwrite(data, 1, size, out_file) != size) {
		cb_abort("fwrite failed\n");
		/* NOTREACHED */
	}
}

/*
 *	cb_getc(in_file)
 *		This will return the next character from "in_file".
 */
int
cb_getc(in_file)
	FILE		*in_file;
{
	return getc(in_file);
}

/*
 *	cb_gettimeofday(time_value, time_zone)
 *		Get the time and time zone.
 */
void
cb_gettimeofday(time_value, time_zone)
	struct timeval	*time_value;
	struct timezone	*time_zone;
{
	struct timezone zilch;

	if (time_zone == NULL) {
		time_zone = &zilch;
	}
	if (gettimeofday(time_value, time_zone) == -1) {
		cb_abort("gettimeofday failed\n");
		/* NOTREACHED */
	}
}

/*
 *	cb_open(file_name, flags)
 *		This will open "file_name"
 */
int
cb_open(file_name, flags)
	char		*file_name;
	int		flags;
{
	int	fd;

	fd = open(file_name, flags, 0775);
	if (fd < 0) {
		cb_abort("open(%s, 0%o, 0775) failed\n", file_name, flags);
		/* NOTREACHED */
	}
	return fd;
}

/*
 *	cb_fstat(fd, status)
 *		This will perform an fstat on "fd" and store the results in
 *		"status".  Any errors will be fatal.
 */
void
cb_fstat(fd, status)
	int		fd;
	struct stat	*status;
{
	if (fstat(fd, status) != 0) {
		cb_abort("Could not fstat(%d)\n", fd);
		/* NOTREACHED */
	}
}

/*
 */
void
cb_stat(file_name, status)
	char		*file_name;
	struct stat	*status;
{
	if (stat(file_name, status) != 0) {
		cb_abort("Could not stat(%s)\n", file_name);
		/* NOTREACHED */
	}
}

/*
 *	cb_close(fd)
 *		This will close the file and complain if the close fails
 */
void
cb_close(fd)
	int	fd;		/* File descriptor */
{
	if (cb_file_mmap[fd].start != NULL) {
		if (munmap(cb_file_mmap[fd].start,
			   cb_file_mmap[fd].length) != 0) {
			cb_abort("munmap failed\n");
		}
		cb_file_mmap[fd].start = NULL;
	}
	if (close(fd) != 0) {
		cb_abort("Could not close\n");
		/* NOTREACHED */
	}
}

/*
 *	cb_write(fd, data, size)
 *		This will write out "size" bytes form "fd" from "data"
 *		or die trying.
 */
void
cb_write(fd, data, size)
	int		fd;		/* File descriptor */
	char		*data;		/* Data to write */
	int		size;		/* Number of bytes to write */
{
	if (write(fd, data, size) != size) {
		cb_abort("Could not write out %d bytes\n", size);
		/* NOTREACHED */
	}
}

#ifndef _CBQUERY
/*
 *	cb_popen(cmd, rw)
 */
FILE *
cb_popen(cmd, rw)
	char	*cmd;
	char	*rw;
{
	register FILE	*file = popen(cmd, rw);

	if (file == NULL) {
		cb_abort("popen(%s, %s) failed\n", cmd, rw);
	}
	return file;
}

/*
 *	cb_pclose(pipe)
 */
void
cb_pclose(pipe)
	FILE	*pipe;
{
	if (pclose(pipe) == -1) {
		cb_abort("pclose() failed\n");
	}
}
#endif

/*
 *	cb_fgets(destination, size, file)
 */
char *
cb_fgets(destination, size, file)
	char	*destination;
	int	size;
	FILE	*file;
{
	register char	*result = fgets(destination, size, file);

	if ((result == NULL) && (ferror(file) != 0)) {
		cb_abort("fgets() failed\n");
	}
	return result;
}

#ifndef _CBQUERY
/*
 *	cb_fputc(chr, file)
 */
void
cb_fputc(chr, file)
	int	chr;
	FILE	*file;
{
	if (fputc(chr, file) == EOF) {
		cb_abort("fputc(%c) failed\n", chr);
	}
}
#endif

/*
 *	cb_readlink(link, destination, size)
 */
void
cb_readlink(link, destination, size)
	char	*link;
	char	*destination;
	int	size;
{
	int	length = readlink(link, destination, size);

	if ((length == -1) || (length == size)) {
		cb_abort("readlink(%s) failed\n", link);
	}
	destination[length] = 0;
}

/*
 *	cb_signal(sig, func)
 */
void
cb_signal(sig, func)
	int		sig;
	void		(*func)();
{
#ifdef i386
	if ((int)signal(sig, (int (*)())func) == -1) {
		cb_abort("signal() failed\n");
	}
#else
	if ((int)signal(sig, func) == -1) {
		cb_abort("signal() failed\n");
	}
#endif
}

#ifndef _CBQUERY
/*
 *	cb_ungetc(chr, file)
 */
void
cb_ungetc(chr, file)
	int	chr;
	FILE	*file;
{
	if (ungetc(chr, file) == EOF) {
		cb_abort("ungetc(%c) failed\n", chr);
	}
}
#endif

/*
 *	cb_mmap_file_private(fd, length)
 */
char *
cb_mmap_file_private(fd, length)
	int		fd;
	int		length;
{
	char		*result;
#ifdef sun386
	int		mode = PROT_READ|PROT_WRITE;
#else
	int		mode = PROT_READ;
#endif

	if (length <= 0) {
		return NULL;
	}
	result = (char *)mmap((char *)NULL,
			      length,
			      mode,
			      MAP_PRIVATE,
			      fd,
			      (off_t)0);
	if (result == (char *)-1) {
		cb_abort("mmap(fd=%d,length=%d) failed\n", fd, length);
	}
	cb_file_mmap[fd].start = result;
	cb_file_mmap[fd].length = length;
	return result;
}

