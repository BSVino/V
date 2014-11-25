#include "v.h"
#include "parse.h"
#include "vm.h"

static std::vector<int>* program;
static std::vector<int>* data;

static int registers_used[REGISTERS] = { 0 };

static register_t emit_find_unused_register()
{
	for (int i = R_1; i < REGISTERS; i++)
	{
		if (!registers_used[i])
			return (register_t)i;
	}

	return R_NONE;
}

static register_t emit_reserve_register()
{
	register_t r = emit_find_unused_register();

	if (r == R_NONE)
		// Push it
		Unimplemented();

	registers_used[r] = 1;
	return r;
}

static void emit_reserve_register(register_t r)
{
	if (registers_used[r])
		// Push it
		Unimplemented();

	registers_used[r] = 1;
}

static void emit_free_register(register_t r)
{
	registers_used[r] = 0;
}

static int emit_expression(size_t expression_id, register_t result)
{
	if (expression_id == ~0)
		return 1;

	auto& expression = ast[expression_id];

	switch (expression.type)
	{
	case NODE_CONSTANT:
	{
		register_t r = emit_reserve_register();
		data->push_back(DATA(atoi(st_get(ast_st, expression.value))));
		program->push_back(INSTRUCTION(I_MOVE, result, data->size() - 1));
		program->push_back(INSTRUCTION(I_DATA, r, result));
		program->push_back(INSTRUCTION(I_LOAD, result, r));
		emit_free_register(r);
		break;
	}

	case NODE_VARIABLE:
		break;

	default:
		Unimplemented();
		V_ERROR("Unexpected expression\n");
	}

	return emit_expression(expression.next_statement, result);
}

static int emit_statement(size_t statement_id)
{
	if (statement_id == ~0)
		return 1;

	auto& statement = ast[statement_id];

	switch (statement.type)
	{
	case NODE_DECLARATION:
		break;

	case NODE_RETURN:
		emit_reserve_register(R_1);
		emit_expression(statement.next_expression, R_1);
		break;

	default:
		Unimplemented();
		V_ERROR("Unexpected statement\n");
	}

	return emit_statement(statement.next_statement);
}

static int emit_procedure(size_t procedure_id)
{
	auto& procedure = ast[procedure_id];

	Assert(procedure.type == NODE_PROCEDURE);

	return emit_statement(procedure.next_statement);
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
