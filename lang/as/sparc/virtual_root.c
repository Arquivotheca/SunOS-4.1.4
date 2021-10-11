static char sccs_id[] = "@(#)virtual_root.c	1.1\t10/31/94";
/* LINTLIBRARY */
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>

extern char    *index();
extern char    *getenv();
extern char    *strcpy();
extern int      errno;

static char    *virtual_root = NULL;
static int      virtual_root_read = 0;

int static 
check_file(file, fl, path, pl, root, rl, result, result_len)
	char           *file, *path, *root, *result;
	int             fl, pl, rl, result_len;
{
	if (file[0] == '/')
	{
		if (rl + fl >= result_len)
			return (ENAMETOOLONG);
		(void) strcpy(result, root);
		(void) strcpy(result + rl, file);
		goto name;
	};
	if (path[0] == '/')
	{
		if (rl + pl + fl + 1 >= result_len)
			return (ENAMETOOLONG);
		(void) strcpy(result, root);
		(void) strcpy(result + rl, path);
		*(result + rl + pl) = '/';
		(void) strcpy(result + rl + pl + 1, file);
		goto name;
	};
	if (pl + fl + 1 >= result_len)
		return (ENAMETOOLONG);
	if (pl > 0)
	{
		(void) strcpy(result, path);
		*(result + pl) = '/';
		(void) strcpy(result + pl + 1, file);
	} else
		(void) strcpy(result + pl, file);
name:	return (access(result, R_OK));
}

int 
find_file_under_roots(file, path, pathindex, roots, result, result_len)
	register char  *file, *path[], *roots, *result;
	int            *pathindex, result_len;
{
	register char  *root_p = roots;
	int             fl, rl;

	if (virtual_root_read == 0)
	{
		virtual_root = getenv("VIRTUAL_ROOT");
		virtual_root_read = 1;
	};
	root_p = roots = virtual_root;
	if (!roots)
		root_p = roots = "";
	if (pathindex != NULL)
		*pathindex = -1;
	while (isspace(*file))
		file++;
	fl = strlen(file);
	while (root_p)
	{
		char          **path_p;
		register int    pathi = 0;
		if ((root_p = index(roots, ':')) != NULL)
			*root_p = 0;
		rl = strlen(roots);
		if ((path != NULL) && (*path != NULL))
			for (path_p = path; *path_p != NULL; path_p++, pathi++)
			{
				int             ret;
				ret = check_file(file, fl, *path_p, strlen(*path_p), roots, rl, result, result_len);
				if ((ret == ENAMETOOLONG) || (ret == 0))
				{
					if ((path != NULL) && (pathindex != NULL))
						*pathindex = pathi;
					if (root_p != NULL)
						*root_p = ':';
					return (ret);
				};
			}
		else
		{
			int             ret;
			ret = check_file(file, fl, "", 0, roots, rl, result, result_len);
			if ((ret == ENAMETOOLONG) || (ret == 0))
			{
				if (root_p != NULL)
					*root_p = ':';
				return (ret);
			};
		};
		if (root_p != NULL)
			*root_p = ':';
		roots = root_p + 1;
	};
	return (ENOENT);
}

int 
open_root(file, flags, mode)
	char           *file;
	int             flags, mode;
{
	int             r;
	char            filename[MAXPATHLEN];

	r = find_file_under_roots(file, (char **) NULL, (int *) NULL, (char *) NULL, filename, sizeof(filename));
	if (r)
	{
		errno = r;
		return (-1);
	} else
		return (open(filename, flags, mode));
}

FILE           *
fopen_root(file, type)
	char           *file, *type;
{
	int             r;
	char            filename[MAXPATHLEN];

	r = find_file_under_roots(file, (char **) NULL, (int *) NULL, (char *) NULL, filename, sizeof(filename));
	if (r)
		return ((FILE *) NULL);
	else
		return (fopen(filename, type));
}

int 
stat_root(file, stat_buffer)
	char           *file;
	struct stat    *stat_buffer;
{
	int             r;
	char            filename[MAXPATHLEN];

	r = find_file_under_roots(file, (char **) NULL, (int *) NULL, (char *) NULL, filename, sizeof(filename));
	if (r)
	{
		errno = r;
		return (-1);
	} else
		return (stat(filename, stat_buffer));
}
