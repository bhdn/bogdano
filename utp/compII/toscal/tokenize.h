# ifndef inc_tokenize_h
#define inc_tokenize_h

#include <stdio.h>

#include "input.h"

#define TOK_SET(k, tokname) do { \
	k->type = tokname; \
	k->name = #tokname; } while(0)

enum token_t {                
	TOK_IDENTIFIER = 0,
	TOK_INTEGER,
	TOK_REAL,
	TOK_SEMICOLON,
	TOK_CHAR,
	TOK_STRING,
	TOK_DOT,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_OPENINGBRACKET,
	TOK_CLOSINGBRACKET,
	TOK_LPARENTHESIS,
	TOK_RPARENTHESIS,
	TOK_ASSIGNMENT,
	TOK_EQUAL,
	TOK_LESSTHAN,
	TOK_LESSEQTHAN,
	TOK_DIFFERENT,
	TOK_GREATERTHAN,
	TOK_GREATEREQTHAN,
	TOK_PLUS,
	TOK_MINUS,
	TOK_COMMA,
	TOK_COLON,  
	TOK_ASTERISK,
	TOK_UNDERLINE,
	TOK_DOUBLEQUOTATIONMARK,
	TOK_SINGLEQUOTATIONMARK,
	TOK_OTHERTHAN,
	TOK_KW__FIRST,
	TOK_KW_PROGRAM,
	TOK_KW_VAR,
	TOK_KW_BEGIN,
	TOK_KW_END,
	TOK_KW_OR,
	TOK_KW_AND,
	TOK_KW_NOT,
	TOK_KW_DIV,
	TOK_KW_LABEL,
	TOK_KW_CONST,
	TOK_KW_IF,
	TOK_KW_THEN,
	TOK_KW_ELSE,
	TOK_KW_WHILE,
	TOK_KW_DO,
	TOK_KW_REPEAT,
	TOK_KW_UNTIL,
	TOK_KW_GOTO,
	TOK_KW_PROCEDURE,
	TOK_KW_FUNCTION,
	TOK_KW_READ,
	TOK_KW_WRITE,
	TOK_KW_MOD,
	TOK_KW__LAST,


	/* special types */
	TOK_EOF,
	TOK_READ_ERROR,
	TOK_PARSE_ERROR
};

#define KEYWORD(value,name) { value, name, #name }

struct {
	char *keyword;
	enum token_t type;
	char *name;
} static const keywords[] = {
	KEYWORD("program", 	TOK_KW_PROGRAM),
	KEYWORD("var",		TOK_KW_VAR),
	KEYWORD("begin",	TOK_KW_BEGIN),
	KEYWORD("end",	 	TOK_KW_END),
	KEYWORD("or",  		TOK_KW_OR),
	KEYWORD("and", 		TOK_KW_AND),
	KEYWORD("not", 		TOK_KW_NOT),
	KEYWORD("div", 		TOK_KW_DIV),
	KEYWORD("label", 	TOK_KW_LABEL),
	KEYWORD("const", 	TOK_KW_CONST),
	KEYWORD("if",  		TOK_KW_IF),
	KEYWORD("then", 	TOK_KW_THEN),
	KEYWORD("else", 	TOK_KW_ELSE),
	KEYWORD("while", 	TOK_KW_WHILE),
	KEYWORD("do",  		TOK_KW_DO),
	KEYWORD("repeat", 	TOK_KW_REPEAT),
	KEYWORD("until", 	TOK_KW_UNTIL),
	KEYWORD("goto", 	TOK_KW_GOTO),
	KEYWORD("procedure", 	TOK_KW_PROCEDURE),
	KEYWORD("function", 	TOK_KW_FUNCTION),
	KEYWORD("read",		TOK_KW_READ),
	KEYWORD("write",	TOK_KW_WRITE),
	KEYWORD("mod",		TOK_KW_MOD)
};

#define NR_KEYWORDS	(sizeof(keywords)/sizeof(keywords[0]))

#define MAX_TOK_PENDING	256
#define MAX_IDENTIFIER	MAX_TOK_PENDING

struct token {
	enum token_t type;
	char *name;
	int ch;
	char repr[MAX_IDENTIFIER];
	size_t pending;
	union {
		int integer;
		float real;
	} token;
	char *error;
};

struct token *fetch_next_token(struct input_state *is, struct token *tok);
void dump_token(struct token *tok, FILE *output);
void tokenizer_dump_error(struct token *tok, struct input_state *is, FILE *output);

#endif /* ifndef inc_tokenize_h */
