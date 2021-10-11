/*	@(#)relocs.h	1.1	10/31/94	*/

extern void relocate();
extern void resolve_relocations();
#ifdef SUN3OBJ
extern void adjust_concat_relocs();
#endif
extern U32BITS write_relocs();
