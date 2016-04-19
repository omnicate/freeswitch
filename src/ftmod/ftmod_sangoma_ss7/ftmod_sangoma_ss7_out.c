/*
 * Copyright (c) 2009, Konrad Hammel <konrad@sangoma.com>
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
 * Contributors:
 * 		Kapil Gupta <kgupta@sangoma.com>
 * 		Pushkar Singh <psingh@sangoma.com>
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
#define SNGSS7_EVNTINFO_IND_INBAND_AVAIL 0x03
/******************************************************************************/

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/
void ft_to_sngss7_iam (ftdm_channel_t * ftdmchan)
{
	const char		*var = NULL;
	SiConEvnt		iam;
	ftdm_bool_t 		native_going_up = FTDM_FALSE;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;;
	sngss7_event_data_t 	*event_clone = NULL;
	int			spId = 0;

	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	sngss7_info->suInstId 	= get_unique_id ();
	sngss7_info->spInstId 	= 0;
	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		sngss7_info->spId 	= 1;
	} else {
		sngss7_info->spId       = spId;
	}

	memset (&iam, 0x0, sizeof (iam));

	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {
		ftdm_span_t *peer_span = NULL;
		ftdm_channel_t *peer_chan = NULL;
		sngss7_chan_data_t *peer_info = NULL;

		var = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "sigbridge_peer");
		ftdm_get_channel_from_string(var, &peer_span, &peer_chan);
		if (!peer_chan) {
			SS7_ERROR_CHAN(ftdmchan, "Failed to find sigbridge peer from string '%s'\n", var);
		} else {
			if (peer_span->signal_type != FTDM_SIGTYPE_SS7) {
				SS7_ERROR_CHAN(ftdmchan, "Peer channel '%s' has different signaling type %d'\n", 
						var, peer_span->signal_type);
			} else {
				peer_info = peer_chan->call_data;
				SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Starting native bridge with peer CIC %d\n", 
						sngss7_info->circuit->cic, peer_info->circuit->cic);

				/* retrieve only first message from the others guys queue (must be IAM) */
				event_clone = ftdm_queue_dequeue(peer_info->event_queue);

				/* make each one of us aware of the native bridge */
				peer_info->peer_data = sngss7_info;
				sngss7_info->peer_data = peer_info;

				/* Go to up until release comes, note that state processing is done different and much simpler when there is a peer,
				   We can't go to UP state right away yet though, so do not set the state to UP here, wait until the end of this function
				   because moving from one state to another causes the ftdmchan->usrmsg structure to be wiped 
				   and we still need those variables for further IAM processing */
				native_going_up = FTDM_TRUE;
			}
		}
	}

	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {
		if (!event_clone) {
			SS7_ERROR_CHAN(ftdmchan, "No IAM event clone in peer queue!%s\n", "");
		} else if (event_clone->event_id != SNGSS7_CON_IND_EVENT) {
			/* first message in the queue should ALWAYS be an IAM */
			SS7_ERROR_CHAN(ftdmchan, "Invalid initial peer message type '%d'\n", event_clone->event_id);
		} else {
			ftdm_caller_data_t *caller_data = &ftdmchan->caller_data;

			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx IAM (Bridged, dialing %s)\n", sngss7_info->circuit->cic, caller_data->dnis.digits);

			/* copy original incoming IAM */
			memcpy(&iam, &event_clone->event.siConEvnt, sizeof(iam));


                        if (iam.cdPtyNum.eh.pres == PRSNT_NODEF && iam.cdPtyNum.natAddrInd.pres == PRSNT_NODEF) {
                                const char *val = NULL;
                                val = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_cld_nadi");
                                if (!ftdm_strlen_zero(val)) {
                                        ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Found user supplied Called NADI value \"%s\"\n", val);
                                        iam.cdPtyNum.natAddrInd.val = atoi(val);
                                }
                        }


			/* Change DNIS to whatever was specified, do not change NADI or anything else! */
			copy_tknStr_to_sngss7(caller_data->dnis.digits, &iam.cdPtyNum.addrSig, &iam.cdPtyNum.oddEven);

			/* SPIROU certification hack 
			   If the IAM already contain RDINF, just increment the count and set the RDNIS digits
			   otherwise, honor RDNIS and RDINF stuff coming from the user */
			if (iam.redirInfo.eh.pres == PRSNT_NODEF) {
				const char *val = NULL;
				if (iam.redirInfo.redirCnt.pres) {
					iam.redirInfo.redirCnt.val++;
					SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Tx IAM (Bridged), redirect count incremented = %d\n", sngss7_info->circuit->cic, iam.redirInfo.redirCnt.val);
				}
				val = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_rdnis_digits");
				if (!ftdm_strlen_zero(val)) {
					SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Tx IAM (Bridged), found user supplied RDNIS digits = %s\n", sngss7_info->circuit->cic, val);
					copy_tknStr_to_sngss7((char*)val, &iam.redirgNum.addrSig, &iam.redirgNum.oddEven);
				} else {
					SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Tx IAM (Bridged), not found user supplied RDNIS digits\n", sngss7_info->circuit->cic);
				}
			} else {
				SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Tx IAM (Bridged), redirect info not present, attempting to copy user supplied values\n", sngss7_info->circuit->cic);
				/* Redirecting Number */
				copy_redirgNum_to_sngss7(ftdmchan, &iam.redirgNum);

				/* Redirecting Information */
				copy_redirgInfo_to_sngss7(ftdmchan, &iam.redirInfo);
			}

			if (iam.origCdNum.eh.pres != PRSNT_NODEF) {
				/* Original Called Number */
				copy_ocn_to_sngss7(ftdmchan, &iam.origCdNum);
			}
			copy_access_transport_to_sngss7(ftdmchan, &iam.accTrnspt);
			copy_numPortFwdInfo_to_sngss7(ftdmchan, &iam.numPortFwdInfo);
			copy_hopCounter_to_sngss7(ftdmchan, &iam.hopCounter);
			copy_usr2UsrInfo_to_sngss7(ftdmchan, &iam.usr2UsrInfo);

			if (iam.natFwdCalIndLnk.rci.val) {
				ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "SS7-UK: Native Bridge ss7_fci_lxl_rci[%d]\n",iam.natFwdCalIndLnk.rci.val);
			}

			if (iam.natFwdCalIndLnk.isi.val) {
				ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "SS7-UK: Native Bridge ss7_fci_lxl_isi[%d]\n",iam.natFwdCalIndLnk.isi.val);
			}

		}
	} else if (sngss7_info->circuit->transparent_iam &&
		sngss7_retrieve_iam(ftdmchan, &iam) == FTDM_SUCCESS) {
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx IAM (Transparent)\n", sngss7_info->circuit->cic);

		/* Called Number information */
		copy_cdPtyNum_to_sngss7(ftdmchan, &iam.cdPtyNum, NULL);

		/* Redirecting Number */
		copy_redirgNum_to_sngss7(ftdmchan, &iam.redirgNum);

		/* Redirecting Information */
		copy_redirgInfo_to_sngss7(ftdmchan, &iam.redirInfo);

		/* Location Number information */
		copy_locPtyNum_to_sngss7(ftdmchan, &iam.cgPtyNum1);

		/* Forward Call Indicators */
		copy_fwdCallInd_to_sngss7(ftdmchan, &iam.fwdCallInd);

		/* Original Called Number */
		copy_ocn_to_sngss7(ftdmchan, &iam.origCdNum);

		copy_access_transport_to_sngss7(ftdmchan, &iam.accTrnspt);

		copy_NatureOfConnection_to_sngss7(ftdmchan, &iam.natConInd);
		copy_numPortFwdInfo_to_sngss7(ftdmchan, &iam.numPortFwdInfo);
		copy_hopCounter_to_sngss7(ftdmchan, &iam.hopCounter);
		copy_usr2UsrInfo_to_sngss7(ftdmchan, &iam.usr2UsrInfo);
	} else {
		/* Nature of Connection Indicators */
		copy_natConInd_to_sngss7(ftdmchan, &iam.natConInd);

		/* Forward Call Indicators */
		copy_fwdCallInd_to_sngss7(ftdmchan, &iam.fwdCallInd);

		/* Transmission medium requirements */
		copy_txMedReq_to_sngss7(ftdmchan, &iam.txMedReq);

		if (SNGSS7_SWITCHTYPE_ANSI(g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType)) {
			/* User Service Info A */
			copy_usrServInfoA_to_sngss7(ftdmchan, &iam.usrServInfoA);
		} else {
			/* User Service Info */
			copy_usrServInfoA_to_sngss7(ftdmchan, &iam.usrServInfo);
		}

		/* Fill in User Service Information only if it is set from dialplan */
		var = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_enable_usr_srv_info");
		if (!ftdm_strlen_zero(var)) {
			if (ftdm_true(var)) {
				SS7_INFO_CHAN(ftdmchan,"Setting up User service information for switch %d\n", g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType);
				/* User Service Info */
				copy_usrServInfoA_to_sngss7(ftdmchan, &iam.usrServInfo);
			} else {
				SS7_INFO_CHAN(ftdmchan,"Invalid User service informatin value present for switch %d\n", g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType);
			}
		} else {
			SS7_INFO_CHAN(ftdmchan,"User service information not present for switch %d\n", g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType);
		}
		
		/* Called Number information */
		copy_cdPtyNum_to_sngss7(ftdmchan, &iam.cdPtyNum, NULL);
		/* If flag true then dont fill cgpty into IAM 
		 * Q.785 - 3.6.3 test case*/
		if (is_clip_disable(ftdmchan) == FTDM_FAIL) {
			/* Calling Number information */
			copy_cgPtyNum_to_sngss7(ftdmchan, &iam.cgPtyNum);
		}

		/* chargeNum*/
		copy_chargeNum_to_sngss7(ftdmchan, &iam.chargeNum);

		/* Location Number information */
		copy_locPtyNum_to_sngss7(ftdmchan, &iam.cgPtyNum1);

		/* Generic Number information */
		copy_genNmb_to_sngss7(ftdmchan, &iam.genNmb);

		/* Calling Party's Category */
		copy_cgPtyCat_to_sngss7(ftdmchan, &iam.cgPtyCat);

		/* Redirecting Number */
		copy_redirgNum_to_sngss7(ftdmchan, &iam.redirgNum);

		/* Redirecting Information */
		copy_redirgInfo_to_sngss7(ftdmchan, &iam.redirInfo);

		/* Original Called Number */
		copy_ocn_to_sngss7(ftdmchan, &iam.origCdNum);

		/* Access Transport - old implementation, taking from channel variable of ss7_clg_subaddr */
		copy_accTrnspt_to_sngss7(ftdmchan, &iam.accTrnspt);

#ifdef SS7_UK
		SS7_INFO_CHAN(ftdmchan,"Tx IAM : SwitchType[%d]\n", g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType);
		if (g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType == LSI_SW_UK) {
			copy_divtlineid_to_sngss7(ftdmchan, &iam.lstDvrtLineId);
			copy_nfci_to_sngss7(ftdmchan, &iam.natFwdCalInd);
			copy_presnum_to_sngss7(ftdmchan, &iam.presntNum);
			copy_nflxl_to_sngss7(ftdmchan, &iam.natFwdCalIndLnk);
			/* make sure parameter compatibility should be last one to fill
			 * as it looks for filled parameters to add entry in this IE */
			copy_paramcompatibility_to_sngss7(ftdmchan, &iam, &iam.parmCom);
		}
#endif

		/* Access Transport - taking from channel variable of ss7_access_transport_urlenc.
		    This will overwirte the IE value set be above old implementation.
		*/
		copy_access_transport_to_sngss7(ftdmchan, &iam.accTrnspt);
		copy_NatureOfConnection_to_sngss7(ftdmchan, &iam.natConInd);
		copy_numPortFwdInfo_to_sngss7(ftdmchan, &iam.numPortFwdInfo);
		copy_hopCounter_to_sngss7(ftdmchan, &iam.hopCounter);
		copy_usr2UsrInfo_to_sngss7(ftdmchan, &iam.usr2UsrInfo);

		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx IAM clg = \"%s\" (NADI=%d), cld = \"%s\" (NADI=%d), loc = %s (NADI=%d) (spId=%d)\n",
									sngss7_info->circuit->cic,
									ftdmchan->caller_data.cid_num.digits,
									iam.cgPtyNum.natAddrInd.val,
									ftdmchan->caller_data.dnis.digits,
									iam.cdPtyNum.natAddrInd.val,
									ftdmchan->caller_data.loc.digits,
									iam.cgPtyNum1.natAddrInd.val,
									sngss7_info->spId);
	}

#ifdef ACC_TEST
		if (iam.cgPtyCat.cgPtyCat.val == CAT_PRIOR) {
			sng_increment_acc_statistics(ftdmchan, ACC_IAM_TRANS_PRI_DEBUG);
		} else  {
			sng_increment_acc_statistics(ftdmchan, ACC_IAM_TRANS_DEBUG);
		}
#endif

	sng_cc_con_request (sngss7_info->spId,
						sngss7_info->suInstId,
						sngss7_info->spInstId,
						sngss7_info->circuit->id,
						&iam,
						0);

	if (native_going_up) {
		/* Note that this function (ft_to_sngss7_iam) is run within the main SS7 processing loop in
		   response to the DIALING state handler, we can set the state to UP here and that will
		   implicitly complete the DIALING state, but we *MUST* also advance the state handler
		   right away for a native bridge, otherwise, the processing state function (ftdm_sangoma_ss7_process_state_change)
		   will complete the state without having executed the handler for FTDM_CHANNEL_STATE_UP, and we won't notify
		   the user sending FTDM_SIGEVENT_UP which can cause the application to misbehave (ie, no audio) */
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_UP);
		ftdm_channel_advance_states(ftdmchan);
	}
	
	ftdm_safe_free(event_clone);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

void ft_to_sngss7_inf(ftdm_channel_t *ftdmchan, SiCnStEvnt *inr)
{
	SiCnStEvnt evnt;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&evnt, 0x0, sizeof (evnt));
	
	evnt.infoInd.eh.pres	   = PRSNT_NODEF;
	evnt.infoInd.cgPtyAddrRespInd.pres = PRSNT_NODEF;
	evnt.infoInd.cgPtyCatRespInd.pres = PRSNT_NODEF;

	evnt.infoInd.chrgInfoRespInd.pres =  PRSNT_NODEF;
	evnt.infoInd.chrgInfoRespInd.val = 0;
	evnt.infoInd.solInfoInd.pres = PRSNT_NODEF;
	evnt.infoInd.solInfoInd.val = 0;
	evnt.infoInd.holdProvInd.pres =  PRSNT_NODEF;
	evnt.infoInd.holdProvInd.val = 0;	
	evnt.infoInd.spare.pres =  PRSNT_NODEF;
	evnt.infoInd.spare.val = 0;

	if (inr->infoReqInd.eh.pres == PRSNT_NODEF) {
		if ((inr->infoReqInd.holdingInd.pres ==  PRSNT_NODEF) && (inr->infoReqInd.holdingInd.val == HOLD_REQ)) {
			SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Received INR requesting holding information. Holding is not supported in INF.\n", sngss7_info->circuit->cic);
		}
		if ((inr->infoReqInd.chrgInfoReqInd.pres ==  PRSNT_NODEF) && (inr->infoReqInd.chrgInfoReqInd.val == CHRGINFO_REQ)) {
			SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Received INR requesting charging information. Charging is not supported in INF.\n", sngss7_info->circuit->cic);
		}
		if ((inr->infoReqInd.malCaIdReqInd.pres ==  PRSNT_NODEF) && (inr->infoReqInd.malCaIdReqInd.val == CHRGINFO_REQ)) {
			SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Received INR requesting malicious call id. Malicious call id is not supported in INF.\n", sngss7_info->circuit->cic);
		}

		/* Q.785 - 3.6.3 : Add flag to forcefully avoid filling calling party number i.e. set to CGPRTYADDRESP_NOTAVAIL */
		if (is_clip_disable(ftdmchan) == FTDM_SUCCESS) {
			evnt.infoInd.cgPtyAddrRespInd.val=CGPRTYADDRESP_NOTAVAIL;
		}
		else {
			if ((inr->infoReqInd.cgPtyAdReqInd.pres ==  PRSNT_NODEF) && (inr->infoReqInd.cgPtyAdReqInd.val == CGPRTYADDREQ_REQ)) {
				evnt.infoInd.cgPtyAddrRespInd.val=CGPRTYADDRESP_INCL;
				copy_cgPtyNum_to_sngss7 (ftdmchan, &evnt.cgPtyNum);
			} else {
				evnt.infoInd.cgPtyAddrRespInd.val=CGPRTYADDRESP_NOTINCL;
			}
		}
		
		if ((inr->infoReqInd.cgPtyCatReqInd.pres ==  PRSNT_NODEF) && (inr->infoReqInd.cgPtyCatReqInd.val == CGPRTYCATREQ_REQ)) {
			evnt.infoInd.cgPtyCatRespInd.val = CGPRTYCATRESP_INCL;
			copy_cgPtyCat_to_sngss7 (ftdmchan, &evnt.cgPtyCat);
		} else {
			evnt.infoInd.cgPtyCatRespInd.val = CGPRTYCATRESP_NOTINCL;
		}
	}
	else {
		SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Received INR with no information request. Sending back default INF.\n", sngss7_info->circuit->cic);
	}
		
	sng_cc_inf(spId,
			  sngss7_info->suInstId,
			  sngss7_info->spInstId,
			  sngss7_info->circuit->id, 
			  &evnt, 
			  INFORMATION);

	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx INF (spId = %d)\n", sngss7_info->circuit->cic, spId);
	
}

void ft_to_sngss7_inr(ftdm_channel_t *ftdmchan)
{
	SiCnStEvnt evnt;
	const char *val = NULL;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&evnt, 0x0, sizeof (evnt));

	evnt.infoReqInd.eh.pres	   = PRSNT_NODEF;
	evnt.infoReqInd.cgPtyAdReqInd.pres = PRSNT_NODEF;
	evnt.infoReqInd.cgPtyAdReqInd.val=CGPRTYADDREQ_REQ;

	val = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_inr_holdind_disable");
	if (!ftdm_strlen_zero(val)) {
		evnt.infoReqInd.holdingInd.pres =  NOTPRSNT;
	} else {
		evnt.infoReqInd.holdingInd.pres =  PRSNT_NODEF;
		evnt.infoReqInd.holdingInd.val = HOLD_REQ;
	}

	val = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_inr_cpc_disable");
	if (!ftdm_strlen_zero(val)) {
		evnt.infoReqInd.cgPtyCatReqInd.pres = NOTPRSNT;
	} else {
		evnt.infoReqInd.cgPtyCatReqInd.pres = PRSNT_NODEF;
		evnt.infoReqInd.cgPtyCatReqInd.val = CGPRTYCATREQ_REQ;
	}

	val = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_inr_chrginfo_disable");
	if (!ftdm_strlen_zero(val)) {
		evnt.infoReqInd.chrgInfoReqInd.pres =  NOTPRSNT;
	} else {
		evnt.infoReqInd.chrgInfoReqInd.pres =  PRSNT_NODEF;
		evnt.infoReqInd.chrgInfoReqInd.val = CHRGINFO_REQ;
	}

	val = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_inr_malcalid_disable");
	if (!ftdm_strlen_zero(val)) {
		evnt.infoReqInd.malCaIdReqInd.pres =  NOTPRSNT;
	} else {
		evnt.infoReqInd.malCaIdReqInd.pres =  PRSNT_NODEF;
		evnt.infoReqInd.malCaIdReqInd.val = MLBG_INFOREQ;
	}

	sng_cc_inr(spId,
			  sngss7_info->suInstId,
			  sngss7_info->spInstId,
			  sngss7_info->circuit->id, 
			  &evnt, 
			  INFORMATREQ);

	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx INR (spId= %d)\n", sngss7_info->circuit->cic, spId);
}

void ft_to_sngss7_acm (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	SiCnStEvnt acm;
	const char *backwardInd = NULL;
	const char *isdnUsrPrtInd = NULL;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&acm, 0x0, sizeof (acm));
	
	/* fill in the needed information for the ACM */
	acm.bckCallInd.eh.pres 				= PRSNT_NODEF;
	acm.bckCallInd.chrgInd.pres			= PRSNT_NODEF;
	acm.bckCallInd.chrgInd.val			= CHRG_CHRG;
	acm.bckCallInd.cadPtyStatInd.pres	= PRSNT_NODEF;
	acm.bckCallInd.cadPtyStatInd.val	= 0x01;
	acm.bckCallInd.cadPtyCatInd.pres	= PRSNT_NODEF;
	if (g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType == LSI_SW_UK) {
		acm.bckCallInd.cadPtyCatInd.val		= CADCAT_NOIND;
	} else {
		acm.bckCallInd.cadPtyCatInd.val		= CADCAT_ORDSUBS;
	}
	backwardInd = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "acm_bi_cpc");
	if (!ftdm_strlen_zero(backwardInd)) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Found user supplied backward indicator Called Party Category ACM, value \"%s\"\n", backwardInd);
		/* CPC valid values 0/1/2 , considering only 0/2 as 1 is already assigned by-def */
		if (!strcasecmp(backwardInd, "NULL")) {
			acm.bckCallInd.cadPtyCatInd.val		= CADCAT_NOIND;
		} else if (atoi(backwardInd) == 2 ) {
			acm.bckCallInd.cadPtyCatInd.val		= CADCAT_PAYPHONE; 
		}
	}
	acm.bckCallInd.end2EndMethInd.pres	= PRSNT_NODEF;
	acm.bckCallInd.end2EndMethInd.val	= E2EMTH_NOMETH;
	acm.bckCallInd.intInd.pres			= PRSNT_NODEF;
	if (g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType == LSI_SW_UK) {
		acm.bckCallInd.intInd.val 			= INTIND_INTW; 
	} else {
		acm.bckCallInd.intInd.val 			= INTIND_NOINTW; 
	}
	acm.bckCallInd.end2EndInfoInd.pres	= PRSNT_NODEF;
	acm.bckCallInd.end2EndInfoInd.val	= E2EINF_NOINFO;

	acm.bckCallInd.isdnUsrPrtInd.pres	= PRSNT_NODEF;
	acm.bckCallInd.isdnUsrPrtInd.val	= ISUP_NOTUSED;
	isdnUsrPrtInd = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "acm_bi_isdnuserpart_ind");
	if (!ftdm_strlen_zero(isdnUsrPrtInd)) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Found user supplied backward indicator ISDN User part indicator value \"%s\"\n", isdnUsrPrtInd);
		if (!strcasecmp(isdnUsrPrtInd, "NULL")) {
			acm.bckCallInd.isdnUsrPrtInd.val	= ISUP_NOTUSED;
		} else if (atoi(isdnUsrPrtInd) == 1 ) {
			acm.bckCallInd.isdnUsrPrtInd.val	= ISUP_USED;
		}
	} else {
		acm.bckCallInd.isdnUsrPrtInd.val	= ISUP_USED;
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "User supplied backward indicator ISDN User part indicator NOT FOUND..using default value \"%d\"\n", acm.bckCallInd.isdnUsrPrtInd.val);
	}


	backwardInd = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "acm_bi_iup");
	if (!ftdm_strlen_zero(backwardInd)) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Found user supplied backward indicator ISDN user part indicator ACM, value \"%s\"\n", backwardInd);
		if (atoi(backwardInd) != 0 ) {
			acm.bckCallInd.isdnUsrPrtInd.val	= ISUP_USED;
		}
	}
	acm.bckCallInd.holdInd.pres			= PRSNT_NODEF;
	acm.bckCallInd.holdInd.val			= HOLD_NOTREQD;
	acm.bckCallInd.isdnAccInd.pres		= PRSNT_NODEF;
	acm.bckCallInd.isdnAccInd.val		= ISDNACC_NONISDN;
	acm.bckCallInd.echoCtrlDevInd.pres	= PRSNT_NODEF;
	switch (ftdmchan->caller_data.bearer_capability) {
	/**********************************************************************/
	case (FTDM_BEARER_CAP_SPEECH):
		acm.bckCallInd.echoCtrlDevInd.val	= 0x1;
		break;
	/**********************************************************************/
	case (FTDM_BEARER_CAP_UNRESTRICTED):
		acm.bckCallInd.echoCtrlDevInd.val	= 0x0;
		break;
	/**********************************************************************/
	case (FTDM_BEARER_CAP_3_1KHZ_AUDIO):
		acm.bckCallInd.echoCtrlDevInd.val	= 0x1;
		break;
	/**********************************************************************/
	default:
		SS7_ERROR_CHAN(ftdmchan, "Unknown Bearer capability falling back to speech%s\n", " ");
		acm.bckCallInd.echoCtrlDevInd.val	= 0x1;
		break;
	/**********************************************************************/
	} /* switch (ftdmchan->caller_data.bearer_capability) */


#if 0
	/* for BT making echo control consistent with IAM field */
	acm.bckCallInd.echoCtrlDevInd.val	= sngss7_info->echoControlIncluded;
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx ACM with EchoControl[%d]\n", sngss7_info->circuit->cic, acm.bckCallInd.echoCtrlDevInd.val);
#endif

	acm.bckCallInd.sccpMethInd.pres		= PRSNT_NODEF;
	acm.bckCallInd.sccpMethInd.val		= SCCPMTH_NOIND;

	/* fill in any optional parameters */
	if (sngss7_test_options(&g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id], SNGSS7_ACM_OBCI_BITA)) {
		SS7_DEBUG_CHAN(ftdmchan, "Found ACM_OBCI_BITA flag:0x%X\n", g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].options);
		acm.optBckCalInd.eh.pres				= PRSNT_NODEF;
		acm.optBckCalInd.inbndInfoInd.pres		= PRSNT_NODEF;
		acm.optBckCalInd.inbndInfoInd.val		= 0x1;
		acm.optBckCalInd.caFwdMayOcc.pres		= PRSNT_DEF;
		acm.optBckCalInd.simpleSegmInd.pres		= PRSNT_DEF;
		acm.optBckCalInd.mlppUserInd.pres		= PRSNT_DEF;
		acm.optBckCalInd.usrNetIneractInd.pres	= PRSNT_DEF;
		acm.optBckCalInd.netExcDelInd.pres		= PRSNT_DEF;
	} /* if (sngss7_test_options(isup_intf, SNGSS7_ACM_OBCI_BITA)) */

	/* send the ACM request to LibSngSS7 */
	sng_cc_con_status  (spId,
						sngss7_info->suInstId,
						sngss7_info->spInstId,
						sngss7_info->circuit->id, 
						&acm, 
						ADDRCMPLT);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx ACM (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

void ft_to_sngss7_cpg (ftdm_channel_t *ftdmchan, int indication, int presentation)
{
	SiCnStEvnt cpg;
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&cpg, 0, sizeof (cpg));

	cpg.evntInfo.eh.pres = PRSNT_NODEF;

	cpg.evntInfo.evntInd.pres = PRSNT_NODEF;
	cpg.evntInfo.evntInd.val = indication; /* Event Indicator:  = In-band info is now available */

	
	cpg.evntInfo.evntPresResInd.pres = PRSNT_NODEF;
	cpg.evntInfo.evntPresResInd.val = presentation;		
	
	/* send the CPG request to LibSngSS7 */
	sng_cc_con_status  (spId, sngss7_info->suInstId, sngss7_info->spInstId, sngss7_info->circuit->id, &cpg, PROGRESS);

	ftdm_log_chan(ftdmchan, FTDM_LOG_INFO, "[CIC:%d]Tx CPG (spId = %d)\n", sngss7_info->circuit->cic, spId);
	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

void ft_to_sngss7_anm (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	SiConEvnt anm;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&anm, 0x0, sizeof (anm));
	
	/* send the ANM request to LibSngSS7 */
	sng_cc_con_response(spId,
						sngss7_info->suInstId,
						sngss7_info->spInstId,
						sngss7_info->circuit->id, 
						&anm, 
						5);

  SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx ANM (spId = %d)\n", sngss7_info->circuit->cic, spId);

  SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
  return;
}

/******************************************************************************/
void ft_to_sngss7_rel (ftdm_channel_t * ftdmchan)
{
	const char *loc_ind 	 = NULL;
	const char *rel_cause    = NULL;
	const char *redirect_num = NULL;

	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	SiRelEvnt rel;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&rel, 0x0, sizeof (rel));
	
	rel.causeDgn.eh.pres = PRSNT_NODEF;
	rel.causeDgn.location.pres = PRSNT_NODEF;

	loc_ind = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_rel_loc");
	if (!ftdm_strlen_zero(loc_ind)) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Found user supplied location indicator in REL, value \"%s\"\n", loc_ind);
		rel.causeDgn.location.val = atoi(loc_ind);
	} else {
		rel.causeDgn.location.val = 0x01;
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "No user supplied location indicator in REL, using 0x01\"%s\"\n", "");
	}
	rel_cause = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_rel_cause");
	if (!ftdm_strlen_zero(rel_cause)) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Found user supplied release cause in REL, value \"%s\"\n", rel_cause);
		rel.causeDgn.causeVal.val = atoi(rel_cause);
	} else {
		rel.causeDgn.causeVal.val = (uint8_t) ftdmchan->caller_data.hangup_cause;
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "No user supplied release cause in REL, using \"%d\"\n", ftdmchan->caller_data.hangup_cause);
	}

	redirect_num = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_redirect_number");
	if (!ftdm_strlen_zero(redirect_num)) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Found user supplied re-direct number in REL, value \"%s\"\n", redirect_num);
		copy_cdPtyNum_to_sngss7(ftdmchan, &rel.redirNum, (char*)redirect_num);
	} else {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "No user supplied re-direct number in REL, using 0x01\"%s\"\n", "");
	}

	if (g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].switchType == LSI_SW_UK) {
		/* BT specific location value based on cause as mentioned in BT spec ND0117 */
		rel.causeDgn.location.val = ILOC_NETINTER;

		if ((rel.causeDgn.causeVal.val == 24) ||
				(rel.causeDgn.causeVal.val == 31) ||
				(rel.causeDgn.causeVal.val == 17) ||
				(rel.causeDgn.causeVal.val == 21) ||
				(rel.causeDgn.causeVal.val == 4)) {
			rel.causeDgn.location.val = ILOC_USER;
		}

		if (rel.causeDgn.causeVal.val == 34) {
			rel.causeDgn.location.val = ILOC_TRANNET;
		}
	}

	rel.causeDgn.cdeStand.pres = PRSNT_NODEF;
	rel.causeDgn.cdeStand.val = 0x00;
	rel.causeDgn.recommend.pres = NOTPRSNT;
	rel.causeDgn.causeVal.pres = PRSNT_NODEF;
	rel.causeDgn.dgnVal.pres = NOTPRSNT;
	
	/* send the REL request to LibSngSS7 */
	sng_cc_rel_request (spId,
			sngss7_info->suInstId,
			sngss7_info->spInstId, 
			sngss7_info->circuit->id, 
			&rel);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx REL cause=%d (spId = %d)\n",
							sngss7_info->circuit->cic,
							rel.causeDgn.causeVal.val,
							spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_rlc (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	SiRelEvnt rlc;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&rlc, 0x0, sizeof (rlc));
	
	/* send the RLC request to LibSngSS7 */
	sng_cc_rel_response (spId,
						sngss7_info->suInstId,
						sngss7_info->spInstId, 
						sngss7_info->circuit->id, 
						&rlc);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx RLC (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_rsc (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	sng_cc_sta_request (spId,
				sngss7_info->suInstId,
				sngss7_info->spInstId,
				sngss7_info->circuit->id,
				sngss7_info->globalFlg,
				SIT_STA_CIRRESREQ,
				NULL);

	/* stop RSC retransmission timer */
	if (sngss7_info->t_waiting_rsca.hb_timer_id) {
		SS7_DEBUG_CHAN(ftdmchan, "RSCA Timer %d already running thus stopping that first for CIC %d\n",
			   (int)sngss7_info->t_waiting_rsca.hb_timer_id, sngss7_info->circuit->cic);

		if (ftdm_sched_cancel_timer (sngss7_info->t_waiting_rsca.sched, sngss7_info->t_waiting_rsca.hb_timer_id)) {
			SS7_DEBUG("Unable to Stop RSC re-transmission timer for span %d Circuit %d!\n",
					ftdmchan->span_id, sngss7_info->circuit->cic);
			goto done;
		} else {
			sngss7_info->t_waiting_rsca.hb_timer_id = 0;
		}
	}

	/* start timer of waiting for RSCA message */
	if (ftdm_sched_timer (sngss7_info->t_waiting_rsca.sched,
				"t_waiting_rsca",
				sngss7_info->t_waiting_rsca.beat,
				sngss7_info->t_waiting_rsca.callback,
				&sngss7_info->t_waiting_rsca,
				&sngss7_info->t_waiting_rsca.hb_timer_id))
	{
		SS7_ERROR ("Unable to schedule timer of waiting for RSCA. \n");
	}

done:
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx RSC (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_rsca (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}


	sng_cc_sta_request (spId,
						sngss7_info->suInstId,
						sngss7_info->spInstId,
						sngss7_info->circuit->id,
						sngss7_info->globalFlg, 
						SIT_STA_CIRRESRSP, 
						NULL);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx RSC-RLC (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
  return;
}

/******************************************************************************/
/* sending continuity check */
void ft_to_sngss7_ccr (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	SiStaEvnt ccr;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	/* clean out the gra struct */
	memset (&ccr, 0x0, sizeof (ccr));

	ccr.contInd.eh.pres = PRSNT_NODEF;

	ccr.contInd.contInd.pres = PRSNT_NODEF;
	ccr.contInd.contInd.val = CONT_CHKSUCC;

	sng_cc_sta_request (spId,
			sngss7_info->suInstId,
			sngss7_info->spInstId,
			sngss7_info->circuit->id,
			sngss7_info->globalFlg,
			SIT_STA_CONTCHK,
			&ccr);

	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx CCR (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
/* generate COT and send it to peer */
void ft_to_sngss7_cot (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	SiStaEvnt cot;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	/* clean out the gra struct */
	memset (&cot, 0x0, sizeof (cot));

	cot.contInd.eh.pres = PRSNT_NODEF;

	cot.contInd.contInd.pres = PRSNT_NODEF;
	cot.contInd.contInd.val = CONT_CHKSUCC;

	sng_cc_sta_request (spId,
			sngss7_info->suInstId,
			sngss7_info->spInstId,
			sngss7_info->circuit->id,
			sngss7_info->globalFlg,
			SIT_STA_CONTREP,
			&cot);

	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx CCR (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_blo (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;

	/*
	SS7_INFO_CHAN(ftdmchan, "[CIC:%d]blk_flag = 0x%x, ckt_flag = 0x%x\n, cmd_pending_flag = 0x%x\n", 
					sngss7_info->circuit->cic, sngss7_info->blk_flags, sngss7_info->ckt_flags, sngss7_info->cmd_pending_flags);
	*/
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}


	sngss7_set_cmd_pending_flag(sngss7_info, FLAG_CMD_PENDING_WAIT_FOR_RX_BLA);
	SS7_DEBUG ("Set FLAG_CMD_PENDING_WAIT_FOR_RX_BLA flag. \n");
	
	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_info->circuit->id,
						sngss7_info->globalFlg, 
						SIT_STA_CIRBLOREQ, 
						NULL);
	
	/* start timer of waiting for BLA message */
	if (ftdm_sched_timer (sngss7_info->t_waiting_bla.sched,
					     "t_waiting_bla",
					     sngss7_info->t_waiting_bla.beat,
					     sngss7_info->t_waiting_bla.callback,
					     &sngss7_info->t_waiting_bla,
					     &sngss7_info->t_waiting_bla.hb_timer_id)) 
	{
		SS7_ERROR ("Unable to schedule timer of waiting for BLA. \n");
	}


	sngss7_set_cmd_pending_flag(sngss7_info, FLAG_CMD_UBL_DUMB);
	if (sngss7_info->t_block_ubl.hb_timer_id) {
		ftdm_sched_cancel_timer (sngss7_info->t_block_ubl.sched, sngss7_info->t_waiting_uba.hb_timer_id);
		SS7_DEBUG_CHAN(ftdmchan, "[CIC:%d]Re-schedule disabling UBL transmission timer.\n", sngss7_info->circuit->cic);
	}	
	if (ftdm_sched_timer (sngss7_info->t_block_ubl.sched,
					     "t_block_ubl",
					     sngss7_info->t_block_ubl.beat,
					     sngss7_info->t_block_ubl.callback,
					     &sngss7_info->t_block_ubl,
					     &sngss7_info->t_block_ubl.hb_timer_id)) 
	{
		SS7_ERROR_CHAN(ftdmchan, "[CIC:%d]Unable to schedule timer of disabling UBL transmission.\n", sngss7_info->circuit->cic);
	}

	/*
	SS7_DEBUG_CHAN(ftdmchan, "[CIC:%d]blk_flag = 0x%x, ckt_flag = 0x%x\n, cmd_pending_flag = 0x%x\n", 
					sngss7_info->circuit->cic, sngss7_info->blk_flags, sngss7_info->ckt_flags, sngss7_info->cmd_pending_flags);
	*/
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx BLO (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_bla (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_info->circuit->id,
						sngss7_info->globalFlg, 
						SIT_STA_CIRBLORSP, 
						NULL);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx BLA (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void
ft_to_sngss7_ubl (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	/*
	SS7_INFO_CHAN(ftdmchan, "[CIC:%d]blk_flag = 0x%x, ckt_flag = 0x%x\n, cmd_pending_flag = 0x%x\n", 
					sngss7_info->circuit->cic, sngss7_info->blk_flags, sngss7_info->ckt_flags, sngss7_info->cmd_pending_flags);
	*/

	if (sngss7_test_cmd_pending_flag(sngss7_info, FLAG_CMD_PENDING_WAIT_FOR_RX_BLA) ) {
		sngss7_set_cmd_pending_flag(sngss7_info, FLAG_CMD_PENDING_WAIT_FOR_TX_UBL);
		SS7_DEBUG_CHAN(ftdmchan, "[CIC:%d]Set pending UBL request on Rx BLA.\n",	sngss7_info->circuit->cic);
		SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
		return;
	} 

	if (!sngss7_test_cmd_pending_flag(sngss7_info, FLAG_CMD_UBL_DUMB) ) {
		sng_cc_sta_request (spId,
							0,
							0,
							sngss7_info->circuit->id,
							sngss7_info->globalFlg, 
							SIT_STA_CIRUBLREQ, 
							NULL);
		
		/* start timer of waiting for BLA message */
		if (ftdm_sched_timer (sngss7_info->t_waiting_uba.sched,
						     "t_waiting_uba",
						     sngss7_info->t_waiting_uba.beat,
						     sngss7_info->t_waiting_uba.callback,
						     &sngss7_info->t_waiting_uba,
						     &sngss7_info->t_waiting_uba.hb_timer_id)) 
		{
			SS7_ERROR ("Unable to schedule timer of waiting for BLA. \n");
		}
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx UBL (spId = %d)\n", sngss7_info->circuit->cic, spId);
	} else {
		SS7_DEBUG_CHAN(ftdmchan, "[CIC:%d]UBL not allowed, setting timer for retransmission for spId = %d.\n",	sngss7_info->circuit->cic, spId);
		sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_UNBLK_TX);
	}

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_uba (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_info->circuit->id,
						sngss7_info->globalFlg, 
						SIT_STA_CIRUBLRSP, 
						NULL);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx UBA (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_lpa (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	sng_cc_sta_request (spId,
						sngss7_info->suInstId,
						sngss7_info->spInstId,
						sngss7_info->circuit->id,
						sngss7_info->globalFlg, 
						SIT_STA_LOOPBACKACK, 
						NULL);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx LPA (spId = %d)\n", sngss7_info->circuit->cic, spId);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
return;
}

/******************************************************************************/
void ft_to_sngss7_gra (ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	SiStaEvnt	gra;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	/* clean out the gra struct */
	memset (&gra, 0x0, sizeof (gra));

	gra.rangStat.eh.pres = PRSNT_NODEF;

	/* fill in the range */	
	gra.rangStat.range.pres = PRSNT_NODEF;
	gra.rangStat.range.val = sngss7_info->rx_grs.range;

	/* fill in the status */
	gra.rangStat.status.pres = PRSNT_NODEF;
	gra.rangStat.status.len = ((sngss7_info->rx_grs.range + 1) >> 3) + (((sngss7_info->rx_grs.range + 1) & 0x07) ? 1 : 0); 
	
	/* the status field should be 1 if blocked for maintenace reasons 
	* and 0 is not blocked....since we memset the struct nothing to do
	*/
	
	/* send the GRA to LibSng-SS7 */
	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_info->rx_grs.circuit,
						0,
						SIT_STA_GRSRSP,
						&gra);
	
	SS7_INFO_CHAN(ftdmchan, "[CIC:%d]Tx GRA (%d:%d) (spId = %d)\n",
							sngss7_info->circuit->cic,
							sngss7_info->circuit->cic,
							(sngss7_info->circuit->cic + sngss7_info->rx_grs.range),
							spId);
	

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_grs (ftdm_channel_t *fchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_chan_data_t *cinfo = fchan->call_data;
	
	SiStaEvnt grs;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(fchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	ftdm_assert(sngss7_test_ckt_flag(cinfo, FLAG_GRP_RESET_TX) && 
		   !sngss7_test_ckt_flag(cinfo, FLAG_GRP_RESET_SENT), "Incorrect flags\n");

	memset (&grs, 0x0, sizeof(grs));
	grs.rangStat.eh.pres    = PRSNT_NODEF;
	grs.rangStat.range.pres = PRSNT_NODEF;
	grs.rangStat.range.val  = cinfo->tx_grs.range;

	sng_cc_sta_request (spId,
		0,
		0,
		cinfo->tx_grs.circuit,
		0,
		SIT_STA_GRSREQ,
		&grs);

	SS7_INFO_CHAN(fchan, "[CIC:%d]Tx GRS (%d:%d), (spId = %d)\n",
		cinfo->circuit->cic,
		cinfo->circuit->cic,
		(cinfo->circuit->cic + cinfo->tx_grs.range),
		spId);

	sngss7_set_ckt_flag(cinfo, FLAG_GRP_RESET_SENT);

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
}

/******************************************************************************/
void ft_to_sngss7_cgba(ftdm_channel_t * ftdmchan)
{	
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_span_data_t 	*sngss7_span = ftdmchan->span->signal_data;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	int					x = 0;
	
	SiStaEvnt cgba;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&cgba, 0x0, sizeof(cgba));

	/* fill in the circuit group supervisory message */
	cgba.cgsmti.eh.pres = PRSNT_NODEF;
	cgba.cgsmti.typeInd.pres = PRSNT_NODEF;
	cgba.cgsmti.typeInd.val = sngss7_span->rx_cgb.type;

	cgba.rangStat.eh.pres = PRSNT_NODEF;
	/* fill in the range */	
	cgba.rangStat.range.pres = PRSNT_NODEF;
	cgba.rangStat.range.val = sngss7_span->rx_cgb.range;
	/* fill in the status */
	cgba.rangStat.status.pres = PRSNT_NODEF;
	cgba.rangStat.status.len = ((sngss7_span->rx_cgb.range + 1) >> 3) + (((sngss7_span->rx_cgb.range + 1) & 0x07) ? 1 : 0);
	for(x = 0; x < cgba.rangStat.status.len; x++){
		cgba.rangStat.status.val[x] = sngss7_span->rx_cgb.status[x];
	}

	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_span->rx_cgb.circuit,
						0,
						SIT_STA_CGBRSP,
						&cgba);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx CGBA (%d:%d) (spId = %d)\n",
							sngss7_info->circuit->cic,
							sngss7_info->circuit->cic,
							(sngss7_info->circuit->cic + sngss7_span->rx_cgb.range),
							spId);

	/* clean out the saved data */
	memset(&sngss7_span->rx_cgb, 0x0, sizeof(sngss7_group_data_t));

	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_cgua(ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	
	sngss7_span_data_t 	*sngss7_span = ftdmchan->span->signal_data;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	int					x = 0;
	
	SiStaEvnt cgua;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&cgua, 0x0, sizeof(cgua));

	/* fill in the circuit group supervisory message */
	cgua.cgsmti.eh.pres = PRSNT_NODEF;
	cgua.cgsmti.typeInd.pres = PRSNT_NODEF;
	cgua.cgsmti.typeInd.val = sngss7_span->rx_cgu.type;

	cgua.rangStat.eh.pres = PRSNT_NODEF;
	/* fill in the range */	
	cgua.rangStat.range.pres = PRSNT_NODEF;
	cgua.rangStat.range.val = sngss7_span->rx_cgu.range;
	/* fill in the status */
	cgua.rangStat.status.pres = PRSNT_NODEF;
	cgua.rangStat.status.len = ((sngss7_span->rx_cgu.range + 1) >> 3) + (((sngss7_span->rx_cgu.range + 1) & 0x07) ? 1 : 0);
	for(x = 0; x < cgua.rangStat.status.len; x++){
		cgua.rangStat.status.val[x] = sngss7_span->rx_cgu.status[x];
	}

	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_span->rx_cgu.circuit,
						0,
						SIT_STA_CGURSP,
						&cgua);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx CGUA (%d:%d) (spId = %d)\n",
							sngss7_info->circuit->cic,
							sngss7_info->circuit->cic,
							(sngss7_info->circuit->cic + sngss7_span->rx_cgu.range),
							spId);

	/* clean out the saved data */
	memset(&sngss7_span->rx_cgu, 0x0, sizeof(sngss7_group_data_t));


	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_cgb(ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	sngss7_span_data_t 	*sngss7_span = ftdmchan->span->signal_data;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	SiStaEvnt 			cgb;
	int					x = 0;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&cgb, 0x0, sizeof(cgb));

	/* fill in the circuit group supervisory message */
	cgb.cgsmti.eh.pres			= PRSNT_NODEF;
	cgb.cgsmti.typeInd.pres		= PRSNT_NODEF;
	cgb.cgsmti.typeInd.val		= sngss7_span->tx_cgb.type;

	/* fill in the range */	
	cgb.rangStat.eh.pres 		= PRSNT_NODEF;
	cgb.rangStat.range.pres		= PRSNT_NODEF;
	cgb.rangStat.range.val 		= sngss7_span->tx_cgb.range;

	/* fill in the status */
	cgb.rangStat.status.pres	= PRSNT_NODEF;
	cgb.rangStat.status.len 	= ((sngss7_span->tx_cgb.range + 1) >> 3) + (((sngss7_span->tx_cgb.range + 1) & 0x07) ? 1 : 0);
	for(x = 0; x < cgb.rangStat.status.len; x++){
		cgb.rangStat.status.val[x] = sngss7_span->tx_cgb.status[x];
	}

	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_span->tx_cgb.circuit,
						0,
						SIT_STA_CGBREQ,
						&cgb);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx CGB (%d:%d) (spId = %d)\n",
							sngss7_info->circuit->cic,
							sngss7_info->circuit->cic,
							(sngss7_info->circuit->cic + sngss7_span->tx_cgb.range),
							spId);

	/* clean out the saved data */
	memset(&sngss7_span->tx_cgb, 0x0, sizeof(sngss7_group_data_t));


	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/******************************************************************************/
void ft_to_sngss7_cgu(ftdm_channel_t * ftdmchan)
{
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	sngss7_span_data_t 	*sngss7_span = ftdmchan->span->signal_data;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	SiStaEvnt 			cgu;
	int					x = 0;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	/* fill in the circuit group supervisory message */
	cgu.cgsmti.eh.pres			= PRSNT_NODEF;
	cgu.cgsmti.typeInd.pres		= PRSNT_NODEF;
	cgu.cgsmti.typeInd.val		= sngss7_span->tx_cgu.type;

	/* fill in the range */	
	cgu.rangStat.eh.pres 		= PRSNT_NODEF;
	cgu.rangStat.range.pres		= PRSNT_NODEF;
	cgu.rangStat.range.val 		= sngss7_span->tx_cgu.range;

	/* fill in the status */
	cgu.rangStat.status.pres	= PRSNT_NODEF;
	cgu.rangStat.status.len 	= ((sngss7_span->tx_cgu.range + 1) >> 3) + (((sngss7_span->tx_cgu.range + 1) & 0x07) ? 1 : 0);
	for(x = 0; x < cgu.rangStat.status.len; x++){
		cgu.rangStat.status.val[x] = sngss7_span->tx_cgu.status[x];
	}

	sng_cc_sta_request (spId,
						0,
						0,
						sngss7_span->tx_cgu.circuit,
						0,
						SIT_STA_CGUREQ,
						&cgu);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx CGU (%d:%d) (spId = %d)\n",
							sngss7_info->circuit->cic,
							sngss7_info->circuit->cic,
							(sngss7_info->circuit->cic + sngss7_span->tx_cgu.range),
							spId);

	/* clean out the saved data */
	memset(&sngss7_span->tx_cgu, 0x0, sizeof(sngss7_group_data_t));


	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/* French SPIROU send Charge Unit */
/* No one calls this function yet, but it has been implemented to complement TXA messages */
void ft_to_sngss7_itx (ftdm_channel_t * ftdmchan)
{
#ifndef SANGOMA_SPIROU
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "ITX message not supported!, please update your libsng_ss7\n");
#else
	const char* var = NULL;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	SiCnStEvnt itx;
	int spId = 0;

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	memset (&itx, 0x0, sizeof (itx));

	itx.msgNum.eh.pres = PRSNT_NODEF;
	itx.msgNum.msgNum.pres = PRSNT_NODEF;
	var = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_itx_msg_num");
	if (!ftdm_strlen_zero(var)) {
		itx.msgNum.msgNum.val = atoi(var);
	} else {
		itx.msgNum.msgNum.val = 0x1;
	}
	
	itx.chargUnitNum.eh.pres = PRSNT_NODEF;
	itx.chargUnitNum.chargUnitNum.pres = PRSNT_NODEF;
	var = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_itx_charge_unit");
	if (!ftdm_strlen_zero(var)) {
		itx.chargUnitNum.chargUnitNum.val = atoi(var);
	} else {
		itx.chargUnitNum.chargUnitNum.val = 0x1;
	}

	ftdm_log_chan(ftdmchan, FTDM_LOG_INFO, "ITX Charging Unit:%d Msg Num:%d\n", itx.chargUnitNum.chargUnitNum.val, itx.msgNum.msgNum.val);
	sng_cc_con_status  (spId, sngss7_info->suInstId, sngss7_info->spInstId, sngss7_info->circuit->id, &itx, CHARGE_UNIT);

	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx ITX (spId = %d)\n", sngss7_info->circuit->cic, spId);
#endif
	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
}

/* French SPIROU send Charging Acknowledgement */
void ft_to_sngss7_txa (ftdm_channel_t * ftdmchan)
{	
#ifndef SANGOMA_SPIROU
	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);
	ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "TXA message not supported!, please update your libsng_ss7\n");	
#else
	SiCnStEvnt txa;
	sngss7_chan_data_t	*sngss7_info = ftdmchan->call_data;
	int spId = 0;

	SS7_FUNC_TRACE_ENTER (__FTDM_FUNC__);

	/* Please take out the spId from respective isap if in case not found then assume it as 1,
	 * but please donot simply hardcode it to 1 as it may lead to numerous problems in case of
	 * dynamic reconfiguration */
	spId = ftmod_ss7_get_spId_by_span_id(ftdmchan->span_id);

	if (!spId) {
		spId 	= 1;
	}

	memset (&txa, 0x0, sizeof(txa));

	sng_cc_con_status(spId, sngss7_info->suInstId, sngss7_info->spInstId, sngss7_info->circuit->id, &txa, CHARGE_ACK);
	
	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx TXA (spId = %d)\n", sngss7_info->circuit->cic, spId);
#endif
	SS7_FUNC_TRACE_EXIT (__FTDM_FUNC__);
	return;
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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
/******************************************************************************/
