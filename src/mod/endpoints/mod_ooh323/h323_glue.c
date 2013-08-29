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
 * h323_glue.c -- OOH323 Endpoint (code to tie ooh323c to FreeSWITCH)
 *
 */


#include "mod_ooh323.h"
#include "ooh245.h"

// #define VIDEO_MAX_BIT_RATE 192400 // magic huh? no bigger than this
#define VIDEO_MAX_BIT_RATE 3840

/** Global endpoint structure */
extern ooEndPoint gH323ep;
ooh323_profile_t *h323peer_global_profile;

/* Logicall channel callbacks */
static int startReceiveChannel(ooCallData *call, ooLogicalChannel *pChannel);
static int startTransmitChannel(ooCallData *call, ooLogicalChannel *pChannel);
static int stopReceiveChannel(ooCallData *call, ooLogicalChannel *pChannel);
static int stopTransmitChannel(ooCallData *call, ooLogicalChannel *pChannel);

static int startVideoReceiveChannel(ooCallData *call, ooLogicalChannel *pChannel);
static int startVideoTransmitChannel(ooCallData *call, ooLogicalChannel *pChannel);
static int stopVideoReceiveChannel(ooCallData *call, ooLogicalChannel *pChannel);
static int stopVideoTransmitChannel(ooCallData *call, ooLogicalChannel *pChannel);

/* Endpoint callbacks */
static int onIncomingCall(ooCallData* call);
static int onOutgoingCall(ooCallData* call);
static int onNewCallCreated(ooCallData* call);
static int onCallEstablished(ooCallData *call);
static int onCallCleared(ooCallData* call);
static int onAlerting(ooCallData* call);
static int onOpenLogicalChannels(ooCallData *call);
static int onReceivedDTMF(ooCallData *call, const char *dtmf);
static int onReceivedVideoFastUpdate(ooCallData *call, int channelNo);

/* H225 callbacks */
static int onReceivedSetup(ooCallData *call, Q931Message *pmsg);
static int onReceivedConnect(ooCallData *call, Q931Message *pmsg);
static int onBuiltSetup(ooCallData *call, Q931Message *pmsg);
static int onBuiltConnect(ooCallData *call, Q931Message *pmsg);

switch_status_t ep_add_codec_capability(char *codec_string);
switch_status_t ooh323_set_call_capability(ooCallData *call, char *codec_string, DTMF dtmf);
void set_media_info(ooCallData *call, switch_port_t port, switch_port_t video_port);
switch_status_t prepare_codec(ooh323_private_t *tech_pvt);
void ooh323_gen_remote_sdp(switch_core_session_t *session);
switch_bool_t check_acl(ooh323_profile_t *profile, char *ip);

/* ================= Logical channel callbacks ===================== */
/* Callback to start receive media channel */
static int startReceiveChannel (ooCallData *call, ooLogicalChannel *pChannel)
{
	ooH323EpCapability *cap;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Starting %s channel [%s]:%d (type:%d) at %s:%d(%d) <- %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	return OO_OK;
}

/* Callback to start transmit media channel */
static int startTransmitChannel (ooCallData *call, ooLogicalChannel *pChannel)
{
	ooH323EpCapability *cap;
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	ooh323_private_t *tech_pvt = switch_core_session_get_private(session);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Starting %s channel [%s]:%d (type:%d) at %s:%d(%d) -> %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	switch_mutex_lock(tech_pvt->mutex);

	switch_set_string(tech_pvt->local_rtp_ip, pChannel->localIP);
	switch_set_string(tech_pvt->remote_rtp_ip, pChannel->remoteIP);
	tech_pvt->remote_rtp_port = pChannel->remoteMediaPort;
	tech_pvt->acap = pChannel->chanCap;
	ooh323_gen_remote_sdp(session);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "H323 Remote SDP:\n%s\n", tech_pvt->remote_sdp_str);

	{
		uint8_t match = 0;
		uint8_t p = 0;

		match = switch_core_media_negotiate_sdp(session, tech_pvt->remote_sdp_str, &p, SDP_TYPE_REQUEST);
		if (match) {
			if (switch_core_media_activate_rtp(session) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Error activate RTP\n");
			} else {
				switch_set_flag_locked(tech_pvt, TFLAG_RTP);
				switch_channel_clear_flag(switch_core_session_get_channel(session), CF_AUDIO_PAUSE);
			}
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "SDP not match: %d\n", match);
		}
	}

	switch_mutex_unlock(tech_pvt->mutex);

	return OO_OK;
}

/* Callback to stop receive media channel */
static int stopReceiveChannel (ooCallData *call, ooLogicalChannel *pChannel)
{
	ooH323EpCapability *cap;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Stoping %s channel [%s]:%d (type:%d) at %s:%d(%d) <- %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	return OO_OK;
}

/* Callback to stop transmit media channel */
static int stopTransmitChannel (ooCallData *call, ooLogicalChannel *pChannel)
{
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	ooh323_private_t *tech_pvt = switch_core_session_get_private(session);
	ooH323EpCapability *cap;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Stoping %s channel [%s]:%d (type:%d) at %s:%d(%d) -> %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	switch_clear_flag_locked(tech_pvt, TFLAG_RTP);
	switch_core_session_kill_channel(session, SWITCH_SIG_KILL);
	// switch_core_media_deactivate_rtp(session);

	return OO_OK;
}

/* Callback to start video receive media channel */
static int startVideoReceiveChannel(ooCallData *call, ooLogicalChannel *pChannel)
{
	ooH323EpCapability *cap;
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	ooh323_private_t *tech_pvt = switch_core_session_get_private(session);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Starting Video %s channel [%s]:%d (type:%d) at %s:%d(%d) <- %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		if (cap->cap == OO_H264VIDEO && cap->params) {
			OOH264CapParams *params = cap->params;
			if (params->pt) {
				tech_pvt->video_pt = params->pt;
				// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "H264 pt: %d\n", params->pt);
			}
		}
		cap = cap->next;
	}

	return OO_OK;
}

/* Callback to start video transmit media channel */
static int startVideoTransmitChannel (ooCallData *call, ooLogicalChannel *pChannel)
{
	ooH323EpCapability *cap;
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	ooh323_private_t *tech_pvt = switch_core_session_get_private(session);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Starting Video %s channel [%s]:%d (type:%d) at %s:%d(%d) -> %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);

		if (cap->cap == OO_H264VIDEO && cap->params) { // TODO: figure out cap->params is always NULL
			OOH264CapParams *params = cap->params;
			if (params->pt) {
				tech_pvt->video_pt = params->pt;
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "H264 pt: %d\n", params->pt);
			}
		}

		cap = cap->next;
	}

	switch_mutex_lock(tech_pvt->mutex);

	switch_set_string(tech_pvt->local_rtp_ip, pChannel->localIP);
	switch_set_string(tech_pvt->remote_rtp_ip, pChannel->remoteIP);
	tech_pvt->remote_video_rtp_port = pChannel->remoteMediaPort;
	tech_pvt->vcap = pChannel->chanCap;
	ooh323_gen_remote_sdp(session);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "H323 Remote SDP:\n%s\n", tech_pvt->remote_sdp_str);

	{
		uint8_t match = 0;
		uint8_t p = 0;

		/* Set to re-negotiate media when we build video sdp */
		switch_channel_set_flag(channel, CF_REINVITE);
		switch_media_handle_set_media_flag(tech_pvt->media_handle, SCMF_RENEG_ON_REINVITE);

		match = switch_core_media_negotiate_sdp(session, tech_pvt->remote_sdp_str, &p, SDP_TYPE_REQUEST);
		if (match) {
			if (switch_core_media_activate_rtp(session) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Error activate RTP\n");
			} else {
				switch_channel_clear_flag(switch_core_session_get_channel(session), CF_VIDEO_PAUSE);
			}
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "SDP not match: %d\n", match);
		}
	}

	switch_core_memory_pool_set_data(switch_core_session_get_pool(session), "__session", session);
	switch_channel_set_flag(channel, CF_VIDEO);

	switch_mutex_unlock(tech_pvt->mutex);

	return OO_OK;
}

/* Callback to stop video receive media channel */
static int stopVideoReceiveChannel (ooCallData *call, ooLogicalChannel *pChannel)
{
	ooH323EpCapability *cap;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Stoping Video %s channel [%s]:%d (type:%d) at %s:%d(%d) <- %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	return OO_OK;
}

/* Callback to stop transmit media channel */
static int stopVideoTransmitChannel (ooCallData *call, ooLogicalChannel *pChannel)
{
	// switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	// ooh323_private_t *tech_pvt = switch_core_session_get_private(session);
	ooH323EpCapability *cap;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"Stoping Video %s channel [%s]:%d (type:%d) at %s:%d(%d) -> %s:%d(%d)\n",
		pChannel->dir, call->callToken, pChannel->channelNo, pChannel->type,
		pChannel->localIP, pChannel->localRtpPort, pChannel->localRtcpPort,
		pChannel->remoteIP, pChannel->remoteMediaPort, pChannel->remoteMediaControlPort);

	for (cap = pChannel->chanCap; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	return OO_OK;
}

/* ================= H323 Endpoint callbacks ======================= */

/* Callback to process alerting signal */
static int onAlerting (ooCallData* call)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "onAlerting - %s\n", call->callToken);

	/* TODO: user would add application specific logic here to handle   */
	/* an H.225 alerting message..               */

	return OO_OK;
}

/* Callback to handle an incoming call request */
static int onIncomingCall (ooCallData* call)
{
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	switch_channel_t *channel;
	ooh323_private_t *tech_pvt = NULL;
	char number[OO_MAX_NUMBER_LENGTH];

	assert(session != NULL);

	channel = switch_core_session_get_channel(session);
	tech_pvt = switch_core_session_get_private(session);

	assert(channel);
	assert(tech_pvt);

	tech_pvt->dialplan = h323peer_global_profile->dialplan;
	tech_pvt->context  = h323peer_global_profile->context;

	if (call->remoteDisplayName) {
		tech_pvt->caller_id_name = switch_core_session_strdup(session, call->remoteDisplayName);
	}

	if (ooCallGetCallingPartyNumber(call, number, OO_MAX_NUMBER_LENGTH) == OO_OK) {
		tech_pvt->caller_id_number = switch_core_session_strdup(session, number);
	} else

	if (call->remoteAliases) {
		ooAliases *alias;
		char var[64];

		for (alias = call->remoteAliases; alias; alias = alias->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "remoteAlices: type=%d, value=%s\n", alias->type, alias->value);
			snprintf(var, 64, "h323_remote_aliases_%d", alias->type);
			switch_channel_set_variable_printf(channel, var, alias->value);
			if (alias->type == T_H225AliasAddress_h323_ID) {
				if (!tech_pvt->caller_id_name) {
					tech_pvt->caller_id_name = switch_core_session_strdup(session, alias->value);
				}
			} else if (alias->type == T_H225AliasAddress_dialedDigits) {
				if (!tech_pvt->caller_id_number) {
					tech_pvt->caller_id_number = switch_core_session_strdup(session, alias->value);
				}
			} else if (alias->type == T_H225AliasAddress_email_ID) {
				tech_pvt->caller_email = switch_core_session_strdup(session, alias->value);
			} else if (alias->type == T_H225AliasAddress_url_ID) {
				tech_pvt->caller_url = switch_core_session_strdup(session, alias->value);
			}
		}
	}

	if (!tech_pvt->caller_id_name) {
		tech_pvt->caller_id_name = switch_core_session_strdup(session, call->remoteIP);
	}

	if (!tech_pvt->caller_id_number) tech_pvt->caller_id_number = "anonymous";

	number[0] = '\0';

	if (ooCallGetCalledPartyNumber(call, number, OO_MAX_NUMBER_LENGTH) == OO_OK) {
		tech_pvt->destination_number = switch_core_session_strdup(session, call->calledPartyNumber);
	} else {
		if (call->ourAliases) {
			ooAliases *alias;
			char var[64];

			for (alias = call->ourAliases; alias; alias = alias->next) {
				snprintf(var, 64, "h323_our_aliases_%d", alias->type);
				switch_channel_set_variable_printf(channel, var, alias->value);
				if (alias->type == T_H225AliasAddress_h323_ID) {
					if (!tech_pvt->destination_number) {
						tech_pvt->destination_number = switch_core_session_strdup(session, alias->value);
					}
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "ourAlices: type=%d, value=%s\n", alias->type, alias->value);
				}
			}
		}

		if (!tech_pvt->destination_number) {
			tech_pvt->destination_number = h323peer_global_profile->extn;
		}

		if (zstr(tech_pvt->destination_number)) {
			tech_pvt->destination_number = "h323";
		}
	}

	/* disable gk for simple*/
	OO_SETFLAG(call->flags, OO_M_DISABLEGK);

	tech_pvt->name = switch_core_session_strdup(session, call->callToken);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "onIncomingCall from \"%s\"(%s) ----> %s\n",
		tech_pvt->caller_id_name,
		tech_pvt->caller_id_number, call->callToken);

	if (!check_acl(tech_pvt->profile, call->remoteIP)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "IP %s is rejected from ACL\n", call->remoteIP);
		ooh323_hangup_call(tech_pvt); // do we really need this?
		return OO_FAILED;
	}

	switch_core_session_add_stream(session, NULL);

	if ((tech_pvt->caller_profile =
		switch_caller_profile_new(switch_core_session_get_pool(session), "ooh323",
			tech_pvt->dialplan, tech_pvt->caller_id_name,
			tech_pvt->caller_id_number, NULL, NULL, NULL, NULL, "mod_ooh323",
			tech_pvt->context, tech_pvt->destination_number)) != 0) {
		char *name = switch_mprintf("ooh323/%s", tech_pvt->name);
		switch_channel_set_name(channel, name);
		switch_safe_free(name);
		switch_channel_set_caller_profile(channel, tech_pvt->caller_profile);

		switch_channel_set_variable(channel, "h323_call_token", call->callToken);
		switch_channel_set_variable(channel, "h323_call_type", call->callType);
		switch_channel_set_variable_printf(channel, "h323_call_mode", "%d", call->callMode);
		switch_channel_set_variable_printf(channel, "h323_call_reference", "%d", call->callReference);
		switch_channel_set_variable(channel, "h323_our_caller_id", call->ourCallerId);
		// switch_channel_set_variable(channel, "h323_our_identifer", callIdentifier.data);
		if (call->callingPartyNumber) {
			switch_channel_set_variable(channel, "h323_calling_party_number", call->callingPartyNumber);
		}
		if (call->calledPartyNumber) {
			switch_channel_set_variable(channel, "h323_called_party_number", call->calledPartyNumber);
		}
		switch_channel_set_variable_printf(channel, "h323_flags", "%d", call->flags);
		switch_channel_set_variable_printf(channel, "h323_flags_hex", "0x%x", call->flags);
		switch_channel_set_variable_printf(channel, "h323_h245listenport", "%d", call->h245listenport);
		switch_channel_set_variable(channel, "h323_local_ip", call->localIP);
		switch_channel_set_variable(channel, "h323_remote_ip", call->remoteIP);
		switch_channel_set_variable_printf(channel, "h323_remote_port", "%d", call->remotePort);
		switch_channel_set_variable_printf(channel, "h323_remote_h245_port", "%d", call->remoteH245Port);
		switch_channel_set_variable(channel, "h323_remote_display_name", call->remoteDisplayName);
		switch_channel_set_variable_printf(channel, "h323_next_session_id", "%u", call->nextSessionID);

	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error allocating caller profile for %s\n", call->callToken);
		switch_core_session_destroy(&session);
		return OO_FAILED;
	}

	{
		int i;
		ooH323EpCapability *cap;

		for (cap = call->ourCaps; cap; ) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ourCaps: dir=%d, cap=%s(%d), capType=%d\n",
				cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
			cap = cap->next;
		}

		for (cap = call->remoteCaps; cap; ) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "remoteCaps: dir=%d, cap=%s(%d), capType=%d\n",
				cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
			cap = cap->next;
		}

		for (cap = call->jointCaps; cap; ) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "jointCaps: dir=%d, cap=%s(%d), capType=%d\n",
				cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
			cap = cap->next;
		}

		for (i = 0; i<call->capPrefs.index; i++) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "prefs[%d]: %d\n", i, call->capPrefs.order[i]);
		}

		ooh323_set_call_capability(call, tech_pvt->profile->inbound_codec_string, H323_DTMF_RFC2833);

	}

	// core media
	switch_channel_set_flag(channel, CF_VIDEO_POSSIBLE);
	switch_channel_set_variable(channel, "codec_string", tech_pvt->profile->inbound_codec_string);
	switch_core_media_prepare_codecs(session, SWITCH_TRUE);
	tech_pvt->mparams.local_sdp_str = NULL;

	switch_core_media_choose_port(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO, 0);
	switch_core_media_choose_port(tech_pvt->session, SWITCH_MEDIA_TYPE_VIDEO, 0);
	/* ? We should use SDP_TYPE_RESPONSE here but it doesn't generate multiple codecs */
	switch_core_media_gen_local_sdp(session, SDP_TYPE_REQUEST, NULL, 0, NULL, 0);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "H323 Local SDP:\n%s\n", tech_pvt->mparams.local_sdp_str);

	// core media end

	tech_pvt->local_rtp_port = atoi(switch_channel_get_variable(channel, SWITCH_LOCAL_MEDIA_PORT_VARIABLE));
	tech_pvt->local_video_rtp_port = atoi(switch_channel_get_variable(channel, SWITCH_LOCAL_VIDEO_PORT_VARIABLE));

	set_media_info(call, tech_pvt->local_rtp_port, tech_pvt->local_video_rtp_port);
	if (0) prepare_codec(tech_pvt); // we should not need this anymore

	/* indicate ooh323 attached to this session */
	switch_set_flag_locked(tech_pvt, TFLAG_SIG);

	/* prevent core io read from us before we are ready */
	switch_channel_set_flag(channel, CF_AUDIO_PAUSE);
	switch_channel_set_flag(channel, CF_VIDEO_PAUSE);

	/* ready to go */
	switch_channel_set_state(channel, CS_INIT);

	if (switch_core_session_thread_launch(session) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Error spawning thread\n");
		switch_core_session_destroy(&session);
		return OO_FAILED;
	}

	return OO_OK;
}

static int onOutgoingCall (ooCallData* call)
{
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	ooh323_private_t *tech_pvt = NULL;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "onOutgoingCall - %s\n", call->callToken);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt);


	ooh323_set_call_capability(call, tech_pvt->profile->outbound_codec_string, H323_DTMF_RFC2833);

	return OO_OK;
}

/* Callback to handle outgoing call admitted */
static int onNewCallCreated (ooCallData* call)
{
	switch_core_session_t *session = NULL;
	switch_channel_t *channel = NULL;
	ooh323_private_t *tech_pvt;
	switch_call_direction_t direction;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "onNewCallCreated - %s\n", call->callToken);
	if (!strncasecmp(call->callType, "incoming", 8)) {
		direction = SWITCH_CALL_DIRECTION_INBOUND;

		session = switch_core_session_request(ooh323_endpoint_interface, direction, SOF_NONE, NULL);
		if (session == NULL) return OO_FAILED;

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,"New session, uuid = %s\n", switch_core_session_get_uuid(session));

		call->usrData = session;
		channel = switch_core_session_get_channel(session);

		if (channel == NULL) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Channel is NULL\n");
			switch_core_session_destroy(&session);
			return OO_FAILED;
		}

		tech_pvt = switch_core_session_alloc(session, sizeof(ooh323_private_t));

		if (ooh323_tech_init(tech_pvt, session) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "tech_init ERROR, where is my memeory pool?\n");
			switch_core_session_destroy(&session);
			return OO_FAILED;
		}

		switch_set_flag(tech_pvt, TFLAG_INBOUND);
		ooh323_attach_private(session, h323peer_global_profile, tech_pvt);

	} else {
		direction = SWITCH_CALL_DIRECTION_OUTBOUND;
		assert(call->usrData);

		session = switch_core_session_locate(call->usrData);

		if (!session) return OO_FAILED;

		call->usrData = session;

		channel = switch_core_session_get_channel(session);
		tech_pvt = switch_core_session_get_private(session);
		assert(channel);
		assert(tech_pvt);

		// core media
		switch_channel_set_variable(channel, "codec_string", tech_pvt->profile->outbound_codec_string);
		switch_core_media_prepare_codecs(session, SWITCH_TRUE);
		tech_pvt->mparams.local_sdp_str = NULL;

		switch_core_media_choose_port(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO, 0);
		switch_core_media_choose_port(tech_pvt->session, SWITCH_MEDIA_TYPE_VIDEO, 0);
		switch_core_media_gen_local_sdp(session, SDP_TYPE_REQUEST, NULL, 0, NULL, 0);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "H323 Local SDP:\n%s\n", tech_pvt->mparams.local_sdp_str);
		// core media end

		tech_pvt->local_rtp_port = atoi(switch_channel_get_variable(channel, SWITCH_LOCAL_MEDIA_PORT_VARIABLE));
		tech_pvt->local_video_rtp_port = atoi(switch_channel_get_variable(channel, SWITCH_LOCAL_VIDEO_PORT_VARIABLE));

		set_media_info(call, tech_pvt->local_rtp_port, tech_pvt->local_video_rtp_port);
		// prepare_codec(tech_pvt);

		switch_set_flag(tech_pvt, TFLAG_SIG);

		switch_core_session_rwunlock(session);
	}

	tech_pvt->h323_call = call;
	tech_pvt->h323_callToken = call->callToken;

	if(h323peer_global_profile->dtmf_type & H323_DTMF_RFC2833) {
		ooCallEnableDTMFRFC2833(call, h323peer_global_profile->te);
	} else if(h323peer_global_profile->dtmf_type & H323_DTMF_H245ALPHANUMERIC) {
		ooCallEnableDTMFH245Alphanumeric(call);
	} else if(h323peer_global_profile->dtmf_type & H323_DTMF_H245SIGNAL) {
		ooCallEnableDTMFH245Signal(call);
	}

	return OO_OK;
}

/* Callback to handle call established event */
static int onCallEstablished(ooCallData *call)
{
	int i;
	ooH323EpCapability *cap;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "onCallEstablished - %s\n", call->callToken);

	for (cap = call->ourCaps; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ourCaps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	for (cap = call->remoteCaps; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "remoteCaps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	for (cap = call->jointCaps; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "jointCaps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	for (i = 0; i<call->capPrefs.index; i++) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "prefs[%d]: %d\n", i, call->capPrefs.order[i]);
	}

	return OO_OK;
}

/* Callback to handle call established event */
static __attribute__ ((unused)) int onOpenLogicalChannels(ooCallData *call)
{
	int i;
	ooH323EpCapability *cap;
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	ooh323_private_t *tech_pvt;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "onOpenLogicalChannels - %s\n", call->callToken);

	for (cap = call->ourCaps; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ourCaps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	for (cap = call->remoteCaps; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "remoteCaps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	for (cap = call->jointCaps; cap; ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "jointCaps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);
		cap = cap->next;
	}

	for (i = 0; i<call->capPrefs.index; i++) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "prefs[%d]: %d\n", i, call->capPrefs.order[i]);
	}

	if (!tech_pvt->audio_started) {
		ooOpenLogicalChannel(call, OO_CAP_TYPE_AUDIO);
		tech_pvt->audio_started = SWITCH_TRUE;
	}

	if (!tech_pvt->video_started) {
		ooOpenLogicalChannel(call, OO_CAP_TYPE_VIDEO);
		tech_pvt->video_started = SWITCH_TRUE;
	}

	if (0) onOpenLogicalChannels(call); // prevent compiler warning if we don't use this callback

	return OO_OK;
}

static int onReceivedDTMF(ooCallData *call, const char *dtmf)
{
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	switch_channel_t *channel = switch_core_session_get_channel(session);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "DTMF:%s %s\n", dtmf, call->callToken);

	assert(channel);

	switch_channel_queue_dtmf_string(channel, dtmf);

	return OO_OK;
}

static int onReceivedVideoFastUpdate(ooCallData *call, int channelNo)
{
	switch_core_session_t *session, *nsession;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
		"Received videoFastUpdate %s channelNo=%d\n", call->callToken, channelNo);

	session = (switch_core_session_t *)call->usrData;
	assert(session);

	switch_core_session_get_partner(session, &nsession);

	if (nsession) {
		switch_core_session_refresh_video(nsession);
		switch_core_session_rwunlock(nsession);
	}

	return OO_OK;
}

/* Callback to handle call cleared */
static int onCallCleared (ooCallData* call)
{
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	ooh323_private_t *tech_pvt;

	/* Caution: Everything can be invalid if the session is gone,
	   We try to prevent this from happening by the TFLAG_SIG flag, hope it works
	 */

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
		"onCallCleared -(call end reason %d:%s ) -- %s\n",
	call->callEndReason, ooGetReasonCodeText(call->callEndReason), call->callToken);

	tech_pvt = switch_core_session_get_private(session);

	assert(channel != NULL);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "hangup channel %s %s\n",
		switch_core_session_get_uuid(session), call->callToken);
	switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);

	switch_clear_flag_locked(tech_pvt, TFLAG_SIG);

	return OO_OK;
}

/* ========================== H225 callbacks ======================= */

static int onReceivedSetup(ooCallData *call, Q931Message *pmsg)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "OOH323 onReceivedSetup %s\n", call->callToken);
	return OO_OK;
}

static int onReceivedConnect(ooCallData *call, Q931Message *pmsg)
{
	switch_core_session_t *session = (switch_core_session_t *)call->usrData;
	switch_channel_t *channel;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "OOH323 onReceivedConnect %s\n", call->callToken);
	assert(session);

	channel = switch_core_session_get_channel(session);
	assert(channel);

	switch_channel_mark_answered(channel);

	return OO_OK;
}

static int onBuiltSetup(ooCallData *call, Q931Message *pmsg)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "OOH323 onBuiltSetup %s\n", call->callToken);
	return OO_OK;
}

static int onBuiltConnect(ooCallData *call, Q931Message *pmsg)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "OOH323 onBuiltConnect %s\n", call->callToken);
	return OO_OK;
}

/* functions and wrappers for low level ooh323c functions */

switch_status_t ooh323_start(ooh323_profile_t *profile)
{
	OOH323CALLBACKS h323Callbacks = { 0 };
	OOH225MsgCallbacks h225Callbacks = { 0 };
	int ret = 0;

	h323peer_global_profile = profile;

#ifdef _WIN32
	ooSocketsInit (); /* Initialize the windows socket api  */
#endif

	/* Initialize the H323 endpoint - faststart and tunneling enabled by default*/
	ret = ooH323EpInitialize(profile->call_mode, profile->logfile);
	if (ret != OO_OK) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to initialize H.323 endpoint\n");
		return SWITCH_STATUS_FALSE;
	}

	if (!profile->is_fast_start) ooH323EpDisableFastStart();
	if (!profile->is_tunneling) ooH323EpDisableH245Tunneling();

	if (profile->trace_level == 1) {
		ooH323EpSetTraceLevel(OOTRCLVLDBGA);
	} else if(profile->trace_level == 2) {
		ooH323EpSetTraceLevel(OOTRCLVLDBGB);
	} else if(profile->trace_level >= 3) {
		ooH323EpSetTraceLevel(OOTRCLVLDBGC);
	}

	if(!ooUtilsIsStrEmpty(profile->user)) ooH323EpSetCallerID(profile->user);

	if(!ooUtilsIsStrEmpty(profile->h323id)) ooH323EpAddAliasH323ID(profile->h323id);

	if(!ooUtilsIsStrEmpty(profile->user_num)) ooH323EpSetCallingPartyNumber(profile->user_num);

	if(profile->tcpmax != 0 && profile->tcpmin != 0 && profile->tcpmin < profile->tcpmax) {
		ooH323EpSetTCPPortRange(profile->tcpmin, profile->tcpmax);
	}

	if(profile->udpmin != 0 && profile->udpmax != 0 && profile->udpmin < profile->udpmax) {
		ooH323EpSetUDPPortRange(profile->udpmin, profile->udpmax);
	}

	if(profile->rtpmin != 0 && profile->rtpmax != 0 && profile->rtpmin  < profile->rtpmax) {
		ooH323EpSetRTPPortRange(profile->rtpmin, profile->rtpmax);
	}

	ooH323EpSetLocalAddress(profile->listen_ip, profile->listen_port);

	/* Make sure that multiple instances of h323peer use separate cmdPort*/
#ifdef _WIN32
	ooH323EpCreateCmdListener(profile->cmdPort);
#endif
	/* Register callbacks */
	h323Callbacks.onNewCallCreated = onNewCallCreated;
	h323Callbacks.onAlerting = onAlerting;
	h323Callbacks.onIncomingCall = onIncomingCall;
	h323Callbacks.onOutgoingCall = onOutgoingCall;
	h323Callbacks.onCallEstablished = onCallEstablished;
	h323Callbacks.onCallCleared = onCallCleared;
	h323Callbacks.openLogicalChannels = NULL;//onOpenLogicalChannels;
	h323Callbacks.onReceivedVideoFastUpdate = onReceivedVideoFastUpdate;
	h323Callbacks.onReceivedDTMF = onReceivedDTMF;
	h323Callbacks.onReceivedCommand = NULL;
	ooH323EpSetH323Callbacks(h323Callbacks);

	h225Callbacks.onReceivedSetup = onReceivedSetup;
	h225Callbacks.onReceivedConnect = onReceivedConnect;
	h225Callbacks.onBuiltSetup = onBuiltSetup;
	h225Callbacks.onBuiltConnect = onBuiltConnect;

	ooH323EpSetH225MsgCallbacks(h225Callbacks);
	// ooH323EpSetAsGateway();
	ooH323EpDisableAutoAnswer();

	if(profile->dtmf_type & H323_DTMF_RFC2833) {
		ooH323EpEnableDTMFRFC2833(0);
	} else if(profile->dtmf_type & H323_DTMF_H245ALPHANUMERIC) {
		ooH323EpEnableDTMFH245Alphanumeric();
	} else if(profile->dtmf_type & H323_DTMF_H245SIGNAL) {
		ooH323EpEnableDTMFH245Signal();
	}

	/* Add audio/video capability */
	ep_add_codec_capability(profile->inbound_codec_string);

	if(profile->gk_mode != RasNoGatekeeper) {
		if(profile->gk_mode == RasDiscoverGatekeeper) {
			ret = ooGkClientInit(RasDiscoverGatekeeper, NULL, 0);
		} else if(profile->gk_mode == RasUseSpecificGatekeeper) {
			ret = ooGkClientInit(RasUseSpecificGatekeeper, profile->gk_ip, profile->gk_port);
		}

		if(ret != OO_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"Failed to initialize gatekeeper client\n");
			return SWITCH_STATUS_FALSE;
		}
	}


	/* Gatekeeper */
	if (profile->gk_mode == RasUseSpecificGatekeeper) {
		ooGkClientInit(RasUseSpecificGatekeeper, profile->gk_ip, profile->gk_port);
		// ooGkClientSetGkId(profile->gk_id);
	} else if (profile->gk_mode == RasDiscoverGatekeeper) {
		ooGkClientInit(RasDiscoverGatekeeper, 0, 0);
		ooGkClientSetBandwidth(0); //unlimited
	}

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_listener_loop(ooh323_profile_t *profile)
{
	int ret = 0;

	/* Create H.323 Listener */
	ret = ooCreateH323Listener();
	if (ret != OO_OK) {
		OOTRACEERR1 ("Failed to Create H.323 Listener");
		return SWITCH_STATUS_FALSE;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "ooh323(%s) listening on %s:%d\n",
		profile->call_mode == OO_CALLMODE_AUDIOCALL ? "audio" : (
			profile->call_mode == OO_CALLMODE_AUDIORX ? "audiorx" : (
				profile->call_mode == OO_CALLMODE_VIDEOCALL ? "video" : "unknown callmode")),
		profile->listen_ip, profile->listen_port);

	/* Loop forever to process events */
	ooMonitorChannels();

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
		"ooh323 shuting down %s:%d\n", profile->listen_ip, profile->listen_port);

	ooH323EpDestroy();

	return SWITCH_STATUS_SUCCESS;
}

void ooh323_stop(ooh323_private_t *profile){
	ooStopMonitor();
}

switch_status_t ooh323_make_call(ooh323_private_t *tech_pvt)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	OOStkCmdStat h323_stat;
	ooCallOptions opts = { 0 };
	char callToken[128] = { 0 };

	opts.callMode = OO_CALLMODE_VIDEOCALL;
	opts.usrData = (void *)switch_core_session_get_uuid(tech_pvt->session);

	switch_mutex_lock(tech_pvt->profile->mutex);

	if((h323_stat = ooMakeCall(tech_pvt->destination_number, callToken, sizeof(callToken), &opts)) != OO_STKCMD_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
			"Error making h323 call to %s, reason: %s\n", tech_pvt->destination_number, ooGetStkCmdStatusCodeTxt(h323_stat));
		switch_clear_flag_locked(tech_pvt, TFLAG_SIG);
		status = SWITCH_STATUS_FALSE;
		switch_mutex_unlock(tech_pvt->profile->mutex);
		goto end;
	}

	switch_mutex_unlock(tech_pvt->profile->mutex);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "new callToken: %s\n", callToken);
	tech_pvt->h323_callToken = switch_core_session_strdup(tech_pvt->session, callToken);
	tech_pvt->name = switch_core_session_strdup(tech_pvt->session, callToken);

	status = SWITCH_STATUS_SUCCESS;

end:
	return status;
}

switch_status_t ooh323_answer_call(ooh323_private_t *tech_pvt)
{
	switch_status_t status;
	OOStkCmdStat h323_stat;

	switch_mutex_lock(tech_pvt->profile->mutex);

	if((h323_stat = ooAnswerCall(tech_pvt->h323_callToken)) == OO_STKCMD_SUCCESS) {
		status = SWITCH_STATUS_SUCCESS;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
			"H323 Answer Error %s\n", ooGetStkCmdStatusCodeTxt(h323_stat));
		status = SWITCH_STATUS_FALSE;
	}

	switch_mutex_unlock(tech_pvt->profile->mutex);

	return status;
}

switch_status_t ooh323_manual_ringback(ooh323_private_t *tech_pvt)
{
	switch_mutex_lock(tech_pvt->profile->mutex);

	ooManualRingback(tech_pvt->h323_callToken);

	switch_mutex_unlock(tech_pvt->profile->mutex);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_video_refresh(ooh323_private_t *tech_pvt)
{
	OOStkCmdStat h323_stat;

	switch_mutex_lock(tech_pvt->profile->mutex);

	if ((h323_stat = ooSendVideoFastUpdate(tech_pvt->h323_callToken)) != OO_STKCMD_SUCCESS) {
		switch_mutex_unlock(tech_pvt->profile->mutex);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
			"H323 Answer Error %s\n", ooGetStkCmdStatusCodeTxt(h323_stat));
		return SWITCH_STATUS_FALSE;
	}

	switch_mutex_unlock(tech_pvt->profile->mutex);
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_hangup_call(ooh323_private_t *tech_pvt)
{
	switch_mutex_lock(tech_pvt->profile->mutex);

	ooHangCall(tech_pvt->h323_callToken, OO_REASON_LOCAL_CLEARED);

	switch_mutex_unlock(tech_pvt->profile->mutex);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_attach_private(switch_core_session_t *session, ooh323_profile_t *profile, ooh323_private_t *tech_pvt)
{
	switch_assert(session != NULL);
	switch_assert(tech_pvt != NULL);

	// switch_core_session_add_stream(session, NULL);

	switch_mutex_lock(tech_pvt->flag_mutex);

	tech_pvt->profile = profile;
	tech_pvt->mparams.rtpip = switch_core_session_strdup(session, profile->listen_ip);
	profile->inuse++;

	// switch_mutex_unlock(profile->flag_mutex);
	switch_mutex_unlock(tech_pvt->flag_mutex);

	if (tech_pvt->bte) {
		tech_pvt->recv_te = tech_pvt->te = tech_pvt->bte;
	} else if (!tech_pvt->te) {
		tech_pvt->mparams.recv_te = tech_pvt->mparams.te = profile->te;
	}

	tech_pvt->mparams.dtmf_type = profile->dtmf_type == H323_DTMF_RFC2833 ? DTMF_2833 : DTMF_NONE;

	tech_pvt->session = session;
	tech_pvt->channel = switch_core_session_get_channel(session);

	// switch_core_media_check_dtmf_type(session);
	// switch_channel_set_cap(tech_pvt->channel, CC_MEDIA_ACK);
	// switch_channel_set_cap(tech_pvt->channel, CC_BYPASS_MEDIA);
	// switch_channel_set_cap(tech_pvt->channel, CC_PROXY_MEDIA);
	// switch_channel_set_cap(tech_pvt->channel, CC_JITTERBUFFER);
	switch_channel_set_cap(tech_pvt->channel, CC_FS_RTP);
	// switch_channel_set_cap(tech_pvt->channel, CC_QUEUEABLE_DTMF_DELAY);
	// tech_pvt->mparams.ndlb = tech_pvt->profile->mndlb;
	tech_pvt->mparams.inbound_codec_string = profile->inbound_codec_string;
	tech_pvt->mparams.outbound_codec_string = profile->outbound_codec_string;
	// tech_pvt->mparams.auto_rtp_bugs = profile->auto_rtp_bugs;
	tech_pvt->mparams.timer_name = profile->timer_name;
	// tech_pvt->mparams.vflags = profile->vflags;
	// tech_pvt->mparams.manual_rtp_bugs = profile->manual_rtp_bugs;
	// tech_pvt->mparams.manual_video_rtp_bugs = profile->manual_video_rtp_bugs;
	// tech_pvt->mparams.extsipip = profile->extsipip;
	// tech_pvt->mparams.extrtpip = profile->extrtpip;
	// tech_pvt->mparams.local_network = profile->local_network;
	// tech_pvt->mparams.sipip = profile->sipip;
	// tech_pvt->mparams.jb_msec = profile->jb_msec;
	// tech_pvt->mparams.rtcp_audio_interval_msec = profile->rtcp_audio_interval_msec;
	// tech_pvt->mparams.rtcp_video_interval_msec = profile->rtcp_video_interval_msec;
	// tech_pvt->mparams.sdp_username = profile->sdp_username;
	// tech_pvt->mparams.cng_pt = tech_pvt->cng_pt;
	// tech_pvt->mparams.rtp_timeout_sec = profile->rtp_timeout_sec;
	// tech_pvt->mparams.rtp_hold_timeout_sec = profile->rtp_hold_timeout_sec;

	switch_media_handle_create(&tech_pvt->media_handle, session, &tech_pvt->mparams);

	if (profile->disable_rtp_autoadj) {
		switch_media_handle_set_media_flag(tech_pvt->media_handle, SCMF_DISABLE_RTP_AUTOADJ);
	}

	switch_core_session_set_private(session, tech_pvt);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_tech_init(ooh323_private_t *tech_pvt, switch_core_session_t *session)
{

	switch_assert(session != NULL);

	if (tech_pvt == NULL) return SWITCH_STATUS_FALSE;

	memset(tech_pvt, 0, sizeof(*tech_pvt));
	tech_pvt->read_frame.data = tech_pvt->databuf;
	tech_pvt->read_frame.buflen = sizeof(tech_pvt->databuf);
	switch_mutex_init(&tech_pvt->mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
	switch_mutex_init(&tech_pvt->flag_mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
	tech_pvt->session = session;

	return SWITCH_STATUS_SUCCESS;
}

#define CODEC_CALLBACKS &startReceiveChannel, &startTransmitChannel, &stopReceiveChannel, &stopTransmitChannel
switch_status_t ep_add_codec_capability(char *codec_string)
{
	char *mydata = NULL, *argv[20];
	int argc;
	int i = 0;
	char *codec;
	char *rate;
	// char *ptime;
	int ret = 0;

	mydata = strdup(codec_string);
	argc = switch_separate_string(mydata, ',', argv, (sizeof(argv) / sizeof(argv[0])));

	for (i = 0; i < argc; i++) {
		codec = argv[i];
		if ((rate = strchr(codec, '@'))) {
			*rate++ = '\0';
			if (strchr(rate, 'h')) {
				//todo
			} else {
				//todo
			}
		}

		if (!strcasecmp("PCMU", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
			ret |= ooH323EpAddG711Capability(OO_G711ULAW64K, 20, 20, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("PCMA", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
			ret |= ooH323EpAddG711Capability(OO_G711ALAW64K, 20, 20, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("G729", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
		    ret |= ooH323EpAddG729Capability(OO_G729A, 2, 24, OORXANDTX, CODEC_CALLBACKS);
		// } else if (strcasecmp, "G729b", codec) {
		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
		// 	ret |= ooH323EpAddG729Capability(OO_G729, 2, 24, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("G723", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
			ret |= ooH323EpAddG7231Capability(OO_G7231, 4, 7, FALSE, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("GSM", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
			ret |= ooH323EpAddGSMCapability(OO_GSMFULLRATE, 4, FALSE, FALSE, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("H263", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
			ret |= ooH323EpAddH263VideoCapability(OO_H263VIDEO, 1, 1, 1, 0, 0,
				VIDEO_MAX_BIT_RATE, OORXANDTX,
				&startVideoReceiveChannel, &startVideoTransmitChannel,
				&stopVideoReceiveChannel, &stopVideoTransmitChannel);
		} else if (!strcasecmp("H264", codec)) {
			OOH264CapParams params = { 0 };
			params.maxBitRate = VIDEO_MAX_BIT_RATE;
			// params.pt = 109;

			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);

			ret |= ooH323EpAddH264VideoCapability(OO_H264VIDEO, &params, OORXANDTX,
				&startVideoReceiveChannel, &startVideoTransmitChannel,
				&stopVideoReceiveChannel, &stopVideoTransmitChannel);
		}
	}

	free(mydata);

	if (ret > 0) return SWITCH_STATUS_FALSE;

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ooh323_set_call_capability(ooCallData *call, char *codec_string, DTMF dtmf)
{
	char *mydata = NULL, *argv[20];
	int argc;
	int i = 0;
	char *codec;
	char *rate;
	// char *ptime;
	int ret = 0;
	int rxframes = 40;
	int txframes = 40;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Set cap for %s\n", call->callToken);

	if(dtmf & H323_DTMF_RFC2833) {
		ret |= ooCallEnableDTMFRFC2833(call, 0);
	} else if(dtmf & H323_DTMF_H245ALPHANUMERIC) {
		ret |= ooCallEnableDTMFH245Alphanumeric(call);
	} else if(dtmf & H323_DTMF_H245SIGNAL) {
		ret |= ooCallEnableDTMFH245Signal(call);
	}

	mydata = strdup(codec_string);
	argc = switch_separate_string(mydata, ',', argv, (sizeof(argv) / sizeof(argv[0])));

	for (i = 0; i < argc; i++) {
		codec = argv[i];
		if ((rate = strchr(codec, '@'))) {
			*rate++ = '\0';
			if (strchr(rate, 'h')) {
				//todo
			} else {
				//todo
			}
		}

		if (!strcasecmp("PCMU", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add Call CAP [%s]\n", codec);
			ret |= ooCallAddG711Capability(call, OO_G711ULAW64K, txframes, rxframes, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("PCMA", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add Call CAP [%s]\n", codec);
			ret |= ooCallAddG711Capability(call, OO_G711ALAW64K, txframes, rxframes, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("G729", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add Call CAP [%s]\n", codec);
		    ret |= ooCallAddG729Capability(call, OO_G729A, 2, 24, OORXANDTX, CODEC_CALLBACKS);
		// } else if (strcasecmp, "G729b", codec) {
		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add EP CAP [%s]\n", codec);
		// 	ret |= ooCallAddG729Capability(call, OO_G729, 2, 24, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("G723", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add Call CAP [%s]\n", codec);
			ret |= ooCallAddG7231Capability(call, OO_G7231, 4, 7, FALSE, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("GSM", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add Call CAP [%s]\n", codec);
			ret |= ooCallAddGSMCapability(call, OO_GSMFULLRATE, 4, FALSE, FALSE, OORXANDTX, CODEC_CALLBACKS);
		} else if (!strcasecmp("H263", codec)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add Call CAP [%s]\n", codec);
			ret |= ooCallAddH263VideoCapability(call, OO_H263VIDEO, 1, 1, 1, 0, 0,
				VIDEO_MAX_BIT_RATE, OORXANDTX,
				&startVideoReceiveChannel, &startVideoTransmitChannel,
				&stopVideoReceiveChannel, &stopVideoTransmitChannel);
		} else if (!strcasecmp("H264", codec)) {
			OOH264CapParams params = { 0 };
			params.maxBitRate = VIDEO_MAX_BIT_RATE;
			// params.pt = 109;

			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Add Call CAP [%s]\n", codec);
			ret |= ooCallAddH264VideoCapability(call, OO_H264VIDEO, &params, OORXANDTX,
				&startVideoReceiveChannel, &startVideoTransmitChannel,
				&stopVideoReceiveChannel, &stopVideoTransmitChannel);
		}
	}

	free(mydata);

	if (ret > 0) return SWITCH_STATUS_FALSE;

	return SWITCH_STATUS_SUCCESS;
}

void set_media_info(ooCallData *call, switch_port_t local_port, switch_port_t local_video_port)
{
	ooMediaInfo mediaInfo;

	memset(&mediaInfo, 0, sizeof(ooMediaInfo));
	memset(&mediaInfo, 0, sizeof(ooMediaInfo));

	/* Configure mediainfo for transmit media channel of type G711 */
	mediaInfo.lMediaPort = local_port;
	mediaInfo.lMediaCntrlPort = mediaInfo.lMediaPort + 1;
	strcpy(mediaInfo.lMediaIP, call->localIP);
	strcpy(mediaInfo.dir, "transmit");
	mediaInfo.cap = OO_G711ULAW64K;
	ooAddMediaInfo(call, mediaInfo);

	strcpy(mediaInfo.dir, "receive");
	ooAddMediaInfo(call, mediaInfo);

	mediaInfo.cap = OO_G711ALAW64K;
	ooAddMediaInfo(call, mediaInfo);

	mediaInfo.cap = OO_GSMFULLRATE;
	ooAddMediaInfo(call, mediaInfo);

	mediaInfo.cap = OO_G729A;
	ooAddMediaInfo(call, mediaInfo);

	/* video */
	mediaInfo.lMediaPort = local_video_port;
	mediaInfo.lMediaCntrlPort = mediaInfo.lMediaPort + 1;

	mediaInfo.cap = OO_H263VIDEO;
	ooAddMediaInfo(call, mediaInfo);

	strcpy(mediaInfo.dir, "transmit");
	ooAddMediaInfo(call, mediaInfo);

	/* h264 */
	mediaInfo.cap = OO_H264VIDEO;
	ooAddMediaInfo(call, mediaInfo);

	strcpy(mediaInfo.dir, "receive");
	ooAddMediaInfo(call, mediaInfo);
}

switch_status_t prepare_codec(ooh323_private_t *tech_pvt)
{
	uint32_t rate = 8000;
	uint32_t ptime = 20;

	if (switch_core_codec_init(&tech_pvt->read_codec,
								"PCMU",
								NULL,
								rate,
								ptime,
								1,
								SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
								NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
		goto fail;
	}

	if (switch_core_codec_init(&tech_pvt->write_codec,
								"PCMU",
								NULL,
								rate,
								ptime,
								1,
								SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
								NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
		goto fail;
	}

	if (switch_core_session_set_read_codec(tech_pvt->session, &tech_pvt->read_codec) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set read codec?\n");
		goto fail;
	}

	if (switch_core_session_set_write_codec(tech_pvt->session, &tech_pvt->write_codec) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set write codec?\n");
		goto fail;
	}

	switch_channel_set_variable(switch_core_session_get_channel(tech_pvt->session), "timer_name", "soft");


	if (switch_core_codec_init(&tech_pvt->read_video_codec,
								"H263",
								NULL,
								90000,
								0,
								1,
								0,
								NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
		goto fail;
	}

	if (switch_core_codec_init(&tech_pvt->write_video_codec,
								"H263",
								NULL,
								90000,
								0,
								1,
								0,
								NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
		goto fail;
	}

	if (switch_core_session_set_video_read_codec(tech_pvt->session, &tech_pvt->read_video_codec) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set read codec?\n");
		goto fail;
	}

	if (switch_core_session_set_video_write_codec(tech_pvt->session, &tech_pvt->write_video_codec) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set write codec?\n");
		goto fail;
	}

	return SWITCH_STATUS_SUCCESS;

fail:

	if (tech_pvt->read_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->read_codec);
	}

	if (tech_pvt->write_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->write_codec);
	}

	if (tech_pvt->read_video_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->read_video_codec);
	}

	if (tech_pvt->write_video_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->write_video_codec);
	}

	switch_core_session_set_read_codec(tech_pvt->session, NULL);
	switch_core_session_set_write_codec(tech_pvt->session, NULL);
	switch_core_session_set_video_read_codec(tech_pvt->session, NULL);
	switch_core_session_set_video_write_codec(tech_pvt->session, NULL);

	return SWITCH_STATUS_FALSE;
}

#define SDPBUFLEN 65536
#define MAX_H323_CODECS 10

typedef struct h323_codec_s {
	OOCapabilities cap;
	switch_payload_t pt;
	int ptime;
} h323_codec_t;

/*TODO: We'll improve here to turn H323 Capabilities into SDP*/
void ooh323_gen_remote_sdp(switch_core_session_t *session)
{
	ooh323_private_t *tech_pvt = switch_core_session_get_private(session);
	char *ip = tech_pvt->remote_rtp_ip;
	switch_port_t port = tech_pvt->remote_rtp_port;
	switch_port_t video_port = tech_pvt->remote_video_rtp_port;
	char *buf;
	char srbuf[128] = "";
	char *family = strchr(ip, ':') ? "IP6" : "IP4";
	int ptime = 20;
	h323_codec_t audio_codecs[MAX_H323_CODECS];
	h323_codec_t video_codecs[MAX_H323_CODECS];
	int num_acodecs = 0;
	int num_vcodecs = 0;
	int i;
	ooH323EpCapability *cap;

	for (cap = tech_pvt->acap; cap; cap = cap->next) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "SDP Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);

		if (num_acodecs >= MAX_H323_CODECS) continue;

		switch(cap->cap) {
			case OO_G711ULAW64K:
				audio_codecs[num_acodecs].pt = 0;
				audio_codecs[num_acodecs].ptime = 20;
				num_acodecs++;
				break;
			case OO_G711ALAW64K:
				audio_codecs[num_acodecs].pt = 8;
				audio_codecs[num_acodecs].ptime = 20;
				num_acodecs++;
				break;
			case OO_GSMFULLRATE:
				audio_codecs[num_acodecs].pt = 3;
				audio_codecs[num_acodecs].ptime = 20;
				num_acodecs++;
				break;
			case OO_G729A:
				audio_codecs[num_acodecs].pt = 18;
				audio_codecs[num_acodecs].ptime = 20;
				num_acodecs++;
				break;
			default:
				break;
		}
	}

	for (cap = tech_pvt->vcap; cap; cap = cap->next) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "SDP Channel Caps: dir=%d, cap=%s(%d), capType=%d\n",
			cap->dir,ooGetCapTypeText(cap->cap), cap->cap, cap->capType);

		if (num_vcodecs >= MAX_H323_CODECS) continue;

		switch(cap->cap) {
			case OO_H263VIDEO:
				{
					// OOH263CapParams *params;
					// a=fmtp:34 QCIF=2;CIF=2;VGA=2;CIF4=2
				}
				video_codecs[num_vcodecs].cap = cap->cap;
				video_codecs[num_vcodecs].pt = 34;
				num_vcodecs++;
				break;
			case OO_H264VIDEO:
				{
					OOH264CapParams *params = (OOH264CapParams *)cap->params;
					if (params && params->pt) {
						video_codecs[num_vcodecs].pt = params->pt;
					} else if (tech_pvt->video_pt) {
						video_codecs[num_vcodecs].pt = tech_pvt->video_pt;
					} else {
						video_codecs[num_vcodecs].pt = 109;
					}
					video_codecs[num_vcodecs].cap = cap->cap;
					num_vcodecs++;
				}
				break;
			default:
				break;
		}
	}

	switch_zmalloc(buf, SDPBUFLEN);
	assert(buf);

	switch_snprintf(buf, SDPBUFLEN,
					"v=0\n"
					"o=FreeSWITCH 1234567890 1234567890 IN %s %s\n"
					"s=FreeSWITCH\n"
					"c=IN %s %s\n"
					"t=0 0\n"
					"%s",
					family, ip, family, ip, srbuf);

	switch_snprintf(buf + strlen(buf), SDPBUFLEN - strlen(buf),
		"m=audio %d RTP/AVP", port);

	for (i = 0; i < num_acodecs; i++) {
		switch_snprintf(buf + strlen(buf), SDPBUFLEN - strlen(buf), " %d", audio_codecs[i].pt);
	}

	switch_snprintf(buf + strlen(buf), SDPBUFLEN - strlen(buf), " 101\n");

	switch_snprintf(buf + strlen(buf), SDPBUFLEN - strlen(buf),
		"a=rtpmap:101 telephone-event/8000\n"
		"a=fmtp:101 0-16\n"
		"a=ptime:%d\n"
		"a=sendrecv\n", ptime);

	if (video_port)
		for (i = 0; i < num_vcodecs; i++) {

			if (video_codecs[i].cap == OO_H263VIDEO) {
				switch_snprintf(buf + strlen(buf), SDPBUFLEN - strlen(buf),
					"m=video %d RTP/AVP 34\n"
					"a=rtpmap:34 H263/90000\n"
					"a=fmtp:34 CIF=1;QCIF=1;VGA=2;CIF4=4\n", video_port);
				break;
			}
			if (video_codecs[i].cap == OO_H264VIDEO) {
				switch_snprintf(buf + strlen(buf), SDPBUFLEN - strlen(buf),
					"m=video %d RTP/AVP %d\n"
					"a=rtpmap:%d H264/90000\n"
					"a=fmtp:%d profile-level-id=428014;packetization-mode=0\n",
					video_port, video_codecs[i].pt, video_codecs[i].pt, video_codecs[i].pt);
				break;
			}
	}

	tech_pvt->remote_sdp_str = switch_core_session_strdup(session, buf);

	free(buf);
}

switch_bool_t check_acl(ooh323_profile_t *profile, char *ip)
{
	char *acl_context = NULL;

	if (profile->acl_count) {
		uint32_t x = 0;
		int ok = 1;
		char *last_acl = NULL;
		const char *token = NULL;

		for (x = 0; x < profile->acl_count; x++) {
			last_acl = profile->acl[x];
			if ((ok = switch_check_network_list_ip_token(ip, last_acl, &token))) {

				if (profile->acl_pass_context[x]) {
					acl_context = profile->acl_pass_context[x];
				}

				break;
			}

			if (profile->acl_fail_context[x]) {
				acl_context = profile->acl_fail_context[x];
			} else {
				acl_context = NULL;
			}
		}

		if (!ok) return SWITCH_FALSE;

	}

	if (!acl_context) acl_context = NULL; // avoid compiler warning before we use this
	return SWITCH_TRUE;
}

/*
v=0
o=FreeSWITCH 1383296271 1383296272 IN IP4 192.168.1.107
s=FreeSWITCH
c=IN IP4 192.168.1.107
t=0 0
m=audio 18430 RTP/AVP 0 8 101
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=ptime:20
a=sendrecv
m=video 3232 RTP/AVP 109
a=rtpmap:109 H264/90000
a=fmtp:109 profile-level-id=428014;packetization-mode=0
*/

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
