#include "blade.h"
#include "tap.h"


int main(void)
{
	ks_pool_t *pool = NULL;
	ks_status_t status;

	blade_init();

	plan(1);

	status = ks_pool_open(&pool);
	ks_pool_close(&pool);

	ok(status == 0);
	done_testing();

	blade_shutdown();

	return 0;
}
