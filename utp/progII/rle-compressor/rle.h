#ifndef RLE_h
#define RLE_h

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

#endif /* ifdef RLE_h */