#include <stdio.h>

#include "rle.h"
#include "rle_decompress.h"

int decompress(FILE *fin, FILE *fout)
{
	size_t readed = 0;
	size_t r;
	char buf[BUFFER_SIZE];
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

