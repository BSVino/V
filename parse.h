#pragma once

#include <vector>

#include "v.h"
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

	// Operator tokens should be updated in parse_precendence_array and parse_operator_node if they are modified.
	TOKEN_PLUS,             // +
	TOKEN_MINUS,            // -
	TOKEN_TIMES,            // *
	TOKEN_DIVIDED,          // /
	TOKEN_ASSIGN,           // =
} token_t;

typedef enum {
	NODE_NONE = 0,
	NODE_PROCEDURE,
	NODE_DECLARATION,
	NODE_RETURN,
	NODE_VARIABLE,
	NODE_CONSTANT,
	NODE_PROCEDURE_CALL,

	// These nodes should be updated in parse_operator_node if they are modified.
	NODE_SUM,
	NODE_DIFFERENCE,
	NODE_PRODUCT,
	NODE_QUOTIENT,
	NODE_ASSIGN,
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
		decl_next_parameter = ~0;
	}

	size_t      parent;
	node_type_t type;
	st_string   value;
	size_t      next_statement;
	size_t      next_expression;

	union {
		struct { // Procedures
			size_t proc_first_parameter;
		};
		struct { // Declarations
			token_t decl_data_type;
			size_t  decl_next_parameter;
		};
		struct { // Operations
			size_t oper_left;
			size_t oper_right;
		};
	};
};

#define PARSE_EAT(x) \
do { \
	int r = parse_eat(x); \
	if (!r) V_ERROR("ERROR: Expected " #x ".\n"); \
} while (0) \

int parse_begin(const char* file_contents, size_t file_size);

extern std::vector<ast_node> ast;
extern std::vector<char> ast_st;
extern std::vector<size_t> ast_globals;
extern size_t ast_main;

