/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2012, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 * Anthony Minessale II <anthm@freeswitch.org>
 *
 * switch_apr.h -- APR includes header
 *
 * TODO: BSD / NetBSD Atomic Operation Support
 * TODO: C11 Atomic Operation Support
 *
 */
/*! \file switch_atomic.h
    \brief Atomic Operations
	
	Basic atomic operations.

*/
#ifndef SWITCH_ATOMIC_H
#define SWITCH_ATOMIC_H

SWITCH_BEGIN_EXTERN_C




/**
 * @defgroup switch_atomic Multi-Threaded Atomic Operations
 * @ingroup switch_atomic
 * @{
 */

/** *DEPRECATED* Opaque type used for the atomic operations */
#ifdef apr_atomic_t
    typedef apr_atomic_t switch_atomic_t;
#else
    typedef uint32_t switch_atomic_t;
#endif

typedef int32_t switch_atomic32_t;
typedef int64_t switch_atomic64_t;
typedef void* switch_atomic_ptr_t;

/**
 * *DEPRECATED* Some architectures require atomic operations internal structures to be
 * initialized before use.
 * @param pool The memory pool to use when initializing the structures.
 */
SWITCH_DECLARE(switch_status_t) switch_atomic_init(switch_memory_pool_t *pool);

/**
 * *DEPRECATED* Uses an atomic operation to read the uint32 value at the location specified
 * by mem.
 * @param mem The location of memory which stores the value to read.
 */
SWITCH_DECLARE(uint32_t) switch_atomic_read(volatile switch_atomic_t *mem);

/**
 * *DEPRECATED* Uses an atomic operation to set a uint32 value at a specified location of
 * memory.
 * @param mem The location of memory to set.
 * @param val The uint32 value to set at the memory location.
 */
SWITCH_DECLARE(void) switch_atomic_set(volatile switch_atomic_t *mem, uint32_t val);

/**
 * *DEPRECATED* Uses an atomic operation to add the uint32 value to the value at the
 * specified location of memory.
 * @param mem The location of the value to add to.
 * @param val The uint32 value to add to the value at the memory location.
 */
SWITCH_DECLARE(void) switch_atomic_add(volatile switch_atomic_t *mem, uint32_t val);

/**
 * *DEPRECATED* Uses an atomic operation to increment the value at the specified memroy
 * location.
 * @param mem The location of the value to increment.
 */
SWITCH_DECLARE(void) switch_atomic_inc(volatile switch_atomic_t *mem);

/**
 * *DEPRECATED* Uses an atomic operation to decrement the value at the specified memroy
 * location.
 * @param mem The location of the value to decrement.
 */
SWITCH_DECLARE(int)  switch_atomic_dec(volatile switch_atomic_t *mem);



SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_add(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_sub(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_or(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_and(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_fetch_and_xor(switch_atomic32_t *loc, int32_t value);

SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_add(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_sub(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_or(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_and(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_fetch_and_xor(switch_atomic64_t *loc, int64_t value);

SWITCH_DECLARE(int32_t) switch_atomic32_add_and_fetch(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_sub_and_fetch(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_or_and_fetch(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_and_and_fetch(switch_atomic32_t *loc, int32_t value);
SWITCH_DECLARE(int32_t) switch_atomic32_xor_and_fetch(switch_atomic32_t *loc, int32_t value);

SWITCH_DECLARE(int64_t) switch_atomic64_add_and_fetch(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_sub_and_fetch(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_or_and_fetch(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_and_and_fetch(switch_atomic64_t *loc, int64_t value);
SWITCH_DECLARE(int64_t) switch_atomic64_xor_and_fetch(switch_atomic64_t *loc, int64_t value);

SWITCH_DECLARE(int32_t) switch_atomic32_inc(switch_atomic32_t *loc);
SWITCH_DECLARE(int32_t) switch_atomic32_dec(switch_atomic32_t *loc);
SWITCH_DECLARE(int32_t) switch_atomic32_get(switch_atomic32_t *loc);
SWITCH_DECLARE(int32_t) switch_atomic32_set(switch_atomic32_t *loc, int32_t value);

SWITCH_DECLARE(int64_t) switch_atomic64_inc(switch_atomic64_t *loc);
SWITCH_DECLARE(int64_t) switch_atomic64_dec(switch_atomic64_t *loc);
SWITCH_DECLARE(int64_t) switch_atomic64_get(switch_atomic64_t *loc);
SWITCH_DECLARE(int64_t) switch_atomic64_set(switch_atomic64_t *loc, int64_t value);

SWITCH_DECLARE(switch_bool_t) switch_atomic32_bool_cas(switch_atomic32_t *loc, int32_t old_value, int32_t new_value);
SWITCH_DECLARE(int32_t) switch_atomic32_val_cas(switch_atomic32_t *loc, int32_t old_value, int32_t new_value);

SWITCH_DECLARE(switch_bool_t) switch_atomic64_bool_cas(switch_atomic64_t *loc, int64_t old_value, int64_t new_value);
SWITCH_DECLARE(int64_t) switch_atomic64_val_cas(switch_atomic64_t *loc, int64_t old_value, int64_t new_value);

SWITCH_DECLARE(switch_atomic_ptr_t) switch_atomic_ptr_get(switch_atomic_ptr_t *loc);
SWITCH_DECLARE(switch_atomic_ptr_t) switch_atomic_ptr_set(switch_atomic_ptr_t *loc, switch_atomic_ptr_t value);
SWITCH_DECLARE(switch_atomic_ptr_t) switch_atomic_ptr_cas(switch_atomic_ptr_t *loc, switch_atomic_ptr_t old_value, switch_atomic_ptr_t new_value);


SWITCH_END_EXTERN_C
#endif
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

/** @} */
