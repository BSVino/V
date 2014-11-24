#pragma once

#include <vector>

#include "stringtable.h"

typedef enum {
	TOKEN_EOF = -1,
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
	NODE_VARIABLE,
	NODE_CONSTANT,
} node_type_t;

struct ast_node {
	ast_node()
	{
		clear();
	}

	void clear()
	{
		parent = ~0;
		type = NODE_NONE;
		value = ~0;
		next_statement = ~0;
		next_expression = ~0;
	}

	size_t      parent;
	node_type_t type;
	st_string   value;
	size_t      next_statement;
	size_t      next_expression;

	union {
		struct { // Procedures
			size_t proc_first_parameter;
			size_t proc_num_parameters;
			size_t proc_first_statement;
		};
		struct { // Declarations
			token_t decl_data_type;
		};
	};
};

#define LEX_ERROR(msg) do { printf(msg); return 0; } while (0);

#define LEX_EAT(x) \
do { \
	int r = lex_eat(x); \
	if (!r) LEX_ERROR("ERROR: Expected " #x ".\n"); \
} while (0) \

#define LEX_REQUIRE(x, error) \
do { \
	int r = (x); \
	if (!r) { \
		printf("ERROR: Required a " error ".\n"); \
		return r; \
		} \
} while (0) \

int lex_begin(const char* file_contents, size_t file_size);

extern std::vector<ast_node> ast;
extern std::vector<char> ast_st;
extern size_t ast_main;

