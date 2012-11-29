/* 
* FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
* Copyright (C) 2005-2011, Anthony Minessale II <anthm@freeswitch.org>
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
* Anthony Minessale II <anthm@freeswitch.org>
* Portions created by the Initial Developer are Copyright (C)
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
* 
* Mathieu Rene <mrene@avgs.ca>
*
* tdm.c -- FreeTDM Controllable Channel Module
*
*/

#include <switch.h>
#include "freetdm.h"
#include "private/ftdm_core.h"

void ctdm_init(switch_loadable_module_interface_t *module_interface);

/* Parameters */

#define kSPAN_ID "span"
#define kCHAN_ID "chan"
#define kSPAN_NAME "span_name"
#define kPREBUFFER_LEN "prebuffer_len"
#define kECHOCANCEL "echo_cancel"
#define kDTMF_REMOVAL "mg-tdm-dtmfremoval"


static struct {
    switch_memory_pool_t *pool;
    switch_endpoint_interface_t *endpoint_interface;
} ctdm;

typedef struct {
    int span_id;
    int chan_id;
    ftdm_channel_t *ftdm_channel;
    switch_core_session_t *session;
    switch_codec_t read_codec, write_codec;
    switch_frame_t read_frame;
    int prebuffer_len;
    
	unsigned char databuf[SWITCH_RECOMMENDED_BUFFER_SIZE];
} ctdm_private_t;

static switch_status_t channel_on_init(switch_core_session_t *session);
static switch_status_t channel_on_destroy(switch_core_session_t *session);
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, 
													switch_memory_pool_t **pool,
													switch_originate_flag_t flags, switch_call_cause_t *cancel_cause);
static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);
static switch_status_t channel_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg);
static switch_status_t channel_receive_event(switch_core_session_t *session, switch_event_t *event);
static switch_status_t channel_send_dtmf(switch_core_session_t *session, const switch_dtmf_t *dtmf);


static ftdm_status_t ctdm_span_prepare(ftdm_span_t *span);
static ftdm_status_t reload_span_config(void);

switch_state_handler_table_t ctdm_state_handlers = {
	.on_init = channel_on_init,
	.on_destroy = channel_on_destroy
};

switch_io_routines_t ctdm_io_routines = {
    .send_dtmf = channel_send_dtmf,
	.outgoing_channel = channel_outgoing_channel,
	.read_frame = channel_read_frame,
	.write_frame = channel_write_frame,
	.receive_message = channel_receive_message,
    .receive_event = channel_receive_event
};

static void ctdm_report_alarms(ftdm_channel_t *channel)
{
	switch_event_t *event = NULL;
	ftdm_alarm_flag_t alarmflag = 0;

	if (switch_event_create(&event, SWITCH_EVENT_TRAP) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "failed to create alarms events\n");
		return;
	}
	
	if (ftdm_channel_get_alarms(channel, &alarmflag) != FTDM_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to retrieve alarms %s:%d\n", ftdm_channel_get_span_name(channel), ftdm_channel_get_ph_id(channel));
		return;
	}

	switch_event_add_header(event, SWITCH_STACK_BOTTOM, "span-name", "%s", ftdm_channel_get_span_name(channel));
	switch_event_add_header(event, SWITCH_STACK_BOTTOM, "span-number", "%d", ftdm_channel_get_span_id(channel));
	switch_event_add_header(event, SWITCH_STACK_BOTTOM, "chan-number", "%d", ftdm_channel_get_ph_id(channel));
	
	if (alarmflag) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "condition", "ftdm-alarm-trap");
	} else {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "condition", "ftdm-alarm-clear");
	}

	if (alarmflag & FTDM_ALARM_RED) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "red");
	}
	if (alarmflag & FTDM_ALARM_YELLOW) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "yellow");
	}
	if (alarmflag & FTDM_ALARM_RAI) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "rai");
	}
	if (alarmflag & FTDM_ALARM_BLUE) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "blue");
	}
	if (alarmflag & FTDM_ALARM_AIS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "ais");
	}
	if (alarmflag & FTDM_ALARM_GENERAL) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "general");
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Reporting [%s] alarms for %s:%d\n", 
					  (alarmflag?"ftdm-alarm-trap":"ftdm-alarm-clear"), ftdm_channel_get_span_name(channel), ftdm_channel_get_ph_id(channel));

	switch_event_fire(&event);
	return;
}

static ftdm_channel_t *ctdm_get_channel_from_event(switch_event_t *event, ftdm_span_t *span)
{
	uint32_t chan_id = 0;
	const char *chan_number = NULL;

	chan_number = switch_event_get_header(event, "chan-number");

	if (zstr(chan_number)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No channel number specified\n");
		return NULL;
	}
	chan_id = atoi(chan_number);
	if (!chan_id) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid channel number:%s\n", chan_number);
		return NULL;
	}

	return ftdm_span_get_channel_ph(span, chan_id);
}


static void ctdm_event_handler(switch_event_t *event)
{
	ftdm_status_t status = FTDM_FAIL;
	switch(event->event_id) {
		case SWITCH_EVENT_TRAP:
			{
				ftdm_span_t *span = NULL;
				ftdm_channel_t *channel = NULL;
				const char *span_name = NULL;

				const char *cond = switch_event_get_header(event, "condition");
				const char *command = switch_event_get_header(event, "command");
				if (zstr(cond)) {
					return;
				}

                if (!strcmp(cond, "mg-tdm-reload")) {
                    status = reload_span_config();
                    if (status == FTDM_SUCCESS) {
                        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "span configuration reloaded successfully\n");
                    } else if (status != FTDM_EINVAL) {
                        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "span configuration reloading failed\n");
                    }
                    return;
                }

				span_name = switch_event_get_header(event, "span-name");
				
				if (ftdm_span_find_by_name(span_name, &span) != FTDM_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find span [%s]\n", span_name);
					return;
				}

				if (!strcmp(cond, "mg-tdm-prepare")) {
					status = ctdm_span_prepare(span);
					if (status == FTDM_SUCCESS) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s:prepared successfully\n", span_name);
					} else if (status != FTDM_EINVAL) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s:Failed to prepare span\n", span_name);
					}
                } else if (!strcmp(cond, "mg-tdm-check")) {
					channel = ctdm_get_channel_from_event(event, span);
					if (!channel) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not find channel\n");
						return;
					}

					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Requesting alarm status for %s:%d\n", 
									  ftdm_channel_get_span_name(channel), ftdm_channel_get_ph_id(channel));

					ctdm_report_alarms(channel);
				} else if (!strcmp(cond, "mg-tdm-dtmfremoval")) {
					uint8_t enable = 0;
					channel = ctdm_get_channel_from_event(event, span);
					if (!channel) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not find channel\n");
						return;
					}

					if (zstr(command)) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s:No command specified for mg-tdm-dtmfremoval\n", span_name);
						return;
					}

					if (!strcmp(command, "enable")) {
						enable = 1;
					}

					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s DTMF-removal for %s:%d\n",
									  enable ? "Enabling" : "Disabling", ftdm_channel_get_span_name(channel), ftdm_channel_get_ph_id(channel));

					ftdm_channel_command(channel, enable ? FTDM_COMMAND_ENABLE_DTMF_REMOVAL : FTDM_COMMAND_DISABLE_DTMF_REMOVAL, 0);
				}
			}
			break;
		default:
			break;
	}
	return;
}

void ctdm_init(switch_loadable_module_interface_t *module_interface)
{
    switch_endpoint_interface_t *endpoint_interface;
    ctdm.pool = module_interface->pool;
    endpoint_interface = switch_loadable_module_create_interface(module_interface, SWITCH_ENDPOINT_INTERFACE);
    endpoint_interface->interface_name = "tdm";
    endpoint_interface->io_routines = &ctdm_io_routines;
    endpoint_interface->state_handler = &ctdm_state_handlers;
    ctdm.endpoint_interface = endpoint_interface;

	switch_event_bind("mod_freetdm", SWITCH_EVENT_TRAP, SWITCH_EVENT_SUBCLASS_ANY, ctdm_event_handler, NULL);
}

static FIO_SIGNAL_CB_FUNCTION(on_signal_cb)
{
	uint32_t chanid, spanid;
	switch_event_t *event = NULL;
	ftdm_alarm_flag_t alarmbits = FTDM_ALARM_NONE;

	chanid = ftdm_channel_get_ph_id(sigmsg->channel);
	spanid = ftdm_channel_get_span_id(sigmsg->channel);
	
	switch(sigmsg->event_id) {
		case FTDM_SIGEVENT_ALARM_CLEAR:
		case FTDM_SIGEVENT_ALARM_TRAP:
			{
				if (ftdm_channel_get_alarms(sigmsg->channel, &alarmbits) != FTDM_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "failed to retrieve alarms\n");
					return FTDM_FAIL;
				}

				if (switch_event_create(&event, SWITCH_EVENT_TRAP) != SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "failed to create alarms events\n");
					return FTDM_FAIL;
				}
				if (sigmsg->event_id == FTDM_SIGEVENT_ALARM_CLEAR) {
					ftdm_log(FTDM_LOG_NOTICE, "Alarm cleared on channel %d:%d\n", spanid, chanid);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "condition", "ftdm-alarm-clear");
				} else {
					ftdm_log(FTDM_LOG_NOTICE, "Alarm raised on channel %d:%d\n", spanid, chanid);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "condition", "ftdm-alarm-trap");
				}
			}
			break;
		default:
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unhandled event %d\n", sigmsg->event_id);
			break;
	}

	if (event) {
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "span-name", "%s", ftdm_channel_get_span_name(sigmsg->channel));
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "span-number", "%d", spanid);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "chan-number", "%d", chanid);

		if (alarmbits & FTDM_ALARM_RED) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "red");
		}
		if (alarmbits & FTDM_ALARM_YELLOW) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "yellow");
		}
		if (alarmbits & FTDM_ALARM_RAI) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "rai");
		}
		if (alarmbits & FTDM_ALARM_BLUE) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "blue");
		}
		if (alarmbits & FTDM_ALARM_AIS) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "ais");
		}
		if (alarmbits & FTDM_ALARM_GENERAL) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alarm", "general");
		}

		switch_event_fire(&event);
	}
	return FTDM_SUCCESS;
}

static ftdm_status_t ctdm_span_prepare(ftdm_span_t *span)
{
	if (ftdm_span_register_signal_cb(span, on_signal_cb) != FTDM_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register signal CB\n");
		return FTDM_FAIL;
	}
	return ftdm_span_start(span);
}

static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, 
													switch_memory_pool_t **pool,
													switch_originate_flag_t flags, switch_call_cause_t *cancel_cause)
{
    const char  *szchanid = switch_event_get_header(var_event, kCHAN_ID),
                *span_name = switch_event_get_header(var_event, kSPAN_NAME),
                *szprebuffer_len = switch_event_get_header(var_event, kPREBUFFER_LEN);
    int chan_id;
    int span_id;
    switch_caller_profile_t *caller_profile;
    ftdm_span_t *span;
    ftdm_channel_t *chan;
    switch_channel_t *channel;
    char name[128];
    const char *dname;
    ftdm_codec_t codec;
    uint32_t interval;
    ctdm_private_t *tech_pvt = NULL;

    if (zstr(szchanid) || zstr(span_name)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Both ["kSPAN_ID"] and ["kCHAN_ID"] have to be set.\n");
        goto fail;
    }
    
    chan_id = atoi(szchanid);
    
    if (ftdm_span_find_by_name(span_name, &span) == FTDM_SUCCESS) {
         span_id = ftdm_span_get_id(span);
    } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find span [%s]\n", span_name);
        goto fail;
    }

    if (!(*new_session = switch_core_session_request(ctdm.endpoint_interface, SWITCH_CALL_DIRECTION_OUTBOUND, 0, pool))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't request session.\n");
        goto fail;
    }
    
    channel = switch_core_session_get_channel(*new_session);
    
    if (ftdm_channel_open_ph(span_id, chan_id, &chan) != FTDM_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't open span or channel.\n"); 
        goto fail;
    }

    span = ftdm_channel_get_span(chan);

    tech_pvt = switch_core_session_alloc(*new_session, sizeof *tech_pvt);
	tech_pvt->chan_id = ftdm_channel_get_ph_id(chan);
	tech_pvt->span_id = ftdm_channel_get_ph_span_id(chan);
    tech_pvt->ftdm_channel = chan;
    tech_pvt->session = *new_session;
    tech_pvt->read_frame.buflen = sizeof(tech_pvt->databuf);
    tech_pvt->read_frame.data = tech_pvt->databuf;
    tech_pvt->prebuffer_len = zstr(szprebuffer_len) ? 0 : atoi(szprebuffer_len);
    switch_core_session_set_private(*new_session, tech_pvt);

    caller_profile = switch_caller_profile_clone(*new_session, outbound_profile);
    switch_channel_set_caller_profile(channel, caller_profile);

	snprintf(name, sizeof(name), "tdm/%d:%d", ftdm_channel_get_ph_span_id(chan), ftdm_channel_get_ph_id(chan));

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connect outbound TDM channel %s\n", name);
	switch_channel_set_name(channel, name);

    switch_channel_set_state(channel, CS_INIT);

	if (FTDM_SUCCESS != ftdm_channel_command(chan, FTDM_COMMAND_GET_CODEC, &codec)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to retrieve channel codec.\n");
		return SWITCH_STATUS_GENERR;
	}

    if (FTDM_SUCCESS != ftdm_channel_command(chan, FTDM_COMMAND_GET_INTERVAL, &interval)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to retrieve channel interval.\n");
		return SWITCH_STATUS_GENERR;
	}
    
    if (FTDM_SUCCESS != ftdm_channel_command(chan, FTDM_COMMAND_SET_PRE_BUFFER_SIZE, &tech_pvt->prebuffer_len)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to set channel pre buffer size.\n");
		return SWITCH_STATUS_GENERR;        
    }

    if (FTDM_SUCCESS != ftdm_channel_command(tech_pvt->ftdm_channel, FTDM_COMMAND_ENABLE_ECHOCANCEL, NULL)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Failed to set enable echo cancellation.\n");
	}
    
	switch(codec) {
        case FTDM_CODEC_ULAW:
		{
			dname = "PCMU";
		}
            break;
        case FTDM_CODEC_ALAW:
		{
			dname = "PCMA";
		}
            break;
        case FTDM_CODEC_SLIN:
		{
			dname = "L16";
		}
            break;
        default:
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid codec value retrieved from channel, codec value: %d\n", codec);
			goto fail;
		}
	}

    
	if (switch_core_codec_init(&tech_pvt->read_codec,
							   dname,
							   NULL,
							   8000,
							   interval,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
        goto fail;
	} else {
		if (switch_core_codec_init(&tech_pvt->write_codec,
								   dname,
								   NULL,
								   8000,
								   interval,
								   1,
								   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
								   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
			switch_core_codec_destroy(&tech_pvt->read_codec);
            goto fail;
		}
	}
    
    if (switch_core_session_set_read_codec(*new_session, &tech_pvt->read_codec) != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set read codec?\n");
        goto fail;
    }
    
    if (switch_core_session_set_write_codec(*new_session, &tech_pvt->write_codec) != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set write codec?\n");        
    }
    
    if (switch_core_session_thread_launch(*new_session) != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't start session thread.\n"); 
        goto fail;
    }
    
    switch_channel_mark_answered(channel);
    
    return SWITCH_CAUSE_SUCCESS;

fail:
    
    if (tech_pvt) {
        if (tech_pvt->ftdm_channel) {
            ftdm_channel_close(&tech_pvt->ftdm_channel);
        }
        
        if (tech_pvt->read_codec.implementation) {
			switch_core_codec_destroy(&tech_pvt->read_codec);
		}
		
		if (tech_pvt->write_codec.implementation) {
			switch_core_codec_destroy(&tech_pvt->write_codec);
		}
    }
    
    if (*new_session) {
        switch_core_session_destroy(new_session);
    }
    return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
}

static switch_status_t channel_on_init(switch_core_session_t *session)
{
    switch_channel_t *channel = switch_core_session_get_channel(session);
    
    switch_channel_set_state(channel, CS_CONSUME_MEDIA);   
    return SWITCH_STATUS_SUCCESS;   
}

static switch_status_t channel_on_destroy(switch_core_session_t *session)
{
    ctdm_private_t *tech_pvt = switch_core_session_get_private(session);
    
 	if ((tech_pvt = switch_core_session_get_private(session))) {
	
		if (FTDM_SUCCESS != ftdm_channel_command(tech_pvt->ftdm_channel, FTDM_COMMAND_ENABLE_ECHOCANCEL, NULL)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Failed to enable echo cancellation.\n");
		}
        
		if (tech_pvt->read_codec.implementation) {
			switch_core_codec_destroy(&tech_pvt->read_codec);
		}
		
		if (tech_pvt->write_codec.implementation) {
			switch_core_codec_destroy(&tech_pvt->write_codec);
		}
        
        ftdm_channel_close(&tech_pvt->ftdm_channel);
	}

    return SWITCH_STATUS_SUCCESS;
}


static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id)
{
    ftdm_wait_flag_t wflags = FTDM_READ;
    ftdm_status_t status;
    ctdm_private_t *tech_pvt;
    const char *name;
    switch_channel_t *channel;
    int chunk;
    uint32_t span_id, chan_id;
    ftdm_size_t len;
    char dtmf[128] = "";
    
    channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
	
	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);
    
	name = switch_channel_get_name(channel);

top:
    wflags = FTDM_READ;
    chunk = ftdm_channel_get_io_interval(tech_pvt->ftdm_channel) * 2;
    status = ftdm_channel_wait(tech_pvt->ftdm_channel, &wflags, chunk);
    
    
	span_id = ftdm_channel_get_span_id(tech_pvt->ftdm_channel);
	chan_id = ftdm_channel_get_ph_id(tech_pvt->ftdm_channel);

    if (status == FTDM_FAIL) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to read from channel %s device %d:%d!\n", name, span_id, chan_id);
        goto fail;
    }

    if (status == FTDM_TIMEOUT) {
        goto top;
    }

    if (!(wflags & FTDM_READ)) {
        goto top;
    }

    len = tech_pvt->read_frame.buflen;
    if (ftdm_channel_read(tech_pvt->ftdm_channel, tech_pvt->read_frame.data, &len) != FTDM_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Failed to read from channel %s device %d:%d!\n", name, span_id, chan_id);
    }

    *frame = &tech_pvt->read_frame;
    tech_pvt->read_frame.datalen = (uint32_t)len;
    tech_pvt->read_frame.samples = tech_pvt->read_frame.datalen;
    tech_pvt->read_frame.codec = &tech_pvt->read_codec;

    if (ftdm_channel_get_codec(tech_pvt->ftdm_channel) == FTDM_CODEC_SLIN) {
        tech_pvt->read_frame.samples /= 2;
    }

    while (ftdm_channel_dequeue_dtmf(tech_pvt->ftdm_channel, dtmf, sizeof(dtmf))) {
        switch_dtmf_t _dtmf = { 0, switch_core_default_dtmf_duration(0) };
        char *p;
        for (p = dtmf; p && *p; p++) {
            if (is_dtmf(*p)) {
                _dtmf.digit = *p;
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Queuing DTMF [%c] in channel %s device %d:%d\n", *p, name, span_id, chan_id);
                switch_channel_queue_dtmf(channel, &_dtmf);
            }
        }
    }

    return SWITCH_STATUS_SUCCESS;

fail:
    return SWITCH_STATUS_GENERR;
}

static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id)
{
    ftdm_wait_flag_t wflags = FTDM_WRITE;
    ctdm_private_t *tech_pvt;
    const char *name;
    switch_channel_t *channel;
    uint32_t span_id, chan_id;
    ftdm_size_t len;
    unsigned char data[SWITCH_RECOMMENDED_BUFFER_SIZE] = {0};
    
    channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
	
	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);
    
	span_id = ftdm_channel_get_span_id(tech_pvt->ftdm_channel);
	chan_id = ftdm_channel_get_ph_id(tech_pvt->ftdm_channel);
    
	name = switch_channel_get_name(channel);   
    
    if (switch_test_flag(frame, SFF_CNG)) {
		frame->data = data;
		frame->buflen = sizeof(data);
		if ((frame->datalen = tech_pvt->write_codec.implementation->encoded_bytes_per_packet) > frame->buflen) {
			goto fail;
		}
		memset(data, 255, frame->datalen);
	}
    
    wflags = FTDM_WRITE;	
	ftdm_channel_wait(tech_pvt->ftdm_channel, &wflags, ftdm_channel_get_io_interval(tech_pvt->ftdm_channel) * 10);
	
	if (!(wflags & FTDM_WRITE)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Dropping frame! (write not ready) in channel %s device %d:%d!\n", name, span_id, chan_id);
		return SWITCH_STATUS_SUCCESS;
	}
    
	len = frame->datalen;
	if (ftdm_channel_write(tech_pvt->ftdm_channel, frame->data, frame->buflen, &len) != FTDM_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Failed to write to channel %s device %d:%d!\n", name, span_id, chan_id);
	}
    
    return SWITCH_STATUS_SUCCESS;

fail:
    return SWITCH_STATUS_GENERR;
}

static switch_status_t channel_send_dtmf(switch_core_session_t *session, const switch_dtmf_t *dtmf)
{
	ctdm_private_t *tech_pvt = NULL;
	char tmp[2] = "";
    
	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);
        
	tmp[0] = dtmf->digit;
	ftdm_channel_command(tech_pvt->ftdm_channel, FTDM_COMMAND_SEND_DTMF, tmp);
    
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_receive_event(switch_core_session_t *session, switch_event_t *event)
{
    const char *command = switch_event_get_header(event, "command");
    ctdm_private_t *tech_pvt = switch_core_session_get_private(session);

    if (!zstr(command)) {

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "FreeTDM received %s command \n",command);

        if (!strcasecmp(command, kPREBUFFER_LEN)) {
            const char *szval = switch_event_get_header(event, kPREBUFFER_LEN);
            int val = !zstr(szval) ? atoi(szval) : 0;

            /*******************************************************************************************************/
            if (tech_pvt->prebuffer_len == val) {
                tech_pvt->prebuffer_len = val;
                if (FTDM_SUCCESS != ftdm_channel_command(tech_pvt->ftdm_channel, 
                            FTDM_COMMAND_SET_PRE_BUFFER_SIZE, &tech_pvt->prebuffer_len)) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to set channel pre buffer size.\n");
                    return SWITCH_STATUS_GENERR;        
                }
            }
            /*******************************************************************************************************/
        } else if (!strcasecmp(command, kECHOCANCEL)) {
            /*******************************************************************************************************/
            const char *szval = switch_event_get_header(event, kECHOCANCEL);
            int enabled = !!switch_true(szval);

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
                    "FreeTDM sending echo cancel [%s] command for %s:%d\n",
                    enabled ? "enable" : "disable",
                    ftdm_channel_get_span_name(tech_pvt->ftdm_channel), ftdm_channel_get_ph_id(tech_pvt->ftdm_channel));

            if (FTDM_SUCCESS != 
                ftdm_channel_command(tech_pvt->ftdm_channel, enabled ? FTDM_COMMAND_ENABLE_ECHOCANCEL : FTDM_COMMAND_DISABLE_ECHOCANCEL, NULL)) {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                        "Failed to %s echo cancellation.\n", enabled ? "enable" : "disable");
                return SWITCH_STATUS_FALSE;        
            }
            /*******************************************************************************************************/
        } else if (!strcasecmp(command, kDTMF_REMOVAL)) {
            /*******************************************************************************************************/
            const char *szval = switch_event_get_header(event, kDTMF_REMOVAL);
            int enabled = !!switch_true(szval);

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "FreeTDM sending DTMF-removal[%s] command for %s:%d\n",
                    enabled ? "enable" : "disable",
                    ftdm_channel_get_span_name(tech_pvt->ftdm_channel), ftdm_channel_get_ph_id(tech_pvt->ftdm_channel));

            if (FTDM_SUCCESS != 
                    ftdm_channel_command(tech_pvt->ftdm_channel, enabled ? FTDM_COMMAND_ENABLE_DTMF_REMOVAL : FTDM_COMMAND_DISABLE_DTMF_REMOVAL, 0)) {

                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                        "Failed to %s DTMF-removal \n", enabled ? "enable" : "disable");
                return SWITCH_STATUS_FALSE;        
            }

            /*******************************************************************************************************/
        }else {
            /*******************************************************************************************************/
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "FreeTDM received unknown command [%s] \n",command);
        }
    }

    return SWITCH_STATUS_SUCCESS;
}

static ftdm_status_t reload_span_config(void)
{
	/* 
	1. load span config file
	2. foreach span {
	3.     if (span not existing)  {
	4.          add new span and start signaling configuration;
	        }
	    }
	*/
	const char *cf = "freetdm.conf";
	ftdm_config_t span_cfg;
	char *var, *val;
	int catno = -1;
	ftdm_span_t *span = NULL;
	ftdm_status_t ret = FTDM_SUCCESS;
	ftdm_channel_config_t chan_config;
	unsigned d = 0, configured = 0;
	ftdm_size_t len = 0;
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Reload span configuration.\n");
	
	/* step 1: load span config file */
	if (!ftdm_config_open_file(&span_cfg, cf)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Not able to open span config file %s. \n", cf);
		return FTDM_FAIL;
	}
#ifdef RECONFIG_DBG
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Successfully opened span config file %s. \n", cf);
	}
#endif

	while (ftdm_config_next_pair(&span_cfg, &var, &val)) {
		if (*span_cfg.category == '#') {
			if (span_cfg.catno != catno) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Skipping %s\n", span_cfg.category);
				catno = span_cfg.catno;
			}
		} else if (!strncasecmp(span_cfg.category, "span", 4)) {
			if (span_cfg.catno != catno) {
				char *type = span_cfg.category + 4;
				char *name;
				
				if (*type == ' ') {
					type++;
				}
				
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "found config for span\n");
				catno = span_cfg.catno;
				
				if (ftdm_strlen_zero(type)) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "failure creating span, no type specified.\n");
					span = NULL;
					continue;
				}

				if ((name = strchr(type, ' '))) {
					*name++ = '\0';
				}

				/* Verify is trunk_type was specified for previous span */
				if (span && span->trunk_type == FTDM_TRUNK_NONE) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "trunk_type not specified for span %d (%s)\n", span->span_id, span->name);
					ret = FTDM_FAIL;
					goto done;
				}


				if (FTDM_FAIL == ftdm_span_find_by_name(name, &span) ) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Span [%s] was NOT found in current running configuration. \n Trying to create span. \n", name);
					
					if (ftdm_span_create(type, name, &span) == FTDM_SUCCESS) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "created span %d (%s) of type %s\n", span->span_id, span->name, type);
						d = 0;
						/* it is confusing that parameters from one span affect others, so let's clear them */
						memset(&chan_config, 0, sizeof(chan_config));
						sprintf(chan_config.group_name, "__default");
						/* default to storing iostats */
						chan_config.iostats = FTDM_TRUE;
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "failure creating span of type %s\n", type);
						span = NULL;
						continue;
					}
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Span [%s] was found in current running configuration. \n", name);
					span = NULL;
					continue;
				}
			}

			if (!span) {
				continue;
			}

			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "span %d [%s]=[%s]\n", span->span_id, var, val);
			
			
			if (!strcasecmp(var, "trunk_type")) {
				ftdm_trunk_type_t trtype = ftdm_str2ftdm_trunk_type(val);
				ftdm_span_set_trunk_type(span, trtype);
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "setting trunk type to '%s'\n", ftdm_trunk_type2str(trtype)); 
			} else if (!strcasecmp(var, "b-channel")) {
				unsigned chans_configured = 0;
				chan_config.type = FTDM_CHAN_TYPE_B;
				if (ftdm_configure_span_channels(span, val, &chan_config, &chans_configured) == FTDM_SUCCESS) {
					configured += chans_configured;
				}
			} else if (!strcasecmp(var, "group")) {
				len = strlen(val);
				if (len >= FTDM_MAX_NAME_STR_SZ) {
					len = FTDM_MAX_NAME_STR_SZ - 1;
					ftdm_log(FTDM_LOG_WARNING, "Truncating group name %s to %"FTDM_SIZE_FMT" length\n", val, len);
				}
				memcpy(chan_config.group_name, val, len);
				chan_config.group_name[len] = '\0';
			}  else if (!strcasecmp(var, "txgain")) {
				if (sscanf(val, "%f", &(chan_config.txgain)) != 1) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "invalid txgain: '%s'\n", val);
				}
			} else if (!strcasecmp(var, "rxgain")) {
				if (sscanf(val, "%f", &(chan_config.rxgain)) != 1) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "invalid rxgain: '%s'\n", val);
				}
			}			
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "unknown param [%s] '%s' / '%s'\n", span_cfg.category, var, val);
		}
	}
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_freetdm reload config, %d new channels have been configured. \n", configured);
	
done:
	ftdm_config_close_file(&span_cfg);

#ifdef RECONFIG_DBG
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Successfully closed span config file %s. \n", cf);
#endif

	return ret;
}
