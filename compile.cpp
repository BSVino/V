#include "compile.h"

#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "v.h"
#include "vhash.h"
#include "parse.h"
#include "emit.h"

using namespace std;

static vector<size_t> scope_identifiers; // : i -> ast index
static vector<size_t> scope_blocks; // : i -> scope_identifiers

static program_data pd;

void scope_open()
{
	scope_blocks.push_back(scope_identifiers.size());
}

// Pass me the index of an ast node that has a valid value field
int scope_push(size_t identifier_ast_index)
{
	Assert(identifier_ast_index < ast.size() && ast[identifier_ast_index].value != ~0);
	const char* id_name = st_get(ast_st, ast[identifier_ast_index].value);

	// Check for duplicate identifiers in the same scope. If one exists, that is an error.
	for (size_t i = scope_blocks.back(); i < scope_identifiers.size(); i++)
	{
		if (strcmp(st_get(ast_st, ast[scope_identifiers[i]].value), id_name) == 0)
			V_ERROR("Variable already in scope\n");
	}

	scope_identifiers.push_back(identifier_ast_index);

	return 1;
}

size_t scope_find(const char* id_name)
{
	int identifiers_size = (int)scope_identifiers.size() - 1;
	for (int i = identifiers_size; i >= 0; i--)
	{
		size_t ast_index = scope_identifiers[i];
		if (strcmp(st_get(ast_st, ast[ast_index].value), id_name) == 0)
			return ast_index;
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
		if (!(check_expression(expression_node.oper_left) && check_expression(expression_node.oper_right)))
			return 0;
		break;

	case NODE_PROCEDURE_CALL:
	{
		size_t procedure_index = scope_find(st_get(ast_st, expression_node.value));
		V_REQUIRE(procedure_index != ~0, "known procedure");
		V_REQUIRE(ast[procedure_index].type == NODE_PROCEDURE, "known procedure");
		pd.call_graph.push_back(procedure_index);
		break;
	}

	case NODE_RETURN:
		return check_expression(expression_node.next_expression);

	case NODE_DECLARATION:
		V_REQUIRE(scope_find(st_get(ast_st, expression_node.value)) == ~0, "unknown identifier");
		scope_push(expression_id);
		break;

	case NODE_ASSIGN:
		if (!(check_expression(expression_node.oper_left) && check_expression(expression_node.oper_right)))
			return 0;
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

	const char* procedure_name = st_get(ast_st, procedure_node.value);

	bool call_graph_found;
	vhash<procedure_calls>::hash_t call_graph_hash;
	size_t call_graph_index = pd.call_graph_procedures.find(procedure_name, &call_graph_hash, &call_graph_found);

	procedure_calls calls;
	calls.first = pd.call_graph.size();
	calls.status = procedure_calls::UNUSED;
	calls.procedure_list_index = pd.procedure_list.size();
	pd.call_graph_procedures.set(procedure_name, call_graph_index, call_graph_hash, calls);
	pd.procedure_list.push_back(procedure_id);

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
		if (!check_expression(statement_id))
			return 0;

		statement_id = ast[statement_id].next_statement;
	}

	auto* call_graph_entry = pd.call_graph_procedures.get(call_graph_index);
	call_graph_entry->length = pd.call_graph.size() - call_graph_entry->first;

	scope_close();
	return 1;
}

void program_data::init()
{
	call_graph.clear();
	call_graph_procedures.clear();
	procedure_list.clear();
}

int check_ast()
{
	scope_identifiers.clear();
	scope_blocks.clear();
	pd.init();

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

int analyze_call_graph()
{
	vector<const char*> stack;
	stack.push_back("main");

	while (stack.size())
	{
		bool found;
		size_t procedure_index = pd.call_graph_procedures.find(stack.back(), 0, &found);

		stack.pop_back();

		Assert(found);
		if (!found)
			V_ERROR("procedure not found while crawling call graph");

		auto* call = pd.call_graph_procedures.get(procedure_index);
		call->status = procedure_calls::USED;
		for (size_t i = call->first; i < call->first + call->length; i++)
			stack.push_back(st_get(ast_st, ast[pd.call_graph[i]].value));
	}

	return 1;
}

int compile(const char* string, size_t length, std::vector<instruction_t>& program, std::vector<int>& data)
{
	if (!parse_begin(string, length))
		return 0;

	if (!check_ast())
		return 0;

	if (ast_main == ~0)
		V_ERROR("No main procedure found.\n");

	if (!analyze_call_graph())
		return 0;

	if (!emit_begin(ast_main, &pd, &program, &data))
		return 0;

	return 1;
}

int compile_file(const char* filename, std::vector<instruction_t>& program, std::vector<int>& data)
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

