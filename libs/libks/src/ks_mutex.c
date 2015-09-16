/* 
 * Cross Platform Thread/Mutex abstraction
 * Copyright(C) 2007 Michael Jerris
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so.
 *
 * This work is provided under this license on an "as is" basis, without warranty of any kind,
 * either expressed or implied, including, without limitation, warranties that the covered code
 * is free of defects, merchantable, fit for a particular purpose or non-infringing. The entire
 * risk as to the quality and performance of the covered code is with you. Should any covered
 * code prove defective in any respect, you (not the initial developer or any other contributor)
 * assume the cost of any necessary servicing, repair or correction. This disclaimer of warranty
 * constitutes an essential part of this license. No use of any covered code is authorized hereunder
 * except under this disclaimer. 
 *
 */

#ifdef WIN32
/* required for TryEnterCriticalSection definition.  Must be defined before windows.h include */
#define _WIN32_WINNT 0x0400
#endif

#include "ks.h"
#include "ks_threadmutex.h"

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

typedef enum {
	KS_MUTEX_TYPE_DEFAULT,
	KS_MUTEX_TYPE_NON_RECURSIVE
} ks_mutex_type_t;

struct ks_mutex {
#ifdef WIN32
	CRITICAL_SECTION mutex;
	HANDLE handle;
#else
	pthread_mutex_t mutex;
#endif
	ks_pool_t * pool;
	ks_mutex_type_t type;
};

static void ks_mutex_cleanup(ks_pool_t *mpool, void *ptr, void *arg, int type, ks_pool_cleanup_action_t action)
{
	ks_mutex_t *mutex = (ks_mutex_t *) ptr;

	switch(action) {
	case KS_MPCL_ANNOUNCE:
		break;
	case KS_MPCL_TEARDOWN:
		break;
	case KS_MPCL_DESTROY:
#ifdef WIN32
		if (mutex->type == KS_MUTEX_TYPE_NON_RECURSIVE) {
			CloseHandle(mutex->handle);
		} else {
			DeleteCriticalSection(&mutex->mutex);
		}
#else
		pthread_mutex_destroy(&mutex->mutex);
#endif
		break;
	}
}


KS_DECLARE(ks_status_t) ks_mutex_create(ks_mutex_t **mutex, unsigned int flags, ks_pool_t *pool)
{
	ks_status_t status = KS_STATUS_FAIL;
#ifndef WIN32
	pthread_mutexattr_t attr;
#endif
	ks_mutex_t *check = NULL;
	int err = 0;

	if (!pool)
		goto done;

	check = (ks_mutex_t *) ks_pool_alloc(pool, sizeof(**mutex), &err);

	if (!check)
		goto done;

	check->pool = pool;
	check->type = KS_MUTEX_TYPE_DEFAULT;

#ifdef WIN32
	if (flags & KS_MUTEX_FLAG_NON_RECURSIVE) {
		check->type = KS_MUTEX_TYPE_NON_RECURSIVE;
		check->handle = CreateEvent(NULL, FALSE, TRUE, NULL);
	} else {
		InitializeCriticalSection(&check->mutex);
	}
#else
	if (flags & KS_MUTEX_FLAG_NON_RECURSIVE) {
		if (pthread_mutex_init(&check->mutex, NULL))
			goto done;

	} else {
		if (pthread_mutexattr_init(&attr))
			goto done;

		if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE))
			goto fail;

		if (pthread_mutex_init(&check->mutex, &attr))
			goto fail;
	}

	goto success;

  fail:
	pthread_mutexattr_destroy(&attr);
	goto done;

  success:
#endif
	*mutex = check;
	status = KS_STATUS_SUCCESS;
	ks_pool_set_cleanup(pool, check, NULL, 0, ks_mutex_cleanup);

  done:
	return status;
}

KS_DECLARE(ks_status_t) ks_mutex_destroy(ks_mutex_t **mutex)
{
	ks_mutex_t *mp = *mutex;
	int err;

	*mutex = NULL;

	if (!mp) {
		return KS_STATUS_FAIL;
	}

	err = ks_pool_safe_free(mp->pool, mp);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_mutex_lock(ks_mutex_t *mutex)
{
#ifdef WIN32
	if (mutex->type == KS_MUTEX_TYPE_NON_RECURSIVE) {
        DWORD ret = WaitForSingleObject(mutex->handle, INFINITE);
		if ((ret != WAIT_OBJECT_0) && (ret != WAIT_ABANDONED)) {
            return KS_STATUS_FAIL;
		}
	} else {
		EnterCriticalSection(&mutex->mutex);
	}
#else
	if (pthread_mutex_lock(&mutex->mutex))
		return KS_STATUS_FAIL;
#endif
	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_mutex_trylock(ks_mutex_t *mutex)
{
#ifdef WIN32
	if (mutex->type == KS_MUTEX_TYPE_NON_RECURSIVE) {
        DWORD ret = WaitForSingleObject(mutex->handle, 0);
		if ((ret != WAIT_OBJECT_0) && (ret != WAIT_ABANDONED)) {
            return KS_STATUS_FAIL;
		}
	} else {
		if (!TryEnterCriticalSection(&mutex->mutex))
			return KS_STATUS_FAIL;
	}
#else
	if (pthread_mutex_trylock(&mutex->mutex))
		return KS_STATUS_FAIL;
#endif
	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_mutex_unlock(ks_mutex_t *mutex)
{
#ifdef WIN32
	if (mutex->type == KS_MUTEX_TYPE_NON_RECURSIVE) {
        if (!SetEvent(mutex->handle)) {
			return KS_STATUS_FAIL;
		}
	} else {
		LeaveCriticalSection(&mutex->mutex);
	}
#else
	if (pthread_mutex_unlock(&mutex->mutex))
		return KS_STATUS_FAIL;
#endif
	return KS_STATUS_SUCCESS;
}





/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
