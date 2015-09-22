#include "ks.h"
#include "tap.h"

static ks_pool_t *pool;
static ks_mutex_t *mutex;
static ks_mutex_t *mutex_non_recursive;
static int counter1 = 0;
static int counter2 = 0;
static int counter3 = 0;
static int threadscount = 0;

static ks_thread_t *thread1;
static ks_thread_t *thread2;
static ks_thread_t *thread3;
static ks_thread_t *thread4;
static ks_thread_t *thread5;
static ks_thread_t *thread6;
static ks_thread_t *thread7;
static ks_thread_t *thread8;
static ks_thread_t *thread9;
static ks_thread_t *thread10;
static ks_thread_t *thread11;
static ks_thread_t *thread12;

#define LOOP_COUNT 10000
static void *thread_test_function_cleanup(ks_thread_t *thread, void *data)
{
	int i;
	int d = (int)(intptr_t)data;

	while (thread->running) {
		sleep(1);
	}

	if ( d == 1 ) {
		ks_mutex_lock(mutex);
		counter3++;
		ks_mutex_unlock(mutex);
	}

	return NULL;
}

static void *thread_test_function_detatched(ks_thread_t *thread, void *data)
{
	int i;
	int d = (int)(intptr_t)data;

	for (i = 0; i < LOOP_COUNT; i++) {
		ks_mutex_lock(mutex);
		if (d == 1) {
			counter2++;
		}
		ks_mutex_unlock(mutex);
	}
	ks_mutex_lock(mutex);
	threadscount++;
	ks_mutex_unlock(mutex);
	
	return NULL;
}

static void *thread_test_function_atatched(ks_thread_t *thread, void *data)
{
	int i;
	int d = (int)(intptr_t)data;

	for (i = 0; i < LOOP_COUNT; i++) {
		ks_mutex_lock(mutex);
		if (d == 1) {
			counter1++;
		}
		ks_mutex_unlock(mutex);
	}
	return NULL;
}

static void create_threads_cleanup(void)
{
	void *d = (void *)(intptr_t)1;
	ok( (ks_thread_create(&thread1, thread_test_function_cleanup, d, pool) == KS_STATUS_SUCCESS) );
	ok( (ks_thread_create(&thread2, thread_test_function_cleanup, d, pool) == KS_STATUS_SUCCESS) );
	ok( (ks_thread_create(&thread3, thread_test_function_cleanup, d, pool) == KS_STATUS_SUCCESS) );
	ok( (ks_thread_create(&thread4, thread_test_function_cleanup, d, pool) == KS_STATUS_SUCCESS) );
}

static void create_threads_atatched(void)
{
	void *d = (void *)(intptr_t)1;
	ok( (ks_thread_create(&thread1, thread_test_function_atatched, d, pool) == KS_STATUS_SUCCESS) );
	ok( (ks_thread_create(&thread2, thread_test_function_atatched, d, pool) == KS_STATUS_SUCCESS) );
	ok( (ks_thread_create(&thread3, thread_test_function_atatched, d, pool) == KS_STATUS_SUCCESS) );
	ok( (ks_thread_create(&thread4, thread_test_function_atatched, d, pool) == KS_STATUS_SUCCESS) );
}

static void create_threads_detatched(void)
{
	ks_status_t status;
	void *d = (void *)(intptr_t)1;
	
	status = ks_thread_create_ex(&thread5, thread_test_function_detatched, d, KS_THREAD_FLAG_DETATCHED, KS_THREAD_DEFAULT_STACK, KS_PRI_NORMAL, pool);
	ok( status == KS_STATUS_SUCCESS );
	status = ks_thread_create_ex(&thread6, thread_test_function_detatched, d, KS_THREAD_FLAG_DETATCHED, KS_THREAD_DEFAULT_STACK, KS_PRI_NORMAL, pool);
	ok( status == KS_STATUS_SUCCESS );
	status = ks_thread_create_ex(&thread7, thread_test_function_detatched, d, KS_THREAD_FLAG_DETATCHED, KS_THREAD_DEFAULT_STACK, KS_PRI_NORMAL, pool);
	ok( status == KS_STATUS_SUCCESS );
	status = ks_thread_create_ex(&thread8, thread_test_function_detatched, d, KS_THREAD_FLAG_DETATCHED, KS_THREAD_DEFAULT_STACK, KS_PRI_NORMAL, pool);
	ok( status == KS_STATUS_SUCCESS );
}

static void join_threads(void)
{
	ok( (KS_STATUS_SUCCESS == ks_thread_join(thread1)) );
	ok( (KS_STATUS_SUCCESS == ks_thread_join(thread2)) );
	ok( (KS_STATUS_SUCCESS == ks_thread_join(thread3)) );
	ok( (KS_STATUS_SUCCESS == ks_thread_join(thread4)) );
}

static void check_atatched(void)
{
	ok( counter1 == (LOOP_COUNT * 4) );
}

static void check_detached(void)
{
	ok( counter2 == (LOOP_COUNT * 4) );
}

static void create_pool(void)
{
	ok( (ks_pool_open(&pool) == KS_STATUS_SUCCESS) );
}

static void check_cleanup(void)
{
	ok( (ks_pool_close(&pool) == KS_STATUS_SUCCESS) );
	ok( (counter3 == 4) );
}

static void create_mutex(void)
{
	ok( (ks_mutex_create(&mutex, KS_MUTEX_FLAG_DEFAULT, pool) == KS_STATUS_SUCCESS) );
}

static void create_mutex_non_recursive(void)
{
	ok( (ks_mutex_create(&mutex_non_recursive, KS_MUTEX_FLAG_NON_RECURSIVE, pool) == KS_STATUS_SUCCESS) );
}

static void test_recursive_mutex(void)
{
	ks_status_t status;

	ks_mutex_lock(mutex);
	status = ks_mutex_trylock(mutex);
	if (status == KS_STATUS_SUCCESS) {
		ks_mutex_unlock(mutex);
	}
	ok(status == KS_STATUS_SUCCESS);
	ks_mutex_unlock(mutex);
}

static void test_non_recursive_mutex(void)
{
	ks_status_t status;
	ks_mutex_lock(mutex_non_recursive);
	status = ks_mutex_trylock(mutex_non_recursive);
	if (status == KS_STATUS_SUCCESS) {
		ks_mutex_unlock(mutex_non_recursive);
	}
	ok(status != KS_STATUS_SUCCESS);
	ks_mutex_unlock(mutex_non_recursive);
}


int main(int argc, char **argv)
{
	plan(25);

	create_pool();
	create_mutex();
	create_mutex_non_recursive();
	test_recursive_mutex();
	test_non_recursive_mutex();
	create_threads_atatched();
	join_threads();
	check_atatched();
	create_threads_detatched();
	while (threadscount != 4) sleep(1);
	check_detached();
	create_threads_cleanup();
	check_cleanup();

	done_testing();
	exit(0);
}
