/**
 * A crappy C implementation to find the information gain inside a CSV
 * table.
 *
 * Advantages:
 *
 * - None known.
 *
 * Problems:
 *
 * - If the number of classes gets too high, there will be a huge memory
 *   waste in the hash tables of each attribute class;
 *
 * - Conversely, if the number of attributes is too high, it will be too
 *   slow to add new entries in the refmap (hm, actually this is not a big
 *   issue, as we are limited by IO).
 *
 * Bogdano Arendartchuk <debogdano@gmail.com>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#undef DEBUG

#include "hash.h"

#define LINE_BUFFER_SIZE	(1024*1024)
#define HASH_SIZE_ATTRIBUTES	1024
#define HASH_SIZE_CLASSES	64


/* classes don't need to have their names stored, just their "uniqueness" 
 * is enough */
struct class_entry {
	unsigned long count; /* FIXME correct type */
	struct hash_table *refmap; /* counts in which classes of the ref.
				    * attribute it appeared */
};

struct attribute_gain {
	size_t attribute;
	double gain;
};

struct table_stats {
	unsigned int lines;
	struct hash_table **attributes;
	size_t nr_attributes;
	struct hash_table *refclasses;
	size_t refattr;
};

struct class_entry *new_class_entry()
{
	struct class_entry *cl;

	cl = (struct class_entry*) malloc(sizeof(struct class_entry));
	if (!cl)
		return NULL;
	cl->count = 0;
	cl->refmap = hash_init(HASH_SIZE_CLASSES);
	if (!cl->refmap) {
		free(cl);
		return NULL;
	}

	return cl;
}

void free_class_entry(struct class_entry *cl)
{
	hash_free(cl->refmap);
	free(cl);
}

void free_table_stats(struct table_stats *ts)
{
	size_t i;
	hash_iter_t hi;
	struct class_entry *ce;

	if (!ts)
		return;
	if (ts->refclasses)
		hash_free(ts->refclasses);
	if (ts->attributes)
		for (i = 0; i < ts->nr_attributes; i++) {
			for_each_hash_value(ts->attributes[i], hi, ce) {
				free_class_entry(ce);
			}
			hash_free(ts->attributes[i]);
		}
	free(ts->attributes);
	free(ts);
}

struct table_stats *new_table_stats(size_t nr_attributes)
{
	struct table_stats *ts;
	size_t i;

	ts = (struct table_stats*) malloc(sizeof(struct table_stats));
	if (!ts)
		return NULL;

	ts->refclasses = hash_init(HASH_SIZE_ATTRIBUTES);
	if (!ts->refclasses)
		goto failed;

	ts->attributes = (struct hash_table**)
		malloc(sizeof(struct hash_table*) * nr_attributes);
	if (!ts->attributes)
		goto failed;
	for (i = 0; i < nr_attributes; i++) {
		ts->attributes[i] = hash_init(HASH_SIZE_ATTRIBUTES);
		if (!ts->attributes[i]) {
			for (; i >= 0; i--)
				hash_free(ts->attributes[i]);
			free(ts->attributes);
			goto failed;
		}
	}

	ts->nr_attributes = nr_attributes;
	ts->lines = 0;
	ts->refattr = 0;

	return ts;
failed:
	free_table_stats(ts);
	return NULL;
}


#ifdef DEBUG

void dump_table_stats(struct table_stats *ts)
{
	size_t i, j, k;
	struct class_entry *ce;

	/* FIXME there may be more entries in a hash entry! */

	printf("lines: %d\n", ts->lines);
	for (i = 0; i < ts->nr_attributes; i++) {
		printf("attribute %d:\n", i);
		for (j = 0; j < ts->attributes[i]->size; j++) {
			if (!ts->attributes[i]->entries[j])
				continue; /* empty pigeonhole */
			printf("  class %s:\n",
     			       ts->attributes[i]->entries[j]->key);
			ce = (struct class_entry*)
				ts->attributes[i]->entries[j]->data;
			for (k = 0; k < ce->refmap->size; k++) {
				if (!ce->refmap->entries[k])
					continue; /* empty pigeonhole */
				printf("        refmap %s ->\t%d\n",
				       ce->refmap->entries[k]->key,
				       (size_t) ce->refmap->entries[k]->data);
			}

		}

		}

	printf("classes: %u\n", ts->refclasses->size);
	for (i = 0; i < ts->refclasses->size; i++) {
		if (!ts->refclasses->entries[i])
			continue;
		printf("class: %s, count: %u\n",
		       ts->refclasses->entries[i]->key,
		       (unsigned int) ts->refclasses->entries[i]->data);
	}

}
#endif

struct table_stats *parse_line(struct table_stats *ts, char *line,
		size_t line_size, size_t lineno)
{
	char *refclass;
	char *token;
	size_t refsize;
	size_t ia;
	size_t nrefclasses;
	size_t size;
	unsigned int refcount;
	unsigned int refhash;
	struct class_entry *ce;
	struct hash_table *classes;

	/* first of all obtain the hash of the reference
	 * attribute that is used in this line, this way we
	 * can store the stats for each attribute class in
	 * a simple loop around strtok_r */
	refclass = strrchr(line, ',');
	if (!refclass) {
		/* FIXME we can't handle files with only one column */
		fprintf(stderr, "ouch, missing comma in line: "
				"%s\n", line);
		errno = EINVAL;
		goto failed;
	}
	++refclass; /* skip the comma */

	refsize = strlen(refclass);
	refhash = get_hash(refclass, refsize);

	refcount  = (unsigned int)
		hash_get(ts->refclasses, refclass, refsize, refhash);
	hash_put(ts->refclasses, refclass, refsize,
	         (void*)(refcount + 1), refhash);

	/* now we can iterate over the classes in this line
	 * and update their stats */
	for (ia = 0; token = strtok(line, ",");
	     ia++, line = NULL) {
		if (ia == ts->refattr)
			continue;
		if (ia >= ts->nr_attributes) {
			fprintf(stderr, "format error: "
					"the number of columns of the "
					"line %u doesn't match the first "
					"line count (%u columns)\n",
					lineno, ts->nr_attributes);
			errno  = EINVAL;
			goto failed;
		}

		classes = ts->attributes[ia];
		size = strlen(token);
		ce = (struct class_entry*)
		        hash_get(classes, token, size, 0);
		if (!ce) {
			ce = new_class_entry();
			if(!ce)
				goto failed;
			if(!hash_put(classes, token, size, (void*)ce, 0))
				goto failed;
		}

		ce->count++;

		/* increase the count for this class in
		 * this refclass */
		nrefclasses = (size_t) hash_get(ce->refmap, refclass,
		                                refsize, refhash);
		hash_put(ce->refmap, refclass, refsize,
		         (void*) (nrefclasses + 1), refhash);
	}
	return ts;
failed:
	return NULL;
}

void strip_newline(char *str)
{
	for (;*str; str++)
		if (*str == '\n' || *str == '\r')
			*str = '\0';
}

struct table_stats *collect_stats(FILE *stream)
{
	char line[LINE_BUFFER_SIZE];
	char *lineptr; /* used by strsep */
	unsigned int refhash;
	size_t nr_attributes;
	struct table_stats *ts = NULL;

	nr_attributes = 0;
	while (fgets(line, sizeof(line)-1, stream)) {
		lineptr = line;

		strip_newline(line);

		if (nr_attributes == 0) {
			/* count how many attributes we have and initialize
			 * the needed structures */
			while (*lineptr++)
				if (*lineptr == ',')
					nr_attributes++;
			nr_attributes++;

			ts = new_table_stats(nr_attributes);
			if (!ts)
				goto failed;

			/* It defines which is the attribute we will use as
			 * the reference for this table. Here we could ask
			 * the user which is the attribute he wants to use.
			 */
			ts->refattr = nr_attributes - 1;

			fseek(stream, 0L, SEEK_SET);
		}
		else {
			ts->lines++;
			if (!parse_line(ts, line, sizeof(line)-1, ts->lines))
				goto failed;
		}
	}
	if (!feof(stream))
		goto failed;

	return ts;
failed:
	free_table_stats(ts);
	return NULL;

}

struct attribute_gain *get_gain(struct table_stats *ts, size_t *count)
{
	size_t i, nrefclasses;
	unsigned int refcount;
	double p;
	double attrentropy, refentropy, attrgain;
	hash_iter_t hi, rmhi;
	struct class_entry *ce;

	struct attribute_gain *allgains;

	/* obtain the entropy of the reference attribute */
	refentropy = 0.0;
	for_each_hash_value(ts->refclasses, hi, refcount) {
		p  = (double) refcount / (double) ts->lines;
		refentropy += -p * log2(p);
	}
#ifdef DEBUG
	printf("refentropy: %lf\n", refentropy);
#endif

	/* allocate structures to put the gain information */
	allgains = (struct attribute_gain*)
		malloc(sizeof(struct attribute_gain) * ts->nr_attributes);
	if (!allgains)
		goto failed;
	*count = ts->nr_attributes;

	/* for each attribute found: */
	for (i = 0; i < ts->nr_attributes; i++) {

		attrgain = refentropy;

		/* for each class of the attribute: */
		for_each_hash_value(ts->attributes[i], hi, ce) {
			attrentropy = 0.0;

			/* for each class of the reference attribute
			 * referenced along with the current class: */
			for_each_hash_value(ce->refmap, rmhi, nrefclasses) {
				p = (double) nrefclasses / (double) ce->count;
				attrentropy += -p * log2(p);
			}
			p = (double) ce->count / (double) ts->lines;
			attrgain += -p * attrentropy;
		}

		allgains[i].attribute = i;
		allgains[i].gain = attrgain;
	}

	return allgains;
failed:
	return NULL;
}

int attribute_gain_cmp(const void *ptr1, const void *ptr2)
{
	struct attribute_gain *ag1, *ag2;

	ag1 = (struct attribute_gain*) ptr1;
	ag2 = (struct attribute_gain*) ptr2;

	if (ag1->gain > ag2->gain)
		return -1;
	else if (ag2->gain > ag1->gain)
		return 1;
	else
		return 0;
}

void sort_by_gain(struct attribute_gain *allgains, size_t count)
{
	qsort(allgains, count, sizeof(struct attribute_gain),
	      attribute_gain_cmp);
}

void output_gains(FILE *outstream, struct attribute_gain *allgains,
		size_t count, size_t refattr)
{
	size_t i;

	for (i = 0; i < count; i++) {
		if (allgains[i].attribute == refattr)
			continue;

		fprintf(outstream, "attribute%u %lf\n",
			allgains[i].attribute,
		        allgains[i].gain);
	}
}

int main(int argc, char **argv) 
{
	size_t count;
	FILE *instream, *outstream;
	struct table_stats *ts;
	struct attribute_gain *allgains;
	int errcode = 0;

	if (argc <= 1) {
		printf("Usage:\n\n"
		       "%s <file.cvs> [outputfile]\n\n"
		       "UTP, Intro. IA, Bogdano Arendartchuk <debogdano@gmail.com>\n"
		       "%s %s\n", 
		       argv[0], __FILE__, __DATE__);
		return 0;
	}
	else if (argc == 2)
		outstream = stdout;
	else if (argc == 3) {
		outstream = fopen(argv[2], "w");
		if (!outstream) {
			perror("failed while opening output file");
			errcode = 1;
			goto failed_open_output;
		}
	}
	else {
		fprintf(stderr, "invalid number of arguments\n");
		errcode = 1;
		goto failed_parsing_args;
	}

	instream = fopen(argv[1], "r");
	if (!instream) {
		perror("failed while opening input file");
		errcode = 2;
		goto failed_open_input;
	}

	ts = collect_stats(instream);
	if (!ts) {
		perror("failed parsing file");
		errcode = 3;
		goto failed_collect;
	}
#ifdef DEBUG
	dump_table_stats(ts);
#endif

	allgains = get_gain(ts, &count);
	if (!allgains) {
		perror("failed calculating gains");
		errcode = 4;
		goto failed_gain;
	}
	sort_by_gain(allgains, count);
	output_gains(outstream, allgains, count, ts->refattr);

	free(allgains);
failed_gain:
	free_table_stats(ts);
failed_collect:
	fclose(instream);
failed_open_input:
	if (outstream != stdout)
		fclose(outstream);
failed_parsing_args:
failed_open_output:

	return errcode;
}
