/**
 *   dMMMMMMP dMP dMP dMMMMMP 
 *     dMP   dMP dMP dMP      
 *    dMP   dMMMMMP dMMMP     
 *   dMP   dMP dMP dMP        
 *  dMP   dMP dMP dMMMMMP     
 *                            
 *   dMMMMMMP .aMMMb  dMP dMP dMMMMMP dMMMMb  dMP dMMMMMP dMMMMMP dMMMMb 
 *     dMP   dMP"dMP dMP.dMP dMP     dMP dMP amr   .dMP" dMP     dMP.dMP 
 *    dMP   dMP dMP dMMMMK" dMMMP   dMP dMP dMP  .dMP"  dMMMP   dMMMMK"  
 *   dMP   dMP.aMP dMP"AMF dMP     dMP dMP dMP .dMP"   dMP     dMP"AMF   
 *  dMP    VMMMP" dMP dMP dMMMMMP dMP dMP dMP dMMMMMP dMMMMMP dMP dMP    
 *                                                                    
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /* VStudio-related */

#include "input.h"
#include "tokenize.h"

int push_lexeme_ch(struct token *tok, int ch)
{
	if (tok->pending >= MAX_TOK_PENDING - 1)
		return 0;

	tok->repr[tok->pending++] = ch;

	return 1;
}

void finish_lexeme(struct token *tok)
{
	tok->repr[tok->pending] = '\0';
}

void token_error(struct token *tok, struct input_state *is, char *message)
{
	tok->type = TOK_PARSE_ERROR;
	tok->error = message ? message : "error parsing token";
}

int set_token_keyword(const char *name, struct token *tok)
{
	size_t i;

	/* FIXME use a hash tamble to compare the keywords */
	for (i = 0; i < NR_KEYWORDS; i++)
		if (strcmp(name, keywords[i].keyword) == 0) {
			tok->type = keywords[i].type;
			tok->name = keywords[i].name;
			return 1;
		}
	return 0;
}

struct token *fetch_next_token(struct input_state *is, struct token *tok)
{
	int ch;
	int state = 0;

	tok->pending = 0;

	while (1) {
		ch = input_next(is);
		if (ch < 0) {
			tok->type = TOK_READ_ERROR;
			tok->error = strerror(errno);
			goto error;
		}

		switch (state) {
		case 0:
			if (isalpha(ch) || ch == '_') {
				input_step_back(is);
				state = 1;
			}
			else if (isdigit(ch)) {
				input_step_back(is);
				state = 2;
			}
			else if (ch == '+') {
				state = 250;
			}
			else if (ch == '-') {
				state = 251;
			}
			else if (ch == '=') {
				input_step_back(is);
				state = 5;
			}
			else if (ch == '<') {
				state = 6;
			}
			else if (ch == '>') {
				state = 8;
			}
			else if (ch == ';') {
				state = 10;
			}
			else if (ch == '.') {
				state = 11;
			}
			else if (ch == ':') {
				state = 12;
			}
			else if (ch == ',') {
				state = 14;
			}
			else if (ch == '(') {
				state = 15;
			}
			else if (ch == ')') {
				state = 16;
			}
			else if (ch == '[') {
				state = 19;
			}
			else if (ch == ']') {
				state = 20;
			}
			else if (ch == '{') {
				state = 21;
			}
			else if (ch == '}') {
				state = 22;
			}
			else if (ch == '*') {
				state = 23;
			}
			else if (ch == '\'') {
                                state = 24;
			}

			/* special case here: we want EOF to be seen as
			 * another character, so that it makes easier to
			 * handle the "outro" thing
			 */
			else if (!isspace(ch) && ch != -INPUT_EOF) {
				token_error(tok, is, NULL);
				goto error;
			}
			break;

		case 1: /* identifier */
			if (!(isalpha(ch) || isdigit(ch) || ch == '_')) {
				input_step_back(is);
				finish_lexeme(tok);
				if (!set_token_keyword(tok->repr, tok))
					TOK_SET(tok, TOK_IDENTIFIER);
				goto done;
			}
			else if (!push_lexeme_ch(tok, ch))
				goto error_lexeme_size;
			break;

		case 2: /* integer */
			if (isdigit(ch))
				push_lexeme_ch(tok, ch);
			else if (ch == '.') {
				if (!push_lexeme_ch(tok, ch))
					goto error_lexeme_size;
				state = 3;
			}
			else {
				input_step_back(is);
				TOK_SET(tok, TOK_INTEGER);
				finish_lexeme(tok);
				/* FIXME handle (possible?) parsing errors
				*/
				tok->token.integer = atoi(tok->repr);
				goto done;
			}
			break;

		case 250:
			input_step_back(is);
			TOK_SET(tok, TOK_PLUS);
			tok->ch = '+';
			goto done;

		case 251:
			input_step_back(is);
			TOK_SET(tok, TOK_MINUS);
			tok->ch = '-';
			goto done;

		case 3: /* real or something */
			if (isdigit(ch)) {
				input_step_back(is);
				state = 4;
			}
			else {
				/* weird: most of the * programming
				 * languages allow using just * "1." to
				 * real numbers, ".1" as well.
				 */
				token_error(tok, is, NULL);
				goto error;
			}
			break;

		case 4: /* real, the last digits */
			if (isdigit(ch)) {
				if (!push_lexeme_ch(tok, ch))
					goto error_lexeme_size;
			}
			else {
				input_step_back(is);
				TOK_SET(tok, TOK_REAL);
				finish_lexeme(tok);
				tok->token.real = atof(tok->repr);
				goto done;
			}
			break;
	
		case 5: /* equal */
			TOK_SET(tok, TOK_EQUAL);
			tok->ch = ch;
			goto done;

		case 6: /* less than */
			if (ch == '=') {
				TOK_SET(tok, TOK_LESSEQTHAN);
				strcpy(tok->repr, "<=");
			}
			else if (ch == '>') {
				TOK_SET(tok, TOK_DIFFERENT);
				strcpy(tok->repr, "<>");
			}
			else {
				input_step_back(is);
				TOK_SET(tok, TOK_LESSTHAN);
				tok->ch = '<';
			}
			goto done;

	       /* the state 7 (<=) was merged into state 6 */

		case 8: /* more than */
			if (ch == '=') {
				TOK_SET(tok, TOK_GREATEREQTHAN);
				strcpy(tok->repr, ">=");
			}
			else {
				input_step_back(is);
				TOK_SET(tok, TOK_GREATERTHAN);
				tok->ch = '>';
			}
			goto done;

		/* the state 9 was merged into stat 8 */

		case 10: /* semi-colon */
			input_step_back(is);
			TOK_SET(tok, TOK_SEMICOLON);
			tok->ch = ';';
			goto done;

		case 11: /* dot */
			input_step_back(is);
			TOK_SET(tok, TOK_DOT);
			tok->ch = '.';
			goto done;

		case 12: /* colon */
			if (ch == '=') {
				TOK_SET(tok, TOK_ASSIGNMENT);
				strcpy(tok->repr, ":=");
			}
			else {
				input_step_back(is);
				TOK_SET(tok, TOK_COLON);
				tok->ch = ':';
			}
			goto done;

		/* the state 13 was merged into the state 12 */

		case 14: /* comma */
			input_step_back(is);
			TOK_SET(tok, TOK_COMMA);
			tok->ch = ',';
			goto done;

		case 15: /* left parentesis and comments */
			if (ch == '*')
				state = 151;
			else {
				input_step_back(is);
				TOK_SET(tok, TOK_LPARENTHESIS);
				tok->ch = '(';
				goto done;
			}
			break;

		case 151: /* characters of a comment, discarded */
			if (ch == '*')
				state = 152;
			break;

		case 152: /* handles the closing of a comment */
			if (ch == ')')
				/* Closed the comment, we don't generate
				 * tokens here.
				 */
				state = 0;
			else
				state = 151;
			break;

		case 16: /* right parentesis */
			input_step_back(is);
			TOK_SET(tok, TOK_RPARENTHESIS);
			tok->ch = ')';
			goto done;

		/* the states 17 and 18 were merged into 250 */

		case 19:
			input_step_back(is);
			TOK_SET(tok, TOK_OPENINGBRACKET);
			tok->ch = '[';
			goto done;

		case 20:
			input_step_back(is);
			TOK_SET(tok, TOK_CLOSINGBRACKET);
			tok->ch = ']';
			goto done;

		case 21:
			input_step_back(is);
			TOK_SET(tok, TOK_LBRACE);
			tok->ch = '{';
			goto done;

		case 22:
			input_step_back(is);
			TOK_SET(tok, TOK_RBRACE);
			tok->ch = '}';
			goto done;

		case 23:
			input_step_back(is);
			TOK_SET(tok, TOK_ASTERISK);
			tok->ch = '*';
			goto done;
			
		case 24: /* string (pascal strings use single-quotes) */

			/* FIXME: multiline strings are not portable
			 * because they can have newlines as \r\n or \n,
			 * depending on the OS the file was written.
			 *
			 * They can even result in weird behaviors because
			 * a TOK_CHAR can be seen as TOK_STRING on Windows
			 * because it will not be of length 1.
			 */
			if (ch == '\\')
				state = 25;
			else if (ch == '\'') {
				finish_lexeme(tok);
				if (tok->pending == 1)
					/* single-byte strings should be
					 * seen as 'char' (crappy!)
					 */
					TOK_SET(tok, TOK_CHAR);
				else
					TOK_SET(tok, TOK_STRING);
				goto done;
			}
			else if (!push_lexeme_ch(tok, ch))
				goto error_lexeme_size;
			break;

		case 25: /* escaped slash or single-quote */
			if (ch == '\'' || ch == '\\') {
				/* handle escaped characters \\ and \' */
				if (!push_lexeme_ch(tok, ch))
					goto error_lexeme_size;
			}
			else {
				/* if it is not a slash or quote, push it
				 * as if nothing happened */
				if (!push_lexeme_ch(tok, '\\'))
					goto error_lexeme_size;
				if (!push_lexeme_ch(tok, ch))
					goto error_lexeme_size;
			}
			state = 24;
			break;
		}

		if (ch == -INPUT_EOF) {
			if (state != 0) {
				token_error(tok, is, "unexpected end of file");
				goto error;
			}
			goto eof;
		}
	}

done:
	return tok;

error_lexeme_size:
	token_error(tok, is, "too long lexeme");
	return NULL;
eof:
	TOK_SET(tok, TOK_EOF);
error:
	return NULL;
}

void dump_token(struct token *tok, FILE *output)
{
	/* prints the tokens based on their type */
	fprintf(output, "%s: ", tok->name);

	/* Here we must dump the real value of the token depending on the
	 * type, for example to TOK_INTEGER we must call
	 * fprintf(output, "%d", tok->token.integer), and so on for each
	 * type of token.
	 *
	 * our "else" clause will always be the single-char tokens, such as
	 * + ( ) [ ], which will have the ->ch member dumped
	 */

	switch (tok->type) {

	case TOK_CHAR:
	case TOK_STRING:
	case TOK_ASSIGNMENT:
	case TOK_GREATEREQTHAN:
	case TOK_LESSEQTHAN:
	case TOK_DIFFERENT:
	case TOK_IDENTIFIER:
		fprintf(output, "%s\n", tok->repr);
		break;

	case TOK_INTEGER:
		fprintf(output, "%d\n", tok->token.integer);
		break;

	case TOK_REAL:
		fprintf(output, "%f\n", tok->token.real);
		break;

	default:
		if (tok->type > TOK_KW__FIRST && tok->type < TOK_KW__LAST)
			fprintf(output, "%s\n", tok->repr);
		else
			fprintf(output, "%c\n", tok->ch);
	}

}

void tokenizer_dump_error(struct token *tok, struct input_state *is, FILE *output)
{
	switch (tok->type) {
	case TOK_READ_ERROR:
	case TOK_PARSE_ERROR:
		fputs("on ", output);
		input_dump_position(is, output);
		fprintf(output, ": %s\n", tok->error);
	default: 
		/* just to silent gcc (bad) */
		break;
	}
}

