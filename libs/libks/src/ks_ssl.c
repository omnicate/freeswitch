/*
 * Copyright (c) 2007-2014, Anthony Minessale II
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ks.h>

static ks_mutex_t **ssl_mutexes;
static ks_pool_t *ssl_pool = NULL;
static int ssl_count = 0;
static int is_init = 0;

static inline void ks_ssl_lock_callback(int mode, int type, char *file, int line)
{
	if (mode & CRYPTO_LOCK) {
		ks_mutex_lock(ssl_mutexes[type]);
	}
	else {
		ks_mutex_unlock(ssl_mutexes[type]);
	}
}

static inline unsigned long ks_ssl_thread_id(void)
{
	return (unsigned long) ks_thread_self();
}

KS_DECLARE(void) ks_ssl_init_ssl_locks(void)
{

	int i, num;
	
	if (is_init) return;

	is_init = 1;

	SSL_library_init();

	if (ssl_count == 0) {
		num = CRYPTO_num_locks();
		
		ssl_mutexes = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(ks_mutex_t*));
		ks_assert(ssl_mutexes != NULL);

		ks_pool_open(&ssl_pool);

		for (i = 0; i < num; i++) {
			ks_mutex_create(&(ssl_mutexes[i]), KS_MUTEX_FLAG_DEFAULT, ssl_pool);
			ks_assert(ssl_mutexes[i] != NULL);
		}

		CRYPTO_set_id_callback(ks_ssl_thread_id);
		CRYPTO_set_locking_callback((void (*)(int, int, const char*, int))ks_ssl_lock_callback);
	}

	ssl_count++;
}

KS_DECLARE(void) ks_ssl_destroy_ssl_locks(void)
{
	int i;

	if (!is_init) return;

	is_init = 0;

	if (ssl_count == 1) {
		CRYPTO_set_locking_callback(NULL);
		for (i = 0; i < CRYPTO_num_locks(); i++) {
			if (ssl_mutexes[i]) {
				ks_mutex_destroy(&ssl_mutexes[i]);
			}
		}

		OPENSSL_free(ssl_mutexes);
		ssl_count--;
	}
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
