/*
 * The incredible RLE compressor!
 *
 * Bogdano Arendartchuk <debogdano@gmail.com>
 *
 * Usage:
 *
 *   $ rle <action> <infile> <outfile>
 * 
 * Where:
 * 
 * - action is -c to compress and -d to decompress
 * - infile is the infille and outfile is the outfile (!)
 */

#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE	512

static const char *error_messages[] = {
	"finished",
	"", /* invalid command line option, usage() already does it */
	"input file not found",
	"could not create output file"
};

enum actions {
	ACT_COMPRESS = 0,
	ACT_DECOMPRESS
};

enum errors {
	ERR_OK = 0,
	ERR_INVARGS,
	ERR_INOTFOUND,
	ERR_OFAILED

};


int compress(FILE *fin, FILE *fout) 
{
	size_t count;
	size_t i;
	char buf[BUFFER_SIZE];
	char wbuf[BUFFER_SIZE];
	unsigned char freq;
	char last;
	size_t wi;

	while(count = fread(buf, sizeof(buf[0]), BUFFER_SIZE, fin)) {
		last = buf[0];
		freq = 0;
		for (i = 1; i < count; i++) {
			if (buf[i] != last || freq >= (unsigned char)-1 ) {
				wbuf[wi++] = freq;
				wbuf[wi++] = last;
				if (wi >= BUFFER_SIZE) {
					fwrite(wbuf, sizeof(wbuf[0]), )
				}
				fputc(freq, fout);
				fputc(last, fout);
				freq = 0;
			}
			else
				freq++;
			last = buf[i];
		}
	}
	
}

int decompress(FILE *fin, FILE *fout)
{
	///
}


void usage(char *argv[])
{
	printf("%s <action> <infile> <outfile>\n"
	       "action -c for compressing, -d for decompressing\n",
	       argv[0]);
}

void showerr(errcode)
{
	fputs(error_messages[errcode], stderr);
}


int main(int argc, char *argv[])
{
	int errcode = 0;
	int action;
	char *errmsg;
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
		compress(fin, fout);
	else
		decompress(fin, fout);

	fclose(fin);
	fclose(fout);

	showerr(errcode);
	exit(errcode);
}
