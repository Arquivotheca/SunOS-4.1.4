/*	@(#)spt.h  1.1 94/10/31 SMI	*/

struct spt_data {
	u_int		refcnt;
	u_int		size;
	struct anon	**anon;
	struct as	*as;
};


/* Functions used in ipc_shm.c due to spt */

struct spt_data	*sptcreate(/*size*/);
void   sptdestroy(/* spt */);
struct spt_data *spt_lookup();
int segspt_shmattach();

