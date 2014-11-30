#include "emit.h"

#include <algorithm> // for min/max

#include "vhash.h"

#include "v.h"
#include "parse.h"
#include "vm.h"
#include "optimize.h"

using namespace std;

static vector<int>* program;
static vector<int>* data;

static vhash<unsigned short> v2r; // A map of variables to registers
static vector<const char*>   r2v; // A map of registers to variables
static size_t next_const;

static vector<instruction_3ac> procedure_3ac; // The procedure in three-address code

static size_t emit_find_register(const char* variable)
{
	Assert(strncmp(variable, EMIT_CONST_REGISTER, 9) != 0);

	vhash<unsigned short>::hash_t hash;
	bool found;
	size_t it = v2r.find(variable, &hash, &found);
	if (found)
		return *v2r.get(it);
	else
	{
		size_t index = r2v.size();
		v2r.set(variable, it, hash, index);
		r2v.push_back(variable);
		return index;
	}
}

static size_t emit_auto_register()
{
	static char str[100];
	sprintf(str, EMIT_CONST_REGISTER "%d", next_const++);

	Assert(!v2r.entry_exists(str));

	size_t index = r2v.size();
	v2r.set(str, index);
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
		procedure_3ac.back().flags = instruction_3ac::I3AC_NONE;
		break;

	case NODE_VARIABLE:
		*result_register = emit_find_register(st_get(ast_st, expression.value));
		break;

	case NODE_SUM:
	{
		size_t left, right;
		emit_expression(expression.oper_left, &left);
		emit_expression(expression.oper_right, &right);

		*result_register = emit_auto_register();
		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().i = I3_ADD;
		procedure_3ac.back().r_dest = *result_register;
		procedure_3ac.back().r_arg1 = left;
		procedure_3ac.back().r_arg2 = right;
		procedure_3ac.back().flags = instruction_3ac::I3AC_NONE;
		break;
	}

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
		procedure_3ac.back().r_dest = EMIT_RETURN_REGISTER_INDEX;
		procedure_3ac.back().r_arg1 = expression_register;

		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().i = I3_JUMP;
		procedure_3ac.back().r_arg1 = EMIT_JUMP_END_OF_PROCEDURE;
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
		if (instruction.r_dest == EMIT_RETURN_REGISTER_INDEX)
			unique_register = EMIT_RETURN_REGISTER_INDEX;

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

static void emit_convert_to_bytecode(vector<instruction_3ac>& input)
{
	for (size_t i = 0; i < input.size(); i++)
	{
		auto& in_i = input[i];

		switch (in_i.i)
		{
		case I3_DATA:
			data->push_back(DATA(in_i.r_arg1));
			//program->push_back(INSTRUCTION(I_MOVE, data->size()-1, 0));
		}
	}
}

// timeline maps ssa registers (variables) to their first write and last reads.
// variable_registers maps ssa registers (variables) to their destination registers for the VM.
static void emit_allocate_registers(size_t num_target_registers, vector<size_t>* variable_registers, vector<register_timeline>* timeline)
{
	variable_registers->clear();
	variable_registers->resize(num_target_registers);

	memset(variable_registers->data(), ~0, sizeof(size_t)*variable_registers->size());

	// Horrible n^2 suboptimal algorithm but should be good enough for my purposes.
	for (size_t i = 0; i < timeline->size(); i++)
	{
		auto& variable = (*timeline)[i];

		if (variable.write_instruction == ~0 || variable.last_read == ~0)
			continue;

		size_t possible_registers = ~0;

		for (size_t j = 0; j < timeline->size(); j++)
		{
			auto& other = (*timeline)[j];

			if (other.write_instruction == ~0 || other.last_read == ~0)
				continue;

			if ((*variable_registers)[j] == ~0)
				continue;

			Unimplemented();
			// If 'other' has already been allocated a register, check if we share a live time with it
			size_t last_write = std::max(variable.write_instruction, other.write_instruction);
			size_t first_read = std::min(variable.last_read, other.last_read);

			if (last_write <= first_read)
				possible_registers &= ~j;
		}

		for (size_t j = 0; j < sizeof(size_t)*8; j++)
		{
			if (possible_registers & (1 << j))
			{
				(*variable_registers)[i] = j;
				break;
			}
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
	size_t return_register = emit_find_register(EMIT_RETURN_REGISTER); // Reserve the first value for the return address
	Assert(return_register == EMIT_RETURN_REGISTER_INDEX);

	if (!emit_statement(procedure.next_statement))
		return 0;

	emit_convert_to_ssa(procedure_3ac);

	vector<register_timeline> timeline;
	optimize_copy_propagation(&procedure_3ac, &timeline);

	vector<size_t> variable_registers;
	emit_allocate_registers((size_t)(R_12-R_1+1), &variable_registers, &timeline);

	program->clear();
	for (size_t i = 0; i < procedure_3ac.size(); i++)
	{
		auto* instruction = &procedure_3ac[i];

		if (instruction->flags & instruction_3ac::I3AC_UNUSED)
			continue;

		switch (instruction->i)
		{
		case I3_DATA:
			if (instruction->r_arg1 < (1 << ARG1_BITS))
			{
				Assert(variable_registers[instruction->r_dest] != ~0);
				program->push_back(INSTRUCTION(I_MOVE, R_1 + variable_registers[instruction->r_dest], instruction->r_arg1));
			}
			else
			{
				Assert(variable_registers[instruction->r_dest] != ~0);
				data->push_back(DATA(instruction->r_arg1));
				program->push_back(INSTRUCTION(I_MOVE, R_1 + variable_registers[instruction->r_dest], data->size() - 1));
				program->push_back(INSTRUCTION(I_DATALOAD, R_1 + variable_registers[instruction->r_dest], R_1 + variable_registers[instruction->r_dest]));
			}
			break;

		case I3_MOVE:
			Assert(variable_registers[instruction->r_dest] != ~0);
			Assert(variable_registers[instruction->r_arg1] != ~0);
			program->push_back(INSTRUCTION(I_MOVE, R_1 + variable_registers[instruction->r_dest], R_1 + variable_registers[instruction->r_arg1]));
			break;

		case I3_JUMP:
			if (instruction->r_arg1 == EMIT_JUMP_END_OF_PROCEDURE)
				program->push_back(INSTRUCTION(I_DIE, 0, 0));
			else
				Unimplemented();
			break;

		default:
			Unimplemented();
			break;
		}
	}

	return 1;
}

int emit_begin(size_t main_procedure, std::vector<int>& input_program, std::vector<int>& input_data)
{
	input_program.clear();
	input_data.clear();
	program = &input_program;
	data = &input_data;

	if (!emit_procedure(main_procedure))
		return 0;

	program->push_back(INSTRUCTION(I_DIE, 0, 0));

	return 1;
}
