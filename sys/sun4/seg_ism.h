/*	@(#)seg_ism.h 1.1 94/10/31 SMI	*/

#ifndef _dbe_seg_ism_h
#define _dbe_seg_ism_h


/* segismu driver */

struct segismu_data {
	struct shmism_data	*siu_shmism_data;
};

int segismu_create(/* seg, crargs */);  


/* segismk driver */

struct shmism_data {
	/* The first three members must be identical to struct anon_map */
	u_int		refcnt;	    /* reference count on this structure */
	u_int		size;	    /* size in bytes mapped by the anon array */
	struct		anon **anon;/* pointer to an array of anon * pointers */
	struct as	*as;
};

int segismk_create(/* seg, crargs */);

/* Public functions */
struct shmism_data * 	ismcreate(/* size */);
void 			ismdestroy(/* shmism_data */);
struct shmism_data *	ismlookup(/* as, addr */);
int 			ismalign_p(/* addr */);


#endif !_dbe_seg_ism_h
