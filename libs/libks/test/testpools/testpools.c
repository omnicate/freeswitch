#include <stdlib.h>
#include <stdio.h>
#include "mpool.h"
#include <string.h>

static void fill(char *str, int bytes, char c)
{
	memset(str, c, bytes -1);
	*(str+(bytes-1)) = '\0';
}

int main(int argc, char **argv)
{
	mpool_t *pool;
	int err = 0;
	char *str = NULL;
	int bytes = 1024;

	if (argc > 1) {
		int tmp = atoi(argv[1]);

		if (tmp > 0) {
			bytes = tmp;
		} else {
			fprintf(stderr, "INVALID\n");
			exit(255);
		}
	}

	pool = mpool_open(MPOOL_FLAG_DEFAULT, 0, NULL, &err);

	printf("OPEN:\n");
	if (!pool || err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "OPEN ERR: %d [%s]\n", err, mpool_strerror(err));
		exit(255);
	}

	printf("ALLOC:\n");
	str = mpool_alloc(pool, bytes, &err);

	if (err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "ALLOC ERR: [%s]\n", mpool_strerror(err));
		exit(255);
	}

	fill(str, bytes, '.');
	printf("%s\n", str);

	printf("FREE:\n");

	err = mpool_free(pool, str);
	if (err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "FREE ERR: [%s]\n", mpool_strerror(err));
		exit(255);
	}

	printf("ALLOC2:\n");

	str = mpool_alloc(pool, bytes, &err);

	if (err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "ALLOC2 ERR: [%s]\n", mpool_strerror(err));
		exit(255);
	}

	fill(str, bytes, '-');
	printf("%s\n", str);


	printf("RESIZE:\n");
	bytes *= 2;
	str = mpool_resize(pool, str, bytes, &err);

	if (err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "RESIZE ERR: [%s]\n", mpool_strerror(err));
		exit(255);
	}

	fill(str, bytes, '*');
	printf("%s\n", str);


	printf("FREE 2:\n");

	err = mpool_free(pool, str);
	if (err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "FREE2 ERR: [%s]\n", mpool_strerror(err));
		exit(255);
	}


	printf("CLEAR:\n");
	err = mpool_clear(pool);

	if (err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "CLEAR ERR: [%s]\n", mpool_strerror(err));
		exit(255);
	}

	printf("CLOSE:\n");
	err = mpool_close(pool);
	
	if (err != MPOOL_ERROR_NONE) {
		fprintf(stderr, "CLOSE ERR: [%s]\n", mpool_strerror(err));
		exit(255);
	}
	
	exit(0);
}
