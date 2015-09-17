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

#include "ks.h"

size_t thread_default_stacksize = 240 * 1024;

void ks_thread_override_default_stacksize(size_t size)
{
	thread_default_stacksize = size;
}

static void ks_thread_cleanup(ks_pool_t *mpool, void *ptr, void *arg, int type, ks_pool_cleanup_action_t action)
{
	ks_thread_t *thread = (ks_thread_t *) ptr;

	switch(action) {
	case KS_MPCL_ANNOUNCE:
		thread->running = 0;
		break;
	case KS_MPCL_TEARDOWN:
		if (!(thread->flags & KS_THREAD_FLAG_DETATCHED)) {
			ks_thread_join(thread);
		}
		break;
	case KS_MPCL_DESTROY:
#ifdef WIN32
		if (!(thread->flags & KS_THREAD_FLAG_DETATCHED)) {
			CloseHandle(thread->handle);
		}
#endif
		break;
	}
}

static void *KS_THREAD_CALLING_CONVENTION thread_launch(void *args)
{
	void *exit_val;
	ks_thread_t *thread = (ks_thread_t *) args;

#ifdef HAVE_PTHREAD_SETSCHEDPARAM
	if (thread->priority) {
		int policy;
		struct sched_param param = { 0 };
		pthread_t tt = pthread_self();

		pthread_getschedparam(tt, &policy, &param);
		param.sched_priority = thread->priority;
		pthread_setschedparam(tt, policy, &param);
	}
#endif

	exit_val = thread->function(thread, thread->private_data);
#ifndef WIN32
	pthread_attr_destroy(&thread->attribute);
#endif

	return exit_val;
}

KS_DECLARE(ks_status_t) ks_thread_join(ks_thread_t *thread) {
#ifdef WIN32
	WaitForSingleObject(thread->handle, INFINITE);
#else
	void *ret;
	pthread_join(thread->handle, &ret);
#endif
	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_thread_create_ex(ks_thread_t **rthread, ks_thread_function_t func, void *data,
										 uint32_t flags, size_t stack_size, ks_thread_priority_t priority, ks_pool_t *pool)
{
	ks_thread_t *thread = NULL;
	ks_status_t status = KS_STATUS_FAIL;

	if (!rthread) goto done;

	*rthread = NULL;

	if (!func || !pool) goto done;

	thread = (ks_thread_t *) ks_pool_alloc(pool, sizeof(ks_thread_t));

	if (!thread) goto done;

	thread->private_data = data;
	thread->function = func;
	thread->stack_size = stack_size;
	thread->running = 1;
	thread->flags = flags;
	thread->priority = priority;

#if defined(WIN32)
	thread->handle = (void *) _beginthreadex(NULL, (unsigned) thread->stack_size, (unsigned int (__stdcall *) (void *)) thread_launch, thread, 0, NULL);

	if (!thread->handle) {
		goto fail;
	}

	if (priority >= 99) {
		SetThreadPriority(thread->handle, THREAD_PRIORITY_TIME_CRITICAL);
	} else if (priority >= 50) {
		SetThreadPriority(thread->handle, THREAD_PRIORITY_ABOVE_NORMAL);
	} else if (priority >= 10) {
		SetThreadPriority(thread->handle, THREAD_PRIORITY_NORMAL);
	} else if (priority >= 1) {
		SetThreadPriority(thread->handle, THREAD_PRIORITY_LOWEST);
	}

	if (flags & KS_THREAD_FLAG_DETATCHED) {
		CloseHandle(thread->handle);
	}

	status = KS_STATUS_SUCCESS;
	goto done;
#else

	if (pthread_attr_init(&thread->attribute) != 0)
		goto fail;

	if ((flags & KS_THREAD_FLAG_DETATCHED) && pthread_attr_setdetachstate(&thread->attribute, PTHREAD_CREATE_DETACHED) != 0)
		goto failpthread;

	if (thread->stack_size && pthread_attr_setstacksize(&thread->attribute, thread->stack_size) != 0)
		goto failpthread;

	if (pthread_create(&thread->handle, &thread->attribute, thread_launch, thread) != 0)
		goto failpthread;

	status = KS_STATUS_SUCCESS;
	goto done;

  failpthread:

	pthread_attr_destroy(&thread->attribute);
#endif

  fail:
	if (thread) {
		thread->running = 0;
		if (pool) {
			ks_pool_safe_free(pool, thread);
		}
	}
  done:
	if (status == KS_STATUS_SUCCESS) {
		*rthread = thread;
		ks_pool_set_cleanup(pool, thread, NULL, 0, ks_thread_cleanup);
	}

	return status;
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
