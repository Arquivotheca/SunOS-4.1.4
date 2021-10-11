#ident "@(#)cb_misc.c 1.1 94/10/31"

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
 * This file implements miscellaneous routines used by the browser.
 */

#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_directory_h_INCLUDED
#include "cb_directory.h"
#endif

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

extern	char		*cb_program_name;

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		cb_create_dir();
extern	void		cb_abort();
extern	void		cb_fatal();
extern	void		cb_warning();
extern	void		cb_show_message();
extern	void		cb_free();
extern	char		*cb_get_wd();
extern	int		cb_line_hash_value();
extern	void		cb_make_absolute_path();
extern	void		cb_make_relative_path();
extern	time_t		cb_get_timestamp();
extern	time_t		cb_get_time_of_day();

/*
 *	cb_create_dir(dir)
 *		Create the directory even if it is located at
 *		the end of a symlink.
 */
void
cb_create_dir(dir)
	char	*dir;
{
	struct stat	stat_buff;
	char		path[MAXPATHLEN];
	char		link[MAXPATHLEN];
	char		*base;

	(void)strcpy(path, dir);
	while (lstat(path, &stat_buff) == 0) {
		if ((stat_buff.st_mode & S_IFMT) != S_IFLNK) {
			break;
		}
		cb_readlink(path, link, sizeof(link));
		if (link[0] == 0) {
			(void)strcpy(path, link);
		} else {
			if ((base = rindex(path, '/')) == NULL) {
				(void)strcpy(path, link);
			} else {
				(void)strcpy(base+1, link);
			}
		}
	}
	(void)mkdir(path, 0777);
}

/*
 *	cb_abort(format, args, ...)
 *		Print "args" using "format" and then dump core.
 */
/*VARARGS0*/
void
cb_abort(va_alist)
	va_dcl
{
	va_list		args;		/* Argument list */
	char		*format;	/* Format string */
	char		message[1000];

	(void)sprintf(message,
		      "%s: Internal error: errno= %d %s: ",
		      cb_program_name,
		      errno,
		      errno < sys_nerr ? sys_errlist[errno] : "");
	va_start(args);
	format = va_arg(args, char *);
	(void)vsprintf(message + strlen(message),
		       format,
		       args);
	va_end(args);
	if (getenv("SBQUERY_DROP_CORE") != NULL) {
		cb_show_message(message);
		(void)strcat(message, "Producing core file in 5 seconds ...");
		sleep(5);
		abort();
	} else {
		cb_show_message(message);
		exit(1);
	}
}

/*
 *	cb_fatal(format, args)
 *		Print "args" using "format" and then terminate the program
 *		with an exit code or 1.
 */
/*VARARGS0*/
void
cb_fatal(va_alist)
	va_dcl
{
	va_list		args;		/* Argument list */
	char		*format;	/* Format string */
	char		message[1000];

	(void)sprintf(message, "%s: Fatal error: ", cb_program_name);
	va_start(args);
	format = va_arg(args, char *);
	(void)vsprintf(message + strlen(message),
		       format,
		       args);
	va_end(args);
	cb_show_message(message);
	exit(1);
}

/*
 *	cb_warning(format, args)
 *		Print "args" using "format" 
 */
/*VARARGS0*/
void
cb_warning(va_alist)
	va_dcl
{
	va_list		args;		/* Argument list */
	char		*format;	/* Format string */
	char		message[1000];

	(void)sprintf(message, "%s: Warning: ", cb_program_name);
	va_start(args);
	format = va_arg(args, char *);
	(void)vsprintf(message + strlen(message),
		       format,
		       args);
	va_end(args);
	cb_show_message(message);
}

/*
 *	cb_show_message(message)
 */
static void
cb_show_message(message)
	char		*message;
{
	char		*filename;
static	FILE		*file = NULL;

	(void)fflush(stdout);
	if ((filename = getenv("SAVE_ERROR_MESSAGES")) == NULL) {
		(void)fprintf(stderr, message);
	} else {
		if (file == NULL) {
			if ((file = fopen(filename, "a")) == NULL) {
				(void)fprintf(stderr,
					      "Could not open %s for append\n",
					      filename);
				exit(2);
			}
		}
		(void)fprintf(file, message);

		(void)fprintf(stderr, "******************************\n");
		(void)fprintf(stderr, "*\n");
		(void)fprintf(stderr, "* %s", message);
		(void)fprintf(stderr, "*\n");
		(void)fprintf(stderr, "******************************\n\n");
	}
	(void)fflush(stderr);
}

/*
 *	cb_free(data)
 *		This will free the storage associated with data.  This is a
 *		convenient place to attach memory monitoring code.
 */
void
cb_free(data)
	char		*data;		/* Data to free */
{
	(void)free(data);
}

/*
 *	cb_get_wd(recheck)
 *		This will return the current working directory.
 */
#ifdef _CBQUERY
static
#endif
char *
cb_get_wd(recheck)
	int	recheck;
{
static	char		*cwd = NULL;
	char		temp[MAXPATHLEN];
	char		*p;
	struct stat	dot;
	struct stat	other;

	if ((cwd == NULL) || recheck) {
		p = getenv("PWD");
		dot.st_ino = 1;
		other.st_ino = 2;
		if (p != NULL) {
			(void)stat(".", &dot);
			(void)stat(p, &other);
		}
		if ((dot.st_ino != other.st_ino) ||
		    (dot.st_dev != other.st_dev)) {
			(void)getwd(temp);
			p = temp;
		}
		cwd = cb_string_unique(p);
	}
	return cwd;
}

/*
 *	cb_line_hash_value(line)
 *		Compute the hash value for one line of text.
 *		Terminates on \n and null.
 */
int
cb_line_hash_value(line)
	register char	*line;
{
	register int	result = 0;
	register int	ch;

	while (ch = *line++) {
		if (ch == '\n') {
			return result;
		}
		result += ch;
	}
	return result;
}

/*
 *	cb_make_absolute_path(path, use_twiddle, buffer)
 *		This return the absolute path associated
 * 		with "path".
 */
void
cb_make_absolute_path(path, use_twiddle, buffer)
	char		*path;
	int		use_twiddle;
	char		*buffer;
{
	char		*p;
static	char		*home = NULL;
static	int		home_len;

	while ((path[0] == '.') && (path[1] == '/')) {
		path += 2;
	}
	if ((path[0] == '.') && (path[1] == 0)) {
		(void)strcpy(buffer, cb_get_wd(0));
		return;
	}
	if (path[0] == '/') {
		(void)strcpy(buffer, path);
	} else {
		/*
		 * Strip leading ../ from path and concatenate with cwd
		 */
		(void)strcpy(buffer, cb_get_wd(0));
		while ((path[0] == '.') &&
		       (path[1] == '.') &&
		       ((path[2] == '/') || (path[2] == 0))) {
			if (path[2] == '/') {
				path += 3;
			} else {
				path += 2;
			}
			if ((p = rindex(buffer, '/')) != NULL) {
				*p = 0;
			}
		}
		if (path[0] != 0) {
			(void)strcat(buffer, "/");
			(void)strcat(buffer, path);
		}
	}

	/* strip "/./" */
	for (p = buffer, path = buffer; *p != 0; ) {
	found_dot:
		if ((p[0] == '/') &&
		    (p[1] == '.') &&
		    (p[2] == '/')) {
			p += 2;
			goto found_dot;
		}
		*path++ = *p++;
	}
	*path = 0;

	/* strip "//" */
	for (p = buffer, path = buffer; *p != 0; ) {
	found_double:
		if ((p[0] == '/') &&
		    (p[1] == '/')) {
			p++;
			goto found_double;
		}
		*path++ = *p++;
	}
	*path = 0;

	path = buffer;
	if (use_twiddle) {
		if (home == NULL) {
			home = getenv("HOME");
			home_len = strlen(home);
		}
		if ((strlen(buffer) > home_len) &&
		    !strncmp(home, buffer, home_len)) {
			buffer[home_len - 1] = '~';
			path += home_len - 1;
		}
	}
	return;
}

/*
 *	cb_make_relative_path(path, result)
 */
void
cb_make_relative_path(path, result)
	register char		*path;
	register char		*result;
{
	register int		ncomps = 1;
	register int		i;
	register int		len;
	register char		*tocomp;
	register char		*cp;
	register char		*wd = cb_get_wd(0);

	/*
	 * Find the number of components in the wd name.
	 * ncomp= number of slashes + 1.
	 */
	for (cp = wd; *cp != 0; cp++) {
		if (*cp == '/') {
			ncomps++;
		}
	}

	/*
	 * See how many components match to determine how many,
	 * if any, ".."s will be needed.
	 */
	*result = 0;
	tocomp = path;
	while ((*wd != 0) && (*wd == *path)) {
		if (*wd == '/') {
			ncomps--;
			tocomp = &path[1];
		}
		wd++;
		path++;
	}

	/*
	 * Now for some special cases.
	 * Check for exact matches and for either name terminating exactly.
	 */
	if (*wd == 0) {
		if (*path == 0) {
			(void)strcpy(result, ".");
			return;
		}
		if (*path == '/') {
			ncomps--;
			tocomp = &path[1];
		}
	} else if ((*wd == '/') && (*path == 0)) {
		ncomps--;
		tocomp = path;
	}
	/*
	 * Add on the ".."s.
	 */
	for (i = 0; i < ncomps; i++) {
		(void)strcat(result, "../");
	}

	/*
	 * Add on the remainder, if any, of the path name.
	 */
	if (*tocomp == 0) {
		len = strlen(result);
		result[len - 1] = 0;
	} else {
		(void)strcat(result, tocomp);
	}
	return;
}

#ifdef UNUSED
/*
 *	cb_log(format, args)
 *		This will write "args" into a log file using "format".
 */
/*VARARGS1*/
void
cb_log(format, args)
	char		*format;	/* Format string */
	va_list		*args;		/* Argument list */
{
	static FILE	*log_file = NULL;
	char		log_name[MAXPATHLEN];
	static int	hostid;
	static int	pid;
	struct timeval	time;
	static int	start_sec;
	static int	start_usec;
	
	if (log_file == NULL) {
		hostid = gethostid();
		pid = getpid();
		cb_gettimeofday(&time, (struct timezone *)NULL);
		start_sec = time.tv_sec;
		start_usec = time.tv_usec;
		(void)sprintf(log_name, "/tmp/%x.%d.%02d.%06d.log",
			      hostid, pid, start_sec, start_usec);
		log_file = cb_fopen(log_name, "w", (char *)NULL, 0);
	}
	if (format == NULL) {
		cb_fclose(log_file);
		log_file = NULL;
	} else {
		cb_gettimeofday(&time, (struct timezone *)NULL);
		(void)fprintf(log_file, "%02d.%06d %d %x ", time.tv_sec,
			      time.tv_usec, pid, hostid);
		(void)vfprintf(log_file, format, &args);
	}
}
#endif UNUSED

/*
 *	cb_get_timestamp(file_name)
 *		This will return the timestamp for "file_name".
 */
time_t
cb_get_timestamp(file_name)
	char		*file_name;
{
	struct stat	status;

	cb_stat(file_name, &status);
	return status.st_mtime;
}

/*
 *	cb_get_time_of_day()
 *		This will return the current time of day.
 */
time_t
cb_get_time_of_day()
{
	struct timeval	time_value;

	cb_gettimeofday(&time_value, (struct timezone *) NULL);
	return (time_t)time_value.tv_sec;
}

