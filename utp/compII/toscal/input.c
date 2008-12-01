/**
 *   ooooooooooooo oooo                  
 *   8'   888   `8 `888                  
 *        888       888 .oo.    .ooooo.  
 *        888       888P"Y88b  d88' `88b 
 *        888       888   888  888ooo888 
 *        888       888   888  888    .o 
 *       o888o     o888o o888o `Y8bod8P' 
 *                                       
 *                                       
 *                                       
 *   ooooo                                        .   
 *   `888'                                      .o8   
 *    888  ooo. .oo.   oo.ooooo.  oooo  oooo  .o888oo 
 *    888  `888P"Y88b   888' `88b `888  `888    888   
 *    888   888   888   888   888  888   888    888   
 *    888   888   888   888   888  888   888    888 . 
 *   o888o o888o o888o  888bod8P'  `V88V"V8P'   "888" 
 *                      888                           
 *                     o888o                          
*/
#include "input.h"

int input_next(struct input_state *is)
{
	int ch;

	is->last = is->current;
	is->first = 0;
	
	ch = fgetc(is->stream);
	if (ch == EOF) {
		if (feof(is->stream))
			ch = -INPUT_EOF;
		else
			return -INPUT_ERROR;
	}

	is->current = ch;

	if (ch == '\n') {
		is->lineno++;
		is->last_linepos = is->linepos;
		is->linepos = 0;
	}
	else
		is->linepos++;

	return ch;
}

int input_step_back(struct input_state *is)
{
	if (ungetc(is->current, is->stream) == EOF)
		return 0;
	if (is->current == '\n') {
		is->lineno--;
		is->linepos = 0;
	}
	else
		is->linepos--;
	
	return 1;
}

void input_dump_position(struct input_state *is, FILE *stream)
{
	size_t pos;
	size_t line;

	/* when stepping back from first char of line, the line number
	 * should be ok */
	if (is->linepos == 0) {
		pos = is->last_linepos;
		line = is->lineno - 1;
	}
	else {
		pos = is->linepos;
		line = is->lineno;
	}
	fprintf(stream, "line %u position %u", line, pos);
}

struct input_state *init_input_state(FILE *stream)
{
	struct input_state *is;

	is = (struct input_state*) malloc(sizeof(struct input_state));
	if (!is)
		return NULL;
	is->stream = stream;
	is->lineno = 1;
	is->linepos = 0;
	is->last_linepos = 1; /* 1 in the case of the first char of the
	                       *  first line */
	is->first = 1;
	is->current = 0;
	is->last = 0;

	return is;
}

void close_input_state(struct input_state *is)
{
	free(is);
}

