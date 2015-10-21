/*
 * mod_ooh323 for FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2013-2016, Seven Du <dujinfang@gmail.com>
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
 * The Original Code is mod_ooh323 for FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is Seven Du.
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Anthony Minessale II <anthm@freeswitch.org>
 * Zhu Bo <wavecb@gmail.com>
 * Seven Du <dujinfang@gmail.com>
 *
 * mod_ooh323.c -- OOH323 Endpoint Module
 *
 */


#include "mod_ooh323.h"

switch_status_t ooh323_on_execute(switch_core_session_t *session);
switch_status_t ooh323_send_dtmf(switch_core_session_t *session, const switch_dtmf_t *dtmf);
switch_status_t ooh323_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg);
switch_status_t ooh323_receive_event(switch_core_session_t *session, switch_event_t *event);
switch_status_t ooh323_on_init(switch_core_session_t *session);
switch_status_t ooh323_on_hangup(switch_core_session_t *session);
switch_status_t ooh323_on_destroy(switch_core_session_t *session);
switch_status_t ooh323_on_routing(switch_core_session_t *session);
switch_status_t ooh323_on_exchange_media(switch_core_session_t *session);
switch_status_t ooh323_on_soft_execute(switch_core_session_t *session);
switch_call_cause_t ooh323_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
											switch_caller_profile_t *outbound_profile,
											switch_core_session_t **new_session, switch_memory_pool_t **pool, switch_originate_flag_t flags,
											switch_call_cause_t *cancel_cause);
switch_status_t ooh323_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
switch_status_t ooh323_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);
switch_status_t ooh323_read_video_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
switch_status_t ooh323_write_video_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);
switch_status_t ooh323_kill_channel(switch_core_session_t *session, int sig);

SWITCH_MODULE_LOAD_FUNCTION(mod_ooh323_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ooh323_shutdown);
SWITCH_MODULE_DEFINITION(mod_ooh323, mod_ooh323_load, mod_ooh323_shutdown, NULL);

switch_state_handler_table_t ooh323_state_handlers = {
	/*.on_init */ ooh323_on_init,
	/*.on_routing */ ooh323_on_routing,
	/*.on_execute */ ooh323_on_execute,
	/*.on_hangup */ ooh323_on_hangup,
	/*.on_exchange_media */ ooh323_on_exchange_media,
	/*.on_soft_execute */ ooh323_on_soft_execute,
	/*.on_consume_media */ NULL,
	/*.on_hibernate */ NULL,
	/*.on_reset */ NULL,
	/*.on_park */ NULL,
	/*.on_reporting */ NULL,
	/*.on_destroy */ ooh323_on_destroy
};

switch_io_routines_t ooh323_io_routines = {
	/*.outgoing_channel */ ooh323_outgoing_channel,
	/*.read_frame */ ooh323_read_frame,
	/*.write_frame */ ooh323_write_frame,
	/*.kill_channel */ ooh323_kill_channel,
	/*.send_dtmf */ ooh323_send_dtmf,
	/*.receive_message */ ooh323_receive_message,
	/*.receive_event */ ooh323_receive_event,
	/*.state_change */ NULL,
	/*.read_video_frame */ ooh323_read_video_frame,
	/*.write_video_frame */ ooh323_write_video_frame,
	/*.state_run*/ NULL,
	/*.get_jb*/ NULL
};

struct ooh323_globals {
	switch_memory_pool_t *pool;
	switch_endpoint_interface_t *ooh323_endpoint_interface;
	switch_thread_t *ooh323_thread;
	ooh323_profile_t profile;
	switch_mutex_t *mutex;
	int calls;
};

/* Global endpoint structure */
static struct ooh323_globals globals;
/* Global interface */
switch_endpoint_interface_t *ooh323_endpoint_interface;
/* a CNG frame data*/

char cng_data[2];

/*
   State methods they get called when the state changes to the specific state
   returning SWITCH_STATUS_SUCCESS tells the core to execute the standard state method next
   so if you fully implement the state you can return SWITCH_STATUS_FALSE to skip it.
*/
switch_status_t ooh323_on_init(switch_core_session_t *session)
{
	switch_channel_t *channel;
	ooh323_private_t *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_mutex_lock(globals.mutex);
	globals.calls++;
	switch_mutex_unlock(globals.mutex);

	switch_set_flag_locked(tech_pvt, TFLAG_IO);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_on_routing(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL ROUTING\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_on_execute(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL EXECUTE\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_on_destroy(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);

	if (tech_pvt == NULL) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "no tech_pvt on session %s!!!\n",
			switch_core_session_get_uuid(session));
		return SWITCH_STATUS_SUCCESS;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"channel on destroy %s\n", switch_channel_get_name(channel));

	while(switch_test_flag(tech_pvt, TFLAG_SIG)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,
			"waiting ooh323 [%s] to be completed\n", tech_pvt->h323_callToken);
		switch_yield(1000000);
	}

	switch_media_handle_destroy(session);

#if 0
	if (tech_pvt->local_rtp_port) {
		switch_rtp_release_port(tech_pvt->local_rtp_ip, tech_pvt->local_rtp_port);
	}

	if (tech_pvt->local_video_rtp_port) {
		switch_rtp_release_port(tech_pvt->local_rtp_ip, tech_pvt->local_video_rtp_port);
	}

	if (tech_pvt->read_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->read_codec);
	}

	if (tech_pvt->write_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->write_codec);
	}

#endif

	// switch_core_session_set_read_codec(tech_pvt->session, NULL);
	// switch_core_session_set_write_codec(tech_pvt->session, NULL);

	switch_mutex_lock(globals.mutex);
	globals.calls--;
	switch_mutex_unlock(globals.mutex);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_on_hangup(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	if (switch_test_flag(tech_pvt, TFLAG_SIG)) {
		ooh323_hangup_call(tech_pvt);
	}

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_kill_channel(switch_core_session_t *session, int sig)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);
	switch (sig) {
	case SWITCH_SIG_BREAK:
		if (switch_core_media_ready(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO)) {
			switch_core_media_break(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO);
		}
		if (switch_core_media_ready(tech_pvt->session, SWITCH_MEDIA_TYPE_VIDEO)) {
			switch_core_media_break(tech_pvt->session, SWITCH_MEDIA_TYPE_VIDEO);
		}
		break;
	case SWITCH_SIG_KILL:
		switch_clear_flag_locked(tech_pvt, TFLAG_IO);
		switch_set_flag_locked(tech_pvt, TFLAG_HUP);

		if (switch_core_media_ready(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO)) {
			switch_core_media_kill_socket(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO);
		}
		if (switch_core_media_ready(tech_pvt->session, SWITCH_MEDIA_TYPE_VIDEO)) {
			switch_core_media_kill_socket(tech_pvt->session, SWITCH_MEDIA_TYPE_VIDEO);
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_on_exchange_media(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "CHANNEL LOOPBACK\n");
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_on_soft_execute(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "CHANNEL TRANSMIT\n");
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_send_dtmf(switch_core_session_t *session, const switch_dtmf_t *dtmf)
{
	ooh323_private_t *tech_pvt = switch_core_session_get_private(session);
	switch_assert(tech_pvt != NULL);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;
	uint32_t sanity = 1000;
	switch_status_t status = SWITCH_STATUS_FALSE;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	if (switch_test_flag(tech_pvt, TFLAG_HUP)) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_test_flag(tech_pvt, TFLAG_RTP)) {
		switch_yield(20000); // TODO: use session timer
		*frame = &tech_pvt->read_frame;
		(*frame)->codec = switch_core_session_get_read_codec(session);
		goto cng;
	}

	while (!(switch_core_media_ready(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO) && !switch_channel_test_flag(channel, CF_REQ_MEDIA))) {
		switch_ivr_parse_all_messages(tech_pvt->session);
		if (--sanity && switch_channel_up(channel)) {
			switch_yield(10000);
		} else {
			switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_RECOVERY_ON_TIMER_EXPIRE);
			return SWITCH_STATUS_GENERR;
		}
	}

	switch_mutex_lock(tech_pvt->mutex);
	status = switch_core_media_read_frame(session, frame, flags, stream_id, SWITCH_MEDIA_TYPE_AUDIO);
	switch_mutex_unlock(tech_pvt->mutex);

	if (status == SWITCH_STATUS_SUCCESS || status == SWITCH_STATUS_BREAK) {
		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "read audio %d\n", (*frame)->datalen);
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;

cng:

	switch_set_flag(*frame, SFF_CNG);
	(*frame)->datalen = 2;
	(*frame)->buflen = 2;
	(*frame)->data = cng_data;

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	if (switch_test_flag(tech_pvt, TFLAG_HUP)) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_test_flag(tech_pvt, TFLAG_RTP)) {
		return SWITCH_STATUS_SUCCESS;
	}

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "writing audio %d\n", frame->datalen);

	switch_mutex_lock(tech_pvt->mutex);
	switch_core_media_write_frame(session, frame, flags, stream_id, SWITCH_MEDIA_TYPE_AUDIO);
	switch_mutex_unlock(tech_pvt->mutex);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_read_video_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;
	switch_status_t status = SWITCH_STATUS_FALSE;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	if (switch_test_flag(tech_pvt, TFLAG_HUP)) {
		return SWITCH_STATUS_FALSE;
	}

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "read video stream\n");

	if (!switch_test_flag(tech_pvt, TFLAG_RTP)) {
		switch_yield(20000);
		*frame = &tech_pvt->read_video_frame;
		(*frame)->codec = switch_core_session_get_video_read_codec(session);
		goto cng;
	}

	status = switch_core_media_read_frame(session, frame, flags, stream_id, SWITCH_MEDIA_TYPE_VIDEO);

	if (status == SWITCH_STATUS_SUCCESS || status == SWITCH_STATUS_BREAK) {
		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "read video %d\n", (*frame)->datalen);
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;

cng:

	switch_set_flag(*frame, SFF_CNG);
	(*frame)->datalen = 2;
	(*frame)->buflen = 2;
	(*frame)->data = cng_data;

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_write_video_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "writing video %d\n", frame->datalen);

	if (switch_test_flag(tech_pvt, TFLAG_HUP)) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_test_flag(tech_pvt, TFLAG_RTP)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (!switch_test_flag(tech_pvt, TFLAG_IO)) {
		return SWITCH_STATUS_SUCCESS;
	}
	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "writing video %d\n", frame->datalen);

	if (SWITCH_STATUS_SUCCESS == switch_core_media_write_frame(session, frame, flags, stream_id, SWITCH_MEDIA_TYPE_VIDEO)) {
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

switch_status_t ooh323_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_channel_t *channel;
	ooh323_private_t *tech_pvt;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = (ooh323_private_t *) switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (msg->message_id) {
	case SWITCH_MESSAGE_INDICATE_ANSWER:
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Answering h323 channel %s\n", tech_pvt->h323_callToken);
		if(ooh323_answer_call(tech_pvt) == SWITCH_STATUS_SUCCESS) {
			switch_channel_mark_answered(channel);
		}
		break;
	case SWITCH_MESSAGE_INDICATE_RINGING:
		ooh323_manual_ringback(tech_pvt);
		switch_channel_mark_ring_ready(channel);
		break;
	case SWITCH_MESSAGE_INDICATE_PROGRESS:
		switch_channel_mark_pre_answered(channel);
		break;
	case SWITCH_MESSAGE_INDICATE_HOLD:
	case SWITCH_MESSAGE_INDICATE_UNHOLD:
		break;
	case SWITCH_MESSAGE_INDICATE_DISPLAY:
		break;
	case SWITCH_MESSAGE_INDICATE_VIDEO_REFRESH_REQ:
		// if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		{
			if (ooh323_video_refresh(tech_pvt) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
					"Error sending fast update %s\n", tech_pvt->h323_callToken);
			}
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

/* Make sure when you have 2 sessions in the same scope that you pass the appropriate one to the routines
   that allocate memory or you will have 1 channel with memory allocated from another channel's pool!
*/
switch_call_cause_t ooh323_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **inpool, switch_originate_flag_t flags,
													switch_call_cause_t *cancel_cause)
{

	// return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;

#if 1
	ooh323_private_t *tech_pvt;
	switch_caller_profile_t *caller_profile;
	switch_channel_t *channel = NULL;
	const char *uuid = NULL;
	char *dest = outbound_profile->destination_number;

	*new_session = switch_core_session_request_uuid(ooh323_endpoint_interface,
		SWITCH_CALL_DIRECTION_OUTBOUND, flags, inpool, switch_event_get_header(var_event, "origination_uuid"));

	if (!*new_session) {
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	uuid = switch_core_session_get_uuid(*new_session);
	assert(uuid);

	switch_core_session_add_stream(*new_session, NULL);

	if (zstr(outbound_profile->destination_number)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No destination number\n");
		switch_core_session_destroy(new_session);
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	channel = switch_core_session_get_channel(*new_session);
	if (!channel) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No new Channel\n");
		switch_core_session_destroy(new_session);
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	tech_pvt = switch_core_session_alloc(*new_session, sizeof(ooh323_private_t));

	if (ooh323_tech_init(tech_pvt, *new_session) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Doh! no tech_init?\n");
		switch_core_session_destroy(new_session);
		// switch_mutex_unlock(globals.mutex);
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	ooh323_attach_private(*new_session, &globals.profile, tech_pvt);

	if (outbound_profile) {
		char name[128];

		snprintf(name, sizeof(name), "ooh323/%s", dest);
		switch_channel_set_name(channel, name);
		caller_profile = switch_caller_profile_clone(*new_session, outbound_profile);
		switch_channel_set_caller_profile(channel, caller_profile);
		tech_pvt->caller_profile = caller_profile;
		switch_channel_set_flag(channel, CF_AUDIO_PAUSE);
		switch_channel_set_flag(channel, CF_VIDEO_PAUSE_READ);
		switch_channel_set_flag(channel, CF_VIDEO_PAUSE_WRITE);
		switch_channel_set_flag(channel, CF_VIDEO_POSSIBLE);
		switch_channel_set_state(channel, CS_INIT);
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No Caller Profile\n");
		switch_core_session_destroy(new_session);
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	switch_set_flag(tech_pvt, TFLAG_OUTBOUND);
	tech_pvt->caller_id_name = switch_core_session_strdup(*new_session, outbound_profile->caller_id_name);
	tech_pvt->caller_id_number = switch_core_session_strdup(*new_session, outbound_profile->caller_id_number);
	tech_pvt->destination_number = switch_core_session_strdup(*new_session, dest);

	if (ooh323_make_call(tech_pvt) != SWITCH_STATUS_SUCCESS) {
		switch_core_session_destroy(new_session);
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

#endif
	return SWITCH_CAUSE_SUCCESS;
}

switch_status_t ooh323_receive_event(switch_core_session_t *session, switch_event_t *event)
{
	ooh323_private_t *tech_pvt = switch_core_session_get_private(session);
	switch_assert(tech_pvt != NULL);

	return SWITCH_STATUS_SUCCESS;
}


void *SWITCH_THREAD_FUNC ooh323_thread_run(switch_thread_t *thread, void *obj){

	ooh323_listener_loop(&globals.profile);
	return NULL;
}

SWITCH_STANDARD_API(ooh323_function)
{
	int argc;
	char *mydata = NULL, *argv[3];
	ooh323_profile_t *profile = &globals.profile;

	if (!cmd) {
		goto error;
	}

	mydata = strdup(cmd);
	switch_assert(mydata);

	argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc < 1) goto error;

	stream->write_function(stream, "OOH323 Endpoint\n==================================\n");
	stream->write_function(stream, "Listen:          %s:%d\n", profile->listen_ip, profile->listen_port);
	stream->write_function(stream, "Call Mode:       %s\n",
		profile->call_mode == OO_CALLMODE_VIDEOCALL ? "Video" : "Audio");
	stream->write_function(stream, "Gatekeeper:      ");
		if (profile->gk_mode == RasNoGatekeeper) {
			stream->write_function(stream, "None\n");
		} else if (profile->gk_mode == RasDiscoverGatekeeper) {
			stream->write_function(stream, "Discover\n");
		} else {
			stream->write_function(stream, "%s:%d\n", profile->gk_ip, profile->gk_port);
			stream->write_function(stream, "GK ID:           %s\n", profile->gk_id);
		}
	stream->write_function(stream, "Faststart:       %s\n",
		profile->is_fast_start == TRUE ? "True" : "False");
	stream->write_function(stream, "Tunneling:       %s\n",
		profile->is_tunneling == TRUE ? "True" : "False");
	stream->write_function(stream, "Trace Level:     %d (%s)\n", profile->trace_level, profile->logfile);

	stream->write_function(stream, "DTMF Type:       ");

	switch (profile->dtmf_type) {
		case H323_DTMF_RFC2833:
			stream->write_function(stream, "RFC2833 (%d)\n", profile->te);
			break;
		case H323_DTMF_Q931:
			stream->write_function(stream, "Q931\n");
			break;
		case H323_DTMF_H245ALPHANUMERIC:
			stream->write_function(stream, "H245ALPHANUMERIC\n");
			break;
		case H323_DTMF_H245SIGNAL:
			stream->write_function(stream, "H245SIGNAL\n");
			break;
		case H323_DTMF_INBAND:
			stream->write_function(stream, "INBAND\n");
			break;
		default:
			stream->write_function(stream, "NONE\n");
	}

	stream->write_function(stream, "H323 User:       %s\n", profile->user);
	stream->write_function(stream, "H323 ID:         %s\n", profile->h323id);
	stream->write_function(stream, "H323 User Num:   %s\n", profile->user_num);
	stream->write_function(stream, "Dialplan:        %s\n", profile->dialplan);
	stream->write_function(stream, "Context:         %s\n", profile->context);

	if (profile->extn) {
		stream->write_function(stream, "Extn:            %s\n", profile->extn);
	}

	if (profile->timer_name) {
		stream->write_function(stream, "Timer:           %s\n", profile->timer_name);
	}

	if (profile->inbound_codec_string) {
		stream->write_function(stream, "Inbound Codec:   %s\n", profile->inbound_codec_string);
	}

	if (profile->outbound_codec_string) {
		stream->write_function(stream, "Outbound Codec:  %s\n", profile->outbound_codec_string);
	}

	if (profile->tcpmin && profile->tcpmax) {
		stream->write_function(stream, "TCP Port Range:  %d ~ %d\n", profile->tcpmin, profile->tcpmax);
	}

	if (profile->tcpmin && profile->tcpmax) {
		stream->write_function(stream, "UDP Port Range:  %d ~ %d\n", profile->udpmin, profile->udpmax);
	}

	if (profile->tcpmin && profile->tcpmax) {
		stream->write_function(stream, "RTP Port Range:  %d ~ %d\n", profile->rtpmin, profile->rtpmax);
	}

	goto ok;

error:

	stream->write_function(stream, "Error");

ok:

	switch_safe_free(mydata);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_APP(wait_for_media_function)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	int sanity = 50;

	if (!zstr(data)) sanity = atoi(data);

	while (sanity-- > 0 && switch_channel_ready(channel) &&
		((!switch_core_session_get_read_codec(session)) || switch_channel_test_flag(channel, CF_AUDIO_PAUSE)) ) {
		switch_ivr_sleep(session, 200, SWITCH_TRUE, NULL);
		if (sanity % 10 == 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Waiting for media ... %d\n", sanity);
		}
	}
}

switch_status_t config(void)
{
	char *cf = "ooh323.conf";
	ooh323_profile_t *profile = &globals.profile;
	switch_memory_pool_t *pool = globals.pool;
	switch_xml_t cfg, xml, settings, param;

	switch_set_string(profile->listen_ip, switch_core_get_variable("local_ip_v4"));
	profile->call_mode = OO_CALLMODE_VIDEOCALL;
	profile->gk_mode = RasNoGatekeeper;
	profile->gk_id = "FreeSWITCH";
	profile->gk_port = 1719;
	profile->listen_port = 1720;
	profile->is_fast_start = TRUE;
	profile->is_tunneling = TRUE;
	profile->trace_level = 3;
	profile->te = 101;
	profile->dtmf_type = H323_DTMF_RFC2833;
	profile->user = DEFAULT_H323ID;
	profile->h323id = DEFAULT_H323ID;
	profile->user_num = "0000000000";
	profile->dialplan = DEFAULT_DIALPLAN;
	profile->context = DEFAULT_CONTEXT;

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return SWITCH_STATUS_FALSE;
	}

	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *)switch_xml_attr_soft(param, "name");
			char *val = (char *)switch_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "call-mode")) {
				if(!strcasecmp(val, "video")) {
					profile->call_mode = OO_CALLMODE_VIDEOCALL;
				} else if(!strcasecmp(val, "audio")) {
					profile->call_mode = OO_CALLMODE_AUDIOCALL;
				} else if(!strcasecmp(val, "audiorx")) {
					profile->call_mode = OO_CALLMODE_AUDIORX;
				}
			} else if (!strcasecmp(var, "gk-ip")) {
				if(!strcasecmp(val, "discover")) {
					profile->gk_mode = RasDiscoverGatekeeper;
				} else {
					profile->gk_mode = RasUseSpecificGatekeeper;
					switch_set_string(profile->gk_ip, val);
				}
			} else if (!strcasecmp(var, "gk-port")) {
				int tmp = atoi(val);
				if (tmp > 0 && tmp < 65535) {
					profile->gk_port = tmp;
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set a negative gk port!\n");
				}
			} else if (!strcasecmp(var, "gk-id")) {
				profile->gk_id = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "listen-ip")) {
				 if(!zstr(val)) {
					switch_set_string(profile->listen_ip, val);
				 }
			} else if (!strcasecmp(var, "listen-port")) {
				int tmp = atoi(val);
				if (tmp > 0 && tmp < 65535) {
					profile->listen_port = tmp;
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set a negative listen port!\n");
				}
			} else if (!strcasecmp(var, "faststart")) {
				profile->is_fast_start = switch_true(val) ? TRUE : FALSE;
			} else if (!strcasecmp(var, "h245tunneling")) {
				profile->is_tunneling = switch_true(val) ? TRUE : FALSE;
			} else if (!strcasecmp(var, "h323-id")) {
				profile->h323id = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "h323-user")) {
				profile->user = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "h323-usernum")) {
				profile->user_num = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "trace-level")) {
				profile->trace_level = (switch_payload_t) atoi(val);
			} else if (!strcasecmp(var, "log-file")) {
				profile->logfile = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "dialplan")) {
				profile->dialplan = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "context")) {
				profile->context = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "extn")) {
				profile->extn = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "rtp-timer-name")) {
				profile->timer_name = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "disable-rtp-auto-adjust")) {
				profile->disable_rtp_autoadj = switch_true(val);
			} else if (!strcasecmp(var, "inbound-codec-prefs")) {
				profile->inbound_codec_string = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "outbound-codec-prefs")) {
				profile->outbound_codec_string = switch_core_strdup(pool, val);
			} else if (!strcasecmp(var, "dtmf-type")) {
				if (!strcasecmp(val, "rfc2833")) {
					profile->dtmf_type = H323_DTMF_RFC2833;
				} else if (!strcasecmp(val, "Q931")) {
					profile->dtmf_type = H323_DTMF_Q931;
				} else if (!strcasecmp(val, "H245ALPHANUMERIC")) {
					profile->dtmf_type = H323_DTMF_H245ALPHANUMERIC;
				} else if (!strcasecmp(val, "H245SIGNAL")) {
					profile->dtmf_type = H323_DTMF_H245SIGNAL;
				} else if (!strcasecmp(val, "INBAND")) {
					profile->dtmf_type = H323_DTMF_INBAND;
				}
			} else if (!strcasecmp(var, "rfc2833-pt")) {
				profile->te = (switch_payload_t) atoi(val);
			} else if (!strcasecmp(var, "apply-inbound-acl")) {
				if (profile->acl_count < H323_MAX_ACL) {
					char *list, *pass = NULL, *fail = NULL;

					list = switch_core_strdup(pool, val);

					if ((pass = strchr(list, ':'))) {
						*pass++ = '\0';
						if ((fail = strchr(pass, ':'))) {
							*fail++ = '\0';
						}

						if (zstr(pass)) pass = NULL;
						if (zstr(fail)) fail = NULL;

						profile->acl_pass_context[profile->acl_count] = pass;
						profile->acl_fail_context[profile->acl_count] = fail;
					}

					profile->acl[profile->acl_count++] = list;

				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Max acl records of %d reached\n", H323_MAX_ACL);
				}
			} else if (!strcasecmp(var, "tcp-min-port")) {
				profile->tcpmin = (switch_port_t) atoi(val);
			} else if (!strcasecmp(var, "tcp-max-port")) {
				profile->tcpmax = (switch_port_t) atoi(val);
			} else if (!strcasecmp(var, "udp-min-port")) {
				profile->udpmin = (switch_port_t) atoi(val);
			} else if (!strcasecmp(var, "udp-max-port")) {
				profile->udpmax = (switch_port_t) atoi(val);
			} else if (!strcasecmp(var, "rtp-min-port")) {
				profile->rtpmin = (switch_port_t) atoi(val);
			} else if (!strcasecmp(var, "rtp-max-port")) {
				profile->rtpmax = (switch_port_t) atoi(val);
			}
		}
	}

	switch_xml_free(xml);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_ooh323_load)
{
	ooh323_profile_t *profile = &globals.profile;
	switch_threadattr_t *thd_attr = NULL;
	switch_api_interface_t *api_interface;
	switch_application_interface_t *app_interface;

	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	ooh323_endpoint_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_ENDPOINT_INTERFACE);
	ooh323_endpoint_interface->interface_name = "ooh323";
	ooh323_endpoint_interface->io_routines = &ooh323_io_routines;
	ooh323_endpoint_interface->state_handler = &ooh323_state_handlers;

	memset(&globals, 0, sizeof(globals));

	globals.pool = pool;
	switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, pool);
	switch_mutex_init(&profile->mutex, SWITCH_MUTEX_NESTED, pool);

	if (config() != SWITCH_STATUS_SUCCESS) return SWITCH_STATUS_TERM;

	if (ooh323_start(profile) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error start H323 profile!\n");
		return SWITCH_STATUS_FALSE;
	}

	switch_threadattr_create(&thd_attr, globals.pool);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	//switch_threadattr_priority_set(thd_attr, SWITCH_PRI_REALTIME);
	switch_thread_create(&globals.ooh323_thread,
						thd_attr,
						ooh323_thread_run,
						NULL,
						globals.pool);

	SWITCH_ADD_API(api_interface, "ooh323", "OOH323 function", ooh323_function, "<status>");
	SWITCH_ADD_APP(app_interface, "wait_for_media", "Wait for media to be ready", "Wait for media to be ready", wait_for_media_function, "", SAF_NONE);

	switch_console_set_complete("add ooh323 status");

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ooh323_shutdown)
{
	switch_status_t st;
	ooh323_stop(&globals.profile);
	if (globals.ooh323_thread) {
		switch_thread_join(&st, globals.ooh323_thread);
	}
	return SWITCH_STATUS_SUCCESS;
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
