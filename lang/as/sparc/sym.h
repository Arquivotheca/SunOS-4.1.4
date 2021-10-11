/*	@(#)sym.h	1.1	10/31/94	*/

#define SYM_FOUND(p)	((p) != NULL)	/* for return values from *_lookup */
#define SYM_NOTFOUND(p)	((p) == NULL)	/* for return values from *_lookup */

extern	struct symbol *sym_lookup();
extern	struct symbol *sym_add();
extern	struct symbol *sym_new();
extern	struct symbol *sym_first();
extern	struct symbol *sym_next();
extern	void sym_inittbl();
extern	void sym_delete();

extern	struct opcode *opc_lookup();
extern	void opc_inittbl();
