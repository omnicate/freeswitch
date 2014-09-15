/*
 * Copyright (c) 2009 Konrad Hammel <konrad@sangoma.com>
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
 *
 * Contributors: 
 * 		 Pushkar Singh <psingh@sangoma.com>
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

/* Automatic Congestion Control Default Configuration */
ftdm_status_t ftmod_ss7_acc_default_config(void);
/* Check whether remote is congested or not */
ftdm_status_t ftdm_sangoma_ss7_get_congestion_status(ftdm_channel_t *ftdmchan);
/* Check ACC status on reception of Release Message */
ftdm_status_t ftdm_check_acc(sngss7_chan_data_t *sngss7_info, SiRelEvnt *siRelEvnt, ftdm_channel_t *ftdmchan);
/* Handle Active Calls rate */
ftdm_status_t sng_acc_handle_call_rate(ftdm_bool_t inc, ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong);
/* Get Remote congestion Structure */
 ftdm_sngss7_rmt_cong_t* sng_acc_get_cong_struct(ftdm_channel_t *ftdmchan);
/* Free ACC Hash list */
static void sngss7_free_acc_hashlist(void);
/* Free ACC parameters */
void sngss7_free_acc(void);
/******************************************************************************/

/* FUNCTIONS ******************************************************************/
/******************************************************************************/
/* Set Default configuration for Automatic Congestion Control Feature
 * if any parameter is not properly configured */
ftdm_status_t ftmod_ss7_acc_default_config()
{
	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	if (!g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate = CALL_CONTROL_RATE;
		SS7_DEBUG("Setting Default value for ACC Traffic reduction rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate);
	}    

	if (!g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate = CALL_CONTROL_RATE;
		SS7_DEBUG("Setting Default value for ACC Traffic increment rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate);
	}    

	if (!g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate = LVL1_CALL_CNTRL_RATE;
		SS7_DEBUG("Setting Default value for ACC Congestion Level 1 Traffic Reduction Rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate);
	}

	if (!g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate = LVL2_CALL_CNTRL_RATE;
		SS7_DEBUG("Setting Default value for ACC Congestion Level 2 Traffic Reduction Rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate);
	}

	return FTDM_SUCCESS;
}
  
/******************************************************************************/
/* Get Local/Remote Congestion and queue or Release call accordingly */
ftdm_status_t ftdm_sangoma_ss7_get_congestion_status(ftdm_channel_t *ftdmchan)
{
	int cpu_usage = 0;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;

	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	cpu_usage = ftdm_get_cpu_usage();

	if (!(sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan))) {
		SS7_INFO_CHAN(ftdmchan,"Could not find congestion structure for DPC[%d]... Incoming Call\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
		return FTDM_FAIL;
	}

	/* check if cpu usgae is more than that it is expected by user */
	if (cpu_usage > g_ftdm_sngss7_data.cfg.max_cpu_usage) {
		sngss7_rmt_cong->loc_calls_rejected++;
		SS7_DEBUG_CHAN(ftdmchan, "Hanging up call due to High cpu usage of [%d] against configured cpu limit[%d]!\n",
				cpu_usage, g_ftdm_sngss7_data.cfg.max_cpu_usage);
		ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_SWITCH_CONGESTION;
		sngss7_set_call_flag (sngss7_info, FLAG_CONG_REL);
		if (ftdm_queue_enqueue(sngss7_reject_queue.sngss7_call_rej_queue, ftdmchan)) {
			/* end the call */
			SS7_DEBUG_CHAN(ftdmchan, "Unable to queue as Call reject congestion queue is already full%s\n", "");
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
			return FTDM_BREAK;
		}
		return FTDM_SUCCESS;
	}

	if (!sngss7_rmt_cong) {
		SS7_INFO_CHAN(ftdmchan,"DPC[%d] is not preset in ACC hash list\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
		return FTDM_FAIL;
	}

	/* If remote exchange is congested then pass calls as per call block rate depending up on the congestion level */
	if ((sngss7_rmt_cong->sngss7_rmtCongLvl) && (sngss7_rmt_cong->dpc == g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc)) {
		if (!(sng_acc_handle_call_rate(FTDM_TRUE, sngss7_rmt_cong))) {
			return FTDM_FAIL;
		}

		sngss7_rmt_cong->calls_rejected++;
		SS7_DEBUG_CHAN(ftdmchan, "Reached maximum level of %d percentage to allow calls\n", (100 - sngss7_rmt_cong->call_blk_rate));
		SS7_DEBUG_CHAN(ftdmchan, "Call is rejected as the remote end is congested having congestion level as %d\n", sngss7_rmt_cong->sngss7_rmtCongLvl);
		ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_SWITCH_CONGESTION;
		sngss7_set_call_flag (sngss7_info, FLAG_CONG_REL);
		if (ftdm_queue_enqueue(sngss7_reject_queue.sngss7_call_rej_queue, ftdmchan)) {
			/* end the call */
			SS7_DEBUG_CHAN(ftdmchan, "Unable to queue as Call reject congestion queue is already full as bucket is full%s\n", "");
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
			return FTDM_BREAK;
		}
		return FTDM_SUCCESS;
	}

	return FTDM_FAIL;
}

/******************************************************************************/
/* Mark remote congestion depending upon the value of the ACL parameter
 * received in the RELEASE message */
ftdm_status_t ftdm_check_acc(sngss7_chan_data_t *sngss7_info, SiRelEvnt *siRelEvnt, ftdm_channel_t *ftdmchan)
{
	/* Contains Information w.r.t. congested DPC */
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	uint32_t dpc = g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc;


	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	if (!dpc) {
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d] Invalid DPC[%d]\n", sngss7_info->circuit->cic, dpc);
		return FTDM_FAIL;
	}

	if (!(sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan))) {
		SS7_INFO_CHAN(ftdmchan,"Could not find congestion structure for DPC[%d]\n", dpc);
		return FTDM_FAIL;
	}

	/* Check if remote exchange is congested */
	if (siRelEvnt->autoCongLvl.eh.pres) {
		if (siRelEvnt->autoCongLvl.auCongLvl.pres) {
			/* * If remote congestion is not known at all then mark remote congestion level value, mark call_block
			 *   rate as configured depending upon the congestion level received and start t29 & t30 timers and
			 *   then release the call */
			if ((!sngss7_rmt_cong->sngss7_rmtCongLvl) && (sngss7_rmt_cong->dpc == dpc)) {
				sngss7_rmt_cong->sngss7_rmtCongLvl = siRelEvnt->autoCongLvl.auCongLvl.val;
				/* Assign call block rate as configured depending upon the congestion level received */
				if (sngss7_rmt_cong->sngss7_rmtCongLvl == 1) {
					sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate;
				} else {
					sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate;
				}

				SS7_INFO_CHAN(ftdmchan,"Received automatic congestion from remote dpc[%d] with congestion level as[%d]\n", 
						sngss7_rmt_cong->dpc, sngss7_rmt_cong->sngss7_rmtCongLvl);
			} else {
				/* * If remote congestion is already known and t29 & t30 timer is already running:
				 *   * Do nothing & release call normally.
				 *
				 * * If remote congestion is already known, t29 timer is not running but t30 is still running:
				 *   * When ACL 1 is received:
				 *     * If current remote congestion Level is 1 then increase call block rate by configured
				 *       rate, restart t29 & t30 timer and then release call.
				 *     * If current remote congestion Level is 2 then decrease call block rate to level 1,
				 *       restart t29 & t30 timer and then release call.
				 *
				 *   * When ACL 2 is received:
				 *     * If current remote congestion Level is 1 then increase call block rate by level 2,
				 *       restart t29 & t30 timer and then release call.
				 *     * If current remote congestion Level is 2 then restart t29 & t30 timer and then release
				 *       call.
				 */
				if (sngss7_rmt_cong->dpc == dpc) {
					if (sngss7_rmt_cong->t29.tmr_id) {
						return FTDM_SUCCESS;
					} else if (sngss7_rmt_cong->t30.tmr_id) {
						if ((sngss7_rmt_cong->sngss7_rmtCongLvl == siRelEvnt->autoCongLvl.auCongLvl.val) && (sngss7_rmt_cong->sngss7_rmtCongLvl == 1)) {
							if (sngss7_rmt_cong->call_blk_rate < g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate) {
								sngss7_rmt_cong->call_blk_rate = sngss7_rmt_cong->call_blk_rate + g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate;
							}
						}

						if ((siRelEvnt->autoCongLvl.auCongLvl.val == 1) && (sngss7_rmt_cong->sngss7_rmtCongLvl == 2)) {
							sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate;
							sngss7_rmt_cong->sngss7_rmtCongLvl--;
						} else if ((siRelEvnt->autoCongLvl.auCongLvl.val == 2) && (sngss7_rmt_cong->sngss7_rmtCongLvl == 1)) {
							sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate;
							sngss7_rmt_cong->sngss7_rmtCongLvl++;
						}

						/* kill t29 if active in order to restart the same */
						if (sngss7_rmt_cong->t29.tmr_id) {
							if (ftdm_sched_cancel_timer (sngss7_rmt_cong->t29.tmr_sched, sngss7_rmt_cong->t29.tmr_id)) {
								SS7_ERROR ("Unable to Restart T29 timer\n");
							} else {
								sngss7_rmt_cong->t29.tmr_id = 0;
							}
						}

						/* kill t30 if active inorder to restart the same */
						if (sngss7_rmt_cong->t30.tmr_id) {
							if (ftdm_sched_cancel_timer (sngss7_rmt_cong->t30.tmr_sched, sngss7_rmt_cong->t30.tmr_id)) {
								SS7_ERROR ("Unable to Restart T30 timer\n");
							} else {
								sngss7_rmt_cong->t30.tmr_id = 0;
							}
						}
					}
				}
			}

			/* Restart T29/T30 if we received REL in congested state */
			if (sngss7_rmt_cong->t29.tmr_id) {
				SS7_INFO_CHAN(ftdmchan,"Starting T29 Timer %d is already running donot start it\n", sngss7_rmt_cong->t29.tmr_id);
				goto start_t30;
			}

			SS7_INFO_CHAN(ftdmchan,"Starting T29 Timer for [%d]msec due to remote congestion level[%d] present at dpc[%d]\n", sngss7_rmt_cong->t29.beat, sngss7_rmt_cong->sngss7_rmtCongLvl, sngss7_rmt_cong->dpc);
			/* Start T29 & T30 timer */
			if (ftdm_sched_timer (sngss7_rmt_cong->t29.tmr_sched,
						"t29",
						sngss7_rmt_cong->t29.beat,
						sngss7_rmt_cong->t29.callback,
						&sngss7_rmt_cong->t29,
						&sngss7_rmt_cong->t29.tmr_id)) {
				SS7_ERROR ("Unable to schedule timer T29\n");
				return FTDM_SUCCESS;
			} else {
				SS7_INFO_CHAN(ftdmchan,"T29 Timer started with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t29.tmr_id, sngss7_rmt_cong->dpc);
			}

start_t30:
			if (sngss7_rmt_cong->t30.tmr_id) {
				SS7_INFO_CHAN(ftdmchan,"Starting T30 Timer %d is already running donot start it\n", sngss7_rmt_cong->t30.tmr_id);
				goto done;
			}

			SS7_INFO_CHAN(ftdmchan,"Starting T30 Timer for [%d]msec due to remote congestion level[%d] present at dpc[%d]\n", sngss7_rmt_cong->t30.beat, sngss7_rmt_cong->sngss7_rmtCongLvl, sngss7_rmt_cong->dpc);
			if (ftdm_sched_timer (sngss7_rmt_cong->t30.tmr_sched,
						"t30",
						sngss7_rmt_cong->t30.beat,
						sngss7_rmt_cong->t30.callback,
						&sngss7_rmt_cong->t30,
						&sngss7_rmt_cong->t30.tmr_id)) {
				SS7_ERROR ("Unable to schedule timer T30. Thus, stopping T29 Timer also.\n");

				/* Kill t29 timer if active */
				if (sngss7_rmt_cong->t29.tmr_id) {
					if (ftdm_sched_cancel_timer (sngss7_rmt_cong->t29.tmr_sched, sngss7_rmt_cong->t29.tmr_id)) {
						SS7_ERROR ("Unable to Cancle timer T29 timer\n");
					} else {
						sngss7_rmt_cong->t29.tmr_id = 0;
					}
				}
			} else {
				SS7_INFO_CHAN(ftdmchan,"T30 Timer started with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t30.tmr_id, sngss7_rmt_cong->dpc);
			}

		}
done:
		/* Remove ACC parameter from RELEASE Message */
		siRelEvnt->autoCongLvl.eh.pres = NOTPRSNT;
		siRelEvnt->autoCongLvl.auCongLvl.pres = NOTPRSNT;
		siRelEvnt->autoCongLvl.auCongLvl.val = NOTPRSNT;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
/* Check If inc flag is true then increase the number of calls allowed iff
 * number of active calls is less than that of maximum number of calls allowed
 */
ftdm_status_t sng_acc_handle_call_rate(ftdm_bool_t inc, ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong)
{
	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		goto done;

	if (sngss7_rmt_cong->sngss7_rmtCongLvl) {
		if (inc) {
			if (sngss7_rmt_cong->calls_allowed < (g_ftdm_sngss7_data.cfg.accCfg.max_bkt_size - ((g_ftdm_sngss7_data.cfg.accCfg.max_bkt_size * sngss7_rmt_cong->call_blk_rate)/100))) {
				SS7_DEBUG("Number of calls allowed[%d], Max Bucket size[%d], Call Block Rate[%d]\n\n\n", sngss7_rmt_cong->calls_allowed, g_ftdm_sngss7_data.cfg.accCfg.max_bkt_size, sngss7_rmt_cong->call_blk_rate);

				sngss7_rmt_cong->calls_allowed++;
				sngss7_rmt_cong->calls_passed++;
				return FTDM_SUCCESS;
			}
		} else {
			if (sngss7_rmt_cong->calls_allowed) {
				sngss7_rmt_cong->calls_allowed--;
				return FTDM_SUCCESS;
			}
		}
	}

done:
	return FTDM_FAIL;
}

/******************************************************************************/
/* Get ACC Structure */
 ftdm_sngss7_rmt_cong_t* sng_acc_get_cong_struct(ftdm_channel_t *ftdmchan)
{
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	uint32_t dpc = 0x00;
	char hash_dpc[25];

	if (!sngss7_info) {
		return FTDM_SUCCESS;
	}

	dpc = g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc;
	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_SUCCESS;

	memset(hash_dpc, 0 , sizeof(hash_dpc));
	if (!dpc) {
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d] Invalid DPC[%d]\n", sngss7_info->circuit->cic, dpc);
		return FTDM_SUCCESS;
	}

	sprintf(hash_dpc, "%d", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);

	/* Get remote congestion information for respective DPC from hash list */
	sngss7_rmt_cong = hashtable_search(ss7_rmtcong_lst, (void *)hash_dpc);

	if (!sngss7_rmt_cong) {
		SS7_INFO_CHAN(ftdmchan,"DPC[%d] is not preset in ACC hash list\n", dpc);
		return FTDM_SUCCESS;
	}

	return sngss7_rmt_cong;
}

/******************************************************************************/
/* Iterate through ACC hash list and remove hash list members one by one */
static void sngss7_free_acc_hashlist()
{
	ftdm_hash_iterator_t *i = NULL;
	const void *key = NULL;
	void *val = NULL;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;


	for (i = hashtable_first(ss7_rmtcong_lst); i; i = hashtable_next(i)) {
		hashtable_this(i, &key, NULL, &val);
		if (!key || !val) {
			break;
		}
		sngss7_rmt_cong = (ftdm_sngss7_rmt_cong_t*)val;
		ftdm_safe_free(sngss7_rmt_cong);
	}
}

/******************************************************************************/
/* Free all the parameter as allocated w.r.t. ACC Feature */
void sngss7_free_acc()
{
	ftdm_sngss7_call_reject_queue_t  *reject_queue = NULL;
	int max_wait = 10;

	SS7_DEBUG("De-allocating memory allocated to ACC parameters\n");
	while (max_wait-- > 0) {
		fprintf(stderr, "Waiting for all threads to finish \n");
		ftdm_sleep(100);
		max_wait--;
	}

	reject_queue = &sngss7_reject_queue;
	if (reject_queue->sngss7_call_rej_queue) {
		ftdm_queue_destroy(&reject_queue->sngss7_call_rej_queue);
		SS7_DEBUG("De-allocating reject queue\n");
	}

	reject_queue = NULL;

	if (ss7_rmtcong_lst) {
		SS7_DEBUG("De-allocation Hashlists\n");
		/* free all the members of ACC hash list */
		sngss7_free_acc_hashlist();
		/* delete the hash table */
		hashtable_destroy(ss7_rmtcong_lst);
		ss7_rmtcong_lst = NULL;
	}
	SS7_DEBUG("De-allocation Complete\n");
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
