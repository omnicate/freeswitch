/*
 * Copyright (c) 2009, Sangoma Technologies
 * Konrad Hammel <konrad@sangoma.com>
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
 * 		Moises Silva <moy@sangoma.com>
 * 		Kapil Gupta <kgupta@sangoma.com>
 * 		Pushkar Singh <psingh@sangoma.com>
 *
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/

/* GLOBALS ********************************************************************/

/******************************************************************************/

/* PROTOTYPES *****************************************************************/

/******************************************************************************/

/* FUNCTIONS ******************************************************************/
void handle_isup_t35(void *userdata)
{
    SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);

    sngss7_timer_data_t *timer = userdata;
    sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
    ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;

    /* now that we have the right channel...put a lock on it so no-one else can use it */
    ftdm_channel_lock(ftdmchan);

    /* Q.764 2.2.5 Address incomplete (T35 expiry action is hangup with cause 28 according to Table A.1/Q.764) */
    SS7_ERROR("[Call-Control] Timer 35 expired on CIC = %d\n", sngss7_info->circuit->cic);

    /* set the flag to indicate this hangup is started from the local side */
    sngss7_set_ckt_flag(sngss7_info, FLAG_LOCAL_REL);

    /* hang up on timer expiry */
    ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_INVALID_NUMBER_FORMAT;

    /* end the call */
    ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_CANCEL);

    /* kill t10 t39 if active */
    if (sngss7_info->t10.hb_timer_id) {
        ftdm_sched_cancel_timer (sngss7_info->t10.sched, sngss7_info->t10.hb_timer_id);
    }
    if (sngss7_info->t39.hb_timer_id) {
        ftdm_sched_cancel_timer (sngss7_info->t39.sched, sngss7_info->t39.hb_timer_id);
    }

    /*unlock*/
    ftdm_channel_unlock(ftdmchan);

    SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
    return;
}


void handle_isup_t10(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);

	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;

	ftdm_channel_lock(ftdmchan);

	SS7_DEBUG("[Call-Control] Timer 10 expired on CIC = %d\n", sngss7_info->circuit->cic);

	/* send the call to the user */
	ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RING);

	ftdm_channel_unlock(ftdmchan);

	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}

void handle_isup_t39(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);

	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;

	/* now that we have the right channel...put a lock on it so no-one else can use it */
	ftdm_channel_lock(ftdmchan);

	/* Q.764 2.2.5 Address incomplete (T35 expiry action is hangup with cause 28 according to Table A.1/Q.764) */
	SS7_ERROR("[Call-Control] Timer 39 expired on CIC = %d\n", sngss7_info->circuit->cic);

	/* set the flag to indicate this hangup is started from the local side */
	sngss7_set_ckt_flag(sngss7_info, FLAG_LOCAL_REL);

	/* hang up on timer expiry */
	ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_INVALID_NUMBER_FORMAT;

	/* end the call */
	ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_CANCEL);

	/* kill t10 t35 if active */
	if (sngss7_info->t10.hb_timer_id) {
		ftdm_sched_cancel_timer (sngss7_info->t10.sched, sngss7_info->t10.hb_timer_id);
	}
	if (sngss7_info->t35.hb_timer_id) {
		ftdm_sched_cancel_timer (sngss7_info->t35.sched, sngss7_info->t35.hb_timer_id);
	}

	/*unlock*/
	ftdm_channel_unlock(ftdmchan);

	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}

/* Changes w.r.t. ACC feature */
void handle_route_t29(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);

	sng_acc_tmr_t *timer_data = userdata;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = timer_data->sngss7_rmt_cong;

	if (!sngss7_rmt_cong) {
		SS7_DEBUG("NSG-ACC: Invalid User Data on T29 timer expiry\n");
		goto end;
	}

	if (sngss7_rmt_cong->t29.tmr_id) {
		sngss7_rmt_cong->t29.tmr_id = 0;
		SS7_DEBUG("NSG-ACC: Changing T29 Timer-Id to 0 on timer expiry\n");
	}

	ftdm_mutex_lock(sngss7_rmt_cong->mutex);
	sngss7_rmt_cong->calls_allowed = 0;
	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

	SS7_DEBUG("NSG-ACC: Timer 29 expired for DPC[%d]. Thus, resetting active calls.\n", sngss7_rmt_cong->dpc);

end:
	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}

void handle_route_t30(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);

	sng_acc_tmr_t *timer_data = userdata;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = timer_data->sngss7_rmt_cong;

	if (!sngss7_rmt_cong) {
		SS7_DEBUG("NSG-ACC: Invalid User Data on T30 timer expiry\n");
		goto end;
	}

	SS7_DEBUG("NSG-ACC: Timer 30 expired for DPC[%d].....\n", sngss7_rmt_cong->dpc);

	if (sngss7_rmt_cong->t30.tmr_id) {
		sngss7_rmt_cong->t30.tmr_id = 0;
		SS7_DEBUG("NSG-ACC: Changing T30 Timer-Id to 0 on timer expiry\n");
	}

	if (sngss7_rmt_cong->sngss7_rmtCongLvl) {
		ftdm_mutex_lock(sngss7_rmt_cong->mutex);
		if (sngss7_rmt_cong->sngss7_rmtCongLvl == 2) {
			sngss7_rmt_cong->sngss7_rmtCongLvl--;
			sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate;
		} else {
			if ((sngss7_rmt_cong->call_blk_rate - g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate) > 0) {
				sngss7_rmt_cong->call_blk_rate = sngss7_rmt_cong->call_blk_rate - g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate;
			} else {
				SS7_DEBUG("NSG-ACC: Congestion Ended for dpc[%d] with Total number of call rejected[%d] and calls passed[%d]\n", sngss7_rmt_cong->dpc, sngss7_rmt_cong->calls_rejected, sngss7_rmt_cong->calls_passed);
				sngss7_rmt_cong->call_blk_rate = 0;
				sngss7_rmt_cong->calls_allowed = 0;
				sngss7_rmt_cong->calls_passed = 0;
				sngss7_rmt_cong->calls_rejected = 0;
				sngss7_rmt_cong->loc_calls_rejected = 0;
				/* changes ends */
				sngss7_rmt_cong->sngss7_rmtCongLvl = 0;
				sng_acc_free_active_calls_hashlist(sngss7_rmt_cong);
				ftdm_mutex_unlock(sngss7_rmt_cong->mutex);
				/* No need to run Timer as we have released all congestion resources */
				/* We are not happy to handle traffic */
				goto end;
			}
		}

		ftdm_mutex_unlock(sngss7_rmt_cong->mutex);
		/* Restart Timer 29 & Timer 30 */
		SS7_DEBUG("NSG-ACC: Restarting T-29 & T-30 timers as still DPC[%d] has congestion level of [%d]\n", sngss7_rmt_cong->dpc, sngss7_rmt_cong->sngss7_rmtCongLvl);
		/* This should never happen but in order to handle error properly check is required */
		if (sngss7_rmt_cong->t29.tmr_id) {
			SS7_DEBUG("NSG-ACC: Starting T29 Timer %d is already running donot start it\n", sngss7_rmt_cong->t29.tmr_id);
			goto start_t30;
		}

		if (ftdm_sched_timer (sngss7_rmt_cong->t29.tmr_sched,
					"t29",
					sngss7_rmt_cong->t29.beat,
					sngss7_rmt_cong->t29.callback,
					&sngss7_rmt_cong->t29,
					&sngss7_rmt_cong->t29.tmr_id)) {
			SS7_ERROR ("NSG-ACC: Unable to schedule timer T29\n");
			goto end;
		} else {
			SS7_DEBUG("NSG-ACC: T29 Timer is re-started for [%d]msec with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t29.beat, sngss7_rmt_cong->t29.tmr_id, sngss7_rmt_cong->dpc);
		}

start_t30:
		/* This should never happen but in order to handle error properly check is required */
		if (sngss7_rmt_cong->t30.tmr_id) {
			SS7_DEBUG("NSG-ACC: Starting T30 Timer %d is already running donot start it\n", sngss7_rmt_cong->t30.tmr_id);
			goto end;
		}
		if (ftdm_sched_timer (sngss7_rmt_cong->t30.tmr_sched,
					"t30",
					sngss7_rmt_cong->t30.beat,
					sngss7_rmt_cong->t30.callback,
					&sngss7_rmt_cong->t30,
					&sngss7_rmt_cong->t30.tmr_id)) {
			SS7_ERROR ("NSG-ACC: Unable to schedule timer T30. Thus, stopping T29 Timer also.\n");

			/* Kill t29 timer if active */
			if (sngss7_rmt_cong->t29.tmr_id) {
				if (ftdm_sched_cancel_timer (sngss7_rmt_cong->t29.tmr_sched, sngss7_rmt_cong->t29.tmr_id)) {
					SS7_ERROR ("NSG-ACC: Unable to Cancle timer T29 timer\n");
				} else {
					sngss7_rmt_cong->t29.tmr_id = 0;
				}
			}
		} else {
			SS7_DEBUG("NSG-ACC: T30 Timer started for [%d]msec with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t30.beat, sngss7_rmt_cong->t30.tmr_id, sngss7_rmt_cong->dpc);
		}
	} else {
		/* Start Releasing traffic normally i.e. empty SS7 Call Queue */
		SS7_DEBUG("NSG-ACC: Remote end DPC[%d] is uncongested with congestion level[%d] and thus traffic is resumed\n", sngss7_rmt_cong->dpc, sngss7_rmt_cong->sngss7_rmtCongLvl);
	}

end:
	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}

/* This timer basically calculate the average calls per second each time timer experies and assign
 * bucket size depending upon the number of calls received and timer expiry rate */
void handle_route_acc_call_rate(void *userdata)
{
	sng_acc_tmr_t *timer_data = userdata;
	uint32_t rate = 0;	/* in seconds */
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = timer_data->sngss7_rmt_cong;

	if (!sngss7_rmt_cong) {
		SS7_ERROR("NSG-ACC: Invalid ftdm_sngss7_rmt_cong_t \n");
		return;
	}

	sngss7_rmt_cong->acc_call_rate.tmr_id = 0;
	rate = sngss7_rmt_cong->acc_call_rate.beat/1000;

	ftdm_mutex_lock(sngss7_rmt_cong->mutex);
	if (sngss7_rmt_cong->calls_received) {
		sngss7_rmt_cong->avg_call_rate = (sngss7_rmt_cong->calls_received/rate);
		if (!sngss7_rmt_cong->avg_call_rate) {
			sngss7_rmt_cong->avg_call_rate = sngss7_rmt_cong->calls_received;
		}

		if (sngss7_rmt_cong->sngss7_rmtCongLvl) {
			sngss7_rmt_cong->max_bkt_size = sngss7_rmt_cong->avg_call_rate;
		}

		SS7_DEBUG("NSG-ACC: Calls received[%d], Timer rate[%d] & Revised Bucket Size[%d]\n", sngss7_rmt_cong->calls_received, rate, sngss7_rmt_cong->max_bkt_size);
		sngss7_rmt_cong->calls_received = 0;
	}
	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

	/* Restart timer */
	if (ftdm_sched_timer (sngss7_rmt_cong->acc_call_rate.tmr_sched,
				"acc_call_rate",
				sngss7_rmt_cong->acc_call_rate.beat,
				sngss7_rmt_cong->acc_call_rate.callback,
				&sngss7_rmt_cong->acc_call_rate,
				&sngss7_rmt_cong->acc_call_rate.tmr_id)) {
		SS7_ERROR ("NSG-ACC: Unable to schedule ACC Call Rate Timer\n");
	}
}

#ifdef ACC_TEST
void handle_route_acc_debug(void *userdata)
{
	sng_acc_tmr_t *timer_data = userdata;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = timer_data->sngss7_rmt_cong;

	if (!sngss7_rmt_cong) {
		SS7_ERROR("NSG-ACC: Invalid ftdm_sngss7_rmt_cong_t \n");
		return;
	}

	sng_prnt_acc_debug(sngss7_rmt_cong->dpc);
	sngss7_rmt_cong->acc_debug.tmr_id = 0;

	/* Restart timer */
	if (ftdm_sched_timer (sngss7_rmt_cong->acc_debug.tmr_sched,
				"acc_debug",
				sngss7_rmt_cong->acc_debug.beat,
				sngss7_rmt_cong->acc_debug.callback,
				&sngss7_rmt_cong->acc_debug,
				&sngss7_rmt_cong->acc_debug.tmr_id)) {
		SS7_ERROR ("NSG-ACC: Unable to schedule ACC DEBUG Timer\n");
	}
}
#endif

void handle_wait_bla_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_wait_bla_timeout() timer kicked in.  %s\n", " ");
	ft_to_sngss7_blo(ftdmchan);
	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}
void handle_wait_uba_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_wait_uba_timeout() timer kicked in. %s\n", " ");
	ft_to_sngss7_blo(ftdmchan);
	ft_to_sngss7_ubl(ftdmchan);
	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}
void handle_tx_ubl_on_rx_bla_timer(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_tx_ubl_on_rx_bla_timer() timer kicked in. %s\n", " ");
	sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_UNBLK_TX);
	ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}

void handle_wait_rsca_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);

	if (timer->hb_timer_id) {
		ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_wait_rsca_timeout() Clearing timer %d\n", (int)timer->hb_timer_id);
		timer->hb_timer_id = 0;
	}

	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_wait_rsca_timeout() timer kicked in. %s\n", " ");
	ft_to_sngss7_rsc(ftdmchan);
	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
}

void handle_disable_ubl_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FTDM_FUNC__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_disable_ubl_timeout() timer kicked in.  %s\n", " ");
	sngss7_clear_cmd_pending_flag(sngss7_info, FLAG_CMD_UBL_DUMB);
	
	if (sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_UNBLK_TX) ||
	    sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_UNBLK_TX) ||
	    sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_MN_UNBLK_TX)) {
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
	}
	SS7_FUNC_TRACE_EXIT(__FTDM_FUNC__);
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

