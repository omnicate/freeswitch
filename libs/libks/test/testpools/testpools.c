#include <stdlib.h>
#include <stdio.h>
#include "ks_mpool.h"
#include <string.h>
#include <ks.h>

static void fill(char *str, int bytes, char c)
{
	memset(str, c, bytes -1);
	*(str+(bytes-1)) = '\0';
}

int main(int argc, char **argv)
{
	ks_mpool_t *pool;
	int err = 0;
	char *str = NULL;
	int bytes = 1024;
	ks_status_t status;

	if (argc > 1) {
		int tmp = atoi(argv[1]);

		if (tmp > 0) {
			bytes = tmp;
		} else {
			fprintf(stderr, "INVALID\n");
			exit(255);
		}
	}

	status = ks_mpool_open(&pool, &err);

	printf("OPEN:\n");
	if (status != KS_SUCCESS) {
		fprintf(stderr, "OPEN ERR: %d [%s]\n", err, ks_mpool_strerror(err));
		exit(255);
	}

	printf("ALLOC:\n");
	str = ks_mpool_alloc(pool, bytes, &err);

	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "ALLOC ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}

	fill(str, bytes, '.');
	printf("%s\n", str);

	printf("FREE:\n");

	err = ks_mpool_free(pool, str);
	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "FREE ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}

	printf("ALLOC2:\n");

	str = ks_mpool_alloc(pool, bytes, &err);

	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "ALLOC2 ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}

	fill(str, bytes, '-');
	printf("%s\n", str);


	printf("RESIZE:\n");
	bytes *= 2;
	str = ks_mpool_resize(pool, str, bytes, &err);

	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "RESIZE ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}

	fill(str, bytes, '*');
	printf("%s\n", str);


	printf("FREE 2:\n");

	err = ks_mpool_free(pool, str);
	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "FREE2 ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}


	printf("CLEAR:\n");
	err = ks_mpool_clear(pool);

	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "CLEAR ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}

	printf("CLOSE:\n");
	status = ks_mpool_close(&pool, &err);
	
	if (status != KS_SUCCESS) {
		fprintf(stderr, "CLOSE ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}
	
	exit(0);
}
