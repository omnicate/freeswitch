/*
 * FreeWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2014-2015, Pushkar Singh <psingh@sangoma.com>
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
 * The Initial Developer of the Original Code is Pushkar Singh <psingh@sangoma.com>
 * Portions created by the Initial Developer are Copyright (C) the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 * Pushkar Singh <psingh@sangoma.com>
 *
 * tdm_transparent.c -- FreeTDM Part of Endpoint Module to handle Transparent E1 functionality.
 *
 */

/******************************************************************************
 * File Name        : tdm_transparent.c
 *
 * File Description : This file is a part of FreeTDM Endpoint module i.e.
 * 		      mod_freetdm and it contains the fucntionality for
 * 		      tranparent E1 i.e. receiving of data on channel cnfigured
 * 		      as transparent and send it to particular UDP sockect as
 * 		      configured and vice versa.
 *
 ******************************************************************************/

/* INCLUDE ********************************************************************/
#include "mod_freetdm.h"

/* DEFINES ********************************************************************/
#define MAX_MSG_SIZE 4096
#define ftdm_log_tchan(fchan, level, format, ...) ftdm_log(level, "[s%dc%d][%d:%d] " format, ftdm_channel_get_span_id(fchan), ftdm_channel_get_id(fchan), ftdm_channel_get_ph_span_id(fchan), ftdm_channel_get_ph_id(fchan), __VA_ARGS__)
#define ftdm_log_tchan_msg(fchan, level, msg) ftdm_log(level, "[s%dc%d][%d:%d] " msg, ftdm_channel_get_span_id(fchan), ftdm_channel_get_span_id(fchan), ftdm_channel_get_ph_span_id(fchan), ftdm_channel_get_ph_id(fchan))


/* STRUCTURES *****************************************************************/
typedef struct {
	switch_sockaddr_t *local_sock_addr;
	switch_sockaddr_t *remote_sock_addr;
	switch_socket_t *socket;
	switch_pollfd_t *sock_pollfd;
	int socket_fd;
} tchan_socket_t;

typedef struct {
	ftdm_channel_t *ftdmchan;
	uint32_t chan_id;
	char remote_ip[25];
	int remote_port;
	char local_ip[25];
	int local_port;
	tchan_socket_t tchan_sock;
	switch_memory_pool_t *pool;
} tchan_info_t;

typedef struct {
	const char *remote_ip;
	const char *remote_port;
	const char *local_ip;
	const char *local_port;
} tchan_parse_info_t;

static struct {
	int chunk_size;
	ftdm_bool_t echo_cancel;
	ftdm_bool_t tchan_cfg_status;
} tchan_gen_cfg;

typedef enum tchan_hw_link_status {
	TCHAN_HW_LINK_DISCONNECTED = 0,
	TCHAN_HW_LINK_CONNECTED
} tchan_hw_link_status_t;

/* GLOBALS ********************************************************************/
/* Contain info of all transparent channels configured */
static switch_hash_t *tchan_list;
static int32_t tchan_span_threads;
static switch_memory_pool_t *global_pool;

/* FUNCTIONS ******************************************************************/
ftdm_status_t tchan_get_sock_status(tchan_info_t *chan_info);
void tchan_destroy(uint32_t span_id);

/******************************************************************************/
/*
 * Desc: Traverse through transparent channels hash list and check if the socket
 *       for same local IP+PORT combination is already being created then no
 *       need to create socket again else create socket as each transparent
 *       channel must have unique IP+PORT combination
 * Ret : FTDM_SUCCESS if socket is created successfully else FTDM_FAIL
 */
static ftdm_status_t tchan_create_udp_sock(tchan_info_t *tchan_info)
{
	switch_socket_t *tmp_socket = NULL;
	ftdm_status_t ret = FTDM_FAIL;

	switch_assert(tchan_info->ftdmchan != NULL);

	if (!tchan_info->pool) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "No pool allocated!\n");
		goto done;
	}

	/* Get local sock addr info */
	if (switch_sockaddr_info_get(&tchan_info->tchan_sock.local_sock_addr, tchan_info->local_ip, SWITCH_INET, tchan_info->local_port, 0, tchan_info->pool) != SWITCH_STATUS_SUCCESS) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Socket Error! Failed to obtain local socket address information.\n");
		goto done;
	}

	/* Get Remote sock addr info */
	if (switch_sockaddr_info_get(&tchan_info->tchan_sock.remote_sock_addr, tchan_info->remote_ip, SWITCH_INET, tchan_info->remote_port, 0, tchan_info->pool) != SWITCH_STATUS_SUCCESS) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Socket Error! Failed to obtain remote socket address information\n");
		goto done;
	}

	/* Create Socket for Local IP:PORT */
	if (switch_socket_create(&tchan_info->tchan_sock.socket, switch_sockaddr_get_family(tchan_info->tchan_sock.local_sock_addr), SOCK_DGRAM, SWITCH_PROTO_UDP, tchan_info->pool)
			!= SWITCH_STATUS_SUCCESS) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Socket Error. Can not create socket!\n");
		goto done;
	}

	/* set socket as non block socket */
	switch_socket_opt_set(tchan_info->tchan_sock.socket, SWITCH_SO_NONBLOCK, TRUE);

	tmp_socket = tchan_info->tchan_sock.socket;
	if ((ret = tchan_get_sock_status(tchan_info)) == FTDM_SUCCESS) {
		if (switch_socket_bind(tchan_info->tchan_sock.socket, tchan_info->tchan_sock.local_sock_addr) != SWITCH_STATUS_SUCCESS) {
			ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Socket Error. Bind Failure! Bind Error: %s\n", strerror(errno));
			switch_socket_shutdown(tchan_info->tchan_sock.socket, SWITCH_SHUTDOWN_READWRITE);
			switch_socket_close(tchan_info->tchan_sock.socket);
			tchan_info->tchan_sock.socket = NULL;
			ret = FTDM_FAIL;
			goto done;
		}
	} else {
		if (!(tmp_socket == tchan_info->tchan_sock.socket)) {
			switch_socket_shutdown(tmp_socket, SWITCH_SHUTDOWN_READWRITE);
			switch_socket_close(tmp_socket);
			tmp_socket = NULL;
			tchan_info->tchan_sock.socket = NULL;
		}
		goto done;
	}

done:
	if (ret == FTDM_SUCCESS) {
		ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_INFO, "Socket created successfully for ip:port %s:%d\n",
			           tchan_info->local_ip, tchan_info->local_port);
	} else {
		ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Failed to Create Socket for ip:port %s:%d! Socket Error: %s\n",
			           tchan_info->local_ip, tchan_info->local_port, strerror(errno));
	}

	return ret;
}

/******************************************************************************/
/*
 * Desc: Create insert and maintain channel hash list if not
 * 	 already created for the span passed and store
 * 	 related info into hash list
 * Ret : FTDM_SUCCESS inserted successfull else FTDM_FAIL
 */
static ftdm_status_t tchan_insert_sock_list(ftdm_span_t *span, tchan_info_t *tchan_info)
{
	ftdm_status_t status = FTDM_FAIL;
	char hash_key[10] = { 0 };
	char *trans_val = NULL;
	uint32_t span_id = 0;
	int sock_fd = 0;

	switch_assert(span != NULL);

	sock_fd = switch_socket_fd_get(tchan_info->tchan_sock.sock_pollfd->desc.s);
	if (sock_fd == 0) {
		ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Invalid socket fd %d found!\n", sock_fd);
		goto done;
	}

	tchan_info->tchan_sock.socket_fd = sock_fd;
	span_id = ftdm_span_get_id(span);

	if (SPAN_CONFIG[span_id].sock_list == NULL) {
		if (!global_pool) {
			ftdm_log(FTDM_LOG_ERROR, "Gloabl pool is not assigned yet\n");
			goto done;
		}

		switch_core_hash_init(&SPAN_CONFIG[span_id].sock_list, global_pool);

		if (!SPAN_CONFIG[span_id].sock_list) {
			ftdm_log(FTDM_LOG_ERROR, "Failed to create span %d socket hash list\n", span_id);
			goto done;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Span %d socket hash list successfully created\n", span_id);
		}
	}

	/* check if same socket fd for span passed is already present in socket hash list */
	snprintf(hash_key, sizeof(hash_key), "%d", sock_fd);
	if (switch_core_hash_find(SPAN_CONFIG[span_id].sock_list, (void *)hash_key)) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Invalid Configuration. Trying to add same socket!\n");
		goto done;
	}

	if (!tchan_info->pool) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "No pool allocated!\n");
		goto done;
	}

	/* Inserting socket info into socket hash list */
	trans_val = switch_core_strdup(tchan_info->pool, hash_key);
	if (switch_core_hash_insert(SPAN_CONFIG[span_id].sock_list, (void *)trans_val, tchan_info) != SWITCH_STATUS_SUCCESS) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Hashtable Insert Failure: Unable to insert socket info\n");
		goto done;
	}
	status = FTDM_SUCCESS;

done:
	return status;
}

/******************************************************************************/
/*
 * Desc: Insert & maintain transparent channels to transparent channel hash
 * 	 list and store its parameters in hash list as configured
 * Ret : FTDM_SUCCESS inserted successfull else FTDM_FAIL
 */
static ftdm_status_t tchan_insert_hash_list(ftdm_span_t *span, uint32_t chan_id, tchan_parse_info_t *parse_info)
{
	tchan_info_t *tchan_info = NULL;
	ftdm_channel_t *ftdmchan = NULL;
	ftdm_status_t status = FTDM_FAIL;
	char hash_key[25] = { 0 };
	uint16_t rtp_start_val = 0;
	uint16_t rtp_end_val = 0;
	char *trans_val = NULL;
	uint32_t span_id = 0;

	switch_assert(span != NULL);
	span_id = ftdm_span_get_id(span);
	if (!chan_id) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid channel Id %d:%d to be inserted in hash list", span_id, chan_id);
		return status;
	}

	/* check if same transparent channel for span passed is already present in channel hash list */
	snprintf(hash_key, sizeof(hash_key), "s%dc%d", span_id, chan_id);
	if (switch_core_hash_find(tchan_list, (void *)hash_key)) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid Configuration. Trying to add same channel %d:%d into transparent channel list ", span_id, chan_id);
		return status;
	}

	/* Add transparent channel information per span in to hash list if channel is not present in hash list */
	tchan_info = calloc(1, sizeof(*tchan_info));
	if (!tchan_info) {
		ftdm_log(FTDM_LOG_ERROR, "[s%dc%d] [%d:%d] Memory allocation failed!\n", span_id, chan_id, span_id, chan_id);
		return status;
	}

	if (!tchan_info->pool) {
		ftdm_log(FTDM_LOG_INFO, "[s%dc%d] [%d:%d] No pool allocated. Thus, allocating memory pool.\n", span_id, chan_id, span_id, chan_id);
	}

	/* Create memory pool for each transparent channel */
	if (switch_core_new_memory_pool(&tchan_info->pool) != SWITCH_STATUS_SUCCESS) {
		ftdm_log(FTDM_LOG_ERROR, "[s%dc%d] [%d:%d] Memory allocation failed. No pool available\n", span_id, chan_id, span_id, chan_id);
		goto done;
	}

	/* Get transparent channel structure for span that needs to be inserted in transparent channels
	 * hash list */
	ftdmchan = ftdm_span_get_channel_ph(span, chan_id);
	if (!ftdmchan) {
		ftdm_log(FTDM_LOG_ERROR, "Tranparent channel %d:%d not found!\n", span_id, chan_id);
		goto done;
	}

	tchan_info->ftdmchan = ftdmchan;
	tchan_info->chan_id = chan_id;

	rtp_start_val = switch_core_get_rtp_port_range_start_port();
	rtp_end_val = switch_core_get_rtp_port_range_end_port();

	if (parse_info->remote_ip) {
		ftdm_set_string(tchan_info->remote_ip, parse_info->remote_ip);
	}

	if (parse_info->remote_port) {
		tchan_info->remote_port = atoi(parse_info->remote_port);
		if (tchan_info->remote_port <= 0) {
			ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Invalid Remote UDP port: %d!\n", tchan_info->remote_port);
			goto done;
		}
	}

	if (parse_info->local_ip) {
		ftdm_set_string(tchan_info->local_ip, parse_info->local_ip);
	}

	if (parse_info->local_port) {
		tchan_info->local_port = atoi(parse_info->local_port);
		if (tchan_info->local_port <= 0) {
			ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Invalid Local UDP port: %d!\n", tchan_info->local_port);
			goto done;
		} else if ((tchan_info->local_port >= rtp_start_val) && (tchan_info->local_port <= rtp_end_val)) {
			ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Local UDP port: %d is within RTP port range i.e. from %d to %d!\n",
				       tchan_info->local_port, rtp_start_val, rtp_end_val);
			goto done;
		}
	}

	/* Traverse through transparent channels hash list and check if socket is already being
	 * created for the same local ip+port combination then no need to create socket again
	 * and return failure as each transparent channel must have unique IP+PORT combination */
	status = tchan_create_udp_sock(tchan_info);

	/* if socket is created successfully the create socket poll fd & poll set to poll on */
	if (status == FTDM_SUCCESS) {
		if (!SPAN_CONFIG[span_id].tchan_gen.pool) {
			/* Create memory pool for pollset per span basis */
			if (switch_core_new_memory_pool(&SPAN_CONFIG[span_id].tchan_gen.pool) != SWITCH_STATUS_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "Memory allocation failed. No memory pool available for pollset!\n");
				status = FTDM_FAIL;
				goto done;
			}

			/* create poll set in order to add pollfd's */
			if (switch_pollset_create(&SPAN_CONFIG[span_id].tchan_gen.pollset, FTDM_MAX_CHANNELS_SPAN, SPAN_CONFIG[span_id].tchan_gen.pool, 0) != SWITCH_STATUS_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "Poll Set Create failed!\n");
				status = FTDM_FAIL;
				goto done;
			}
		}

		/* creating socket poll fd to poll on */
		switch_socket_create_pollfd(&tchan_info->tchan_sock.sock_pollfd, tchan_info->tchan_sock.socket, SWITCH_POLLIN | SWITCH_POLLERR, NULL, tchan_info->pool);

		/* Add socket poll fd created to pollset */
		if (switch_pollset_add(SPAN_CONFIG[span_id].tchan_gen.pollset, tchan_info->tchan_sock.sock_pollfd) != SWITCH_STATUS_SUCCESS) {
			ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Unable to add pollfd created to unviversal pollset!\n");
			status = FTDM_FAIL;
			goto done;
		}
	} else if (status == FTDM_FAIL) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Invalid UDP Socket: Unable to insert transparent channel info!\n");
		goto done;
	}

	/* Inserting channel info into transparent channel hash list */
	trans_val = switch_core_strdup(tchan_info->pool, hash_key);
	if (switch_core_hash_insert(tchan_list, (void *)trans_val, tchan_info) != SWITCH_STATUS_SUCCESS) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Hashtable Insert Failure: Unable to insert transparent channel info\n");
		status = FTDM_FAIL;
		goto done;
	}

	if ((status = tchan_insert_sock_list(span, tchan_info)) != FTDM_SUCCESS) {
		ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Failed to insert socket %d in to socket hash list per span basis!\n");
		if (tchan_info->pool) {
			switch_core_destroy_memory_pool(&tchan_info->pool);
		}
		return status;
	}

	ftdm_log_tchan_msg(tchan_info->ftdmchan, FTDM_LOG_DEBUG, "Transparent channel successfully inserted in hash list\n");
	status = FTDM_SUCCESS;

done:
	if (status == FTDM_FAIL) {
		free(tchan_info);
		if (tchan_info->pool) {
			switch_core_destroy_memory_pool(&tchan_info->pool);
		}
	}

	return status;
}

/******************************************************************************/
/*
 * Desc: Get socket status i.e. whether socket for same IP:PORT is already
 * 	 created
 * Ret : FTDM_SUCCESS if socket is not already created with same IP:PORT
 * 	 combination else FTDM_FAIL
 */
ftdm_status_t tchan_get_sock_status(tchan_info_t *chan_info)
{
	switch_hash_index_t *idx = NULL;
	tchan_info_t *tchan_info = NULL;
	ftdm_status_t ret = FTDM_SUCCESS;
	const void *key = NULL;
	uint32_t span_id = 0;
	void *val = NULL;

	switch_assert(chan_info->ftdmchan != NULL);

	span_id = ftdm_channel_get_span_id(chan_info->ftdmchan);
	if (!SPAN_CONFIG[span_id].sock_list) {
		ftdm_log_tchan_msg(chan_info->ftdmchan, FTDM_LOG_INFO, "This is first entry thus socket hash list is not yet created!\n");
		return ret;
	}

	/* Iterate through all socket present in span socket hash list and if socket is already present for
	 * local ip:port & remote ip:port then assign the same to the new transparent channel that has to be
	 * inserted in the hash list */
	for (idx = switch_hash_first(NULL, SPAN_CONFIG[span_id].sock_list); idx; idx = switch_hash_next(idx)) {
		switch_hash_this(idx, &key, NULL, &val);
		if (!key || !val) {
			break;
		}

		tchan_info = (tchan_info_t*)val;

		if (!(strcmp(tchan_info->local_ip, chan_info->local_ip)) && (tchan_info->local_port == chan_info->local_port)) {
			ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_DEBUG, "Found channel with same local IP:PORT %s:%d\n",
					tchan_info->local_ip, tchan_info->local_port);

			chan_info->tchan_sock.socket = tchan_info->tchan_sock.socket;
			ret = FTDM_FAIL;
			break;
		}
	}

	if (ret == FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_INFO, "No transparent channel found in hash list with local IP:PORT %s:%d\n",
			 chan_info->local_ip, chan_info->local_port);
	}

	return ret;
}

/******************************************************************************/
/*
 * Desc: Get transparent channel information from the hash list depending upon
 * 	 the ftdmchan_ftdmspan/socket fd passed to it
 * Ret : FTDM_SUCCESS if channel is present in the hash list else FTDM_FAIL
 */
static tchan_info_t *tchan_get_info(ftdm_channel_t *ftdmchan, ftdm_span_t *ftdmspan, const switch_pollfd_t *sock_fd)
{
	tchan_info_t *tchan_info = NULL;
	char hash_key[25] = { 0 };
	uint32_t span_id = 0;
	uint32_t chan_id = 0;
	int socket_fd = 0;

	if ((!ftdmchan) && (!sock_fd)) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid channel & socket poll fd passed!\n");
		return NULL;
	}

	if (ftdmchan) {
		span_id = ftdm_channel_get_span_id(ftdmchan);
		chan_id = ftdm_channel_get_ph_id(ftdmchan);

		snprintf(hash_key, sizeof(hash_key), "s%dc%d", span_id, chan_id);
		tchan_info = switch_core_hash_find(tchan_list, (void *)hash_key);
	} else if ((sock_fd) && (ftdmspan)) {
		socket_fd = switch_socket_fd_get(sock_fd->desc.s);
		span_id = ftdm_span_get_id(ftdmspan);

		if (!socket_fd) {
			ftdm_log(FTDM_LOG_ERROR, "Invalid socket fd %d found!\n", socket_fd);
			return NULL;
		}

		snprintf(hash_key, sizeof(hash_key), "%d", socket_fd);
		tchan_info = switch_core_hash_find(SPAN_CONFIG[span_id].sock_list, (void *)hash_key);

	} else {
		ftdm_log(FTDM_LOG_ERROR, "Invalid channel & socket poll fd passed!\n");
		return NULL;
	}

	if (!tchan_info) {
		ftdm_log(FTDM_LOG_ERROR, "No transparent channel present in hash list with key %s!\n", hash_key);
	}

	return tchan_info;
}

/******************************************************************************/
/*
 * Desc: Get all transparent channels for span on which poll need to be done.
 * Ret : Number of transparent channel present in the hash list for the span
 * 	 passed
 */
static uint32_t tchan_get_span_chan_list(ftdm_span_t *ftdmspan, ftdm_channel_t *ftdmchan[])
{
	tchan_info_t *tchan_info = NULL;
	switch_hash_index_t *idx = NULL;
	const void *key = NULL;
	uint32_t index = 0;
	void *val = NULL;

	switch_assert(ftdmspan != NULL);
	for (idx = switch_hash_first(NULL, tchan_list); idx; idx = switch_hash_next(idx)) {
		switch_hash_this(idx, &key, NULL, &val);
		if (!key || !val) {
			break;
		}

		tchan_info = (tchan_info_t*)val;
		if (ftdm_channel_get_span_id(tchan_info->ftdmchan) == ftdm_span_get_id(ftdmspan)) {
			ftdmchan[index] = tchan_info->ftdmchan;
			index++;
		}
	}

	return index;
}

/******************************************************************************/
/*
 * Desc: Transparent channels message handler per span basis
 * 	 1) Check if any data/message is present on any of the transparent
 * 	    channels for span passed, if present then read message from that
 * 	    transparent channel and write it to its respective remote UDP
 * 	    socket (IP:PORT) as configured
 * 	 2) Check if any data/message is present on any of the local UDP socket
 * 	    (local IP:PORT) for tranparent channels, if present then read
 * 	    message present and write it to its respective transparent channel
 * 	    as configured
 * Ret : Exits Only when SPAN thread will stops
 */
static void *tchan_msg_handler(ftdm_thread_t * me, void *obj)
{
	ftdm_span_t *ftdmspan = (ftdm_span_t *) obj;
	/* poll realted variables */
	ftdm_channel_t *ftdmchan_poll[FTDM_MAX_CHANNELS_PHYSICAL_SPAN] = { 0 };
	ftdm_status_t poll_status = FTDM_FAIL;
	switch_status_t swtch_poll_status = SWITCH_STATUS_FALSE;
	const switch_pollfd_t *sock_fd = NULL;

	tchan_hw_link_status_t link_status = TCHAN_HW_LINK_DISCONNECTED;
	ftdm_bool_t flush_iostats = FTDM_FALSE;
	ftdm_status_t status = FTDM_FAIL;
	tchan_info_t *tchan_info = NULL;
	char data[MAX_MSG_SIZE] = { 0 };
	ftdm_wait_flag_t event = 0;
	ftdm_size_t msg_len = 0;
	char hbuf[2048] = { 0 };
	uint32_t num_chan = 0;
	int32_t num_sockfd = 0;
	uint32_t span_id = 0;
	int32_t waitms = 0;
	uint32_t idx = 0;
	int interval = 10;

	switch_assert(ftdmspan != NULL);

	span_id = ftdm_span_get_id(ftdmspan);

	/* Get all transparent channels for span for which thread is being created */
	/* Iterate through all hash list of transparent channels and prepare a list
	 * of channels to poll on */
	num_chan = tchan_get_span_chan_list(ftdmspan, ftdmchan_poll);
	if (num_chan == 0) {
		ftdm_log(FTDM_LOG_ERROR, "No Transparent channels found for span %d\n", span_id);
		goto done;
	} else {
		ftdm_log(FTDM_LOG_INFO, "%d transparent channels found for span %d\n", num_chan, span_id);
	}

	/* Open all transparent channels & apply global transparent channel configuration */
	for (idx = 0; idx < num_chan; idx++) {
		/* setting up echo cancellation mode per channel */
		if (tchan_gen_cfg.echo_cancel == FTDM_TRUE) {
			ftdm_channel_command(ftdmchan_poll[idx], FTDM_COMMAND_ENABLE_ECHOCANCEL, NULL);
		} else {
			ftdm_channel_command(ftdmchan_poll[idx], FTDM_COMMAND_DISABLE_ECHOCANCEL, NULL);
		}

		if (ftdm_channel_open_chan(ftdmchan_poll[idx]) != FTDM_SUCCESS) {
			ftdm_log(FTDM_LOG_ERROR, "Failed to Open transparent channel at index %d for span %d\n", idx, span_id);
			goto done;
		} else {
			ftdm_log_tchan_msg(ftdmchan_poll[idx], FTDM_LOG_INFO, "Transparent channel successfully Open\n");
			/* setting maximum chunk size that can be read */
			ftdm_channel_command(ftdmchan_poll[idx], FTDM_COMMAND_SET_INTERVAL, &interval);
		}
	}

	ftdm_log(FTDM_LOG_INFO, "Setting Chunk size %d and Echo Cancellation to %d!\n", interval, tchan_gen_cfg.echo_cancel);

	idx = 0;
	waitms = 100;
	while (ftdm_get_span_running_status(ftdmspan)) {
		ftdm_sleep(waitms);
	}

	while (ftdm_running() && !ftdm_get_span_running_status(ftdmspan)) {
		poll_status = FTDM_FAIL;
		status = FTDM_FAIL;

		/* check if the LINK is in connected state */
		ftdm_channel_command(ftdmchan_poll[0], FTDM_COMMAND_GET_LINK_STATUS, &link_status);

		/* In case if wanpipe gets disconnected then there is no need to poll through
		 * channels as HW LINK is not in connected state and thus polling to span
		 * channels will un-necessary add latency in polling UDP Socket and this could
		 * leads to stack up of large number of meesages in UDP socket receive queue
		 * which can lead to huge delay in transmitting messages */
		if ((link_status == TCHAN_HW_LINK_DISCONNECTED))  {
			if (flush_iostats == FTDM_FALSE) {
				ftdm_log(FTDM_LOG_WARNING, "Transparent channel link is disconnected. Thus, poll on UDP socket only!\n");
				flush_iostats = FTDM_TRUE;
			}
			goto udp_poll;
		}

		/* In case this flag is set this means that the HW LINK is UP now and therefore start polling
		 * through transparent channels in case but before that flush IO STATS for all transparent
		 * channels as any messages already present in TX queue may lead to add latency in
		 * transmitting message */
		if (flush_iostats == FTDM_TRUE) {
			for (idx = 0; idx < num_chan; idx++) {
				ftdm_channel_command(ftdmchan_poll[idx], FTDM_COMMAND_FLUSH_IOSTATS, NULL);
				ftdm_log_tchan_msg(ftdmchan_poll[idx], FTDM_LOG_DEBUG, "Flushing IOSTATS as link get back in connected\n");
			}
			flush_iostats = FTDM_FALSE;
		}

		event = FTDM_READ;
		/* poll on all transparent channels of the span. If any data is present on any
		 * channel then transmit it over respective udp socket as configured */
		poll_status = ftdm_channel_wait(ftdmchan_poll[0], &event, waitms);


		memset(hbuf, 0, sizeof(hbuf));
		switch (poll_status) {
			case FTDM_TIMEOUT:
				ftdm_log(FTDM_LOG_DEBUG, "Poll TIMEOUT received for span %d\n", span_id);
				break;

			case FTDM_BREAK:
				ftdm_log(FTDM_LOG_DEBUG, "Poll BREAK received for for span %d\n", span_id);
				break;

			case FTDM_SUCCESS:
				for (idx = 0; idx < num_chan; idx++) {
					if (event & FTDM_READ) {
						memset(data, 0, sizeof(data));
						msg_len = sizeof(data);
						if ((status = ftdm_channel_read(ftdmchan_poll[idx], data, &msg_len)) != FTDM_SUCCESS) {
							if (status == FTDM_FAIL) {
								ftdm_log_tchan_msg(ftdmchan_poll[idx], FTDM_LOG_ERROR, "Failed to read transparent channel!\n");
							} else {
								ftdm_log_tchan_msg(ftdmchan_poll[idx], FTDM_LOG_DEBUG, "Transparent channel read timeout!\n");
							}
							continue;
						}

						if (msg_len) {
							/* get that particular taransparent channel Infomation & then send
							 * data to its respective socket as configured */
							tchan_info = tchan_get_info(ftdmchan_poll[idx], ftdmspan, NULL);

							if (tchan_info) {
								/* write data received on particular span transparent channel to its respective UDP socket */
								if (switch_socket_sendto(tchan_info->tchan_sock.socket, tchan_info->tchan_sock.remote_sock_addr, 0, data, &msg_len) != SWITCH_STATUS_SUCCESS) {
									ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "Failed to send to UDP: %s\n", strerror(errno));
									break;
								} 
#if 0
								/* To Print Raw messages received on channel for debugging purpose */
								else {
									print_hex_bytes((void *)data, msg_len, hbuf, sizeof(hbuf));
									ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_DEBUG, "Channel Read %"FTDM_SIZE_FMT"\n%s\n", msg_len, hbuf);
									ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_INFO, "Sent successfully to send to UDP of len = %"FTDM_SIZE_FMT"\n", msg_len);
								}
#endif
							}

						}
					} else {
						ftdm_log(FTDM_LOG_INFO, "No data to read on span %d chan %d\n", span_id, ftdm_channel_get_id(ftdmchan_poll[idx]));
					}
				}

				break;

			default:
				ftdm_log(FTDM_LOG_ERROR, "Invalid status!\n");
				poll_status = FTDM_FAIL;
				break;
		}

udp_poll:
		memset(hbuf, 0, sizeof(hbuf));

		/* poll for certain time on all socket fds present  and check if any data is present then write the same on to the
		 * channel as configured */
		swtch_poll_status = switch_pollset_poll(SPAN_CONFIG[span_id].tchan_gen.pollset, waitms, &num_sockfd, &sock_fd);

		if (swtch_poll_status!= SWITCH_STATUS_SUCCESS && swtch_poll_status != SWITCH_STATUS_TIMEOUT) {
			ftdm_log(FTDM_LOG_ERROR, "Poll on UDP Socket failed!\n");
		} else if (swtch_poll_status == SWITCH_STATUS_TIMEOUT) {
			ftdm_log(FTDM_LOG_DEBUG, "Poll TIMEOUT on UDP Socket!\n");
		}

		for (idx = 0; idx < num_sockfd; idx++) {
			if (!(sock_fd[idx].rtnevents & SWITCH_POLLIN)) {
				continue;
			}
			msg_len = sizeof(data);
			memset(data, 0 , sizeof(data));
			tchan_info = NULL;

			tchan_info = tchan_get_info(NULL, ftdmspan, &sock_fd[idx]);

			if (tchan_info) {
				/* read data from UDP socket */
				if (switch_socket_recvfrom(tchan_info->tchan_sock.local_sock_addr, tchan_info->tchan_sock.socket, 0, data, &msg_len) != SWITCH_STATUS_SUCCESS) {
					ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "UDP Recv Error on index %d!\n", idx);
					continue;
				}

				if (msg_len > 0) {
					/* No need to write on respective transparent channel as HW LINK not
					 * in connected state */
					if (flush_iostats == FTDM_TRUE) {
						continue;
					}

#if 0
					/* To Print Raw messages received on UDP socket only for debugging purpose */
					ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_INFO, "UDP Data Recv of length %"FTDM_SIZE_FMT"\n", msg_len);
					print_hex_bytes((void *)data, msg_len, hbuf, sizeof(hbuf));
					ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_DEBUG, "UDP Data RECV: %"FTDM_SIZE_FMT"\n%s\n", msg_len, hbuf);
#endif

					/* write data received on respective span channel */
					if (ftdm_channel_write(tchan_info->ftdmchan, data, MAX_MSG_SIZE, &msg_len) != FTDM_SUCCESS) {
						ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "MSG of len %"FTDM_SIZE_FMT" failed to write\n",
								msg_len);
						continue;
					}
				} else if (!msg_len) {
					ftdm_log_tchan(tchan_info->ftdmchan, FTDM_LOG_ERROR, "UDP Data Recv of invalid length %"FTDM_SIZE_FMT" Error: %s\n", msg_len, strerror(errno));
				}
			}
		}
	}
done:
	tchan_span_threads--;

	/* If this is the last thread which is shutting down then destroy all the
	 * transparent related parameters explicitly stored in the memory */
	tchan_destroy(span_id);

	ftdm_log(FTDM_LOG_INFO, "Exiting span %d thread\n", span_id);
	return NULL;
}

/******************************************************************************/
/*
 * Desc: Configure transparent channels per span & maintain a hash list in
 * 	 order to store all the configuration for transparent channels per
 * 	 span as per configuration
 * Ret : FTDM_SUCCESS if successfully creates & maintain hash list for all
 * 	 transparent channels per span else FTDM_FAIL
 */
static ftdm_status_t tchan_gen_config(switch_xml_t general)
{
	switch_xml_t param;

	/* Parse all general configuration parameters value for transparent span */
	for (param = switch_xml_child(general, "param"); param; param = param->next) {
		char *var = (char *) switch_xml_attr_soft(param, "name");
		char *val = (char *) switch_xml_attr_soft(param, "value");

		if (!strcasecmp(var, "chunk-size")) {
			tchan_gen_cfg.chunk_size = atoi(val);
			if (!tchan_gen_cfg.chunk_size) {
				ftdm_log(FTDM_LOG_INFO, "Invalid chunck size %d value for transparent channels. Thus, setting it to default i.e. 10\n",
						var);
				tchan_gen_cfg.chunk_size = 10;
			}
		} else if (!strcasecmp(var, "echo-cancel")) {
			if (!strcasecmp(val, "yes")) {
				tchan_gen_cfg.echo_cancel = FTDM_TRUE;
			} else {
				tchan_gen_cfg.echo_cancel = FTDM_FALSE;
			}
		} else {
			ftdm_log(FTDM_LOG_ERROR, "Invalid configuration parameter %s present in transparent channel configuration!\n", var);
			return FTDM_FAIL;
		}
	}
	ftdm_log(FTDM_LOG_INFO, "Configured chunk size as %d and echo-cancle as %d for all transparent channels\n",
		 tchan_gen_cfg.chunk_size, tchan_gen_cfg.echo_cancel);

	ftdm_log(FTDM_LOG_INFO, "Transparent channels general configuration completed\n");
	
	return FTDM_SUCCESS;
}

/******************************************************************************/
/*
 * Desc: Configure transparent channels per span & maintain a hash list in
 * 	 order to store all configuration for transparent channels per span
 * 	 as per configuration
 * Ret : FTDM_SUCCESS if successfully creates & maintain hash list for all
 * 	 transparent channels per span else FTDM_FAIL
 */
static ftdm_status_t tchan_config(ftdm_span_t *span, switch_xml_t mychan)
{
	char *tchan = (char *) switch_xml_attr(mychan, "id");
	tchan_parse_info_t *parse_info = { 0 };
	switch_memory_pool_t *pool = NULL;
	ftdm_status_t ret = FTDM_FAIL;
	uint32_t chan_id = 0;
	switch_xml_t param;
	uint32_t idx = 0;
	uint32_t span_id = 0;

	switch_assert(span != NULL);

	span_id = ftdm_span_get_id(span);

	/* Check if transparent channel configuration is present in the freetdm.conf.xml */
	if (!tchan) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid Configuration for span %d. Transparent Channel Id not present in configuration file!\n", span_id);
		goto done;
	}

	while (tchan[idx] != '\0') {
		if ((tchan[idx] == '-') || (tchan[idx] == ',')) {
			ftdm_log(FTDM_LOG_ERROR, "Invalid Configuration for span %d. Multiple channels/Range of channels are not allowed!\n", span_id);
			goto done;
		}
		idx++;
		if (idx > 1) {
			break;
		}
	}

	chan_id = atoi(tchan);
	if (!chan_id) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid Configuration. Invalid Transparent Channel %d:%d!\n", span_id, chan_id);
		goto done;
	}

	switch_core_new_memory_pool(&pool);

	/* Create Transparent channel hash list if it is not present at all*/
	if (tchan_list == NULL) {
		switch_core_new_memory_pool(&global_pool);
		switch_core_hash_init(&tchan_list, global_pool);

		if (!tchan_list) {
			ftdm_log(FTDM_LOG_ERROR, "Failed to create transparent channel hash list\n");
			goto done;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Tansparent channel hash list successfully created\n");
		}
	}

	/* Store all inforamtion for transparent channel as present in configuration file */
	parse_info = switch_core_alloc(pool, sizeof(*parse_info));
	if (!parse_info) {
		ftdm_log(FTDM_LOG_ERROR, "Memory allocation failed for parse info for channel %d:%d!\n", span_id, chan_id);
		goto done;
	}

	/* Parse all parameters values for transparent span */
	for (param = switch_xml_child(mychan, "param"); param; param = param->next) {
		char *var = (char *) switch_xml_attr_soft(param, "name");
		char *val = (char *) switch_xml_attr_soft(param, "value");

		if (!strcasecmp(var, "remote-ip")) {
			parse_info->remote_ip = val;
		} else if (!strcasecmp(var, "remote-port")) {
			parse_info->remote_port = val;
		} else if (!strcasecmp(var, "local-ip")) {
			parse_info->local_ip = val;
		} else if (!strcasecmp(var, "local-port")) {
			parse_info->local_port = val;
		} else {
			ftdm_log(FTDM_LOG_ERROR, "Invalid configuration parameter %s present for channel %d!\n", var, span_id, chan_id);
			goto done;
		}
	}

	if ((!parse_info->remote_ip) || (!parse_info->remote_port) || (!parse_info->local_ip) || (!parse_info->local_port)) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid Configuration. Missing mandatory parameter for channel %d:%d\n", span_id, chan_id);
		goto done;
	}

	/* Insert all transparent channels into transparent channel hash list
	 * if already not present */
	ret = tchan_insert_hash_list(span, chan_id, parse_info);

done:
	if (pool) {
		switch_core_destroy_memory_pool(&pool);
	}

	return ret;
}

/******************************************************************************/
/* Desc: Creates a tranparent span thread once SPAN is configured and started
 * Ret : FTDM_SUCCESS if thread is successfully created else FTDM_FAIL
 */
static ftdm_status_t tchan_create_span_thread(ftdm_span_t *ftdmspan)
{
	switch_assert(ftdmspan != NULL);

	/* create thread in order to handle messages received on Transparent channel i.e. TDM */
	if (ftdm_thread_create_detached(tchan_msg_handler, ftdmspan) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_ERROR, "Failed to create transparent channel message handler thread for span %d\n", ftdm_span_get_id(ftdmspan));
		return FTDM_FAIL;
	}

	tchan_span_threads++;
	return FTDM_SUCCESS;
}

/******************************************************************************/
/*
 * Desc: Parse Transparent span configuration as present in freetdm.conf.xml
 * 	 and maintain a hash list in order to store each transparent channel
 * 	 configuration per span basis
 * Ret : FTDM_SUCCESS if configuration parsing is successfull & FTDM_FAIL
 */
ftdm_status_t parse_transparent_spans(switch_xml_t cfg, switch_xml_t spans)
{
	switch_xml_t myspan, mychan, general;
	ftdm_status_t ret = FTDM_FAIL;
	ftdm_span_t *span = NULL;
	uint32_t span_id = 0;

	/* default general configuration for all transparent channels */
	tchan_gen_cfg.chunk_size = 10;
	tchan_gen_cfg.echo_cancel = FTDM_FALSE;
	/* tchan_cfg_status gives the status whether transparent channel configuration is successfull or not */
	tchan_gen_cfg.tchan_cfg_status = FTDM_FALSE;

	/* parse general configuration for all transparent channels of all transparent spans
	 * this is an optional configuration */
	for (general = switch_xml_child(spans, "span_gen"); general; general = general->next) {
		if (FTDM_FAIL == tchan_gen_config(general)) {
			goto done;
		}
	}

	/* Go throgh all span in which transparent channels are configured as present in
	 * configuration file */
	for (myspan = switch_xml_child(spans, "span"); myspan; myspan = myspan->next) {
		ftdm_status_t zstatus = FTDM_FAIL;
		char *id = (char *) switch_xml_attr(myspan, "id");
		char *name = (char *) switch_xml_attr(myspan, "name");

		ret = FTDM_FAIL;
		if (!name && !id) {
			ftdm_log(FTDM_LOG_ERROR, "Transparent span missing required attribute 'id' or 'name'.\n");
			goto done;
		}

		if (name) {
			zstatus = ftdm_span_find_by_name(name, &span);
		} else {
			if (switch_is_number(id)) {
				span_id = atoi(id);
				zstatus = ftdm_span_find(span_id, &span);
			}

			if (zstatus != FTDM_SUCCESS) {
				zstatus = ftdm_span_find_by_name(id, &span);
			}
		}
		if (zstatus != FTDM_SUCCESS) {
			ftdm_log(FTDM_LOG_ERROR, "Invalid configuration. Transparent span with id:%s name:%s not present\n", switch_str_nil(id), switch_str_nil(name));
			goto done;
		}

		if (!span_id) {
			span_id = ftdm_span_get_id(span);
		}

		/* Go through all transparent channels that needs to be configured & store their respective parameters values */
		for (mychan = switch_xml_child(myspan, "channel"); mychan; mychan = mychan->next) {
			if (FTDM_FAIL == tchan_config(span, mychan)) {
				goto done;
			}
			ret = FTDM_SUCCESS;
		}

		/* it is possible that in xml file channel parameter is not present in such a case that is not
		 * a valid transparent channel configuration */
		if (ret != FTDM_FAIL) {
			ret = tchan_create_span_thread(span);
		} else {
			ftdm_log(FTDM_LOG_ERROR, "Invalid configuration. No transparent channels present for span:%s!\n", switch_str_nil(name));
		}

		if (ret != FTDM_SUCCESS) {
			goto done;
		}

		ret = FTDM_SUCCESS;
	}

done:
	/* destroy transparent channel hash list */
	if (ret == FTDM_FAIL) {
		if (tchan_list) {
			tchan_destroy(span_id);
		}
	} else {
		tchan_gen_cfg.tchan_cfg_status = FTDM_TRUE;
	}

	return ret;
}

/******************************************************************************/
/*
 * Desc: Start transparent span if not started yet
 * Ret : FTDM_SUCCESS if thread starts successfully else FTDM_FAIL
 */
ftdm_status_t transparent_span_start(switch_xml_t cfg, switch_xml_t spans)
{
	switch_xml_t myspan;
	ftdm_status_t ret = FTDM_FAIL;
	ftdm_span_t *span = NULL;
	uint32_t span_id = 0;

	/* as transparent channel configuration is not successfull thus no need to
	 * start transparent span */
	if (tchan_gen_cfg.tchan_cfg_status == FTDM_FALSE) {
		goto done;
	}

	/* Go throgh all span in which transparent channels are configured as present in
	 * configuration file */
	for (myspan = switch_xml_child(spans, "span"); myspan; myspan = myspan->next) {
		ftdm_status_t zstatus = FTDM_FAIL;
		char *id = (char *) switch_xml_attr(myspan, "id");
		char *name = (char *) switch_xml_attr(myspan, "name");

		ret = FTDM_FAIL;
		if (!name && !id) {
			ftdm_log(FTDM_LOG_ERROR, "Transparent span missing required attribute 'id' or 'name'.\n");
			goto done;
		}

		if (name) {
			zstatus = ftdm_span_find_by_name(name, &span);
		} else {
			if (switch_is_number(id)) {
				span_id = atoi(id);
				zstatus = ftdm_span_find(span_id, &span);
			}

			if (zstatus != FTDM_SUCCESS) {
				zstatus = ftdm_span_find_by_name(id, &span);
			}
		}
		if (zstatus != FTDM_SUCCESS) {
			ftdm_log(FTDM_LOG_ERROR, "Invalid configuration. Transparent span with id:%s name:%s not present\n", switch_str_nil(id), switch_str_nil(name));
			goto done;
		}

		if (!span_id) {
			span_id = ftdm_span_get_id(span);
		}

		/* If span is not already started then then start it */
		if (ftdm_get_span_running_status(span)) {
			SPAN_CONFIG[span_id].span = span;
			ftdm_log(FTDM_LOG_DEBUG, "Configured transparent FreeTDM span %d\n", span_id);

			if(FTDM_SUCCESS != ftdm_span_start(span)){
				ftdm_log(FTDM_LOG_ERROR, "Error Starting transparent FreeTDM span %d\n", span_id);
				goto done;
			}
		}
	}

	ret = FTDM_SUCCESS;

done:
	/* destroy transparent channel hash list */
	if ((ret == FTDM_FAIL) && (tchan_list)) {
		tchan_destroy(span_id);
	}

	return ret;
}

/******************************************************************************/
/* Desc: Destroy transparent channels hash list once all threads exists
 * Ret : void
 */
static void tchan_destroy_hash_list(uint32_t span_id)
{
	tchan_info_t *tchan_info = NULL;
	switch_hash_index_t *idx = NULL;
	char hash_key[25] = { 0 };
	const void *key = NULL;
	uint32_t chan_id = 0;
	void *val = NULL;
	int sock_fd = 0;

	if (!span_id) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid span_id %d!\n", span_id);
		return;
	}

	for (idx = switch_hash_first(NULL, SPAN_CONFIG[span_id].sock_list); idx; idx = switch_hash_next(idx)) {
		switch_hash_this(idx, &key, NULL, &val);
		if (!key || !val) {
			break;
		}

		tchan_info = (tchan_info_t*)val;
		if (!tchan_info->ftdmchan) {
			ftdm_log(FTDM_LOG_ERROR, "Invalid entry found at index %d in socket hash list!\n", idx);
			break;
		}

		chan_id = tchan_info->chan_id;
		if (!chan_id) {
			ftdm_log(FTDM_LOG_ERROR, "[s%dc%d] [%d:%d] Invalid channel ID %d found for span %d!\n", span_id, chan_id, span_id, chan_id);
			break;
		}

		sock_fd = tchan_info->tchan_sock.socket_fd;
		if (!sock_fd) {
			ftdm_log(FTDM_LOG_ERROR, "[s%dc%d] [%d:%d] Invalid socket fd %d found!\n",
				       span_id, chan_id, sock_fd, span_id, chan_id);
			break;
		}

		/* deleting entry from socket hash list */
		memset(hash_key, 0 , sizeof(hash_key));
		snprintf(hash_key, sizeof(hash_key), "%d", sock_fd);
		switch_core_hash_delete(SPAN_CONFIG[span_id].sock_list, (void *)hash_key);
		ftdm_log(FTDM_LOG_INFO, "[s%dc%d] [%d:%d] Entry removed successfully from Socket hash list\n", span_id, chan_id, span_id, chan_id);

		/* deleting entry from socket hash list */
		memset(hash_key, 0 , sizeof(hash_key));
		snprintf(hash_key, sizeof(hash_key), "s%dc%d", span_id, chan_id);
		switch_core_hash_delete(tchan_list, (void *)hash_key);
		ftdm_log(FTDM_LOG_INFO, "[s%dc%d] [%d:%d] Entry removed successfully from Transparent Channel hash list\n", span_id, chan_id, span_id, chan_id);

		if (tchan_info->tchan_sock.socket) {
			/* close socket created */
			switch_socket_shutdown(tchan_info->tchan_sock.socket, SWITCH_SHUTDOWN_READWRITE);
			switch_socket_close(tchan_info->tchan_sock.socket);

			tchan_info->tchan_sock.socket = NULL;
			tchan_info->tchan_sock.local_sock_addr = NULL;
			tchan_info->tchan_sock.remote_sock_addr = NULL;

			ftdm_log(FTDM_LOG_INFO, "Closing Socket Connection & destroying memory pool!\n");
		}

		/* destroy socket memory pool */
		switch_core_destroy_memory_pool(&tchan_info->pool);
		if (tchan_info) {
			free(tchan_info);
		}
	}

	/* delete the hash table */
	if (SPAN_CONFIG[span_id].sock_list) {
		switch_core_hash_destroy(&SPAN_CONFIG[span_id].sock_list);
	}
	SPAN_CONFIG[span_id].sock_list = NULL;
	if (SPAN_CONFIG[span_id].tchan_gen.pool) {
		/* destroying memory pool cretated for pollset */
		switch_core_destroy_memory_pool(&SPAN_CONFIG[span_id].tchan_gen.pool);
	}

	ftdm_log(FTDM_LOG_INFO, "Span %d socket hash list successfully destroyed!\n", span_id);
}

/******************************************************************************/
/* Desc: Destroy transparent channels hash list and all the pool memory as
 * 	 allocated for parameters related to transparent channels as well as
 * 	 mutex allocated
 * Ret : void
 */
void tchan_destroy(uint32_t span_id)
{
	if (SPAN_CONFIG[span_id].sock_list) {
		tchan_destroy_hash_list(span_id);
	}

	if ((!tchan_span_threads) && (tchan_list)) {
		/* delete channel hash table */
		switch_core_hash_destroy(&tchan_list);
	}

	if (global_pool) {
		switch_core_destroy_memory_pool(&global_pool);
	}
}
/******************************************************************************/
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
/******************************************************************************/
