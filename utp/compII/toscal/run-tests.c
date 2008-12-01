/*
 * A small program to run the tests and check whether the output of the
 * tests have changed. It may help tracking regressions in the code
 *
 * It should run on Windows too.
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#define PATH_BASE	"./tests/tokenizer"
#define PATH_SUCCESS	PATH_BASE "/success"
#define PATH_FAILED	PATH_BASE "/fail"
#define BINARY_PATH	"./tokenize"
#define OUTPUT_SUFFIX	"-output"

int check_path(const char *path, int succeed)
{
	int err, failed;
	char command[PATH_MAX * 3 + 4];
	char output[BUFSIZ];
	char original[BUFSIZ];
	char origpath[PATH_MAX+1];
	FILE *fout;
	FILE *forig;
	size_t readedorig;

	if (access(BINARY_PATH, X_OK|R_OK)) {
		fprintf(stderr, "where is the '%s'?\n", BINARY_PATH);
		goto error;
	}

	strcpy(origpath, path);
	strcat(origpath, OUTPUT_SUFFIX);

	strcpy(command, BINARY_PATH);
	strcat(command, " ");
	strncat(command, path, PATH_MAX);
	strcat(command, " 2>&1 "); /* ha-ha (FIXME won't run on Windows!!!) */

	fout = popen(command, "r");
	if (!fout)
		goto error;

	if (!fread(output, sizeof(char), sizeof(output), fout)) {
		pclose(fout);
		goto error;
	}

	err = pclose(fout);

	forig = fopen(origpath, "r");
	if (!forig) {
		perror(origpath);
		goto error;
	}

	readedorig = fread(original, sizeof(char), sizeof(original), forig);
	if (readedorig == 0) {
		fclose(forig);
		goto error;
	}
	fclose(forig);

	failed = 0;

	if (memcmp(output, original, readedorig)) {
		printf("DIFFER");
		failed = 1;
	}

	if ((succeed && err != 0) || (!succeed && err == 0)) {
		printf("FAILED ret %d ", err);
		failed = 1;
	}

	if (!failed)
		printf("GOOD");

	printf(" %s\n", path);


	return 1;
error:
	return 0;
}

int expect_on(const char *path, int succeed)
{
	DIR *dirp;
	struct dirent *dent;
	struct stat st;
	char checkpath[PATH_MAX+1];

	dirp = opendir(path);
	if (!dirp)
		goto error;

	while (dent = readdir(dirp)) {
		if (strcmp(rindex(dent->d_name, '.'), ".txt"))
			continue;

		strncpy(checkpath, path, PATH_MAX);
		strncat(checkpath, "/", PATH_MAX);
		strncat(checkpath, dent->d_name, PATH_MAX);

		if (stat(checkpath, &st) == -1)
			goto error;

		if (!S_ISREG(st.st_mode))
			continue;

		if (!check_path(checkpath, succeed))
			goto error;
	}

	return 1;
error:
	return 0;
}

int main(int argc, char *argv[])
{
	if (!expect_on(PATH_SUCCESS, 1))
		goto error;
	if (!expect_on(PATH_FAILED, 0))
		goto error;

	return 0;
error:
	perror("while checking stuff");

	return 1;
}
