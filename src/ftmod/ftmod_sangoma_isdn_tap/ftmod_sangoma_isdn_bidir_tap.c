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
 *		 James Zhang <jzhang@sangoma.com>
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
/*                              implementation                                  */
/*                                                                              */
/********************************************************************************/

/**********************************************************************************
 * Fun   : sangoma_isdn_bidir_tap_generate_invite()
 *
 * Desc  : Get channel on which SETUP message is received and then move the channel
 * 	   state to RING to generate respective SIP INVITE
 *
 * Ret   : FTDM_FAIL | FTDM_SUCCESS
**********************************************************************************/
ftdm_status_t sangoma_isdn_bidir_tap_generate_invite(sngisdn_tap_t *tap, passive_call_t  *pcall, int crv)
{
	sngisdn_tap_t  *peertap = tap->peerspan->signal_data;
	ftdm_channel_t *peerfchan = NULL;
	ftdm_status_t  ret = FTDM_FAIL;
	ftdm_channel_t *fchan = NULL;
	ftdm_channel_t *pchan = NULL;

	if (!pcall) {
		ftdm_log(FTDM_LOG_ERROR, "[%s Callref- %d] Invalid pcall received!\n",
			 tap->span->name, crv);
		goto done;
	}

	/* Channel on which SETUP is received */
	fchan = sangoma_isdn_bidir_tap_get_fchan(tap, pcall, pcall->chanId, 1, CALL_DIRECTION_INBOUND, FTDM_TRUE);
	if (!fchan) {
		ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Trying to Open odd/unavailable channel\n",
				tap->span->name, pcall->chanId, crv);
		goto done;
	}
	pcall->chan_pres = FTDM_TRUE;
	pcall->fchan = fchan;

	/* Peer Channel on which PROCEED/CONNECT will be received */
	peerfchan = sangoma_isdn_tap_get_fchan_by_chanid(peertap, pcall->chanId);
	if (!peerfchan) {
		ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] No peer channel available\n",
				tap->span->name, pcall->chanId, crv);
		goto done;
	}

	pchan = peerfchan;
	ftdm_channel_lock(peerfchan);
	if (ftdm_test_flag(peerfchan, FTDM_CHANNEL_INUSE)) {
		reuse_chan_con_recv++;
		ftdm_log_chan(peerfchan, FTDM_LOG_NOTICE,
					  "[%s:%d Callref- %d] Channel is already in use ..state[%s]. Clearing inuse flag!\n",
					  peertap->span->name, pcall->chanId, crv, ftdm_channel_state2str(peerfchan->state));

		if (ftdm_test_flag(pchan, FTDM_CHANNEL_OPEN)) {
			pchan->call_data = NULL;
			pchan->pflags = 0;
			ftdm_channel_close(&pchan);

			ftdm_log_chan(peerfchan, FTDM_LOG_NOTICE,
						  "[%s:%d Callref- %d] Channel moving to down state when SETUP received!\n",
						  peertap->span->name, pcall->chanId, crv, ftdm_channel_state2str(peerfchan->state));
		}
		ftdm_clear_flag(peerfchan, FTDM_CHANNEL_INUSE);
	}
	ftdm_channel_unlock(peerfchan);

	ftdm_channel_lock(fchan);
	/* fchan =A leg on which SETUP is received,
	 * peerfchan = B leg on which PROCEED/CONNECT is received)
	 * move channel state is to DIALING as fchan sent an
	 * INVITE to its peer */
	fchan->call_data = peerfchan;
	ftdm_set_state(fchan, FTDM_CHANNEL_STATE_DIALING);
	ftdm_channel_unlock(fchan);

	/* update span state */
	sangoma_isdn_tap_check_state(tap->span);

	ret = FTDM_SUCCESS;
done:
	return ret;
}

/**********************************************************************************
 * Fun   : sangoma_isdn_bidir_tap_handle_event_con()
 *
 * Desc  : Handle connection and change INBOUND channel state to to UP
 *
 * Ret   : void
**********************************************************************************/
ftdm_status_t sangoma_isdn_bidir_tap_handle_event_con(sngisdn_tap_t *tap, uint8_t msgType, sng_decoder_interface_t *intfCb, int crv)
{
	sngisdn_tap_t   *peertap = tap->peerspan->signal_data;
	char		call_ref[25] = { 0 };
	ftdm_channel_t  *peerfchan = NULL;
	passive_call_t  *peerpcall = NULL;
	con_call_info_t *call_info = NULL;
	char			*crv_key = NULL;
	passive_call_t  *pcall = NULL;
	ftdm_channel_t  *fchan = NULL;

	ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d] %s event receive\n",
		 tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));


	/* check that we already know about this call in the peer PRI (which was the one receiving the PRI_EVENT_RING event) */
	if (!(peerpcall = sangoma_isdn_tap_get_pcall_bycrv(peertap, crv))) {
		/* We have not found peerpcall, that means on A leg we didnt receive any setup
		 * This can happen due to syncronization/timing issue where we process B leg first then A leg */

		/* Check if the same call referece is already present in hash list */
		sprintf(call_ref, "%d", crv);
		if (hashtable_search(peertap->pend_call_lst, (void *)call_ref)) {
			ftdm_log(FTDM_LOG_CRIT, "Call with call referece %d is already inserted in the hash table\n", crv);
			return FTDM_SUCCESS;
		}

		/* Add the call inforamtion in to call hash list based on call reference value if it is not present */
		ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d] Inserting peercall in hash list\n", tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));
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
			ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Donot wait for SETUP as channel ID in present CONNECT message",
				 tap->span->name, intfCb->chan_id, crv);

			peerpcall = sangoma_isdn_tap_create_pcall(peertap, msgType, intfCb, crv);
			if (!peerpcall) {
				ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] Failed to create pcall wait for SETUP message to be received",
					 peertap->span->name, intfCb->chan_id, crv);

				return FTDM_FAIL;
			}

			peerpcall->callref_val = crv;
			peerpcall->chanId = intfCb->chan_id;
			peerpcall->recv_setup = FTDM_FALSE;

			connect_recv_with_chanId++;

			/* Get the channel on which SETUP is received and then generate an Invite */
			if (peerpcall->chan_pres == FTDM_FALSE) {
				ftdm_log(FTDM_LOG_DEBUG,
					 "[%s Callref- %d] Generating SIP INVITE as %s is the first event received for this call \n",
					 peertap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));

				sangoma_isdn_bidir_tap_generate_invite(peertap, peerpcall, crv);
			}

		} else {
			ftdm_log(FTDM_LOG_DEBUG, "[%s Callref- %d] %s event received before SETUP, holding its processing\n",
				 tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));

			ftdm_log(FTDM_LOG_DEBUG,
				 "[%s Callref- %d] Ignoring %s event Waiting for Call SETUP to receive as channel Id is not present in CONNECT message\n",
				 tap->span->name, crv, SNG_DECODE_ISDN_EVENT(msgType));

			return FTDM_FAIL;
		}
	}

	/* at this point we should know the real b chan that will be used and can therefore proceed to notify about the call, but
	 * only if a couple of call tests are passed first */
	ftdm_log(FTDM_LOG_DEBUG, "[%s:%d Callref- %d] %s event receive\n",
		 tap->span->name, peerpcall->chanId, crv, SNG_DECODE_ISDN_EVENT(msgType));

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
			ftdm_log(FTDM_LOG_DEBUG, "Setting chanId as %d for %s event received on Span %s from its peer\n",
				 pcall->chanId, SNG_DECODE_ISDN_EVENT(msgType), tap->span->name);
		} else if (intfCb->chan_id) {
			pcall->chanId = intfCb->chan_id;
			ftdm_log(FTDM_LOG_DEBUG, "Setting chanId as %d for %s event received on Span %s\n",
				 pcall->chanId, SNG_DECODE_ISDN_EVENT(msgType), tap->span->name);
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Invalid chanId as %d for %s event received on Span %s\n",
				 intfCb->chan_id, SNG_DECODE_ISDN_EVENT(msgType), tap->span->name);
			return FTDM_FAIL;
		}
	}

	if (peerpcall->chan_pres == FTDM_FALSE) {
		ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Invalid channel present\n",
			 peertap->span->name, peerpcall->chanId, crv);

		return FTDM_FAIL;
	}

	pcall->recv_conn = FTDM_TRUE;

	/* pcall must have the channel at this point */
	fchan = sangoma_isdn_bidir_tap_get_fchan(tap, pcall, pcall->chanId, 1, CALL_DIRECTION_NONE, FTDM_FALSE);
	if (!fchan) {
		ftdm_log(FTDM_LOG_ERROR, "[%s:%d Callref- %d] Connect requested on odd/unavailable\n",
			 tap->span->name, pcall->chanId, crv);
		return FTDM_FAIL;
	}
	peerfchan = peerpcall->fchan;
	pcall->fchan = fchan;

	ftdm_channel_lock(fchan);
	fchan->call_data = peerfchan; /* fchan=B leg, pointing to peerfchan(A leg) */
	ftdm_channel_unlock(fchan);

	ftdm_channel_lock(peerfchan);
	peerfchan->call_data = fchan; /* peerfchan=A leg(SETUP received), pointing to fchan(B leg) */
	ftdm_channel_unlock(peerfchan);

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

	/* Once Call is answered then only change channel state to UP */
	ftdm_channel_lock(pcall->fchan);
	ftdm_log_chan(pcall->fchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] Tap call answered in state %s\n",
		          tap->span->name, pcall->chanId, crv, ftdm_channel_state2str(pcall->fchan->state));

	ftdm_set_pflag(pcall->fchan, ISDN_TAP_NETWORK_ANSWER);
	if(ftdm_test_flag(pcall->fchan, FTDM_CHANNEL_OUTBOUND)) {
		if (ftdm_channel_get_state(pcall->fchan) == FTDM_CHANNEL_STATE_RINGING) {
			ftdm_set_state(pcall->fchan, FTDM_CHANNEL_STATE_UP);
		} else {
			early_connect_recv++;
			ftdm_log_chan(fchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] Waiting to change channel state to %s\n",
				      tap->span->name, pcall->chanId, crv, ftdm_channel_state2str(fchan->state));
		}
	} else {
			ftdm_log_chan(fchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] Waiting to receive 200 OK to move channel state to UP\n",
				      tap->span->name, pcall->chanId, crv);
	}
	ftdm_channel_unlock(pcall->fchan);

	/* update span state */
	sangoma_isdn_tap_check_state(tap->span);

	ftdm_channel_lock(peerfchan);
	ftdm_log_chan(peerfchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] PeerTap call was answered in state %s\n",
		      peertap->span->name, peerpcall->chanId, crv, ftdm_channel_state2str(peerfchan->state));

	ftdm_set_pflag(peerfchan, ISDN_TAP_NETWORK_ANSWER);

	if(ftdm_test_flag(peerfchan, FTDM_CHANNEL_OUTBOUND)) {
		peerpcall->recv_conn = FTDM_TRUE;

		if (ftdm_channel_get_state(peerpcall->fchan) ==  FTDM_CHANNEL_STATE_RINGING) {
			ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_UP);
		} else {
			early_connect_recv++;
			ftdm_log_chan(peerfchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] Waiting to change channel state to %s\n",
				      peertap->span->name, peerpcall->chanId, crv, ftdm_channel_state2str(peerfchan->state));
		}
	} else {
			ftdm_log_chan(peerfchan, FTDM_LOG_INFO, "[%s:%d Callref- %d] Waiting to receive 200 OK to move channel state to UP\n",
						  peertap->span->name, peerpcall->chanId, crv);
	}
	ftdm_channel_unlock(peerfchan);

	/* update peer span state */
	sangoma_isdn_tap_check_state(peertap->span);

	return FTDM_SUCCESS;

}

/**********************************************************************************
 * Fun   : sangoma_isdn_bidir_tap_handle_event_rel()
 *
 * Desc  : Handle call release/disconnect event and move channel to down state
 *
 * Ret   : FTDM_SUCCESS
**********************************************************************************/
ftdm_status_t sangoma_isdn_bidir_tap_handle_event_rel(sngisdn_tap_t *tap, uint8_t msgType, int crv)
{
	sngisdn_tap_t *peertap = tap->peerspan->signal_data;
	ftdm_channel_t *peerfchan = NULL;
	passive_call_t *peerpcall = NULL;
	passive_call_t *pcall = NULL;
	ftdm_channel_t *fchan = NULL;

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

	/* Get pcall and clear its data if pcall fchannel is not present the please, go for
	 * peerpcall and move channel state to DOWN/TERMINATING */
	if (!(pcall = sangoma_isdn_tap_get_pcall_bycrv(tap, crv))) {
		ftdm_log(FTDM_LOG_DEBUG,
				"[%s Callref- %d] Ignoring release/disconnect since we don't know about it\n",
				tap->span->name, crv);
		goto end;
	}

	if (pcall->fchan) {
		fchan = pcall->fchan;

		ftdm_channel_lock(fchan);
		if (fchan->last_state == FTDM_CHANNEL_STATE_HANGUP) {
			ftdm_set_state(fchan, FTDM_CHANNEL_STATE_DOWN);

			ftdm_log_chan(fchan,FTDM_LOG_DEBUG,"[%s:%d Callref- %d] Changing channel state to DOWN\n",
				      tap->span->name, fchan->physical_chan_id, crv);
		} else if ((fchan->state < FTDM_CHANNEL_STATE_TERMINATING) && (fchan->state != FTDM_CHANNEL_STATE_DOWN)) {
			ftdm_set_state(fchan, FTDM_CHANNEL_STATE_TERMINATING);

			ftdm_log_chan(fchan,FTDM_LOG_DEBUG,"[%s:%d Callref- %d] Changing channel state to TERMINATING\n",
				      tap->span->name, fchan->physical_chan_id, crv);
		}
		ftdm_channel_unlock(fchan);

		/* update span state */
		sangoma_isdn_tap_check_state(tap->span);
		goto end;
	}

	if (!(peerpcall = sangoma_isdn_tap_get_pcall_bycrv(peertap, crv))) {
		ftdm_log(FTDM_LOG_DEBUG,
			 "[%s Callref- %d] Ignoring release/disconnect since we don't know about it\n",
			 peertap->span->name, crv);
		goto end;
	}

	if (peerpcall->fchan) {
		peerfchan = peerpcall->fchan;

		ftdm_channel_lock(peerfchan);
		if (peerfchan->last_state == FTDM_CHANNEL_STATE_HANGUP) {
			ftdm_set_state(peerfchan, FTDM_CHANNEL_STATE_DOWN);

			ftdm_log_chan(peerfchan,FTDM_LOG_DEBUG,"[%s:%d Callref- %d] Changing peer channel state to DOWN\n",
				      peertap->span->name, peerfchan->physical_chan_id, crv);
		} else if ((peerfchan->state < FTDM_CHANNEL_STATE_TERMINATING) && (peerfchan->state != FTDM_CHANNEL_STATE_DOWN)) {
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
 * Fun   : sangoma_isdn_bidir_tap_get_fchan()
 *
 * Desc  : Get pcall channel
 *
 * Ret   : fchan | NULL
**********************************************************************************/
ftdm_channel_t *sangoma_isdn_bidir_tap_get_fchan(sngisdn_tap_t *tap, passive_call_t *pcall, int chan_id, int force_clear_chan, tap_call_direction_t call_dir, ftdm_bool_t to_open)
{
	ftdm_channel_t *fchan = NULL;
	int err = 0;

	fchan = sangoma_isdn_tap_get_fchan_by_chanid(tap, chan_id);
	if (!fchan) {
		err = 1;
		goto done;
	}

	ftdm_channel_lock(fchan);

	if (to_open == FTDM_TRUE) {
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

		/* check if the channel is in INVALID state then please change the state */
		if ((fchan->state != FTDM_CHANNEL_STATE_DOWN) && (fchan->state < FTDM_CHANNEL_STATE_TERMINATING)) {
			ftdm_log_chan(fchan, FTDM_LOG_INFO, "Bi-directional tap Channel in state [%s]. Changing state to TERMINATING!\n", ftdm_channel_state2str(fchan->state));
			ftdm_set_state(fchan, FTDM_CHANNEL_STATE_TERMINATING);
		} else if (fchan->state == FTDM_CHANNEL_STATE_HANGUP) {
			ftdm_log_chan_msg(fchan, FTDM_LOG_INFO, "Bi-directional tap Channel is in HANGUP state please move channel to DOWN state\n");
			ftdm_set_state(fchan, FTDM_CHANNEL_STATE_DOWN);
		}

		if (fchan->state != FTDM_CHANNEL_STATE_DOWN) {
			/* unlock channel here */
			ftdm_channel_unlock(fchan);
			/* update channel_state */
			sangoma_isdn_tap_check_state(tap->span);

			/* wait until channel moves to DOWN state in order to process
			 * INVITE properly as if after INVITE peer channel moves to
			 * DOWN state then it may lead to loss of SIP Call */
			while (fchan->state != FTDM_CHANNEL_STATE_DOWN) {
				/* until channel moves to down state */
			}
			ftdm_channel_lock(fchan);
		}

		if (ftdm_channel_open_chan(fchan) != FTDM_SUCCESS) {
			ftdm_log_chan(fchan, FTDM_LOG_ERROR, "Could not open tap channel %s\n","");
			err = 1;
			goto done;
		}
		ftdm_log_chan(fchan, FTDM_LOG_DEBUG, "Tap channel openned successfully \n %s","");
	}

	memset(fchan->caller_data.cid_num.digits, 0, sizeof(fchan->caller_data.cid_num.digits));
	memset(fchan->caller_data.dnis.digits, 0, sizeof(fchan->caller_data.dnis.digits));
	memset(fchan->caller_data.cid_name, 0, sizeof(fchan->caller_data.cid_name));
	fchan->caller_data.call_reference = 0;

	ftdm_set_string(fchan->caller_data.cid_num.digits, pcall->callingnum.digits);
	ftdm_set_string(fchan->caller_data.cid_name, pcall->callingnum.digits);
	ftdm_set_string(fchan->caller_data.dnis.digits, pcall->callednum.digits);
	fchan->caller_data.call_reference = pcall->callref_val;

	if (call_dir == CALL_DIRECTION_INBOUND) {
		ftdm_clear_flag(fchan, FTDM_CHANNEL_OUTBOUND);
	} if (call_dir == CALL_DIRECTION_OUTBOUND) {
		ftdm_set_flag(fchan, FTDM_CHANNEL_OUTBOUND);
	}

done:
	if (fchan) {
		ftdm_channel_unlock(fchan);
	}

	if (err) {
		return NULL;
	}

	return fchan;
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
