#include "ks.h"
#include "tap.h"

int test1(void)
{
	ks_pool_t *pool;
	ks_hash_t *hash;
	int i, sum1 = 0, sum2 = 0;

	ks_pool_open(&pool);
	ks_hash_create(&hash, KS_HASH_MODE_DEFAULT, KS_HASH_FREE_BOTH, pool);

	for (i = 1; i < 1001; i++) {
		char *key = ks_pprintf(pool, "KEY %d", i);
		char *val = ks_pprintf(pool, "%d", i);
		ks_hash_insert(hash, key, val);
		sum1 += i;
	}


	
	ks_hash_iterator_t *itt;

	for (itt = ks_hash_first(hash); itt; itt = ks_hash_next(&itt)) {
		const void *key;
		void *val;

		ks_hash_this(itt, &key, NULL, &val);

		printf("%s=%s\n", (char *)key, (char *)val);
		sum2 += atoi(val);

		ks_hash_remove(hash, (char *)key);
	}


	//ks_hash_destroy(&hash);

	ks_pool_close(&pool);

	return (sum1 == sum2);
}


int main(int argc, char **argv)
{
	plan(1);

	ok(test1());

	done_testing();

	exit(0);
}
