/*	@(#)fio.h 1.1 94/10/31 SMI	*/


/*
 * Structure describing a source file.
 */
struct	file {
	char	*f_name;		/* name of this file */
	int	f_flags;		/* see below */
	off_t	*f_lines;		/* array of seek pointers */
/* line i stretches from lines[i-1] to lines[i] - 1, if first line is 1 */
	int	f_nlines;		/* number of lines in file */
/* lines array actually has nlines+1 elements, so last line is bracketed */
} *file, *filenfile;
int	nfile;		/* current number of files */
int	NFILE;		/* fileslots in file[] */

#define	F_INDEXED	0x1		/* file has been indexed */

struct	file *openfile;
FILE	*OPENFILE;
off_t	openfoff;
char	filebuf[BUFSIZ];

struct	file *fget(), *indexf();

char	linebuf[BUFSIZ];		/* file line returned by getline */
char	*savestr();

#define	FILEX(file,line)	(((file)<<(2*NBBY))|(line))
#define	XFILE(filex)		((filex)>>(2*NBBY))
#define	XLINE(filex)		((filex)&((1<<(2*NBBY))-1))

int	filex;
