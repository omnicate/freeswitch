#include "ks.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void fill(char *str, int bytes, char c)
{
	memset(str, c, bytes -1);
	*(str+(bytes-1)) = '\0';
}

struct foo {
	int x;
	char *str;
};


void cleanup(ks_mpool_t *mpool, void *ptr, void *arg, int type, ks_mpool_cleanup_action_t action)
{
	struct foo *foo = (struct foo *) ptr;

	printf("Cleanup %p action: %d\n", ptr, action);

	switch(action) {
	case KS_MPCL_ANNOUNCE:
		break;
	case KS_MPCL_TEARDOWN:
		break;
	case KS_MPCL_DESTROY:
		printf("DESTROY STR [%s]\n", foo->str);
		free(foo->str);
		foo->str = NULL;
	}
}



int main(int argc, char **argv)
{
	ks_mpool_t *pool;
	int err = 0;
	char *str = NULL;
	int bytes = 1024;
	ks_status_t status;
	struct foo *foo;

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
	if (status != KS_STATUS_SUCCESS) {
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

	err = ks_mpool_safe_free(pool, str);
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


	printf("ALLOC OBJ:\n");

	foo = ks_mpool_alloc(pool, sizeof(struct foo), &err);

	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "ALLOC OBJ: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	} else {
		printf("ALLOC OBJ [%p]:\n", (void *) foo);
	}

	foo->x = 12;
	foo->str = strdup("This is a test 1234 abcd; This will be called on explicit free\n");
	ks_mpool_set_cleanup(pool, foo, NULL, 0, cleanup);

	printf("FREE OBJ:\n");

	err = ks_mpool_safe_free(pool, foo);
	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "FREE OBJ ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}


	printf("ALLOC OBJ2:\n");

	foo = ks_mpool_alloc(pool, sizeof(struct foo), &err);

	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "ALLOC OBJ2: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	} else {
		printf("ALLOC OBJ2 [%p]:\n", (void *) foo);
	}

	foo->x = 12;
	foo->str = strdup("This is a second test 1234 abcd; This will be called on pool clear/destroy\n");
	ks_mpool_set_cleanup(pool, foo, NULL, 0, cleanup);


	printf("ALLOC OBJ3:\n");

	foo = ks_mpool_alloc(pool, sizeof(struct foo), &err);

	if (err != KS_MPOOL_ERROR_NONE) {
		fprintf(stderr, "ALLOC OBJ3: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	} else {
		printf("ALLOC OBJ3 [%p]:\n", (void *) foo);
	}

	foo->x = 12;
	foo->str = strdup("This is a third test 1234 abcd; This will be called on pool clear/destroy\n");
	ks_mpool_set_cleanup(pool, foo, NULL, 0, cleanup);



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
	
	if (status != KS_STATUS_SUCCESS) {
		fprintf(stderr, "CLOSE ERR: [%s]\n", ks_mpool_strerror(err));
		exit(255);
	}
	
	exit(0);
}
