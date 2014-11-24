#include "v.h"
#include "lex.h"

static int emit_expression(size_t expression_id)
{
	if (expression_id == ~0)
		return 1;

	auto& expression = ast[expression_id];

	switch (expression.type)
	{
	case NODE_CONSTANT:
		break;

	case NODE_VARIABLE:
		break;

	default:
		Unimplemented();
		LEX_ERROR("Unexpected expression\n");
	}

	return emit_expression(expression.next_statement);
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
		emit_expression(statement.next_expression);
		break;

	default:
		Unimplemented();
		LEX_ERROR("Unexpected statement\n");
	}

	return emit_statement(statement.next_statement);
}

int emit_procedure(size_t procedure_id)
{
	auto& procedure = ast[procedure_id];

	Assert(procedure.type == NODE_PROCEDURE);

	return emit_statement(procedure.next_statement);
}
