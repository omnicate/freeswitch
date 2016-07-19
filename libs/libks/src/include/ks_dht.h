/*
Copyright (c) 2009-2011 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _KS_DHT_H
#define _KS_DHT_H

#include "ks.h"

KS_BEGIN_EXTERN_C

typedef enum {
	KS_DHT_EVENT_NONE = 0,
	KS_DHT_EVENT_VALUES = 1,
	KS_DHT_EVENT_VALUES6 = 2,
	KS_DHT_EVENT_SEARCH_DONE = 3,
	KS_DHT_EVENT_SEARCH_DONE6 = 4
} ks_dht_event_t;

typedef void dht_callback(void *closure, ks_dht_event_t event, const unsigned char *info_hash, const void *data, size_t data_len);

typedef struct dht_handle_s dht_handle_t;

KS_DECLARE(int) dht_periodic(dht_handle_t *h, const void *buf, size_t buflen, const struct sockaddr *from, int fromlen,
				 time_t *tosleep, dht_callback *callback, void *closure);
KS_DECLARE(int) dht_init(dht_handle_t **h, int s, int s6, const unsigned char *id, const unsigned char *v, unsigned int port);
KS_DECLARE(int) dht_insert_node(dht_handle_t *h, const unsigned char *id, struct sockaddr *sa, int salen);
KS_DECLARE(int) dht_ping_node(dht_handle_t *h, struct sockaddr *sa, int salen);
KS_DECLARE(int) dht_search(dht_handle_t *h, const unsigned char *id, int port, int af, dht_callback *callback, void *closure);
KS_DECLARE(int) dht_nodes(dht_handle_t *h, int af, int *good_return, int *dubious_return, int *cached_return, int *incoming_return);
KS_DECLARE(void) dht_dump_tables(dht_handle_t *h, FILE *f);
KS_DECLARE(int) dht_get_nodes(dht_handle_t *h, struct sockaddr_in *sin, int *num, struct sockaddr_in6 *sin6, int *num6);
KS_DECLARE(int) dht_uninit(dht_handle_t **h);

/* This must be provided by the user. */
int dht_blacklisted(const struct sockaddr *sa, int salen);
void dht_hash(void *hash_return, int hash_size, const void *v1, int len1, const void *v2, int len2, const void *v3, int len3);
int dht_random_bytes(void *buf, size_t size);

KS_DECLARE(int) ks_dht_send_message_mutable(dht_handle_t *h, unsigned char *sk, unsigned char *pk, const struct sockaddr *sa, int salen,
											char *message_id, int sequence, char *message, ks_time_t life);

KS_DECLARE(int) ks_dht_send_message_mutable_cjson(dht_handle_t *h, unsigned char *sk, unsigned char *pk, const struct sockaddr *sa, int salen,
												  char *message_id, int sequence, cJSON *message, ks_time_t life);
KS_END_EXTERN_C

#endif /* _KS_DHT_H */

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
