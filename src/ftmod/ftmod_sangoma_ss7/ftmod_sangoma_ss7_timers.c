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
 *
 * Moises Silva <moy@sangoma.com>
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
    SS7_FUNC_TRACE_ENTER(__FUNCTION__);

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

    SS7_FUNC_TRACE_EXIT(__FUNCTION__);
    return;
}


void handle_isup_t10(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;

	ftdm_channel_lock(ftdmchan);

	SS7_DEBUG("[Call-Control] Timer 10 expired on CIC = %d\n", sngss7_info->circuit->cic);

	/* send the call to the user */
	ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RING);

	ftdm_channel_unlock(ftdmchan);

	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}

void handle_isup_t39(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

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

	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}

/* Changes w.r.t. ACC feature */
void handle_isup_t29(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;

	ftdm_channel_lock(ftdmchan);

	SS7_DEBUG("Timer 29 expired.....\n");

	ftdm_channel_unlock(ftdmchan);

	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}

void handle_isup_t30(void *userdata)
{
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);

	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	ftdm_channel_t 	    *tmp_ftdmchan = NULL;

	ftdm_channel_lock(ftdmchan);

	SS7_DEBUG("Timer 30 expired.....\n");

	if (sngss7_rmtCongLvl) {
		sngss7_rmtCongLvl--;
	}

	if (sngss7_rmtCongLvl) {
		/* Restart Timer 29 & Timer 30 */
		if (ftdm_sched_timer (sngss7_info->t29.sched,
					"t39",
					sngss7_info->t29.beat,
					sngss7_info->t29.callback,
					&sngss7_info->t29,
					&sngss7_info->t29.hb_timer_id)) {
			SS7_ERROR ("Unable to schedule timer T29\n");
			goto end;
		}

		if (ftdm_sched_timer (sngss7_info->t30.sched,
					"t30",
					sngss7_info->t30.beat,
					sngss7_info->t30.callback,
					&sngss7_info->t30,
					&sngss7_info->t30.hb_timer_id)) {
			SS7_ERROR ("Unable to schedule timer T30\n");
		}

	} else {
		/* Start Releasing traffic normally i.e. empty SS7 Call Queue */
		while ((tmp_ftdmchan = sngss7_dequeueCall())) {
			SS7_DEBUG_CHAN(tmp_ftdmchan, "Sending outgoing call from \"%s\" to \"%s\" to LibSngSS7\n",
					tmp_ftdmchan->caller_data.ani.digits,
					tmp_ftdmchan->caller_data.dnis.digits);

			/*call sangoma_ss7_dial to make outgoing call */
			ft_to_sngss7_iam(tmp_ftdmchan);
		}
	}

end:
	ftdm_channel_unlock(ftdmchan);

	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}

void handle_wait_bla_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_wait_bla_timeout() timer kicked in.  %s\n", " ");
	ft_to_sngss7_blo(ftdmchan);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}
void handle_wait_uba_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_wait_uba_timeout() timer kicked in. %s\n", " ");
	ft_to_sngss7_blo(ftdmchan);
	ft_to_sngss7_ubl(ftdmchan);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}
void handle_tx_ubl_on_rx_bla_timer(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_tx_ubl_on_rx_bla_timer() timer kicked in. %s\n", " ");
	sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_UNBLK_TX);
	ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}

void handle_wait_rsca_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_wait_rsca_timeout() timer kicked in. %s\n", " ");
	ft_to_sngss7_rsc(ftdmchan);
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
}
void handle_disable_ubl_timeout(void *userdata)
{
	sngss7_timer_data_t *timer = userdata;
	sngss7_chan_data_t  *sngss7_info = timer->sngss7_info;
	ftdm_channel_t      *ftdmchan = sngss7_info->ftdmchan;
	
	SS7_FUNC_TRACE_ENTER(__FUNCTION__);
	ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "handle_disable_ubl_timeout() timer kicked in.  %s\n", " ");
	sngss7_clear_cmd_pending_flag(sngss7_info, FLAG_CMD_UBL_DUMB);
	
	if (sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_UNBLK_TX) ||
	    sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_UNBLK_TX) ||
	    sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_MN_UNBLK_TX)) {
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
	}
	SS7_FUNC_TRACE_EXIT(__FUNCTION__);
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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
/******************************************************************************/

