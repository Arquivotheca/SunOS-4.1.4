/* @(#)obj.h	1.1	10/31/94 */
#ifdef SUN3OBJ
extern Bool sun3_seg_name_ok();
#endif
extern unsigned char get_toolversion();
extern void write_obj_file();
extern void objsym_init();
extern SymbolIndex objsym_write_aout_symbol();
extern void unlink_obj_tmp_files();
extern void obj_init();
extern void obj_cleanup();
