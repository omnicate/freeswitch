/*
 * Copyright (c) 2011 David Yat Sin <dyatsin@sangoma.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms|with or without
 * modification|are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice|this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice|this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES|INCLUDING|BUT NOT
 * LIMITED TO|THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT|INDIRECT|INCIDENTAL|SPECIAL,
 * EXEMPLARY|OR CONSEQUENTIAL DAMAGES (INCLUDING|BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE|DATA|OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY|WHETHER IN CONTRACT|STRICT LIABILITY|OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE|EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"


/* FUNCTIONS ******************************************************************/

uint8_t sngss7_is_mtp2api_enable()
{
    if ( ftmod_ss7_is_operating_mode_pres(SNG_SS7_OPR_MODE_MTP2_API) ) {
		return 1;
	}
	return 0;
}

/******************************************************************************/

static void *ftdm_sangoma_ss7_run_mtp2_api(ftdm_thread_t * me, void *obj)
{
	ftdm_interrupt_t	*ftdm_sangoma_ss7_int[3];
	ftdm_span_t 		*ftdmspan = (ftdm_span_t *) obj;
	ftdm_channel_t 		*ftdmchan = NULL;
	ftdm_event_t 		*event = NULL;
	sngss7_event_data_t	*sngss7_event = NULL;
	sngss7_span_data_t	*sngss7_span = (sngss7_span_data_t *)ftdmspan->signal_data;

	ftdm_log (FTDM_LOG_INFO, "ftmod_sangoma_ss7 monitor thread for span=%u started.\n", ftdmspan->span_id);
	
	/* set IN_THREAD flag so that we know this thread is running */
	ftdm_set_flag (ftdmspan, FTDM_SPAN_IN_THREAD);

	/* get an interrupt queue for this span for channel state changes */
	if (ftdm_queue_get_interrupt (ftdmspan->pendingchans, &ftdm_sangoma_ss7_int[0]) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_CRIT, "Failed to get a ftdm_interrupt for span = %d for channel state changes!\n", ftdmspan->span_id);
		goto ftdm_sangoma_ss7_run_exit;
	}

	/* get an interrupt queue for this span for Trillium events */
	if (ftdm_queue_get_interrupt (sngss7_span->event_queue, &ftdm_sangoma_ss7_int[1]) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_CRIT, "Failed to get a ftdm_interrupt for span = %d for Trillium event queue!\n", ftdmspan->span_id);
		goto ftdm_sangoma_ss7_run_exit;
	}

	/* TODO: Need to add this to ftdm_sangoma_ss7_run_mtp2 as well!! */
	if (ftdm_queue_get_interrupt(ftdmspan->pendingsignals, &ftdm_sangoma_ss7_int[2]) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_CRIT, "%s:Failed to get a signal interrupt for span = %s!\n", ftdmspan->name);
		goto ftdm_sangoma_ss7_run_exit;
	}

	while (ftdm_running () && !(ftdm_test_flag (ftdmspan, FTDM_SPAN_STOP_THREAD))) {

		/* signal the core that sig events are queued for processing */
		ftdm_span_trigger_signals(ftdmspan);

		/* check the channel state queue for an event*/
		switch ((ftdm_interrupt_multiple_wait(ftdm_sangoma_ss7_int, 3, 100))) {
			/**********************************************************************/
			case FTDM_SUCCESS:	/* process all pending state changes */

				/* clean out all pending channel state changes */
				while ((ftdmchan = ftdm_queue_dequeue (ftdmspan->pendingchans))) {
				
					/*first lock the channel */
					ftdm_mutex_lock(ftdmchan->mutex);

					/* process state changes for this channel until they are all done */
					ftdm_channel_advance_states(ftdmchan);
 
					/* unlock the channel */
					ftdm_mutex_unlock (ftdmchan->mutex);
				}/* while ((ftdmchan = ftdm_queue_dequeue(ftdmspan->pendingchans)))  */

				/* clean out all pending stack events */
				while ((sngss7_event = ftdm_queue_dequeue(sngss7_span->event_queue))) {
					ftdm_sangoma_ss7_process_stack_event(sngss7_event);
					ftdm_safe_free(sngss7_event);
				}/* while ((sngss7_event = ftdm_queue_dequeue(ftdmspan->signal_data->event_queue))) */
				break;
				/**********************************************************************/
			case FTDM_TIMEOUT:
				SS7_DEVEL_DEBUG ("ftdm_interrupt_wait timed-out on span = %d\n",ftdmspan->span_id);

				break;
				/**********************************************************************/
			case FTDM_FAIL:
				SS7_ERROR ("ftdm_interrupt_wait returned error!\non span = %d\n", ftdmspan->span_id);

				break;
				/**********************************************************************/
			default:
				SS7_ERROR("ftdm_interrupt_wait returned with unknown code on span = %d\n",ftdmspan->span_id);

				break;
				/**********************************************************************/
		} /* switch ((ftdm_interrupt_wait(ftdm_sangoma_ss7_int, 100))) */
		switch (ftdm_span_poll_event(ftdmspan, 0, NULL)) {
			/**********************************************************************/
			case FTDM_SUCCESS:
				while (ftdm_span_next_event(ftdmspan, &event) == FTDM_SUCCESS);
				break;
				/**********************************************************************/
			case FTDM_TIMEOUT:
				/* No events pending */
				break;
				/**********************************************************************/
			default:
				SS7_ERROR("%s:Failed to poll span event\n", ftdmspan->name);
				/**********************************************************************/
		} /* switch (ftdm_span_poll_event(span, 0)) */
	}
	/* clear the IN_THREAD flag so that we know the thread is done */
	ftdm_clear_flag (ftdmspan, FTDM_SPAN_IN_THREAD);

	ftdm_log (FTDM_LOG_DEBUG, "ftmod_sangoma_ss7 monitor thread for span=%u stopping.\n",ftdmspan->span_id);

	return NULL;

ftdm_sangoma_ss7_run_exit:

	/* clear the IN_THREAD flag so that we know the thread is done */
	ftdm_clear_flag (ftdmspan, FTDM_SPAN_IN_THREAD);

	ftdm_log (FTDM_LOG_INFO,"ftmod_sangoma_ss7 monitor thread for span=%u stopping due to error.\n",ftdmspan->span_id);

	ftdm_sangoma_ss7_stop (ftdmspan);

	return NULL;
}

/******************************************************************************/

ftdm_status_t sngss7_activate_mtp2api(ftdm_span_t * span)
{
	ftdm_iterator_t *chaniter = NULL;
	ftdm_iterator_t *curr = NULL;
	ftdm_channel_t *ftdmchan = NULL;

	chaniter = ftdm_span_get_chan_iterator(span, NULL);
	for (curr = chaniter; curr; curr = ftdm_iterator_next(curr)) {
		ftdmchan = (ftdm_channel_t*)ftdm_iterator_current(curr);

		if (!FTDM_IS_VOICE_CHANNEL(ftdmchan)) {
			ftdm_log_chan_msg(ftdmchan, FTDM_LOG_DEBUG, "Configuring for MTP2 API!\n");
			if (ft_to_sngss7_activate_mtp2(ftdmchan)) {
				ftdm_log(FTDM_LOG_CRIT, "Failed to activate LibSngSS7 MTP2 !\n");
				return FTDM_FAIL;
			}
		}
	}
	ftdm_iterator_free(chaniter);

	/*start the span monitor thread */
	if (ftdm_thread_create_detached (ftdm_sangoma_ss7_run_mtp2_api, span) != FTDM_SUCCESS) {
		ftdm_log(FTDM_LOG_CRIT, "Failed to start Span Monitor Thread!\n");
		return FTDM_FAIL;
	}
	return FTDM_SUCCESS;
}

/******************************************************************************/


ftdm_status_t sngss7_cfg_mtp2api(ftdm_span_t * span)
{
	ftdm_iterator_t *chaniter = NULL;
	ftdm_iterator_t *curr = NULL;
	ftdm_channel_t *ftdmchan = NULL;

	span->sig_write	= ftmod_sangoma_ss7_mtp2_transmit;

	chaniter = ftdm_span_get_chan_iterator(span, NULL);
	for (curr = chaniter; curr; curr = ftdm_iterator_next(curr)) {
		ftdmchan = (ftdm_channel_t*)ftdm_iterator_current(curr);

		if (!FTDM_IS_VOICE_CHANNEL(ftdmchan)) {
			ftdm_log_chan_msg(ftdmchan, FTDM_LOG_DEBUG, "Configuring for MTP2 API!\n");
			if (ft_to_sngss7_cfg_mtp2_api(ftdmchan)) {
				ftdm_log (FTDM_LOG_CRIT, "Failed to configure LibSngSS7 for MTP2 API!\n");
				return FTDM_FAIL;
			}
		}
	}
	ftdm_iterator_free(chaniter);

	return FTDM_SUCCESS;
}

/******************************************************************************/

ftdm_status_t ft_to_sngss7_cfg_mtp2_api(ftdm_channel_t *ftdmchan)
{
	int x = 0;
	uint16_t mtp1Id = 0;
	uint16_t mtp2Id = 0;
	sngss7_chan_data_t *ss7_info = NULL;
	ftdm_sngss7_operating_modes_e opr_mode = SNG_SS7_OPR_MODE_NONE;

	/* check if we have done gen_config already */
	/* TODO: We should perform general configuration on module load */
	if (!(g_ftdm_sngss7_data.gen_config.mtp2api)) {
		if (sng_validate_license(g_ftdm_sngss7_data.cfg.license,
			g_ftdm_sngss7_data.cfg.signature)) {

			ftdm_log(FTDM_LOG_CRIT, "License verification failed..ending!\n");
			return FTDM_FAIL;
		}
			
		/* start up the stack manager */
		if (sng_isup_init_sm()) {
			ftdm_log(FTDM_LOG_CRIT, "Failed to start Stack Manager\n");
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Started Stack Manager!\n");
			sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_SM_STARTED);
		}

		if (sng_isup_init_mtp3_li()) {
			ftdm_log(FTDM_LOG_CRIT, "Failed to start MTP3  Lower interface\n");
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Started MTP3 Lower interface!\n");
		}

		if (ftmod_ss7_mtp3_li_gen_config()) {
			ftdm_log(FTDM_LOG_CRIT, "MTP3 Lower Interface General configuration FAILED!\n");
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "MTP3 Lower Interface General configuration DONE\n");
		}

		if (sng_isup_init_mtp2()) {
			ftdm_log(FTDM_LOG_CRIT, "Failed to start MTP2\n");
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Started MTP2!\n");
			sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP2_STARTED);
		}
		if (sng_isup_init_mtp1()) {
			ftdm_log(FTDM_LOG_CRIT, "Failed to start MTP2\n");
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "Started MTP1!\n");
				sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP1_STARTED);
		}
		if (ftmod_ss7_mtp1_gen_config()) {
			ftdm_log(FTDM_LOG_CRIT, "MTP1 General configuration FAILED!\n");
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "MTP1 General configuration DONE\n");
		}
		if (ftmod_ss7_mtp2_gen_config()) {
			ftdm_log(FTDM_LOG_CRIT, "MTP2 General configuration FAILED!\n");
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "MTP2 General configuration DONE\n");
		}

		/* update the global gen_config so we don't do it again */
		g_ftdm_sngss7_data.gen_config.mtp2api = SNG_GEN_CFG_STATUS_DONE;
	} /* if (!(g_ftdm_sngss7_data.gen_config)) */
	
	for (x = 1; x < MAX_MTP_LINKS; x++) {
		if (g_ftdm_sngss7_data.cfg.mtp1Link[x].id != 0 &&
			g_ftdm_sngss7_data.cfg.mtp1Link[x].ftdmchan == ftdmchan) {

			mtp1Id = x;
			break;
		}
	}

	if (!mtp1Id) {
		ftdm_log_chan_msg(ftdmchan, FTDM_LOG_ERROR, "Failed to find matching MTP1 Link\n");
		return FTDM_FAIL;
	}

	for (x = 1; x < MAX_MTP_LINKS; x++) {
		if (g_ftdm_sngss7_data.cfg.mtp2Link[x].id != 0 &&
			g_ftdm_sngss7_data.cfg.mtp2Link[x].mtp1Id == mtp1Id) {

			mtp2Id = x;
			break;
		}
	}

	if (!mtp2Id) {
		ftdm_log_chan_msg(ftdmchan, FTDM_LOG_ERROR, "Failed to find matching MTP2 Link\n");
		return FTDM_FAIL;
	}

	/* Create a mtp3LiLink with the same ID */
	g_ftdm_sngss7_data.cfg.mtp3LiLink[mtp2Id].id = mtp2Id;
	g_ftdm_sngss7_data.cfg.mtp3LiLink[mtp2Id].mtp2Id = mtp2Id;

#ifdef SD_HSL
	switch(g_ftdm_sngss7_data.cfg.mtp2Link[mtp2Id].sapType) {
		case SNGSS7_SAPTYPE_LSL:
		case SNGSS7_SAPTYPE_HSL:
			g_ftdm_sngss7_data.cfg.mtp1Link[mtp1Id].extSeqNum = 0;
			break;
		case SNGSS7_SAPTYPE_HSL_EXT:
			g_ftdm_sngss7_data.cfg.mtp1Link[mtp1Id].extSeqNum = 1;
			break;
	}
#endif

	if (!(g_ftdm_sngss7_data.cfg.mtp1Link[mtp1Id].flags & SNGSS7_CONFIGURED)) {
		opr_mode = ftmod_ss7_get_operating_mode_by_span_id(ftdmchan->span_id);

		if (ftmod_ss7_mtp1_psap_config(mtp1Id, opr_mode)) {
			ftdm_log(FTDM_LOG_CRIT, "MTP1 PSAP %d configuration FAILED!\n", mtp1Id);
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "MTP1 PSAP %d configuration DONE!\n", mtp1Id);
		}

		/* set the SNGSS7_CONFIGURED flag */
		g_ftdm_sngss7_data.cfg.mtp1Link[mtp1Id].flags |= SNGSS7_CONFIGURED;
	}

	if (!(g_ftdm_sngss7_data.cfg.mtp2Link[mtp2Id].flags & SNGSS7_CONFIGURED)) {
		if (ftmod_ss7_mtp2_dlsap_config(mtp2Id)) {
			ftdm_log(FTDM_LOG_CRIT, "MTP2 DLSAP %d configuration FAILED!\n", mtp2Id);
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "MTP2 DLSAP %d configuration DONE!\n", mtp2Id);
		}
		/* set the SNGSS7_CONFIGURED flag */
		g_ftdm_sngss7_data.cfg.mtp2Link[mtp2Id].flags |= SNGSS7_CONFIGURED;
	}

	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[mtp2Id].flags & SNGSS7_CONFIGURED)) {
		if (ftmod_ss7_mtp3_li_dlsap_config(mtp2Id)) {
			ftdm_log(FTDM_LOG_CRIT, "MTP2 API Interface %d configuration FAILED!\n", mtp2Id);
			return FTDM_FAIL;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "MTP2 API Interface %d configuration DONE!\n", mtp2Id);
		}

		/* set the SNGSS7_CONFIGURED flag */
		g_ftdm_sngss7_data.cfg.mtp3LiLink[mtp2Id].flags |= SNGSS7_CONFIGURED;
		g_ftdm_sngss7_data.cfg.mtp3LiLink[mtp2Id].ftdmchan = ftdmchan;
	}

	ss7_info = ftdm_malloc(sizeof(*ss7_info));
	memset(ss7_info, 0, sizeof(*ss7_info));
	ss7_info->api_data.mtp1_id = mtp1Id;
	ss7_info->api_data.mtp2_id = mtp2Id;
	ss7_info->ftdmchan = ftdmchan;
	ftdmchan->call_data = ss7_info;
	ftdm_set_flag(ftdmchan, FTDM_CHANNEL_DIGITAL_MEDIA);
	
	return FTDM_SUCCESS;
}

/******************************************************************************/
int ft_to_sngss7_activate_mtp2(ftdm_channel_t *ftdmchan)
{
	SuId suId;
	sngss7_chan_data_t *ss7_info;

	ss7_info = (sngss7_chan_data_t*)ftdmchan->call_data;

	suId = ss7_info->api_data.mtp2_id;
	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_ACTIVE)) {
		/* configure mtp3 */
		if ((ftmod_ss7_bind_mtp3lilink(suId))) {
			ftdm_log(FTDM_LOG_CRIT, "MTP3 Lower Interface Bind %d FAILED!\n", suId);
			return 1;;
		} else {
			ftdm_log(FTDM_LOG_DEBUG, "MTP3 Lower Interface Bind %d OK!\n", suId);
		}

		/* set the SNGSS7_CONFIGURED flag */
		g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags |= SNGSS7_ACTIVE;
	}
	return 0;
}

/******************************************************************************/
int ftmod_ss7_mtp3_li_gen_config(void)
{
	ApMngmt	cfg;
	Pst		pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTAP;

	/*clear the configuration structure*/
	memset(&cfg, 0x0, sizeof(ApMngmt));

	/*fill in some general sections of the header*/
	smHdrInit(&cfg.hdr);

	/* fill in the post structure */
	smPstInit(&cfg.t.cfg.s.apGen.sm);

	/*fill in the specific fields of the header*/
	cfg.hdr.msgType					= TCFG;
	cfg.hdr.entId.ent 				= ENTAP;
	cfg.hdr.entId.inst				= S_INST;
	cfg.hdr.elmId.elmnt 			= STGEN;

	cfg.t.cfg.s.apGen.sm.srcEnt		= ENTAP;
	cfg.t.cfg.s.apGen.sm.dstEnt		= ENTSM;

	cfg.t.cfg.s.apGen.nmbDLSap		= MAX_MTP_LINKS;		/* number of MTP Data Link SAPs */
	
	return(sng_cfg_mtp3_li(&pst, &cfg));
}

/******************************************************************************/

int ftmod_ss7_mtp3_li_dlsap_config(int id)
{
	Pst				pst;
	ApMngmt			cfg;
	sng_mtp3_li_link_t	*k = &g_ftdm_sngss7_data.cfg.mtp3LiLink[id];

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTAP;

	/*clear the configuration structure*/
	memset(&cfg, 0x0, sizeof(cfg));

	/*fill in some general sections of the header*/
	smHdrInit(&cfg.hdr);

	/*fill in the specific fields of the header*/
	cfg.hdr.msgType						= TCFG;
	cfg.hdr.entId.ent 					= ENTAP;
	cfg.hdr.entId.inst					= S_INST;
	cfg.hdr.elmId.elmnt 				= STDLSAP;

	cfg.hdr.elmId.elmntInst1 			= k->id;

	cfg.t.cfg.s.apDLSAP.pst.dstEnt			= ENTSD;				/* entity */
	cfg.t.cfg.s.apDLSAP.pst.dstInst			= S_INST;				/* instance */
	cfg.t.cfg.s.apDLSAP.pst.srcInst			= S_INST;				/* instance */
	cfg.t.cfg.s.apDLSAP.pst.dstProcId		= SFndProcId();		/* Process Id */
	cfg.t.cfg.s.apDLSAP.pst.srcProcId		= SFndProcId();		/* Process Id */
	cfg.t.cfg.s.apDLSAP.pst.srcEnt			= ENTAP;
	cfg.t.cfg.s.apDLSAP.pst.prior			= PRIOR0;				/* priority */
	cfg.t.cfg.s.apDLSAP.pst.route			= RTESPEC;				/* route */
	cfg.t.cfg.s.apDLSAP.pst.selector		= 0;					/* lower layer selector */
	cfg.t.cfg.s.apDLSAP.pst.region			= S_REG;				/* memory region id */
	cfg.t.cfg.s.apDLSAP.pst.pool			= S_POOL;				/* memory pool id */
	cfg.t.cfg.s.apDLSAP.spId				= k->mtp2Id;			/* service provider id */
	cfg.t.cfg.s.apDLSAP.suId				= k->id;				/* service user id */
	
	return(sng_cfg_mtp3_li(&pst, &cfg));
}



/******************************************************************************/
int ftmod_ss7_bind_mtp3lilink(uint32_t id)
{
	ApMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTAP;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(cntrl));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTAP;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3LiLink[id].id;

	cntrl.t.cntrl.action		= ABND;		/* Bind */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3_li(&pst, &cntrl));
}

/******************************************************************************/

int ftmod_ss7_unbind_mtp3lilink(uint32_t id)
{
	ApMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTAP;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(cntrl));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTAP;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3LiLink[id].id;

	cntrl.t.cntrl.action		= AUBND_DIS;		/* unbind and disable */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp3_li(&pst, &cntrl));
}

/******************************************************************************/

int ftmod_ss7_activate_mtp3lilink(uint32_t id, Status status)
{
	ApMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTAP;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(cntrl));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTAP;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3LiLink[id].id;

	cntrl.t.cntrl.action		= AENA;		/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;

	cntrl.t.cntrl.status		= status;

	return (sng_cntrl_mtp3_li(&pst, &cntrl));
}

/******************************************************************************/

int ftmod_ss7_deactivate_mtp3lilink(uint32_t id)
{
	ApMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTAP;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(cntrl));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTAP;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3LiLink[id].id;

	cntrl.t.cntrl.action		= ADISIMM;	/* Deactivate */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3_li(&pst, &cntrl));
}

/******************************************************************************/
void handle_sng_mtp3_li_alarm(Pst *pst, ApMngmt *sta)
{
	int	x;
	for (x = 1; x < (MAX_MTP_LINKS+1); x++) {
		if (g_ftdm_sngss7_data.cfg.mtp3Link[x].id == sta->hdr.elmId.elmntInst1) {
			break;
		}
	}
	switch (sta->t.usta.alarm.category) {
		case LCM_CATEGORY_INTERFACE:
			switch (sta->t.usta.alarm.event) {
				/* Event that have no cause */
				case LCM_EVENT_BND_OK:
					ftdm_log(FTDM_LOG_INFO, "[MTP3LI:%d] %s(%d)\n",
									x,
									DECODE_LCM_EVENT(sta->t.usta.alarm.event),
									sta->t.usta.alarm.event);
					break;
				default:
					ftdm_log(FTDM_LOG_INFO, "[MTP3LI:%d] %s(%d) : %s(%d)\n",
									x,
									DECODE_LCM_EVENT(sta->t.usta.alarm.event),
						 			sta->t.usta.alarm.event,
									DECODE_LCM_CAUSE(sta->t.usta.alarm.cause),
									sta->t.usta.alarm.cause);
					break;
			}
			break;
		default:
			ftdm_log(FTDM_LOG_ERROR, "[MTP3LI:%d] %s(%d) : %s(%d)\n",
							x,
							DECODE_LCM_EVENT(sta->t.usta.alarm.event),
							sta->t.usta.alarm.event,
							DECODE_LCM_CAUSE(sta->t.usta.alarm.cause),
							sta->t.usta.alarm.cause);
			break;
	}
	return;
}

/******************************************************************************/

void sngss7_mtp2api_dat_ind(int16_t suId, void *data, int16_t len)
{
	void *mydata;
	ftdm_sigmsg_t	sig;
	ftdm_channel_t *ftdmchan = NULL;	
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

	ftdm_assert(len > 0, "Received data with invalid length\n");

	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_CONFIGURED)) {
		ftdm_log(FTDM_LOG_CRIT, "Received CON CFM on unconfigued MTP3 LI LINK\n");
		SS7_FUNC_TRACE_EXIT(__FUNCTION__);
		return;
	}

	ftdm_assert(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan, "MTP3 LI Link does not have a ftdmchan!!");

	ftdmchan = g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan;

	mydata = ftdm_malloc(len);
	ftdm_assert(mydata, "Failed to allocate memory\n");	
	memcpy(mydata, data, len);

	memset(&sig, 0, sizeof(sig));
	sig.chan_id = ftdmchan->chan_id;
	sig.span_id = ftdmchan->span_id;
	sig.channel = ftdmchan;
	sig.event_id = FTDM_SIGEVENT_IO_INDATA;

	ftdm_sigmsg_set_raw_data(&sig, mydata, len);

	if (ftdm_span_send_signal(ftdmchan->span, &sig) != FTDM_SUCCESS) {
		ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "Failed to send event to user \n");
	}

	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return;
}
/******************************************************************************/

void sngss7_mtp2api_dat_cfm(int16_t suId, void *data, int16_t len, MtpStatus status, SeqU24 credit)
{
	/* Description:
	 * MTP2 invokes this function to return transmitted but unacknowledged data to Layer 3. Layer 3 needs
	 * to send SdUiStdStaReq to retrieve unacknowledged messages from MTP2 during changeover procedure.
	 * 
	 * status = DL_TX_NACK_MORE = more data buffers available
	 * status = DL_TX_NACK_NO_MORE = this is the last data buffer
	 * 
	 * credit = not used by MTP 2
	 */	
	void *mydata;
	ftdm_sigmsg_t	sig;
	ftdm_channel_t *ftdmchan = NULL;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

	ftdm_assert(len > 0, "Received data with invalid length\n");

	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_CONFIGURED)) {
		ftdm_log(FTDM_LOG_CRIT, "Received CON CFM on unconfigued MTP3 LI LINK\n");
		SS7_FUNC_TRACE_EXIT(__FUNCTION__);
		return;
	}

	ftdm_assert(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan, "MTP3 LI Link does not have a ftdmchan!!");

	ftdmchan = g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan;

	mydata = ftdm_malloc(len);
	ftdm_assert(mydata, "Failed to allocate memory\n");	
	memcpy(mydata, data, len);

	memset(&sig, 0, sizeof(sig));
	sig.chan_id = ftdmchan->chan_id;
	sig.span_id = ftdmchan->span_id;
	sig.channel = ftdmchan;
	if (status == DL_TX_NACK_MORE) {
		sig.ev_data.raw_id = SNGSS7_IND_DATA_MORE;
	} else {
		sig.ev_data.raw_id = SNGSS7_IND_DATA_NO_MORE;
	}
	
	sig.event_id = FTDM_SIGEVENT_RAW;

	ftdm_sigmsg_set_raw_data(&sig, mydata, len);

	if (ftdm_span_send_signal(ftdmchan->span, &sig) != FTDM_SUCCESS) {
		ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "Failed to send event to user \n");
	}

	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return;
}

/******************************************************************************/

void sngss7_mtp2api_con_cfm(int16_t suId)
{
	sngss7_chan_data_t *sngss7_info = NULL;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	/* This function informs upper layer that the connection
	 * has succeeded and that the link is now connected */

	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_CONFIGURED)) {
		ftdm_log(FTDM_LOG_CRIT, "Received CON CFM on unconfigued MTP3 LI LINK\n");
		SS7_FUNC_TRACE_EXIT(__FUNCTION__);
		return;
	}

	ftdm_assert(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan, "MTP3 LI Link does not have a ftdmchan!!");
		
	sngss7_info = (sngss7_chan_data_t*)g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan->call_data;
	ftdm_assert(sngss7_info, "Channel does not have link information!\n");
	
	sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);
	
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return;
}

/******************************************************************************/

void sngss7_mtp2api_disc_ind(uint16_t suId, Reason reason)
{
	uint8_t user_reason = 0;
	sngss7_chan_data_t *sngss7_info = NULL;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	/* This function informs upper layer that the MTP2 Layer connection
	* has failed */

	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_CONFIGURED)) {
		ftdm_log(FTDM_LOG_CRIT, "Received DISC IND on unconfigued MTP3 LI LINK\n");
		SS7_FUNC_TRACE_EXIT(__FUNCTION__);
		return;
	}

	ftdm_assert(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan, "MTP3 LI Link does not have a ftdmchan!!");

	sngss7_info = (sngss7_chan_data_t*)g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan->call_data;
	ftdm_assert(sngss7_info, "Channel does not have link information!\n");

	switch (reason) {
		case DL_DISC_SM:
			user_reason = SNGSS7_REASON_SM;
			break;
		case DL_DISC_SUERM:
			user_reason = SNGSS7_REASON_SUERM;
			break;
		case DL_DISC_ACK:
			user_reason = SNGSS7_REASON_ACK;
			break;
		case DL_DISC_TE:
			user_reason = SNGSS7_REASON_TE;
			break;
		case DL_DISC_BSN:
			user_reason = SNGSS7_REASON_BSN;
			break;
		case DL_DISC_FIB:
			user_reason = SNGSS7_REASON_FIB;
			break;
		case DL_DISC_CONG:
			user_reason = SNGSS7_REASON_CONG;
			break;
		case DL_DISC_LSSU_SIOS:
			user_reason = SNGSS7_REASON_LSSU_SIOS;
			break;
		case DL_DISC_TMR2_EXP:
			user_reason = SNGSS7_REASON_TMR2_EXP;
			break;
		case DL_DISC_TMR3_EXP:
			user_reason = SNGSS7_REASON_TMR3_EXP;
			break;
		case DL_DISC_LSSU_SIOS_IAC:
			user_reason = SNGSS7_REASON_LSSU_SIOS_IAC;
			break;
		case DL_DISC_PROV_FAIL:
			user_reason = SNGSS7_REASON_PROV_FAIL;
			break;
		case DL_DISC_TMR1_EXP:
			user_reason = SNGSS7_REASON_TMR1_EXP;
			break;
		case DL_DISC_LSSU_SIN:
			user_reason = SNGSS7_REASON_LSSU_SIN;
			break;
		case DL_DISC_CTS_LOST:
			user_reason = SNGSS7_REASON_CTS_LOST;
			break;
		case DL_DISC_DAT_IN_OOS:
			user_reason = SNGSS7_REASON_DAT_IN_OOS;
			break;
		case DL_DISC_DAT_IN_WAITFLUSHCONT:
			user_reason = SNGSS7_REASON_DAT_IN_WAITFLUSHCONT;
			break;
		case DL_DISC_RETRV_IN_INS:
			user_reason = SNGSS7_REASON_RETRV_IN_INS;
			break;
		case DL_DISC_CON_IN_INS:
			user_reason = SNGSS7_REASON_CON_IN_INS;
			break;
		case DL_DISC_UPPER_SAP_DIS:
			user_reason = SNGSS7_REASON_UPPER_SAP_DIS;
			break;
		default:
			ftdm_log(FTDM_LOG_CRIT, "Unknown DISC reason received: %d\n", reason);
			break;
	}

	/* Max possible value for reason is 27 */
	sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, user_reason);
	
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return;
}

/******************************************************************************/

void sngss7_mtp2api_sta_ind(uint16_t suId, MtpStatus status)
{
	sngss7_chan_data_t *sngss7_info = NULL;
	ftdm_signaling_status_t sigstatus = FTDM_SIG_STATE_DOWN;
	uint8_t user_reason = 0;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	/* Description:
	* MTP2 invokes this function to indicate the status of the remote signalling point.
	*
	* status = DL_REM_PROC_DN = Remote processor down
	* status = DL_REM_PROC_UP = Remote processor up
	*/

	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_CONFIGURED)) {
		ftdm_log(FTDM_LOG_CRIT, "Received DISC IND on unconfigued MTP3 LI LINK\n");
		SS7_FUNC_TRACE_EXIT(__FUNCTION__);
		return;
	}

	ftdm_assert(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan, "MTP3 LI Link does not have a ftdmchan!!");

	sngss7_info = (sngss7_chan_data_t*)g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan->call_data;
	ftdm_assert(sngss7_info, "Channel does not have link information!\n");

	switch(status) {
		case DL_REM_PROC_DN:
			sigstatus = FTDM_SIG_STATE_SUSPENDED;
			user_reason = SNGSS7_REASON_REM_PROC_DN;
			break;
		case DL_REM_PROC_UP:
			sigstatus = FTDM_SIG_STATE_UP;
			user_reason = SNGSS7_REASON_REM_PROC_UP;
			break;

	}
	sngss7_set_sig_status(sngss7_info, sigstatus, user_reason);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return;
}

/******************************************************************************/

void sngss7_mtp2api_sta_cfm(uint16_t suId, Action action, SeqU24 status)
{
	ftdm_channel_t *ftdmchan;
	uint8_t user_reason = 0;
	sngss7_chan_data_t *sngss7_info = NULL;
	ftdm_signaling_status_t sigstatus = FTDM_SIG_STATE_DOWN;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_CONFIGURED)) {
		ftdm_log(FTDM_LOG_CRIT, "Received DISC IND on unconfigued MTP3 LI LINK\n");
		SS7_FUNC_TRACE_EXIT(__FUNCTION__);
		return;
	}

	ftdm_assert(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan, "MTP3 LI Link does not have a ftdmchan!!");
	ftdmchan = g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan;

	sngss7_info = (sngss7_chan_data_t*) ftdmchan->call_data;
	ftdm_assert(sngss7_info, "Channel does not have link information!\n");

	switch(action) {
		case DL_RETRV_BSN:
			{
				uint16_t *mydata;
				ftdm_sigmsg_t	sig;
				
				memset(&sig, 0, sizeof(sig));
				sig.chan_id = ftdmchan->chan_id;
				sig.span_id = ftdmchan->span_id;
				sig.channel = ftdmchan;

				sig.ev_data.raw_id = SNGSS7_IND_RETRV_BSN;
				sig.event_id = FTDM_SIGEVENT_RAW;

				mydata = ftdm_malloc(sizeof(*mydata));

				ftdm_assert(mydata, "Failed to allocate memory");

				*mydata = status;

				ftdm_sigmsg_set_raw_data(&sig, mydata, sizeof(*mydata));

				if (ftdm_span_send_signal(ftdmchan->span, &sig) != FTDM_SUCCESS) {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "Failed to send event to user \n");
				}
			}
			break;
		case DL_RETRV_EMPTY:
			{
				/* User requested to retrieve messages, but no messages in buffer */
				ftdm_sigmsg_t	sig;
				memset(&sig, 0, sizeof(sig));
				sig.chan_id = ftdmchan->chan_id;
				sig.span_id = ftdmchan->span_id;
				sig.channel = ftdmchan;
				sig.ev_data.raw_id = SNGSS7_IND_DATA_EMPTY;

				sig.event_id = FTDM_SIGEVENT_RAW;
				if (ftdm_span_send_signal(ftdmchan->span, &sig) != FTDM_SUCCESS) {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "Failed to send event to user \n");
				}
			}
			break;
		case DL_FLC_STAT:
			switch(status) {
				case DL_STARTFLC:
					sigstatus = FTDM_SIG_STATE_SUSPENDED;
					user_reason = SNGSS7_REASON_START_FLC;
					break;
				case DL_ENDFLC:
					sigstatus = FTDM_SIG_STATE_UP;
					user_reason = SNGSS7_REASON_END_FLC;
					break;
				default:
					ftdm_log_chan(ftdmchan, FTDM_LOG_CRIT, "Received invalid STA CFM FLC status %d\n", status);
					return;
			}
			sngss7_set_sig_status(sngss7_info, sigstatus, user_reason);
			break;
		default:
			ftdm_log_chan(ftdmchan, FTDM_LOG_CRIT, "Received invalid STA CFM action %d\n", action);
			break;

	}
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return;
}

/******************************************************************************/

void sngss7_mtp2api_flc_ind(uint16_t suId, Action action)
{
	sngss7_chan_data_t *sngss7_info = NULL;
	ftdm_signaling_status_t sigstatus = FTDM_SIG_STATE_DOWN;
	uint8_t user_reason = 0;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

	if (!(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].flags & SNGSS7_CONFIGURED)) {
		ftdm_log(FTDM_LOG_CRIT, "Received DISC IND on unconfigued MTP3 LI LINK\n");
		SS7_FUNC_TRACE_EXIT(__FUNCTION__);
		return;
	}

	ftdm_assert(g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan, "MTP3 LI Link does not have a ftdmchan!!");

	sngss7_info = (sngss7_chan_data_t*)g_ftdm_sngss7_data.cfg.mtp3LiLink[suId].ftdmchan->call_data;
	ftdm_assert(sngss7_info, "Channel does not have link information!\n");

	switch(action) {
		case DL_STARTFLC:
			sigstatus = FTDM_SIG_STATE_SUSPENDED;
			user_reason = SNGSS7_REASON_START_FLC;
			break;
		case DL_ENDFLC:
			sigstatus = FTDM_SIG_STATE_UP;
			user_reason = SNGSS7_REASON_END_FLC;
			break;

	}
	sngss7_set_sig_status(sngss7_info, sigstatus, user_reason);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return;
}

/******************************************************************************/

ftdm_status_t ftmod_sangoma_ss7_mtp2_set_sig_status(ftdm_channel_t *ftdmchan, ftdm_signaling_status_t status)
{
	ftdm_status_t ret = FTDM_SUCCESS;
	sngss7_chan_data_t *sngss7_info;
	
	SuId suId;
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	sngss7_info = (sngss7_chan_data_t*) ftdmchan->call_data;
	suId = sngss7_info->api_data.mtp2_id;

	ftdm_assert(ftdmchan->usrmsg, "Channel does not have user data attached\n");
	switch(status) {
		case FTDM_SIG_STATE_UP:
			switch(ftdmchan->usrmsg->raw_id) {
				case SNGSS7_SIGSTATUS_NORM:
					ftmod_ss7_activate_mtp3lilink(suId, DL_CON_L1L2_NORM);
					break;
				case SNGSS7_SIGSTATUS_EMERG:
					ftmod_ss7_activate_mtp3lilink(suId, DL_CON_L1L2_EMRG);
					break;
				case SNGSS7_SIGSTATUS_FLC_STAT:
					sng_ss7_ap_sta_req(suId, DL_FLC_STAT, 0);
					break;
				case SNGSS7_SIGSTATUS_LOC_PROC_UP:
					sng_ss7_ap_sta_req(suId, DL_NO_ACTION, DL_LOC_PROC_UP);
					break;
				default:
					ftdm_log_chan(ftdmchan, FTDM_LOG_ERROR, "Invalid sigstatus ID for UP (%d)\n", ftdmchan->usrmsg->raw_id);
					ret = FTDM_FAIL;
					break;
			}
			break;
		case FTDM_SIG_STATE_SUSPENDED:
			switch(ftdmchan->usrmsg->raw_id) {
				case SNGSS7_SIGSTATUS_LOC_PROC_DN:
					sng_ss7_ap_sta_req(suId, DL_NO_ACTION, DL_LOC_PROC_DN);
					break;
				default:
					ftdm_log_chan(ftdmchan, FTDM_LOG_ERROR, "Invalid sigstatus ID for SUSPEND (%d)\n", ftdmchan->usrmsg->raw_id);
					ret = FTDM_FAIL;
					break;
			}
			break;
		default:			
			ret = FTDM_FAIL;
	}
	
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return ret;
}

/******************************************************************************/

ftdm_status_t ftmod_sangoma_ss7_mtp2_transmit(ftdm_channel_t *ftdmchan, void *data, ftdm_size_t size)
{
	sngss7_chan_data_t *sngss7_info;
	
	SuId suId;
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	sngss7_info = (sngss7_chan_data_t*) ftdmchan->call_data;
	suId = sngss7_info->api_data.mtp2_id;

	sng_ss7_ap_dat_req(suId, data, size);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	return FTDM_BREAK;
}

/******************************************************************************/

/* This function is called by MTP2 User App to request list of unacknowledged frames */
ftdm_status_t ftmod_sangoma_ss7_mtp2_indicate(ftdm_channel_t *ftdmchan)
{
	SuId suId;
	sngss7_chan_data_t *ss7_info;
	Action action = 0;
	MtpStatus status = 0;
	int16_t ret = 0;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

	ss7_info = (sngss7_chan_data_t*)ftdmchan->call_data;	
	suId = ss7_info->api_data.mtp2_id;
	
	ftdm_assert(ftdmchan->usrmsg, "Channel does not have user data attached\n");
	switch(ftdmchan->usrmsg->raw_id) {
		case SNGSS7_REQ_FLUSH_BUFFERS:
			action = DL_NO_ACTION;
			status = DL_FLUSH_BUF;
			break;
		case SNGSS7_REQ_CONTINUE:
			action = DL_NO_ACTION;
			status = DL_CONTINUE;
			break;
		case SNGSS7_REQ_RETRV_BSN:
			action = DL_RETRV_BSN;
			break;
		case SNGSS7_REQ_RETRV_MSG:
			{
				/* BSN is stored in usrmsg */
				ftdm_size_t len;
				void *vdata;
				
				if (ftdm_usrmsg_get_raw_data(ftdmchan->usrmsg, &vdata, &len) != FTDM_SUCCESS) {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "Failed to retrieve user raw data\n");
					return FTDM_FAIL;
				}

				if (len != 2) {
					ftdm_log_chan_msg(ftdmchan, FTDM_LOG_CRIT, "Invalid raw data lengh for BSN\n");
					return FTDM_FAIL;
				}
				action = DL_RETRV_MSG;
				status = *((uint16_t*)vdata);
			}
			break;
		case SNGSS7_REQ_DROP_MSGQ:
			action = DL_DROP_MSGQ;
			break;
	}

	ret = sng_ss7_ap_sta_req(suId, action, status);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
	if (!ret) {
		return FTDM_SUCCESS;
	} else {
		return FTDM_FAIL;
	}
	
}

/******************************************************************************/
