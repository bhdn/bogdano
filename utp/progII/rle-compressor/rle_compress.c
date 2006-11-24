#include <stdio.h>

#include "rle.h"
#include "rle_compress.h"

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
		 * in the file, it shows there's something really nasty in
		 * the code */
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