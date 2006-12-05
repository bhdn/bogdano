/* $Id$
 * $HeadURL$
 *
 * The incredible (and buggy) RLE compressor!
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

#include "rle.h"
#include "rle_compress.h"
#include "rle_decompress.h"

static const char *error_messages[] = {
	"", /* finished */
	"", /* invalid command line option, usage() already does it */
	"failed to open input file",
	"could not create output file",
	"invalid file format",
	"writing error"
};

void usage(char *argv[])
{
	printf("%s <action> <infile> <outfile>\n"
	       "action -c for compressing, -d for decompressing\n",
	       argv[0]);
}

void showerr(int errcode)
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
		return ERR_INVARGS;
	}
	if (argv[1][1] == 'c') 
		action = ACT_COMPRESS;
	else if (argv[1][1] == 'd') 
		action = ACT_DECOMPRESS;
	else {
		usage(argv);
		return ERR_INVARGS;
	}

	fin = fopen(argv[2], "rb");
	if (!fin) {
		showerr(ERR_IFAILED);
		return ERR_IFAILED;
	}

	fout = fopen(argv[3], "wb+");
	if (!fout) {
		showerr(ERR_OFAILED);
		fclose(fin);
		return ERR_OFAILED;
	}


	if (action == ACT_COMPRESS)
		errcode = compress(fin, fout);
	else
		errcode = decompress(fin, fout);

	fclose(fin);
	fclose(fout);

	showerr(errcode);
	return errcode;
}
