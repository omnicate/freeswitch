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
 * Contributors: Pushkar Singh <psingh@sangoma.com>
 *
 */

/********************************************************************************/
/*                                                                              */
/*                           libraries inclusion                                */
/*                                                                              */
/********************************************************************************/
#define _GNU_SOURCE
#ifndef WIN32
#include <poll.h>
#endif

#include "libsangoma.h"
#include "private/ftdm_core.h"
#include <stdarg.h>
#include <ctype.h>
#ifdef WIN32
#include <io.h>
#endif
#include "sng_decoder.h"
#include "ftmod_sangoma_isup_tap.h"

/********************************************************************************/
/*                                                                              */
/*                           	       MACROS                                   */
/*                                                                              */
/********************************************************************************/
#define MAX_VOICE_SPAN 100

/********************************************************************************/
/*                                                                              */
/*                               global declaration                             */
/*                                                                              */
/********************************************************************************/
static int g_decoder_lib_init;
static sng_decoder_event_interface_t    sng_event;
static ftdm_io_interface_t ftdm_sngisup_tap_interface;

static ftdm_status_t ftdm_sangoma_isup_tap_start(ftdm_span_t *span);
void sangoma_isup_tap_set_debug(struct sngisup_t *ss7_isup, int debug);
static void handle_isup_tapping_log(uint8_t level, char *fmt,...);
int ftdm_sangoma_isup_get_switch_type(const char *val);
static void sangoma_isup_tap_check_event(ftdm_span_t* span);
int ftdm_sangoma_isup_get_chanId(sngisup_tap_t *tap, int cir_val);
int ftdm_get_voice_span(int *v_span_id, const char *val);

/********************************************************************************/
/*                                                                              */
/*                              implementation                                  */
/*                                                                              */
/********************************************************************************/

/**********************************************************************************
 * Fun   : FIO_IO_UNLOAD_FUNCTION(ftdm_sangoma_isup_tap_unload)
 *
 * Desc  : Call back function in order to unload the sng_isup_tap module
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_IO_UNLOAD_FUNCTION(ftdm_sangoma_isup_tap_unload)
{
	/* Release Decoder lib resources */
	if(g_decoder_lib_init) {
		sng_isup_decoder_lib_deinit();
		g_decoder_lib_init = 0x00;
	}
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : FIO_CHANNEL_GET_SIG_STATUS_FUNCTION(sangoma_isup_tap_get_channel_sig_status)
 *
 * Desc  : Call back function in order to get the signalling status w.r.t a
 * 	   particular channel as requested by user
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_CHANNEL_GET_SIG_STATUS_FUNCTION(sangoma_isup_tap_get_channel_sig_status)
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
 * Fun   : FIO_SPAN_GET_SIG_STATUS_FUNCTION(sangoma_isup_tap_get_span_sig_status)
 *
 * Desc  : Call back function in order to get the signalling status w.r.t a
 * 	   particular span as requested by use
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_SPAN_GET_SIG_STATUS_FUNCTION(sangoma_isup_tap_get_span_sig_status)
{
	*status = FTDM_SIG_STATE_UP;
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : FIO_CHANNEL_OUTGOING_CALL_FUNCTION(sangoma_isup_tap_outgoing_call)
 *
 * Desc  : Call back function for outgoing call but in case of tapping one cannot
 * 	   perform any outgoing calls
 *
 * Ret   : FTDM_FAIL
**********************************************************************************/
static FIO_CHANNEL_OUTGOING_CALL_FUNCTION(sangoma_isup_tap_outgoing_call)
{
	ftdm_log(FTDM_LOG_ERROR, "Cannot dial on ISUP tapping line!\n");
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

	return flags;
}

/**********************************************************************************
 * Fun   : FIO_API_FUNCTION(ftdm_sangoma_isup_tap_api)
 *
 * Desc  : Call back function in order provide CLI commands w.r.t dng_isup_tap
 * 	   module
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_API_FUNCTION(ftdm_sangoma_isup_tap_api)
{
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
				sngisup_tap_t *tap = span->signal_data;
				if (span->start != ftdm_sangoma_isup_tap_start) {
					stream->write_function(stream, "%s: -ERR invalid span.\n", __FILE__);
					goto done;
				}

				/* Setting Up the debug value */
				sangoma_isup_tap_set_debug(tap->ss7_isup, parse_debug(argv[2]));
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
}

/**********************************************************************************
 * Fun   : FIO_IO_LOAD_FUNCTION(ftdm_sangoma_isup_tap_io_init)
 *
 * Desc  : Call back function in order to initialize set io w.r.t sng_isup_tap
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static FIO_IO_LOAD_FUNCTION(ftdm_sangoma_isup_tap_io_init)
{
	memset(&ftdm_sngisup_tap_interface, 0, sizeof(ftdm_sngisup_tap_interface));

	ftdm_sngisup_tap_interface.name = "sangoma_isup_tap";
	ftdm_sngisup_tap_interface.api = ftdm_sangoma_isup_tap_api;

	*fio = &ftdm_sngisup_tap_interface;

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : FIO_SIG_LOAD_FUNCTION(ftdm_sangoma_isup_tap_init)
 *
 * Desc  : Call back function in order to initialize sngisup_tap module + stack +
 * 	   decoder library
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static FIO_SIG_LOAD_FUNCTION(ftdm_sangoma_isup_tap_init)
{
	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Macro : ftdm_state_map_t
 *
 * Desc  : Mapping FTDM events
**********************************************************************************/
static ftdm_state_map_t sngisup_tap_state_map = {
	{
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_DOWN, FTDM_END},
			{FTDM_CHANNEL_STATE_RING, FTDM_END}
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_RING, FTDM_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP, FTDM_END}
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_END},
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_TERMINATING, FTDM_END},
			{FTDM_CHANNEL_STATE_DOWN, FTDM_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_PROGRESS, FTDM_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP, FTDM_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_UP, FTDM_END},
		},
		{
			ZSD_INBOUND,
			ZSM_UNACCEPTABLE,
			{FTDM_CHANNEL_STATE_UP, FTDM_END},
			{FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_TERMINATING, FTDM_END},
		},

	}
};

/**********************************************************************************
 * Fun   : ftdm_status_t state_advance
 *
 * Desc  : Change FTDM state depending upon ISUP message received
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
static ftdm_status_t state_advance(ftdm_channel_t *ftdmchan)
{
	ftdm_status_t status;
	ftdm_sigmsg_t sig;

	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "processing state %s\n", ftdm_channel_state2str(ftdmchan->state));

	memset(&sig, 0, sizeof(sig));
	sig.chan_id = ftdmchan->chan_id;
	sig.span_id = ftdmchan->span_id;
	sig.channel = ftdmchan;

	ftdm_channel_complete_state(ftdmchan);

	switch (ftdmchan->state) {
		case FTDM_CHANNEL_STATE_DOWN:
			{
				ftdm_channel_t *fchan = ftdmchan;

				/* Destroy the peer data first */
				if (fchan->call_data) {
					ftdm_channel_t *peerchan = fchan->call_data;
					ftdm_channel_t *pchan = peerchan;

					ftdm_channel_lock(peerchan);

					pchan->call_data = NULL;
					pchan->pflags = 0;
					ftdm_channel_close(&pchan);

					ftdm_channel_unlock(peerchan);
				} else {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "No call data?\n");
				}

				ftdmchan->call_data = NULL;
				ftdmchan->pflags = 0;
				ftdm_channel_close(&fchan);
			}
			break;

		case FTDM_CHANNEL_STATE_PROGRESS:
		case FTDM_CHANNEL_STATE_PROGRESS_MEDIA:
		case FTDM_CHANNEL_STATE_HANGUP:
			break;

		case FTDM_CHANNEL_STATE_UP:
			{
				/* Start Tapping Call as call is in connected state */
				sig.event_id = FTDM_SIGEVENT_UP;
				if ((status = ftdm_span_send_signal(ftdmchan->span, &sig) != FTDM_SUCCESS)) {
					ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
				}
			}
			break;

		case FTDM_CHANNEL_STATE_RING:
			{
				/* We will start Tapping calls only when Connect is recieved */
			}
			break;

		case FTDM_CHANNEL_STATE_TERMINATING:
			{
				if (ftdmchan->last_state != FTDM_CHANNEL_STATE_HANGUP) {
					sig.event_id = FTDM_SIGEVENT_STOP;
					status = ftdm_span_send_signal(ftdmchan->span, &sig);
				}
				ftdm_set_state_locked(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
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
 * Fun   : ftmod_isup_process_event()
 *
 * Desc  : Handle wanpipe events does not change state from Down to Up once ALARM
 * 	   Clear is recieved as We only change channel state to UP once call is
 * 	   being answered on that particular channel and Tapping starts
 *
 * Ret   : FTDM_SUCCESS
 **********************************************************************************/
static __inline__ ftdm_status_t ftmod_isup_process_event(ftdm_span_t *span, ftdm_event_t *event)
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
 * Fun   : sangoma_isup_tap_check_state()
 *
 * Desc  : Checks the state of the span
 *
 * Ret   : void
**********************************************************************************/
static __inline__ void sangoma_isup_tap_check_state(ftdm_span_t *span)
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
 * Fun   : sangoma_isup_tap__get_pcall_bycic()
 *
 * Desc  : Get sngisup_tap call w.r.t to CIC value if the message is respect to the
 * 	   call already in connected state
 *
 * Ret   : tap->pcalls | NULL
**********************************************************************************/
static passive_call_t *sangoma_isup_tap_get_pcall_bycic(sngisup_tap_t *tap, int cir_val)
{
	int i;
	int tstcic;

	ftdm_mutex_lock(tap->pcalls_lock);

	/* Go through all call present and check if the CIC value matches to any one of the call then return the
	 * respective call control block else return NULL */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		tstcic = tap->pcalls[i].cic_val;
		if (tstcic == cir_val) {
			if (tap->pcalls[i].inuse) {
				ftdm_mutex_unlock(tap->pcalls_lock);
				return &tap->pcalls[i];
			}

			/* This just means the cir_val is being re-used in another call before this one was destroyed */
			ftdm_log(FTDM_LOG_DEBUG, "Found CIC %d in slot %d of span %s with call %d but is no longer in use\n",
					cir_val, i, tap->span->name, tap->pcalls[i].cic_val);
		}
	}
	ftdm_mutex_unlock(tap->pcalls_lock);
	ftdm_log(FTDM_LOG_DEBUG, "CIC value %d not found active in span %s\n", cir_val, tap->span->name);
	return NULL;
}

/**********************************************************************************
 * Fun   : sangoma_isup_tap_get_pcall()
 * 
 * Desc  : It mainly gives the pcall structure w.r.t the tap on which call is
 * 	   getting established on the basis of CIC value.
 *
 * Ret   : tap->pcalls | NULL
 ***********************************************************************************/
static passive_call_t *sangoma_isup_tap_get_pcall(sngisup_tap_t *tap, void *cic)
{
	int i;
	int cir_val = 0;
	int *tmp_cic = NULL;
	passive_call_t* pcall = NULL;

	ftdm_mutex_lock(tap->pcalls_lock);

	/* If cic matches to any of the existing cic value the Enable call ref slot */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		if (cic) {
			tmp_cic = (int*)cic;
			cir_val = *tmp_cic;
		}

		if (cir_val == tap->pcalls[i].cic_val) {
			if (cic == NULL) {
				tap->pcalls[i].inuse = 1;
				ftdm_log(FTDM_LOG_DEBUG, "Enabling slot %d in span %s for CIC %d\n", i, tap->span->name, cir_val);
			} else if (!tap->pcalls[i].inuse) {
				ftdm_log(FTDM_LOG_DEBUG, "Recyclying slot %d in span %s for CIC %d/%p\n",
						i, tap->span->name, cir_val, cic);
				memset(&tap->pcalls[i], 0, sizeof(tap->pcalls[0]));
				tap->pcalls[i].cic_val = cir_val;
				tap->pcalls[i].inuse = 1;
			}
			pcall = &tap->pcalls[i];
			ftdm_mutex_unlock(tap->pcalls_lock);
			return pcall;
		}
	}

	ftdm_mutex_unlock(tap->pcalls_lock);
	return NULL;
}

/**********************************************************************************
 * Fun   : sangoma_isup_tap_put_pcall()
 *
 * Desc  : Once DISCONNECT or RELEASE COMPLETE message is received this funtion
 * 	   release the slot on which call got established
 *
 * Ret   : void
**********************************************************************************/
static void sangoma_isup_tap_put_pcall(sngisup_tap_t *tap, void *cic)
{
	int i;
	int cir_val;
	int tstcic;
	int *tmp_cic = cic;

	if (!cic) {
		ftdm_log(FTDM_LOG_ERROR, "Cannot put pcall for null CIC in span %s\n", tap->span->name);
		return;
	}

	ftdm_mutex_lock(tap->pcalls_lock);

	/* Get the CIC value */
	cir_val = *tmp_cic;

	/* Check if the CIC matches to any of the present call CIC. If matches then release the slot
	 * on which respective call is active */
	for (i = 0; i < ftdm_array_len(tap->pcalls); i++) {
		if (!tap->pcalls[i].cic_val) {
			continue;
		}
		tstcic = tap->pcalls[i].cic_val;

		if (tstcic == cir_val) {
			if (tap->pcalls[i].inuse) {
				ftdm_log(FTDM_LOG_DEBUG, "Releasing slot %d in span %s used by CIC %d/%d\n", i,
						tap->span->name, cir_val, tap->pcalls[i].cic_val);
				tap->pcalls[i].inuse = 0;
				tap->pcalls[i].proceeding = 0;
				tap->pcalls[i].cic_val = 0;
				tap->pcalls[i].fchan = NULL;
			}
		}
	}

	ftdm_mutex_unlock(tap->pcalls_lock);
}

/**********************************************************************************
 * Fun   : sangoma_isup_tap_get_fchan()
 *
 * Desc  : Get the channel on which call needs to be tapped
 *
 * Ret   : fchan | NULL
**********************************************************************************/
static __inline__ ftdm_channel_t *sangoma_isup_tap_get_fchan(sngisup_tap_t *tap, passive_call_t *pcall, int channel)
{
	ftdm_channel_t *fchan = NULL;
	int idx = 0;
	int err = 0;
	uint16_t chanpos = SNG_ISUP_CHANNEL(channel);
	ftdm_log(FTDM_LOG_CRIT, "Channel position %d and total channel configured %d\n", chanpos, tap->span->chan_count);
	if (!chanpos || (chanpos > tap->span->chan_count)) {
		ftdm_log(FTDM_LOG_CRIT, "Invalid isup tap channel %d requested in span %s\n", channel, tap->span->name);
		return NULL;
	}

	/* Check through all the configured channels matching channel ID and return the channel structure */
	for (idx = 1; idx < FTDM_MAX_CHANNELS_SPAN; idx++) {
		fchan = tap->span->channels[SNG_ISUP_CHANNEL(idx)];
		if ((fchan == NULL) || (fchan->physical_chan_id == channel)) {
			break;
		}
	}

	if (!fchan) {
		err = 1;
		goto done;
	}

	ftdm_channel_lock(fchan);

	if (ftdm_test_flag(fchan, FTDM_CHANNEL_INUSE)) {
		ftdm_log(FTDM_LOG_ERROR, "Channel %d requested in span %s is already in use!\n", channel, tap->span->name);
		err = 1;
		goto done;
	}

	if (ftdm_channel_open_chan(fchan) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_ERROR, "Could not open tap channel %d requested in span %s\n", channel, tap->span->name);
		err = 1;
		goto done;
	}

	memset(&fchan->caller_data, 0, sizeof(fchan->caller_data));

	ftdm_set_string(fchan->caller_data.cid_num.digits, pcall->callingnum.digits);
	ftdm_set_string(fchan->caller_data.cid_name, pcall->callingnum.digits);
	ftdm_set_string(fchan->caller_data.dnis.digits, pcall->callednum.digits);

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
 * Fun   : handle_isup_passive_event()
 *
 * Desc  : Handle the events(i.e. ISUP messgages) once received and start tapping
 * 	   call depending up on the events received
 *
 * Ret   : void
**********************************************************************************/
static void handle_isup_passive_event(sngisup_tap_t *tap, uint8_t msgType, sng_isup_decoder_interface_t *intfCb)
{
	passive_call_t *pcall = NULL;
	passive_call_t *peerpcall = NULL;
	ftdm_channel_t *fchan = NULL;
	ftdm_channel_t *peerfchan = NULL;
	int cir_val = 0;
	int chan_id = 0;
	int idx = 0;
	sngisup_tap_t *peertap = tap->peerspan->signal_data;
	sngisup_tap_t *rcv_call_tap = NULL;						/* Span Info on which call is recieved */
	sngisup_tap_t *peer_rcv_call_tap = NULL;				/* Peer Span which serves the revieved call */

	if (!intfCb)
		return;

	cir_val = intfCb->cic;
	chan_id = ftdm_sangoma_isup_get_chanId(tap, cir_val);
	if (chan_id == 0) {
		/* Check the cic Id in voice channels */
		for (idx = 0; idx < MAX_VOICE_SPAN; idx++) {
			if (tap->voice_span[idx].span_id == 0) {
				ftdm_log(FTDM_LOG_ERROR, "Tapping can not be proceed as no channel found w.r.t to the cic value %d\n", cir_val);
				return ;
			}
			rcv_call_tap = tap->voice_span[idx].span->signal_data;

			chan_id = ftdm_sangoma_isup_get_chanId(rcv_call_tap, cir_val);
			if (chan_id) {
				/* If the respective channel is found then break as check in other pure voice spans */
				if (peertap->voice_span[idx].span_id == 0) {
					ftdm_log(FTDM_LOG_ERROR, "Could not found peer span w.r.t voice span %s\n", rcv_call_tap->span->name);
					return ;
				}
				peer_rcv_call_tap = peertap->voice_span[idx].span->signal_data;
				tap = rcv_call_tap;
				peertap = peer_rcv_call_tap;
				break;
			}
		}

		if (chan_id == 0) {
			ftdm_log(FTDM_LOG_ERROR, "Could not find the channel w.r.t CIC value %d\n", cir_val);
			return ;
		}
	}

	switch (msgType) {
		case M_INIADDR:
			/* we cannot use ftdm_channel_t because we still dont know which channel will be used 
			 * (ie, flexible channel was requested), thus, we need our own list of CIC values */

			ftdm_log(FTDM_LOG_DEBUG, "Initial Address Message[IAM] recieved on span:  %s with CIC value as %d\n",
					tap->span->name, cir_val);
			pcall = sangoma_isup_tap_get_pcall_bycic(tap, cir_val);

			if (pcall) {
				ftdm_log(FTDM_LOG_WARNING, "There is a call with CIC %d already, ignoring duplicated setup event\n", cir_val);
				break;
			}

			/* Try to get a recycled call (ie, get the call depending up on the CIC value that the ISUP stack allocated previously and then
			 * re-used for the next RING event because we did not destroy it fast enough) */
			pcall = sangoma_isup_tap_get_pcall(tap, &cir_val);
			if (!pcall) {
				ftdm_log(FTDM_LOG_DEBUG, "Creating new pcall with CIC = %d\n", cir_val);
				/* ok so the call is really not known to us, let's get a new one */
				pcall = sangoma_isup_tap_get_pcall(tap, NULL);
				if (!pcall) {
					ftdm_log(FTDM_LOG_ERROR, "Failed to get a free passive ISUP call slot for CIC %d, this is a bug!\n", cir_val);
					break;
				}
			}

			pcall->cic_val = cir_val;
			pcall->proceeding = 0;
			pcall->chanId = chan_id;

			/* If called number is present then only fill the same */
			if (strlen(intfCb->cld_data.num)) {
				ftdm_log(FTDM_LOG_DEBUG,"cld_num[%s]\n", intfCb->cld_data.num);
				ftdm_set_string(pcall->callednum.digits, intfCb->cld_data.num); 
			}

			/* If calling number is present then only fill the same */
			if (strlen(intfCb->clg_data.num)) {
				ftdm_log(FTDM_LOG_DEBUG,"clg_num[%s]\n", intfCb->clg_data.num);
				ftdm_set_string(pcall->callingnum.digits, intfCb->clg_data.num); 
			}

			break;

		case M_ADDCOMP:
			ftdm_log(FTDM_LOG_DEBUG, "ADDRESS COMPLETE MESSAGE[ACM] received on span  %s with CIC value as %d\n", tap->span->name, cir_val);
			break;

		case M_CALLPROG:
			/* at this point we should know the real b chan that will be used and can therefore proceed to notify about the call, but
			 * only if a couple of call tests are passed first */
			ftdm_log(FTDM_LOG_DEBUG, "CALL PROGRESS MESSAGE[CPG] received on Span %s with CIC value as %d\n", tap->span->name, cir_val);

			/* CALL PROCEED will come from B end (A to B call) ,
			 * peertap is refering to A party here and tap referring to B party */

			/* CP - will come on B leg , whereas IAM received on A leg */
			/* tap , pcall , fchan = B leg
			 * peertap, peerptap , peerfchan = A leg
			 */

			/* check that we already know about this call in the peer ISUP (which was the one receiving the IAM event) */
			if (!(peerpcall = sangoma_isup_tap_get_pcall_bycic(peertap, cir_val))) {
				ftdm_log(FTDM_LOG_DEBUG,
						"Ignoring Call PROGRESS event on Span %s for CIC value as %d since we don't know about it\n",
						tap->span->name, cir_val);
				break;
			}

			if (peerpcall->proceeding) {
				ftdm_log(FTDM_LOG_DEBUG, "Duplicate Call PROGRESS event received with CIC value as %d\n", cir_val);
				ftdm_log(FTDM_LOG_DEBUG, "Ignoring Duplicate event received\n");
				break;
			}
			peerpcall->proceeding = 1;

			/* This call should not be known to this ISUP yet ... */
			if ((pcall = sangoma_isup_tap_get_pcall_bycic(tap, cir_val))) {
				ftdm_log(FTDM_LOG_ERROR,
						"Ignoring subsequent call progress event on span:%s channel:%d for CIC %d\n",
						tap->span->name, pcall->chanId, cir_val);
				break;
			}

			/* Check if the call pointer is being recycled */
			pcall = sangoma_isup_tap_get_pcall(tap, &cir_val);
			if (!pcall) {
				pcall = sangoma_isup_tap_get_pcall(tap, NULL);
				if (!pcall) {
					ftdm_log(FTDM_LOG_ERROR, "Failed to get a free peer ISUP passive call slot for CIC %d in span %s, this is a bug!\n",
							cir_val, tap->span->name);
					break;
				}
				pcall->cic_val = cir_val;
				pcall->chanId = chan_id;
			}

			ftdm_log(FTDM_LOG_DEBUG, "Call PROGRESSING on Channel %d\n", pcall->chanId);
			/* tap = B */
			fchan = sangoma_isup_tap_get_fchan(tap, pcall, pcall->chanId);
			if (!fchan) {
				ftdm_log(FTDM_LOG_ERROR, "Call Progress requested on odd/unavailable channel %s:1 for CIC %d\n",
						tap->span->name, cir_val);
				break;
			}

			ftdm_log(FTDM_LOG_DEBUG, "Call PROGRESSING on Channel %d\n", peerpcall->chanId);
			/* peerfchan = A party (in a to b call) */
			peerfchan = sangoma_isup_tap_get_fchan(peertap, peerpcall, peerpcall->chanId);
			if (!peerfchan) {
				ftdm_log(FTDM_LOG_ERROR, "Call Progress requested on odd/unavailable channel %s:1 for CIC %d\n",
						peertap->span->name, cir_val);
				break;
			}
			pcall->fchan = fchan;
			peerpcall->fchan = peerfchan;

			ftdm_log_chan(fchan, FTDM_LOG_NOTICE, "Starting new tapped call for CIC %d\n", cir_val);

			ftdm_channel_lock(fchan);
			fchan->call_data = peerfchan; /* fchan=B leg, pointing to peerfchan(A leg) */
			ftdm_set_state(fchan, FTDM_CHANNEL_STATE_RING);
			ftdm_channel_unlock(fchan);

			ftdm_channel_lock(peerfchan);
			peerfchan->call_data = fchan; /* peerfchan=A leg(IAM received), pointing to fchan(B leg) */
			ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_RING);
			ftdm_channel_unlock(peerfchan);

			break;

		case M_ANSWER:
			ftdm_log(FTDM_LOG_DEBUG, "CALL ANSWER MESSAGE recieved on span:%s channel:%d is received with CIC value as %d\n",
					tap->span->name, chan_id, cir_val);

			/* ANSWER will come on B leg
			 * tap , pcall , fchan = B leg
			 * */

			if (!(pcall = sangoma_isup_tap_get_pcall_bycic(tap, cir_val))) {
				ftdm_log(FTDM_LOG_DEBUG,
						"Ignoring answer on span:%s channel:%d for CIC %d since we don't know about it\n",
						tap->span->name, chan_id, cir_val);
				break;
			}

			if (!pcall->fchan) {
				ftdm_log(FTDM_LOG_ERROR,
						"Received answer on Span:%s channel:%d for CIC %d but we never got a channel\n",
						tap->span->name, chan_id, cir_val);
				break;
			}

			/* Once Call is answered then only change channel state to UP */
			ftdm_channel_lock(pcall->fchan);
			ftdm_log_chan(pcall->fchan, FTDM_LOG_NOTICE, "Tapped call was answered in state %s\n", ftdm_channel_state2str(pcall->fchan->state));
			ftdm_set_pflag(pcall->fchan, ISUP_TAP_NETWORK_ANSWER);
			ftdm_set_state(pcall->fchan, FTDM_CHANNEL_STATE_UP);
			ftdm_channel_unlock(pcall->fchan);

			/* B leg answered, move A leg state also to ANSWER */
			peerfchan = (ftdm_channel_t*)pcall->fchan->call_data;
			if (!peerfchan) {
				ftdm_log(FTDM_LOG_ERROR,"Peer channel not found \n");
				break;
			}
			ftdm_channel_lock(peerfchan);
			ftdm_log_chan(peerfchan, FTDM_LOG_NOTICE, "Peer : Tapped call was answered in state %s\n", ftdm_channel_state2str(peerfchan->state));
			ftdm_set_pflag(peerfchan, ISUP_TAP_NETWORK_ANSWER);
			ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_UP);
			ftdm_channel_unlock(peerfchan);
			break;

		case M_RELSE:
		case M_RELCOMP:
			ftdm_log(FTDM_LOG_DEBUG, "Call %s event is received on span:%s channel:%d with CIC value as %d\n",
					SNG_DECODE_ISUP_EVENT(msgType),tap->span->name, chan_id, cir_val);

			if (!(pcall = sangoma_isup_tap_get_pcall_bycic(tap, cir_val))) {
				ftdm_log(FTDM_LOG_DEBUG,
						"Ignoring hangup in span:%s channel:%d for CIC %d since we don't know about it\n",
						tap->span->name, chan_id, cir_val);
				goto end;
			}

			if (pcall->fchan) {
				fchan = pcall->fchan;
				ftdm_channel_lock(fchan);
				if (fchan->state < FTDM_CHANNEL_STATE_TERMINATING) {
					ftdm_set_state(fchan, FTDM_CHANNEL_STATE_TERMINATING);
					ftdm_log(FTDM_LOG_DEBUG, "Changing span[%d] channel[%d] state to TERMINATING\n", fchan->span_id, fchan->chan_id);
				}
				ftdm_channel_unlock(fchan);
			}

			if (pcall->fchan) {
				/* Move peer state also to disconnect */
				peerfchan = (ftdm_channel_t*)pcall->fchan->call_data;
			}

			if (peerfchan) {
				ftdm_channel_lock(peerfchan);
				if (peerfchan->state < FTDM_CHANNEL_STATE_TERMINATING) {
					ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_TERMINATING);
					ftdm_log(FTDM_LOG_DEBUG, "Changing peer span[%d] channel[%d] state to TERMINATING\n", peerfchan->span_id, peerfchan->chan_id);
				}
				ftdm_channel_unlock(peerfchan);
			}

end:
			sangoma_isup_tap_put_pcall(tap, &cir_val);
			sangoma_isup_tap_put_pcall(peertap, &cir_val);
			break;

		default:
			ftdm_log(FTDM_LOG_DEBUG, "Ignoring passive event %s on span %s\n", SNG_DECODE_ISUP_EVENT(msgType), tap->span->name);
			break;

	}
}

/**********************************************************************************
 * Fun   : sangoma_isup_tap_check_event()
 *
 * Desc  : Get the channel on which call needs to be tapped
 *
 * Ret   : fchan | NULL
**********************************************************************************/
static void sangoma_isup_tap_check_event(ftdm_span_t* span)
{
	ftdm_status_t status;
	ftdm_event_t *event = NULL;

	status = ftdm_span_poll_event(span, 5, NULL);
	switch (status) {
		case FTDM_SUCCESS:
			{
				while (ftdm_span_next_event(span, &event) == FTDM_SUCCESS) {
					if (event->e_type == FTDM_EVENT_OOB) {
						ftmod_isup_process_event(span, event);
					}
				}
				break;
			}

		case FTDM_FAIL:
			{
				ftdm_log(FTDM_LOG_DEBUG, "Event Failure\n");
				break;
			}

		default:
			break;
	}
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isup_tap_run()
 *
 * Desc  : Run the sngisup_tap thread and always keep on monitoring on the messages
 * 	   getting received on peers as well as self sngisup_tap D-channel.
 * 	   Depending upon which tapping needs to be started
 *
 * Ret   : void | NULL
**********************************************************************************/
static void *ftdm_sangoma_isup_tap_run(ftdm_thread_t *me, void *obj)
{
	ftdm_span_t *span = (ftdm_span_t *) obj;
	ftdm_span_t *peer = NULL;
	sngisup_tap_t *tap = span->signal_data;
	sangoma_status_t status;
	wanpipe_api_t tdm_api;
	wp_api_hdr_t rxhdr;
	sngisup_tap_t *peer_tap = NULL;
	sng_isup_decoder_interface_t intf;        	/* Interface structure between decoder and app */
	sng_fd_t fd = INVALID_HANDLE_VALUE;		/* Signalling channel fd */
	unsigned char 	data[1024];			/* Stores ISUP Messages */
	uint8_t msgType = 0;    			/* Type of the message received */
	uint32_t input_flags = 0;
	uint32_t output_flags = 0;
	int sio = 0;
	int link_status=0;
	int mlen=0;
	int queueSize=0;
	int res = 0;

	memset(&intf,0,sizeof(intf));

	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISUP thread started on span %s\n", span->name);

	tap->span = span;

	ftdm_set_flag(span, FTDM_SPAN_IN_THREAD);

	if (ftdm_channel_open(span->span_id, tap->dchan->chan_id, &tap->dchan) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_ERROR, "Failed to open D-channel for span %s\n", span->name);
		goto done;
	}

	if ((tap->ss7_isup = sangoma_isup_new_cb(tap->dchan->sockfd, tap))) {
		sangoma_isup_tap_set_debug(tap->ss7_isup, tap->debug);
	} else {
		ftdm_log(FTDM_LOG_CRIT, "Failed to create tapping ISUP\n");
		goto done;
	}

	tap->sng.spanno = span->span_id;
	tap->sng.channo = tap->dchan->chan_id;

	fd = tap->dchan->sockfd;

	/* create the sangoma_wait_obj */
	status = sangoma_wait_obj_create(&tap->sng_wait_obj, fd, SANGOMA_DEVICE_WAIT_OBJ);
	if(status != SANG_STATUS_SUCCESS) {
		ftdm_log(FTDM_LOG_ERROR, "Failed to create Rx Sangoma Wait Object for span %d and chan %d\n",
				tap->sng.spanno,
				tap->sng.channo);
		goto done;
	}

	/* Wait to set object context*/
	sangoma_wait_obj_set_context(tap->sng_wait_obj, &tap->sng);

	/* create the sangoma_wait_obj for the Tx thread */
	status = sangoma_wait_obj_create(&tap->tx_sng_wait_obj, fd, SANGOMA_DEVICE_WAIT_OBJ);
	if(status != SANG_STATUS_SUCCESS){
		ftdm_log(FTDM_LOG_ERROR,"Failed to create Tx Sangoma Wait Object for span %d, chan %d\n",
				tap->sng.spanno,
				tap->sng.channo);

		/* delete the rx wait object */
		sangoma_wait_obj_delete(&tap->sng_wait_obj);
		goto done;
	}

	/* wait to set object context */
	sangoma_wait_obj_set_context(tap->tx_sng_wait_obj, &tap->sng);

	/* get and set the current tx queue size */
	memset(&tdm_api, 0x0, sizeof(wanpipe_api_t));
	queueSize = sangoma_get_tx_queue_sz(fd, &tdm_api);
	ftdm_log(FTDM_LOG_DEBUG, "Span = %d, chan = %d current tx queue size = %d\n",
			tap->sng.spanno,
			tap->sng.channo,
			queueSize);

	queueSize = 500;
	memset(&tdm_api, 0x0, sizeof(wanpipe_api_t));
	if ((sangoma_set_tx_queue_sz(fd, &tdm_api, queueSize)) != 0) {
		ftdm_log(FTDM_LOG_ERROR, "Failed to increase signalling link  queue size to %d\n", queueSize);
	} else {
		queueSize = sangoma_get_tx_queue_sz(fd, &tdm_api);
		ftdm_log(FTDM_LOG_DEBUG, "Span = %d, chan = %d current tx queue size = %d\n",
				tap->sng.spanno,
				tap->sng.channo,
				queueSize);
	}

	/* get and set the current rx queue size */
	memset(&tdm_api, 0x0, sizeof(wanpipe_api_t));
	queueSize = sangoma_get_rx_queue_sz(fd, &tdm_api);
	ftdm_log(FTDM_LOG_DEBUG, "Span = %d, chan = %d current rx queue size = %d\n",
			tap->sng.spanno,
			tap->sng.channo,
			queueSize);

	queueSize = 500;

	memset(&tdm_api, 0x0, sizeof(wanpipe_api_t));
	if ((sangoma_set_rx_queue_sz(fd, &tdm_api, queueSize)) != 0) {
		ftdm_log(FTDM_LOG_ERROR, "Failed to increase signalling link queue size to %d\n", queueSize);
	} else {
		queueSize = sangoma_get_rx_queue_sz(fd, &tdm_api);
		ftdm_log(FTDM_LOG_DEBUG, "Span = %d, chan = %d current rx queue size = %d\n",
				tap->sng.spanno,
				tap->sng.channo,
				queueSize);
	}

	/* get the current fe status (connected/disconnected */
	memset(&tdm_api, 0x0, sizeof(wanpipe_api_t));
	if ((sangoma_get_fe_status(fd, &tdm_api, (unsigned char *)&link_status)) != 0) {
		ftdm_log(FTDM_LOG_ERROR,"Failed to get the current Span status for span %d, chan %d\n",
				tap->sng.spanno,
				tap->sng.channo);
	}

	/* To print the configuration w.r.t the signalling channel */
	{
		wan_api_ss7_cfg_status_t wan_ss7_cfg;
		/* get the Wanpipe SS7 configuration */
		memset(&tdm_api, 0x0, sizeof(wanpipe_api_t));
		memset(&wan_ss7_cfg, 0x0, sizeof(wan_api_ss7_cfg_status_t));
		if ((sangoma_ss7_get_cfg_status(fd, &tdm_api, &wan_ss7_cfg)) != 0) {
			ftdm_log(FTDM_LOG_ERROR,"Failed to get the SS7 CFG for span %d, chan %d\n",
					tap->sng.spanno,
					tap->sng.channo);
		}

		ftdm_log(FTDM_LOG_INFO, "SS7 HW Configuratoin\n");
		ftdm_log(FTDM_LOG_INFO, "SS7 hw support   = %s\n",wan_ss7_cfg.ss7_hw_enable?"Enabled":"Disabled");
		ftdm_log(FTDM_LOG_INFO, "SS7 hw mode      = %s\n",wan_ss7_cfg.ss7_hw_mode?"4096":"128");
		ftdm_log(FTDM_LOG_INFO, "SS7 hw lssu size = %d\n",wan_ss7_cfg.ss7_hw_lssu_size);
		ftdm_log(FTDM_LOG_INFO, "SS7 driver repeat= %s\n",wan_ss7_cfg.ss7_driver_repeat?"Enabled":"Disabled");
	}

	/* The last span starting runs the show ...
	 * This simplifies locking and avoid races by having multiple threads for a single tapped link
	 * Since both threads really handle a single tapped link there is no benefit on multi-threading, just complications ... */
	peer = tap->peerspan;
	peer_tap = peer->signal_data;
	if (!ftdm_test_flag(tap, ISUP_TAP_MASTER)) {
		ftdm_log(FTDM_LOG_DEBUG, "Running dummy thread on span %s\n", span->name);
		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {
			ftdm_sleep(100);
		}
	} else {
		ftdm_log(FTDM_LOG_DEBUG, "Master tapping thread on span %s (fd1=%d, fd2=%d)\n", span->name,
				tap->dchan->sockfd, peer_tap->dchan->sockfd);

		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {

			sangoma_isup_tap_check_state(span);
			sangoma_isup_tap_check_state(peer);

			sangoma_isup_tap_check_event(span);
			sangoma_isup_tap_check_event(peer);

			input_flags = POLLIN | POLLHUP | POLLPRI;
			output_flags = 0;

			fd = sangoma_wait_obj_get_fd(tap->sng_wait_obj);
			res = sangoma_waitfor(tap->sng_wait_obj,
					input_flags,
					(uint32_t *)&output_flags,
					1000);

			switch (res) {
				case SANG_STATUS_APIPOLL_TIMEOUT:
					break;

				case SANG_STATUS_SUCCESS:
					if( output_flags & POLLPRI ) {
						sangoma_isup_tap_check_event(span);
					}

					if( output_flags & POLLIN ) {
						do {
							memset(&rxhdr, '\0', sizeof(wp_api_hdr_t));
							memset(&data, 0, sizeof(data));

							/* Read messages if any on d-channel */
							mlen = sangoma_readmsg( fd,
									&rxhdr,
									sizeof( wp_api_hdr_t),
									&data,
									sizeof(data),
									0 );

							/* Service Indicator will be present in 4th byte */
							sio = (int)data[3];

							/* check if received message is ISUP or not */
							if (mlen < 8 || ((sio & 0x0f) != SNG_ISUP)) {
								msgType = 0;
							} else {
								/* Decode the message and depending up on the message take respective actions for tapping */
								msgType =(uint8_t)isup_decode(tap->switch_type, (data + 8), (mlen - 8), tap->decCb, &intf);
								ftdm_log(FTDM_LOG_DEBUG, "%s Message(%d) received\n", SNG_DECODE_ISUP_EVENT(msgType), msgType);
							}

							/* If its a valid SS7(ISUP) message then only handle events */
							if (msgType) {
								ftdm_log(FTDM_LOG_DEBUG, "ISUP Message Received\n");
								handle_isup_passive_event(tap, msgType, &intf);
								sangoma_isup_tap_check_state(span);
							}
						} while (rxhdr.wp_api_rx_hdr_number_of_frames_in_queue > 1);
					}
					sangoma_isup_tap_check_event(span);

				default:
					break;
			}

			mlen = 0;
			res = 0;
			input_flags = POLLIN | POLLHUP | POLLPRI;
			output_flags = 0;

			fd = sangoma_wait_obj_get_fd(peer_tap->sng_wait_obj);
			res = sangoma_waitfor(peer_tap->sng_wait_obj,
					input_flags,
					(uint32_t *)&output_flags,
					1000);

			switch (res) {
				case SANG_STATUS_APIPOLL_TIMEOUT:
					break;

				case SANG_STATUS_SUCCESS:
					if( output_flags & POLLPRI ) {
						sangoma_isup_tap_check_event(peer);
					}

					if( output_flags & POLLIN ) {
						do {
							memset(&rxhdr, '\0', sizeof(wp_api_hdr_t));
							memset(&data, 0, sizeof(data));

							/* Read messages if any on d-channel */
							mlen = sangoma_readmsg( fd,
									&rxhdr,
									sizeof( wp_api_hdr_t),
									&data,
									sizeof(data),
									0 );

							/* Service Indicator will be present in 4th byte */
							sio = (int)data[3];

							/* Check Whether message is for ISUP or not */
							if (mlen < 8 || ((sio & 0x0f) != SNG_ISUP)) {
								msgType = 0;
							} else {
								/* Decode message and take respective actions depending upon the msgType for tapping */
								msgType =(uint8_t)isup_decode(peer_tap->switch_type, (data + 8), (mlen - 8), peer_tap->decCb, &intf);
								ftdm_log(FTDM_LOG_DEBUG, "%s Message(%d) received\n", SNG_DECODE_ISUP_EVENT(msgType), msgType);
							}

							/* If its a valid SS7(ISUP) message then only handle events */
							if (msgType) {
								ftdm_log(FTDM_LOG_DEBUG, "ISUP Message Received\n");
								handle_isup_passive_event(peer_tap, msgType, &intf);
								sangoma_isup_tap_check_state(peer);
							}
						} while (rxhdr.wp_api_rx_hdr_number_of_frames_in_queue > 1);
					}
					sangoma_isup_tap_check_event(peer);

				default:
					break;
			}
		} /* end of while */
	} /* end if */

done:
	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISUP thread ended on span %s\n", span->name);

	ftdm_clear_flag(span, FTDM_SPAN_IN_THREAD);
	ftdm_clear_flag(tap, ISUP_TAP_RUNNING);
	ftdm_clear_flag(tap, ISUP_TAP_MASTER);
	return NULL;
}

static void *ftdm_sangoma_isup_voice_span_run(ftdm_thread_t *me, void *obj)
{
	ftdm_span_t *span = (ftdm_span_t *) obj;
	ftdm_span_t *peer = NULL;
	sngisup_tap_t *tap = span->signal_data;

	sngisup_tap_t *peer_tap = NULL;

	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISUP pure voice thread started on span %s\n", span->name);

	if (!span) {
		ftdm_log(FTDM_LOG_ERROR, "No voice span found in order to start thread\n");
		goto done;
	}

	tap->span = span;

	ftdm_set_flag(span, FTDM_SPAN_IN_THREAD);

	tap->sng.spanno = span->span_id;

	peer = tap->peerspan;
	peer_tap = peer->signal_data;
	if (!ftdm_test_flag(tap, ISUP_TAP_MASTER)) {
		ftdm_log(FTDM_LOG_DEBUG, "Running dummy thread on pure voice span %s\n", span->name);
		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {
			ftdm_sleep(100);
		}
	} else {
		ftdm_log(FTDM_LOG_DEBUG, "Master tapping thread on pure voice span %s\n", span->name);

		while (ftdm_running() && !ftdm_test_flag(span, FTDM_SPAN_STOP_THREAD)) {

			sangoma_isup_tap_check_state(span);
			sangoma_isup_tap_check_state(peer);

			sangoma_isup_tap_check_event(span);
			sangoma_isup_tap_check_event(peer);

		} /* end of while */
	} /* end if */

done:
	ftdm_log(FTDM_LOG_DEBUG, "Tapping ISUP thread ended on span %s\n", span->name);

	ftdm_clear_flag(span, FTDM_SPAN_IN_THREAD);
	ftdm_clear_flag(tap, ISUP_TAP_RUNNING);
	ftdm_clear_flag(tap, ISUP_TAP_MASTER);
	return NULL;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isup_tap_stop()
 *
 * Desc  : Stop configured sngisup_tap span
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isup_tap_stop(ftdm_span_t *span)
{
	sngisup_tap_t *tap = span->signal_data;

	/* Free the allocated memory for isup decoder control block */
	if (tap->decCb) {
		sng_isup_decoder_freecb(tap->decCb);
	}

	if (!ftdm_test_flag(tap, ISUP_TAP_RUNNING)) {
		return FTDM_FAIL;
	}

	ftdm_set_flag(span, FTDM_SPAN_STOP_THREAD);
	/* release local isup message control block */
	ftdm_safe_free(tap->ss7_isup);

	while (ftdm_test_flag(span, FTDM_SPAN_IN_THREAD)) {
		ftdm_sleep(100);
	}

	ftdm_mutex_destroy(&tap->pcalls_lock);
	return FTDM_SUCCESS;
}

#if 0
/**********************************************************************************
 * Fun   : ftdm_sangoma_isup_tap_sig_read()
 *
 * Desc  : It mainly reads the isup tap signalling messages depending up on the
 * 	   channel ID
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isup_tap_sig_read(ftdm_channel_t *ftdmchan, void *data, ftdm_size_t size)
{
	ftdm_status_t status;
	fio_codec_t codec_func;
	ftdm_channel_t *peerchan = ftdmchan->call_data;
	sngisup_tap_t *tap = ftdmchan->span->signal_data;
	ftdm_size_t *chanbuf;
	ftdm_size_t *peerbuf;
	ftdm_size_t *mixedbuf;
	ftdm_size_t i = 0;
	ftdm_size_t sizeread = size;

	if (!data || !size) {
		return FTDM_FAIL;
	}

	chanbuf = ftdm_malloc(sizeof(ftdm_size_t) * size);
	peerbuf = ftdm_malloc(sizeof(ftdm_size_t) * size);
	mixedbuf = ftdm_malloc(sizeof(ftdm_size_t) * size);

	if (!FTDM_IS_VOICE_CHANNEL(ftdmchan) || !ftdmchan->call_data) {
		status =  FTDM_SUCCESS;
		goto done;
	}

	if (tap->mixaudio == ISUP_TAP_MIX_SELF) {
		status = FTDM_SUCCESS;
		goto done;
	}

	if (tap->mixaudio == ISUP_TAP_MIX_PEER) {
		/* start out by clearing the self audio to make sure we don't return audio we were
		 * not supposed to in an error condition  */
		memset(data, FTDM_SILENCE_VALUE(ftdmchan), size);
	}

	if (ftdmchan->native_codec != peerchan->native_codec) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_CRIT, "Invalid peer channel with format %d, ours = %d\n", 
				peerchan->native_codec, ftdmchan->native_codec);
		status =  FTDM_FAIL;
		goto done;
	}

	memcpy(chanbuf, data, size);
	status = peerchan->fio->read(peerchan, peerbuf, &sizeread);
	if (status != FTDM_SUCCESS) {
		ftdm_log_chan_msg(ftdmchan, FTDM_LOG_ERROR, "Failed to read from peer channel!\n");
		status =  FTDM_FAIL;
		goto done;
	}
	if (sizeread != size) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_ERROR, "read from peer channel only %"FTDM_SIZE_FMT" bytes!\n", sizeread);
		status =  FTDM_FAIL;
		goto done;
	}

	if (tap->mixaudio == ISUP_TAP_MIX_PEER) {
		/* only the peer audio is requested */
		memcpy(data, peerbuf, size);
		status = FTDM_SUCCESS;
		goto done;
	}

	codec_func = peerchan->native_codec == FTDM_CODEC_ULAW ? fio_ulaw2slin : peerchan->native_codec == FTDM_CODEC_ALAW ? fio_alaw2slin : NULL;
	if (codec_func) {
		sizeread = size;
		codec_func(chanbuf, sizeof(chanbuf), &sizeread);
		sizeread = size;
		codec_func(peerbuf, sizeof(peerbuf), &sizeread);
	}

	for (i = 0; i < size; i++) {
		mixedbuf[i] = ftdm_saturated_add(chanbuf[i], peerbuf[i]);
	}

	codec_func = peerchan->native_codec == FTDM_CODEC_ULAW ? fio_slin2ulaw : peerchan->native_codec == FTDM_CODEC_ALAW ? fio_slin2alaw : NULL;
	if (codec_func) {
		size = sizeof(mixedbuf);
		codec_func(mixedbuf, size, &size);
	}
	memcpy(data, mixedbuf, size);

	status = FTDM_SUCCESS;

done:
	ftdm_safe_free(chanbuf);
	ftdm_safe_free(peerbuf);
	ftdm_safe_free(mixedbuf);
	return status;
}
#endif

/**********************************************************************************
 * Fun   : ftdm_sangoma_isup_tap_start()
 *
 * Desc  : Start configured sngisup_tap span
 *
 * Ret   : FTDM_FAIL | ret
**********************************************************************************/
static ftdm_status_t ftdm_sangoma_isup_tap_start(ftdm_span_t *span)
{
	ftdm_status_t ret;
	sngisup_tap_t *tap = span->signal_data;
	sngisup_tap_t *peer_tap = tap->peerspan->signal_data;

	if (ftdm_test_flag(tap, ISUP_TAP_RUNNING)) {
		return FTDM_FAIL;
	}

	ftdm_mutex_create(&tap->pcalls_lock);

	ftdm_clear_flag(span, FTDM_SPAN_STOP_THREAD);
	ftdm_clear_flag(span, FTDM_SPAN_IN_THREAD);

	ftdm_set_flag(tap, ISUP_TAP_RUNNING);
	if (peer_tap && ftdm_test_flag(peer_tap, ISUP_TAP_RUNNING)) {
		/* our peer already started, we're the master */
		ftdm_set_flag(tap, ISUP_TAP_MASTER);
	}

	if (!tap->is_voice) {
		ret = ftdm_thread_create_detached(ftdm_sangoma_isup_tap_run, span);
	} else {
		ret = ftdm_thread_create_detached(ftdm_sangoma_isup_voice_span_run, span);
	}

	if (ret != FTDM_SUCCESS) {
		return ret;
	}

	return ret;
}

/**************************************************************************************
 * Fun   : FIO_CONFIGURE_SPAN_SIGNALING_FUNCTION(ftdm_sangoma_isup_tap_configure_span)
 *
 * Desc  : Configure the isup tap span w.r.t the configuration described as in
 * 	   freetdm.conf.xml by the user
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**************************************************************************************/
static FIO_CONFIGURE_SPAN_SIGNALING_FUNCTION(ftdm_sangoma_isup_tap_configure_span)
{
	const char *var, *val;
	const char *debug = NULL;
	sngisup_tap_mix_mode_t mixaudio = ISUP_TAP_MIX_BOTH;
	ftdm_channel_t *dchan = NULL;
	sngisup_tap_t *tap = NULL;
	ftdm_span_t *peerspan = NULL;
	ftdm_span_t *tmp_span = NULL;
	unsigned paramindex = 0;
	int swtch_typ = LSI_SW_ITU2000; 	/* By default switch type will always be ITU 2000 */
	int cic_val_start = 0; 			/* Value from which CIC value is starting for respective span */
	ftdm_bool_t pure_voice = FTDM_FALSE;
	int v_span_id[MAX_VOICE_SPAN];
	int idx = 0;

	memset(v_span_id, 0, MAX_VOICE_SPAN);

	if (span->trunk_type >= FTDM_TRUNK_NONE) {
		ftdm_log(FTDM_LOG_WARNING, "Invalid trunk type '%s' defaulting to T1.\n", ftdm_trunk_type2str(span->trunk_type));
		span->trunk_type = FTDM_TRUNK_T1;
	}

	for (paramindex = 0; ftdm_parameters[paramindex].var; paramindex++) {
		var = ftdm_parameters[paramindex].var;
		val = ftdm_parameters[paramindex].val;
		ftdm_log(FTDM_LOG_DEBUG, "Tapping ISUP key=value, %s=%s\n", var, val);

		if (!strcasecmp(var, "debug")) {
			debug = val;
		} else if (!strcasecmp(var, "mixaudio")) {
			if (ftdm_true(val) || !strcasecmp(val, "both")) {
				ftdm_log(FTDM_LOG_DEBUG, "Setting mix audio mode to 'both' for span %s\n", span->name);
				mixaudio = ISUP_TAP_MIX_BOTH;
			} else if (!strcasecmp(val, "peer")) {
				ftdm_log(FTDM_LOG_DEBUG, "Setting mix audio mode to 'peer' for span %s\n", span->name);
				mixaudio = ISUP_TAP_MIX_PEER;
			} else {
				ftdm_log(FTDM_LOG_DEBUG, "Setting mix audio mode to 'self' for span %s\n", span->name);
				mixaudio = ISUP_TAP_MIX_SELF;
			}
		} else if (!strcasecmp(var, "peerspan")) {
			if (ftdm_span_find_by_name(val, &peerspan) != FTDM_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "Invalid tapping peer span %s\n", val);
				break;
			}
		} else if (!strcasecmp(var, "switch_type")) {
			swtch_typ = ftdm_sangoma_isup_get_switch_type(val);
			if (swtch_typ == -1) {
				ftdm_log(FTDM_LOG_ERROR, "Invalid switch type '%s' for span '%s'\n", val, span->name);
				return FTDM_FAIL;
			}
		} else if (!strcasecmp(var, "cicbase")) {
			cic_val_start = atoi(val);
			if  ((cic_val_start == 0) && (!strcasecmp(val, "0"))) {
				ftdm_log(FTDM_LOG_ERROR, "Invalid Cicbase value %s for span %s. It must be numeric\n", val, span->name);
				return FTDM_FAIL;
			}
		} else if (!strcasecmp(var, "is_voice")) {
			if (!strcasecmp(val, "1")) {
				pure_voice = FTDM_TRUE;
			}
		} else if (!strcasecmp(var, "voice_span")) {
			if (ftdm_get_voice_span(v_span_id, val) != FTDM_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "No voice span for this span %s\n", span->name);
				v_span_id[0] = 0;
			}
		} else {
			ftdm_log(FTDM_LOG_ERROR,  "Unknown isup tapping parameter [%s]", var);
		}
	}

	if ((pure_voice == FTDM_TRUE) && (v_span_id[0] != 0)) {
		ftdm_log(FTDM_LOG_ERROR, "Since %s is a pure voice span is cannot be a signalling span\n", span->name);
		return FTDM_FAIL;
	}

	if (pure_voice != FTDM_TRUE) {
		for (idx = 1; idx <= span->chan_count; idx++) {
			if (span->channels[idx]->type == FTDM_CHAN_TYPE_DQ921) {
				dchan = span->channels[idx];
			}
		}

		if (!dchan) {
			ftdm_log(FTDM_LOG_ERROR, "Could not found the signalling-channel/d-channel !\n");
			return FTDM_FAIL;
		}
	}

	if (!peerspan) {
		ftdm_log(FTDM_LOG_ERROR, "No valid peerspan was specified!\n");
		return FTDM_FAIL;
	}

	if (cic_val_start == 0) {
		ftdm_log(FTDM_LOG_ERROR, "No cicbase was specified!\n");
		return FTDM_FAIL;
	}

	tap = ftdm_calloc(1, sizeof(*tap));
	if (!tap) {
		return FTDM_FAIL;
	}

	tap->debug = parse_debug(debug);
	tap->dchan = dchan;
	tap->peerspan = peerspan;
	tap->mixaudio = mixaudio;
	tap->switch_type = swtch_typ;
	tap->cicbase = cic_val_start;
	tap->is_voice = pure_voice;

	if (pure_voice != FTDM_TRUE) {
		for (idx = 0; idx < 100; idx++) {
			if (v_span_id[idx] == 0) {
				tap->voice_span[idx].span_id = 0;
				tap->voice_span[idx].span = NULL;
				break;
			}

			tap->voice_span[idx].span_id = v_span_id[idx];
			if (ftdm_span_find(v_span_id[idx], &tmp_span) != FTDM_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "No voice span present with span_id = %d\n", v_span_id[idx]);
				return FTDM_FAIL;
			}

			tap->voice_span[idx].span = tmp_span;
		}
	}

	tap->decCb = sng_isup_decoder_getcb();
	if (NULL == tap->decCb) {
		ftdm_log(FTDM_LOG_ERROR, " Failed to get memory for isup decoder control block \n");
		return FTDM_FAIL;
	}
	/* Initialize ISUP Message Decoder */
	if (isup_decoder_init(tap->decCb)) {
		/* Free the allocated memory for isup decoder control block */
		if (tap->decCb) {
			sng_isup_decoder_freecb(tap->decCb);
		}

		ftdm_log(FTDM_LOG_ERROR, " Failed to initialize isup decoder Control Block \n");
		return FTDM_FAIL;
	}

	span->start = ftdm_sangoma_isup_tap_start;
	span->stop = ftdm_sangoma_isup_tap_stop;
	span->sig_read = NULL;
	span->signal_cb = sig_cb;

	span->signal_data = tap;
	span->signal_type = FTDM_SIGTYPE_SS7;
	span->outgoing_call = sangoma_isup_tap_outgoing_call;

	span->get_channel_sig_status = sangoma_isup_tap_get_channel_sig_status;
	span->get_span_sig_status = sangoma_isup_tap_get_span_sig_status;

	span->state_map = &sngisup_tap_state_map;
	span->state_processor = state_advance;

	/* Initialize Decoder lib */
	if (!g_decoder_lib_init) {

		sng_event.sng_log = handle_isup_tapping_log;

		/* Initialize Decoder lib */
		if (sng_isup_decoder_lib_init(&sng_event)) {
			ftdm_log(FTDM_LOG_ERROR, " Failed to initialize decoder lib \n");
			return FTDM_FAIL;
		}

		g_decoder_lib_init = 0x01;
	}

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : sangoma_isup_new_cb()
 *
 * Desc  : Initialize the isup tap(sngisup_t) new control block
 *
 * Ret   : ss7_isup(i.e. ss7_isup strtcture)
**********************************************************************************/
struct sngisup_t *sangoma_isup_new_cb(ftdm_socket_t fd, void *userdata)
{
	struct sngisup_t *sng_ss7;
	static int id = 0;

	id++;
	if (!(sng_ss7 = ftdm_calloc(1, sizeof(*sng_ss7))))
		return NULL;

	sng_ss7->fd = fd;
	sng_ss7->userdata = userdata;
	sng_ss7->cic = 1;	/* By default Cic value is 1 */
	sng_ss7->id = id;

	return sng_ss7;
}

/**********************************************************************************
 * Fun   : sangoma_isup_get_userdata()
 *
 * Desc  : Get user data w.r.t the pased ss7_isup control block
 *
 * Ret   : ss7_isup | NULL
**********************************************************************************/
void *sangoma_isup_get_userdata(struct sngisup_t *ss7_isup)
{
	return ss7_isup ? ss7_isup->userdata : NULL;
}

/**********************************************************************************
 * Fun   : handle_isup_tapping_log()
 *
 * Desc  : prints the LOG depending up on the LOG LEVEL
 *
 * Ret   : void
**********************************************************************************/
void handle_isup_tapping_log(uint8_t level, char *fmt,...)
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
 * Fun   : sangoma_isup_tap_set_debug()
 *
 * Desc  : Set isup_tap debug level
 *
 * Ret   : void
**********************************************************************************/
void sangoma_isup_tap_set_debug(struct sngisup_t *ss7_isup, int debug)
{
	if (!ss7_isup)
		return;
	ss7_isup->debug = debug;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isup_get_chanId()
 *
 * Desc  : Get the the channel ID w.r.t CIC base and cic value
 *
 * Ret   : int (CIC value)
**********************************************************************************/
int ftdm_sangoma_isup_get_chanId(sngisup_tap_t *tap, int cir_val)
{
	if (cir_val > (tap->cicbase + 31)) {
		ftdm_log(FTDM_LOG_DEBUG, "Invalid CIC Value for span %s\n", tap->span->name);
		return 0;
	}

	/* Returns the channel Id depending up on the CIC base configured and the CIC value recieved
	 * in ISUP message */
	if (!(tap->cicbase - 1) % 32) {
		return ((cir_val % 32));
	} else {
		return (cir_val - tap->cicbase + 1);
	}

	return 0;
}

/**********************************************************************************
 * Fun   : ftdm_get_voice_span()
 *
 * Desc  : Get the respective voice span with respect to span_id
 *
 * Ret   : FTDM_SUCCESS | FTDM_FAIL
**********************************************************************************/
int ftdm_get_voice_span(int *v_span_id, const char *val)
{
	int id = 0;
	int idx = 0;
	char *temp = NULL;

	temp = strtok((char *)val, ", ");

	while (temp != NULL) {
		id = atoi(temp);
		if (id == 0) {
			ftdm_log(FTDM_LOG_ERROR, "Invalid span Id for voice span\n");
			return FTDM_FAIL;
		}

		temp = strtok(NULL, ", ");
		v_span_id[idx] = id;
		idx++;
	}
	v_span_id = 0;

	return FTDM_SUCCESS;
}

/**********************************************************************************
 * Fun   : ftdm_sangoma_isup_get_switch_type()
 *
 * Desc  : Get the respective switch type with respect to span
 *
 * Ret   : switch_type | By default switch type remains ETSI
**********************************************************************************/
int ftdm_sangoma_isup_get_switch_type(const char *val)
{
	int switch_type;

	if (!strcasecmp(val, "itu88")) {
		switch_type = LSI_SW_ITU;
	} else if (!strcasecmp(val, "itu92")) {
		switch_type = LSI_SW_ITU;
	} else if (!strcasecmp(val, "itu97")) {
		switch_type = LSI_SW_ITU97;
	} else if (!strcasecmp(val, "itu00")) {
		switch_type = LSI_SW_ITU2000;
	} else if (!strcasecmp(val, "ansi88")) {
		switch_type = LSI_SW_ANS88;
	} else if (!strcasecmp(val, "ansi92")) {
		switch_type = LSI_SW_ANS92;
	} else if (!strcasecmp(val, "ansi95")) {
		switch_type = LSI_SW_ANS95;
	} else if (!strcasecmp(val, "etsiv2")) {
		switch_type = LSI_SW_ETSI;
	} else if (!strcasecmp(val, "etsiv3")) {
		switch_type = LSI_SW_ETSIV3;
	} else if (!strcasecmp(val, "india")) {
		switch_type = LSI_SW_INDIA;
	} else if (!strcasecmp(val, "uk")) {
		switch_type = LSI_SW_UK;
	} else if (!strcasecmp(val, "russia")) {
		switch_type = LSI_SW_RUSSIA;
	} else if (!strcasecmp(val, "china")) {
		switch_type = LSI_SW_CHINA;
	} else {
		switch_type = -1;
	}

	return switch_type;
}

/**********************************************************************************
 * Macro : ftdm_module
 *
 * Desc  : FreeTDM sangoma_isup_tap signaling and IO module definition
 *
 * Ret   : void
 **********************************************************************************/
EX_DECLARE_DATA ftdm_module_t ftdm_module = {
	"sangoma_isup_tap",
	ftdm_sangoma_isup_tap_io_init,
	ftdm_sangoma_isup_tap_unload,
	ftdm_sangoma_isup_tap_init,
	NULL,
	NULL,
	ftdm_sangoma_isup_tap_configure_span,
};


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
