#include <map>

#include "v.h"
#include "parse.h"
#include "vm.h"

using namespace std;

static vector<int>* program;
static vector<int>* data;

static map<const char*, size_t> v2r; // A map of variables to registers
static vector<const char*>      r2v;   // A map of registers to variables
static size_t next_const;

#define CONST_REGISTER "__v_const"
#define RETURN_REGISTER "__v_return"
#define RETURN_REGISTER_INDEX 0
#define JUMP_END_OF_PROCEDURE (size_t)(~0)

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
};

static vector<instruction_3ac> procedure_3ac; // The procedure in three-address code

static size_t emit_find_register(const char* variable)
{
	Assert(strncmp(variable, CONST_REGISTER, 9) != 0);

	auto& it = v2r.find(variable);
	if (it == v2r.end())
	{
		size_t index = r2v.size();
		v2r[variable] = index;
		r2v.push_back(variable);
		return index;
	}
	else
		return it->second;
}

static size_t emit_auto_register()
{
	static char str[100];
	sprintf(str, CONST_REGISTER "%d", next_const);

	Assert(v2r.find(str) == v2r.end());

	size_t index = r2v.size();
	v2r[str] = index;
	r2v.push_back(str);
	return index;
}

static int emit_expression(size_t expression_id, size_t* result_register)
{
	if (expression_id == ~0)
		return 1;

	auto& expression = ast[expression_id];

	switch (expression.type)
	{
	case NODE_CONSTANT:
		*result_register = emit_auto_register();
		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().i = I3_DATA;
		procedure_3ac.back().r_dest = *result_register;
		procedure_3ac.back().r_arg1 = atoi(st_get(ast_st, expression.value));
		break;

	case NODE_VARIABLE:
		*result_register = emit_find_register(st_get(ast_st, expression.value));
		break;

	default:
		Unimplemented();
		V_ERROR("Unexpected expression\n");
	}

	return emit_expression(expression.next_statement, result_register);
}

static int emit_statement(size_t statement_id)
{
	if (statement_id == ~0)
		return 1;

	auto& statement = ast[statement_id];

	switch (statement.type)
	{
	case NODE_DECLARATION:
		if (statement.next_expression)
		{
			size_t expression_register;

			if (!emit_expression(statement.next_expression, &expression_register))
				return 0;

			size_t target = emit_find_register(st_get(ast_st, statement.value));

			procedure_3ac.push_back(instruction_3ac());
			procedure_3ac.back().i = I3_MOVE;
			procedure_3ac.back().r_dest = target;
			procedure_3ac.back().r_arg1 = expression_register;
		}
		break;

	case NODE_RETURN:
		size_t expression_register;
		if (!emit_expression(statement.next_expression, &expression_register))
			return 0;

		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().i = I3_MOVE;
		procedure_3ac.back().r_dest = RETURN_REGISTER_INDEX;
		procedure_3ac.back().r_arg1 = expression_register;

		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().i = I3_JUMP;
		procedure_3ac.back().r_arg1 = JUMP_END_OF_PROCEDURE;
		return 1;

	default:
		Unimplemented();
		V_ERROR("Unexpected statement\n");
	}

	return emit_statement(statement.next_statement);
}

static void emit_convert_to_ssa(vector<instruction_3ac>& input)
{
	vector<size_t> register_map;
	register_map.resize(input.size() + 1);

#ifdef _DEBUG
	memset(register_map.data(), ~0, sizeof(size_t) * register_map.size());
#endif

	for (size_t i = 0; i < input.size(); i++)
	{
		size_t unique_register = i + 1;

		auto& instruction = input[i];

		// This is okay because the emit code reserves the 0 register
		// until just before a return, it's never used otherwise.
		if (instruction.r_dest == RETURN_REGISTER_INDEX)
			unique_register = RETURN_REGISTER_INDEX;

		register_map[instruction.r_dest] = unique_register;

		instruction.r_dest = unique_register;

		if (instruction.i != I3_JUMP && instruction.i != I3_DATA)
		{
			// These instructions use registers as their arguments. Replace them with the new one.
			Assert(register_map[instruction.r_arg1] != ~0);
			instruction.r_arg1 = register_map[instruction.r_arg1];

			if (instruction.i != I3_MOVE)
				instruction.r_arg2 = register_map[instruction.r_arg2];
		}
	}
}

static int emit_procedure(size_t procedure_id)
{
	auto& procedure = ast[procedure_id];

	Assert(procedure.type == NODE_PROCEDURE);

	procedure_3ac.clear();
	v2r.clear();
	r2v.clear();
	next_const = 0;
	size_t return_register = emit_find_register(RETURN_REGISTER); // Reserve the first value for the return address
	Assert(return_register == RETURN_REGISTER_INDEX);

	if (!emit_statement(procedure.next_statement))
		return 0;

	emit_convert_to_ssa(procedure_3ac);

	return 1;
}

int emit_begin(size_t procedure_id, std::vector<int>& input_program, std::vector<int>& input_data)
{
	input_program.clear();
	input_data.clear();
	program = &input_program;
	data = &input_data;

	if (!emit_procedure(procedure_id))
		return 0;

	program->push_back(INSTRUCTION(I_DIE, 0, 0));

	return 1;
}
