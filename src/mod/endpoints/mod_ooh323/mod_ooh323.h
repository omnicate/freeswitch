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
 * mod_ooh323.h -- OOH323 Endpoint Module
 *
 */

#ifndef MOD_OOH323_H___
#define MOD_OOH323_H___

#include "mod_ooh323.h"
#include "oochannels.h"
#include "ooh323ep.h"
#include "ooCalls.h"
#include "ooCapability.h"
#include "ooStackCmds.h"
#include "ooTimer.h"
#include "ooUtils.h"
#include "ooh245.h"
#include <switch.h>

/* Default values */
#define DEFAULT_CONTEXT         "default"
#define DEFAULT_DIALPLAN        "XML"
#define DEFAULT_H323ID          "FreeSWITCH"
#define DEFAULT_H323ACC         "freeswitch"

#define H323_MAX_ACL 100

typedef enum {
	H323_DTMF_RFC2833          = (1 << 0),
	H323_DTMF_Q931             = (1 << 1),
	H323_DTMF_H245ALPHANUMERIC = (1 << 2),
	H323_DTMF_H245SIGNAL       = (1 << 3),
	H323_DTMF_INBAND           = (1 << 4),
} DTMF;

typedef enum {
	TFLAG_IO                   = (1 << 0),    /* indicate IO */
	TFLAG_SIG                  = (1 << 1),    /* indicate ooh323 sig attached */
	TFLAG_RTP                  = (1 << 2),    /* indicate RTP */
	TFLAG_HUP                  = (1 << 3),    /* HUP */
	TFLAG_INBOUND              = (1 << 4),
	TFLAG_OUTBOUND             = (1 << 5),
	TFLAG_READING              = (1 << 6),
	TFLAG_WRITING              = (1 << 7),
	/* No new flags below this line */
	TFLAG_MAX
} TFLAGS;

struct ooh323_profile {
	char listen_ip[32];
	int  listen_port;
	enum OOCallMode call_mode;
	char gk_ip[32];
	int gk_port;
	int  gk_mode;
	char *gk_id;
	char *dialplan;
	char *context;
	char *extn;
	char localIPAddr[20];
	char callToken[20];
	char *h323id;
	char *user;
	char *user_num;
	int trace_level;
	char *logfile;
	int  cmdPort;

	switch_port_t tcpmin;
	switch_port_t tcpmax;
	switch_port_t udpmin;
	switch_port_t udpmax;
	switch_port_t rtpmin;
	switch_port_t rtpmax;

	int is_fast_start;
	int is_tunneling;

	uint32_t inuse;
	switch_payload_t te;
	switch_payload_t recv_te;
	switch_payload_t bte;
	DTMF dtmf_type;
	char *inbound_codec_string;
	char *outbound_codec_string;
	char *timer_name;
	switch_bool_t disable_rtp_autoadj;

	switch_mutex_t *mutex;

	char *acl[H323_MAX_ACL];
	char *acl_pass_context[H323_MAX_ACL];
	char *acl_fail_context[H323_MAX_ACL];
	uint32_t acl_count;

};

typedef struct ooh323_profile ooh323_profile_t;

struct ooh323_private {
	uint32_t flags;

	ooh323_profile_t *profile;

	switch_core_media_params_t mparams;
	switch_media_handle_t *media_handle;

	switch_codec_t read_codec;
	switch_codec_t write_codec;

	char local_rtp_ip[20];
	char local_ext_rtp_ip[20];
	uint32_t local_rtp_port;
	char remote_rtp_ip[20];
	uint32_t remote_rtp_port;

	uint32_t local_video_rtp_port;
	uint32_t remote_video_rtp_port;

	switch_codec_t read_video_codec;
	switch_codec_t write_video_codec;
	switch_frame_t read_frame;
	switch_frame_t read_video_frame;
	unsigned char databuf[SWITCH_RECOMMENDED_BUFFER_SIZE];	/* < Buffer for read_frame */

	switch_caller_profile_t *caller_profile;

	switch_mutex_t *mutex;
	switch_mutex_t *flag_mutex;

	switch_core_session_t *session;
	switch_channel_t *channel;

	char *remote_sdp_str;

	char *dialplan;
	char *context;
	char *name;
	char *caller_id_name;
	char *caller_id_number;
	char *caller_email;
	char *caller_url;
	char *destination_number;

	switch_bool_t audio_started;
	switch_bool_t video_started;

	switch_payload_t te;
	switch_payload_t recv_te;
	switch_payload_t bte;

	switch_payload_t video_pt; // to be improved

	ooCallData *h323_call;
	char *h323_callToken;
	ooH323EpCapability *acap;
	ooH323EpCapability *vcap;
};

typedef struct ooh323_private ooh323_private_t;

extern switch_endpoint_interface_t *ooh323_endpoint_interface;

switch_status_t ooh323_start (ooh323_profile_t *profile);
switch_status_t ooh323_listener_loop(ooh323_profile_t *profile);
void ooh323_stop();

switch_status_t ooh323_make_call(ooh323_private_t *tech_pvt);
switch_status_t ooh323_answer_call(ooh323_private_t *tech_pvt);
switch_status_t ooh323_manual_ringback(ooh323_private_t *tech_pvt);
switch_status_t ooh323_video_refresh(ooh323_private_t *tech_pvt);
switch_status_t ooh323_hangup_call(ooh323_private_t *tech_pvt);

switch_status_t ooh323_tech_init(ooh323_private_t *tech_pvt, switch_core_session_t *session);
switch_status_t ooh323_attach_private(switch_core_session_t *session, ooh323_profile_t *profile, ooh323_private_t *tech_pvt);

#endif
