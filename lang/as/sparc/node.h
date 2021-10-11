/* @(#)node.h	1.1	10/31/94 */

extern void set_node_info();
extern Node * new_node();
extern void   free_node();
extern Node * new_marker_node();
extern Node * first_node();
extern void   nodes_init();
extern void   add_nodes();
extern void   insert_after_node();
extern void   insert_before_node();
extern void   make_orphan_node();
extern void   make_orphan_block();
extern void   delete_node();
