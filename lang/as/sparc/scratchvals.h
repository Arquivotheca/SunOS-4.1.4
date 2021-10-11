/* @(#)scratchvals.h	1.1	10/31/94 */

extern struct value	    *get_scratch_value();
extern void		     reset_scratch_values();
extern struct val_list_head *new_val_list_head();
extern void		     free_scratch_value();
extern void		     free_value_list();
