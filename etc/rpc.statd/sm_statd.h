/*      @(#)sm_statd.h 1.1 94/10/31 SMI                              */

struct stat_chge {
	char *name;
	int state;
};
typedef struct stat_chge stat_chge;

#define SM_NOTIFY 6



