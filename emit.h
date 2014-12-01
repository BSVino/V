#pragma once

#include <vector>

#include "vhash.h"

#include "compile.h"

extern int emit_begin(size_t main_procedure, program_data* pd, std::vector<int>* program, std::vector<int>* data);

typedef enum {
	I3_DATA,     // dest <- arg1          arg1 a constant
	I3_MOVE,     // dest <- arg1          arg1 a register
	I3_ADD,      // dest <- arg1 + arg2   registers
	I3_SUBTRACT, // dest <- arg1 - arg2   registers
	I3_MULTIPLY, // dest <- arg1 * arg2   registers
	I3_DIVIDE,   // dest <- arg1 / arg2   registers
	I3_CALL,     // (eip + arg1)()        arg1 a register
	I3_RETURN,   // return arg1           arg1 a register
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

	union
	{
		struct { // Call
			size_t call_relocation; // procedure_info::relocations[call_relocation]
		};
	};
};

#define EMIT_CONST_REGISTER "__v_const"

