/*	@(#)segments.h	1.1	10/31/94	*/

extern void segments_init();
extern void change_segment();
extern void change_named_segment();
extern void close_all_segments();
extern void do_for_each_segment();
extern void optimize_all_segments();
extern void emit_byte();
extern void emit_halfword();
extern void emit_word();
extern void align();
extern void emit_skipbytes();

extern LOC_COUNTER get_dot();
extern SEGMENT get_dot_seg();
extern char   *get_dot_seg_name();
struct symbol *get_dot_seg_symp();

extern U8BITS *get_obj_code_ptr();

extern U32BITS get_seg_size();
extern SEGMENT get_seg_number();
extern SEGMENT lookup_seg_number();

#ifdef SUN3OBJ
  extern void ABSwrite_seg();
#endif /*SUN3OBJ*/

#ifdef DEBUG
  extern char * get_seg_name();
  extern void emit_instr();
#else /*DEBUG*/
#  define emit_instr(i) emit_word(i)
#endif /*DEBUG*/
