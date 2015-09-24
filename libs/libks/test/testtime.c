#include "ks.h"
#include "tap.h"

int main(int argc, char **argv)
{
	int64_t now, then;
	int diff;
	int i;

	plan(2);

	then = ks_time_now();
	
	printf("THEN: %lld\n", then);

	ks_sleep(2000000);

	now = ks_time_now();
	printf("NOW: %lld\n", now);

	diff = (now - then) / 1000;
	printf("DIFF %ums\n", diff);
	

	ok( diff > 1990 && diff < 2010 );

	then = ks_time_now();
	
	printf("THEN: %lld\n", then);

	for (i = 0; i < 100; i++) {
		ks_sleep(20000);
	}

	now = ks_time_now();
	printf("NOW: %lld\n", now);

	diff = (now - then) / 1000;
	printf("DIFF %ums\n", diff);

	ok( diff > 1950 && diff < 2050 );

	done_testing();

	exit(0);
}
