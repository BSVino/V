#include "parse.h"

#include <algorithm>

#include "stringtable.h"

using namespace std;

static const char* p;
static const char* p_end;

static token_t     token;
static const char* token_string;
static size_t      token_length;

vector<ast_node> ast;
vector<char> ast_st;
size_t ast_main = ~0;
vector<size_t> ast_globals;

static int lex_strcmp(const char* s1, const char* s2, size_t n1, size_t n2)
{
	return strncmp(s1, s2, min(n1, n2));
}

static token_t lex_next()
{
	if (p == p_end)
		return token = TOKEN_EOF;

	while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t' && p <= p_end)
		p++;

	if (p == p_end)
		return token = TOKEN_EOF;

	token_string = p;

	if (*p >= 'a' && *p <= 'z' || *p >= 'A' && *p <= 'Z')
	{
		token = TOKEN_IDENTIFIER;
		while (*p >= 'a' && *p <= 'z' || *p >= 'A' && *p <= 'Z' || *p >= '0' && *p <= '9')
			p++;

		token_length = p - token_string;

		if (lex_strcmp(token_string, "int", token_length, 3) == 0)
			return token = TOKEN_INT;
		else if (lex_strcmp(token_string, "return", token_length, 6) == 0)
			return token = TOKEN_RETURN;

		return token;
	}
	else if (*p >= '0' && *p <= '9')
	{
		token = TOKEN_NUMBER;
		while (*p >= '0' && *p <= '9' || *p == '.')
			p++;

		token_length = p - token_string;

		return token;
	}
	else if (*p == ':' && *(p + 1) == '=')
		return p += 2, token = TOKEN_DECLARE_ASSIGN;
	else if (*p == ':')
		return p++, token = TOKEN_DECLARE;
	else if (*p == '=')
		return p++, token = TOKEN_ASSIGN;
	else if (*p == '(')
		return p++, token = TOKEN_OPEN_PAREN;
	else if (*p == ')')
		return p++, token = TOKEN_CLOSE_PAREN;
	else if (*p == '{')
		return p++, token = TOKEN_OPEN_CURLY;
	else if (*p == '}')
		return p++, token = TOKEN_CLOSE_CURLY;
	else if (*p == ',')
		return p++, token = TOKEN_COMMA;
	else if (*p == ';')
		return p++, token = TOKEN_SEMICOLON;
	else if (*p == '+')
		return p++, token = TOKEN_PLUS;
	else if (*p == '-')
		return p++, token = TOKEN_MINUS;
	else if (*p == '*')
		return p++, token = TOKEN_TIMES;
	else if (*p == '/')
		return p++, token = TOKEN_DIVIDED;

	return token = TOKEN_UNKNOWN;
}

static int parse_eat(token_t t)
{
	if (token != t)
		return 0;

	lex_next();
	return 1;
}

static int parse_peek(token_t t)
{
	return token == t;
}

static token_t parse_peek2()
{
	const char* p_stash     = p;
	const char* p_end_stash = p_end;

	token_t     token_stash        = token;
	const char* token_string_stash = token_string;
	size_t      token_length_stash = token_length;

	token_t r = lex_next();

	p = p_stash;
	p_end = p_end_stash;
	token = token_stash;
	token_string = token_string_stash;
	token_length = token_length_stash;

	return r;
}

static int parse_type()
{
	if (!parse_peek(TOKEN_INT))
		return 0;

	lex_next();
	return 1;
}

static char parse_precendence_array[] = {
	1, // TOKEN_PLUS
	1, // TOKEN_MINUS
	2, // TOKEN_TIMES
	2, // TOKEN_DIVIDED
	3, // TOKEN_ASSIGN
};

static int parse_precendence(token_t t)
{
	return parse_precendence_array[t - TOKEN_PLUS];
}

static node_type_t parse_operator_node(token_t t)
{
	return (node_type_t)(t - TOKEN_PLUS + NODE_SUM);
}

// expression_terminals <- number | id
// It is the caller's responsibility to set the parent of the node returned by expression_index.
static int parse_expression_terminals(size_t* expression_index)
{
	if (parse_peek(TOKEN_NUMBER))
	{
		ast.push_back(ast_node());
		ast.back().value = st_add(ast_st, token_string, token_length);
		ast.back().type = NODE_CONSTANT;
		*expression_index = ast.size() - 1;
		PARSE_EAT(TOKEN_NUMBER);
		return 1;
	}

	if (parse_peek(TOKEN_IDENTIFIER))
	{
		ast.push_back(ast_node());
		ast.back().value = st_add(ast_st, token_string, token_length);
		ast.back().type = NODE_VARIABLE;
		*expression_index = ast.size() - 1;
		PARSE_EAT(TOKEN_IDENTIFIER);
		return 1;
	}

	return 0;
}

extern int parse_expression(size_t expression_parent, size_t* expression_index);

// expression_operand <- "(" expression ")" | expression_terminals
// It is the caller's responsibility to set the parent of the node returned by expression_index.
static int parse_expression_operand(size_t* expression_index)
{
	/*if (parse_peek(TOKEN_MINUS))
	{
		PARSE_EAT(TOKEN_MINUS);
		return parse_expression_precedence(UNARY_MINUS_PRECEDENCE, 0, 0);
	}*/

	if (parse_peek(TOKEN_OPEN_PAREN))
	{
		Unimplemented();
		V_REQUIRE(parse_expression(0, expression_index), "expression");
		PARSE_EAT(TOKEN_CLOSE_PAREN);
		return 1;
	}

	V_REQUIRE(parse_expression_terminals(expression_index), "terminal");

	return 1;
}

// operator <- "=" | "+" | "-" | "*" | "/" | ...
static int parse_peek_operator()
{
	return parse_peek(TOKEN_ASSIGN) ||
		parse_peek(TOKEN_PLUS) ||
		parse_peek(TOKEN_TIMES) ||
		parse_peek(TOKEN_MINUS) ||
		parse_peek(TOKEN_DIVIDED);
}

// expression_precedence(p) <- operand { operator expression_precedence(op+1) }
static int parse_expression_precedence(int precedence, size_t expression_parent, size_t* expression_index)
{
	size_t left_operand_index;
	V_REQUIRE(parse_expression_operand(&left_operand_index), "expression");

	// If the operand is not part of an operation, the parent should be expression_parent.
	// Otherwise it will get overridden in the loop below.
	ast[left_operand_index].parent = expression_parent;
	*expression_index = left_operand_index;

	size_t last_operator = ~0;

	while (parse_peek_operator() && parse_precendence(token) >= precedence)
	{
		token_t op = token;
		PARSE_EAT(op);

		size_t operation_index = ast.size();
		ast_node operation;
		operation.type = parse_operator_node(op);
		operation.parent = expression_parent;
		operation.oper_left = left_operand_index;
		ast.push_back(operation);

		ast[left_operand_index].parent = operation_index;
		*expression_index = operation_index;

		if (last_operator != ~0)
			ast[last_operator].parent = operation_index;
		last_operator = operation_index;
		left_operand_index = operation_index;

		size_t right_operand_index;
		V_REQUIRE(parse_expression_precedence(parse_precendence(op) + 1, operation_index, &right_operand_index), "expression");

		ast[operation_index].oper_right = right_operand_index;
	}

	return 1;
}

// expression <- expression_precedence(0)
static int parse_expression(size_t expression_parent, size_t* expression_index)
{
	return parse_expression_precedence(0, expression_parent, expression_index);
}

// declaration <- id ":" type ["=" expression] | id ":=" expression
static int parse_declaration(size_t parent, size_t* index)
{
	if (!parse_peek(TOKEN_IDENTIFIER))
		return 0;

	if (parse_peek2() != TOKEN_DECLARE && parse_peek2() != TOKEN_DECLARE_ASSIGN)
		return 0;

	*index = ast.size();

	ast_node declaration;
	declaration.parent = parent;
	declaration.value = st_add(ast_st, token_string, token_length);
	declaration.type = NODE_DECLARATION;
	ast.push_back(declaration);

	PARSE_EAT(TOKEN_IDENTIFIER);

	if (parse_peek(TOKEN_DECLARE))
	{
		PARSE_EAT(TOKEN_DECLARE);
		ast[*index].decl_data_type = token;
		V_REQUIRE(parse_type(), "type");

		if (parse_peek(TOKEN_ASSIGN))
		{
			PARSE_EAT(TOKEN_ASSIGN);
			size_t assign_expression;
			V_REQUIRE(parse_expression(*index, &assign_expression), "expression");
			ast[*index].next_expression = assign_expression;
		}
	}
	else if (parse_peek(TOKEN_DECLARE_ASSIGN))
	{
		PARSE_EAT(TOKEN_DECLARE_ASSIGN);
		ast[*index].decl_data_type = token;

		size_t assign_expression;
		if (parse_expression(*index, &assign_expression))
			ast[*index].next_expression = assign_expression;
	}
	else
		V_ERROR("Invalid declaration");

	return 1;
}

// return_statement <- 'return' expression
static int parse_return_statement(size_t parent, size_t* index)
{
	if (!parse_peek(TOKEN_RETURN))
		return 0;

	PARSE_EAT(TOKEN_RETURN);

	*index = ast.size();
	ast_node statement;
	statement.parent = parent;
	statement.type = NODE_RETURN;
	ast.push_back(statement);

	size_t expression_id;

	int r = parse_expression(*index, &expression_id);

	ast[*index].next_expression = expression_id;

	return r;
}

// statement <- declaration ";" | expression ";" | return_statement ";"
static int parse_statement(size_t parent, size_t* index)
{
	if (parse_peek(TOKEN_IDENTIFIER))
	{
		if (parse_declaration(parent, index))
		{
			PARSE_EAT(TOKEN_SEMICOLON);
			return 1;
		}
		else
		{
			V_REQUIRE(parse_expression(parent, index), "expression");
			PARSE_EAT(TOKEN_SEMICOLON);
			return 1;
		}
	}

	if (parse_return_statement(parent, index))
	{
		PARSE_EAT(TOKEN_SEMICOLON);
		return 1;
	}

	return 0;
}

// procedure_declaration <- "(" [declaration {, declaration}] ")" "{" {statement} "}"
static int parse_procedure_declaration(st_string identifier)
{
	size_t procedure_id = ast.size();

	ast_globals.push_back(procedure_id);

	ast.push_back(ast_node());
	ast.back().type = NODE_PROCEDURE;
	ast.back().value = identifier;

	ast.back().proc_first_parameter = ~0;
	ast.back().next_statement = ~0;

	if (strcmp(st_get(ast_st, ast.back().value), "main") == 0)
		ast_main = procedure_id;

	PARSE_EAT(TOKEN_OPEN_PAREN);
	{
		size_t declaration_id;
		if (parse_declaration(procedure_id, &declaration_id))
		{
			ast[procedure_id].proc_first_parameter = declaration_id;
			size_t last_parameter = declaration_id;

			while (parse_peek(TOKEN_COMMA))
			{
				PARSE_EAT(TOKEN_COMMA);
				V_REQUIRE(parse_declaration(procedure_id, &declaration_id), "declaration");
				ast[last_parameter].next_statement = declaration_id;
				last_parameter = declaration_id;
			}
		}
	}
	PARSE_EAT(TOKEN_CLOSE_PAREN);

	PARSE_EAT(TOKEN_OPEN_CURLY);
	{
		size_t last_statement = procedure_id;
		size_t statement_id;
		while (parse_statement(procedure_id, &statement_id))
		{
			ast[last_statement].next_statement = statement_id;
			last_statement = statement_id;
		}
	}
	PARSE_EAT(TOKEN_CLOSE_CURLY);

	return 1;
}

/*
	global <- ";"
		| id ":=" procedure_declaration [";"]
		| id ":" type "=" ? ";"          // Not done yet.
*/
static int parse_global()
{
	if (parse_peek(TOKEN_SEMICOLON))
	{
		PARSE_EAT(TOKEN_SEMICOLON);
		return 1;
	}

	st_string identifier = st_add(ast_st, token_string, token_length);

	PARSE_EAT(TOKEN_IDENTIFIER);

	if (parse_peek(TOKEN_DECLARE))
	{
		Unimplemented();
	}
	else if (parse_peek(TOKEN_DECLARE_ASSIGN))
	{
		PARSE_EAT(TOKEN_DECLARE_ASSIGN);

		if (parse_peek(TOKEN_OPEN_PAREN))
		{
			V_REQUIRE(parse_procedure_declaration(identifier), "procedure");

			if (parse_peek(TOKEN_SEMICOLON))
				PARSE_EAT(TOKEN_SEMICOLON);

			return 1;
		}
	}
	else
		V_ERROR("Invalid declaration");

	return 1;
}

int parse_begin(const char* file_contents, size_t file_size)
{
	ast.clear();
	ast_st.clear();
	ast_globals.clear();
	ast_main = ~0;

	p = file_contents;
	p_end = file_contents + file_size;

	lex_next(); // Prime the pump
	while (token != TOKEN_EOF)
	{
		V_REQUIRE(parse_global(), "global statement");
	}

	return 1;
}


