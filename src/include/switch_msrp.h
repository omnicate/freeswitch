/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2011-2016, Seven Du <dujinfang@gmail.com>
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
 * Seven Du <dujinfang@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 *
 * msrp.h -- MSRP lib
 *
 */
 #ifndef _MSRP_H
#define _MSRP_H

#include <switch.h>

enum {
	MSRP_ST_WAIT_HEADER,
	MSRP_ST_PARSE_HEADER,
	MSRP_ST_WAIT_BODY,
	MSRP_ST_DONE,
	MSRP_ST_ERROR,

	MSRP_METHOD_REPLY,
	MSRP_METHOD_SEND,
	MSRP_METHOD_AUTH,
	MSRP_METHOD_REPORT,

};

enum {
	MSRP_H_FROM_PATH,
	MSRP_H_TO_PATH,
	MSRP_H_MESSAGE_ID,
	MSRP_H_CONTENT_TYPE,
	MSRP_H_SUCCESS_REPORT,
	MSRP_H_FAILURE_REPORT,
	MSRP_H_UNKNOWN
};

typedef struct msrp_msg_s {
	int state;
	int method;
	char *headers[12];
	int last_header;
	char *transaction_id;
	char *delimiter;
	int code_number;
	char *code_description;
	switch_size_t byte_start;
	switch_size_t byte_end;
	switch_size_t bytes;
	switch_size_t payload_bytes;
	int range_star; /* range-end is '*' */
	char *last_p;
	char *payload;
	struct msrp_msg_s *next;
} msrp_msg_t;

typedef struct msrp_msg_s switch_msrp_msg_t;

typedef struct {
	switch_memory_pool_t *pool;
	char *remote_path;
	char *remote_accept_types;
	char *remote_accept_wrapped_types;
	char *remote_setup;
	char *remote_file_selector;
	char *local_path;
	char *local_accept_types;
	char *local_accept_wrapped_types;
	char *local_setup;
	char *local_file_selector;
	int local_port;
	char *call_id;
	msrp_msg_t *msrp_msg;
	msrp_msg_t *last_msg;
	switch_mutex_t *mutex;
	switch_size_t msrp_msg_buffer_size;
	switch_size_t msrp_msg_count;
	switch_socket_t *socket;
	switch_frame_t frame;
	uint8_t frame_data[SWITCH_RTP_MAX_BUF_LEN];
} switch_msrp_session_t;

switch_status_t switch_msrp_init();
switch_status_t switch_msrp_destroy();
switch_msrp_session_t *switch_msrp_session_new(switch_memory_pool_t *pool);
switch_status_t switch_msrp_session_destroy(switch_msrp_session_t **ms);
// switch_status_t switch_msrp_session_push_msg(switch_msrp_session_t *ms, msrp_msg_t *msg);
switch_msrp_msg_t *switch_msrp_session_pop_msg(switch_msrp_session_t *ms);
switch_status_t switch_msrp_send(switch_msrp_session_t *ms, msrp_msg_t *msg);

void load_msrp_apis_and_applications(switch_loadable_module_interface_t **moudle_interface);

#endif

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
