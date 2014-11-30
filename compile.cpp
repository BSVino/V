#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "v.h"
#include "parse.h"
#include "emit.h"

using namespace std;

vector<size_t> scope_identifiers;
vector<size_t> scope_blocks;

void scope_open()
{
	scope_blocks.push_back(scope_identifiers.size());
}

int scope_push(size_t identifier)
{
	const char* id_name = st_get(ast_st, ast[identifier].value);

	// Check for duplicate identifiers in the same scope. If one exists, that is an error.
	for (size_t i = scope_blocks.back(); i < scope_identifiers.size(); i++)
	{
		if (strcmp(st_get(ast_st, ast[scope_identifiers[i]].value), id_name) == 0)
			V_ERROR("Variable already in scope\n");
	}

	scope_identifiers.push_back(identifier);

	return 1;
}

size_t scope_find(const char* id_name)
{
	for (int i = (int)scope_identifiers.size()-1; i > 0; i--)
	{
		if (strcmp(st_get(ast_st, ast[scope_identifiers[i]].value), id_name) == 0)
			return i;
	}

	return ~0;
}

void scope_close()
{
	while (scope_identifiers.size() > scope_blocks.back())
		scope_identifiers.pop_back();

	scope_blocks.pop_back();
}

int check_expression(size_t expression_id)
{
	if (expression_id == ~0)
		return 1;

	auto& expression_node = ast[expression_id];

	switch (expression_node.type)
	{
	case NODE_CONSTANT:
		break;

	case NODE_VARIABLE:
		V_REQUIRE(scope_find(st_get(ast_st, expression_node.value)) != ~0, "known identifier");
		break;

	case NODE_SUM:
	case NODE_DIFFERENCE:
	case NODE_PRODUCT:
	case NODE_QUOTIENT:
		break;

	default:
		Unimplemented();
		break;
	}

	return check_expression(expression_node.next_expression);
}

int check_procedure(size_t procedure_id)
{
	scope_open();

	auto& procedure_node = ast[procedure_id];

	// Add the parameters to the scope
	size_t parameter_id = procedure_node.proc_first_parameter;
	while (parameter_id != ~0)
	{
		auto& parameter = ast[parameter_id];

		Assert(parameter.parent == procedure_id);

		if (parameter.type == NODE_DECLARATION)
			V_REQUIRE(scope_push(parameter_id), "unique identifier name");

		parameter_id = parameter.decl_next_parameter;
	}

	size_t statement_id = procedure_node.next_statement;
	while (statement_id != ~0)
	{
		auto& statement = ast[statement_id];

		switch (statement.type)
		{
		case NODE_DECLARATION:
			V_REQUIRE(scope_push(statement_id), "unique identifier name");
			break;

		case NODE_RETURN:
			if (!check_expression(statement.next_expression))
				return 0;
			break;

		case NODE_ASSIGN:
			break;

		default:
			Unimplemented();
			break;
		}

		statement_id = statement.next_statement;
	}

	scope_close();
	return 1;
}

int check_ast()
{
	scope_identifiers.clear();
	scope_blocks.clear();

	scope_open();

	// First add all identifiers in global scope.
	for (size_t i = 0; i < ast_globals.size(); i++)
	{
		size_t id = ast_globals[i];
		auto& node = ast[id];

		Assert(node.parent == ~0);

		if (node.type == NODE_PROCEDURE)
			V_REQUIRE(scope_push(id), "unique identifier name");
	}

	for (size_t i = 0; i < ast_globals.size(); i++)
	{
		size_t id = ast_globals[i];
		auto& node = ast[id];

		Assert(node.parent == ~0);

		if (node.type == NODE_PROCEDURE)
		{
			if (!check_procedure(id))
				return 0;
		}
	}

	scope_close(); // You know, for completeness.

	Assert(scope_identifiers.size() == 0);
	Assert(scope_blocks.size() == 0);

	return 1;
}

int compile(const char* string, size_t length, std::vector<int>& program, std::vector<int>& data)
{
	if (!parse_begin(string, length))
		return 0;

	if (!check_ast())
		return 0;

	if (ast_main == ~0)
		V_ERROR("No main procedure found.\n");

	if (!emit_begin(ast_main, program, data))
		return 0;

	return 1;
}

int compile_file(const char* filename, std::vector<int>& program, std::vector<int>& data)
{
	FILE* fp = fopen(filename, "rb");

	if (!fp)
		return 0;

	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	rewind(fp);

	char* file_contents = (char*)malloc(file_size + 1);

	size_t read = fread(file_contents, 1, file_size, fp);
	file_contents[file_size] = 0;

	fclose(fp);

	int result = compile(file_contents, file_size, program, data);

	free(file_contents);

	return result;
}

