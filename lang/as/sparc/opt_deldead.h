/* @(#)opt_deldead.h	1.1	10/31/94	*/

#define first_operand_is_zero(np)   ((np)->n_operands.op_regs[O_RS1] == RN_G0)

extern Bool second_operand_is_zero();
extern Bool second_operand_is_one();
extern Bool is_mov_equivalent();
extern Bool convert_mov_equivalent_into_real_mov();
