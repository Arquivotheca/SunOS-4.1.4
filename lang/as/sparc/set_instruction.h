/*	@(#)set_instruction.h	1.1	10/31/94	*/

	/* The three output variants of the SET pseudo-instruction. */
typedef enum { SET_JUST_SETHI, SET_JUST_MOV, SET_SETHI_AND_OR }
	SET_INSTR_VARIATION;

extern SET_INSTR_VARIATION minimal_set_instruction();
