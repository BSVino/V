#include "lex.h"

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

static int lex_strcmp(const char* s1, const char* s2, size_t n1, size_t n2)
{
	return strncmp(s1, s2, min(n1, n2));
}

static token_t lex_next()
{
	if (p == p_end)
		return TOKEN_EOF;

	while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t' && p <= p_end)
		p++;

	if (p == p_end)
		return TOKEN_EOF;

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

	return TOKEN_UNKNOWN;
}

static int lex_eat(token_t t)
{
	if (token != t)
		return 0;

	lex_next();
	return 1;
}

static int lex_peek(token_t t)
{
	if (token != t)
		return 0;

	return 1;
}

static int lex_type()
{
	if (!lex_peek(TOKEN_INT))
		return 0;

	lex_next();
	return 1;
}

static int lex_expression(size_t expression_parent, size_t* expression_index)
{
	if (lex_peek(TOKEN_NUMBER))
	{
		ast.push_back(ast_node());
		ast.back().value = st_add(ast_st, token_string, token_length);
		ast.back().parent = expression_parent;
		ast.back().type = NODE_VARIABLE;
		*expression_index = ast.size() - 1;
		LEX_EAT(TOKEN_NUMBER);
		return 1;
	}

	if (lex_peek(TOKEN_IDENTIFIER))
	{
		ast.push_back(ast_node());
		ast.back().value = st_add(ast_st, token_string, token_length);
		ast.back().parent = expression_parent;
		ast.back().type = NODE_VARIABLE;
		*expression_index = ast.size() - 1;
		LEX_EAT(TOKEN_IDENTIFIER);
		return 1;
	}

	return 0;
}

static int lex_declaration(ast_node* declaration)
{
	if (!lex_peek(TOKEN_IDENTIFIER))
		return 0;

	declaration->value = st_add(ast_st, token_string, token_length);
	declaration->type = NODE_DECLARATION;
	LEX_EAT(TOKEN_IDENTIFIER);
	LEX_EAT(TOKEN_DECLARE);
	declaration->decl_data_type = token;
	LEX_REQUIRE(lex_type(), "type");

	return 1;
}

static int lex_return_statement(ast_node* statement, size_t* statement_id)
{
	if (!lex_peek(TOKEN_RETURN))
		return 0;

	LEX_EAT(TOKEN_RETURN);

	ast.push_back(*statement);
	*statement_id = ast.size() - 1;
	ast[*statement_id].type = NODE_RETURN;

	size_t expression_id;

	int r = lex_expression(ast.size() - 1, &expression_id);

	ast[*statement_id].next_expression = expression_id;

	return r;
}

static int lex_statement(ast_node* statement, size_t* statement_id)
{
	if (lex_declaration(statement))
	{
		ast.push_back(*statement);
		*statement_id = ast.size() - 1;
		LEX_EAT(TOKEN_SEMICOLON);
		return 1;
	}

	if (lex_return_statement(statement, statement_id))
	{
		LEX_EAT(TOKEN_SEMICOLON);
		return 1;
	}

	return 0;
}

static int lex_procedure()
{
	size_t procedure_id = ast.size();

	ast.push_back(ast_node());
	ast.back().type = NODE_PROCEDURE;
	ast.back().value = st_add(ast_st, token_string, token_length);

	ast.back().proc_first_parameter = ast.size();
	ast.back().proc_num_parameters = 0;
	ast.back().next_statement = ~0;

	if (strcmp(st_get(ast_st, ast.back().value), "main") == 0)
		ast_main = procedure_id;

	LEX_EAT(TOKEN_IDENTIFIER);

	LEX_EAT(TOKEN_DECLARE_ASSIGN);

	LEX_EAT(TOKEN_OPEN_PAREN);
	{
		ast_node declaration;
		declaration.parent = procedure_id;
		if (lex_declaration(&declaration))
		{
			ast.push_back(declaration);
			ast[procedure_id].proc_num_parameters++;
			while (lex_peek(TOKEN_COMMA))
			{
				LEX_EAT(TOKEN_COMMA);
				LEX_REQUIRE(lex_declaration(&declaration), "declaration");
				ast.push_back(declaration);
				ast[procedure_id].proc_num_parameters++;
				declaration.clear();
			}
		}
	}
	LEX_EAT(TOKEN_CLOSE_PAREN);

	LEX_EAT(TOKEN_OPEN_CURLY);
	{
		ast_node statement;
		size_t last_statement = procedure_id;
		size_t statement_id;
		statement.parent = procedure_id;
		while (lex_statement(&statement, &statement_id))
		{
			ast[last_statement].next_statement = statement_id;
			last_statement = statement_id;
			statement.clear();
			statement.parent = procedure_id;
		}
	}
	LEX_EAT(TOKEN_CLOSE_CURLY);

	return 1;
}

int lex_begin(const char* file_contents, size_t file_size)
{
	ast.clear();
	ast_st.clear();
	ast_main = ~0;

	p = file_contents;
	p_end = file_contents + file_size;

	lex_next(); // Prime the pump
	if (!lex_procedure())
		LEX_ERROR("Expected a procedure.\n");

	return 1;
}

