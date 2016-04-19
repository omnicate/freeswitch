/*
 * Copyright (c) 2014 Pushkar Singh <psingh@sangoma.com>
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

ftdm_hash_t *ss7_rmtcong_lst;	/* Hash list of all dpc to store congestion values */

/******************************************************************************/

/* PROTOTYPES *****************************************************************/

/* Automatic Congestion Control Default Configuration */
ftdm_status_t ftmod_ss7_acc_default_config(void);
/* Configure the call block rate as per number of cics configured per DPC basis */
ftdm_status_t sng_acc_assign_max_bucket(uint32_t intfId, uint32_t cics_cfg);
/* Check whether remote is congested or not */
ftdm_status_t ftdm_sangoma_ss7_get_congestion_status(ftdm_channel_t *ftdmchan);
/* Check ACC status on reception of Release Message */
ftdm_status_t ftdm_check_acc(sngss7_chan_data_t *sngss7_info, SiRelEvnt *siRelEvnt, ftdm_channel_t *ftdmchan);
/* Handle Active Calls rate */
ftdm_status_t sng_acc_handle_call_rate(ftdm_bool_t inc, ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong, ftdm_channel_t *ftdmchan);
/* Get Remote congestion Structure */
ftdm_sngss7_rmt_cong_t* sng_acc_get_cong_struct(ftdm_channel_t *ftdmchan);
/* Free ACC Hash list */
static void sngss7_free_acc_hashlist(void);
/* Free ACC parameters */
void sngss7_free_acc(void);
#ifdef ACC_TEST
/* Print ACC Call Statistics in a acc_debug.txt file */
ftdm_status_t sng_prnt_acc_debug(uint32_t dpc);
/* Increment the ACC DEBUG counters */
ftdm_status_t sng_increment_acc_statistics(ftdm_channel_t *ftdmchan, uint32_t acc_debug_lvl);
#endif

/* Insert calls active after congetion is determined */
ftdm_status_t sng_acc_active_call_insert(ftdm_channel_t *ftdmchan);
/* Free active call hash list */
ftdm_status_t sng_acc_free_active_calls_hashlist(ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong);
/* Increment Call Received Counter */
ftdm_status_t ftdm_sangoma_ss7_received_call(ftdm_channel_t *ftdmchan);

/******************************************************************************/

/* FUNCTIONS ******************************************************************/
/******************************************************************************/
/* Set Default configuration for Automatic Congestion Control Feature
 * if any parameter is not properly configured */
ftdm_status_t ftmod_ss7_acc_default_config()
{
	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	if (ss7_rmtcong_lst == NULL) {
		/* initializing global ACC structure */
		ss7_rmtcong_lst = create_hashtable(MAX_DPC_CONFIGURED, ftdm_hash_hashfromstring, ftdm_hash_equalkeys);

		if (!ss7_rmtcong_lst) {
			SS7_DEBUG("NSG-ACC: Failed to create hash list for ss7_rmtcong_lst \n");
			return FTDM_SUCCESS;
		} else {
			SS7_DEBUG("NSG-ACC: ss7_rmtcong_lst successfully created \n");
		}
	}

	if (!g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate = CALL_CONTROL_RATE;
		SS7_DEBUG("NSG-ACC: Setting Default value for ACC Traffic reduction rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate);
	}

	if (!g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate = CALL_CONTROL_RATE;
		SS7_DEBUG("NSG-ACC: Setting Default value for ACC Traffic increment rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate);
	}    

	if (!g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate = LVL1_CALL_CNTRL_RATE;
		SS7_DEBUG("NSG-ACC: Setting Default value for ACC Congestion Level 1 Traffic Reduction Rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate);
	}

	if (!g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate) {
		g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate = LVL2_CALL_CNTRL_RATE;
		SS7_DEBUG("NSG-ACC: Setting Default value for ACC Congestion Level 2 Traffic Reduction Rate to %d\n", g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate);
	}

	return FTDM_SUCCESS;
}

/* Configure the call block rate as per number of cics configured per DPC basis */
ftdm_status_t sng_acc_assign_max_bucket(uint32_t intfId, uint32_t cics_cfg)
{
	ftdm_sngss7_rmt_cong_t  *sngss7_rmt_cong = NULL;
	ftdm_hash_iterator_t 	*i = NULL;
	const void 		*key = NULL;
	void 			*val = NULL;
	int 			loop = 0;
	uint32_t 		idx = 0;

	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	/* Iterate trough hash table and update the call block rate */
	for (i = hashtable_first(ss7_rmtcong_lst); i; i = hashtable_next(i)) {
		idx = 0;
		hashtable_this(i, &key, NULL, &val);
		if (!key || !val) {
			break;
		}

		sngss7_rmt_cong = (ftdm_sngss7_rmt_cong_t *)val;
		/* Check if any entry is present in ACC hastable */
		if(!sngss7_rmt_cong) {
			continue;
		}

		while (idx < (MAX_ISUP_INFS)) {
			if (intfId == g_ftdm_sngss7_data.cfg.isupIntf[idx].id) {
				if (g_ftdm_sngss7_data.cfg.isupIntf[idx].dpc == sngss7_rmt_cong->dpc) {
					if (!sngss7_rmt_cong->max_bkt_size) {
						sngss7_rmt_cong->max_bkt_size = sngss7_rmt_cong->max_bkt_size + cics_cfg;
						SS7_DEBUG("NSG-ACC: Configured maximum bucket size as %d for DPC %d\n",sngss7_rmt_cong->max_bkt_size, sngss7_rmt_cong->dpc);
					}
					loop = 1;
					break;
				}
			}
			idx++;
		}
		if (loop) {
			break;
		}
	}

	if (loop) {
		return FTDM_SUCCESS;
	}

	return FTDM_FAIL;
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

	//SS7_INFO_CHAN(ftdmchan,"NSG-ACC: Getting congestion Status for DPC[%d]\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
	if (!(sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan))) {
		SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: Could not find congestion structure for DPC[%d]... Incoming Call\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
		return FTDM_FAIL;
	}

	/* check if cpu usgae is more than that it is expected by user */
	if (cpu_usage > g_ftdm_sngss7_data.cfg.max_cpu_usage) {
		sngss7_rmt_cong->loc_calls_rejected++;
		SS7_DEBUG_CHAN(ftdmchan, "NSG-ACC: Hanging up call due to High cpu usage of [%d] against configured cpu limit[%d]!\n",
				cpu_usage, g_ftdm_sngss7_data.cfg.max_cpu_usage);
		/*ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_SWITCH_CONGESTION;*/
		ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_NORMAL_CIRCUIT_CONGESTION;
		sngss7_set_call_flag (sngss7_info, FLAG_CONG_REL);
		if (ftdm_queue_enqueue(sngss7_reject_queue.sngss7_call_rej_queue, ftdmchan)) {
			/* end the call */
			/* in case of native bridge */
			if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {
				SS7_ERROR_CHAN(ftdmchan, "NSG-ACC: Unable to queue as Call reject congestion queue is already full for Native Bridge. Thus, moving to TERMINATIG STATE%s\n", "");
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_TERMINATING);
			} else {
				SS7_ERROR_CHAN(ftdmchan, "NSG-ACC: Unable to queue as Call reject congestion queue is already full. Thus, moving to HANGUP state%s\n", "");
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
			}

			return FTDM_BREAK;
		}
		return FTDM_SUCCESS;
	}

	if (!sngss7_rmt_cong) {
		SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: DPC[%d] is not preset in ACC hash list\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
		return FTDM_FAIL;
	}

	/* If remote exchange is congested then pass calls as per call block rate depending up on the congestion level */
	if ((sngss7_rmt_cong->sngss7_rmtCongLvl) && (sngss7_rmt_cong->dpc == g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc)) {

		SS7_DEBUG_CHAN(ftdmchan, "NSG-ACC: DPC[%d] is congested having congestion Level as  %d\n", sngss7_rmt_cong->dpc, sngss7_rmt_cong->sngss7_rmtCongLvl);

		if (!(sng_acc_handle_call_rate(FTDM_TRUE, sngss7_rmt_cong, ftdmchan))) {
			return FTDM_FAIL;
		}

		ftdm_mutex_lock(sngss7_rmt_cong->mutex);
		sngss7_rmt_cong->calls_rejected++;
		ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

		SS7_DEBUG_CHAN(ftdmchan, "NSG-ACC: Reached maximum level of %d percentage to allow calls\n", (100 - sngss7_rmt_cong->call_blk_rate));
		SS7_DEBUG_CHAN(ftdmchan, "NSG-ACC: Call is rejected as the remote end is congested having congestion level as %d\n", sngss7_rmt_cong->sngss7_rmtCongLvl);

		/*ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_SWITCH_CONGESTION;*/
		ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_NORMAL_CIRCUIT_CONGESTION;
		sngss7_set_call_flag (sngss7_info, FLAG_CONG_REL);
		if (ftdm_queue_enqueue(sngss7_reject_queue.sngss7_call_rej_queue, ftdmchan)) {
			/* end the call */
			SS7_ERROR_CHAN(ftdmchan, "NSG-ACC: Unable to queue as Call reject congestion queue is already full as bucket is full%s\n", "");
			/* in case of native bridge */
			if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_TERMINATING);
			} else {
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
			}

			return FTDM_BREAK;
		}
		return FTDM_SUCCESS;
	} else {
		/* Since there is no congestion. Thus, this is a normal call and we donnot have to decrement the counter of
		 * number of allowed calls in case of congestion */
		sngss7_set_call_flag (sngss7_info, FLAG_NRML_CALL);
		SS7_DEBUG_CHAN(ftdmchan, "NSG-ACC: This is a normal call for dpc[%d] as congestion is not present till now\n", sngss7_rmt_cong->dpc);
	}

	return FTDM_FAIL;
}

/******************************************************************************/
/* Mark remote congestion depending upon the value of the ACL parameter
 * received in the RELEASE message */
ftdm_status_t ftdm_check_acc(sngss7_chan_data_t *sngss7_info, SiRelEvnt *siRelEvnt, ftdm_channel_t *ftdmchan)
{
	int lvlchange = 0;
	/* Contains Information w.r.t. congested DPC */
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	uint32_t dpc = g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc;


	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	if (!dpc) {
		SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: [CIC:%d] Invalid DPC[%d]\n", sngss7_info->circuit->cic, dpc);
		return FTDM_FAIL;
	}

	if (!(sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan))) {
		SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: Could not find congestion structure for DPC[%d]\n", dpc);
		return FTDM_FAIL;
	}

#ifdef ACC_TEST
	/* Increment ACC DEBUG counter */
	if (siRelEvnt->autoCongLvl.eh.pres) {
		if (siRelEvnt->autoCongLvl.auCongLvl.pres) {
			if (sngss7_rmt_cong->sngss7_rmtCongLvl == 1) {
				sng_increment_acc_statistics(ftdmchan, ACC_REL_RECV_ACL1_DEBUG);
			} else if (sngss7_rmt_cong->sngss7_rmtCongLvl == 2) {
				sng_increment_acc_statistics(ftdmchan, ACC_REL_RECV_ACL2_DEBUG);
			} else if (sngss7_rmt_cong->sngss7_rmtCongLvl == 0) {
				/* This is an invalid case but in order to be on the safer side include this */
				sng_increment_acc_statistics(ftdmchan, ACC_REL_RECV_DEBUG);
			}
		} else {
			sng_increment_acc_statistics(ftdmchan, ACC_REL_RECV_DEBUG);
		}
	} else {
		sng_increment_acc_statistics(ftdmchan, ACC_REL_RECV_DEBUG);
	}
#endif

	/* Check if remote exchange is congested */
	if (siRelEvnt->autoCongLvl.eh.pres) {
		if (siRelEvnt->autoCongLvl.auCongLvl.pres) {
			/* * If remote congestion is not known at all then mark remote congestion level value, mark call_block
			 *   rate as configured depending upon the congestion level received and start t29 & t30 timers and
			 *   then release the call */
			if ((!sngss7_rmt_cong->sngss7_rmtCongLvl) && (sngss7_rmt_cong->dpc == dpc)) {
				ftdm_mutex_lock(sngss7_rmt_cong->mutex);
				sngss7_rmt_cong->sngss7_rmtCongLvl = siRelEvnt->autoCongLvl.auCongLvl.val;
				/* Assign call block rate as configured depending upon the congestion level received */
				if (sngss7_rmt_cong->sngss7_rmtCongLvl == 1) {
					sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate;
				} else {
					sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate;
				}

				/* change acc bucket size depending upon the call rate once congestion starts */
				sngss7_rmt_cong->max_bkt_size = sngss7_rmt_cong->avg_call_rate;

				ftdm_mutex_unlock(sngss7_rmt_cong->mutex);
				SS7_INFO_CHAN(ftdmchan,"NSG-ACC: Entering to Congestion : Received ACL[%d] from remote dpc[%d], Congestion Rate[%d] \n",
						sngss7_rmt_cong->sngss7_rmtCongLvl, sngss7_rmt_cong->dpc, sngss7_rmt_cong->call_blk_rate);
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
						/* Add counter so we can keep count within T29 how many ACL we received */
						/* TODO */
						SS7_INFO_CHAN(ftdmchan,"NSG-ACC: T29: ACL received, continue with call %s\n","");
						return FTDM_SUCCESS;
					} else if (sngss7_rmt_cong->t30.tmr_id) {
						SS7_INFO_CHAN(ftdmchan,"NSG-ACC: Resetting T29 %s\n","");
						ftdm_mutex_lock(sngss7_rmt_cong->mutex);
						if ((sngss7_rmt_cong->sngss7_rmtCongLvl == siRelEvnt->autoCongLvl.auCongLvl.val) && (sngss7_rmt_cong->sngss7_rmtCongLvl == 1)) {
							if (sngss7_rmt_cong->call_blk_rate < g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate) {
								sngss7_rmt_cong->call_blk_rate = sngss7_rmt_cong->call_blk_rate + g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate;
								SS7_INFO_CHAN(ftdmchan,"NSG-ACC: T30 : ACL[1] Reducing Call Rate to [%d] for dpc[%d]\n" , sngss7_rmt_cong->call_blk_rate,dpc);
								lvlchange = 1;
							}
						}

						if ((siRelEvnt->autoCongLvl.auCongLvl.val == 1) && (sngss7_rmt_cong->sngss7_rmtCongLvl == 2)) {
							sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate;
							sngss7_rmt_cong->sngss7_rmtCongLvl--;
							lvlchange = 1;
							SS7_INFO_CHAN(ftdmchan,"NSG-ACC: T30 : ACL[1] from ACL[2] Increasing Call Rate to [%d] for dpc[%d]\n" , sngss7_rmt_cong->call_blk_rate,dpc);
						} else if ((siRelEvnt->autoCongLvl.auCongLvl.val == 2) && (sngss7_rmt_cong->sngss7_rmtCongLvl == 1)) {
							sngss7_rmt_cong->call_blk_rate = g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate;
							sngss7_rmt_cong->sngss7_rmtCongLvl++;
							lvlchange = 1;
							SS7_INFO_CHAN(ftdmchan,"NSG-ACC: T30 : ACL[2] from ACL[1] Reducing Call Rate to [%d] for dpc[%d]\n" , sngss7_rmt_cong->call_blk_rate,dpc);
						}


						if (lvlchange) {
							/* reset whole active call list as block rate or congestion level is changed so that
							 * we can keep count for this level only and discard previous active calls count */
							sng_acc_free_active_calls_hashlist(sngss7_rmt_cong);
							sngss7_rmt_cong->calls_allowed = 0;
						}

						ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

						/* kill t29 if active in order to restart the same */
						if (sngss7_rmt_cong->t29.tmr_id) {
							if (ftdm_sched_cancel_timer (sngss7_rmt_cong->t29.tmr_sched, sngss7_rmt_cong->t29.tmr_id)) {
								SS7_ERROR ("NSG-ACC: Unable to Restart T29 timer\n");
							} else {
								sngss7_rmt_cong->t29.tmr_id = 0;
							}
						}

						/* kill t30 if active inorder to restart the same */
						if (sngss7_rmt_cong->t30.tmr_id) {
							if (ftdm_sched_cancel_timer (sngss7_rmt_cong->t30.tmr_sched, sngss7_rmt_cong->t30.tmr_id)) {
								SS7_ERROR ("NSG-ACC: Unable to Restart T30 timer\n");
							} else {
								sngss7_rmt_cong->t30.tmr_id = 0;
							}
						}
					}
					else {
						SS7_ERROR ("NSG-ACC: ACL[%d] received for Dpc[%d] while no T29/30 timer running \n",
								siRelEvnt->autoCongLvl.auCongLvl.val,dpc);
					}
				}
				else {
					SS7_ERROR ("NSG-ACC: Dpc[%d] not matched with incoming dpc[%d]\n", sngss7_rmt_cong->dpc, dpc);
				}
			}

			/* Restart T29/T30 if we received REL in congested state */
			if (sngss7_rmt_cong->t29.tmr_id) {
				SS7_INFO_CHAN(ftdmchan,"NSG-ACC: Starting T29 Timer %d is already running donot start it\n", sngss7_rmt_cong->t29.tmr_id);
				goto start_t30;
			}

			SS7_INFO_CHAN(ftdmchan,"NSG-ACC: Starting T29 Timer for [%d]msec due to remote congestion level[%d] present at dpc[%d]\n", 
					sngss7_rmt_cong->t29.beat, sngss7_rmt_cong->sngss7_rmtCongLvl, sngss7_rmt_cong->dpc);
			/* Start T29 & T30 timer */
			if (ftdm_sched_timer (sngss7_rmt_cong->t29.tmr_sched,
						"t29",
						sngss7_rmt_cong->t29.beat,
						sngss7_rmt_cong->t29.callback,
						&sngss7_rmt_cong->t29,
						&sngss7_rmt_cong->t29.tmr_id)) {
				SS7_ERROR ("NSG-ACC: Unable to schedule timer T29\n");
				return FTDM_SUCCESS;
			} else {
				SS7_INFO_CHAN(ftdmchan,"NSG-ACC: T29 Timer started with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t29.tmr_id, sngss7_rmt_cong->dpc);
			}

start_t30:
			if (sngss7_rmt_cong->t30.tmr_id) {
				SS7_INFO_CHAN(ftdmchan,"NSG-ACC: Starting T30 Timer %d is already running donot start it\n", sngss7_rmt_cong->t30.tmr_id);
				goto done;
			}

			SS7_INFO_CHAN(ftdmchan,"NSG-ACC: Starting T30 Timer for [%d]msec due to remote congestion level[%d] present at dpc[%d]\n", sngss7_rmt_cong->t30.beat, sngss7_rmt_cong->sngss7_rmtCongLvl, sngss7_rmt_cong->dpc);
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
				SS7_INFO_CHAN(ftdmchan,"NSG-ACC: T30 Timer started with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t30.tmr_id, sngss7_rmt_cong->dpc);
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
ftdm_status_t sng_acc_handle_call_rate(ftdm_bool_t inc, ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong, ftdm_channel_t *ftdmchan)
{
	ftdm_status_t ret=FTDM_FAIL;

	if (!g_ftdm_sngss7_data.cfg.sng_acc) {
		return ret ;
	}

	ftdm_mutex_lock(sngss7_rmt_cong->mutex);

	if (sngss7_rmt_cong->sngss7_rmtCongLvl) {
		if (inc) {
			SS7_INFO_CHAN(ftdmchan, "NSG-ACC: Bkt Size[%d], Blk Rate[%d], Current Call Allowed[%d], Max limit[%d]\n",
						 sngss7_rmt_cong->max_bkt_size, sngss7_rmt_cong->call_blk_rate, sngss7_rmt_cong->calls_allowed,
						 (sngss7_rmt_cong->max_bkt_size - ((sngss7_rmt_cong->max_bkt_size * sngss7_rmt_cong->call_blk_rate)/100)));

			if (sngss7_rmt_cong->calls_allowed < (sngss7_rmt_cong->max_bkt_size - ((sngss7_rmt_cong->max_bkt_size * sngss7_rmt_cong->call_blk_rate)/100))) {
				sngss7_rmt_cong->calls_allowed++;
				sngss7_rmt_cong->calls_passed++;
				ret = FTDM_SUCCESS;
				/* Calls allowed, make entry to active call list */
				sng_acc_active_call_insert(ftdmchan);
				goto done;
			} else {
				SS7_INFO_CHAN(ftdmchan, "NSG-ACC: Rejecting incoming call due to reaching limit.. Allowed Calls[%d], Max Bucket size[%d], Call Block Rate[%d]\n",
						sngss7_rmt_cong->calls_allowed, sngss7_rmt_cong->max_bkt_size, sngss7_rmt_cong->call_blk_rate);
			}
		} else {
			if (sngss7_rmt_cong->calls_allowed) {
				/*sngss7_rmt_cong->calls_allowed--;*/
				ret = FTDM_SUCCESS;
				goto done;
			} else {
				SS7_INFO_CHAN(ftdmchan, "NSG-ACC: Rejecting incoming call %s\n", "");
			}
		}
	}

done:
	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);
	return ret;
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
		SS7_ERROR("NSG-ACC: [CIC:%d] Invalid DPC[%d]\n", sngss7_info->circuit->cic, dpc);
		return FTDM_SUCCESS;
	}

	sprintf(hash_dpc, "%d", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);

	/* Get remote congestion information for respective DPC from hash list */
	sngss7_rmt_cong = hashtable_search(ss7_rmtcong_lst, (void *)hash_dpc);

	if (!sngss7_rmt_cong) {
		SS7_ERROR("NSG-ACC: DPC[%d] is not preset in ACC hash list\n", dpc);
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

#ifdef ACC_TEST
		/* Closing the file pointer */
		if (sngss7_rmt_cong->log_file_ptr) {
			SS7_DEBUG("NSG-ACC: Closing File pointer for dpc[%d]", sngss7_rmt_cong->dpc);
			fclose(sngss7_rmt_cong->log_file_ptr);
		}
#endif
		sng_acc_free_active_calls_hashlist(sngss7_rmt_cong);
		hashtable_destroy(sngss7_rmt_cong->ss7_active_calls);

		/* Destroy mutex */
		ftdm_mutex_destroy(&sngss7_rmt_cong->mutex);
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
		SS7_DEBUG("De-allocation ACC Hashlists\n");
		/* free all the members of ACC hash list */
		sngss7_free_acc_hashlist();
		/* delete the acc hash table */
		hashtable_destroy(ss7_rmtcong_lst);
		ss7_rmtcong_lst = NULL;
	}
	SS7_DEBUG("De-allocation Complete\n");
}

#ifdef ACC_TEST
/******************************************************************************/
/* Print ACC statistics into a file */
ftdm_status_t sng_prnt_acc_debug(uint32_t dpc)
{
	char hash_dpc[10] = { 0 };
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	time_t curr_time = time(NULL);

	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	if (!dpc) {
		SS7_ERROR("NSG-ACC: Invalid dpc[%d]. Thus, cannot log.\n", dpc);
		return FTDM_FAIL;
	}

	snprintf(hash_dpc, sizeof(hash_dpc), "%d", dpc);

	/* Get remote congestion information for respective DPC from hash list */
	sngss7_rmt_cong = hashtable_search(ss7_rmtcong_lst, (void *)hash_dpc);

	if (!sngss7_rmt_cong) {
		SS7_ERROR("NSG-ACC: DPC[%d] is not preset in ACC hash list. Thus, can not log.\n", dpc);
		return FTDM_FAIL;
	}

	if (sngss7_rmt_cong->log_file_ptr == NULL) {
		SS7_ERROR("NSG-ACC: Invalid file descriptor for dpc[%d]\n", dpc);
		return FTDM_FAIL;
	}

	if (!sngss7_rmt_cong->debug_idx) {
		/* Print only first time */
		fprintf(sngss7_rmt_cong->log_file_ptr, "MaxBktSize, RcvIAM, priRcvIAM, transIAM, priTransIAM, recvREL, acl1REL, acl2REL, accLvl, blkRate, blkCalls, allowCalls, Time\n");
		sngss7_rmt_cong->debug_idx = 1;
	}

	time(&curr_time);
	fprintf(sngss7_rmt_cong->log_file_ptr, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %s",
			sngss7_rmt_cong->max_bkt_size, sngss7_rmt_cong->iam_recv, sngss7_rmt_cong->iam_pri_recv, sngss7_rmt_cong->iam_trans,
			sngss7_rmt_cong->iam_pri_trans, sngss7_rmt_cong->rel_recv, sngss7_rmt_cong->rel_rcl1_recv,
			sngss7_rmt_cong->rel_rcl2_recv, sngss7_rmt_cong->sngss7_rmtCongLvl, sngss7_rmt_cong->call_blk_rate,
			sngss7_rmt_cong->calls_rejected, sngss7_rmt_cong->calls_passed, ctime(&curr_time));

	/* Resetting counters to 0 inorder to get every 1 second data */
	ftdm_mutex_lock(sngss7_rmt_cong->mutex);
	sngss7_rmt_cong->iam_recv = 0;
	sngss7_rmt_cong->iam_pri_recv = 0;
	sngss7_rmt_cong->iam_trans = 0;
	sngss7_rmt_cong->iam_pri_trans = 0;
	sngss7_rmt_cong->rel_recv = 0;
	sngss7_rmt_cong->rel_rcl1_recv = 0;
	sngss7_rmt_cong->rel_rcl2_recv = 0;
	sngss7_rmt_cong->calls_rejected = 0;
	sngss7_rmt_cong->calls_passed = 0;
	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

	return FTDM_SUCCESS;
}

/******************************************************************************/
/* Increment parameters required in order to Debug ACC */
ftdm_status_t sng_increment_acc_statistics(ftdm_channel_t *ftdmchan, uint32_t acc_debug_lvl)
{
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	if (!ftdmchan) {
		return FTDM_FAIL;
	}

	if (!(sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan))) {
		SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: Could not find congestion structure for DPC[%d]... DEBUG statistics\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
		return FTDM_FAIL;
	}

	ftdm_mutex_lock(sngss7_rmt_cong->mutex);
	switch (acc_debug_lvl) {
		case ACC_IAM_RECV_DEBUG:
			sngss7_rmt_cong->iam_recv++;
			break;

		case ACC_IAM_PRI_RECV_DEBUG:
			sngss7_rmt_cong->iam_pri_recv++;
			break;

		case ACC_IAM_TRANS_DEBUG:
			sngss7_rmt_cong->iam_trans++;
			break;

		case ACC_IAM_TRANS_PRI_DEBUG:
			sngss7_rmt_cong->iam_pri_trans++;
			break;

		case ACC_REL_RECV_DEBUG:
			sngss7_rmt_cong->rel_recv++;
			break;

		case ACC_REL_RECV_ACL1_DEBUG:
			sngss7_rmt_cong->rel_rcl1_recv++;
			break;

		case ACC_REL_RECV_ACL2_DEBUG:
			sngss7_rmt_cong->rel_rcl2_recv++;
			break;

		default:
			SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: Invalid ACC DEBUG option for DPC[%d]\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
			break;
	}
	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);


	return FTDM_SUCCESS;
}
#endif

/******************************************************************************/
/* Configure the call block rate as per number of cics configured per DPC basis */
ftdm_status_t sng_acc_active_call_insert(ftdm_channel_t *ftdmchan)
{
	ftdm_status_t ret = FTDM_FAIL;
	char span_chan[12] = { 0 };
	ftdm_sngss7_active_calls_t *sngss7_active_call = NULL;
	ftdm_sngss7_rmt_cong_t* sngss7_rmt_cong = NULL;
	char* key = NULL;

	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return ret;

	sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan);

	if (!sngss7_rmt_cong) {
		return ret;
	}

	if (!sngss7_rmt_cong->ss7_active_calls) {
		return ret;
	}

	ftdm_mutex_lock(sngss7_rmt_cong->mutex);

	snprintf(span_chan, sizeof(span_chan), "s%dc%d", ftdmchan->span_id, ftdmchan->chan_id);
	if (hashtable_search(sngss7_rmt_cong->ss7_active_calls, (void *)span_chan)) {
		SS7_ERROR_CHAN(ftdmchan, "NSG-ACC: Entry already present in Cong active call list %s\n", "");
		goto done;
	}

	sngss7_active_call = ftdm_calloc(sizeof(*sngss7_active_call), 1);

	if (!sngss7_active_call) {
		SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: Unable to allocate memory to insert active calls %s\n", "");
		goto done;
	}

	/* Initializing structure variables */
	sngss7_active_call->span_id = ftdmchan->span_id;
	sngss7_active_call->chan_id = ftdmchan->chan_id;

	key = ftdm_strdup(span_chan);

	/* Insert the same in ACC hash list */
	hashtable_insert(sngss7_rmt_cong->ss7_active_calls, (void *)key, sngss7_active_call, HASHTABLE_FLAG_FREE_KEY);
	SS7_INFO_CHAN(ftdmchan, "NSG-ACC: Successfully inserted in active calls hash list %s\n", "");
	ret=FTDM_SUCCESS;

done:
	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);
	return ret;
}

/******************************************************************************/
/* If call is present in active call list then remove it and clear the call */
ftdm_status_t sng_acc_rmv_active_call(ftdm_channel_t *ftdmchan)
{
	ftdm_sngss7_rmt_cong_t* sngss7_rmt_cong = NULL;
	ftdm_sngss7_active_calls_t *sngss7_active_call = NULL;
	char key[25] = { 0 };

	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan);

	if (!sngss7_rmt_cong) {
		return FTDM_FAIL;
	}

	if (!sngss7_rmt_cong->ss7_active_calls) {
		return FTDM_FAIL;
	}

	ftdm_mutex_lock(sngss7_rmt_cong->mutex);

	snprintf(key, sizeof(key), "s%dc%d", ftdmchan->span_id, ftdmchan->chan_id);

	if ((sngss7_active_call = hashtable_search(sngss7_rmt_cong->ss7_active_calls, (void *)key)) == NULL) {
		SS7_ERROR_CHAN(ftdmchan, "NSG-ACC: Entry NOT present in Cong active call list %s\n", "");
		ftdm_mutex_unlock(sngss7_rmt_cong->mutex);
		return FTDM_FAIL;
	}

	SS7_DEBUG("NSG-ACC: Removing %s entry from active calls hash list\n", key);
	hashtable_remove(sngss7_rmt_cong->ss7_active_calls, (void *)key);
	ftdm_safe_free(sngss7_active_call);

	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

	return FTDM_SUCCESS;
}

/******************************************************************************/
/* Iterate through Active Calls hash list and remove hash list members one by one */
ftdm_status_t sng_acc_free_active_calls_hashlist(ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong)
{
	ftdm_hash_iterator_t *i = NULL;
	const void *key = NULL;
	void *val = NULL;
	ftdm_sngss7_active_calls_t *sngss7_active_call = NULL;
	char actv_call[25] = { 0 };
	ftdm_status_t ret = FTDM_FAIL;

	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return ret;

	if (!sngss7_rmt_cong) {
		return ret;
	}

	if (!sngss7_rmt_cong->ss7_active_calls) {
		return ret;
	}

	ftdm_mutex_lock(sngss7_rmt_cong->mutex);

	for (i = hashtable_first(sngss7_rmt_cong->ss7_active_calls); i; i = hashtable_next(i)) {
		memset(actv_call, 0, sizeof(actv_call));

		hashtable_this(i, &key, NULL, &val);
		if (!key || !val) {
			break;
		}
		sngss7_active_call = (ftdm_sngss7_active_calls_t*)val;

		snprintf(actv_call, sizeof(actv_call), "s%dc%d", sngss7_active_call->span_id, sngss7_active_call->chan_id);
		SS7_DEBUG("NSG-ACC: Removing %s entry from active calls hash list\n", actv_call);
		hashtable_remove(sngss7_rmt_cong->ss7_active_calls, (void *)actv_call);
		ftdm_safe_free(sngss7_active_call);
	}
	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

	return FTDM_SUCCESS;
}

/* Increment the number of received call counter */
ftdm_status_t ftdm_sangoma_ss7_received_call(ftdm_channel_t *ftdmchan)
{
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;

	if (!g_ftdm_sngss7_data.cfg.sng_acc)
		return FTDM_FAIL;

	if (!(sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan))) {
		SS7_ERROR_CHAN(ftdmchan,"NSG-ACC: Could not find congestion structure for DPC[%d].\n", g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
		return FTDM_FAIL;
	}

	/* If ACC call rate timer is running then only increase number of calls received */
	ftdm_mutex_lock(sngss7_rmt_cong->mutex);

	if (sngss7_rmt_cong->acc_call_rate.tmr_id) {
		sngss7_rmt_cong->calls_received++;
		SS7_DEBUG_CHAN(ftdmchan, "NSG-ACC: Number of calls received till now is %d for dpc[%d]\n",
				sngss7_rmt_cong->calls_received, g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc);
	}

	ftdm_mutex_unlock(sngss7_rmt_cong->mutex);

	return FTDM_SUCCESS;
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
