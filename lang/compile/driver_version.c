#ifndef lint
static	char sccsid[] = "@(#)driver_version.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/file.h>	/* for access() function args */
#include "driver.h"


static char *
path_plus_filename(path, filename)
	char	*path;
	char	*filename;
{
	char	*fullpath;

	fullpath = get_memory(  strlen(path)
				+ 1 /*"/"*/
				+ strlen(filename)
				+ 1 /*'\0'*/
			     );
	sprintf(fullpath, "%s/%s", path, filename);

	return  fullpath;
}


static FILE *
open_driver_versions_file(root_path)
	char  *root_path;
{
	char		*info_filename;
	FILE		*fp;
	static char	raw_info_filename[] = "/usr/lib/lang_info"; 

	/* Look for the Info File using Qpath.
	 * If that fails, look for it under the given virtual root.
	 */
	if ((info_filename = scan_Qpath_only(raw_info_filename, FALSE)) == NULL)
	{
		info_filename =
			path_plus_filename(root_path, &raw_info_filename[1]);
		/* info_filename is now {ROOTPATH}/usr/lib/lang_info . */
	}

	fp = fopen(info_filename, "r");

	if (fp == NULL)
	{
		debug_info_filename = make_string("lang_info not found");
	}
	else
	{
		debug_info_filename = make_string(info_filename);
	}	

	free(info_filename); 

	return  fp;
}


static void
close_driver_versions_file(filep)
	FILE	*filep;
{
	fclose(filep);
}


static int
get_version_line(filep, product_namep, product_versionp, base_os_versionp)
	FILE	*filep;
	char	*product_namep;
	char	*product_versionp;
	char	*base_os_versionp;
{
	char	line[LOCAL_STRING_LENGTH];

	/* Each line of the file contains these fields, in order:
	 *	product name		(string)
	 *	product version #	(string)
	 *	base-OS version #	(string)
	 *	(possibly other info, which we ignore for now)
	 *
	 * Lines beginning with "#" are comment lines.
	 */

	while ( fgets(line, LOCAL_STRING_LENGTH, filep) != NULL )
	{
		if ( ( 3 == sscanf(line, "%s %s %s", product_namep,
					product_versionp, base_os_versionp) ) 
				&&
		     ( *product_namep != '#' )
		   )
		{
			/* Got a real product line (not blank or a comment) */
			return ~NULL;
		}
	}

	/* fgets() returned NULL, so we hit EOF. */
	return NULL;
}


static Const_intP
get_base_os_version(filep, driver)
	FILE		*filep;
	Const_int	*driver;
{
	char	product_name[LOCAL_STRING_LENGTH];
	char	product_version[LOCAL_STRING_LENGTH];
	char	base_os_version[LOCAL_STRING_LENGTH];
	char	drivers_base_os_version[LOCAL_STRING_LENGTH];

	drivers_base_os_version[0] = '\0';

	while ( get_version_line(filep, &product_name[0], &product_version[0],
				 	       &base_os_version[0]) != NULL )
	{
		/* Note that a product version of "-" causes this line in the
		 * info file to be ignored.  This can be used to leave a
		 * "template" or "placeholder" line in the file for a product
		 * which has not yet been installed, or has be de-intalled.
		 *
		 * In this case, base_os_version should also be "-", although
		 * it is not checked.
		 */
		if ( STR_EQUAL(driver->extra, product_name) &&
		     (! STR_EQUAL(product_version, "-"))
		   )
		{
			/* This line in the version file applies to this
			 * driver!
			 */
			strcpy(drivers_base_os_version, base_os_version);
		}
	}

	/* get_version_line() returned NULL, so we've reached EOF on the
	 * version file.
	 */
	
	if ( drivers_base_os_version[0] == '\0' )
	{
		/* We never found a line for this driver.
		 * Indictate this failure by returning NULL.
		 */
		return  (Const_intP)NULL;
	}
	else
	{
		/* We did find a line (at least one) for this driver.
		 * Now look up its base-os version# among the versions we
		 * know about.
		 */
		return  sw_release_lookup(drivers_base_os_version);
	}
}


/*
 *	Returns TRUE if the given file exists; FALSE otherwise.
 */
static Bool
file_exists(path)
	char	*path;
{
	extern int access();

	return ( access(path, F_OK) == 0 );
}


Const_intP
get_target_base_OS_version(root_path, driver)
	char		*root_path;
	Const_int	*driver;
{
	FILE		*info_file_fp;
	Const_intP	driver_version;

	/* First, look for the magic version file. */
	info_file_fp = open_driver_versions_file(root_path);

	if (info_file_fp == NULL)
	{
		/* If it's not even present, then we either have a 3.x release,
		 * or a 4.0 release which doesn't have a versions file.
		 *
		 * If the file /usr/lib/crt0.o exists, then we assume that it's
		 * a R4.0 file organization, since R3.x did not have that file.
		 * [this is a grungy(!) way to do it, but we have to decide
		 *  SOME way if the lang_info file isn't present].
		 */
		char	*crt0_filename;

		crt0_filename = path_plus_filename(root_path, "usr/lib/crt0.o");
		/* crt0_filename is now {ROOTPATH}/usr/lib/crt0.o . */

		if ( file_exists(crt0_filename) )
		{
			driver_version = &sw_release_40_implicit;
			if(is_on(debug))
			{
				fprintf(stderr,"[baseOS=4.0 (/usr/lib/crt0.o)]\n");
			}
		}
		else
		{
			driver_version = &sw_release_3x_implicit;
			if(is_on(debug))
			{
				fprintf(stderr,"[baseOS=3.x (!/usr/lib/crt0.o)]\n");
			}
		}

		free(crt0_filename);
	}
	else
	{
		/* The magic version file is here (true for Rel 4.0 & later)!
		 * Scan it for the version number of the product, whose driver
		 * we're impersonating.  Well, actually, don't look at the
		 * PRODUCT's version number; look at the BASE O/S version #.
		 */
		driver_version =
			get_base_os_version(info_file_fp, driver);

		if ( driver_version == NULL )
		{
			fatal("Product \"%s\" is not installed.\n",
				driver->extra);
		}

		close_driver_versions_file(info_file_fp);
	}

	return  driver_version;
}


/* look up what driver version is being used */
static char
*get_driver_version(filep, driver)
	FILE		*filep;
	Const_int	*driver;
{
	char	product_name[LOCAL_STRING_LENGTH];
	char	product_version[LOCAL_STRING_LENGTH];
	char	base_os_version[LOCAL_STRING_LENGTH];
	char	drivers_base_version[LOCAL_STRING_LENGTH];

	drivers_base_version[0] = '\0';

	while ( get_version_line(filep, &product_name[0], &product_version[0],
				 	       &base_os_version[0]) != NULL )
	{
		/* Note that a product version of "-" causes this line in the
		 * info file to be ignored.  This can be used to leave a
		 * "template" or "placeholder" line in the file for a product
		 * which has not yet been installed, or has be de-intalled.
		 *
		 * In this case, base_version should also be "-", although
		 * it is not checked.
		 */
		if ( STR_EQUAL(driver->extra, product_name) &&
		     (! STR_EQUAL(product_version, "-"))
		   )
		{
			/* This line in the version file applies to this
			 * driver!
			 */
			strcpy(drivers_base_version, product_version);
		}
	}

	/* get_version_line() returned NULL, so we've reached EOF on the
	 * version file.
	 */
	
	if ( drivers_base_version[0] == '\0' )
	{
		/* We never found a line for this driver.
		 * Indictate this failure by returning NULL.
		 */
		return  (char)NULL;
	}
	else
	{
		return  (drivers_base_version);
	}
}


char
*get_base_driver_version(root_path, driver)
	char		*root_path;
	Const_int	*driver;
{
	FILE		*info_file_fp;
	char 		*driver_version;

	/* First, look for the magic version file. */
	info_file_fp = open_driver_versions_file(root_path);

	/* if not found, thats ok, because we only need the
	 * driver version for 4.0 and later versions
	 * so return null means it is not found.
	 */
	if (info_file_fp == NULL)
	{
		return make_string("Unknown");
	}
	else
	{
		/* The magic version file is here (true for Rel 4.0 & later)!
		 * Scan it for the version number of the of the driver
		 * product, whose driver we're impersonating.  
		 */
		driver_version =
			get_driver_version(info_file_fp, driver);
			
		close_driver_versions_file(info_file_fp);
	}

	return  make_string(driver_version);

}
