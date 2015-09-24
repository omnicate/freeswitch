#include "ks.h"
#include "tap.h"

int main(int argc, char **argv)
{
	int64_t now, then;
	int diff;
	int i;

	plan(2);

	ks_sleep(10000);

	then = ks_time_now();
	
	ks_sleep(2000000);

	now = ks_time_now();

	diff = (now - then) / 1000;
	printf("DIFF %ums\n", diff);
	

	ok( diff > 1990 && diff < 2010 );

	then = ks_time_now();

	for (i = 0; i < 100; i++) {
		ks_sleep(20000);
	}

	now = ks_time_now();

	diff = (now - then) / 1000;
	printf("DIFF %ums\n", diff);

	ok( diff > 1950 && diff < 2050 );

	done_testing();

	exit(0);
}
