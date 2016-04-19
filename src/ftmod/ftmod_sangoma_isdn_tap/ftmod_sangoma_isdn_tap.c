/*
 * Copyright (c) 2015, Pushkar Singh <psingh@sangoma.com>
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
 *
 * Contributors: Moises Silva  <moy@sangoma.com>
 *		 Pushkar Singh <psingh@sangoma.com>
 *
 */

/********************************************************************************/
/*                                                                              */
/*                           libraries inclusion                                */
/*                                                                              */
/********************************************************************************/
#include "ftmod_sangoma_isdn_tap.h"

/********************************************************************************/
/*                                                                              */
/*                               global declaration                             */
/*                                                                              */
/********************************************************************************/
static int g_decoder_lib_init;

/* extern variables */
sng_decoder_event_interface_t    sng_event;
ftdm_io_interface_t ftdm_sngisdn_tap_interface;
ftdm_sngisdn_tap_data_t g_sngisdn_tap_data;

/* Global counters to get the exact number of calls received/tapped/released etc. */
uint64_t setup_recv;
uint64_t reuse_chan_con_recv;	/* Counter to keep track of connect recv on which we found channel in use error */
uint64_t setup_recv_invalid_chanId;
uint64_t setup_not_recv;
uint64_t connect_recv_with_chanId;
uint64_t connect_recv;
uint64_t early_connect_recv;
uint64_t late_invite_recv;
uint64_t call_proceed_recv;
uint64_t release_recv;
uint64_t disconnect_recv;
uint64_t call_tapped;
uint64_t msg_recv_aft_time;

/* Rx/TX QEUE SIZE */
uint32_t tap_dchan_queue_size;

/********************************************************************************/
/*                                                                              */
/*                             function declaration                             */
/*                                                                              */
/********************************************************************************/
static ftdm_status_t ftdm_sangoma_isdn_tap_start(ftdm_span_t *span);
void sangoma_isdn_tap_set_debug(struct sngisdn_t *pri, int debug);
static void handle_isdn_tapping_log(uint8_t level, char *fmt,...);
int ftdm_sangoma_isdn_get_switch_type(const char *val);
static ftdm_status_t sangoma_isdn_tap_handle_event_con(sngisdn_tap_t *tap, uint8_t msgType, sng_decoder_interface_t *intfCb, int crv);

/* Functions to release STALE Call */
static ftdm_status_t sangoma_isdn_tap_is_fchan_inuse(sngisdn_tap_t *tap, int channel);
static ftdm_status_t sangoma_isdn_tap_rel_stale_call(sngisdn_tap_t *tap, int chan_id);
static ftdm_status_t sangoma_isdn_tap_handle_event_rel(sngisdn_tap_t *tap, uint8_t msgType, int crv);
static void sangoma_isdn_tap_check_stale_conevt(sngisdn_tap_t *tap);
static void sangoma_isdn_tap_free_hashlist(sngisdn_tap_t *tap);

/* Get pcall by channel ID */
passive_call_t *sangoma_isdn_tap_get_pcall_bychanId(sngisdn_tap_t *tap, int chan_id);

/* Enable IO stats */
ftdm_status_t ftdm_sangoma_isdn_tap_io_stats(ftdm_stream_handle_t *stream, ftdm_span_t *span, const char* cmd);

static FIO_API_FUNCTION(ftdm_sangoma_isdn_tap_api);
static ftdm_status_t sangoma_isdn_read_msg(sngisdn_tap_t *tap);

/********************************************************************************/
/*                                                                              */
/*                              implementation                                  */
/*                                                                              */
/********************************************************************************/

/**********************************************************************************
 * Fun   : FIO_IO_UNLOAD_FUNCTION(ftdm_sangoma_isdn_tap_unload)
 *
 * Desc  : Call back function in order to unload the sng_isdn_tap module
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_IO_UNLOAD_FUNCTION(ftdm_sangoma_isdn_tap_unload)
{
	/* Release Decoder lib resources */
	if(g_decoder_lib_init) {
		sng_decoder_lib_deinit();
		g_decoder_lib_init = 0x00;
	}
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : FIO_CHANNEL_GET_SIG_STATUS_FUNCTION(sangoma_isdn_tap_get_channel_sig_status)
 *
 * Desc  : Call back function in order to get the signalling status w.r.t a
 * 	   particular channel as requested by user
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_CHANNEL_GET_SIG_STATUS_FUNCTION(sangoma_isdn_tap_get_channel_sig_status)
{
	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_SIG_UP)) {
		*status = FTDM_SIG_STATE_UP;
	} else {
		*status = FTDM_SIG_STATE_DOWN;
	}

	*status = FTDM_SIG_STATE_UP;
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : FIO_SPAN_GET_SIG_STATUS_FUNCTION(sangoma_isdn_tap_get_span_sig_status)
 *
 * Desc  : Call back function in order to get the signalling status w.r.t a
 * 	   particular span as requested by user
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_SPAN_GET_SIG_STATUS_FUNCTION(sangoma_isdn_tap_get_span_sig_status)
{
	*status = FTDM_SIG_STATE_UP;
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : FIO_CHANNEL_OUTGOING_CALL_FUNCTION(sangoma_isdn_tap_outgoing_call)
 *
 * Desc  : Call back function for outgoing call but in case of tapping one cannot
 * 	   perform any outgoing calls
 *
 * Ret   : FTDM_FAIL
**********************************************************************************/
static FIO_CHANNEL_OUTGOING_CALL_FUNCTION(sangoma_isdn_tap_outgoing_call)
{
	sngisdn_tap_t *tap = ftdmchan->span->signal_data;
	if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
		return FTDM_SUCCESS;
	}

	ftdm_log(FTDM_LOG_ERROR, "Cannot dial on ISDN tapping line!\n");
	return FTDM_FAIL;
}

/**********************************************************************************
 * Fun   : parse_debug()
 *
 * Desc  : Sets the dubug level as per users requirement
 *
 * Ret   : DEBUG_ALL | flags | 0
**********************************************************************************/
static int parse_debug(const char *in)
{
	int flags = 0;

	if (!in) {
		return 0;
	}

	if (!strcmp(in, "none")) {
		return 0;
	}

	if (!strcmp(in, "all")) {
		return DEBUG_ALL;
	}

	if (strstr(in, "q921_raw")) {
		flags |= DEBUG_Q921_RAW;
	}

	return flags;
}

/**********************************************************************************
 * Fun   : FIO_IO_LOAD_FUNCTION(ftdm_sangoma_isdn_tap_io_init)
 *
 * Desc  : Call back function in order to initialize set io w.r.t sng_isdn_tap
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_IO_LOAD_FUNCTION(ftdm_sangoma_isdn_tap_io_init)
{
	memset(&ftdm_sngisdn_tap_interface, 0, sizeof(ftdm_sngisdn_tap_interface));

	ftdm_sngisdn_tap_interface.name = "isdn_tap";
	ftdm_sngisdn_tap_interface.api = ftdm_sangoma_isdn_tap_api;

	*fio = &ftdm_sngisdn_tap_interface;

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : FIO_SIG_LOAD_FUNCTION(ftdm_sangoma_isdn_tap_init)
 *
 * Desc  : Call back function in order to initialize sngisdn_tap module + stack +
 * 	   decoder library
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static FIO_SIG_LOAD_FUNCTION(ftdm_sangoma_isdn_tap_init)
{
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Macro : ftdm_state_map_t
 *
 * Desc  : Mapping FTDM events
**********************************************************************************/
static ftdm_state_map_t sngisdn_tap_state_map = {
	{
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_DIALING, FTDM_CHANNEL_STATE_RING, FTDM_CHANNEL_STATE_END}
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_DIALING, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_RING, FTDM_CHANNEL_STATE_END}
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_RING, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_END}
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_RINGING, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_DIALING, FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_END}
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_DIALING, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
			 FTDM_CHANNEL_STATE_PROCEED, FTDM_CHANNEL_STATE_RINGING, FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP,
			 FTDM_CHANNEL_STATE_END}
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
		     FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_HANGUP_COMPLETE,
			 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_END},
		},
		{
			ZSD_OUTBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_RINGING, FTDM_CHANNEL_STATE_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_HANGUP_COMPLETE,
			 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_UP,
			 FTDM_CHANNEL_STATE_END},
		},
	}
};

/**********************************************************************************
 * Fun   : ftdm_status_t state_advance
 *
 * Desc  : Change FTDM state depending upon Q931 message received
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
ftdm_status_t state_advance(ftdm_channel_t *ftdmchan)
{
	sngisdn_tap_t *tap = ftdmchan->span->signal_data;
	passive_call_t *pcall = NULL;
	ftdm_status_t status;
	ftdm_sigmsg_t sigmsg;

	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "processing state %s\n", ftdm_channel_state2str(ftdmchan->state));

	ftdm_channel_complete_state(ftdmchan);

	if (!tap) {
		ftdm_log_chan_msg(ftdmchan, FTDM_LOG_ERROR, "No Signal data/tap structure present!\n");
		return FTDM_FAIL;
	}

	memset(&sigmsg, 0, sizeof(sigmsg));

	sigmsg.chan_id = ftdmchan->chan_id;
	sigmsg.span_id = ftdmchan->span_id;
	sigmsg.channel = ftdmchan;


	switch (ftdmchan->state) {
		case FTDM_CHANNEL_STATE_DOWN:
			{
				ftdm_channel_t *fchan = ftdmchan;

				/* Destroy the peer data first */
				if (fchan->call_data) {
					if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
						ftdm_log_chan(fchan, FTDM_LOG_DEBUG, "Do not close peer channel as ISDN Tapping is running is Bi-Directional mode! %s\n", "");
					} else {
						ftdm_channel_t *peerchan = fchan->call_data;
						ftdm_channel_t *pchan = peerchan;

						ftdm_channel_lock(peerchan);

						pchan->call_data = NULL;
						pchan->pflags = 0;

						ftdm_log_chan(pchan, FTDM_LOG_DEBUG, "Tapping done..Closing Peer channel. %s\n", "");
						ftdm_channel_close(&pchan);

						ftdm_channel_unlock(peerchan);
					}
				} else {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "No call data?\n");
				}

				ftdm_channel_lock(ftdmchan);
				fchan->call_data = NULL;
				fchan->pflags = 0;
				ftdm_log_chan(fchan, FTDM_LOG_DEBUG, "Tapping done..Closing channel.%s\n", "");
				ftdm_channel_close(&fchan);
				ftdm_channel_unlock(ftdmchan);
			}
			break;

		case FTDM_CHANNEL_STATE_PROGRESS:
		case FTDM_CHANNEL_STATE_PROGRESS_MEDIA:
			break;

		case FTDM_CHANNEL_STATE_UP:
			{
				if (tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
					if (!ftdm_test_flag(ftdmchan, FTDM_CHANNEL_OUTBOUND)) {
						ftdm_log_chan_msg(ftdmchan, FTDM_LOG_INFO, "Do nothing as this is an INBOUND channel!\n");
						break;
					}
				}
				if (!(pcall = sangoma_isdn_tap_get_pcall_bychanId(tap, ftdmchan->physical_chan_id))) {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_DEBUG, "No pcall present!\n");
					break;
				}

				/* Start Tapping Call as call is in connected state */
				sigmsg.event_id = FTDM_SIGEVENT_UP;

				/* increasing the counter of calls tapped */
				call_tapped++;

				if ((status = ftdm_span_send_signal(ftdmchan->span, &sigmsg) != FTDM_SUCCESS)) {
					ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
				}
				ftdm_log_chan(ftdmchan, FTDM_LOG_INFO, "[Callref- %d]SIGEVENT_UP Sent!\n", pcall->callref_val);
			}
			break;

		case FTDM_CHANNEL_STATE_RING:
			{
				/* We will start Tapping calls only when Connect is recieved */
#if 0
				sig.event_id = FTDM_SIGEVENT_START;
				/* The ring interface (where the setup was received) is the peer, since we RING the channel
				 * where PROCEED/PROGRESS is received */
				ftdm_sigmsg_add_var(&sig, "sngisdn_tap_ring_interface", SNG_ISDN_TAP_GET_INTERFACE(peer_tap->iface));
				if ((status = ftdm_span_send_signal(ftdmchan->span, &sig) != FTDM_SUCCESS)) {
					ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
				}
#endif
				if (tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
					char variable_val[3] = { 0 };
					ftdm_channel_t *peerchan = NULL;

					sigmsg.event_id = FTDM_SIGEVENT_START;

					peerchan = (ftdm_channel_t*)ftdmchan->call_data;
					if( !peerchan ) {
						ftdm_log_chan_msg(ftdmchan, FTDM_LOG_ERROR, "Not peer chan data! \n");
						break;
					}else {
						ftdm_set_string(peerchan->caller_data.cid_num.digits, ftdmchan->caller_data.cid_num.digits);
						ftdm_set_string(peerchan->caller_data.cid_name, ftdmchan->caller_data.cid_name);
						ftdm_set_string(peerchan->caller_data.dnis.digits, ftdmchan->caller_data.dnis.digits);
						peerchan->caller_data.call_reference = ftdmchan->caller_data.call_reference;
					}

					snprintf(variable_val, sizeof(variable_val), "%d", peerchan->span_id);
					ftdm_sigmsg_add_var(&sigmsg, "isdn_tap_peer_span", variable_val);
					snprintf(variable_val, sizeof(variable_val), "%d", peerchan->chan_id);
					ftdm_sigmsg_add_var(&sigmsg, "isdn_tap_peer_chan", variable_val);

					if (ftdm_span_send_signal(peerchan->span, &sigmsg) != FTDM_SUCCESS) {
						ftdm_set_state_locked(peerchan, FTDM_CHANNEL_STATE_HANGUP);
					}

					if (sigmsg.variables) {
						hashtable_destroy(sigmsg.variables);
						sigmsg.variables = NULL;
					}
				}
			}
			break;

		case FTDM_CHANNEL_STATE_HANGUP:
			{
				ftdm_log(FTDM_LOG_DEBUG, "Getting into HANGUP state handler \n");
				if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
					ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
				}
			}
			break;

		case FTDM_CHANNEL_STATE_HANGUP_COMPLETE:
			{
				if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
					ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
				}
			}
			break;

		case FTDM_CHANNEL_STATE_TERMINATING:
			{
				if (ftdmchan->last_state != FTDM_CHANNEL_STATE_HANGUP) {
					sigmsg.event_id = FTDM_SIGEVENT_STOP;
					ftdm_clear_flag(ftdmchan, FTDM_CHANNEL_USER_HANGUP);
					status = ftdm_span_send_signal(ftdmchan->span, &sigmsg);
				}
				ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
			}
			break;

		case FTDM_CHANNEL_STATE_DIALING:
			if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
				if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_OUTBOUND)) {
					ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_RINGING);
				} else {
					ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_RING);
				}
			}
			break;

		case FTDM_CHANNEL_STATE_RINGING:
			if((tap->tap_mode == ISDN_TAP_BI_DIRECTION) && (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_OUTBOUND))) {
				/* get pcall based on channel ID */
				if (!(pcall = sangoma_isdn_tap_get_pcall_bychanId(tap, ftdmchan->physical_chan_id))) {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_DEBUG, "No pcall present!\n");
					break;
				}

				ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "[Callref- %d] RINGING received for channel\n", pcall->callref_val);
				if (pcall->recv_conn == FTDM_TRUE) {
					late_invite_recv++;
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_DEBUG, "Changing state to UP as connect is already being processed!\n");
					ftdm_set_state_locked(pcall->fchan, FTDM_CHANNEL_STATE_UP);
				} else {
					ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG,
								  "We assume that connect is already being received and channel state is already UP[change state from %s to %s]\n",
								  ftdm_channel_state2str(ftdmchan->last_state), ftdm_channel_state2str(ftdmchan->state));
				}
			}
			break;

		default:
			{
				ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "ignoring state change from %s to %s\n", ftdm_channel_state2str(ftdmchan->last_state), ftdm_channel_state2str(ftdmchan->state));
			}
			break;
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : ftmod_isdn_process_event()
 *
 * Desc  : Handle wanpipe events does not change state from Down to Up once ALARM
 * 	   Clear is recieved as We only change channel state to UP once call is
 * 	   being answered on that particular channel and Tapping starts
 *
 * Ret   : FTDM_SUCCESS
 **********************************************************************************/
ftdm_status_t ftmod_isdn_process_event(ftdm_span_t *span, ftdm_event_t *event)
{
	ftdm_alarm_flag_t alarmbits;
	ftdm_sigmsg_t sig;

	memset(&sig, 0, sizeof(sig));
	sig.span_id = ftdm_channel_get_span_id(event->channel);
	sig.chan_id = ftdm_channel_get_id(event->channel);
	sig.channel = event->channel;

	ftdm_log_chan(event->channel, FTDM_LOG_DEBUG, "EVENT:%d Recieved on Span:%d Channel:%d\n",
			event->enum_id,	ftdm_channel_get_span_id(event->channel),
			ftdm_channel_get_id(event->channel));

	switch (event->enum_id) {
		case FTDM_OOB_ALARM_TRAP:
			{
				sig.event_id = FTDM_OOB_ALARM_TRAP;
				if (ftdm_channel_get_state(event->channel) != FTDM_CHANNEL_STATE_DOWN) {
					ftdm_set_state_locked(event->channel, FTDM_CHANNEL_STATE_RESTART);
				}
				ftdm_set_flag(event->channel, FTDM_CHANNEL_SUSPENDED);
				ftdm_channel_get_alarms(event->channel, &alarmbits);
				ftdm_span_send_signal(span, &sig);

				ftdm_log(FTDM_LOG_WARNING, "channel %d:%d (%d:%d) has alarms [%s]\n",
						ftdm_channel_get_span_id(event->channel),
						ftdm_channel_get_id(event->channel),
						ftdm_channel_get_ph_span_id(event->channel),
						ftdm_channel_get_ph_id(event->channel),
						ftdm_channel_get_last_error(event->channel));
			}
			break;

		case FTDM_OOB_ALARM_CLEAR:
			{
				sig.event_id = FTDM_OOB_ALARM_CLEAR;
				ftdm_clear_flag(event->channel, FTDM_CHANNEL_SUSPENDED);
				ftdm_channel_get_alarms(event->channel, &alarmbits);
				ftdm_span_send_signal(span, &sig);
			}
			break;

		default:
			break;
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_check_state()
 *
 * Desc  : Checks the state of the span
 *
 * Ret   : void
**********************************************************************************/
void sangoma_isdn_tap_check_state(ftdm_span_t *span)
{
	if (ftdm_test_flag(span, FTDM_SPAN_STATE_CHANGE)) {
		uint32_t j;
		ftdm_clear_flag_locked(span, FTDM_SPAN_STATE_CHANGE);
		for(j = 1; j <= span->chan_count; j++) {
			ftdm_channel_lock(span->channels[j]);
			ftdm_channel_advance_states(span->channels[j]);
			ftdm_channel_unlock(span->channels[j]);
		}
	}
}

/**********************************************************************************
 * Fun   : sangoma_isdn_io_read()
 *
 * Desc  : Read message as present on respective sngisdn_tap D-channel
 *
 * Ret   : res | -1
**********************************************************************************/
static int sangoma_isdn_io_read(struct sngisdn_t *isdn_msg, void *buf, int buflen)
{
	int res = 0;
	ftdm_status_t zst;
	sngisdn_tap_t *tap = sangoma_isdn_get_userdata(isdn_msg);
	ftdm_size_t len = buflen;

	/* Read the message as present on D-Channel */
	if ((zst = ftdm_channel_read(tap->dchan, buf, &len)) != FTDM_SUCCESS) {
		if (zst == FTDM_FAIL) {
			ftdm_log(FTDM_LOG_CRIT, "Span %d D channel read fail! [%s]\n", tap->span->span_id, tap->dchan->last_error);
		} else {
			ftdm_log(FTDM_LOG_CRIT, "Span %d D channel read timeout!\n", tap->span->span_id);
		}
		return res;
	}

	/* Getting length of the message read */
	res = (int)len;

	/* Print Q921 message if DEBUG Level is set to DEBUG_Q921_RAW by the user  */
	if (tap->debug & DEBUG_Q921_RAW) {
		char hbuf[2048] = { 0 };

		print_hex_bytes(buf, len, hbuf, sizeof(hbuf));
		ftdm_log_chan(tap->dchan, FTDM_LOG_DEBUG, "READ %"FTDM_SIZE_FMT"\n%s\n", len, hbuf);
	}

	return res;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap__get_pcall_bycrv()
 *
 * Desc  : Get sngisdn_tap call reference value if the message is respect to the
 * 	   call already in connected state
 *
 * Ret   : tap->pcalls | NULL
**********************************************************************************/
passive_call_t *sangoma_isdn_tap_get_pcall_bycrv(sngisdn_tap_t *tap, int crv)
{
	int i;
	int tstcrv;

	ftdm_mutex_lock(tap->pcalls_lock);

	/* Go through all call present and check if the reference value matches to any one of the call the return the
	 * respective call control block else return NULL */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		tstcrv = tap->pcalls[i].callref_val;
		if (tstcrv == crv) {
			if (tap->pcalls[i].inuse) {
				ftdm_mutex_unlock(tap->pcalls_lock);
				return &tap->pcalls[i];
			}

			/* This just means the crv is being re-used in another call before this one was destroyed */
			ftdm_log(FTDM_LOG_DEBUG, "Found crv %d in slot %d of span %s with call %d but is no longer in use\n",
					crv, i, tap->span->name, tap->pcalls[i].callref_val);
		}
	}

	ftdm_log(FTDM_LOG_DEBUG, "Call reference value %d not found active in span %s\n", crv, tap->span->name);
	ftdm_mutex_unlock(tap->pcalls_lock);
	return NULL;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap__get_pcall_bychanID()
 *
 * Desc  : Get sngisdn_tap pcall w.r.t channel assign to it
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
passive_call_t *sangoma_isdn_tap_get_pcall_bychanId(sngisdn_tap_t *tap, int chan_id)
{
	ftdm_channel_t *fchan = NULL;
	int idx = 0;

	ftdm_mutex_lock(tap->pcalls_lock);

	/* Go through all call present and check if the reference value matches to any one of the call the return the
	 * respective call control block else return NULL */
	for (idx = 0; idx < ftdm_array_len(tap->pcalls); idx++) {
		fchan = tap->pcalls[idx].fchan;

		if (!fchan) {
			continue;
		}

		if (chan_id == fchan->physical_chan_id) {
			ftdm_mutex_unlock(tap->pcalls_lock);
			return &tap->pcalls[idx];
		}
	}

	ftdm_log(FTDM_LOG_DEBUG, "pcall with physical channel %d not present at all\n", chan_id);
	ftdm_mutex_unlock(tap->pcalls_lock);

	return NULL;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_get_nfas_pcall_bycrv()
 *
 * Desc  : It mainly gives the pcall structure depending up on the nfas group to
 * 	   which tap is belonges to depeding upon all the spans present in that
 * 	   nfas group span list and call reference passed.
 *
 * Ret   : tap->pcalls | NULL
 ***********************************************************************************/
static passive_call_t *sangoma_isdn_tap_get_nfas_pcall_bycrv(sngisdn_tap_t **tap, int crv)
{
	sngisdn_tap_t *call_tap = NULL;
	sngisdn_tap_t *sig_tap = NULL;
	passive_call_t *pcall = NULL;
	ftdm_span_t *span = NULL;
	int idx = 0;

	if (!tap) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid tap value\n");
		return NULL;
	}

	sig_tap = *tap;

	for (idx = 0; idx < sig_tap->nfas.trunk->num_tap_spans; idx++) {
		if (!sig_tap->nfas.trunk->tap_spans[idx]) {
			ftdm_log(FTDM_LOG_ERROR, "[%s Callref- %d]No ISDN NFAS Tap span found at index %d\n",
				 sig_tap->span->name, crv, idx);
			return NULL;
		}

		span = sig_tap->nfas.trunk->tap_spans[idx];
		call_tap = span->signal_data;

		pcall = sangoma_isdn_tap_get_pcall_bycrv(call_tap, crv);

		if (pcall) {
			*tap = call_tap;

			ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d]Found pcall at index %d\n",
				 call_tap->span->name, crv, idx);
			return pcall;
		}
	}

	return NULL;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_get_pcall()
 *
 * Desc  : It mainly gives the pcall structure w.r.t the tap on which call is
 *	   getting established on the basis of call reference value.
 *
 * Ret   : tap->pcalls | NULL
 ***********************************************************************************/
passive_call_t *sangoma_isdn_tap_get_pcall(sngisdn_tap_t *tap, void *callref)
{
	int i;
	int crv = 0;
	int *tmp_crv = NULL;
	passive_call_t *pcall = NULL;

	ftdm_mutex_lock(tap->pcalls_lock);

	/* If callref matches to any of the existing callref value the Enable call ref slot */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		if (callref) {
			tmp_crv = (int*)callref;
			crv = *tmp_crv;
		}

		if (crv == tap->pcalls[i].callref_val) {
			if (callref == NULL) {
				tap->pcalls[i].inuse = 1;
				ftdm_log(FTDM_LOG_DEBUG, "Enabling callref slot %d in span %s\n", i, tap->span->name);
			} else if (!tap->pcalls[i].inuse) {
				ftdm_log(FTDM_LOG_DEBUG, "Recyclying callref slot %d in span %s for callref %d/%p\n",
						i, tap->span->name, crv, callref);
				memset(&tap->pcalls[i], 0, sizeof(tap->pcalls[0]));
				//tap->pcalls[i].callref = callref;
				tap->pcalls[i].callref_val = crv;
				tap->pcalls[i].inuse = 1;
			}

			ftdm_mutex_unlock(tap->pcalls_lock);
			pcall = &tap->pcalls[i];
			return pcall;
		}
	}

	ftdm_mutex_unlock(tap->pcalls_lock);
	return NULL;
}

void sangoma_isdn_tap_print_pcall(sngisdn_tap_t *tap)
{
	passive_call_t* pcall = NULL;
	int i = 0;

	ftdm_mutex_lock(tap->pcalls_lock);

	/* If callref matches to any of the existing callref value the Enable call ref slot */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		pcall = &tap->pcalls[i];
		if (pcall) {
			ftdm_log(FTDM_LOG_INFO, "Pcall present at slot %d with call reference value as %d\n", i, tap->pcalls[i].callref_val);
		}

	}

	ftdm_mutex_unlock(tap->pcalls_lock);
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_put_pcall()
 *
 * Desc  : Once DISCONNECT or RELEASE COMPLETE message is received this funtion
 *	   release the slot on which call got established
 *
 * Ret   : void
**********************************************************************************/
void sangoma_isdn_tap_put_pcall(sngisdn_tap_t *tap, void *callref)
{
	int i;
	int crv;
	int tstcrv;
	int *tmp_crv = callref;

	if (!callref) {
		ftdm_log(FTDM_LOG_ERROR, "[%s] Cannot put pcall for null callref\n", tap->span->name);
		return;
	}

	ftdm_mutex_lock(tap->pcalls_lock);

	/* Get the call reference value */
	crv = *tmp_crv;

	/* Check if the crv matches to any of the present call crv. If matches then release the slot
	 * on which respective call is active */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		if (!tap->pcalls[i].callref_val) {
			continue;
		}
		tstcrv = tap->pcalls[i].callref_val;

		if (tstcrv == crv) {
			if (tap->pcalls[i].inuse) {
				ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Releasing slot %d\n",
						tap->span->name, tap->pcalls[i].chanId, tap->pcalls[i].callref_val, i);
				sangoma_isdn_tap_reset_pcall(&tap->pcalls[i]);
				tap->pcalls[i].inuse = 0;
			}
		}
	}

	ftdm_mutex_unlock(tap->pcalls_lock);
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_reset_pcall()
 *
 * Desc  : RESET all pcall parameters
 *
 * Ret   : FTDM_FAIL | FTDM_SUCCESS
**********************************************************************************/
ftdm_status_t sangoma_isdn_tap_reset_pcall(passive_call_t *pcall)
{
	if (!pcall) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid pcall to clear\n");
		return FTDM_FAIL;
	}

	ftdm_log(FTDM_LOG_INFO, "Resetting Pcall \n");

	pcall->callref_val = 0;
	pcall->recv_setup = FTDM_FALSE;
	pcall->chan_pres = FTDM_FALSE;
	pcall->recv_conn = FTDM_FALSE;
	pcall->recv_other_msg = FTDM_FALSE;
	pcall->fchan = NULL;
	pcall->chanId = 0;
	pcall->t_setup_recv = time(NULL);
	pcall->t_con_recv = time(NULL);
	pcall->t_msg_recv = time(NULL);
	memset(&pcall->callingnum.digits, 0, sizeof(pcall->callingnum.digits));
	memset(&pcall->callednum.digits, 0, sizeof(pcall->callednum.digits));
	memset(&pcall->callingnum, 0, sizeof(pcall->callingnum));
	memset(&pcall->callednum, 0, sizeof(pcall->callednum));
	pcall->tap_msg.chan_id = 0;
	pcall->tap_msg.span_id = 0;
	pcall->tap_msg.call_ref = 0;
	pcall->tap_msg.call_dir = CALL_DIRECTION_NONE;
	pcall->tap_msg.call_con = 0;
	memset(&pcall->tap_msg.calling_num, 0, sizeof(pcall->tap_msg.calling_num));
	memset(&pcall->tap_msg.called_num, 0, sizeof(pcall->tap_msg.called_num));
	memset(&pcall->tap_msg, 0, sizeof(pcall->tap_msg));
	memset(&pcall->tap_msg, 0, sizeof(pcall->tap_msg));

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_is_con_recv()
 *
 * Desc  : Check if connect/proceed event is already received for the call for
 *	   which SETUP is already received for the same
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
ftdm_status_t sangoma_isdn_tap_is_con_recv(sngisdn_tap_t *tap, int crv, uint8_t msgType)
{
	char call_ref[25];
	ftdm_status_t ret = FTDM_FAIL;
	con_call_info_t *call_info = NULL;

	memset(call_ref, 0 , sizeof(call_ref));
	/* Check in the hash list whether CONNECT/CALL PROCEED is already received for this call */
	sprintf(call_ref, "%d", crv);

	call_info = hashtable_search(tap->pend_call_lst, (void *)call_ref);

	if (call_info) {
		if (call_info->tap_proceed) {
			ftdm_log(FTDM_LOG_INFO, "[%s Callref- %d] Call found in hash table for which tapping is already started\n",
					 tap->span->name, crv);
			ret = FTDM_BREAK;
		} else {
			ftdm_log(FTDM_LOG_INFO, "[%s Callref- %d] Call found in hash table for which connect is already received\n",
					 tap->span->name, crv);
			ret = FTDM_SUCCESS;
		}

		if ((msgType == Q931_DISCONNECT) || (msgType == Q931_RELEASE)) {
			ftdm_log(FTDM_LOG_INFO, "[%s Callref- %d] Call found for which there is no SETUP\n", tap->span->name, crv);
			setup_not_recv++;
		} else if (msgType != Q931_SETUP) {
			ftdm_log(FTDM_LOG_INFO, "[%s Callref- %d] Call found already in hash list for message %s\n",
				     tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));
			return ret;
		}

		hashtable_remove(tap->pend_call_lst, (void *)call_ref);
		ftdm_safe_free(call_info);
	}

	return ret;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_check_stale_call()
 *
 * Desc  : Check if the SETUP received for the call is STALE one then please
 *	   release the STALE call and move channel to DOWN state
 *
 * Ret   : VOID
**********************************************************************************/
static void sangoma_isdn_tap_check_stale_call(sngisdn_tap_t *tap, sng_decoder_interface_t *intfCb, int crv, uint8_t msgType)
{
	sngisdn_tap_t *peertap = tap->peerspan->signal_data;

	/* Stale call means -
	 * 1) We received SETUP on the used channel which means we somehow missed processing the DISCONNECT/RELEASE of that channel
	 *	  and now we again received SETUP on the same channel, hence release channel and move on with processing of received SETUP.
	 *
	 * 2) We received CONNECT before SETUP and we have been waiting for SETUP more then 1 min, hence we lost hope for SETUP..
	 *    release this stale CONNECT message related resources.
	 */

	if (FTDM_SUCCESS == sangoma_isdn_tap_is_fchan_inuse(tap, intfCb->chan_id)) {
		if (FTDM_FAIL == sangoma_isdn_tap_rel_stale_call(tap, intfCb->chan_id)) {
			ftdm_log(FTDM_LOG_ERROR, "[%s:%d] Not able to release STALE Call\n", tap->span->name, intfCb->chan_id);
		}
	}

	if (FTDM_SUCCESS == sangoma_isdn_tap_is_fchan_inuse(peertap, intfCb->chan_id)) {
		if (FTDM_FAIL == sangoma_isdn_tap_rel_stale_call(peertap, intfCb->chan_id)) {
			/* ideally this should not come at all, hence critical error to print if at all happenning */
			ftdm_log(FTDM_LOG_CRIT, "[%s:%d] Not able to release peer STALE Call\n", peertap->span->name, intfCb->chan_id);
		}
	}

	/* check for stale connect events */
	if (msgType != Q931_CONNECT) {
		ftdm_log(FTDM_LOG_INFO, "[%s Callref- %d] Check for Connection received Stale Call\n", tap->span->name, crv);
		sangoma_isdn_tap_check_stale_conevt(tap);
	}
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_reset_pcall_bychanId()
 *
 * Desc  : Clearing STALE pcall if present with same channel ID
 *
 * Ret   : void
**********************************************************************************/
static void sangoma_isdn_tap_reset_pcall_bychanId(sngisdn_tap_t *tap, sng_decoder_interface_t *intfCb, int crv)
{
	passive_call_t *pcall = NULL;
	int i = 0;

	ftdm_mutex_lock(tap->pcalls_lock);

	/* If callref matches to any of the existing callref value the Enable call ref slot */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		pcall = &tap->pcalls[i];
		if (pcall) {
			if (pcall->chanId == intfCb->chan_id) {
				ftdm_log(FTDM_LOG_INFO, "[%s:%d Callref- %d] Found pcall present at slot %d\n",
						tap->span->name, pcall->chanId, tap->pcalls[i].callref_val, i);

				ftdm_log(FTDM_LOG_DEBUG, " [%s:%d Callref- %d] Releasing slot %d\n",
						tap->span->name, pcall->chanId, tap->pcalls[i].callref_val, i);

				if (tap->pcalls[i].fchan) {
					ftdm_log_chan(tap->pcalls[i].fchan, FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Got channel on Stale pcall with channel state as %s\n",
						   tap->span->name, pcall->chanId, tap->pcalls[i].callref_val, ftdm_channel_state2str(tap->pcalls[i].fchan->state));
				}

				tap->pcalls[i].inuse = 0;
				tap->pcalls[i].callref_val = 0;
				tap->pcalls[i].fchan = NULL;
				tap->pcalls[i].t_setup_recv = time(NULL);
				tap->pcalls[i].t_con_recv = time(NULL);
				tap->pcalls[i].t_msg_recv = time(NULL);
				tap->pcalls[i].recv_setup = FTDM_FALSE;
				tap->pcalls[i].recv_conn = FTDM_FALSE;
				tap->pcalls[i].recv_other_msg = FTDM_FALSE;
				tap->pcalls[i].chan_pres = FTDM_FALSE;
				tap->pcalls[i].chanId = 0;
				memset(&tap->pcalls[i].callingnum.digits, 0, sizeof(tap->pcalls[i].callingnum.digits));
				memset(&tap->pcalls[i].callednum.digits, 0, sizeof(tap->pcalls[i].callednum.digits));
				memset(&tap->pcalls[i].callingnum, 0, sizeof(tap->pcalls[i].callingnum));
				memset(&tap->pcalls[i].callednum, 0, sizeof(tap->pcalls[i].callednum));
				break;
			}
		}
		pcall = NULL;
	}

	ftdm_mutex_unlock(tap->pcalls_lock);
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_check_stale_conevt()
 *
 * Desc  : Remove call from hash list if connect/proceed event is recieved before
 *	   60sec and till yet setup is not received for the same call ref value
 *
 * Ret   : VOID
**********************************************************************************/
static void sangoma_isdn_tap_check_stale_conevt(sngisdn_tap_t *tap)
{
	ftdm_hash_iterator_t *idx = NULL;
	con_call_info_t *call_info = NULL;
	const void *key = NULL;
	void *val = NULL;
	char call_ref[25];
	time_t curr_time = time(NULL);


	/* Iterate through all hash list of call on which connect or proceed is received */
	for (idx = hashtable_first(tap->pend_call_lst); idx; idx = hashtable_next(idx)) {
		hashtable_this(idx, &key, NULL, &val);
		if (!key || !val) {
			break;
		}

		curr_time = time(NULL);

		call_info = (con_call_info_t*)val;
		time(&curr_time);
		/* remove the entry from hashlist if we have waited for 60sec or more for the setup to received */
		if (difftime(curr_time, call_info->conrecv_time) >= SNG_TAP_STALE_TME_DURATION) {
			memset(call_ref, 0, sizeof(call_ref));
			sprintf(call_ref, "%d", call_info->callref_val);

			ftdm_log(FTDM_LOG_CRIT, "[%s Callref- %d] Removing Stale call as found one in CONNECT hash list.\n", tap->span->name, call_info->callref_val);
			hashtable_remove(tap->pend_call_lst, (void *)call_ref);
			ftdm_safe_free(call_info);
		}
	}
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_get_fchan_by_chanid()
 *
 * Desc  : Get channel structure as per channel ID presetn in ISDN message
 *
 * Ret   : NULL | ftdm channel
**********************************************************************************/
ftdm_channel_t* sangoma_isdn_tap_get_fchan_by_chanid(sngisdn_tap_t *tap, int channel)
{
	ftdm_channel_t *fchan = NULL;
	int idx = 0;
	uint16_t chanpos = SNG_ISDN_CHANNEL(channel);

	if (!tap || !channel) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid channel %d or tap pointer \n", channel);
		return NULL;
	}

	if (!chanpos || (chanpos > tap->span->chan_count)) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid channel %d as against chanpos %d ,chan_count %d requested in span %s\n",
				channel, chanpos, tap->span->chan_count,tap->span->name);
		return NULL;
	}

	/* Check through all the configured channels matching channel ID and return the channel structure */
	for (idx = 1; idx < 128; idx++) {
		fchan = tap->span->channels[SNG_ISDN_CHANNEL(idx)];
		/* fchan has to be present for all tap channel..break if fchan is not present */
		if ((fchan == NULL) || (fchan->physical_chan_id == channel)) {
			break;
		}
	}

	if (!fchan) {
		ftdm_log(FTDM_LOG_ERROR, "Channel %d requested on span %s not found\n", channel, tap->span->name);
		return NULL;
	}

	return fchan;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_is_fchan_inuse()
 *
 * Desc  : check if fchan requested is already in-use or already open
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t sangoma_isdn_tap_is_fchan_inuse(sngisdn_tap_t *tap, int channel)
{
	ftdm_status_t ret = FTDM_FAIL;
	ftdm_channel_t *fchan = NULL;

	ftdm_log(FTDM_LOG_INFO, "Getting channel %d status\n", channel);

	if (NULL == (fchan = sangoma_isdn_tap_get_fchan_by_chanid(tap, channel))) {
		return ret;
	}

	ftdm_channel_lock(fchan);

	/* Now check if channel is already in use  */
	if ((ftdm_test_flag(fchan, FTDM_CHANNEL_INUSE)) || (ftdm_test_flag(fchan, FTDM_CHANNEL_OPEN))) {
		ftdm_log_chan(fchan, FTDM_LOG_INFO, "Channel is open or in use..state[%s]\n", ftdm_channel_state2str(fchan->state));
		ret =  FTDM_SUCCESS;
	}

	/* check if the channel is in INVALID state the please move it to TERMINATING state */
	if ((tap->tap_mode == ISDN_TAP_BI_DIRECTION) && (ret != FTDM_SUCCESS)) {
		if ((fchan->state != FTDM_CHANNEL_STATE_DOWN) && (fchan->state < FTDM_CHANNEL_STATE_TERMINATING)) {
			ftdm_log_chan(fchan, FTDM_LOG_INFO, "Channel in state [%s]. Please release this channel!\n", ftdm_channel_state2str(fchan->state));
			ret = FTDM_SUCCESS;
		} else if (fchan->state == FTDM_CHANNEL_STATE_HANGUP) {
			ftdm_log_chan_msg(fchan, FTDM_LOG_INFO, "Channel is in HANGUP state please move channel to DOWN state\n");
			ftdm_set_state(fchan, FTDM_CHANNEL_STATE_DOWN);
			ret = FTDM_SUCCESS;
		}
	}

	ftdm_channel_unlock(fchan);

	return ret;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_get_fchan()
 *
 * Desc  : Get the channel on which call needs to be tapped
 *
 * Ret   : fchan | NULL
**********************************************************************************/
static __inline__ ftdm_channel_t *sangoma_isdn_tap_get_fchan(sngisdn_tap_t *tap, passive_call_t *pcall, int channel, int force_clear_chan)
{
	ftdm_channel_t *fchan = NULL;
	int err = 0;

	fchan = sangoma_isdn_tap_get_fchan_by_chanid(tap, channel);
	if (!fchan) {
		err = 1;
		goto done;
	}

	ftdm_channel_lock(fchan);

	if (ftdm_test_flag(fchan, FTDM_CHANNEL_INUSE)) {
		reuse_chan_con_recv++;
		if (force_clear_chan) {
			ftdm_clear_flag(fchan, FTDM_CHANNEL_INUSE);
		} else {
			ftdm_log_chan(fchan, FTDM_LOG_ERROR, "Channel is already in use ..state[%s]\n", ftdm_channel_state2str(fchan->state));
			err = 1;
			goto done;
		}
	}

	if (ftdm_channel_open_chan(fchan) != FTDM_SUCCESS) {
		ftdm_log_chan(fchan, FTDM_LOG_ERROR, "Could not open tap channel %s\n","");
		err = 1;
		goto done;
	}
	ftdm_log_chan(fchan, FTDM_LOG_DEBUG, "Tap channel openned successfully \n %s","");

	memset(fchan->caller_data.cid_num.digits, 0, sizeof(fchan->caller_data.cid_num.digits));
	memset(fchan->caller_data.dnis.digits, 0, sizeof(fchan->caller_data.dnis.digits));
	memset(fchan->caller_data.cid_name, 0, sizeof(fchan->caller_data.cid_name));
	fchan->caller_data.call_reference = 0;

	ftdm_set_string(fchan->caller_data.cid_num.digits, pcall->callingnum.digits);
	ftdm_set_string(fchan->caller_data.cid_name, pcall->callingnum.digits);
	ftdm_set_string(fchan->caller_data.dnis.digits, pcall->callednum.digits);
	fchan->caller_data.call_reference = pcall->callref_val;

done:
	if (fchan) {
		ftdm_channel_unlock(fchan);
	}

	if (err) {
		return NULL;
	}

	return fchan;
}

/**********************************************************************************
 * Fun   : send_protocol_raw_data()
 *
 * Desc  : Send the tap raw message information to Application on receipt of
 *	   release/disconnect message
 *
 * Ret   : void
**********************************************************************************/
void send_protocol_raw_data(isdn_tap_msg_info_t *data, ftdm_channel_t *ftdmchan)
{
	void *mydata;
	ftdm_sigmsg_t	sig;

	ftdm_assert(data, "Invalid protocol raw data pointer. \n");
	ftdm_assert(ftdmchan, "Invalid ftdm channel pointer. \n");

	memset(&sig, 0, sizeof(ftdm_sigmsg_t));
	sig.event_id = FTDM_SIGEVENT_RAW;
	sig.channel = ftdmchan;

	mydata = ftdm_malloc(sizeof(isdn_tap_msg_info_t));
	ftdm_assert(mydata, "Failed to allocate memory\n");
	memcpy(mydata, data, sizeof(isdn_tap_msg_info_t));

	ftdm_sigmsg_set_raw_data(&sig, mydata, sizeof(isdn_tap_msg_info_t));

	if (ftdm_span_send_signal(ftdmchan->span, &sig) != FTDM_SUCCESS) {
		ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "Failed to send raw data event to user \n");
	}
}

/**********************************************************************************
 * Fun   : handle_isdn_passive_event()
 *
 * Desc  : Handle the events(i.e. Q931 messgages) once received and start tapping
 *	   call depending up on the events received
 *
 * Ret   : void
**********************************************************************************/
static void handle_isdn_passive_event(sngisdn_tap_t *tap, uint8_t msgType, sng_decoder_interface_t *intfCb)
{
	sngisdn_tap_t *peertap = tap->peerspan->signal_data;
	ftdm_bool_t is_con_recv = FTDM_FALSE;
	ftdm_status_t status = FTDM_FAIL;
	sngisdn_tap_t *tmp_tap = NULL;
	time_t curr_time = time(NULL);
	passive_call_t *pcall = NULL;
	ftdm_span_t *span = NULL;
	int crv = 0;

	if (!intfCb) {
		ftdm_log(FTDM_LOG_ERROR, "handle_isdn_passive_event failed.Invalid intfCb \n");
		return;
	}

	crv = intfCb->call_ref;

	if (intfCb->chan_id) {
		ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Channel Id present for message %s\n",
			 tap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));
	}

	if (tap->nfas.trunk) {
		if ((intfCb->interface_id < 0) && (intfCb->chan_id)) {
			ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Invalid Interface ID %d for message %s\n",
					tap->span->name, intfCb->chan_id, crv, intfCb->interface_id, SNG_DECODE_ISDN_EVENT(msgType));
			return;
		}

		if (intfCb->interface_id > -1) {
			if (!tap->nfas.trunk->tap_spans[intfCb->interface_id]) {
				ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] NFAS group %s does not have logical span ID %d for message %s\n",
						tap->span->name, intfCb->chan_id, crv, tap->nfas.trunk->name, intfCb->interface_id, SNG_DECODE_ISDN_EVENT(msgType));
				return;
			} else {
				span = tap->nfas.trunk->tap_spans[intfCb->interface_id];

				tap = span->signal_data;
				peertap = tap->peerspan->signal_data;
			}
		} else {
			pcall = sangoma_isdn_tap_get_nfas_pcall_bycrv(&tap, crv);
			if (!pcall) {
				pcall = sangoma_isdn_tap_get_nfas_pcall_bycrv(&peertap, crv);
				if (!pcall) {
					switch (msgType) {
						case Q931_SETUP:
						case Q931_SETUP_ACK:
						case Q931_PROGRESS:
						case Q931_PROCEEDING:
						case Q931_ALERTING:
						case Q931_INFORMATION:
						case Q931_CONNECT:
						case Q931_DISCONNECT:
						case Q931_RELEASE:
							ftdm_log(FTDM_LOG_NOTICE, "[%s Callref- %d] No tap/span found for %s message\n",
									tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));
							break;

						default:
							break;
					}
				} else {
					ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d] Call found on tap peerspan\n", peertap->span->name, crv);
				}
				tap = peertap->peerspan->signal_data;
			} else {
				ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d] Call found on tap span\n", tap->span->name, crv);
			}
			peertap = tap->peerspan->signal_data;
		}
	}

	pcall = NULL;

	switch (msgType) {
		case Q931_SETUP:
			/* Channel id is mandatory in SETUP */
			if (!intfCb->chan_id) {
					ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Invalid Channel found in setup message\n",
							 tap->span->name, intfCb->chan_id, crv);
					setup_recv_invalid_chanId++;
					break;
			}

			/* we cannot use ftdm_channel_t because we still dont know which channel will be used
			 * (ie, flexible channel was requested), thus, we need our own list of call references */
			ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Call SETUP recieved\n",
					 tap->span->name, intfCb->chan_id, crv);

			/* increasing the counter of number of setup received */
			setup_recv++;
			status = sangoma_isdn_tap_is_con_recv(tap, crv, msgType);
			if (status == FTDM_SUCCESS) {
				is_con_recv = FTDM_TRUE;
			} else if (status == FTDM_BREAK) {
				pcall = sangoma_isdn_tap_get_pcall(tap, &crv);
				if (pcall) {
					/* check if the SETUP is received after 5 seconds then the call present is a stale and 
					 * release that call and create a new one */
					time(&curr_time);
					if (!(difftime(curr_time, pcall->t_con_recv) > SNG_TAP_SUBEQUENT_MSG_TME_DURATION)) {
						is_con_recv = FTDM_FALSE;
						goto done;
					} else {
						msg_recv_aft_time++;
						ftdm_log(FTDM_LOG_DEBUG,
							 "[%s:%d Callref- %d] Creating a new pcall as SETUP is received after %d seconds of CONNECT message is received\n",
							 tap->span->name, intfCb->chan_id, crv, SNG_TAP_SUBEQUENT_MSG_TME_DURATION);
					}
				} else {
					ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Pcall not found for which tapping is already started\n",
						 tap->span->name, intfCb->chan_id, crv);
					break;
				}
			}

			/* check if pcall with same crv is already present and for the same other messages like SETUP_ACK,
			 * ALERT, PROGRESS or PROCEED is already being received with channel ID then please do not create
			 * pcall again and proceed with getting pcall parameters from the SETUP message received */
			pcall = sangoma_isdn_tap_get_pcall_bycrv(tap, crv);
			if (pcall) {
				if (pcall->recv_other_msg) {
					/* if the other messages are received more than 5 seconds before than please release the previous
					 * pcall proceed with creating a new one */
					time(&curr_time);
					if (!(difftime(curr_time, pcall->t_msg_recv) > SNG_TAP_SUBEQUENT_MSG_TME_DURATION)) {
						is_con_recv = FTDM_FALSE;
						goto done;
					} else {
						msg_recv_aft_time++;
						ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Creating a new pcall SETUP is received after %d seconds\n",
							 tap->span->name, intfCb->chan_id, crv, SNG_TAP_SUBEQUENT_MSG_TME_DURATION);
					}
				}
			}

			pcall = sangoma_isdn_tap_create_pcall(tap, msgType, intfCb, crv);
			if (!pcall) {
				ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Failed to create pcall for %s\n",
					     tap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));
				break;
			}

			pcall->callref_val = crv;
			pcall->chanId = intfCb->chan_id;

done:
			pcall->recv_setup = FTDM_TRUE;
			/* fill in the time at which setup is received */
			time(&pcall->t_setup_recv);

			/* If called number is present then only fill the same */
			if (!ftdm_strlen_zero(intfCb->cld_num)) {
				ftdm_log(FTDM_LOG_DEBUG,"cld_num[%s]\n", intfCb->cld_num);
				ftdm_set_string(pcall->callednum.digits, intfCb->cld_num);
			}

			/* If calling number is present then only fill the same */
			if (!ftdm_strlen_zero(intfCb->clg_num)) {
				ftdm_log(FTDM_LOG_DEBUG,"clg_num[%s]\n", intfCb->clg_num);
				ftdm_set_string(pcall->callingnum.digits, intfCb->clg_num); 
				ftdm_log(FTDM_LOG_DEBUG,"clg_num[%s]\n", pcall->callingnum.digits);
			}

			sangoma_isdn_tap_set_tap_msg_info(tap, msgType, pcall, intfCb);

			/* Get the channel on which SETUP is received and then generate an Invite */
			if ((pcall->chan_pres == FTDM_FALSE) && (tap->tap_mode == ISDN_TAP_BI_DIRECTION)) {
				sangoma_isdn_bidir_tap_generate_invite(tap, pcall, crv);
			}

			/* Check if respective CONNECT for this SETUP message is already being received */
			if (is_con_recv == FTDM_TRUE) {
				tmp_tap = tap;
				tap = peertap;
				peertap = tmp_tap;
				msgType = Q931_CONNECT;
				ftdm_log(FTDM_LOG_DEBUG,"[%s:%d Callref- %d] Sending CONNECT event, as CONNECT is already received earlier\n",
					 tap->span->name, intfCb->chan_id, crv);

				/* Since CONNECT is already being received for this SETUP message, therefore, processing connect message 
				 * in order to start tapping calls */
				if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
					sangoma_isdn_bidir_tap_handle_event_con(tap, msgType, intfCb, crv);
				} else {
					sangoma_isdn_tap_handle_event_con(tap, msgType, intfCb, crv);
				}
			}

			break;

		case Q931_SETUP_ACK:
		case Q931_PROGRESS:
		case Q931_PROCEEDING:
		case Q931_ALERTING:
			/* NOTE ALL THESE MESSAGES i.e SETUP_ACK, PROGRESS, PROCEEDING, ALERTING, will be transmitted by B leg
			 * We assume A Leg on which SETUP is received
			 * SETUP is sent by A LEG as shown below:
			 * A Leg ------- SETUP ------->  B Leg
			 * A Leg <------ SETUP ACK ----  B Leg
			 * A Leg <------ PROCEED ------  B Leg
			 * A Leg <------- ALERT -------- B Leg
			 * A Leg <------ CONNECT ------  B Leg
			 * A Leg ----- CONNECT ACK ----> B Leg
			 */
			/* check if channel Id is present in message? If channel Id is not present then no need to process message */
			/* Since we only start tapping when we received CONNECT and we would require channel ID in order to start tapping when received connect
			 * messages therefore, when any of the messages i.e. PROGRESS, PROCEED, ALERT, SETUP_ACK is received we will check if the same call reference
			 * is present and for that call reference tapping is being started or not. There may be three condition as given below:
			 * 1) if crv is not present in the hash list then please check the following:
			 *	  1.1) if pcall is present and connect is received befor 5 seconds then release the call and create a new one else 
			 *		   do nothing as it may be due to syncronisation issue during load conditions.
			 *	  1.2) if pcall is present and connect is not received, then check if setup is already received check is the setup is 
			 *	       is received before 5 seconds then release the call and create a new one else do nothing as it may be due to 
			 *	       syncronization issue during load conditions
			 *
			 *	  1.3) if pcall is not present then check for stale and create a new one as it is a new call.
			 *
			 * 2) if tapping is started already the check if the connnect message on that pcall is received more the 5 seconds before
			 *    then release the previous call as it is a stale and create a new pcall and wait for connect message to be received.
			 *
			 * 3) if connect is receive and tapping is not started yet then create a new pcall start tapping.
			 * */
			if (intfCb->chan_id) {
				ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Call %s received\n", tap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));
				status = sangoma_isdn_tap_is_con_recv(peertap, crv, msgType);

				/* when call is not present in connect hash list */
				if ((status == FTDM_FAIL) || (status == FTDM_BREAK))  {
					pcall = sangoma_isdn_tap_get_pcall_bycrv(peertap, crv);
					if (pcall) {
						/* for status == FTDM_BREAK 1 condition i.e. pcall->recv_conn is always true */
						if (pcall->recv_conn) {
							/* check if connect  message has been received more than 5 seconds before then please release the previous
							 * call  and create a new one */
							time(&curr_time);
							if (!(difftime(curr_time, pcall->t_con_recv) > SNG_TAP_SUBEQUENT_MSG_TME_DURATION)) {
								/* possible that due to syncronisation issue message is not in proper order */
								break;
							} else {
								msg_recv_aft_time++;
								ftdm_log(FTDM_LOG_DEBUG,
									 "[%s:%d Callref- %d] Creating a new pcall as %s is received after %d seconds of CONNECT message is received.\n",
									 peertap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType), SNG_TAP_SUBEQUENT_MSG_TME_DURATION);
							}
						} else if (pcall->recv_setup) {
							/* check if setup message has been received more than 5 seconds before then please release the previous
							 * call  and create a new one */
							time(&curr_time);
							if (!(difftime(curr_time, pcall->t_setup_recv) > SNG_TAP_SUBEQUENT_MSG_TME_DURATION)) {
								break;
							} else {
								msg_recv_aft_time++;
								ftdm_log(FTDM_LOG_DEBUG,
									 "[%s:%d Callref- %d] Creating a new pcall as %s is received after %d seconds of SETUP message is received.\n",
									 peertap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType), SNG_TAP_SUBEQUENT_MSG_TME_DURATION);
							}
						}
					}
					/* when call is present in hash list but tapping is not started connect is waiting for SETUP */
				}

				/* create pcall */
				pcall = sangoma_isdn_tap_create_pcall(peertap, msgType, intfCb, crv);
				if (!pcall) {
					ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Failed to create pcall for %s\n",
							peertap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));
					break;
				}
				pcall->callref_val = crv;
				pcall->chanId = intfCb->chan_id;
				pcall->recv_setup = FTDM_FALSE;
				pcall->recv_other_msg = FTDM_TRUE;
				time(&pcall->t_msg_recv);

				/* Get the channel on which SETUP is received and then generate an Invite */
				if ((pcall->chan_pres == FTDM_FALSE) && (peertap->tap_mode == ISDN_TAP_BI_DIRECTION)) {
					sangoma_isdn_bidir_tap_generate_invite(peertap, pcall, crv);
				}

				ftdm_log(FTDM_LOG_DEBUG,"[%s:%d Callref- %d] Sending CONNECT event, as CONNECT is already received earlier on receipt of %s\n",
						tap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));

				if (status == FTDM_SUCCESS) {
					pcall->recv_other_msg = FTDM_FALSE;
					pcall->t_msg_recv = time(NULL);

					msgType = Q931_CONNECT;
					/* Since CONNECT is already being received for this call reference value, therefore, processing connect message
					 * in order to start tapping calls */
					if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
						sangoma_isdn_bidir_tap_handle_event_con(tap, msgType, intfCb, crv);
					} else {
						sangoma_isdn_tap_handle_event_con(tap, msgType, intfCb, crv);
					}
					/* when call is present in hash list and tapping is already started */
				}


			}
			break;

		case Q931_INFORMATION:
			{
				ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d]INFORMATION message recieved.\n",
						tap->span->name, intfCb->chan_id, crv);

				pcall = sangoma_isdn_tap_get_pcall_bycrv(tap, crv);

				if (pcall) {
					/* If called number is present then only fill the same */
					if (!ftdm_strlen_zero(intfCb->cld_num)) {
						char digits[25];
						/* Append received digits with existing called party number */
						sprintf(digits,"%s%s",pcall->callednum.digits,intfCb->cld_num);
						ftdm_set_string(pcall->callednum.digits, digits);
						ftdm_log(FTDM_LOG_DEBUG,"[%s Callref- %d]Received cld_num[%s] in INFORMATION message. Called Number Digits[%s] \n",
								tap->span->name, crv, intfCb->cld_num, pcall->callednum.digits);
					}
				}

				break;
			}

		case Q931_CONNECT:
			connect_recv++;

			if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
				sangoma_isdn_bidir_tap_handle_event_con(tap, msgType, intfCb, crv);
			} else {
				sangoma_isdn_tap_handle_event_con(tap, msgType, intfCb, crv);
			}
			break;

		case Q931_DISCONNECT:
		case Q931_RELEASE:
			if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
				sangoma_isdn_bidir_tap_handle_event_rel(tap, msgType, crv);
			} else {
				sangoma_isdn_tap_handle_event_rel(tap, msgType, crv);
			}
			break;

		default:
			ftdm_log(FTDM_LOG_DEBUG, "Ignoring passive event %d on span %s\n", msgType, tap->span->name);
			break;

	}
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_create_pcall()
 *
 * Desc  : Check for stale call and then create a new pcall if already not present
 *
 * Ret   : tap->pcalls | NULL
**********************************************************************************/
passive_call_t *sangoma_isdn_tap_create_pcall(sngisdn_tap_t *tap, uint8_t msgType, sng_decoder_interface_t *intfCb, int crv)
{
	sngisdn_tap_t *peertap = tap->peerspan->signal_data;
	passive_call_t *pcall =  NULL;

	/* check for stale call */
	sangoma_isdn_tap_check_stale_call(tap, intfCb, crv, msgType);

	/* reset pcall if in case pcall is present with same channel Id */
	sangoma_isdn_tap_reset_pcall_bychanId(tap, intfCb, crv);

	/* Try to get a recycled call (ie, get the call depending up on the crv value that the PRI stack allocated previously and then
	 * re-used for the next RING event because we did not destroy it fast enough) */
	pcall = sangoma_isdn_tap_get_pcall(tap, &crv);
	if (!pcall) {
		ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Creating new pcall for message %s\n",
			 tap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));

		/* ok so the call is really not known to us, let's get a new one */
		/* Creating a new pcall */
		pcall = sangoma_isdn_tap_get_pcall(tap, NULL);
		if (!pcall) {
			ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Failed to get a free passive ISDN call slot for message %s\n",
				 tap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));

			/* PRINT PCALL */
			ftdm_log(FTDM_LOG_ERROR, "***** Print TAP Pcalls structure *****\n");
			sangoma_isdn_tap_print_pcall(tap);

			ftdm_log(FTDM_LOG_ERROR, "\n***** Print PEERTAP Pcalls structure *****\n");
			sangoma_isdn_tap_print_pcall(peertap);

			return NULL;
		}
		/* pcall initialization */
		ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d]Initializing pcall for message %s\n",
				 tap->span->name, intfCb->chan_id, crv, SNG_DECODE_ISDN_EVENT(msgType));
		sangoma_isdn_tap_reset_pcall(pcall);
	}

	return pcall;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_handle_event_con()
 *
 * Desc  : Handle connection/call proceeding event and start tapping call by
 * 	   changing state to UP.
 *
 * Ret   : void
**********************************************************************************/
static ftdm_status_t sangoma_isdn_tap_handle_event_con(sngisdn_tap_t *tap, uint8_t msgType, sng_decoder_interface_t *intfCb, int crv)
{
	sngisdn_tap_t   *peertap     = tap->peerspan->signal_data;
	char	        call_ref[25] = { 0 };
	con_call_info_t *call_info   = NULL;
	passive_call_t  *peerpcall   = NULL;
	ftdm_channel_t  *peerfchan   = NULL;
	passive_call_t  *pcall 	     = NULL;
	ftdm_channel_t  *fchan 	     = NULL;
	char		*crv_key     = NULL;

	/* CONNECT will come from B end (A to B call) ,
	 * peertap is refering to A party here and tap referring to B party */

	/* CONNECT - will come on B leg , whereas SETUP received on A leg */
	/* tap , pcall , fchan = B leg
	 * peertap, peerptap , peerfchan = A leg
	 */

	/* check that we already know about this call in the peer PRI (which was the one receiving the PRI_EVENT_RING event) */
	if (!(peerpcall = sangoma_isdn_tap_get_pcall_bycrv(peertap, crv))) {
		/* We have not found peerpcall, that means on A leg we didnt receive any setup
		 * This can happen due to syncronization/timing issue where we process B leg first then A leg */

		/* Add the call inforamtion in to call hash list based on call reference value if it is not present */
		call_info = ftdm_calloc(sizeof(*call_info), 1);
		if (!call_info) {
			ftdm_log(FTDM_LOG_ERROR, "Unable to allocate memory with crv = %d in peer tap call list for span %s\n", crv, tap->span->name);
			return FTDM_FAIL;
		}

		/* Initializing call info structure values */
		call_info->conrecv_time = time(NULL);
		call_info->callref_val = 0;
		call_info->msgType = 0;
		call_info->recv_con = FTDM_FALSE;
		call_info->recv_setup = FTDM_FALSE;
		call_info->tap_proceed = FTDM_FALSE;

		/* Check if the same call referece is already present in hash list */
		sprintf(call_ref, "%d", crv);
		if (hashtable_search(peertap->pend_call_lst, (void *)call_ref)) {
			ftdm_log(FTDM_LOG_CRIT, "Call with call referece %d is already inserted in the hash table\n", crv);
			ftdm_safe_free(call_info);
			return FTDM_SUCCESS;
		}

		/* Fill in the call reference value */
		call_info->callref_val = crv;

		/* Fill in the time at which connect event is received */
		time(&call_info->conrecv_time);

		/* Fill in message Type */
		call_info->msgType = msgType;

		if (msgType == Q931_CONNECT) {
			call_info->recv_con = FTDM_TRUE;
		} else {
			call_info->recv_con = FTDM_FALSE;
		}
		call_info->recv_setup = FTDM_FALSE;

		if (intfCb->chan_id) {
			call_info->tap_proceed = FTDM_TRUE;
		} else {
			call_info->tap_proceed = FTDM_FALSE;
		}

		memset(call_ref, 0 , sizeof(call_ref));
		sprintf(call_ref, "%d", crv);

		crv_key = ftdm_strdup(call_ref);

		if (!hashtable_insert(peertap->pend_call_lst, (void *)crv_key, call_info, HASHTABLE_FLAG_FREE_KEY)) {
			ftdm_log(FTDM_LOG_ERROR, "Unable to insert call info for call reference value as %d\n", crv);
			return FTDM_FAIL;
		}

		if (intfCb->chan_id) {
			ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Found channel ID in CONNECT message\n", tap->span->name, intfCb->chan_id, crv);

			peerpcall = sangoma_isdn_tap_create_pcall(peertap, msgType, intfCb, crv);
			if (!peerpcall) {
				ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Failed to create pcall. So, Lets wait for SETUP to be received",
						peertap->span->name, intfCb->chan_id, crv);
				return FTDM_FAIL;
			}

			peerpcall->callref_val = crv;
			peerpcall->chanId = intfCb->chan_id;
			peerpcall->recv_setup = FTDM_FALSE;

			connect_recv_with_chanId++;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d] Hold event processing as %s event is received before SETUP\n",
					tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));

			ftdm_log(FTDM_LOG_DEBUG,
					"[%s Callref- %d] Ignoring %s event Waiting for Call SETUP to receive as channel Id is not present in CONNECT message\n",
					tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));

			return FTDM_FAIL;
		}
	}

	/* at this point we should know the real b chan that will be used and can therefore proceed to notify about the call, but
	 * only if a couple of call tests are passed first */
	ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] %s event receive\n", tap->span->name, peerpcall->chanId, crv, SNG_DECODE_ISDN_EVENT(msgType));

	peerpcall->recv_conn = FTDM_TRUE;
	/* Fill in the time at which connect event is received */
	time(&peerpcall->t_con_recv);

	/* This call should not be known to this ISDN yet ... */
	if ((pcall = sangoma_isdn_tap_get_pcall_bycrv(tap, crv))) {
		ftdm_log(FTDM_LOG_ERROR,
				"[%s:%d Callref- %d] Ignoring subsequent connect event\n",
				tap->span->name, peerpcall->chanId, crv);
		return FTDM_FAIL;
	}

	/* Check if the call pointer is being recycled */
	pcall = sangoma_isdn_tap_get_pcall(tap, &crv);
	if (!pcall) {
		/* creating a new peer pcall */
		pcall = sangoma_isdn_tap_get_pcall(tap, NULL);
		if (!pcall) {
			ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Failed to get a free peer ISDN passive call slot!\n",
					tap->span->name, peerpcall->chanId, crv);

			ftdm_log(FTDM_LOG_ERROR, "***** Print TAP Pcalls structure *****\n");
			sangoma_isdn_tap_print_pcall(tap);

			ftdm_log(FTDM_LOG_ERROR, "\n***** Print PEERTAP Pcalls structure *****\n");
			sangoma_isdn_tap_print_pcall(peertap);

			return FTDM_FAIL;
		}
		/* pcall initialization */
		sangoma_isdn_tap_reset_pcall(pcall);

		pcall->callref_val = crv;
		if (peerpcall->chanId) {
			pcall->chanId = peerpcall->chanId;
			ftdm_log(FTDM_LOG_DEBUG, "Setting chanId as %d for %s event received on Span %s from its peer\n", pcall->chanId, SNG_DECODE_ISDN_EVENT(msgType), tap->span->name);
		} else if (intfCb->chan_id) {
			pcall->chanId = intfCb->chan_id;
			ftdm_log(FTDM_LOG_DEBUG, "Setting chanId as %d for %s event received on Span %s\n", pcall->chanId, SNG_DECODE_ISDN_EVENT(msgType), tap->span->name);
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Invalid chanId as %d for %s event received on Span %s\n", intfCb->chan_id, SNG_DECODE_ISDN_EVENT(msgType), tap->span->name);
			return FTDM_FAIL;
		}
	}

	pcall->recv_conn = FTDM_TRUE;

	/* tap = B */
	fchan = sangoma_isdn_tap_get_fchan(tap, pcall, pcall->chanId, 1);
	if (!fchan) {
		ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Connect requested on odd/unavailable\n",
				tap->span->name, pcall->chanId, crv);
		return FTDM_FAIL;
	}

	/* peerfchan = A party (in a to b call) */
	peerfchan = sangoma_isdn_tap_get_fchan(peertap, peerpcall, peerpcall->chanId, 1);
	if (!peerfchan) {
		ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Connect requested on odd/unavailable channel\n",
				peertap->span->name, peerpcall->chanId, crv);
		return FTDM_FAIL;
	}

	pcall->fchan = fchan;
	peerpcall->fchan = peerfchan;

	ftdm_channel_lock(fchan);
	fchan->call_data = peerfchan; /* fchan=B leg, pointing to peerfchan(A leg) */
	ftdm_set_state(fchan, FTDM_CHANNEL_STATE_RING);
	ftdm_channel_unlock(fchan);

	/* update span state */
	sangoma_isdn_tap_check_state(tap->span);

	ftdm_channel_lock(peerfchan);
	peerfchan->call_data = fchan; /* peerfchan=A leg(SETUP received), pointing to fchan(B leg) */
	if (tap->mixaudio == ISDN_TAP_MIX_NULL) {
		ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_RING);
	}
	ftdm_channel_unlock(peerfchan);

	/* update peer span state */
	sangoma_isdn_tap_check_state(peertap->span);

	/* CONNECT will come on B leg
	 * tap , pcall , fchan = B leg
	 * */

	if (!pcall) {
		ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Getting the pcall\n",
				tap->span->name, peerpcall->chanId, crv);

		if (!(pcall = sangoma_isdn_tap_get_pcall_bycrv(tap, crv))) {
			ftdm_log(FTDM_LOG_ERROR,
					"[%s:%d Callref- %d] Ignoring Connect event since we don't know about it\n",
					tap->span->name, peerpcall->chanId, crv);
			return FTDM_FAIL;
		}
	}

	sangoma_isdn_tap_set_tap_msg_info(tap, msgType, pcall, intfCb);

	pcall->tap_msg.chan_id = peerpcall->tap_msg.chan_id;
	/* Setup flag as call is connected */
	pcall->tap_msg.call_con = 1;

	/* No called/calling party address is present in CONNECT message */
	memset(pcall->tap_msg.called_num, 0, sizeof(pcall->tap_msg.called_num));
	memset(pcall->tap_msg.calling_num, 0, sizeof(pcall->tap_msg.calling_num));

	if (!ftdm_strlen_zero(peerpcall->tap_msg.called_num)) {
		ftdm_set_string(pcall->tap_msg.called_num, peerpcall->tap_msg.called_num);
		ftdm_log(FTDM_LOG_DEBUG, "Setting Called Number[%s] in tap message structure\n", pcall->tap_msg.called_num);
	}

	if (!ftdm_strlen_zero(peerpcall->tap_msg.calling_num)) {
		ftdm_set_string(pcall->tap_msg.calling_num, peerpcall->tap_msg.calling_num);
		ftdm_log(FTDM_LOG_DEBUG, "Setting Calling Number[%s] in tap message structure\n", pcall->tap_msg.calling_num);
	}

	peerpcall->tap_msg.call_con = 1;

	if (!pcall->fchan) {
		ftdm_log(FTDM_LOG_ERROR,
				"[%s:%d Callref- %d] Received answer, but we never got a channel\n",
				tap->span->name, pcall->chanId, crv);
		return FTDM_FAIL;
	}

	if ((tap->mixaudio == ISDN_TAP_MIX_BOTH) && !(fchan->caller_data.cid_num.digits)) {
		ftdm_set_string(fchan->caller_data.cid_num.digits, peerpcall->callingnum.digits);
		ftdm_set_string(fchan->caller_data.cid_name, peerpcall->callingnum.digits);
		ftdm_set_string(fchan->caller_data.dnis.digits, peerpcall->callednum.digits);
		fchan->caller_data.call_reference = crv;
	}

	/* Once Call is answered then only change channel state to UP */
	ftdm_channel_lock(pcall->fchan);
	ftdm_log_chan(pcall->fchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] Tap call answered in state %s\n",
			tap->span->name, pcall->chanId, crv, ftdm_channel_state2str(pcall->fchan->state));

	ftdm_set_pflag(pcall->fchan, ISDN_TAP_NETWORK_ANSWER);
	ftdm_set_state(pcall->fchan, FTDM_CHANNEL_STATE_UP);
	ftdm_channel_unlock(pcall->fchan);

	/* update span state */
	sangoma_isdn_tap_check_state(tap->span);

	if (tap->mixaudio == ISDN_TAP_MIX_NULL) {
		/* B leg answered, move A leg state also to ANSWER */
		peerfchan = (ftdm_channel_t*)pcall->fchan->call_data;
		if (!peerfchan) {
			ftdm_log(FTDM_LOG_ERROR,"[%s:%d Callref- %d] Peer channel not found \n", peertap->span->name, peerpcall->chanId, crv);
			return FTDM_FAIL;
		}
		ftdm_channel_lock(peerfchan);
		ftdm_log_chan(peerfchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] PeerTap call was answered in state %s\n",
				peertap->span->name, peerpcall->chanId, crv, ftdm_channel_state2str(peerfchan->state));

		ftdm_set_pflag(peerfchan, ISDN_TAP_NETWORK_ANSWER);
		ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_UP);
		ftdm_channel_unlock(peerfchan);
	}
	/* update peer span state */
	 sangoma_isdn_tap_check_state(peertap->span);


	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_rel_stale_call()
 *
 * Desc  : Release stale call present on channnel on which PROCEED/CONNECT is
 *	   received
 *
 * Ret   : void
**********************************************************************************/
static ftdm_status_t sangoma_isdn_tap_rel_stale_call(sngisdn_tap_t *tap, int chan_id)
{
	passive_call_t *pcall = NULL;
	ftdm_channel_t *fchan = NULL;

	if (!(pcall = sangoma_isdn_tap_get_pcall_bychanId(tap, chan_id))) {
		/* change channel state as channel is already released by pcall */
		if ((tap->tap_mode == ISDN_TAP_BI_DIRECTION) || (tap->nfas.trunk)) {
			if (NULL == (fchan = sangoma_isdn_tap_get_fchan_by_chanid(tap, chan_id))) {
				ftdm_log_chan_msg(fchan, FTDM_LOG_ERROR, "Channel not found\n");
				goto done;
			}

			ftdm_channel_lock(fchan);
			if (fchan->last_state == FTDM_CHANNEL_STATE_HANGUP) {
				ftdm_set_state(fchan, FTDM_CHANNEL_STATE_DOWN);

				ftdm_log_chan_msg(fchan, FTDM_LOG_NOTICE, "Changing channel state to DOWN\n");
			} else if ((fchan->state < FTDM_CHANNEL_STATE_TERMINATING) && (fchan->state != FTDM_CHANNEL_STATE_DOWN)) {
				ftdm_set_state(fchan, FTDM_CHANNEL_STATE_TERMINATING);

				ftdm_log_chan_msg(fchan, FTDM_LOG_NOTICE, "Changing channel state to TERMINATING\n");
			}
			ftdm_channel_unlock(fchan);

			/* update span state */
			sangoma_isdn_tap_check_state(tap->span);
		}

done:
		ftdm_log(FTDM_LOG_DEBUG, "[%s:%d] No Stale pcall present\n", tap->span->name, chan_id);
		return FTDM_FAIL;
	}

	ftdm_log(FTDM_LOG_INFO, "[%s:%d Callref- %d] Releasing Stale Call\n", tap->span->name, chan_id, pcall->callref_val);
	if(tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
		sangoma_isdn_bidir_tap_handle_event_rel(tap, Q931_RELEASE, pcall->callref_val);
	} else {
		sangoma_isdn_tap_handle_event_rel(tap, Q931_RELEASE, pcall->callref_val);
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_handle_event_rel()
 *
 * Desc  : Handle call release/disconnect event and move channel to down state
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static ftdm_status_t sangoma_isdn_tap_handle_event_rel(sngisdn_tap_t *tap, uint8_t msgType, int crv)
{
	passive_call_t *pcall = NULL;
	ftdm_channel_t *fchan = NULL;
	passive_call_t *peerpcall = NULL;
	ftdm_channel_t *peerfchan = NULL;

	sngisdn_tap_t *peertap = tap->peerspan->signal_data;

	ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d] %s event received\n",
			tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));

	if (msgType == Q931_DISCONNECT) {
		/* increasing the counter of number of DISCONNECT received */
		disconnect_recv++;
	} else {
		/* increasing the counter of number of RELEASE received */
		release_recv++;
	}

	sangoma_isdn_tap_is_con_recv(tap, crv, msgType);

	if (!(pcall = sangoma_isdn_tap_get_pcall_bycrv(tap, crv))) {
		ftdm_log(FTDM_LOG_DEBUG,
				"[%s Callref- %d] Ignoring release/dissconnect since we don't know about it\n",
				tap->span->name, crv);
		goto end;
	}

	if (pcall->fchan) {
		fchan = pcall->fchan;

		if (tap->indicate_event == FTDM_TRUE) {
			ftdm_log(FTDM_LOG_DEBUG, "Tap Message sent to application on receipt of %s event on span[%d] chan[%d]\n",
				 SNG_DECODE_ISDN_EVENT(msgType), fchan->span_id, fchan->chan_id);
			/* Sending Call raw data to application */
			send_protocol_raw_data( &pcall->tap_msg, pcall->fchan);
		}

		ftdm_channel_lock(fchan);
		if (fchan->state < FTDM_CHANNEL_STATE_TERMINATING) {
			ftdm_set_state(fchan, FTDM_CHANNEL_STATE_TERMINATING);
			ftdm_log_chan(fchan, FTDM_LOG_DEBUG, "[%s:%d Callref- %d]Changing channel state to TERMINATING\n",
					tap->span->name, fchan->physical_chan_id, crv);
		}
		ftdm_channel_unlock(fchan);
		/* update span state */
		sangoma_isdn_tap_check_state(tap->span);
	}


	/* Move peer state also to disconnect */
	if (pcall->fchan) {
		peerfchan = (ftdm_channel_t*)pcall->fchan->call_data;
	}

	if (peerfchan) {
		if (peertap->indicate_event == FTDM_TRUE) {
			if ((peerpcall = sangoma_isdn_tap_get_pcall_bycrv(peertap, crv))) {
				if (peerfchan->span_id == peerpcall->fchan->span_id) {
					ftdm_log(FTDM_LOG_DEBUG, "Peer tap Message sent to application on receipt of %s event on span[%d] chan[%d]\n",
							SNG_DECODE_ISDN_EVENT(msgType), peerfchan->span_id, peerfchan->chan_id);
					/* Sending peer call raw data to application */
					send_protocol_raw_data( &peerpcall->tap_msg, peerpcall->fchan);
				}
			}
		}

		ftdm_channel_lock(peerfchan);
		if (peerfchan->state < FTDM_CHANNEL_STATE_TERMINATING) {
			ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_TERMINATING);
			ftdm_log_chan(peerfchan,FTDM_LOG_DEBUG,"[%s:%d Callref- %d] Changing peer channel state to TERMINATING\n",
					peertap->span->name, peerfchan->physical_chan_id, crv);
		}
		ftdm_channel_unlock(peerfchan);
		/* update peer span state */
		sangoma_isdn_tap_check_state(peertap->span);
	}

end:
	sangoma_isdn_tap_put_pcall(tap, &crv);
	sangoma_isdn_tap_put_pcall(peertap, &crv);

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_check_event()
 *
 * Desc  : Get the channel on which call needs to be tapped
 *
 * Ret   : fchan | NULL
**********************************************************************************/
static void sangoma_isdn_tap_check_event(ftdm_span_t *span)
{
	ftdm_event_t *event = NULL;

	/* Poll for events, e.g HW DTMF */
	while (ftdm_span_next_event(span, &event) == FTDM_SUCCESS) {
		if (event->e_type == FTDM_EVENT_OOB) {
			ftmod_isdn_process_event(span, event);
		}
	}
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_voice_span_run()
 *
 * Desc  : Run the sngidsn_tap voice span  thread and only check change of state
 * 	   as this is a pure voice span & signalling is getting taken care of by
 * 	   other tap span thread
 *
 * Ret   : void | NULL
**********************************************************************************/
static void *ftdm_sangoma_isdn_tap_voice_span_run(ftdm_thread_t *me, void *obj)
{
	ftdm_span_t *span = (ftdm_span_t *) obj;
	sngisdn_tap_t *tap = span->signal_data;
	ftdm_span_t *peer_span = NULL;

	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISDN pure voice thread started on span %s\n", span->name);
	ftdm_set_flag(span, FTDM_SPAN_IN_THREAD);

	peer_span = tap->peerspan;

	if (!tap) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid Tap value\n");
		goto done;
	}

	if (!ftdm_test_flag(tap, ISDN_TAP_MASTER)) {
		ftdm_log(FTDM_LOG_DEBUG, "Running dummy thread on pure voice span %s\n", span->name);
		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {
			/* We are not using this thread at all, hence just keep on sleeping */
			ftdm_sleep(100);
		}
	} else {
		ftdm_log(FTDM_LOG_DEBUG, "Master tapping thread on pure voice span %s\n", span->name);

		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {
			sangoma_isdn_tap_check_state(span);
			sangoma_isdn_tap_check_state(peer_span);
			ftdm_sleep(10);
		} /* end of while */
	} /* end of if */

done:
	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISDN pure voice thread ended on span %s\n", span->name);

	ftdm_clear_flag(span, FTDM_SPAN_IN_THREAD);
	ftdm_clear_flag(tap, ISDN_TAP_MASTER);

	sangoma_isdn_tap_free_hashlist(tap);

	return NULL;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_run()
 * 
 * Desc  : Run the sngidsn_tap thread and always keep on monitoring on the messages
 * 	   getting received on peers as well as self sngisdn_tap D-channel.
 * 	   Depending upon which tapping needs to be starting
 *
 * Ret   : void | NULL
**********************************************************************************/
static void *ftdm_sangoma_isdn_tap_run(ftdm_thread_t *me, void *obj)
{
	ftdm_span_t *span = (ftdm_span_t *) obj;
	sngisdn_tap_t *tap = span->signal_data;
	sngisdn_tap_t *peer_tap = NULL;
	ftdm_span_t *peer = NULL;

	/* Variables for required for polling */
	ftdm_channel_t *chan_poll[2] = { 0 };
	ftdm_wait_flag_t poll_events[2] = { 0 };
	uint32_t poll_status = 0;
	uint32_t waitms = 0;

	setup_recv = 0;
	connect_recv = 0;
	call_proceed_recv = 0;
	release_recv = 0;
	disconnect_recv = 0;
	call_tapped = 0;
	msg_recv_aft_time = 0;

	if (tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
		early_connect_recv = 0;
		late_invite_recv = 0;
	}

	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISDN thread started on span %s\n", span->name);

	ftdm_set_flag(span, FTDM_SPAN_IN_THREAD);

	if (ftdm_channel_open(span->span_id, tap->dchan->chan_id, &tap->dchan) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_ERROR, "Failed to open D-channel for span %s\n", span->name);
		goto done;
	}

	if ((tap->pri = sangoma_isdn_new_cb(tap->dchan->sockfd, PRI_NETWORK, SW_NI2, tap))) {
		sangoma_isdn_tap_set_debug(tap->pri, tap->debug);
	} else {
		ftdm_log(FTDM_LOG_CRIT, "Failed to create tapping ISDN\n");
		goto done;
	}

	/* The last span starting runs the show ...
	 * This simplifies locking and avoid races by having multiple threads for a single tapped link
	 * Since both threads really handle a single tapped link there is no benefit on multi-threading, just complications ... */
	peer = tap->peerspan;
	peer_tap = peer->signal_data;

	if (!ftdm_test_flag(tap, ISDN_TAP_MASTER)) {
		if (tap->nfas.sigchan == SNGISDN_NFAS_DCHAN_BACKUP) {
			ftdm_log(FTDM_LOG_DEBUG, "Running dummy thread on backup span %s\n", span->name);
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Running dummy thread on span %s\n", span->name);
		}

		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {
			/* We are not using this thread at all, hence just keep on sleeping */
			ftdm_sleep(100);
		}
	} else {
		if (tap->nfas.trunk) {
			while ((tap->nfas.trunk->num_tap_spans != tap->nfas.trunk->num_spans_configured)) {
				/* wait untill all NFAS spans are started */
				continue;
			}
		}

		if (tap->nfas.sigchan == SNGISDN_NFAS_DCHAN_BACKUP) {
			ftdm_log(FTDM_LOG_DEBUG, "Master tapping thread on backup span %s (fd1=%d, fd2=%d)\n",
				 span->name, tap->dchan->sockfd, peer_tap->dchan->sockfd);
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Master tapping thread on span %s (fd1=%d, fd2=%d)\n",
				 span->name, tap->dchan->sockfd, peer_tap->dchan->sockfd);
		}

		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {
			waitms = 10;
			sangoma_isdn_tap_check_state(span);
			sangoma_isdn_tap_check_state(peer);

			chan_poll[0] = tap->dchan;
			chan_poll[1] = peer_tap->dchan;

			poll_events[0] = FTDM_READ;
			poll_events[1] = FTDM_READ;

			/* polling on both the signalling channels i.e. waiting in channels to receive events */
			poll_status = ftdm_channel_wait_multiple(chan_poll, poll_events, 2, waitms);

			switch (poll_status) {
				case FTDM_TIMEOUT:
				case FTDM_BREAK:
					break;

				case FTDM_SUCCESS:

					if (ftdm_test_io_flag(tap->dchan, FTDM_CHANNEL_IO_READ)) {
						if (FTDM_SUCCESS == sangoma_isdn_read_msg(tap)) {
							sangoma_isdn_tap_check_state(span);
							sangoma_isdn_tap_check_state(peer);
						}
						sangoma_isdn_tap_check_state(span);
					}

					if (ftdm_test_io_flag(peer_tap->dchan, FTDM_CHANNEL_IO_READ)) {
						if (FTDM_SUCCESS == sangoma_isdn_read_msg(peer_tap)) {
							sangoma_isdn_tap_check_state(span);
							sangoma_isdn_tap_check_state(peer);
						}
						sangoma_isdn_tap_check_state(peer);
					}
				break;

				default:
					ftdm_log(FTDM_LOG_ERROR, "Invalid status.\n");
					poll_status = 0;
					break;
			}

			sangoma_isdn_tap_check_event(span);
			sangoma_isdn_tap_check_event(peer);
		} /* end of while */
	} /* end if */

done:
	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISDN thread ended on span %s\n", span->name);

	ftdm_clear_flag(span, FTDM_SPAN_IN_THREAD);
	ftdm_clear_flag(tap, ISDN_TAP_MASTER);

	sangoma_isdn_tap_free_hashlist(tap);

	return NULL;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_read_msg()
 *
 * Desc  : Read isdn message and process it of it is q931 message
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t sangoma_isdn_read_msg(sngisdn_tap_t *tap)
{
	int res = 0;
	char 	data[1024] = { 0 };
	uint8_t msgType = 0;					/* Type of the message received */
	sng_decoder_interface_t intf = { 0 };  	/* Interface structure between decoder and app */
	ftdm_status_t ret = FTDM_FAIL;

	/* Initializing variable */
	res = 0;

	/* Read messages process it iff its Q931 message */
	res = sangoma_isdn_io_read(tap->pri, data, sizeof(data));

	if (res < 5 || ((int)data[4] != 8)) {
		msgType = 0;
	} else {
		/* Need to configure the switch type using dialplan */
		msgType = (uint8_t)q931_decode(tap->switch_type, (data + 4), (res - 4), tap->decCb, &intf);
		ftdm_log_chan(tap->dchan, FTDM_LOG_DEBUG, "%s Message(%d) received on %s span\n", SNG_DECODE_ISDN_EVENT(msgType), msgType, tap->span->name);
	}

	/* If its a valid Q931 message then only handle events */
	if (msgType) {
		handle_isdn_passive_event(tap, msgType, &intf);
		ret = FTDM_SUCCESS;
	}

	return ret;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_free_hashlist()
 *
 * Desc  : Deallocate memory as allocated to tap call has list
 *
 * Ret   : void
**********************************************************************************/
static void sangoma_isdn_tap_free_hashlist(sngisdn_tap_t *tap)
{
	ftdm_hash_iterator_t *idx = NULL;
	con_call_info_t *call_info = NULL;
	const void *key = NULL;
	void *val = NULL;
	char call_ref[25];

	/* Iterate through all hash list of call on which connect or proceed is received */
	for (idx = hashtable_first(tap->pend_call_lst); idx; idx = hashtable_next(idx)) {
		hashtable_this(idx, &key, NULL, &val);
		if (!key || !val) {
			break;
		}

		call_info = (con_call_info_t*)val;
		memset(call_ref, 0, sizeof(call_ref));
		sprintf(call_ref, "%d", call_info->callref_val);

		hashtable_remove(tap->pend_call_lst, (void *)call_ref);
		ftdm_safe_free(call_info);
	}

	/* delete the hash table */
	hashtable_destroy(tap->pend_call_lst);
	tap->pend_call_lst = NULL;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_stop()
 *
 * Desc  : Stop configured sngisdn_tap span
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isdn_tap_stop(ftdm_span_t *span)
{
	sngisdn_tap_t *tap = span->signal_data;

	/* Free the allocated memory for q931 decoder control block */
	if (tap->decCb) {
		sng_q931_decoder_freecb(tap->decCb);
	}

	if (!ftdm_test_flag(tap, ISDN_TAP_RUNNING)) {
		return FTDM_FAIL;
	}

	ftdm_clear_flag(tap, ISDN_TAP_RUNNING);

	ftdm_mutex_destroy(&tap->pcalls_lock);

	/* release local isdn message control block */
	ftdm_safe_free(tap->pri);

	ftdm_set_flag(span, FTDM_SPAN_STOP_THREAD);

	while (ftdm_test_flag(span, FTDM_SPAN_IN_THREAD)) {
		ftdm_sleep(100);
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_media_read()
 *
 * Desc  : It mainly reads the isdn tap signalling messages depending up on the
 * 	   channel ID
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isdn_tap_media_read(ftdm_channel_t *ftdmchan, void *data, ftdm_size_t size)
{
	ftdm_status_t status = FTDM_SUCCESS;
	fio_codec_t codec_func;
	ftdm_channel_t *peerchan = ftdmchan->call_data;
	sngisdn_tap_t *tap = ftdmchan->span->signal_data;
	int16_t *chanbuf;
	int16_t *peerbuf;
	int16_t *mixedbuf;
	ftdm_size_t i = 0;
	ftdm_size_t sizeread = size;
	ftdm_wait_flag_t flags = FTDM_READ;

	if (!data || !size) {
		return FTDM_FAIL;
	}

	if (!FTDM_IS_VOICE_CHANNEL(ftdmchan) || !ftdmchan->call_data) {
		return  FTDM_SUCCESS;
	}

	if (tap->mixaudio == ISDN_TAP_MIX_SELF) {
		return FTDM_SUCCESS;
	}

	chanbuf = (int16_t*)malloc(sizeof(int16_t) *size);
	peerbuf = (int16_t*)malloc(sizeof(int16_t) *size);
	mixedbuf = (int16_t*)malloc(sizeof(int16_t) *size);

	if (tap->mixaudio == ISDN_TAP_MIX_PEER) {
		/* start out by clearing the self audio to make sure we don't return audio we were
		 * not supposed to in an error condition  */
		memset(data, FTDM_SILENCE_VALUE(ftdmchan), size);
	}

	if (ftdmchan->native_codec != peerchan->native_codec) {
		ftdm_log_chan(peerchan, FTDM_LOG_CRIT, "Invalid peer channel with format %d, ours = %d\n",
				peerchan->native_codec, ftdmchan->native_codec);
		status =  FTDM_FAIL;
		goto done;
	}

	memcpy(chanbuf, data, size);

	//ftdm_channel_lock(peerchan); //- observed some kind of locking issue , hence not using as of now

	/* check if there is any data to read or not.. */
	ftdm_channel_wait(peerchan, &flags, 0);
	if (flags & FTDM_READ) {
		status = peerchan->fio->read(peerchan, peerbuf, &sizeread);
	}
	//ftdm_channel_unlock(peerchan);

	if (status != FTDM_SUCCESS) {
		ftdm_log_chan_msg(peerchan, FTDM_LOG_ERROR, "Failed to read from peer channel!\n");
		status =  FTDM_FAIL;
		goto done;
	}
	if (sizeread != size) {
		ftdm_log_chan(peerchan, FTDM_LOG_ERROR, "read from peer channel only %"FTDM_SIZE_FMT" bytes!\n", sizeread);
		status =  FTDM_FAIL;
		goto done;
	}

	if (tap->mixaudio == ISDN_TAP_MIX_PEER) {
		/* only the peer audio is requested */
		memcpy(data, peerbuf, size);
		status = FTDM_SUCCESS;
		goto done;
	}

	codec_func = peerchan->native_codec == FTDM_CODEC_ULAW ? fio_ulaw2slin : peerchan->native_codec == FTDM_CODEC_ALAW ? fio_alaw2slin : NULL;
	if (codec_func) {
		sizeread = size;
		codec_func(chanbuf, sizeof(int16_t)*size, &sizeread);
		sizeread = size;
		codec_func(peerbuf, sizeof(int16_t)*size, &sizeread);
	}

	for (i = 0; i < size; i++) {
		mixedbuf[i] = ftdm_saturated_add(chanbuf[i], peerbuf[i]);
	}

	codec_func = peerchan->native_codec == FTDM_CODEC_ULAW ? fio_slin2ulaw : peerchan->native_codec == FTDM_CODEC_ALAW ? fio_slin2alaw : NULL;
	if (codec_func) {
		codec_func(mixedbuf, sizeof(int16_t)*size, &sizeread);
	}
	memcpy(data, mixedbuf, sizeread);

	status = FTDM_SUCCESS;

done:
	ftdm_safe_free(chanbuf);
	ftdm_safe_free(peerbuf);
	ftdm_safe_free(mixedbuf);
	return status;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_dchan_set_queue_size()
 *
 * Desc  : Set Rx/Tx Queue Size for Signalling channel
 *
 * Ret   : NULL
**********************************************************************************/
static void ftdm_sangoma_isdn_tap_dchan_set_queue_size(ftdm_channel_t *dchan)
{
	ftdm_status_t   ret_status;
	uint32_t queue_size = 0;

	if (!tap_dchan_queue_size) {
		/* setting TX/RX size to defualt */
		tap_dchan_queue_size = ISDN_TAP_DCHAN_QUEUE_LEN;
	}

	queue_size = tap_dchan_queue_size;
	ret_status = ftdm_channel_command(dchan, FTDM_COMMAND_SET_RX_QUEUE_SIZE, &queue_size);

	if (ret_status != FTDM_SUCCESS) {
		ftdm_log_chan_msg(dchan, FTDM_LOG_CRIT, "Failed to set Rx Queue size\n");
	} else {
		ftdm_log_chan(dchan, FTDM_LOG_INFO, "Rx Queue size is to %d\n", queue_size);
	}

	queue_size = tap_dchan_queue_size;
	ret_status = ftdm_channel_command(dchan, FTDM_COMMAND_SET_TX_QUEUE_SIZE, &queue_size);

	if (ret_status != FTDM_SUCCESS) {
		ftdm_log_chan_msg(dchan, FTDM_LOG_CRIT, "Failed to set Rx Queue size\n");
	} else {
		ftdm_log_chan(dchan, FTDM_LOG_INFO, "Rx Queue size is to %d\n", queue_size);
	}

	return;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_parse_trunkgroup()
 *
 * Desc  : Set NFAS trunkgroup information
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t sangoma_isdn_tap_parse_trunkgroup(const char *_trunkgroup)
{
	ftdm_status_t ret = FTDM_SUCCESS;
	uint8_t num_tap_spans = 0;
	char *backup_span = NULL;
	char *dchan_span  = NULL;
	char *trunkgroup  = NULL;
	char *name 	  = NULL;
	char *val 	  = NULL;
	int idx 	  = 0;

	trunkgroup =  ftdm_strdup(_trunkgroup);

	/* NFAS format: {nfas-name, num_chans, dchan_span-name, [backup_span-name]} */
	for (val = strtok(trunkgroup, ","); val; val = strtok(NULL, ",")) {
		while (*val == ' ') {
			val++;
		}

		switch(idx++) {
			case 0:
				name = ftdm_strdup(val);
				break;
			case 1:
				num_tap_spans = atoi(val);
				break;
			case 2:
				dchan_span = ftdm_strdup(val);
				break;
			case 3:
				backup_span = ftdm_strdup(val);
		}
	}

	if (!(name) || !(dchan_span) || (num_tap_spans <= 0)) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid tap parameters for trunkgroup: %s\n", _trunkgroup);
		ret = FTDM_FAIL;
		goto done;
	}

	for (idx = 0; idx < g_sngisdn_tap_data.num_nfas; idx++) {
		if (!ftdm_strlen_zero(g_sngisdn_tap_data.nfas[idx].name) &&
		    !strcasecmp(g_sngisdn_tap_data.nfas[idx].name, name)) {
			/* This trunkgroup is already configured */
			goto done;
		}
	}

	/* Configure Trunk group as does not found in the configured list */
	strncpy(g_sngisdn_tap_data.nfas[idx].name, name, sizeof(g_sngisdn_tap_data.nfas[idx].name));
	g_sngisdn_tap_data.nfas[idx].num_tap_spans = num_tap_spans;
	strncpy(g_sngisdn_tap_data.nfas[idx].dchan_span_name, dchan_span, sizeof(g_sngisdn_tap_data.nfas[idx].dchan_span_name));

	if (backup_span) {
		strncpy(g_sngisdn_tap_data.nfas[idx].backup_span_name, backup_span, sizeof(g_sngisdn_tap_data.nfas[idx].backup_span_name));
	}

	g_sngisdn_tap_data.num_nfas++;

done:
	ftdm_safe_free(trunkgroup);
	ftdm_safe_free(name);
	ftdm_safe_free(dchan_span);
	ftdm_safe_free(backup_span);
	return ret;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_parse_spanmap()
 *
 * Desc  : Set NFAS span map information i.e. to which group nfas span is mapped to
 *         what is the logical span Id of that particular span
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t sangoma_isdn_tap_parse_spanmap(const char *_spanmap, ftdm_span_t *span)
{
	sngisdn_tap_t *tap = (sngisdn_tap_t*) span->signal_data;
	ftdm_status_t ret = FTDM_SUCCESS;
	uint8_t logical_span_id = 0;
	char *spanmap = NULL;
	char *name = NULL;
	char *val = NULL;
	int idx = 0;

	spanmap = ftdm_strdup(_spanmap);

	/* Format - {nfas-group-name, logical-span-id} */
	for (val = strtok(spanmap, ","); val; val = strtok(NULL, ",")) {
		while (*val == ' ') {
			val++;
		}

		switch(idx++) {
			case 0:
				name = ftdm_strdup(val);
				break;
			case 1:
				logical_span_id = atoi(val);
				break;

			default:
				break;
		}
	}

	if (!name) {
		ftdm_log(FTDM_LOG_ERROR, "Invalid tap spanmap syntax %s\n", _spanmap);
		ret = FTDM_FAIL;
		goto done;
	}

	for (idx = 0; idx < g_sngisdn_tap_data.num_nfas; idx++) {
		if (!ftdm_strlen_zero(g_sngisdn_tap_data.nfas[idx].name) &&
		    !strcasecmp(g_sngisdn_tap_data.nfas[idx].name, name)) {
			tap->nfas.trunk = &g_sngisdn_tap_data.nfas[idx];
			break;
		}
	}

	if (!tap->nfas.trunk) {
		ftdm_log(FTDM_LOG_ERROR, "Could not find trunkgroup with name %s\n", name);
		ret = FTDM_FAIL;
		goto done;
	}

	/* Fill in self span first */
	if (tap->nfas.trunk->tap_spans[logical_span_id]) {
		ftdm_log(FTDM_LOG_ERROR, "trunkgroup:%s already had a span with logical span id:%d for span %s\n", name, logical_span_id, tap->span->name);
	} else {
		tap->nfas.trunk->tap_spans[logical_span_id] = span;
		tap->nfas.interface_id = logical_span_id;
	}

	tap->nfas.sigchan = SNGISDN_NFAS_DCHAN_NONE;

	if (!strcasecmp(tap->span->name, tap->nfas.trunk->dchan_span_name)) {
		tap->nfas.sigchan = SNGISDN_NFAS_DCHAN_PRIMARY;
		tap->nfas.trunk->dchan = tap;
	}

	if (!strcasecmp(tap->span->name, tap->nfas.trunk->backup_span_name)) {
		tap->nfas.sigchan = SNGISDN_NFAS_DCHAN_BACKUP;
		tap->nfas.trunk->backup = tap;
	}

	tap->nfas.trunk->num_spans_configured++;
done:
	ftdm_safe_free(spanmap);
	ftdm_safe_free(name);
	return ret;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_start()
 *
 * Desc  : Start configured sngisdn_tap span
 *
 * Ret   : FTDM_FAIL | ret
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isdn_tap_start(ftdm_span_t *span)
{
	ftdm_status_t ret;
	sngisdn_tap_t *tap = span->signal_data;
	sngisdn_tap_t *peer_tap = tap->peerspan->signal_data;

	if ((tap->tap_mode == ISDN_TAP_BI_DIRECTION) && (tap->dchan)) {
		ftdm_channel_set_feature(tap->dchan, FTDM_CHANNEL_FEATURE_IO_STATS);
		ftdm_sangoma_isdn_tap_dchan_set_queue_size(tap->dchan);

	}

	if (ftdm_test_flag(tap, ISDN_TAP_RUNNING)) {
		return FTDM_FAIL;
	}

	ftdm_mutex_create(&tap->pcalls_lock);

	ftdm_clear_flag(span, FTDM_SPAN_STOP_THREAD);
	ftdm_clear_flag(span, FTDM_SPAN_IN_THREAD);

	ftdm_set_flag(tap, ISDN_TAP_RUNNING);
	if (peer_tap && ftdm_test_flag(peer_tap, ISDN_TAP_RUNNING)) {
		/* our peer already started, we're the master */
		ftdm_set_flag(tap, ISDN_TAP_MASTER);
	}

	if ((!tap->nfas.trunk) || (tap->nfas.sigchan != SNGISDN_NFAS_DCHAN_NONE)) {
		ret = ftdm_thread_create_detached(ftdm_sangoma_isdn_tap_run, span);
	} else {
		ret = ftdm_thread_create_detached(ftdm_sangoma_isdn_tap_voice_span_run, span);
	}

	if (ret != FTDM_SUCCESS) {
		return ret;
	}

	return ret;
}

/**************************************************************************************
 * Fun   : FIO_CONFIGURE_SPAN_SIGNALING_FUNCTION(ftdm_sangoma_isdn_tap_configure_span)
 *
 * Desc  : Configure the isdn tap span w.r.t the configuration described as in
 * 	   freetdm.conf.xml by the user
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**************************************************************************************/
static FIO_CONFIGURE_SPAN_SIGNALING_FUNCTION(ftdm_sangoma_isdn_tap_configure_span)
{
	sngisdn_tap_mix_mode_t mixaudio = ISDN_TAP_MIX_NULL;
	sngisdn_tap_iface_t iface = ISDN_TAP_IFACE_UNKNOWN;
	sngisd_tap_mode_t tapmode = ISDN_TAP_NORMAL;	/* By default tapping mode will be NORMAL i.e. single way/E1-T1 tapping */
	ftdm_bool_t event_ind = FTDM_FALSE;		/* By default set event indication to false */
	ftdm_channel_t *dchan = NULL;
	ftdm_span_t *peerspan = NULL;
	sngisdn_tap_t *tap = NULL;
	const char *debug = NULL;
	int swtch_typ = SW_ETSI; 			/* By default switch type will always be etsi */
	unsigned paramindex = 0;
	const char *var, *val;
	int intf = NETWORK; 				/* default */
	uint32_t idx = 0;

	tap = ftdm_calloc(1, sizeof(*tap));
	if (!tap) {
		return FTDM_FAIL;
	}

	span->signal_data = tap;

	tap->nfas.trunk = NULL;
	tap->span = span;

	if (span->trunk_type >= FTDM_TRUNK_NONE) {
		ftdm_log(FTDM_LOG_WARNING, "Invalid trunk type '%s' defaulting to T1.\n", ftdm_trunk_type2str(span->trunk_type));
		span->trunk_type = FTDM_TRUNK_T1;
	}

	/* Find out if NFAS is enabled first */
	for (paramindex = 0; ftdm_parameters[paramindex].var; paramindex++) {
		ftdm_log(FTDM_LOG_DEBUG, "Sangoma ISDN Tapping key = value, %s = %s\n", ftdm_parameters[paramindex].var, ftdm_parameters[paramindex].val);
		var = ftdm_parameters[paramindex].var;
		val = ftdm_parameters[paramindex].val;

		if (!strcasecmp(var, "trunkgroup")) {
			if (sangoma_isdn_tap_parse_trunkgroup(val) != FTDM_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "Invalid tapping trunkgoup %s info for span %s!\n", val, span->name);
				return FTDM_FAIL;
			}
		}
	}

	for (paramindex = 0; ftdm_parameters[paramindex].var; paramindex++) {
		var = ftdm_parameters[paramindex].var;
		val = ftdm_parameters[paramindex].val;
		ftdm_log(FTDM_LOG_DEBUG, "Sangoma ISDN Tapping key=value, %s=%s\n", var, val);

		if (!strcasecmp(var, "debug")) {
			debug = val;
		} else if (!strcasecmp(var, "mixaudio")) {
			if (ftdm_true(val) || !strcasecmp(val, "both")) {
				ftdm_log(FTDM_LOG_DEBUG, "Setting mix audio mode to 'both' for span %s\n", span->name);
				mixaudio = ISDN_TAP_MIX_BOTH;
			} else if (!strcasecmp(val, "peer")) {
				ftdm_log(FTDM_LOG_DEBUG, "Setting mix audio mode to 'peer' for span %s\n", span->name);
				mixaudio = ISDN_TAP_MIX_PEER;
			} else {
				ftdm_log(FTDM_LOG_DEBUG, "Setting mix audio mode to 'self' for span %s\n", span->name);
				mixaudio = ISDN_TAP_MIX_SELF;
			}
		} else if (!strcasecmp(var, "interface")) {
			if (!strcasecmp(val, "cpe")) {
				iface = ISDN_TAP_IFACE_CPE;
			} else if (!strcasecmp(val, "net")) {
				iface = ISDN_TAP_IFACE_NET;
			} else {
				ftdm_log(FTDM_LOG_WARNING, "Ignoring invalid tapping interface type %s\n", val);
			}
		} else if (!strcasecmp(var, "peerspan")) {
			if (ftdm_span_find_by_name(val, &peerspan) != FTDM_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "Invalid tapping peer span %s\n", val);
				break;
			}
		} else if (!strcasecmp(var, "tapmode")) {
			if (!strcasecmp(val, "bi-dir")) {
				tapmode = ISDN_TAP_BI_DIRECTION;
			} else {
				tapmode = ISDN_TAP_NORMAL;
			}
			ftdm_log(FTDM_LOG_DEBUG, "ISDN Tapping is running in %s mode\n", SNG_DECODE_ISDN_TAP_MODE(tapmode));
		} else if (!strcasecmp(var, "interface_type")) {
			if (!strcasecmp(val, "network")) {
				intf = NETWORK;
			} else if (!strcasecmp(val, "user")) {
				intf = USER;
			} else {
				ftdm_log(FTDM_LOG_WARNING, "Ignoring invalid tapping interface type %s\n", val);
			}
		} else if (!strcasecmp(var, "switch_type")) {
				swtch_typ = ftdm_sangoma_isdn_get_switch_type(val);
				if (swtch_typ == -1) {
					ftdm_log(FTDM_LOG_ERROR, "Invalid switch type '%s' for span '%s'\n", val, span->name);
					return FTDM_FAIL;
				}
		} else if (!strcasecmp(var, "queue-size")) {
			tap_dchan_queue_size = atoi(val);
			if (tap_dchan_queue_size) {
					ftdm_log(FTDM_LOG_ERROR, "Invalid TX/RX Queue size '%s' for span '%s'\n", val, span->name);
			} else {
				tap_dchan_queue_size = ISDN_TAP_DCHAN_QUEUE_LEN;
			}
			ftdm_log(FTDM_LOG_INFO, "TX/RX Queue size '%d' is received by user configuration for span '%s'\n", tap_dchan_queue_size, span->name);
		} else if (!strcasecmp(var, "spanmap")) {
			if (sangoma_isdn_tap_parse_spanmap(val, span) != FTDM_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "Invalid tap NFAS configuration span_map '%s' for span '%s'\n", val, span->name);
				return FTDM_FAIL;
			}
		} else if (!strcasecmp(var, "trunkgroup")) {
				/*  Nothing to do as this is already being done */
		} else if (!strcasecmp(var, "call_info_indication")) {
			if (ftdm_true(val)) {
				event_ind = FTDM_TRUE;
				ftdm_log(FTDM_LOG_DEBUG, "Setting call infomation indication to TRUE for span '%s'\n", span->name);
			} else {
				event_ind = FTDM_FALSE;
				ftdm_log(FTDM_LOG_DEBUG, "Setting call infomation indication to FALSE for span '%s'\n", span->name);
			}
		} else {
			ftdm_log(FTDM_LOG_ERROR,  "Unknown isdn tapping parameter [%s]\n", var);
		}
	}

	/* get d channel as this is not a pure voice span */
	if ((!tap->nfas.trunk) || (tap->nfas.sigchan != SNGISDN_NFAS_DCHAN_NONE)) {
		for (idx = 1; idx <= span->chan_count; idx++) {
			if (span->channels[idx]->type == FTDM_CHAN_TYPE_DQ921) {
				dchan = span->channels[idx];
				break;
			}
		}

		if (!dchan) {
			ftdm_log(FTDM_LOG_ERROR, "No d-channel specified in freetdm.conf!\n");
			return FTDM_FAIL;
		}

		tap->dchan = dchan;
	}


	if (!peerspan) {
		ftdm_log(FTDM_LOG_ERROR, "No valid peerspan was specified!\n");
		return FTDM_FAIL;
	}

	/* initializing tap call hash lists */
	tap->pend_call_lst = create_hashtable(FTDM_MAX_CHANNELS_PHYSICAL_SPAN, ftdm_hash_hashfromstring, ftdm_hash_equalkeys);

	tap->indicate_event = event_ind;
	tap->debug = parse_debug(debug);
	tap->peerspan = peerspan;
	tap->mixaudio = mixaudio;
	tap->iface = iface;
	tap->switch_type = swtch_typ;
	tap->tap_mode = tapmode;

	/* Get the q931 decoder control block */
	tap->decCb = sng_q931_decoder_getcb();
	if (NULL == tap->decCb) {
		ftdm_log(FTDM_LOG_ERROR, " Failed to get memory for q931 decoder control block \n");
		return FTDM_FAIL;
	}

	/* Initialize Q931 Message Decoder as decoder control block has to be per tap basis */
	if (q931_decoder_init(intf, tap->decCb)) {
		/* Free the allocated memory for q931 decoder control block */
		if (tap->decCb) {
			sng_q931_decoder_freecb(tap->decCb);
		}

		ftdm_log(FTDM_LOG_ERROR, " Failed to initialize q931 decoder control block \n");
		return FTDM_FAIL;
	}


	span->start = ftdm_sangoma_isdn_tap_start;
	span->stop = ftdm_sangoma_isdn_tap_stop;

	if (tap->mixaudio == ISDN_TAP_MIX_NULL) {
		span->sig_read = NULL;
	} else {
		span->sig_read = ftdm_sangoma_isdn_tap_media_read;
	}

	span->signal_cb = sig_cb;

	span->signal_type = FTDM_SIGTYPE_ISDN;
	span->outgoing_call = sangoma_isdn_tap_outgoing_call;

	span->get_channel_sig_status = sangoma_isdn_tap_get_channel_sig_status;
	span->get_span_sig_status = sangoma_isdn_tap_get_span_sig_status;

	span->state_map = &sngisdn_tap_state_map;
	span->state_processor = state_advance;

	if (tap->tap_mode == ISDN_TAP_BI_DIRECTION) {
		ftdm_log(FTDM_LOG_INFO, "Setting SKIP state flag for span %d\n", span->span_id);
		/* setting skip state flag in order to skip states like PROGRESS/PROGREE_MEDIA
		 * and directly move to UP state */
		ftdm_set_flag(span, FTDM_SPAN_USE_SKIP_STATES);
	}

	/* Initialize Decoder lib */
	if (!g_decoder_lib_init) {

		sng_event.sng_log = handle_isdn_tapping_log;

		/* Initialize Decoder lib */
		if (sng_decoder_lib_init(&sng_event)) {
			ftdm_log(FTDM_LOG_ERROR, " Failed to initialize decoder lib \n");
			return FTDM_FAIL;
		}

		g_decoder_lib_init = 0x01;
	}

	/* Enabling Channel IO Stats on span basis */
	ftdm_sangoma_isdn_tap_io_stats(NULL, span, "enable");

	/* main tap list in global structure */
	g_sngisdn_tap_data.tap_spans[g_sngisdn_tap_data.num_spans_configured++] = tap;

	ftdm_log(FTDM_LOG_DEBUG, "Configured tap for span [%d]!\n", span->span_id);
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_new_cb()
 *
 * Desc  : Initialize the isdn tap(sngisdn_t) new control block
 *
 * Ret   : p(i.e. pri strtcture)
**********************************************************************************/
struct sngisdn_t *sangoma_isdn_new_cb(ftdm_socket_t fd, int nodetype, int switchtype, void *userdata)
{
	struct sngisdn_t *p;

	if (!(p = ftdm_calloc(1, sizeof(*p))))
		return NULL;

	p->fd = fd;
	p->userdata = userdata;
	p->localtype = nodetype;
	p->switchtype = switchtype;
	p->cref = 1;

	return p;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_get_userdata()
 *
 * Desc  : Get user data w.r.t the pased pri control block
 *
 * Ret   : pri | NULL
**********************************************************************************/
void *sangoma_isdn_get_userdata(struct sngisdn_t *pri)
{
	return pri ? pri->userdata : NULL;
}

/**********************************************************************************
 * Fun   : handle_isdn_tapping_log()
 *
 * Desc  : prints the LOG depending up on the LOG LEVEL
 *
 * Ret   : void
**********************************************************************************/
void handle_isdn_tapping_log(uint8_t level, char *fmt,...)
{
	char	*data;
	int	ret;
	va_list	ap;

	va_start(ap, fmt);
	ret = ftdm_vasprintf(&data, fmt, ap);
	if (ret == -1) {
		goto end;
	}

	switch (level) {
	/**************************************************************************/
	case SNG_LOGLEVEL_DEBUG:
		ftdm_log(FTDM_LOG_DEBUG, "sng_decoder_lib->%s", data);
		break;
	/**************************************************************************/
	case SNG_LOGLEVEL_WARN:
		ftdm_log(FTDM_LOG_WARNING, "sng_decoder_lib->%s", data);
		break;
	/**************************************************************************/
	case SNG_LOGLEVEL_INFO:
		ftdm_log(FTDM_LOG_INFO, "sng_decoder_lib->%s", data);
		break;
	/**************************************************************************/
	case SNG_LOGLEVEL_ERROR:
		ftdm_log(FTDM_LOG_ERROR, "sng_decoder_lib->%s", data);
		break;
	/**************************************************************************/
	case SNG_LOGLEVEL_CRIT:
		/*printf("%s",data);*/
		ftdm_log(FTDM_LOG_CRIT, "sng_decoder_lib->%s", data);
		break;
	/**************************************************************************/
	default:
		ftdm_log(FTDM_LOG_INFO, "sng_decoder_lib->%s", data);
		break;
	/**************************************************************************/
	}

end:
	ftdm_safe_free(data);
	va_end(ap);
	return;
}

/**********************************************************************************
 * Fun   : pri_set_debug()
 *
 * Desc  : Set isdn_tap debug level
 *
 * Ret   : void
**********************************************************************************/
void sangoma_isdn_tap_set_debug(struct sngisdn_t *pri, int debug)
{
	if (!pri)
		return;
	pri->debug = debug;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_tap_set_tap_msg_info()
 *
 * Desc  : set the call information in to tap msg_info structure that needs to be
 * 	   passed to application when disconnect/release is received
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
ftdm_status_t sangoma_isdn_tap_set_tap_msg_info(sngisdn_tap_t *tap, uint8_t msgType, passive_call_t *pcall, sng_decoder_interface_t *intfCb)
{
	if (!tap) {
		ftdm_log(FTDM_LOG_ERROR, "sangoma_isdn_tap_set_tap_msg_info failed. Invalid tap structure\n");
		return FTDM_FAIL;
	}

	if (!intfCb) {
		ftdm_log(FTDM_LOG_ERROR, "sangoma_isdn_tap_set_tap_msg_info failed. Invalid intreface structure\n");
		return FTDM_FAIL;
	}

	/* Copy span_id to tap message info structure */
	pcall->tap_msg.span_id = tap->span->span_id;
	ftdm_log(FTDM_LOG_DEBUG, "Setting channel ID[%d] in tap message structure\n", pcall->tap_msg.span_id);

	/* Copy Call reference value to tap message structure */
	pcall->tap_msg.call_ref = intfCb->call_ref;
	ftdm_log(FTDM_LOG_DEBUG, "Setting Call referece[%d] in tap message structure\n", pcall->tap_msg.call_ref);

	/* Set flag as no call connect event is received till now */
	pcall->tap_msg.call_con = 0;

	if (msgType == Q931_SETUP) {
		/* Copy chan_id to tap message info structure */
		pcall->tap_msg.chan_id = intfCb->chan_id;
		ftdm_log(FTDM_LOG_DEBUG, "Setting channel ID[%d] in tap message structure\n", pcall->tap_msg.chan_id);

		/* Copy message Direction i.e. whether its an incoming call or outgoing call */
		pcall->tap_msg.call_dir = CALL_DIRECTION_INBOUND;

		/* If called number is present then only fill the same */
		if (!ftdm_strlen_zero(intfCb->cld_num)) {
			ftdm_set_string(pcall->tap_msg.called_num, intfCb->cld_num);
			ftdm_log(FTDM_LOG_DEBUG, "Setting Called Number[%s] in tap message structure\n", pcall->tap_msg.called_num);
		}

		/* If calling number is present then only fill the same */
		if (!ftdm_strlen_zero(intfCb->clg_num)) {
			ftdm_set_string(pcall->tap_msg.calling_num, intfCb->clg_num);
			ftdm_log(FTDM_LOG_DEBUG, "Setting Calling Number[%s] in tap message structure\n", pcall->tap_msg.calling_num);
		}
	} else if (msgType == Q931_CONNECT) {
		/* Copy message Direction i.e. whether its an incoming call or outgoing call */
		pcall->tap_msg.call_dir = CALL_DIRECTION_OUTBOUND;
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_get_switch_type()
 *
 * Desc  : Get the respective switch type with respect to span
 *
 * Ret   : switch_type | By default switch type remains ETSI
**********************************************************************************/
int ftdm_sangoma_isdn_get_switch_type(const char *val)
{
	int switch_type;

	if (!strcasecmp(val, "test")) {
		switch_type = SW_TST;
	} else if (!strcasecmp(val, "ccitt")) {
		switch_type = SW_CCITT;
	} else if (!strcasecmp(val, "att5eb")) {
		switch_type = SW_ATT5EB;
	} else if (!strcasecmp(val, "att5ep")) {
		switch_type = SW_ATT5EP;
	} else if (!strcasecmp(val, "att4e")) {
		switch_type = SW_ATT4E;
	} else if (!strcasecmp(val, "ntdms100b")) {
		switch_type = SW_NTDMS100B;
	} else if (!strcasecmp(val, "ntdms100p")) {
		switch_type = SW_NTDMS100P;
	} else if (!strcasecmp(val, "vn2")) {
		switch_type = SW_VN2;
	} else if (!strcasecmp(val, "vn3")) {
		switch_type = SW_VN3;
	} else if (!strcasecmp(val, "insnet")) {
		switch_type = SW_INSNET;
	} else if (!strcasecmp(val, "tr6mpc")) {
		switch_type = SW_TR6MPC;
	} else if (!strcasecmp(val, "tr6pbx")) {
		switch_type = SW_TR6PBX;
	} else if (!strcasecmp(val, "ausb")) {
		switch_type = SW_AUSB;
	} else if (!strcasecmp(val, "ausp")) {
		switch_type = SW_AUSP;
	} else if (!strcasecmp(val, "ni1")) {
		switch_type = SW_NI1;
	} else if (!strcasecmp(val, "etsi")) {
		switch_type = SW_ETSI;
	} else if (!strcasecmp(val, "bc303tmc")) {
		switch_type = SW_BC303TMC;
	} else if (!strcasecmp(val, "bc303csc")) {
		switch_type = SW_BC303CSC;
	} else if (!strcasecmp(val, "ntdms250")) {
		switch_type = SW_NTDMS250;
	} else if (!strcasecmp(val, "ni2")) {
		switch_type = SW_NI2;
	} else if (!strcasecmp(val, "qsig")) {
		switch_type = SW_QSIG;
	} else if (!strcasecmp(val, "ntmci")) {
		switch_type = SW_NTMCI;
	} else if (!strcasecmp(val, "ntni")) {
		switch_type = SW_NTNI;
	} else if (!strcasecmp(val, "bellcore")) {
		switch_type = SW_BELLCORE;
	} else if (!strcasecmp(val, "gsm")) {
		switch_type = SW_GSM;
	} else {
		switch_type = -1;
	}

	return switch_type;
}

/**********************************************************************************
 * Macro : ftdm_module
 *
 * Desc  : FreeTDM sangoma_isdn_tap signaling and IO module definition
 *
 * Ret   : void
 **********************************************************************************/
EX_DECLARE_DATA ftdm_module_t ftdm_module = {
	"sangoma_isdn_tap",
	ftdm_sangoma_isdn_tap_io_init,
	ftdm_sangoma_isdn_tap_unload,
	ftdm_sangoma_isdn_tap_init,
	NULL,
	NULL,
	ftdm_sangoma_isdn_tap_configure_span,
};

/********************************************************************************/
/*                                                                              */
/*                             COMMAND HANDLERS                                 */
/*                                                                              */
/********************************************************************************/

/**********************************************************************************
 * Fun   : COMMAND_HANDLER(statistics)
 *
 * Desc  : Prints/Give the statics w.r.t. no. of calls received/tapped/released etc
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
COMMAND_HANDLER(statistics)
{
	if (argc) {
		stream->write_function(stream, "-ERR Invalid Tap statistics command\n",  argv[1]);
		return FTDM_FAIL;
	}

	stream->write_function(stream, "**********************************************************\n");
	stream->write_function(stream, "                     Calls Statistics 			  \n");
	stream->write_function(stream, "**********************************************************\n");
	stream->write_function(stream, "    Total Number of Setup Received						   = %d\n", setup_recv);
	stream->write_function(stream, "    Total Number of Setup Received with invalid channel				   = %d\n", setup_recv_invalid_chanId);
	stream->write_function(stream, "    Total Number of Setup not Received						   = %d\n", setup_not_recv);
	stream->write_function(stream, "    Total Number of Call Proceed Received					   = %d\n", call_proceed_recv);
	stream->write_function(stream, "    Total Number of Connnect Received						   = %d\n", connect_recv);
	stream->write_function(stream, "    Total Number of Early Connnect Received					   = %d\n", early_connect_recv);
	stream->write_function(stream, "    Total Number of Late Invite Received					   = %d\n", late_invite_recv);
	stream->write_function(stream, "    Total Number of Connnect Received with channel				   = %d\n", connect_recv_with_chanId);
	stream->write_function(stream, "    Total Number of Message received after %d seconds				   = %d\n", SNG_TAP_SUBEQUENT_MSG_TME_DURATION, msg_recv_aft_time);
	stream->write_function(stream, "    Total Number of Disconnect Received						   = %d\n", disconnect_recv);
	stream->write_function(stream, "    Total Number of Release Received						   = %d\n", release_recv);
	stream->write_function(stream, "    Total Number of Tapped files generated					   = %d\n", call_tapped);
	stream->write_function(stream, "    Total Number of Connect Received where Channel was In-Use			   = %d\n", reuse_chan_con_recv);

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_show_span_status()
 *
 * Desc  : Prints/Give span statistics
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isdn_tap_show_span_status(ftdm_stream_handle_t *stream, ftdm_span_t *span)
{
	ftdm_alarm_flag_t alarmbits = FTDM_ALARM_NONE;
	ftdm_signaling_status_t sigstatus;
	ftdm_channel_t *sig_chan = NULL;
	ftdm_channel_t *fchan = NULL;
	uint32_t chan_idx = 0;

	sig_chan = ((sngisdn_tap_t *)span->signal_data)->dchan;

	if (sig_chan) {
		stream->write_function(stream, "SIGNALLING SPAN %s status ->\n", span->name);
	} else {
		stream->write_function(stream, "PURE VOICE SPAN %s status ->\n", span->name);
	}

	for (chan_idx = 1; chan_idx <= span->chan_count; chan_idx++) {
		fchan = span->channels[chan_idx];
		alarmbits = FTDM_ALARM_NONE;

		if (fchan) {
			ftdm_channel_get_alarms(fchan, &alarmbits);

			ftdm_channel_get_sig_status(fchan, &sigstatus);

			if ((sig_chan) && (sig_chan->span_id == fchan->span_id) && (sig_chan->chan_id == fchan->chan_id)) {
				stream->write_function(stream, "span = %2d|chan = %2d|phy_status = %4s|SIGNALLING LINK|state = %s|\n",
						       span->span_id, fchan->physical_chan_id, alarmbits ? "ALARMED" : "OK",
						       ftdm_channel_state2str(fchan->state));
			} else {
				stream->write_function(stream, "span = %2d|chan = %2d|phy_status = %4s|sig_status = %4s|state = %s|\n",
						       span->span_id, fchan->physical_chan_id, alarmbits ? "ALARMED" : "OK",
						       ftdm_signaling_status2str(sigstatus),
						       ftdm_channel_state2str(fchan->state));
			}
		}
	}

	stream->write_function(stream, "***********************************************************************\n");

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_show_nfas_config()
 *
 * Desc  : Print nfas configuration if present
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isdn_tap_show_nfas_config(ftdm_stream_handle_t *stream, sngisdn_tap_t *tap)
{
	sngisdn_tap_t *nfas_tap = NULL;
	ftdm_span_t *span = NULL;
	int idx = 0;

	stream->write_function(stream, "***********************************************************************\n");
	if (!tap->nfas.trunk) {
		stream->write_function(stream, "Span %s is not configured as NFAS span\n", tap->span->name);
		return FTDM_FAIL;
	}

	stream->write_function(stream, "NFAS Group: %s\n", tap->nfas.trunk->name);

	for (idx = 0; idx < tap->nfas.trunk->num_tap_spans; idx++) {
		if (!tap->nfas.trunk->tap_spans[idx]) {
			ftdm_log(FTDM_LOG_ERROR, "No ISDN NFAS Tap span found at index %d for span %s\n",
					 idx, tap->span->name);
			return FTDM_FAIL;
		}

		span = tap->nfas.trunk->tap_spans[idx];
		nfas_tap = span->signal_data;

		stream->write_function(stream, "Span = %s| logical span ID = %d| Span Type = %s|\n",
				nfas_tap->span->name, idx, SNG_ISDN_TAP_GET_NFAS_SPAN_TYPE(nfas_tap->nfas.sigchan));
	}

	return FTDM_SUCCESS;

}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isdn_tap_io_stats()
 *
 * Desc  : Prints/Set io stats for all channels of a span
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
ftdm_status_t ftdm_sangoma_isdn_tap_io_stats(ftdm_stream_handle_t *stream, ftdm_span_t *span, const char* cmd)
{
	ftdm_iterator_t *chaniter = NULL;
	ftdm_channel_t *ftdmchan = NULL;
	ftdm_iterator_t *curr = NULL;
	ftdm_channel_iostats_t stats;
	int enable = 0;

	if ((NULL == cmd) || (NULL == span)) {
		stream->write_function(stream, "Error Enabling IO Stats..invalid span \n");
		return FTDM_FAIL;
	}

	if (stream) {
		stream->write_function(stream, "***********************************************************************\n");
	}

	chaniter = ftdm_span_get_chan_iterator(span, NULL);
	for (curr = chaniter; curr; curr = ftdm_iterator_next(curr)) {
		ftdmchan = (ftdm_channel_t*)ftdm_iterator_current(curr);

		if (!ftdmchan || !cmd) {
			stream->write_function(stream, "-- Invalid channel/command '%s' found!", cmd);
			return FTDM_FAIL;
		}

		if (!strcasecmp("enable", cmd)) {
			enable = 1;
			ftdm_channel_command(ftdmchan, FTDM_COMMAND_SWITCH_IOSTATS, &enable);
			if (stream) {
				stream->write_function(stream, "IO STATS Enable successfully for span %d chan %d\n", ftdmchan->span_id, ftdmchan->chan_id);
			}
		} else if (!strcasecmp("disable", cmd)) {
			enable = 0;
			ftdm_channel_command(ftdmchan, FTDM_COMMAND_SWITCH_IOSTATS, &enable);
			stream->write_function(stream, "IO STATS Disable successfully for span %d chan %d\n", ftdmchan->span_id, ftdmchan->chan_id);
		} else if (!strcasecmp("flush", cmd)) {
			ftdm_channel_command(ftdmchan, FTDM_COMMAND_FLUSH_IOSTATS, NULL);
			stream->write_function(stream, "IO STATS Flushed successfully for span %d chan %d\n", ftdmchan->span_id, ftdmchan->chan_id);
		} else if (!strcasecmp("show", cmd)) {
			ftdm_channel_command(ftdmchan, FTDM_COMMAND_GET_IOSTATS, &stats);

			stream->write_function(stream, "-- IO statistics for channel %d:%d --\n", ftdm_channel_get_span_id(ftdmchan), ftdm_channel_get_id(ftdmchan));
			stream->write_function(stream, "Rx errors: %u\n", stats.rx.errors);
			stream->write_function(stream, "Rx queue size: %u\n", stats.rx.queue_size);
			stream->write_function(stream, "Rx queue len: %u\n", stats.rx.queue_len);
			stream->write_function(stream, "Rx count: %lu\n", stats.rx.packets);
			stream->write_function(stream, "Tx errors: %u\n", stats.tx.errors);
			stream->write_function(stream, "Tx queue size: %u\n", stats.tx.queue_size);
			stream->write_function(stream, "Tx queue len: %u\n", stats.tx.queue_len);
			stream->write_function(stream, "Tx count: %lu\n", stats.tx.packets);
			stream->write_function(stream, "Tx idle: %lu\n", stats.tx.idle_packets);
		}
	}
	ftdm_iterator_free(chaniter);

	if (stream) {
		stream->write_function(stream, "***********************************************************************\n");
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : COMMAND_HANDLER(channel statistics)
 *
 * Desc  : Prints/Give the statics w.r.t. no. of calls received/tapped/released etc
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
COMMAND_HANDLER(show)
{
	sngisdn_tap_t *tap = NULL;
	ftdm_span_t *span = NULL;
	int span_id = 0;

	if (argc != 2) {
		stream->write_function(stream, "-ERR Invalid Tap show command\n",  argv[1]);
		return FTDM_FAIL;
	}

	if (!strcasecmp(argv[0], "span")) {
		if (!strcasecmp(argv[1], "all")) {
			for (span_id = 0; span_id <= g_sngisdn_tap_data.num_spans_configured; span_id++) {
				tap = g_sngisdn_tap_data.tap_spans[span_id];

				if (tap) {
					ftdm_sangoma_isdn_tap_show_span_status(stream, tap->span);
				}
			}
		} else {
			span_id = atoi(argv[1]);

			if (ftdm_span_find_by_name(argv[0], &span) != FTDM_SUCCESS && ftdm_span_find(span_id, &span) != FTDM_SUCCESS) {
				stream->write_function(stream, "-ERR Failed to find TAP span '%s'\n",  argv[1]);
				return FTDM_FAIL;
			}

			if (!span || !span->signal_data) {
				stream->write_function(stream, "-ERR '%s' is not a valid TAP span\n",  argv[1]);
				return FTDM_FAIL;
			}

			ftdm_sangoma_isdn_tap_show_span_status(stream, span);
		}
	} else if (!strcasecmp(argv[0], "nfas")) {
		if (!strcasecmp(argv[1], "config")) {
			for (span_id = 0; span_id <= g_sngisdn_tap_data.num_spans_configured; span_id++) {
				tap = g_sngisdn_tap_data.tap_spans[span_id];

				if ((tap) && (tap->dchan)) {
					ftdm_sangoma_isdn_tap_show_nfas_config(stream, tap);
				}
			}
		} else {
				stream->write_function(stream, "-ERR '%s' is not a valid TAP span\n",  argv[1]);
				stream->write_function(stream, "-Use: ftdm isdn_tap show nfas config\n",  argv[1]);
				return FTDM_FAIL;
		}
	} else {
		stream->write_function(stream, "-ERR Invalid Tap command\n",  argv[1]);
		return FTDM_FAIL;
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : COMMAND_HANDLER(enable io statistics)
 *
 * Desc  : Prints/Give all channel io statistics
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
COMMAND_HANDLER(io_stats)
{
	sngisdn_tap_t *tap = NULL;
	ftdm_span_t *span = NULL;
	int span_id = 0;

	if (argc != 3) {
		stream->write_function(stream, "-ERR Invalid Tap io_stats command\n",  argv[1]);
		return FTDM_FAIL;
	}

	if (!strcasecmp(argv[0], "show")) {
		if (!strcasecmp(argv[1], "span")) {
			if (!strcasecmp(argv[2], "all")) {
				for (span_id = 0; span_id <= g_sngisdn_tap_data.num_spans_configured; span_id++) {
					tap = g_sngisdn_tap_data.tap_spans[span_id];

					if (tap) {
						ftdm_sangoma_isdn_tap_io_stats(stream, tap->span, "show");
					}
				}
			} else {
				span_id = atoi(argv[2]);

				if (ftdm_span_find_by_name(argv[2], &span) != FTDM_SUCCESS && ftdm_span_find(span_id, &span) != FTDM_SUCCESS) {
					stream->write_function(stream, "-ERR Failed to find TAP span '%s'\n",  argv[1]);
					return FTDM_FAIL;
				}

				if (!span || !span->signal_data) {
					stream->write_function(stream, "-ERR '%s' is not a valid TAP span\n",  argv[1]);
					return FTDM_FAIL;
				}

				ftdm_sangoma_isdn_tap_io_stats(stream, span, "show");
			}
		} else {
			stream->write_function(stream, "-ERR Invalid Tap io_stats show command '%s'\n",  argv[1]);
			stream->write_function(stream, "USAGE -> ftdm sangoma_isdn_tap io_stats show span <all/span_id>\n");
			return FTDM_FAIL;
		}
	} else if (!(strcasecmp(argv[0], "enable")) || !(strcasecmp(argv[0], "disable")) || !(strcasecmp(argv[0], "flush"))) {
		if (!strcasecmp(argv[1], "span")) {
			if (!strcasecmp(argv[2], "all")) {
				for (span_id = 0; span_id <= g_sngisdn_tap_data.num_spans_configured; span_id++) {
					tap = g_sngisdn_tap_data.tap_spans[span_id];

					if (tap) {
						if (!strcasecmp(argv[0], "enable")) {
							ftdm_sangoma_isdn_tap_io_stats(stream, tap->span, "enable");
						} else if (!strcasecmp(argv[0], "disable")) {
							ftdm_sangoma_isdn_tap_io_stats(stream, tap->span, "disable");
						} else if (!strcasecmp(argv[0], "flush")) {
							ftdm_sangoma_isdn_tap_io_stats(stream, tap->span, "flush");
						} else {
							stream->write_function(stream, "-ERR Invalid Tap io_stats enable/disable/flush command\n",  argv[1]);
							stream->write_function(stream, "USAGE -> ftdm sangoma_isdn_tap io_stats <enable/disable/flush> span <all/span_id>\n");
							return FTDM_FAIL;
						}
					}
				}
			} else {
				span_id = atoi(argv[2]);

				if (ftdm_span_find_by_name(argv[2], &span) != FTDM_SUCCESS && ftdm_span_find(span_id, &span) != FTDM_SUCCESS) {
					stream->write_function(stream, "-ERR Failed to find TAP span '%s'\n",  argv[1]);
					return FTDM_FAIL;
				}

				if (!span || !span->signal_data) {
					stream->write_function(stream, "-ERR '%s' is not a valid TAP span\n",  argv[2]);
					return FTDM_FAIL;
				}

				if (!strcasecmp(argv[0], "enable")) {
					ftdm_sangoma_isdn_tap_io_stats(stream, span, "enable");
				} else if (!strcasecmp(argv[0], "disable")) {
					ftdm_sangoma_isdn_tap_io_stats(stream, span, "disable");
				} else if (!strcasecmp(argv[0], "flush")) {
					ftdm_sangoma_isdn_tap_io_stats(stream, span, "flush");
				} else {
					stream->write_function(stream, "-ERR Invalid Tap io_stats enable/disable/flush command\n",  argv[1]);
					stream->write_function(stream, "USAGE -> ftdm sangoma_isdn_tap io_stats <enable/disable/flush> span <all/span_id>\n");
					return FTDM_FAIL;
				}
			}
		} else {
			stream->write_function(stream, "-ERR Invalid Tap io_stats enable/disable/flush command\n",  argv[1]);
			stream->write_function(stream, "USAGE -> ftdm sangoma_isdn_tap io_stats <enable/disable/flush> span <all/span_id>\n");
			return FTDM_FAIL;
		}
	} else {
		stream->write_function(stream, "-ERR Invalid Tap command\n",  argv[1]);
		return FTDM_FAIL;
	}

	return FTDM_SUCCESS;
}

/* Use this API only for testing purpose */
/**********************************************************************************
 * Fun   : COMMAND_HANDLER(reset tap counters)
 *
 * Desc  : Reset all tapping related counters
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
COMMAND_HANDLER(reset)
{
	if (!argc) {
		stream->write_function(stream, "-ERR Invalid Tap reset command\n",  argv[1]);
		return FTDM_FAIL;
	}

	if (!strcasecmp(argv[0], "counters")) {
		setup_recv = 0;
		setup_recv_invalid_chanId = 0;
		setup_not_recv = 0;
		call_proceed_recv = 0;
		connect_recv = 0;
		early_connect_recv = 0;
		late_invite_recv = 0;
		connect_recv_with_chanId = 0;
		msg_recv_aft_time = 0;
		disconnect_recv = 0;
		release_recv = 0;
		call_tapped = 0;
		reuse_chan_con_recv =0;
		stream->write_function(stream, "**********************************************************\n");
		stream->write_function(stream, "               Tap Counetrs Reset successfully 			  \n");
		stream->write_function(stream, "**********************************************************\n");
		stream->write_function(stream, "    Total Number of Setup Received						   = %d\n", setup_recv);
		stream->write_function(stream, "    Total Number of Setup Received with invalid channel				   = %d\n", setup_recv_invalid_chanId);
		stream->write_function(stream, "    Total Number of Setup not Received						   = %d\n", setup_not_recv);
		stream->write_function(stream, "    Total Number of Call Proceed Received					   = %d\n", call_proceed_recv);
		stream->write_function(stream, "    Total Number of Connnect Received						   = %d\n", connect_recv);
		stream->write_function(stream, "    Total Number of Early Connnect Received					   = %d\n", early_connect_recv);
		stream->write_function(stream, "    Total Number of Late Invite Received					   = %d\n", late_invite_recv);
		stream->write_function(stream, "    Total Number of Connnect Received with channel				   = %d\n", connect_recv_with_chanId);
		stream->write_function(stream, "    Total Number of Message received after %d seconds				   = %d\n", SNG_TAP_SUBEQUENT_MSG_TME_DURATION, msg_recv_aft_time);
		stream->write_function(stream, "    Total Number of Disconnect Received						   = %d\n", disconnect_recv);
		stream->write_function(stream, "    Total Number of Release Received						   = %d\n", release_recv);
		stream->write_function(stream, "    Total Number of Tapped files generated					   = %d\n", call_tapped);
		stream->write_function(stream, "    Total Number of Connect Received where Channel was In-Use			   = %d\n", reuse_chan_con_recv);
	} else {
		stream->write_function(stream, "-ERR Invalid Tap command\n",  argv[1]);
		return FTDM_FAIL;
	}

	return FTDM_SUCCESS;
}
/* command map */
/**********************************************************************************
 * Fun   : command map structure
 *
 * Desc  : Maps the command
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
struct {
	const char*CMD; // command
	int  Argc;      // minimum args
	fCMD Handler;   // handling function
}
	ISDN_TAP_COMMANDS[] = {
		COMMAND(statistics, 0),
		COMMAND(show, 1),
		COMMAND(io_stats, 2),
		COMMAND(reset, 1),
	};

/**********************************************************************************
 * Fun   : FIO_API_FUNCTION(ftdm_sangoma_isdn_tap_api)
 *
 * Desc  : Call back function in order provide CLI commands w.r.t sng_isdn_tap
 * 	   module
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_API_FUNCTION(ftdm_sangoma_isdn_tap_api)
{
	char *mycmd = NULL, *argv[10] = { 0 };
	int argc = 0;
	int i;
	ftdm_status_t status = FTDM_FAIL;
	ftdm_status_t syntax = FTDM_FAIL;

	if (data) {
		mycmd = ftdm_strdup(data);
		argc = ftdm_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if(argc > 0) {
		for(i=0;i<sizeof(ISDN_TAP_COMMANDS)/sizeof(ISDN_TAP_COMMANDS[0]);i++) {
			if(strcasecmp(argv[0],ISDN_TAP_COMMANDS[i].CMD)==0) {
				if(argc -1 >= ISDN_TAP_COMMANDS[i].Argc) {
					syntax = FTDM_SUCCESS;
					status = ISDN_TAP_COMMANDS[i].Handler(stream, &argv[1], argc-1);
				}
				break;
			}
		}
	}

	if(FTDM_SUCCESS != syntax) {
		stream->write_function(stream, "%s", FT_SYNTAX);
	}
	else
		if(FTDM_SUCCESS != status) {
			stream->write_function(stream, "%s Command Failed\r\n", ISDN_TAP_COMMANDS[i].CMD);
		}
	ftdm_safe_free(mycmd);

	return FTDM_SUCCESS;
#if 0
	char *mycmd = NULL, *argv[10] = { 0 };
	int argc = 0;

	if (data) {
		mycmd = ftdm_strdup(data);
		argc = ftdm_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (argc > 2) {
		if (!strcasecmp(argv[0], "debug")) {
			ftdm_span_t *span = NULL;

			if (ftdm_span_find_by_name(argv[1], &span) == FTDM_SUCCESS) {
				sngisdn_tap_t *tap = span->signal_data;
				if (span->start != ftdm_sangoma_isdn_tap_start) {
					stream->write_function(stream, "%s: -ERR invalid span.\n", __FILE__);
					goto done;
				}

				/* Setting Up the debug value */
				sangoma_isdn_tap_set_debug(tap->pri, parse_debug(argv[2]));
				stream->write_function(stream, "%s: +OK debug set.\n", __FILE__);
				goto done;
			} else {
				stream->write_function(stream, "%s: -ERR invalid span.\n", __FILE__);
				goto done;
			}
		}
	}


	stream->write_function(stream, "%s: -ERR invalid command.\n", __FILE__);

 done:
	ftdm_safe_free(mycmd);
	return FTDM_SUCCESS;
#endif
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
