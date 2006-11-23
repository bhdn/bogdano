/* $Id$
 * $HeadURL$
 *
 * The incredible RLE compressor!
 *
 * Bogdano Arendartchuk <debogdano@gmail.com>
 *
 *
 * Usage:
 *
 *   $ rle <action> <infile> <outfile>
 * 
 * Where:
 * 
 * - action is -c to compress and -d to decompress
 * - infile is the infille and outfile is the outfile (!)
 * 
 */

#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE	512
#define WBUFFER_SIZE	(BUFFER_SIZE*2)

static const char *error_messages[] = {
	"", /* finished */
	"", /* invalid command line option, usage() already does it */
	"input file not found",
	"could not create output file",
	"invalid file format",
	"writing error"
};

enum actions {
	ACT_COMPRESS = 0,
	ACT_DECOMPRESS
};

enum errors {
	ERR_OK = 0,
	ERR_INVARGS,
	ERR_INOTFOUND,
	ERR_OFAILED,
	ERR_INVFFORMAT,
	ERR_WRITE
};

int compress(FILE *fin, FILE *fout) 
{
	size_t count;
	size_t i;
	char buf[BUFFER_SIZE];
	char wbuf[WBUFFER_SIZE];
	unsigned char freq;
	char last;
	size_t wi;

	/* when freq = 0, last is unset */
	freq = 0;
	wi = 0;
	while((count = fread(buf, sizeof(buf[0]), BUFFER_SIZE, fin))) {
		for (i = 0; i < count; i++) {
			if (freq > 0 && (buf[i] != last ||
			                 freq >= (unsigned char)-1)) {
				
				wbuf[wi++] = freq;
				wbuf[wi++] = last;
				freq = 0;
			}
			freq++;
			last = buf[i];
		}
		/* argh! we must handle when we're counting the last byte
		 * in the file */
		if (count != BUFFER_SIZE) {
			wbuf[wi++] = freq;
			wbuf[wi++] = last;
		}
		if (fwrite(wbuf, sizeof(wbuf[0]), wi, fout) != wi)
			return ERR_WRITE;
		wi = 0; /* reset write buffer */
	}

	return ERR_OK;
}

int decompress(FILE *fin, FILE *fout)
{
	size_t readed = 0;
	size_t written = 0;
	size_t r;
	char buf[BUFFER_SIZE];
	char wbuf[WBUFFER_SIZE];
	unsigned char cnt, k; 
	char ch;

	while ((readed = fread(buf, sizeof(buf[0]), BUFFER_SIZE, fin))) {
		if ((readed % 2) != 0) {
			return ERR_INVFFORMAT;
		}
		for (r = 0; r < readed; r += 2) {
			cnt = buf[r];
			ch = buf[r+1];
			for (k = 0; k < cnt; k++) {
				/* aargh! */
				if(fputc(ch, fout) != ch)
					return ERR_WRITE;
			}
		}
	}

	return ERR_OK;
}


void usage(char *argv[])
{
	printf("%s <action> <infile> <outfile>\n"
	       "action -c for compressing, -d for decompressing\n",
	       argv[0]);
}

void showerr(errcode)
{
	if (errcode != ERR_OK) {
		perror(error_messages[errcode]);
	}
}


int main(int argc, char *argv[])
{
	int errcode = 0;
	int action;
	FILE *fin;
	FILE *fout;

	if (argc != 4 || strlen(argv[1]) != 2 || argv[1][0] != '-') {
		usage(argv);
		exit(ERR_INVARGS);
	}
	if (argv[1][1] == 'c') 
		action = ACT_COMPRESS;
	else if (argv[1][1] == 'd') 
		action = ACT_DECOMPRESS;
	else {
		usage(argv);
		exit(ERR_INVARGS);
	}

	fin = fopen(argv[2], "rb");
	if (!fin) {
		showerr(ERR_INOTFOUND);
		exit(ERR_INOTFOUND);
	}

	fout = fopen(argv[3], "wb+");
	if (!fout) {
		showerr(ERR_OFAILED);
		fclose(fin);
		exit(ERR_OFAILED);
	}


	if (action == ACT_COMPRESS)
		errcode = compress(fin, fout);
	else
		errcode = decompress(fin, fout);

	fclose(fin);
	fclose(fout);

	showerr(errcode);
	exit(errcode);
}
