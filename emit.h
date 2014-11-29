#pragma once

#include <vector>

extern int emit_begin(size_t procedure_id, std::vector<int>& program, std::vector<int>& data);

typedef enum {
	I3_JUMP,     // Jump to label #arg1.
	I3_DATA,     // dest <- arg1 -- arg1 is a constant
	I3_MOVE,     // dest <- arg1 -- arg1 is a register
} instruction_3ac_t;

struct instruction_3ac
{
	size_t r_dest;
	size_t r_arg1;
	size_t r_arg2;
	instruction_3ac_t i;
	typedef enum {
		I3AC_NONE = 0,
		I3AC_UNUSED = (1 << 0),
	} flags_e;
	flags_e flags;
};

#define EMIT_CONST_REGISTER "__v_const"
#define EMIT_RETURN_REGISTER "__v_return"
#define EMIT_RETURN_REGISTER_INDEX 0
#define EMIT_JUMP_END_OF_PROCEDURE (size_t)(~0)

