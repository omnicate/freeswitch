#include "ks_dht.h"
#include "ks_dht-int.h"

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_alloc(ks_dht_message_t **message, ks_pool_t *pool)
{
	ks_dht_message_t *msg;

	ks_assert(message);
	ks_assert(pool);
	
	*message = msg = ks_pool_alloc(pool, sizeof(ks_dht_message_t));
	msg->pool = pool;
	msg->endpoint = NULL;
	msg->raddr = (const ks_sockaddr_t){ 0 };
	msg->args = NULL;
	msg->type[0] = '\0';
	msg->transactionid_length = 0;
	msg->data = NULL;

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_prealloc(ks_dht_message_t *message, ks_pool_t *pool)
{
	ks_assert(message);
	ks_assert(pool);
	
	message->pool = pool;
	message->endpoint = NULL;
	message->raddr = (const ks_sockaddr_t){ 0 };
	message->args = NULL;
	message->type[0] = '\0';
	message->transactionid_length = 0;
	message->data = NULL;

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_free(ks_dht_message_t *message)
{
	ks_assert(message);

	ks_dht_message_deinit(message);
	ks_pool_free(message->pool, message);

	return KS_STATUS_SUCCESS;
}


/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_init(ks_dht_message_t *message, ks_dht_endpoint_t *ep, ks_sockaddr_t *raddr, ks_bool_t alloc_data)
{
	ks_assert(message);
	ks_assert(message->pool);

	message->endpoint = ep;
	message->raddr = *raddr;
	message->data = NULL;
	message->args = NULL;
	message->transactionid_length = 0;
	message->type[0] = '\0';
	if (alloc_data) {
		message->data = ben_dict();
	}

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_deinit(ks_dht_message_t *message)
{
	ks_assert(message);

	message->endpoint = NULL;
	message->raddr = (const ks_sockaddr_t){ 0 };
	message->args = NULL;
	message->type[0] = '\0';
	message->transactionid_length = 0;
	if (message->data) {
		ben_free(message->data);
		message->data = NULL;
	}

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_parse(ks_dht_message_t *message, const uint8_t *buffer, ks_size_t buffer_length)
{
	struct bencode *t;
	struct bencode *y;
	const char *tv;
	const char *yv;
	ks_size_t tv_len;
	ks_size_t yv_len;

	ks_assert(message);
	ks_assert(message->pool);
	ks_assert(buffer);
	ks_assert(!message->data);

    message->data = ben_decode((const void *)buffer, buffer_length);
	if (!message->data) {
		ks_log(KS_LOG_DEBUG, "Message cannot be decoded\n");
		goto failure;
	}

	ks_log(KS_LOG_DEBUG, "Message decoded\n");
	ks_log(KS_LOG_DEBUG, "%s\n", ben_print(message->data));
	
    t = ben_dict_get_by_str(message->data, "t");
	if (!t) {
		ks_log(KS_LOG_DEBUG, "Message missing required key 't'\n");
		goto failure;
	}

	tv = ben_str_val(t);
	tv_len = ben_str_len(t);
	if (tv_len > KS_DHT_MESSAGE_TRANSACTIONID_MAX_SIZE) {
		ks_log(KS_LOG_DEBUG, "Message 't' value has an unexpectedly large size of %d\n", tv_len);
		goto failure;
	}

	memcpy(message->transactionid, tv, tv_len);
	message->transactionid_length = tv_len;
	// @todo hex output of transactionid
	//ks_log(KS_LOG_DEBUG, "Message transaction id is %d\n", *transactionid);

    y = ben_dict_get_by_str(message->data, "y");
	if (!y) {
		ks_log(KS_LOG_DEBUG, "Message missing required key 'y'\n");
		goto failure;
	}

	yv = ben_str_val(y);
	yv_len = ben_str_len(y);
	if (yv_len >= KS_DHT_MESSAGE_TYPE_MAX_SIZE) {
		ks_log(KS_LOG_DEBUG, "Message 'y' value has an unexpectedly large size of %d\n", yv_len);
		goto failure;
	}

	memcpy(message->type, yv, yv_len);
	message->type[yv_len] = '\0';
	ks_log(KS_LOG_DEBUG, "Message type is '%s'\n", message->type);

	return KS_STATUS_SUCCESS;

 failure:
	ks_dht_message_deinit(message);
	return KS_STATUS_FAIL;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_query(ks_dht_message_t *message,
											 uint32_t transactionid,
											 const char *query,
											 struct bencode **args)
{
	struct bencode *a;
	uint32_t tid;

	ks_assert(message);
	ks_assert(query);

	tid = htonl(transactionid);
	
    ben_dict_set(message->data, ben_blob("t", 1), ben_blob((uint8_t *)&tid, sizeof(uint32_t)));
	ben_dict_set(message->data, ben_blob("y", 1), ben_blob("q", 1));
	ben_dict_set(message->data, ben_blob("q", 1), ben_blob(query, strlen(query)));

	// @note r joins message->data and will be freed with it
	a = ben_dict();
	ben_dict_set(message->data, ben_blob("a", 1), a);

	if (args) {
		*args = a;
	}

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_response(ks_dht_message_t *message,
												uint8_t *transactionid,
												ks_size_t transactionid_length,
												struct bencode **args)
{
	struct bencode *r;
	
	ks_assert(message);
	ks_assert(transactionid);
	
    ben_dict_set(message->data, ben_blob("t", 1), ben_blob(transactionid, transactionid_length));
	ben_dict_set(message->data, ben_blob("y", 1), ben_blob("r", 1));
	
	// @note r joins message->data and will be freed with it
	r = ben_dict();
	ben_dict_set(message->data, ben_blob("r", 1), r);

	if (args) {
		*args = r;
	}

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht_message_error(ks_dht_message_t *message,
											 uint8_t *transactionid,
											 ks_size_t transactionid_length,
											 struct bencode **args)
{
	struct bencode *e;
	
	ks_assert(message);
	ks_assert(transactionid);
	
    ben_dict_set(message->data, ben_blob("t", 1), ben_blob(transactionid, transactionid_length));
	ben_dict_set(message->data, ben_blob("y", 1), ben_blob("e", 1));
	
	// @note r joins message->data and will be freed with it
	e = ben_list();
	ben_dict_set(message->data, ben_blob("e", 1), e);

	if (args) {
		*args = e;
	}

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