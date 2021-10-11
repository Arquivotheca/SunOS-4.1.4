#
# @(#)mkpriv.awk	1.1 (Sun) 10/31/94
#	Awk program to strip prefix qualifiers from
#	names in public include files.  A cheap substitute
#	for controlled name scoping.  We only deal with preprocessor
#	directives here; C declarations should be done manually,
#	if at all.
#
{
    if ($1 == "#") {
	#
	# preprocessor directive
	#
	if (($3 ~ /pcc_/) || ($3 ~ /PCC_/)) {
	    #
	    # qualified name defined or used
	    #
	    if ($2 == "define") {
		#
		# public definition; define private name in terms
		# of the public one
		#
		printf("# define %s\t\t%s\n", substr($3,5), $3);
	    } else if (substr($2,1,2) == "if" || $2 == "endif") {
		#
		# conditional compilation directive;
		# emit the same line with the private name
		#
		printf("# %s %s\n", $2, substr($3,5));
		if ($2 == "ifndef") {
		    #
		    # include the public definition file
		    #
		    printf("# include \"%s.h\"\n", substr($3,2,length($3)-2));
		}
	    }
	} else if (substr($2,1,2) == "if" || $2 == "endif") {
	    #
	    # conditional compilation control. Pass it along.
	    #
	    print $0;
	}
    } else {
	#
	# do nothing; it's already been said in the public file.
	#
    }
}
