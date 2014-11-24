#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <algorithm>

#include "stringtable.h"

using namespace std;

typedef enum {
	TOKEN_UNKNOWN = 0,
	TOKEN_IDENTIFIER = 128, // Start at 128 to leave room for ascii
	TOKEN_NUMBER,           // e.g. 123, 2.4, 0
	TOKEN_DECLARE_ASSIGN,   // :=
	TOKEN_DECLARE,          // :
	TOKEN_OPEN_PAREN,       // (
	TOKEN_CLOSE_PAREN,      // )
	TOKEN_OPEN_CURLY,       // {
	TOKEN_CLOSE_CURLY,      // }
	TOKEN_COMMA,            // ,
	TOKEN_SEMICOLON,        // ;
	TOKEN_INT,              // int type keyword
	TOKEN_RETURN,           // return keyword
} token_t;

typedef enum {
	NODE_NONE = 0,
	NODE_PROCEDURE,
	NODE_DECLARATION,
	NODE_RETURN,
} node_type_t;

static const char* p;

static token_t     token;
static const char* token_string;
static size_t      token_length;

struct ast_node {
	ast_node()
	{
		parent = ~0;
		type = NODE_NONE;
		name = ~0;
		next_statement = ~0;
	}

	size_t      parent;
	node_type_t type;
	st_string   name;
	size_t      next_statement;

	union {
		struct { // Procedures
			size_t first_parameter;
			size_t num_parameters;
			size_t first_statement;
		};
		struct { // Declarations
			token_t data_type;
		};
	};
};

static vector<ast_node> ast;
static vector<char> ast_st;

#define LEX_EAT(x) \
do { \
	int r = lex_eat(x); \
	if (!r) { \
		printf("ERROR: Expected " #x ".\n"); \
		return r; \
	} \
} while (0) \

#define LEX_REQUIRE(x, error) \
do { \
	int r = (x); \
	if (!r) { \
		printf("ERROR: Required a " error ".\n"); \
		return r; \
	} \
} while (0) \

static int lex_strcmp(const char* s1, const char* s2, size_t n1, size_t n2)
{
	return strncmp(s1, s2, min(n1, n2));
}

static token_t lex_next()
{
	while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
		p++;

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

static int lex_expression()
{
	if (lex_peek(TOKEN_IDENTIFIER))
	{
		LEX_EAT(TOKEN_IDENTIFIER);
		return 1;
	}

	if (lex_peek(TOKEN_NUMBER))
	{
		LEX_EAT(TOKEN_NUMBER);
		return 1;
	}

	return 0;
}

static int lex_declaration(ast_node* declaration)
{
	if (!lex_peek(TOKEN_IDENTIFIER))
		return 0;

	declaration->name = st_add(ast_st, token_string, token_length);
	declaration->type = NODE_DECLARATION;
	LEX_EAT(TOKEN_IDENTIFIER);
	LEX_EAT(TOKEN_DECLARE);
	declaration->data_type = token;
	LEX_REQUIRE(lex_type(), "type");

	return 1;
}

static int lex_return_statement(ast_node* statement)
{
	if (!lex_peek(TOKEN_RETURN))
		return 0;

	LEX_EAT(TOKEN_RETURN);

	return lex_expression();
}

static int lex_statement(ast_node* statement)
{
	if (lex_declaration(statement))
	{
		ast.push_back(*statement);
		LEX_EAT(TOKEN_SEMICOLON);
		return 1;
	}

	if (lex_return_statement(statement))
	{
		ast.push_back(*statement);
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
	ast.back().name = st_add(ast_st, token_string, token_length);

	ast.back().first_parameter = ast.size();
	ast.back().num_parameters = 0;
	ast.back().next_statement = ~0;

	LEX_EAT(TOKEN_IDENTIFIER);

	LEX_EAT(TOKEN_DECLARE_ASSIGN);

	LEX_EAT(TOKEN_OPEN_PAREN);
	{
		ast_node declaration;
		declaration.parent = procedure_id;
		if (lex_declaration(&declaration))
		{
			ast.push_back(declaration);
			ast[procedure_id].num_parameters++;
			while (lex_peek(TOKEN_COMMA))
			{
				LEX_EAT(TOKEN_COMMA);
				LEX_REQUIRE(lex_declaration(&declaration), "declaration");
				ast.push_back(declaration);
				ast[procedure_id].num_parameters++;
			}
		}
	}
	LEX_EAT(TOKEN_CLOSE_PAREN);

	LEX_EAT(TOKEN_OPEN_CURLY);
	{
		ast_node statement;
		size_t last_statement = procedure_id;
		statement.parent = procedure_id;
		size_t statement_id = ast.size();
		while (lex_statement(&statement))
		{
			ast[last_statement].next_statement = statement_id;
			last_statement = statement_id;
		}
	}
	LEX_EAT(TOKEN_CLOSE_CURLY);

	return 1;
}

void compile(const char* filename, int** program, int** data)
{
	FILE* fp = fopen(filename, "rb");

	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	rewind(fp);

	char* file_contents = (char*)malloc(file_size+1);

	int read = fread(file_contents, 1, file_size, fp);
	file_contents[file_size] = 0;

	fclose(fp);

	p = file_contents;

	lex_next(); // Prime the pump
	lex_procedure();

	free(file_contents);
}
