/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2012, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/F
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Eliot Gable <egable@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Eliot Gable <egable@gmail.com>
 *
 * switch_atomic.c -- Atomic Operations
 *
 * TODO: BSD / NetBSD Atomic Operation Support
 * TODO: C11 Atomic Operation Support
 *
 *
 */

#include <switch.h>
#ifndef WIN32
#include <switch_private.h>
#endif
#include "private/switch_core_pvt.h"

/* apr headers*/
#include <apr.h>
#include <apr_atomic.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_network_io.h>
#include <apr_errno.h>
#include <apr_thread_proc.h>
#include <apr_portable.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_thread_rwlock.h>
#include <apr_file_io.h>
#include <apr_poll.h>
#include <apr_dso.h>
#include <apr_strings.h>
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_file_info.h>
#include <apr_fnmatch.h>
#include <apr_tables.h>

/* apr_vformatter_buff_t definition*/
#include <apr_lib.h>

/* apr-util headers */
#include <apr_queue.h>
#include <apr_uuid.h>
#include <apr_md5.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef DARWIN
#include <libkern/OSAtomic.h>
#endif

SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_add(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = (int32_t)InterlockedExchangeAdd((LONG*)loc, (LONG)value);
#elseif DARWIN
	ret_val = OSAtomicAdd32Barrier(value, loc);
#else
	ret_val = __sync_fetch_and_add(loc, value);
#endif
	return ret_val;
}

SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_sub(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = (int32_t)InterlockedExchangeSubtract((LONG*)loc, (LONG)value);
#elseif DARWIN
	ret_val = OSAtomicAdd32Barrier((value * -1), loc);
#else
	ret_val = __sync_fetch_and_sub(loc, value);
#endif
	return ret_val;
}

SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_or(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = (int32_t)InterlockedOr((LONG*)loc, (LONG)value);
#elseif DARWIN
	ret_val = OSAtomicOr32OrigBarrier(value, loc);
#else
	ret_val = __sync_fetch_and_or(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_and(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = (int32_t)InterlockedAnd((LONG*)loc, (LONG)value);
#elseif DARWIN
	ret_val = OSAtomicAnd32OrigBarrier(value, loc);
#else
	ret_val = __sync_fetch_and_and(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_xor(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = (int32_t)InterlockedXor((LONG*)loc, (LONG)value);
#elseif DARWIN
	ret_val = OSAtomicXor32OrigBarrier(value, loc);
#else
	ret_val = __sync_fetch_and_xor(loc, value);
#endif
	return ret_val;
}



SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_add(switch_atomic64_t *loc, int64_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedExchangeAdd64((LONGLONG*)loc, (LONGLONG)value);
#elseif DARWIN
	ret_val = OSAtomicAdd64Barrier(value, loc);
#else
	ret_val = __sync_fetch_and_add(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_sub(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedExchangeSubtract64((LONGLONG*)loc, (LONGLONG)value);
#elseif DARWIN
	ret_val = OSAtomicAdd64Barrier((value * -1), loc);
#else
	ret_val = __sync_fetch_and_sub(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_or(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedOr64((LONGLONG*)loc, (LONGLONG)value);
#elseif DARWIN
	ret_val = OSAtomicOr64OrigBarrier(value, loc);
#else
	ret_val = __sync_fetch_and_or(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_and(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedAnd64((LONGLONG*)loc, (LONGLONG)value);
#elseif DARWIN
	ret_val = OSAtomicAnd64OrigBarrier(value, loc);
#else
	ret_val = __sync_fetch_and_and(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_xor(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedXor64((LONGLONG*)loc, (LONGLONG)value);
#elseif DARWIN
	ret_val = OSAtomicXor64OrigBarrier(value, loc);
#else
	ret_val = __sync_fetch_and_xor(loc, value);
#endif
	return ret_val;
}



SWITCH_DECLARE(int32_t) switch_atomic32_add_and_fetch(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedExchangeAdd((LONG*)loc, (LONG)value);
	ret_val += value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicAdd32Barrier(value, loc);
	ret_val += value; /* Compute the post-operation value for return */
#else
	ret_val = __sync_add_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_sub_and_fetch(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedExchangeSubtract((LONG*)loc, (LONG)value);
	ret_val -= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicAdd32Barrier((value * -1), loc);
	ret_val -= value; /* Compute the post-operation value for return */
#else
	ret_val = __sync_sub_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_or_and_fetch(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedOr((LONG*)loc, (LONG)value);
	ret_val |= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicOr32Barrier(value, loc);
#else
	ret_val = __sync_or_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_and_and_fetch(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedAnd((LONG*)loc, (LONG)value);
	ret_val &= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicAnd32Barrier(value, loc);
#else
	ret_val = __sync_and_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_xor_and_fetch(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedXor((LONG*)loc, (LONG)value);
	ret_val ^= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicXor32Barrier(value, loc);
#else
	ret_val = __sync_xor_and_fetch(loc, value);
#endif
	return ret_val;
}




SWITCH_DECLARE(int64_t) switch_atomic64_add_and_fetch(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedExchangeAdd64((LONGLONG*)loc, (LONGLONG)value);
	ret_val += value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicAdd64Barrier(value, loc);
	ret_val += value; /* Compute the post-operation value for return */
#else
	ret_val = __sync_add_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_sub_and_fetch(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedExchangeSubtract64((LONGLONG*)loc, (LONGLONG)value);
	ret_val -= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicAdd64Barrier((value * -1), loc);
	ret_val -= value; /* Compute the post-operation value for return */
#else
	ret_val = __sync_sub_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_or_and_fetch(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedOr64((LONGLONG*)loc, (LONGLONG)value);
	ret_val |= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicOr64Barrier(value, loc);
#else
	ret_val = __sync_or_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_and_and_fetch(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedAnd64((LONGLONG*)loc, (LONGLONG)value);
	ret_val &= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicAnd64Barrier(value, loc);
#else
	ret_val = __sync_and_and_fetch(loc, value);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_xor_and_fetch(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedXor64((LONGLONG*)loc, (LONGLONG)value);
	ret_val ^= value; /* Compute the post-operation value for return */
#elseif DARWIN
	ret_val = OSAtomicXor64Barrier(value, loc);
#else
	ret_val = __sync_xor_and_fetch(loc, value);
#endif
	return ret_val;
}




SWITCH_DECLARE(int32_t) switch_atomic32_inc(switch_atomic32_t *loc)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedIncrement((LONG*)loc);
#elseif DARWIN
	ret_val = OSAtomicIncrement32Barrier(loc);
#else
	ret_val = __sync_fetch_and_add(loc, 1);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_dec(switch_atomic32_t *loc)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedDecrement((LONG*)loc);
#elseif DARWIN
	ret_val = OSAtomicDecrement32Barrier(loc);
#else
	ret_val = __sync_fetch_and_sub(loc, 1);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_get(switch_atomic32_t *loc)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedOr((LONG*)loc, (LONG)0);
#elseif DARWIN
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (OSAtomicCompareAndSwap32Barrier(ret_val, ret_val, loc)) break;
	}
#else
	ret_val = __sync_fetch_and_or(loc, 0);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_set(switch_atomic32_t *loc, int32_t value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (InterlockedCompareExchange((LONG*)loc, (LONG)value, (LONG)ret_val) == ret_val) break;
	}
#elseif DARWIN
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (OSAtomicCompareAndSwap32Barrier(ret_val, value, loc)) break;
	}
#else
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (__sync_bool_compare_and_swap(loc, ret_val, value)) break;
	}
#endif
	return ret_val;
}



SWITCH_DECLARE(int64_t) switch_atomic64_inc(switch_atomic64_t *loc)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedIncrement64((LONGLONG*)loc);
#elseif DARWIN
	ret_val = OSAtomicIncrement64Barrier(loc);
#else
	ret_val = __sync_fetch_and_add(loc, 1);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_dec(switch_atomic64_t *loc)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedDecrement64((LONGLONG*)loc);
#elseif DARWIN
	ret_val = OSAtomicDecrement64Barrier(loc);
#else
	ret_val = __sync_fetch_and_sub(loc, 1);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_get(switch_atomic64_t *loc)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedOr64((LONGLONG*)loc, (LONGLONG)0);
#elseif DARWIN
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (OSAtomicCompareAndSwap64Barrier(ret_val, ret_val, loc)) break;
	}
#else
	ret_val = __sync_fetch_and_or(loc, 0);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_set(switch_atomic64_t *loc, int64_t value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (InterlockedCompareExchange64((LONGLONG*)loc, (LONGLONG)value, (LONGLONG)ret_val) == ret_val) break;
	}
#elseif DARWIN
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (OSAtomicCompareAndSwap64Barrier(ret_val, value, loc)) break;
	}
#else
	while (SWITCH_TRUE) {
		ret_val = *loc;
		if (__sync_bool_compare_and_swap(loc, ret_val, value)) break;
	}
#endif
	return ret_val;
}



SWITCH_DECLARE(switch_bool_t) switch_atomic32_bool_cas(switch_atomic32_t *loc, int32_t old_value, int32_t new_value)
{
	switch_bool_t ret_val = SWITCH_FALSE;
#ifdef WIN32
	ret_val = (InterlockedCompareExchange((LONG*)loc, (LONG)new_value, (LONG)old_value) == old_value ? SWITCH_TRUE : SWITCH_FALSE);
#elseif DARWIN
	ret_val = (OSAtomicCompareAndSwap32Barrier(old_value, new_value, loc) ? SWITCH_TRUE : SWITCH_FALSE);
#else
	ret_val = (__sync_bool_compare_and_swap(loc, old_value, new_value) ? SWITCH_TRUE : SWITCH_FALSE);
#endif
	return ret_val;
}


SWITCH_DECLARE(int32_t) switch_atomic32_val_cas(switch_atomic32_t *loc, int32_t old_value, int32_t new_value)
{
	int32_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedCompareExchange((LONG*)loc, (LONG)new_value, (LONG)old_value);
#elseif DARWIN
	ret_val = (OSAtomicCompareAndSwap32Barrier(old_value, new_value, loc) ? old_value : switch_atomic32_get(loc));
#else
	ret_val = __sync_val_compare_and_swap(loc, old_value, new_value);
#endif
	return ret_val;
}



SWITCH_DECLARE(switch_bool_t) switch_atomic64_bool_cas(switch_atomic64_t *loc, int64_t old_value, int64_t new_value)
{
	switch_bool_t ret_val = SWITCH_FALSE;
#ifdef WIN32
	ret_val = (InterlockedCompareExchange64((LONGLONG*)loc, (LONGLONG)new_value, (LONGLONG)old_value) == old_value ? SWITCH_TRUE : SWITCH_FALSE);
#elseif DARWIN
	ret_val = (OSAtomicCompareAndSwap64Barrier(old_value, new_value, loc) ? SWITCH_TRUE : SWITCH_FALSE);
#else
	ret_val = (__sync_bool_compare_and_swap(loc, old_value, new_value) ? SWITCH_TRUE : SWITCH_FALSE);
#endif
	return ret_val;
}


SWITCH_DECLARE(int64_t) switch_atomic64_val_cas(switch_atomic64_t *loc, int64_t old_value, int64_t new_value)
{
	int64_t ret_val = 0;
#ifdef WIN32
	ret_val = InterlockedCompareExchange64((LONGLONG*)loc, (LONGLONG)new_value, (LONGLONG)old_value);
#elseif DARWIN
	ret_val = (OSAtomicCompareAndSwap64Barrier(old_value, new_value, loc) ? old_value : switch_atomic64_get(loc));
#else
	ret_val = __sync_val_compare_and_swap(loc, old_value, new_value);
#endif
	return ret_val;
}


SWITCH_DECLARE(switch_atomic_ptr_t) switch_atomic_ptr_get(switch_atomic_ptr_t *loc)
{
	switch_atomic_ptr_t ret_val = NULL;
#ifdef WIN32
	ret_val = InterlockedCompareExchangePointer((PVOID*)loc, (PVOID)0, (PVOID)0);
#elseif DARWIN
	OSMemoryBarrier();
	ret_val = *loc;    /* Assumes the location is 32-bit aligned on 32-bit systems and 64-bit aligned on 64-bit systems */
	OSMemoryBarrier();
#else
	ret_val = __sync_val_compare_and_swap(loc, 0, 0);
#endif
	return ret_val;
}

SWITCH_DECLARE(switch_atomic_ptr_t) switch_atomic_ptr_set(switch_atomic_ptr_t *loc, switch_atomic_ptr_t value)
{
	switch_atomic_ptr_t ret_val = NULL;
#ifdef WIN32
	while (1) {
		ret_val = switch_atomic_ptr_get(loc);
		if (InterlockedCompareExchangePointer((PVOID*)loc, (PVOID)value, (PVOID)ret_val) == ret_val) break;
#elseif DARWIN
	while (1) {
		OSMemoryBarrier();
		ret_val = *loc;    /* Assumes the location is 32-bit aligned on 32-bit systems and 64-bit aligned on 64-bit systems */
		if (OSAtomicCompareAndSwapPtrBarrier(ret_val, value, loc)) break;
	}
#else
	while (1) {
		ret_val = switch_atomic_ptr_get(loc);
		if (__sync_val_compare_and_swap(loc, ret_val, value) == ret_val) break;
	}
#endif
	return ret_val;
}

SWITCH_DECLARE(switch_atomic_ptr_t) switch_atomic_ptr_cas(switch_atomic_ptr_t *loc, switch_atomic_ptr_t old_value, switch_atomic_ptr_t new_value)
{
	switch_atomic_ptr_t ret_val = NULL;
#ifdef WIN32
	ret_val = InterlockedCompareExchangePointer((PVOID*)loc, (PVOID)new_value, (PVOID)old_value);
#elseif DARWIN
	ret_val = (OSAtomicCompareAndSwapPtrBarrier(old_value, new_value, loc) ? old_value : switch_atomic_ptr_get(loc));
#else
	ret_val = __sync_val_compare_and_swap(loc, old_value, new_value);
#endif
	return ret_val;
}



SWITCH_DECLARE(switch_status_t) switch_atomic_init(switch_memory_pool_t *pool)
{
	return apr_atomic_init((apr_pool_t *) pool);
}

SWITCH_DECLARE(uint32_t) switch_atomic_read(volatile switch_atomic_t *mem)
{
#ifdef apr_atomic_t
	return apr_atomic_read((apr_atomic_t *)mem);
#else
	return apr_atomic_read32((apr_uint32_t *)mem);
#endif
}

SWITCH_DECLARE(void) switch_atomic_set(volatile switch_atomic_t *mem, uint32_t val)
{
#ifdef apr_atomic_t
	apr_atomic_set((apr_atomic_t *)mem, val);
#else
	apr_atomic_set32((apr_uint32_t *)mem, val);
#endif
}

SWITCH_DECLARE(void) switch_atomic_add(volatile switch_atomic_t *mem, uint32_t val)
{
#ifdef apr_atomic_t
	apr_atomic_add((apr_atomic_t *)mem, val);
#else
	apr_atomic_add32((apr_uint32_t *)mem, val);
#endif
}

SWITCH_DECLARE(void) switch_atomic_inc(volatile switch_atomic_t *mem)
{
#ifdef apr_atomic_t
	apr_atomic_inc((apr_atomic_t *)mem);
#else
	apr_atomic_inc32((apr_uint32_t *)mem);
#endif
}

SWITCH_DECLARE(int) switch_atomic_dec(volatile switch_atomic_t *mem)
{
#ifdef apr_atomic_t
	return apr_atomic_dec((apr_atomic_t *)mem);
#else
	return apr_atomic_dec32((apr_uint32_t *)mem);
#endif
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
