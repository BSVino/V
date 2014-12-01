#include "emit.h"

#include <algorithm> // for min/max

#include "vhash.h"

#include "v.h"
#include "parse.h"
#include "vm.h"
#include "optimize.h"

using namespace std;

static vhash<unsigned short> v2r; // A map of variables to registers
static vector<const char*>   r2v; // A map of registers to variables
static size_t next_const;

static vector<instruction_3ac> procedure_3ac; // The procedure in three-address code

struct procedure_info
{
	struct symbol_relocations
	{
		size_t    position;  // : i -> bytecode index
		st_string procedure; // : i -> ast_st
	};

	vector<instruction_t> bytecode;
	vector<symbol_relocations> relocations;
};

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

static int emit_expression(size_t expression_id, size_t* result_register, procedure_info* pi)
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
	case NODE_DIFFERENCE:
	case NODE_PRODUCT:
	case NODE_QUOTIENT:
	{
		size_t left, right;
		if (!emit_expression(expression.oper_left, &left, pi))
			return 0;

		if (!emit_expression(expression.oper_right, &right, pi))
			return 0;

		*result_register = emit_auto_register();
		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().r_dest = *result_register;
		procedure_3ac.back().r_arg1 = left;
		procedure_3ac.back().r_arg2 = right;
		procedure_3ac.back().flags = instruction_3ac::I3AC_NONE;

		// Deja vu
		switch (expression.type)
		{
		case NODE_SUM:        procedure_3ac.back().i = I3_ADD; break;
		case NODE_DIFFERENCE: procedure_3ac.back().i = I3_SUBTRACT; break;
		case NODE_PRODUCT:    procedure_3ac.back().i = I3_MULTIPLY; break;
		case NODE_QUOTIENT:   procedure_3ac.back().i = I3_DIVIDE; break;
		default: Unimplemented();
		}
		break;
	}

	case NODE_PROCEDURE_CALL:
		pi->relocations.push_back(procedure_info::symbol_relocations());
		pi->relocations.back().position = procedure_3ac.size();
		pi->relocations.back().procedure = expression.value;

		*result_register = emit_auto_register();

		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().flags = instruction_3ac::I3AC_NONE;
		procedure_3ac.back().i = I3_CALL;
		procedure_3ac.back().call_relocation = pi->relocations.size() - 1;
		procedure_3ac.back().r_dest = *result_register;
		break;

	default:
		Unimplemented();
		V_ERROR("Unexpected expression\n");
	}

	return emit_expression(expression.next_statement, result_register, pi);
}

static int emit_statement(size_t statement_id, procedure_info* pi)
{
	if (statement_id == ~0)
		return 1;

	auto& statement = ast[statement_id];

	switch (statement.type)
	{
	case NODE_DECLARATION:
		if (statement.next_expression != ~0)
		{
			size_t expression_register;

			if (!emit_expression(statement.next_expression, &expression_register, pi))
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
		if (!emit_expression(statement.next_expression, &expression_register, pi))
			return 0;

		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().i = I3_RETURN;
		procedure_3ac.back().r_arg1 = expression_register;
		return 1;

	case NODE_ASSIGN:
	{
		size_t left, right;
		if (!emit_expression(statement.oper_left, &left, pi))
			return 0;

		if (!emit_expression(statement.oper_right, &right, pi))
			return 0;

		procedure_3ac.push_back(instruction_3ac());
		procedure_3ac.back().i = I3_MOVE;
		procedure_3ac.back().r_dest = left;
		procedure_3ac.back().r_arg1 = right;
		break;
	}

	default:
		Unimplemented();
		V_ERROR("Unexpected statement\n");
	}

	return emit_statement(statement.next_statement, pi);
}

static bool emit_uses_register_arguments(instruction_3ac_t i)
{
	switch (i)
	{
	case I3_DATA:
		return false;
	case I3_MOVE:
	case I3_ADD:
	case I3_SUBTRACT:
	case I3_MULTIPLY:
	case I3_DIVIDE:
		return true;
	case I3_CALL:
		return false;
	case I3_RETURN:
		return true;

	default:
		Unimplemented();
		return false;
	}
}

static bool emit_uses_both_arguments(instruction_3ac_t i)
{
	switch (i)
	{
	case I3_DATA:
	case I3_MOVE:
		return false;
	case I3_ADD:
	case I3_SUBTRACT:
	case I3_MULTIPLY:
	case I3_DIVIDE:
		return true;
	case I3_CALL:
	case I3_RETURN:
		return false;

	default:
		Unimplemented();
		return false;
	}
}

static bool emit_writes_dest(instruction_3ac_t i)
{
	switch (i)
	{
	case I3_DATA:
	case I3_MOVE:
	case I3_ADD:
	case I3_SUBTRACT:
	case I3_MULTIPLY:
	case I3_DIVIDE:
	case I3_CALL:
		return true;
	case I3_RETURN:
		return false;

	default:
		Unimplemented();
		return false;
	}
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
		size_t unique_register = i;

		auto& instruction = input[i];

		if (emit_writes_dest(instruction.i))
		{
			register_map[instruction.r_dest] = unique_register;

			instruction.r_dest = unique_register;
		}

		if (emit_uses_register_arguments(instruction.i))
		{
			// These instructions use registers as their arguments. Replace them with the new one.
			Assert(register_map[instruction.r_arg1] != ~0);
			instruction.r_arg1 = register_map[instruction.r_arg1];

			if (emit_uses_both_arguments(instruction.i))
				instruction.r_arg2 = register_map[instruction.r_arg2];
		}
	}
}

static void emit_convert_to_bytecode(vector<instruction_3ac>* input, vector<size_t>* variable_registers, vector<instruction_t>* program, vector<int>* data, procedure_info* pi)
{
	program->clear();
	for (size_t i = 0; i < procedure_3ac.size(); i++)
	{
		auto* instruction = &procedure_3ac[i];

		if (instruction->flags & instruction_3ac::I3AC_UNUSED)
			continue;

		switch (instruction->i)
		{
		case I3_DATA:
			if (instruction->r_arg1 < (1 << (ARG1_BITS-1)))
			{
				Assert((*variable_registers)[instruction->r_dest] != ~0);
				program->push_back(INSTRUCTION(I_MOVE, R_1 + (*variable_registers)[instruction->r_dest], instruction->r_arg1));
			}
			else
			{
				Assert((*variable_registers)[instruction->r_dest] != ~0);
				data->push_back(DATA(instruction->r_arg1));
				program->push_back(INSTRUCTION(I_MOVE, R_1 + (*variable_registers)[instruction->r_dest], data->size() - 1));
				program->push_back(INSTRUCTION(I_DATALOAD, R_1 + (*variable_registers)[instruction->r_dest], R_1 + (*variable_registers)[instruction->r_dest]));
			}
			break;

		case I3_MOVE:
			Assert((*variable_registers)[instruction->r_dest] != ~0);
			Assert((*variable_registers)[instruction->r_arg1] != ~0);
			program->push_back(INSTRUCTION(I_MOVE, R_1 + (*variable_registers)[instruction->r_dest], R_1 + (*variable_registers)[instruction->r_arg1]));
			break;

		case I3_CALL:
			program->push_back(INSTRUCTION(I_CALL, 0, 0));
			Unimplemented(); // Have to update the relocation table
			break;

		case I3_RETURN:
			program->push_back(INSTRUCTION(I_RETURN, 0, 0));
			break;

		default:
			Unimplemented();
			break;
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

static int emit_procedure(size_t procedure_id, std::vector<instruction_t>* program, std::vector<int>* program_data, procedure_info* pi)
{
	auto& procedure = ast[procedure_id];

	Assert(procedure.type == NODE_PROCEDURE);

	procedure_3ac.clear();
	v2r.clear();
	r2v.clear();
	next_const = 0;

	if (!emit_statement(procedure.next_statement, pi))
		return 0;

	emit_convert_to_ssa(procedure_3ac);

	vector<register_timeline> timeline;
	optimize_copy_propagation(&procedure_3ac, &timeline);

	vector<size_t> variable_registers;
	emit_allocate_registers((size_t)(R_12-R_1+1), &variable_registers, &timeline);

	emit_convert_to_bytecode(&procedure_3ac, &variable_registers, program, program_data, pi);

	return 1;
}

int emit_begin(size_t main_procedure, program_data* pd, std::vector<instruction_t>* program, std::vector<int>* data)
{
	program->clear();
	data->clear();

	vector<procedure_info> procedures; // indexed the same as pd->procedure_list
	procedures.resize(pd->procedure_list.size());

	size_t main_procedure_info = ~0;

	size_t total_procedures_size = 0;

	for (size_t i = 0; i < pd->procedure_list.size(); i++)
	{
		if (pd->procedure_list[i] == main_procedure)
			main_procedure_info = i;

		const char* identifier = st_get(ast_st, ast[pd->procedure_list[i]].value);

		auto* procedure_call = pd->call_graph_procedures.get(identifier);
		if (procedure_call->status == procedure_calls::UNUSED)
			continue;

		size_t procedure = pd->procedure_list[i];
		vector<instruction_t>* bytecode = &procedures[i].bytecode;
		procedure_info* procedure_info = &procedures[i];
		if (!emit_procedure(procedure, bytecode, data, procedure_info))
			return 0;

		total_procedures_size += procedures[i].bytecode.size();
	}

	Assert(main_procedure_info != ~0);
	if (main_procedure_info == ~0)
		return 0;

	program->push_back(INSTRUCTION(I_MOVE, R_1, 1)); // We measure from the end of the call instruction, so we just skip the die.
	program->push_back(INSTRUCTION(I_CALL, R_1, 0));
	program->push_back(INSTRUCTION(I_DIE, 0, 0));

	size_t main_location = 3;

	Assert(program->size() == main_location);
	Assert(main_location < (1<<3)); // This is all that can fit in an I_MOVE instruction. Otherwise use data.

	program->resize(total_procedures_size + main_location);

	size_t current_instruction = main_location;

	{
		auto& main_procedure_bytecode = procedures[main_procedure_info].bytecode;

		memcpy(&(*program)[current_instruction], &main_procedure_bytecode[0], sizeof(instruction_t) * main_procedure_bytecode.size());

		current_instruction += main_procedure_bytecode.size();
	}

	for (size_t i = 0; i < pd->procedure_list.size(); i++)
	{
		if (pd->procedure_list[i] == main_procedure)
			continue;

		auto& procedure_bytecode = procedures[pd->procedure_list[i]].bytecode;

		memcpy(&(*program)[current_instruction], procedure_bytecode.data(), sizeof(instruction_t) * procedure_bytecode.size());

		current_instruction += procedure_bytecode.size();
	}

	return 1;
}
