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
 * David Yat Sin <dyatsin@sangoma.com>
 * James Zhang <jzhang@sangoma.com>
 *
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/

/* GLOBALS ********************************************************************/
static sng_isup_event_interface_t sng_event;
static ftdm_io_interface_t g_ftdm_sngss7_interface;
ftdm_sngss7_data_t g_ftdm_sngss7_data;
ftdm_sngss7_opr_mode g_ftdm_operating_mode;
ftdm_sngss7_call_reject_queue_t  sngss7_reject_queue;	/* Call reject queue */

/******************************************************************************/

/* PROTOTYPES *****************************************************************/
static void *ftdm_sangoma_ss7_run (ftdm_thread_t * me, void *obj);
static void ftdm_sangoma_ss7_process_peer_stack_event (ftdm_channel_t *ftdmchan, sngss7_event_data_t *sngss7_event);

static ftdm_status_t ftdm_sangoma_ss7_start (ftdm_span_t * span);
static void ftdm_sangoma_ss7_cleanup(ftdm_span_t *ftdmspan);
/******************************************************************************/


/* STATE MAP ******************************************************************/
ftdm_state_map_t sangoma_ss7_state_map = {
  {
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_END},
	{FTDM_CHANNEL_STATE_RESTART, FTDM_CHANNEL_STATE_DOWN,
	 FTDM_CHANNEL_STATE_IN_LOOP, FTDM_CHANNEL_STATE_COLLECT,
	 FTDM_CHANNEL_STATE_RING, FTDM_CHANNEL_STATE_PROGRESS,
	 FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP,
	 FTDM_CHANNEL_STATE_CANCEL, FTDM_CHANNEL_STATE_TERMINATING,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_RESTART, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_TERMINATING,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_HANGUP_COMPLETE,
	 FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_IDLE,
	 FTDM_CHANNEL_STATE_IN_LOOP, FTDM_END}
	},
	{
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_IDLE, FTDM_END},
	{FTDM_CHANNEL_STATE_RESTART, FTDM_CHANNEL_STATE_COLLECT, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_DOWN, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_COLLECT, FTDM_CHANNEL_STATE_IN_LOOP, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_IN_LOOP, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_COLLECT, FTDM_CHANNEL_STATE_DOWN,
	 FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_COLLECT, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_CANCEL, FTDM_CHANNEL_STATE_RING, 
	 FTDM_CHANNEL_STATE_IDLE, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_RING, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
	 FTDM_CHANNEL_STATE_RINGING, FTDM_CHANNEL_STATE_PROGRESS, 
	 FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_CHANNEL_STATE_UP,
	 FTDM_END}
	},
	{
	 ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_RINGING, FTDM_END},
	{FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
	 FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_PROGRESS_MEDIA,
	 FTDM_CHANNEL_STATE_UP, FTDM_END},
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_PROGRESS, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
	 FTDM_CHANNEL_STATE_UP,
	 FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
	 FTDM_CHANNEL_STATE_UP, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_UP, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_CANCEL, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_TERMINATING, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_HANGUP, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_END}
	},
   {
	ZSD_INBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_DOWN, FTDM_END}
	},
	/**************************************************************************/
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_END},
	{FTDM_CHANNEL_STATE_RESTART, FTDM_CHANNEL_STATE_DOWN,
	 FTDM_CHANNEL_STATE_IN_LOOP, FTDM_CHANNEL_STATE_DIALING,
	 FTDM_CHANNEL_STATE_PROGRESS, FTDM_CHANNEL_STATE_PROGRESS_MEDIA,
	 FTDM_CHANNEL_STATE_UP, FTDM_CHANNEL_STATE_CANCEL,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
	 FTDM_CHANNEL_STATE_RINGING,
	 FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_RESTART, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_TERMINATING,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_HANGUP_COMPLETE,
	 FTDM_CHANNEL_STATE_DOWN, FTDM_CHANNEL_STATE_IDLE,
	 FTDM_CHANNEL_STATE_IN_LOOP, FTDM_END}
	},
	{
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_IDLE, FTDM_END},
	{FTDM_CHANNEL_STATE_RESTART, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_DOWN, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_DIALING, FTDM_CHANNEL_STATE_IN_LOOP, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_IN_LOOP, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_DIALING, FTDM_CHANNEL_STATE_DOWN, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_RINGING, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_CANCEL, FTDM_CHANNEL_STATE_TERMINATING,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_PROGRESS,
	 FTDM_CHANNEL_STATE_PROGRESS_MEDIA ,FTDM_CHANNEL_STATE_UP, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_DIALING, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_CANCEL, FTDM_CHANNEL_STATE_TERMINATING,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_CHANNEL_STATE_PROGRESS,
	 FTDM_CHANNEL_STATE_RINGING,
	 FTDM_CHANNEL_STATE_PROGRESS_MEDIA ,FTDM_CHANNEL_STATE_UP,
	 FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_PROGRESS, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
	 FTDM_CHANNEL_STATE_UP,
	 FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_PROGRESS_MEDIA, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP,
	 FTDM_CHANNEL_STATE_UP, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_UP, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_TERMINATING, FTDM_CHANNEL_STATE_HANGUP, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_CANCEL, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_TERMINATING, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_HANGUP, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_HANGUP, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_END}
	},
   {
	ZSD_OUTBOUND,
	ZSM_UNACCEPTABLE,
	{FTDM_CHANNEL_STATE_HANGUP_COMPLETE, FTDM_END},
	{FTDM_CHANNEL_STATE_SUSPENDED, FTDM_CHANNEL_STATE_RESTART,
	 FTDM_CHANNEL_STATE_DOWN, FTDM_END}
	},
   }
};

static void handle_hw_alarm(ftdm_event_t *e)
{
	sngss7_chan_data_t *ss7_info = NULL;
	ftdm_channel_t *ftdmchan = NULL;
	int x = 0;

	ftdm_assert(e != NULL, "Null event!\n");
					
	SS7_DEBUG("handle_hw_alarm event [%d/%d]\n",e->channel->physical_span_id,e->channel->physical_chan_id);

	for (x = (g_ftdm_sngss7_data.cfg.procId * MAX_CIC_MAP_LENGTH) + 1; g_ftdm_sngss7_data.cfg.isupCkt[x].id != 0; x++) {
		if (g_ftdm_sngss7_data.cfg.isupCkt[x].type == SNG_CKT_VOICE) {
			ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[x].obj;

			/* NC. Its possible for alarms to come in the middle of configuration
			   especially on large systems */
			if (!ss7_info || !ss7_info->ftdmchan) {
				SS7_DEBUG("handle_hw_alarm: Invalid ss7_info/ftdmchan pointer "
                        "ckt=%i x=%i - ss7_info=%p ftdmchan=%p\n",
						g_ftdm_sngss7_data.cfg.isupCkt[x].id,x,
						ss7_info?ss7_info:NULL,
                        ss7_info?ss7_info->ftdmchan:NULL);
				continue;
			}

			ftdmchan = ss7_info->ftdmchan;
			
			if (e->channel->physical_span_id == ftdmchan->physical_span_id && 
			    e->channel->physical_chan_id == ftdmchan->physical_chan_id) {
				SS7_DEBUG_CHAN(ftdmchan,"handle_hw_alarm: span=%i chan=%i ckt=%i x=%i\n",
						ftdmchan->physical_span_id,ftdmchan->physical_chan_id,g_ftdm_sngss7_data.cfg.isupCkt[x].id,x);
				if (e->enum_id == FTDM_OOB_ALARM_TRAP) {
					SS7_DEBUG_CHAN(ftdmchan,"handle_hw_alarm: Set FLAG_GRP_HW_BLOCK_TX %s\n", " ");
					sngss7_set_ckt_blk_flag(ss7_info, FLAG_GRP_HW_BLOCK_TX);
					if (ftdmchan->state != FTDM_CHANNEL_STATE_SUSPENDED) {
						ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
					}
				} else if (e->enum_id == FTDM_OOB_ALARM_CLEAR) {
					SS7_DEBUG_CHAN(ftdmchan,"handle_hw_alarm: Clear %s \n", " ");
					if (sngss7_test_ckt_blk_flag(ss7_info, FLAG_GRP_HW_BLOCK_TX_DN)) {
						sngss7_clear_ckt_blk_flag(ss7_info, FLAG_GRP_HW_BLOCK_TX);
						sngss7_clear_ckt_blk_flag(ss7_info, FLAG_GRP_HW_BLOCK_TX_DN);
					}
#if 0
					sngss7_clear_ckt_blk_flag(ss7_info, FLAG_GRP_HW_UNBLK_TX);
					if (sngss7_test_ckt_blk_flag(ss7_info, FLAG_GRP_HW_BLOCK_TX_DN)) {
#endif
						sngss7_set_ckt_blk_flag(ss7_info, FLAG_GRP_HW_UNBLK_TX);
						SS7_DEBUG_CHAN(ftdmchan,"handle_hw_alarm: Setting FLAG_GRP_HW_UNBLK_TX %s\n"," ");
						if (ftdmchan->state != FTDM_CHANNEL_STATE_SUSPENDED) {
							ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
						}
#if 0
					}
#endif
				}
			}
		}
	}
}

/******************************************************************************/
/* Dequeue calls at a particular rate when remote is in congested
 * Dequeue calls when local is congested and send stop */
static void *app_sngss7_call_queue_handler(ftdm_thread_t * me, void *obj)
{
	ftdm_channel_t *ftdmchan = NULL;
	sngss7_chan_data_t *sngss7_info = NULL;

	/*  When there is a Local/Remote congestion then some delay is required
	 *  in order to complete thread processing to set channel state properly
	 *  and the reject calls with the appropriate hangup cause code */
	while (ftdm_running()) {
		ftdmchan = NULL;

		/* Dequeue all the calls which needs to be rejected due to local congestion or high CPU usage
		 * and send STOP signal in order to reject the call */
		while ((ftdmchan = ftdm_queue_dequeue(sngss7_reject_queue.sngss7_call_rej_queue))) {
			sngss7_info = ftdmchan->call_data;
			SS7_DEBUG_CHAN(ftdmchan, "Rejecting Call on span %d and chan %d... Due to congestion\n", ftdmchan->span_id, ftdmchan->chan_id);
			/* in case of native bridge */
			if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {
				SS7_DEBUG_CHAN(ftdmchan, "Changing span[%d] chan[%d] state to Terminating in Native Bridge\n", ftdmchan->span_id, ftdmchan->chan_id);
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_TERMINATING);
			} else {
				SS7_DEBUG_CHAN(ftdmchan, "Changing span[%d] chan[%d] state to Hangup \n", ftdmchan->span_id, ftdmchan->chan_id);
				sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_STOP);
				/*ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);*/
			}
		}
		ftdm_sleep(100);
	}

	return NULL;
}


static void check_span_oob_events(ftdm_span_t *ftdmspan)
{
	ftdm_event_t 		*event = NULL;
	/* Poll for events, e.g HW DTMF */
	switch (ftdm_span_poll_event(ftdmspan, 0, NULL)) {
	/**********************************************************************/
	case FTDM_SUCCESS:
		while (ftdm_span_next_event(ftdmspan, &event) == FTDM_SUCCESS) {
			if (event->e_type == FTDM_EVENT_OOB) {
				handle_hw_alarm(event);
			}
		}
		break;
	/**********************************************************************/
	case FTDM_TIMEOUT:
		/* No events pending */
		break;
	/**********************************************************************/
	default:
		SS7_ERROR("%s:Failed to poll span event\n", ftdmspan->name);
	/**********************************************************************/
	}
}

/* MONITIOR THREADS ***********************************************************/
static void *ftdm_sangoma_ss7_run(ftdm_thread_t * me, void *obj)
{
	ftdm_interrupt_t	*ftdm_sangoma_ss7_int[2];
	ftdm_span_t 		*ftdmspan = (ftdm_span_t *) obj;
	ftdm_channel_t 		*ftdmchan = NULL;
	sngss7_event_data_t     *sngss7_event = NULL;
	sngss7_span_data_t	*sngss7_span = (sngss7_span_data_t *)ftdmspan->signal_data;


	int b_alarm_test = 1;
	sngss7_chan_data_t *ss7_info=NULL;

	ftdm_log (FTDM_LOG_INFO, "ftmod_sangoma_ss7 monitor thread for span=%u started.\n", ftdmspan->span_id);

	/* set IN_THREAD flag so that we know this thread is running */
	ftdm_set_flag (ftdmspan, FTDM_SPAN_IN_THREAD);

	/* Create a global Call handling queue.
	 * The app event handler is allowed to run FreeTDM api commands directly */
	if (g_ftdm_sngss7_data.cfg.sng_acc) {
		if (ftdm_thread_create_detached(app_sngss7_call_queue_handler, NULL) != FTDM_SUCCESS) {
			ftdm_log(FTDM_LOG_ERROR, "Failed to create message queue handler thread. \n");
			goto ftdm_sangoma_ss7_run_exit;
		}
	}

	/* get an interrupt queue for this span for channel state changes */
	if (ftdm_queue_get_interrupt (ftdmspan->pendingchans, &ftdm_sangoma_ss7_int[0]) != FTDM_SUCCESS) {
		SS7_CRITICAL ("Failed to get a ftdm_interrupt for span = %d for channel state changes!\n", ftdmspan->span_id);
		goto ftdm_sangoma_ss7_run_exit;
	}

	/* get an interrupt queue for this span for Trillium events */
	if (ftdm_queue_get_interrupt (sngss7_span->event_queue, &ftdm_sangoma_ss7_int[1]) != FTDM_SUCCESS) {
		SS7_CRITICAL ("Failed to get a ftdm_interrupt for span = %d for Trillium event queue!\n", ftdmspan->span_id);
		goto ftdm_sangoma_ss7_run_exit;
	}

	if(SNG_SS7_OPR_MODE_M2UA_SG == g_ftdm_operating_mode){
		ftdm_log (FTDM_LOG_INFO, "FreeTDM running as M2UA_SG mode, freetdm dont have to do anything \n"); 

		while (ftdm_running () && !(ftdm_test_flag (ftdmspan, FTDM_SPAN_STOP_THREAD))) {

			switch ((ftdm_interrupt_multiple_wait(ftdm_sangoma_ss7_int, ftdm_array_len(ftdm_sangoma_ss7_int), 100))) {

				case FTDM_SUCCESS:	/* process all pending state changes */

					SS7_DEVEL_DEBUG ("ftdm_interrupt_wait FTDM_SUCCESS on span = %d\n",ftdmspan->span_id);

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
			}
			check_span_oob_events(ftdmspan);

			/* signal the core that sig events are queued for processing */
			ftdm_span_trigger_signals(ftdmspan);
		}
		goto ftdm_sangoma_ss7_stop;
	}

	while (ftdm_running () && !(ftdm_test_flag (ftdmspan, FTDM_SPAN_STOP_THREAD))) {
		int x = 0;
		if (b_alarm_test) {
			b_alarm_test = 0;
			for (x = (g_ftdm_sngss7_data.cfg.procId * MAX_CIC_MAP_LENGTH) + 1; 
			     g_ftdm_sngss7_data.cfg.isupCkt[x].id != 0; x++) {	
				if (g_ftdm_sngss7_data.cfg.isupCkt[x].type == SNG_CKT_VOICE) {
					ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[x].obj;
					ftdmchan = ss7_info->ftdmchan;
					if (!ftdmchan) {
						continue;
					}

					if (ftdmchan->alarm_flags != 0) { /* we'll send out block */
						sngss7_set_ckt_blk_flag(ss7_info, FLAG_GRP_HW_BLOCK_TX );
					}  else { /* we'll send out reset */
						if (sngss7_test_ckt_blk_flag(ss7_info, FLAG_GRP_HW_BLOCK_TX )) {
							sngss7_clear_ckt_blk_flag( ss7_info, FLAG_GRP_HW_BLOCK_TX );
							sngss7_clear_ckt_blk_flag( ss7_info, FLAG_GRP_HW_BLOCK_TX_DN );
							sngss7_set_ckt_blk_flag (ss7_info, FLAG_GRP_HW_UNBLK_TX);
							SS7_DEBUG("b_alarm_test FLAG_GRP_HW_UNBLK_TX\n");
						}
					}
				}
				usleep(50);
			}
			ftdmchan = NULL;
		}

		/* check the channel state queue for an event*/	
		switch ((ftdm_interrupt_multiple_wait(ftdm_sangoma_ss7_int, ftdm_array_len(ftdm_sangoma_ss7_int), 100))) {
		/**********************************************************************/
		case FTDM_SUCCESS:	/* process all pending state changes */

			/* clean out all pending channel state changes */
			while ((ftdmchan = ftdm_queue_dequeue (ftdmspan->pendingchans))) {
				sngss7_chan_data_t *chan_info = ftdmchan->call_data;
				
				/*first lock the channel */
				ftdm_mutex_lock(ftdmchan->mutex);

				/* process state changes for this channel until they are all done */
				ftdm_channel_advance_states(ftdmchan);

				if (chan_info->peer_data) {
					/* clean out all pending stack events in the peer channel */
					while ((sngss7_event = ftdm_queue_dequeue(chan_info->event_queue))) {
						ftdm_sangoma_ss7_process_peer_stack_event(ftdmchan, sngss7_event);
					       	ftdm_safe_free(sngss7_event);
					}
				}
 
				/* unlock the channel */
				ftdm_mutex_unlock (ftdmchan->mutex);				
			}

			/* clean out all pending stack events */
			while ((sngss7_event = ftdm_queue_dequeue(sngss7_span->event_queue))) {
				ftdm_sangoma_ss7_process_stack_event(sngss7_event);
				ftdm_safe_free(sngss7_event);
			}

			/* signal the core that sig events are queued for processing */
			ftdm_span_trigger_signals(ftdmspan);

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
		}

		/* check if there is a GRA to proccess on the span */
		if (ftdm_test_flag(sngss7_span, SNGSS7_RX_GRA_PENDING)) {
			check_if_rx_gra_started(ftdmspan);
		}

		/* check if there is a GRS being processed on the span */
		if (ftdm_test_flag(sngss7_span, SNGSS7_RX_GRS_PENDING)) {
			/* check if the rx_grs has started */
			check_if_rx_grs_started(ftdmspan);

			/* check if the rx_grs has cleared */
			check_if_rx_grs_processed(ftdmspan);
		}

		/* check if there is a UCIC to be processed on the span */
		if (ftdm_test_flag(sngss7_span, SNGSS7_UCIC_PENDING)) {
			/* process the span wide UCIC */
			process_span_ucic(ftdmspan);
		}

		/* check each channel on the span to see if there is an un-procressed SUS/RES flag */
		check_for_res_sus_flag(ftdmspan);

		/* check each channel on the span to see if it needs to be reconfigured */
		check_for_reconfig_flag(ftdmspan);
		
		check_span_oob_events(ftdmspan);
	}
ftdm_sangoma_ss7_stop:

	ftdm_sangoma_ss7_cleanup(ftdmspan);
	return NULL;

ftdm_sangoma_ss7_run_exit:

	ftdm_sangoma_ss7_cleanup(ftdmspan);
	ftdm_sangoma_ss7_stop (ftdmspan);

	return NULL;
}

/* Perform SS7 Threads clean up */
static void ftdm_sangoma_ss7_cleanup(ftdm_span_t* ftdmspan)
{
	/* Stop all threads */
	ftdm_sleep(1000);

	if (g_ftdm_sngss7_data.cfg.sng_acc) {
		sngss7_free_acc();
	}

	/* clear the IN_THREAD flag so that we know the thread is done */
	ftdm_clear_flag (ftdmspan, FTDM_SPAN_IN_THREAD);

	ftdm_log (FTDM_LOG_INFO,"ftmod_sangoma_ss7 monitor thread for span=%u stopping due to error.\n",ftdmspan->span_id);
}

/*
This function moves events from a channel queue into its peer queue during native bridging

Note this should happen only once during a call. This is needed because the incoming leg of
a call (leg A) may receive multiple events (ie, IAM -> SAM -> SAM -> ITX) before the outbound
leg (leg B) is created. It is a small time frame, depending on how long does the call routing
takes. Once the B-leg is created, this transfer will occur either when the A-leg receives a
new incoming message, or when the A-leg receives a message from its peer (B-leg, ie ACM)
*/
static void sngss7_transfer_peer_events(sngss7_chan_data_t *sngss7_info, ftdm_bool_t wakeup)
{
	ftdm_channel_t *ftdmchan = sngss7_info->ftdmchan;
	ftdm_span_t *peer_span = sngss7_info->peer_data->ftdmchan->span;
	sngss7_event_data_t *peer_event = NULL;
	int qi = 0;
	int cnt = 0;
	if (!sngss7_info->peer_data) {
		SS7_CRIT_CHAN(ftdmchan,"[CIC:%d]Wrong ss7 info, missing peer data!\n", sngss7_info->circuit->cic);
		return;
	}
	SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Transferring %d messages into my peer's queue ...\n",
			sngss7_info->circuit->cic, sngss7_info->peer_event_transfer_cnt);
	/* Transfer any messages we enqueued in our own queue to my peer's queue, peer_event_transfer_cnt
	   is the number of messages in our queue that actually belong to the peer (top to bottom)  */
	for (qi = 0; qi < sngss7_info->peer_event_transfer_cnt; qi++) {
		peer_event = ftdm_queue_dequeue(sngss7_info->event_queue);
		if (peer_event) {
			ftdm_queue_enqueue(sngss7_info->peer_data->event_queue, peer_event);
			cnt++;
		} else {
			/* This should never happen! */
			SS7_CRIT_CHAN(ftdmchan,"[CIC:%d]What!? someone stole my messages!\n", sngss7_info->circuit->cic);
			break;
		}
	}
	SS7_DEBUG_CHAN(ftdmchan,"[CIC:%d]Transferred %d messages into my peer's queue (out of %d)\n",
			sngss7_info->circuit->cic, cnt, sngss7_info->peer_event_transfer_cnt);
	sngss7_info->peer_event_transfer_cnt = 0;
	if (wakeup) {
		/* wake the peer up so it consumes the events we just transferred */
		ftdm_queue_enqueue(peer_span->pendingchans, sngss7_info->peer_data->ftdmchan);
	}
}

/******************************************************************************/
void ftdm_sangoma_ss7_process_stack_event (sngss7_event_data_t *sngss7_event)
{
	sngss7_chan_data_t *sngss7_info = NULL;
	ftdm_channel_t *ftdmchan = NULL;
	sngss7_event_data_t *event_clone = NULL;
	int clone_event = 0;

	/* get the ftdmchan and ss7_chan_data from the circuit */
	if (extract_chan_data(sngss7_event->circuit, &sngss7_info, &ftdmchan)) {
		SS7_ERROR("Failed to extract channel data for circuit = %d!\n", sngss7_event->circuit);
		return;
	}

	/* now that we have the right channel ... put a lock on it so no-one else can use it */
	ftdm_channel_lock(ftdmchan);

	/* while there's a state change present on this channel process it */
	ftdm_channel_advance_states(ftdmchan);

	if (sngss7_event->event_id == SNGSS7_CON_IND_EVENT) {
		clone_event++;
	}

	/* If the call has already started (we only bridge events related to calls)
	 * and the event is not a release confirmation, then clone the event.
	 * We do not clone release cfm events because that is the only event (final event) that is not
	 * bridged to the other leg, the first Spirou customer we had explicitly requested to send
	 * release confirm as soon as the release is received and therefore not wait for the other leg
	 * to send release confirm (hence, not need to clone and enqueue in the other leg) */
	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_CALL_STARTED) && sngss7_event->event_id != SNGSS7_REL_CFM_EVENT) {
		clone_event++;
	}

	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {

		if (sngss7_event->event_id == SNGSS7_SUSP_IND_EVENT) {
			sngss7_set_ckt_flag(sngss7_info, FLAG_SUS_RECVD);
			sngss7_clear_ckt_flag(sngss7_info, FLAG_T6_CANCELED);
		}

		if (sngss7_test_ckt_flag(sngss7_info, FLAG_SUS_RECVD) && 
		   !sngss7_test_ckt_flag(sngss7_info, FLAG_T6_CANCELED)) {
			if (sng_cancel_isup_tmr(sngss7_info->suInstId, ISUP_T6i) == RFAILED ) {
				SS7_ERROR_CHAN(ftdmchan,"[CIC:%d]could not stop timer T6 \n", sngss7_info->circuit->cic);
			} else {
				sngss7_set_ckt_flag(sngss7_info, FLAG_T6_CANCELED);
				SS7_ERROR_CHAN(ftdmchan,"[CIC:%d] isup timer T6 has been cancelled. \n", sngss7_info->circuit->cic);
			}
		}
	}

	/* clone the event and save it for later usage, we do not clone RLC messages */
	if (clone_event) {
		event_clone = ftdm_calloc(1, sizeof(*sngss7_event));
		if (event_clone) {
			memcpy(event_clone, sngss7_event, sizeof(*sngss7_event));
			/* if we have already a peer channel then enqueue the event in their queue */
			if (sngss7_info->peer_data) {
				ftdm_span_t *peer_span = sngss7_info->peer_data->ftdmchan->span;
				if (sngss7_info->peer_event_transfer_cnt) {
					sngss7_transfer_peer_events(sngss7_info, FTDM_FALSE);
				}
				/* we already have a peer attached, wake him up */
				ftdm_queue_enqueue(sngss7_info->peer_data->event_queue, event_clone);
				ftdm_queue_enqueue(peer_span->pendingchans, sngss7_info->peer_data->ftdmchan);
			} else {
				/* we don't have a peer yet, save the event on our own queue for later
				 * only the first event in this queue is directly consumed by our peer (IAM), subsequent events
				 * must be transferred by us to their queue as soon as we find our peer */
				ftdm_queue_enqueue(sngss7_info->event_queue, event_clone);
				if (sngss7_event->event_id != SNGSS7_CON_IND_EVENT) {
					/* This could be an SAM, save it for transfer once we know who our peer is (if we ever find that) */
					sngss7_info->peer_event_transfer_cnt++;
				}
			}
		}
	}

	/* we could test for sngss7_info->peer_data too, bit this flag is set earlier, the earlier we know the better */
	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {
		/* most messages are simply relayed in sig bridge mode, except for hangup which requires state changing */
		switch (sngss7_event->event_id) {
		case SNGSS7_REL_IND_EVENT:
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_TERMINATING);
			break;
		case SNGSS7_REL_CFM_EVENT:
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
			break;
		default:
			break;
		}
		goto done;
	}

	/* figure out the type of event and send it to the right handler */
	switch (sngss7_event->event_id) {
	/**************************************************************************/
	case (SNGSS7_CON_IND_EVENT):
		handle_con_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siConEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_CON_CFM_EVENT):
		handle_con_cfm(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siConEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_CON_STA_EVENT):
		handle_con_sta(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siCnStEvnt, sngss7_event->evntType);
		break;
	/**************************************************************************/
	case (SNGSS7_REL_IND_EVENT):
		handle_rel_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siRelEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_REL_CFM_EVENT):
		handle_rel_cfm(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siRelEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_DAT_IND_EVENT):
		handle_dat_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siInfoEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_FAC_IND_EVENT):
		handle_fac_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit, sngss7_event->evntType,  &sngss7_event->event.siFacEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_FAC_CFM_EVENT):
		handle_fac_cfm(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit, sngss7_event->evntType,  &sngss7_event->event.siFacEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_UMSG_IND_EVENT):
		handle_umsg_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit);
		break;
	/**************************************************************************/
	case (SNGSS7_STA_IND_EVENT):
		handle_sta_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit, sngss7_event->globalFlg, sngss7_event->evntType,  &sngss7_event->event.siStaEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_SUSP_IND_EVENT):
		handle_susp_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siSuspEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_RESM_IND_EVENT):
		handle_resm_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siResmEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_SSP_STA_CFM_EVENT):
		SS7_ERROR("dazed and confused ... hu?!\n");
		break;
	/**************************************************************************/
	default:
		SS7_ERROR("Unknown Event Id!\n");
		break;
	/**************************************************************************/
	}

done:
	/* while there's a state change present on this channel process it */
	ftdm_channel_advance_states(ftdmchan);

	/* unlock the channel */
	ftdm_channel_unlock(ftdmchan);

}

FTDM_ENUM_NAMES(SNG_EVENT_TYPE_NAMES, SNG_EVENT_TYPE_STRINGS)
FTDM_STR2ENUM(ftdm_str2sngss7_event, ftdm_sngss7_event2str, sng_event_type_t, SNG_EVENT_TYPE_NAMES, SNGSS7_INVALID_EVENT)
static void ftdm_sangoma_ss7_process_peer_stack_event (ftdm_channel_t *ftdmchan, sngss7_event_data_t *sngss7_event)
{
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;

	if (ftdmchan->state < FTDM_CHANNEL_STATE_UP && ftdmchan->state != FTDM_CHANNEL_STATE_DOWN) {
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_UP);
		ftdm_channel_advance_states(ftdmchan);
	}

	SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Receiving message %s from bridged peer (our state = %s)\n",
			sngss7_info->circuit->cic, ftdm_sngss7_event2str(sngss7_event->event_id), ftdm_channel_state2str(ftdmchan->state));

	/* It's obvious we have a peer now, since they enqueued something in our event queue,
	   we can transfer stuff to them if needed, this should actually happen just once sometimes at the beginning of the call */
	if (sngss7_info->peer_data && sngss7_info->peer_event_transfer_cnt) {
		sngss7_transfer_peer_events(sngss7_info, FTDM_TRUE);
	}

	switch (sngss7_event->event_id) {

	case (SNGSS7_CON_IND_EVENT):
		SS7_ERROR_CHAN(ftdmchan,"[CIC:%d]Rx IAM while bridged??\n", sngss7_info->circuit->cic);
		break;

	case (SNGSS7_CON_CFM_EVENT):
		/* send the ANM request to LibSngSS7 */
		sng_cc_con_response(1,
							sngss7_info->suInstId,
							sngss7_info->spInstId,
							sngss7_info->circuit->id, 
							&sngss7_event->event.siConEvnt, 
							5); 

		SS7_INFO_CHAN(ftdmchan, "[CIC:%d]Tx peer ANM\n", sngss7_info->circuit->cic);
		break;

	case (SNGSS7_CON_STA_EVENT):
		switch (sngss7_event->evntType) {
		/**************************************************************************/
		case (ADDRCMPLT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer ACM\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (MODIFY):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer MODIFY\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (MODCMPLT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer MODIFY-COMPLETE\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (MODREJ):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer MODIFY-REJECT\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (PROGRESS):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer CPG\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (FRWDTRSFR):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer FOT\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (INFORMATION):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer INF\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (INFORMATREQ):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer INR\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (SUBSADDR):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer SAM\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (EXIT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer EXIT\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (NETRESMGT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer NRM\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (IDENTREQ):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer IDR\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (IDENTRSP):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer IRS\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (MALCLLPRNT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer MALICIOUS CALL\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (CHARGE):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer CRG\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (TRFFCHGE):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer CRG-TARIFF\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (CHARGEACK):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer CRG-ACK\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (CALLOFFMSG):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer CALL-OFFER\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (LOOPPRVNT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer LOP\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (TECT_TIMEOUT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer ECT-Timeout\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (RINGSEND):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer RINGING-SEND\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (CALLCLEAR):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer CALL-LINE Clear\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (PRERELEASE):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer PRI\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (APPTRANSPORT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer APM\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (OPERATOR):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer OPERATOR\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (METPULSE):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer METERING-PULSE\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (CLGPTCLR):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer CALLING_PARTY_CLEAR\n", sngss7_info->circuit->cic);
			break;
		/**************************************************************************/
		case (SUBDIRNUM):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer SUB-DIR\n", sngss7_info->circuit->cic);
			break;
#ifdef SANGOMA_SPIROU
		case (CHARGE_ACK):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer TXA\n", sngss7_info->circuit->cic);		
			break;
		case (CHARGE_UNIT):
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer ITX\n", sngss7_info->circuit->cic);
			break;
#endif
		/**************************************************************************/
		default:
			SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer Unknown Msg %d\n", sngss7_info->circuit->cic, sngss7_event->evntType);
			break;
		/**************************************************************************/
		}
		sng_cc_con_status  (1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					&sngss7_event->event.siCnStEvnt, 
					sngss7_event->evntType);

		break;
	/**************************************************************************/
	case (SNGSS7_REL_IND_EVENT):
	        SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer REL cause=%d\n", sngss7_info->circuit->cic, sngss7_event->event.siRelEvnt.causeDgn.causeVal.val);

		handle_peer_rel_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siRelEvnt);
		//handle_rel_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siRelEvnt);
		sng_cc_rel_request (1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					&sngss7_event->event.siRelEvnt);
		break;

	/**************************************************************************/
	case (SNGSS7_REL_CFM_EVENT):
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer RLC\n", sngss7_info->circuit->cic);
		sng_cc_rel_response (1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					&sngss7_event->event.siRelEvnt);
		//handle_rel_cfm(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siRelEvnt);
		break;

	/**************************************************************************/
	case (SNGSS7_DAT_IND_EVENT):
		//handle_dat_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siInfoEvnt);
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer %s\n", sngss7_info->circuit->cic, ftdm_sngss7_event2str(sngss7_event->event_id));
		sng_cc_dat_request(1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					&sngss7_event->event.siInfoEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_FAC_IND_EVENT):
		//handle_fac_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit, sngss7_event->evntType,  
		//&sngss7_event->event.siFacEvnt);
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer %s -> %d\n", sngss7_info->circuit->cic, 
				ftdm_sngss7_event2str(sngss7_event->event_id), sngss7_event->evntType);
		sng_cc_fac_request(1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					sngss7_event->evntType,
					&sngss7_event->event.siFacEvnt);

		break;
	/**************************************************************************/
	case (SNGSS7_FAC_CFM_EVENT):
		//handle_fac_cfm(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit, 
		//sngss7_event->evntType,  &sngss7_event->event.siFacEvnt);
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer %s -> %d\n", sngss7_info->circuit->cic, 
				ftdm_sngss7_event2str(sngss7_event->event_id), sngss7_event->evntType);
		sng_cc_fac_response(1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					sngss7_event->evntType,
					&sngss7_event->event.siFacEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_UMSG_IND_EVENT):
		//handle_umsg_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit);
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer %s\n", sngss7_info->circuit->cic, ftdm_sngss7_event2str(sngss7_event->event_id));
		sng_cc_umsg_request (1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id);
		break;
	/**************************************************************************/
	case (SNGSS7_STA_IND_EVENT):
		//handle_sta_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit, sngss7_event->globalFlg, sngss7_event->evntType,  &sngss7_event->event.siStaEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_SUSP_IND_EVENT):
		//handle_susp_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siSuspEvnt);
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer %s\n", sngss7_info->circuit->cic, ftdm_sngss7_event2str(sngss7_event->event_id));
		sng_cc_susp_request (1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					&sngss7_event->event.siSuspEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_RESM_IND_EVENT):
		//handle_resm_ind(sngss7_event->suInstId, sngss7_event->spInstId, sngss7_event->circuit,  &sngss7_event->event.siResmEvnt);
		SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Tx peer %s\n", sngss7_info->circuit->cic, ftdm_sngss7_event2str(sngss7_event->event_id));
		sng_cc_resm_request(1,
					sngss7_info->suInstId,
					sngss7_info->spInstId,
					sngss7_info->circuit->id,
					&sngss7_event->event.siResmEvnt);
		break;
	/**************************************************************************/
	case (SNGSS7_SSP_STA_CFM_EVENT):
		SS7_CRITICAL("dazed and confused ... hu?!\n");
		break;
	/**************************************************************************/
	default:
		SS7_ERROR("Failed to relay unknown event id %d!\n", sngss7_event->event_id);
		break;
	/**************************************************************************/
	}

	if ((sngss7_event->event_id == SNGSS7_SUSP_IND_EVENT)) {
		sngss7_set_ckt_flag(sngss7_info, FLAG_SUS_RECVD);
	}

	if (sngss7_test_ckt_flag(sngss7_info, FLAG_SUS_RECVD) && 
	   !sngss7_test_ckt_flag(sngss7_info, FLAG_T6_CANCELED)) {
		if (sng_cancel_isup_tmr(sngss7_info->suInstId, ISUP_T6i) == RFAILED ) {
			SS7_ERROR_CHAN(ftdmchan,"[CIC:%d]could not stop timer T6 \n", sngss7_info->circuit->cic);
		} else {
			sngss7_set_ckt_flag(sngss7_info, FLAG_T6_CANCELED);
			SS7_ERROR_CHAN(ftdmchan,"[CIC:%d] isup timer T6 has been cancelled. \n", sngss7_info->circuit->cic);
		}
	}
}

static ftdm_status_t ftdm_sangoma_ss7_native_bridge_state_change(ftdm_channel_t *ftdmchan);
static ftdm_status_t ftdm_sangoma_ss7_native_bridge_state_change(ftdm_channel_t *ftdmchan)
{
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;

	ftdm_channel_complete_state(ftdmchan);

	SS7_DEBUG_CHAN(ftdmchan, "DEBUG: peer state change to %s\n", ftdm_channel_state2str(ftdmchan->state));
	switch (ftdmchan->state) {

	case FTDM_CHANNEL_STATE_DOWN:
		{
			ftdm_channel_t *close_chan = ftdmchan;
			sngss7_clear_ckt_flag(sngss7_info, FLAG_SUS_RECVD);
			sngss7_clear_ckt_flag(sngss7_info, FLAG_T6_CANCELED);
			sngss7_clear_ckt_flag (sngss7_info, FLAG_SENT_ACM);
			sngss7_clear_ckt_flag (sngss7_info, FLAG_SENT_CPG);
			sngss7_clear_call_flag (sngss7_info, FLAG_CONG_REL);

			sngss7_flush_queue(sngss7_info->event_queue);
			sngss7_info->peer_event_transfer_cnt = 0;
			sngss7_info->peer_data = NULL;
			ftdm_channel_close (&close_chan);
		}
		break;

	case FTDM_CHANNEL_STATE_UP:
		{
			if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_OUTBOUND)) {
				sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_UP);
			}
		}
		break;

	case FTDM_CHANNEL_STATE_TERMINATING:
		{
			/* Release confirm is sent immediately, since Spirou customer asked us not to wait for the second call leg
			 * to come back with a release confirm ... */
			/* when receiving REL we move to TERMINATING and notify the user that the bridge is ending */
			if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_USER_HANGUP)) {
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);
			} else {
				/* Notify the user and wait for their ack before sending RLC */
				sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_STOP);
			}
		}
		break;

	case FTDM_CHANNEL_STATE_HANGUP:
		{
			SS7_DEBUG_CHAN(ftdmchan, "DEBUG:  STATE Hangup received peer state change to %s\n", ftdm_channel_state2str(ftdmchan->state));

			/* If this is not rejected call then only decrement the number of active calls */
			if (!sngss7_test_call_flag (sngss7_info, FLAG_CONG_REL)) {
				if (((sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan)))) {
					if (sngss7_rmt_cong->sngss7_rmtCongLvl) {
						SS7_DEBUG_CHAN(ftdmchan,"Decrementing Number of active calls for congested DPC[%d]\n", sngss7_rmt_cong->dpc);
						/* Decrease number of active calls by 1 for the respective congested DPC */
						sng_acc_handle_call_rate(FTDM_FALSE, sngss7_rmt_cong, ftdmchan);
					}
				}
			}

			if (sngss7_test_call_flag (sngss7_info, FLAG_CONG_REL)) {
				SS7_DEBUG_CHAN(ftdmchan,"Hanging up due to congestion %s. Thus, sending release\n","");
			} else {
				ft_to_sngss7_rlc(ftdmchan);
			}
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
		}
		break;

	default:
		break;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
ftdm_status_t ftdm_sangoma_ss7_process_state_change (ftdm_channel_t *ftdmchan)
{
	sngss7_chan_data_t *sngss7_info = ftdmchan->call_data;
	sng_isup_inf_t *isup_intf = NULL;
	int state_flag = 1; 
	int i = 0;
	const char *val = NULL;
	ftdm_status_t status = FTDM_FAIL;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;

	SS7_DEBUG_CHAN(ftdmchan, "ftmod_sangoma_ss7 processing state %s: ckt=0x%X, blk=0x%X, call=0x%X\n",
						ftdm_channel_state2str (ftdmchan->state),
									sngss7_info->ckt_flags,
									sngss7_info->blk_flags,
									sngss7_info->call_flags);

	if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_NATIVE_SIGBRIDGE)) {
		/* DIALING is the only state we process normally when doing an outgoing call that is natively bridged, 
		 * all other states are run by a different state machine (and the freetdm core does not do any checking) */
		if (ftdmchan->state != FTDM_CHANNEL_STATE_DIALING) {
			return ftdm_sangoma_ss7_native_bridge_state_change(ftdmchan);
		}
	}

	/*check what state we are supposed to be in */
	switch (ftdmchan->state) {
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_COLLECT:	/* IAM received but wating on digits */

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		i = 0;
		while (ftdmchan->caller_data.cid_num.digits[i] != '\0') {
			i++;
		}

		/* check if there is a pulsating character in end of calling party number */
		if (ftdmchan->caller_data.cid_num.digits[i -1] == 'F') {
			/* Remove the pulsating character */
			ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Received Calling party number with pulsing character %s\n", ftdmchan->caller_data.cid_num.digits);
			ftdmchan->caller_data.cid_num.digits[i-1] = '\0';
			ftdm_log_chan(ftdmchan, FTDM_LOG_DEBUG, "Refined Calling party number %s\n", ftdmchan->caller_data.cid_num.digits);
		}

		i = 0;
		while (ftdmchan->caller_data.dnis.digits[i] != '\0'){
			i++;
		}

		/* kill t10 if active */
		if (sngss7_info->t10.hb_timer_id) {
			ftdm_sched_cancel_timer (sngss7_info->t10.sched, sngss7_info->t10.hb_timer_id);
		}

		/* check if the end of pulsing (ST) character has arrived or the right number of digits */
		if (ftdmchan->caller_data.dnis.digits[i-1] == 'F'
		    || sngss7_test_ckt_flag(sngss7_info, FLAG_FULL_NUMBER) ) 
		{
			SS7_DEBUG_CHAN(ftdmchan, "Received the end of pulsing character %s\n", "");

			if (!sngss7_test_ckt_flag(sngss7_info, FLAG_FULL_NUMBER)) {
				/* remove the ST */
				ftdmchan->caller_data.dnis.digits[i-1] = '\0';
				sngss7_set_ckt_flag(sngss7_info, FLAG_FULL_NUMBER);
			}
			
			if (sngss7_test_ckt_flag(sngss7_info, FLAG_INR_TX)) {
				if (!sngss7_test_ckt_flag(sngss7_info, FLAG_INR_SENT) ) {
					ft_to_sngss7_inr(ftdmchan);
					sngss7_set_ckt_flag(sngss7_info, FLAG_INR_SENT);
					
					SS7_DEBUG_CHAN (ftdmchan, "Scheduling T.39 timer %s \n", " ");
					
					/* start ISUP t39 */
					if (ftdm_sched_timer (sngss7_info->t39.sched,
										"t39",
										sngss7_info->t39.beat,
										sngss7_info->t39.callback,
										&sngss7_info->t39,
										&sngss7_info->t39.hb_timer_id)) 
					{
				
						SS7_ERROR ("Unable to schedule timer T39, hanging up call!\n");

						ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_NORMAL_TEMPORARY_FAILURE;
						sngss7_set_ckt_flag (sngss7_info, FLAG_LOCAL_REL);
				
						/* end the call */
						state_flag = 0;
						ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_CANCEL);
					}
				}else {
					state_flag = 0;
					ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RING);
				}
			} else {
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RING);
			}
		} else if (i >= sngss7_info->circuit->min_digits) {
			SS7_DEBUG_CHAN(ftdmchan, "Received %d digits (min digits = %d)\n", i, sngss7_info->circuit->min_digits);

			if (sngss7_test_ckt_flag(sngss7_info, FLAG_INR_TX)) {
				if (!sngss7_test_ckt_flag(sngss7_info, FLAG_INR_SENT) ) {
					ft_to_sngss7_inr(ftdmchan);
					sngss7_set_ckt_flag(sngss7_info, FLAG_INR_SENT);
					
					SS7_DEBUG_CHAN (ftdmchan, "Scheduling T.39 timer %s\n", " " );
					
					/* start ISUP t39 */
					if (ftdm_sched_timer (sngss7_info->t39.sched,
										"t39",
										sngss7_info->t39.beat,
										sngss7_info->t39.callback,
										&sngss7_info->t39,
										&sngss7_info->t39.hb_timer_id)) 
					{
				
						SS7_ERROR ("Unable to schedule timer T39, hanging up call!\n");

						ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_NORMAL_TEMPORARY_FAILURE;
						sngss7_set_ckt_flag (sngss7_info, FLAG_LOCAL_REL);
				
						/* end the call */
						state_flag = 0;
						ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_CANCEL);
					}
					
					state_flag = 0;
					ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_IDLE);
				}else {
					if (sngss7_test_ckt_flag(sngss7_info, FLAG_INF_RX_DN) ) {
						state_flag = 0;
						ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RING);
					}
				}
			} else {
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RING);
			}
		} else {
			/* if we are coming from idle state then we have already been here once before */
			if (ftdmchan->last_state != FTDM_CHANNEL_STATE_IDLE) {
				SS7_INFO_CHAN(ftdmchan, "Received %d out of %d so far: %s...starting T35\n",
										i,
										sngss7_info->circuit->min_digits,
										ftdmchan->caller_data.dnis.digits);
		
				/* start ISUP t35 */
				if (ftdm_sched_timer (sngss7_info->t35.sched,
										"t35",
										sngss7_info->t35.beat,
										sngss7_info->t35.callback,
										&sngss7_info->t35,
										&sngss7_info->t35.hb_timer_id)) {
		
					SS7_ERROR ("Unable to schedule timer, hanging up call!\n");
		
					ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_NORMAL_TEMPORARY_FAILURE;
		
					/* set the flag to indicate this hangup is started from the local side */
					sngss7_set_ckt_flag (sngss7_info, FLAG_LOCAL_REL);
		
					/* end the call */
					state_flag = 0;
					ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_CANCEL);
				}
			}

			/* start ISUP t10 */
			if (ftdm_sched_timer (sngss7_info->t10.sched,
									"t10",
									sngss7_info->t10.beat,
									sngss7_info->t10.callback,
									&sngss7_info->t10,
									&sngss7_info->t10.hb_timer_id)) {
	
				SS7_ERROR ("Unable to schedule timer, hanging up call!\n");
	
				ftdmchan->caller_data.hangup_cause = FTDM_CAUSE_NORMAL_TEMPORARY_FAILURE;
	
				/* set the flag to indicate this hangup is started from the local side */
				sngss7_set_ckt_flag (sngss7_info, FLAG_LOCAL_REL);
	
				/* end the call */
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_CANCEL);
			}
		}

	  break;

	/**************************************************************************/
	case FTDM_CHANNEL_STATE_RING:	/*incoming call request */

		sngss7_clear_ckt_flag(sngss7_info, FLAG_INR_TX);
		sngss7_clear_ckt_flag(sngss7_info, FLAG_INR_SENT);
		sngss7_clear_ckt_flag(sngss7_info, FLAG_INR_RX);
		sngss7_clear_ckt_flag(sngss7_info, FLAG_INR_RX_DN);
		sngss7_clear_ckt_flag(sngss7_info, FLAG_INF_TX);
		sngss7_clear_ckt_flag(sngss7_info, FLAG_INF_SENT);
		sngss7_clear_ckt_flag(sngss7_info, FLAG_INF_RX);
		sngss7_clear_ckt_flag(sngss7_info, FLAG_INF_RX_DN);
		
		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		/* kill t35 if active */
		if (sngss7_info->t35.hb_timer_id) {
			ftdm_sched_cancel_timer (sngss7_info->t35.sched, sngss7_info->t35.hb_timer_id);
		}

		/* cancel t39 timer */
		if (sngss7_info->t39.hb_timer_id) {
			ftdm_sched_cancel_timer (sngss7_info->t39.sched, sngss7_info->t39.hb_timer_id);
		}

		SS7_DEBUG_CHAN(ftdmchan, "Sending incoming call from %s to %s to FTDM core\n",
					ftdmchan->caller_data.ani.digits,
					ftdmchan->caller_data.dnis.digits);

		/* we have enough information to inform FTDM of the call */
		sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_START);

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_DIALING:	/*outgoing call request */

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		if (g_ftdm_sngss7_data.cfg.sng_acc) {
			/* A possible scenario need to check afterwards */
			val = ftdm_usrmsg_get_var(ftdmchan->usrmsg, "ss7_iam_priority");
			if (!ftdm_strlen_zero(val)) {
				if (!(atoi(val) ==  CAT_PRIOR)) {
					SS7_DEBUG("Checking for congestion status\n");
					status = ftdm_sangoma_ss7_get_congestion_status(ftdmchan);
				} else {
					/* set the flag to indicate this is a priority call in case of congestion */
					sngss7_set_call_flag (sngss7_info, FLAG_PRI_CALL);
					SS7_DEBUG("This is a priority Call thus processing it normally\n");
				}
			} else {
				SS7_DEBUG("Since the call is not a priority one. Thus, checking for congestion status\n");
				status = ftdm_sangoma_ss7_get_congestion_status(ftdmchan);
			}
			/* increased number of received calls */
			ftdm_sangoma_ss7_received_call(ftdmchan);

				if ((status == FTDM_BREAK) || (status == FTDM_SUCCESS)) {
					if (status == FTDM_BREAK) {
						state_flag = 0;
					}
					break;
				}
		}

		SS7_DEBUG_CHAN(ftdmchan, "Sending outgoing call from \"%s\" to \"%s\" to LibSngSS7\n",
					   ftdmchan->caller_data.ani.digits,
					   ftdmchan->caller_data.dnis.digits);

		/*call sangoma_ss7_dial to make outgoing call */
		ft_to_sngss7_iam(ftdmchan);

		break;
	/**************************************************************************/
	/* We handle RING indication the same way we would indicate PROGRESS */
	case FTDM_CHANNEL_STATE_RINGING:
		if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_OUTBOUND)) {
			sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_RINGING);
			SS7_DEBUG_CHAN(ftdmchan, "Signal freetdm outgoing channel is in RINGING state %s \n", " ");
		} else {
			/* handle incoming call RINGING state */
			SS7_DEBUG_CHAN(ftdmchan, "Signal freetdm incoming channel is in RINGING state %s \n", " ");
			if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_ACM)) {
				sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_ACM);
				ft_to_sngss7_acm(ftdmchan);

				/* CPG on Alert configuration option set then send CPG as well */
				if (g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].cpg_on_alert == FTDM_TRUE) {
					if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_CPG)) {
						sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_CPG);
						ft_to_sngss7_cpg(ftdmchan, EV_PROGRESS, EVPR_NOIND);
					}
				}
			} else {
				if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_CPG)) {
					sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_CPG);
					ft_to_sngss7_cpg(ftdmchan, EV_ALERT, EVPR_NOIND);
				}
			}
		}
		break;
	case FTDM_CHANNEL_STATE_PROGRESS:

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		/*check if the channel is inbound or outbound */
		if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_OUTBOUND)) {
			/*OUTBOUND...so we were told by the line of this so noifiy the user */
			sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_PROGRESS);

			/* move to progress media  */
			/*
			state_flag = 0;
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_PROGRESS_MEDIA);
			*/
		} else {
			/* inbound call so we need to send out ACM */
			if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_ACM)) {
				sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_ACM);
				ft_to_sngss7_acm(ftdmchan);
			}
			if (g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].cpg_on_progress == FTDM_TRUE) {
				if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_CPG)) {
					sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_CPG);
					ft_to_sngss7_cpg(ftdmchan, EV_PROGRESS, EVPR_NOIND);
				}
			}
		}

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_PROGRESS_MEDIA:

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_OUTBOUND)) {
			/* inform the user there is media avai */
			sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_PROGRESS_MEDIA);
		} else {
			if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_ACM)) {
				sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_ACM);
				ft_to_sngss7_acm(ftdmchan);
			} else {
				if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_CPG)) {
					sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_CPG);
					ft_to_sngss7_cpg(ftdmchan, EV_PROGRESS, EVPR_NOIND);
				}
			}

			if (g_ftdm_sngss7_data.cfg.isupCkt[sngss7_info->circuit->id].cpg_on_progress_media == FTDM_TRUE) {
				if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_CPG)) {
					sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_CPG);
					ft_to_sngss7_cpg(ftdmchan, EV_INBAND, EVPR_NOIND); 
				}
			}
		}

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_UP:	/*call is accpeted...both incoming and outgoing */

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		/*check if the channel is inbound or outbound */
		if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_OUTBOUND)) {
			/*OUTBOUND...so we were told by the line that the other side answered */
			sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_UP);
		} else {
			/*INBOUND...so FS told us it was going to answer...tell the stack */
			if (!sngss7_test_ckt_flag(sngss7_info, FLAG_SENT_ACM)) {
				sngss7_set_ckt_flag(sngss7_info, FLAG_SENT_ACM);
				ft_to_sngss7_acm(ftdmchan);
			}
			ft_to_sngss7_anm(ftdmchan);
		}

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_CANCEL:

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		SS7_ERROR_CHAN(ftdmchan,"Hanging up call before informing user%s\n", " ");

		/*now go to the HANGUP complete state */
		state_flag = 0;
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_TERMINATING:	/*call is hung up remotely */

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}


		/* set the flag to indicate this hangup is started from the remote side */
		sngss7_set_ckt_flag (sngss7_info, FLAG_REMOTE_REL);

		/*this state is set when the line is hanging up */
		sngss7_send_signal(sngss7_info, FTDM_SIGEVENT_STOP);

		/* If the RESET flag is set, do not say in TERMINATING state.
		   Go back to RESTART state and wait for RESET Confirmation */ 
		if (sngss7_tx_reset_status_pending(sngss7_info)) {
			SS7_DEBUG_CHAN(ftdmchan,"Reset pending in Terminating state!%s\n", "");
			state_flag = 0;
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RESTART);
		}

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_HANGUP:	/*call is hung up locally */

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		/* Decrement the value of allowed calls in case of congestion if and only if
		 * 1) It is not a rejected call and also not a normal call i.e. call before congestion is started
		 * 2) It is not a priority call
		 * 3) And there is a congestion present on the dpc from which REL is received */

		/* We started incrementing leaky bucket counter after we detect congestion, hence decrement counters for the calls
		 * which has been received after congestion only i.e. FLAG_NRML_CALL is false */
		if ((!sngss7_test_call_flag (sngss7_info, FLAG_CONG_REL)) && (!sngss7_test_call_flag (sngss7_info, FLAG_NRML_CALL))) {
			if (((sngss7_rmt_cong = sng_acc_get_cong_struct(ftdmchan)))) {
				if (sngss7_rmt_cong->sngss7_rmtCongLvl) {
					if (!sngss7_test_call_flag (sngss7_info, FLAG_PRI_CALL)) {
						/* Check if the call is receivied during same congestion block rate the please fo ahead and
						 * decrement the number of active calls by 1 */
						if (FTDM_SUCCESS == sng_acc_rmv_active_call(ftdmchan)) {
							/* Decrease number of active calls by 1 for the respective congested DPC */
							sng_acc_handle_call_rate(FTDM_FALSE, sngss7_rmt_cong, ftdmchan);
						} else {
							SS7_DEBUG_CHAN(ftdmchan,"NSG-ACC: Not found in call list, hence not decrementing active calls[%d] for dpc[%d]\n",
									(sngss7_rmt_cong->calls_allowed), sngss7_rmt_cong->dpc);
						}
					} else {
						SS7_DEBUG_CHAN(ftdmchan,"NSG-ACC: Received Release for prirotity call on dpc[%d]\n",
								sngss7_rmt_cong->dpc);
					}
				}
			}
		}

		/* check for remote hangup flag */
		if (sngss7_test_ckt_flag (sngss7_info, FLAG_REMOTE_REL)) {
			/* remote release ...do nothing here */
			SS7_DEBUG_CHAN(ftdmchan,"Hanging up remotely requested call!%s\n", "");
		} else if (sngss7_test_ckt_flag (sngss7_info, FLAG_GLARE)) {
			/* release due to glare */
			SS7_DEBUG_CHAN(ftdmchan,"Hanging up requested call do to glare%s\n", "");
		} else if (sngss7_test_call_flag (sngss7_info, FLAG_CONG_REL)) {
			SS7_DEBUG_CHAN(ftdmchan,"Hanging up due to congestion %s\n","");
		} else 	{
			/* set the flag to indicate this hangup is started from the local side */
			sngss7_set_ckt_flag (sngss7_info, FLAG_LOCAL_REL);

			/*this state is set when FS is hanging up...so tell the stack */
			ft_to_sngss7_rel (ftdmchan);

			SS7_DEBUG_CHAN(ftdmchan,"Hanging up locally requested call!%s\n", "");
		}

		/*now go to the HANGUP complete state */
		state_flag = 0;
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP_COMPLETE);

		break;

	/**************************************************************************/
	case FTDM_CHANNEL_STATE_HANGUP_COMPLETE:

		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) {
			SS7_DEBUG("re-entering state from processing block/unblock request ... do nothing\n");
			break;
		}

		if (sngss7_test_ckt_flag (sngss7_info, FLAG_REMOTE_REL)) {
		
			sngss7_clear_ckt_flag (sngss7_info, FLAG_LOCAL_REL);

			/* check if this hangup is from a tx RSC */
			if (sngss7_test_ckt_flag (sngss7_info, FLAG_RESET_TX)) {
				if (!sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_SENT)) {
					ft_to_sngss7_rsc (ftdmchan);
					sngss7_set_ckt_flag(sngss7_info, FLAG_RESET_SENT);

					/* Wait for Reset in HANGUP Complete nothing to do until we
					   get reset response back */
				} else if (sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_TX_RSP)) {
					state_flag = 0;
					ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
				} else {
					/* Stay in hangup complete until RSC is received */
					/* Channel is in use if we go to RESTART we will 
					   restart will just come back to HANGUP_COMPLETE */
				}	
			} else {
				/* if the hangup is from a rx RSC, rx GRS, or glare don't sent RLC */
				if (!(sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_RX)) &&
					!(sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_RX)) &&
					!(sngss7_test_ckt_flag(sngss7_info, FLAG_GLARE))) {

					/* send out the release complete */
					ft_to_sngss7_rlc (ftdmchan);
				}

				/*now go to the DOWN state */
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
			}

			SS7_DEBUG_CHAN(ftdmchan,"Completing remotely requested hangup!%s\n", "");
		} else if (sngss7_test_ckt_flag (sngss7_info, FLAG_LOCAL_REL)) {

			/* if this hang up is do to a rx RESET we need to sit here till the RSP arrives */
			if (sngss7_test_ckt_flag (sngss7_info, FLAG_RESET_TX_RSP)) {
				/* go to the down state as we have already received RSC-RLC */
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
			}

			/* if it's a local release the user sends us to down */
			SS7_DEBUG_CHAN(ftdmchan,"Completing locally requested hangup!%s\n", "");
		} else if (sngss7_test_ckt_flag (sngss7_info, FLAG_GLARE)) {
			SS7_DEBUG_CHAN(ftdmchan,"Completing requested hangup due to glare!%s\n", "");
			state_flag = 0;
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
		} else if (sngss7_test_call_flag (sngss7_info, FLAG_CONG_REL)) {
			SS7_DEBUG_CHAN(ftdmchan,"Completing requested hangup due to congestion!%s\n", "");
			state_flag = 0;
			ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
		} else {
			SS7_DEBUG_CHAN(ftdmchan,"Completing requested hangup for unknown reason!%s\n", "");
			if (sngss7_channel_status_clear(sngss7_info)) {
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
			} 
		}

		break;

	/**************************************************************************/
	case FTDM_CHANNEL_STATE_DOWN:	/*the call is finished and removed */
					
		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) { 
			if (!ftdm_test_flag (ftdmchan, FTDM_CHANNEL_OPEN)) {
				SS7_DEBUG_CHAN(ftdmchan,"Down came from SUSPEND - break %s\n", "");
				break;
			}
		}

		/* check if there is a reset response that needs to be sent */
		if (sngss7_test_ckt_flag (sngss7_info, FLAG_RESET_RX)) {
			/* send a RSC-RLC */
			ft_to_sngss7_rsca (ftdmchan);

			/* clear the reset flag  */
			clear_rx_rsc_flags(sngss7_info);
		}

		/* check if there was a GRS that needs a GRA */
		if ((sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_RX)) &&
			(sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_RX_DN)) &&
			(sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_RX_CMPLT))) {

			/* check if this is the base circuit and send out the GRA
			 * we insure that this is the last circuit to have the state change queued */
			if (sngss7_info->rx_grs.range) {
				/* send out the GRA */
				ft_to_sngss7_gra(ftdmchan);

				/* clean out the spans GRS structure */
				clear_rx_grs_data(sngss7_info);
			}

			/* clear the grp reset flag */
			clear_rx_grs_flags(sngss7_info);
		}

		/* check if we got the reset response */
		if (sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_TX_RSP)) {
			/* clear the reset flag  */
			clear_tx_rsc_flags(sngss7_info);
		}

		if (sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_TX_RSP)) {
			/* clear the reset flag  */
			clear_tx_grs_flags(sngss7_info);
			if (sngss7_info->rx_gra.range) {
				/* clean out the spans GRA structure */
				clear_rx_gra_data(sngss7_info);
			}
		}

		/* check if we came from reset (aka we just processed a reset) */
		if ((ftdmchan->last_state == FTDM_CHANNEL_STATE_RESTART) || 
			(ftdmchan->last_state == FTDM_CHANNEL_STATE_SUSPENDED) || 
			(ftdmchan->last_state == FTDM_CHANNEL_STATE_HANGUP_COMPLETE)) {
				

			/* check if reset flags are up indicating there is more processing to do yet */
			if (!(sngss7_test_ckt_flag (sngss7_info, FLAG_RESET_TX)) &&
				!(sngss7_test_ckt_flag (sngss7_info, FLAG_RESET_RX)) &&
				!(sngss7_test_ckt_flag (sngss7_info, FLAG_GRP_RESET_TX)) &&
				!(sngss7_test_ckt_flag (sngss7_info, FLAG_GRP_RESET_RX))) {
		  
				SS7_DEBUG_CHAN(ftdmchan,"Current flags: ckt=0x%X, blk=0x%X\n", 
									sngss7_info->ckt_flags,
									sngss7_info->blk_flags);

				if (sngss7_channel_status_clear(sngss7_info)) {
					/* check if the sig status is down, and bring it up if it isn't */
					if (!ftdm_test_flag (ftdmchan, FTDM_CHANNEL_SIG_UP)) {
						SS7_DEBUG_CHAN(ftdmchan,"All reset flags cleared %s\n", "");
						/* all flags are down so we can bring up the sig status */
						sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);
					}
				} else {
					state_flag = 0;
					SS7_DEBUG_CHAN(ftdmchan,"Down detected blocked flags go to SUSPEND %s\n", " ");
					ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
					break;
			
				}
			} else {
				SS7_DEBUG_CHAN(ftdmchan,"Reset flags present (0x%X)\n", sngss7_info->ckt_flags);
			
				/* there is still another reset pending so go back to reset*/
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RESTART);
			}
		}

		/* check if t35 is active */
		if (sngss7_info->t35.hb_timer_id) {
			ftdm_sched_cancel_timer (sngss7_info->t35.sched, sngss7_info->t35.hb_timer_id);
		}

		/* clear all of the call specific data store in the channel structure */
		sngss7_info->suInstId = 0;
		sngss7_info->spInstId = 0;
		sngss7_info->globalFlg = 0;
		sngss7_info->spId = 0;
		sngss7_info->priority = FTDM_FALSE;

		/* clear any call related flags */
		sngss7_clear_ckt_flag (sngss7_info, FLAG_REMOTE_REL);
		sngss7_clear_ckt_flag (sngss7_info, FLAG_LOCAL_REL);
		sngss7_clear_ckt_flag (sngss7_info, FLAG_SENT_ACM);
		sngss7_clear_ckt_flag (sngss7_info, FLAG_SENT_CPG);
		sngss7_clear_call_flag (sngss7_info, FLAG_CONG_REL);
		sngss7_clear_call_flag (sngss7_info, FLAG_PRI_CALL);
		sngss7_clear_call_flag (sngss7_info, FLAG_NRML_CALL);


		if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_OPEN)) {
			ftdm_channel_t *close_chan = ftdmchan;
			/* close the channel */
			SS7_DEBUG_CHAN(ftdmchan,"FTDM Channel Close %s\n", "");
			sngss7_flush_queue(sngss7_info->event_queue);
			sngss7_info->peer_event_transfer_cnt = 0;
			ftdm_channel_close (&close_chan);
		}

		/* check if there is a glared call that needs to be processed */
		if (sngss7_test_ckt_flag(sngss7_info, FLAG_GLARE)) {
			sngss7_clear_ckt_flag (sngss7_info, FLAG_GLARE);

			if (sngss7_info->glare.circuit != 0) {
				int bHandle=0;
				switch (g_ftdm_sngss7_data.cfg.glareResolution) {
					case SNGSS7_GLARE_DOWN:
						SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Giving control to the other side, handling copied IAM from glare. \n", sngss7_info->circuit->cic);
						bHandle = 1;
		 				break;

					case SNGSS7_GLARE_PC:
						SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Trying to handle IAM copied from glare. \n", sngss7_info->circuit->cic);
						SS7_INFO_CHAN(ftdmchan,"[CIC:%d]My PC = %d, incoming PC = %d. \n", sngss7_info->circuit->cic,
										g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].spc, 
										g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc );
						
						if( g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].spc > g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId].dpc ) 
						{
							if ((sngss7_info->circuit->cic % 2) == 1 ) {
								bHandle = 1;
							}
						} else {
							if( (sngss7_info->circuit->cic % 2) == 0 ) {
								bHandle = 1;
							}
						}
					
						break;
	 				default:		/* if configured as SNGSS7_GLARE_CONTROL, always abandon incoming glared IAM. */
	 					bHandle = 0;
	 					break;
				}
				
				if (bHandle) {
					SS7_INFO_CHAN(ftdmchan,"[CIC:%d]Handling glare IAM. \n", sngss7_info->circuit->cic);
					handle_con_ind (0, sngss7_info->glare.spInstId, sngss7_info->glare.circuit, &sngss7_info->glare.iam);				
				}
				
				/* clear the glare info */
				memset(&sngss7_info->glare, 0x0, sizeof(sngss7_glare_data_t));
				state_flag = 0;
			}
		}

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_RESTART:	/* CICs needs a Reset */
		  
		SS7_DEBUG_CHAN(ftdmchan,"RESTART: Current flags: ckt=0x%X, blk=0x%X\n", 
									sngss7_info->ckt_flags,
									sngss7_info->blk_flags);
		
		if (sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_UCIC_BLOCK)) {
			if ((sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_RX)) ||
				(sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_RX))) {

				SS7_DEBUG_CHAN(ftdmchan,"Incoming Reset request on CIC in UCIC block, removing UCIC block%s\n", "");

				/* set the unblk flag */
				sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_UCIC_UNBLK);

				/* clear the block flag */
				sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_UCIC_BLOCK);

				/* process the flag */
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);

				/* break out of the processing for now */
				break;
			}
		}


		/* check if this is an outgoing RSC */
		if ((sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_TX)) &&
			!(sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_SENT))) {

			/* don't send out reset before finished hanging up if I'm in-use. */
			if (!ftdm_test_flag(ftdmchan, FTDM_CHANNEL_INUSE)) {
				/* send a reset request */
				ft_to_sngss7_rsc (ftdmchan);
				sngss7_set_ckt_flag(sngss7_info, FLAG_RESET_SENT);
			}

		} /* if (sngss7_test_ckt_flag(sngss7_info, FLAG_RESET_TX)) */

		/* check if this is the first channel of a GRS (this flag is thrown when requesting reset) */
		if ( (sngss7_test_ckt_flag (sngss7_info, FLAG_GRP_RESET_TX)) &&
			!(sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_SENT)) &&
			(sngss7_test_ckt_flag(sngss7_info, FLAG_GRP_RESET_BASE))) {

				/* send out the grs */
				ft_to_sngss7_grs (ftdmchan);

		}
	
		/* if the sig_status is up...bring it down */
		if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_SIG_UP)) {
			sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);
		}

		if (sngss7_test_ckt_flag (sngss7_info, FLAG_GRP_RESET_RX)) {
			/* set the grp reset done flag so we know we have finished this reset */
			sngss7_set_ckt_flag(sngss7_info, FLAG_GRP_RESET_RX_DN);
		} /* if (sngss7_test_ckt_flag (sngss7_info, FLAG_GRP_RESET_RX)) */


		if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_INUSE)) {
			/* bring the call down first...then process the rest of the reset */
			switch (ftdmchan->last_state){
			/******************************************************************/
			case (FTDM_CHANNEL_STATE_TERMINATING):
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_HANGUP);	
				break;
			/******************************************************************/
			case (FTDM_CHANNEL_STATE_HANGUP):
			case (FTDM_CHANNEL_STATE_HANGUP_COMPLETE):
				/* go back to the last state after taking care of the rest of the restart state */
				state_flag = 0;
				ftdm_set_state(ftdmchan, ftdmchan->last_state);
			break;
			/******************************************************************/
			case (FTDM_CHANNEL_STATE_IN_LOOP):
				/* we screwed up in a COT/CCR, remove the loop */
				ftdm_channel_command(ftdmchan, FTDM_COMMAND_DISABLE_LOOP, NULL);

				/* go to down */
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
				break;
			/******************************************************************/
			default:
				/* KONRAD: find out what the cause code should be */
				ftdmchan->caller_data.hangup_cause = 41;

				/* change the state to terminatting, it will throw us back here
				 * once the call is done
				 */
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_TERMINATING);
			break;
			/******************************************************************/
			} /* switch (ftdmchan->last_state) */
		} else {
			/* check if this an incoming RSC or we have a response already */
			if (sngss7_test_ckt_flag (sngss7_info, FLAG_RESET_RX) ||
				sngss7_test_ckt_flag (sngss7_info, FLAG_RESET_TX_RSP) ||
				sngss7_test_ckt_flag (sngss7_info, FLAG_GRP_RESET_TX_RSP) ||
				sngss7_test_ckt_flag (sngss7_info, FLAG_GRP_RESET_RX_CMPLT)) {
	
				SS7_DEBUG_CHAN(ftdmchan, "Reset processed moving to DOWN (0x%X)\n", sngss7_info->ckt_flags);
	
				/* go to a down state to clear the channel and send the response */
				state_flag = 0;
				ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_DOWN);
			} else {
					
				SS7_DEBUG_CHAN(ftdmchan, "Waiting on Reset Rsp/Grp Reset to move to DOWN (0x%X)\n", sngss7_info->ckt_flags);
			}
		}

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_SUSPENDED:	/* circuit has been blocked */

		SS7_DEBUG_CHAN(ftdmchan,"SUSPEND: Current flags: ckt=0x%X, blk=0x%X, circuit->flag=0x%X\n", 
					sngss7_info->ckt_flags, sngss7_info->blk_flags,
					sngss7_info->circuit->flags );
		
		if (!(sngss7_info->circuit->flags & SNGSS7_CONFIGURED)) {
			/* Configure the circuit if RESUME and PAUSED are not set.
			   And also in a case when RESUME is set */
			if (!sngss7_test_ckt_flag(sngss7_info, FLAG_INFID_PAUSED) ||
			     sngss7_test_ckt_flag(sngss7_info, FLAG_INFID_RESUME)) {
				if (ftmod_ss7_isup_ckt_config(sngss7_info->circuit->id)) {
					SS7_CRITICAL("ISUP CKT %d configuration FAILED!\n", sngss7_info->circuit->id);
					sngss7_set_ckt_flag(sngss7_info, FLAG_INFID_PAUSED);
					sngss7_clear_ckt_flag(sngss7_info, FLAG_INFID_RESUME);
				} else {
					SS7_INFO("ISUP CKT %d configuration DONE!\n", sngss7_info->circuit->id);
					sngss7_info->circuit->flags |= SNGSS7_CONFIGURED;
					sngss7_set_ckt_flag(sngss7_info, FLAG_RESET_TX);
				}
			}
		}
		 
		/**********************************************************************/
		if (sngss7_test_ckt_flag(sngss7_info, FLAG_INFID_RESUME)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing RESUME%s\n", "");

			/* clear the RESUME flag */
			sngss7_clear_ckt_flag(sngss7_info, FLAG_INFID_RESUME);

			/* clear the PAUSE flag */
			sngss7_clear_ckt_flag(sngss7_info, FLAG_INFID_PAUSED);

			/* We tried to hangup the call while in PAUSED state.
			   We must send a RESET to clear this circuit */
			if (sngss7_test_ckt_flag (sngss7_info, FLAG_LOCAL_REL)) {
				SS7_DEBUG_CHAN(ftdmchan, "Channel local release on RESUME, restart Reset procedure%s\n", "");
				/* By setting RESET_TX flag the check below sngss7_tx_reset_status_pending() will
				   be true, and will restart the RESET TX procedure */
				sngss7_set_ckt_flag (sngss7_info, FLAG_REMOTE_REL);
				sngss7_set_ckt_flag (sngss7_info, FLAG_RESET_TX);
			}	

			/* We have transmitted Reset/GRS but have not gotten a
			 * Response. In mean time we got a RESUME. We cannot be sure
			 * that our reset has been trasmitted, thus restart reset procedure. */ 
			if (sngss7_tx_reset_status_pending(sngss7_info)) {
				SS7_DEBUG_CHAN(ftdmchan, "Channel transmitted RSC/GRS before RESUME, restart Reset procedure%s\n", "");
				clear_rx_grs_flags(sngss7_info);
				clear_rx_grs_data(sngss7_info);
				clear_tx_grs_flags(sngss7_info);
				clear_tx_grs_data(sngss7_info);
				clear_rx_rsc_flags(sngss7_info);
				clear_tx_rsc_flags(sngss7_info);

				clear_tx_rsc_flags(sngss7_info);
				sngss7_set_ckt_flag(sngss7_info, FLAG_RESET_TX);
			}

			/* if there are any resets present */
			if (!sngss7_channel_status_clear(sngss7_info)) {
				/* don't bring up the sig status but also move to reset */
				if (!sngss7_reset_status_clear(sngss7_info)) {
					goto suspend_goto_restart;
				} else if (!sngss7_block_status_clear(sngss7_info)) {
					/* Do nothing just go through and handle blocks below */
				} else {
					/* This should not happen as above function tests
					 * for reset and blocks */
					SS7_ERROR_CHAN(ftdmchan, "Invalid code path: sngss7_channel_status_clear reset and block are both cleared%s\n", "");
					goto suspend_goto_restart;
				}
			} else {
				/* bring the sig status back up */
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);
			}
		} /* if (sngss7_test_flag(sngss7_info, FLAG_INFID_RESUME)) */

		if (sngss7_test_ckt_flag(sngss7_info, FLAG_INFID_PAUSED)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing PAUSE%s\n", "");

			if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_SIG_UP)) {
				/* bring the sig status down */
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);
			}

			/* Wait for RESUME */
			goto suspend_goto_last;
		} /* if (sngss7_test_ckt_flag(sngss7_info, FLAG_INFID_PAUSED)) { */

		/**********************************************************************/
		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_CKT_MN_BLOCK_RX) &&
			!sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_RX_DN)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_MN_BLOCK_RX flag %s\n", "");

			/* bring the sig status down */
			sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);

			/* send a BLA */
			ft_to_sngss7_bla (ftdmchan);

			/* throw the done flag */
			sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_RX_DN);

		}

		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_CKT_MN_UNBLK_RX)){
			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_MN_UNBLK_RX flag %s\n", "");

			/* clear the block flags */
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_RX);
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_RX_DN);

			/* clear the unblock flag */
			sngss7_clear_ckt_blk_flag (sngss7_info, FLAG_CKT_MN_UNBLK_RX);

		    	SS7_DEBUG_CHAN(ftdmchan,"Current flags: ckt=0x%X, blk=0x%X\n", 
									sngss7_info->ckt_flags,
									sngss7_info->blk_flags);
			/* not bring the cic up if there is a hardware block */
			if (sngss7_channel_status_clear(sngss7_info)) {
				/* bring the sig status up */
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);

			}
			/* send a uba */
			ft_to_sngss7_uba (ftdmchan);

		}


		/**********************************************************************/
		/* hardware block/unblock tx */
		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_GRP_HW_BLOCK_TX ) &&
			!sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_TX_DN )) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing FLAG_GRP_HW_BLOCK_TX flag %s\n", "");
			sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);

			/* dont send block again if the channel is already blocked by maintenance */
			if( !sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_TX) &&
			     !sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_TX_DN) 
			   )  {
				ft_to_sngss7_blo(ftdmchan);
			}
			sngss7_set_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_TX_DN);

		}

		if (sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_UNBLK_TX)) {
			int skip_unblock=0;
			SS7_DEBUG_CHAN(ftdmchan, "Processing FLAG_GRP_HW_UNBLK_TX flag %s\n", "");
#if 0
			if (sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_TX) || 
			    sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_TX_DN)) {
				/* Real unblock */
			} else {
				SS7_ERROR_CHAN(ftdmchan, "FLAG_GRP_HW_UNBLK_TX set while FLAG_GRP_HW_BLOCK_TX is not %s\n", "");
				skip_unblock=1;
			}
#endif
				
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_TX);
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_TX_DN);
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_UNBLK_TX);
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_UNBLK_TX_DN);

			if (sngss7_channel_status_clear(sngss7_info)) {
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);
			}

			if (sngss7_tx_block_status_clear(sngss7_info) && !skip_unblock) {
				sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX_DN);
				sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX);
				ft_to_sngss7_ubl(ftdmchan);
			}

		}

		/**********************************************************************/
#if 0
		/* This logic is handled in the handle_cgu_req and handle_cgb_req */

		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_GRP_HW_BLOCK_RX ) &&
			!sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_RX_DN )) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing FLAG_GRP_HW_BLOCK_RX flag %s\n", "");

			sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);
			
			/* FIXME: Transmit CRG Ack */

			sngss7_set_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_RX_DN);

			goto suspend_goto_last;
		}

		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_GRP_HW_UNBLK_RX	)){
			SS7_DEBUG_CHAN(ftdmchan, "Processing FLAG_GRP_HW_UNBLK_RX flag %s\n", "");

			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_RX);
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_RX_DN);
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_UNBLK_RX);

			if (sngss7_channel_status_clear(sngs7_info)) {
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);
			}

			/* Transmit CRU Ack */

			goto suspend_goto_last;
		}
#endif


		/**********************************************************************/
		if (sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_TX) &&
			!sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_TX_DN)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_MN_BLOCK_TX flag %s\n", "");

			/* bring the sig status down */
			sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);

			/* send a blo */
			ft_to_sngss7_blo (ftdmchan);

			/* throw the done flag */
			sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_TX_DN);

		}
		
		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_CKT_MN_UNBLK_TX)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_MN_UNBLK_TX flag %s\n", "");

			/* clear the block flags */
			sngss7_clear_ckt_blk_flag (sngss7_info, FLAG_CKT_MN_BLOCK_TX);
			sngss7_clear_ckt_blk_flag (sngss7_info, FLAG_CKT_MN_BLOCK_TX_DN);

			/* clear the unblock flag */
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_UNBLK_TX);

			if (sngss7_channel_status_clear(sngss7_info)) {
				/* bring the sig status up */
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);
			}

			if (sngss7_tx_block_status_clear(sngss7_info)) {
				/* send a ubl */
				sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX_DN);
				sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX);
				ft_to_sngss7_ubl(ftdmchan);
			}

		}

		/**********************************************************************/
		if (sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX) &&
			!sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX_DN)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_LC_BLOCK_RX flag %s\n", "");

			/* send a BLA */
			ft_to_sngss7_bla(ftdmchan);

			/* throw the done flag */
			sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX_DN);
			
			if (sngss7_tx_block_status_clear(sngss7_info)) {
				sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX_DN);
				sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_BLOCK_RX);
				ft_to_sngss7_ubl(ftdmchan);
			} else {
				/* bring the sig status down */
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);
			}

		}

		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_CKT_LC_UNBLK_RX)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_LC_UNBLK_RX flag %s\n", "");

			/* clear the block flags */
			sngss7_clear_ckt_blk_flag (sngss7_info, FLAG_CKT_LC_BLOCK_RX);
			sngss7_clear_ckt_blk_flag (sngss7_info, FLAG_CKT_LC_BLOCK_RX_DN);
			
			/* clear the unblock flag */
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_LC_UNBLK_RX);

			/* send a uba */
			ft_to_sngss7_uba(ftdmchan);
			
			if (sngss7_channel_status_clear(sngss7_info)) {
				sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_UP, 0);
			}


		}
		/**********************************************************************/
		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_CKT_UCIC_BLOCK) &&
			!sngss7_test_ckt_blk_flag (sngss7_info, FLAG_CKT_UCIC_BLOCK_DN)) {

			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_UCIC_BLOCK flag %s\n", "");

			/* bring the channel signaling status to down */
			sngss7_set_sig_status(sngss7_info, FTDM_SIG_STATE_DOWN, 0);

			/* remove any reset flags */
			clear_rx_grs_flags(sngss7_info);
			clear_rx_grs_data(sngss7_info);
			clear_tx_grs_flags(sngss7_info);
			clear_tx_grs_data(sngss7_info);
			clear_rx_rsc_flags(sngss7_info);
			clear_tx_rsc_flags(sngss7_info);

			/* throw the done flag */
			sngss7_set_ckt_blk_flag(sngss7_info, FLAG_CKT_UCIC_BLOCK_DN);
			
		}

		if (sngss7_test_ckt_blk_flag (sngss7_info, FLAG_CKT_UCIC_UNBLK)) {
			SS7_DEBUG_CHAN(ftdmchan, "Processing CKT_UCIC_UNBLK flag %s\n", "");

			/* remove the UCIC block flag */
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_UCIC_BLOCK);
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_UCIC_BLOCK_DN);

			/* remove the UCIC unblock flag */
			sngss7_clear_ckt_blk_flag(sngss7_info, FLAG_CKT_UCIC_UNBLK);

			/* throw the channel into reset to sync states */
			
			clear_rx_grs_flags(sngss7_info);
			clear_rx_grs_data(sngss7_info);
			clear_tx_grs_flags(sngss7_info);
			clear_tx_grs_data(sngss7_info);
			clear_rx_rsc_flags(sngss7_info);
			clear_tx_rsc_flags(sngss7_info);

			clear_tx_rsc_flags(sngss7_info);
			sngss7_set_ckt_flag(sngss7_info, FLAG_RESET_TX);

			/* bring the channel into restart again */
			goto suspend_goto_restart;
		}

		SS7_DEBUG_CHAN(ftdmchan,"No block flag processed!%s\n", "");

suspend_goto_last:
		if (ftdmchan->last_state == FTDM_CHANNEL_STATE_UP) {
			/* proceed to UP */
		} else if (!sngss7_reset_status_clear(sngss7_info) || 
				   sngss7_test_ckt_flag(sngss7_info, FLAG_INFID_PAUSED)) {

			/* At this point the circuit is in reset, if the call is 
			   in use make sure that at least REMOTE REL flag is set
			   in order to drop the call on the sip side */
			if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_INUSE)) {
				if (!sngss7_test_ckt_flag (sngss7_info, FLAG_LOCAL_REL) &&
                    !sngss7_test_ckt_flag (sngss7_info, FLAG_REMOTE_REL)) {
					sngss7_set_ckt_flag (sngss7_info, FLAG_REMOTE_REL);	
				}
			}
			SS7_DEBUG_CHAN(ftdmchan,"Channel opted to stay in RESTART due to reset!%s\n", "");
			SS7_DEBUG_CHAN(ftdmchan,"Current flags: ckt=0x%X, blk=0x%X, circuit->flag=0x%X\n",
			                                         sngss7_info->ckt_flags, sngss7_info->blk_flags,
			                                         sngss7_info->circuit->flags );

			goto suspend_goto_restart;

		} else if (sngss7_channel_status_clear(sngss7_info)) {
			   
			/* In this case all resets and blocks are clear sig state is up, thus go to DOWN */
			if (ftdmchan->last_state == FTDM_CHANNEL_STATE_RESTART ||
				ftdmchan->last_state == FTDM_CHANNEL_STATE_TERMINATING) {
				ftdmchan->last_state = FTDM_CHANNEL_STATE_DOWN;
			}

			SS7_DEBUG_CHAN(ftdmchan,"Channel signallig is UP: proceed to State %s!\n", 
							ftdm_channel_state2str(ftdmchan->last_state));
			SS7_DEBUG_CHAN(ftdmchan,"Current flags: ckt=0x%X, blk=0x%X, circuit->flag=0x%X\n",
			                                         sngss7_info->ckt_flags, sngss7_info->blk_flags,
			                                         sngss7_info->circuit->flags );

		} else { 	

			if (ftdmchan->last_state == FTDM_CHANNEL_STATE_DOWN) {
				ftdmchan->last_state = FTDM_CHANNEL_STATE_RESTART;
			}
			SS7_DEBUG_CHAN(ftdmchan,"Channel signaling is in block state: proceed to State=%s]\n", 
							ftdm_channel_state2str(ftdmchan->last_state));
			SS7_DEBUG_CHAN(ftdmchan,"Current flags: ckt=0x%X, blk=0x%X, circuit->flag=0x%X\n",
			                                         sngss7_info->ckt_flags, sngss7_info->blk_flags,
			                                         sngss7_info->circuit->flags);
		}

		state_flag = 0;
		ftdm_set_state(ftdmchan, ftdmchan->last_state);
		break;

suspend_goto_restart:
		state_flag = 0;
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_RESTART);
		break;

	/**************************************************************************/
	case FTDM_CHANNEL_STATE_IN_LOOP:	/* COT test */

		isup_intf = &g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId];

		if (sngss7_test_options(isup_intf, SNGSS7_LPA_FOR_COT)) {
			/* send the lpa */
			ft_to_sngss7_lpa (ftdmchan);
		} 

		break;
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_IDLE:
			state_flag = 0;
			ftdm_set_state(ftdmchan, ftdmchan->last_state);
		break;
	/**************************************************************************/
	default:
		/* we don't handle any of the other states */
		SS7_ERROR_CHAN(ftdmchan, "ftmod_sangoma_ss7 does not support %s state\n",  ftdm_channel_state2str (ftdmchan->state));
		
		break;
	/**************************************************************************/
	}

	if (state_flag) {
		/* clear the state change flag...since we might be setting a new state */
		ftdm_channel_complete_state(ftdmchan);
	}
	return FTDM_SUCCESS;
}

/******************************************************************************/
static FIO_CHANNEL_INDICATE_FUNCTION(ftdm_sangoma_ss7_indicate)
{
    ftdm_status_t status = FTDM_FAIL;

    switch (indication) {
        case FTDM_CHANNEL_INDICATE_RAW:
            if (SNG_SS7_OPR_MODE_MTP2_API == g_ftdm_operating_mode) {
                return ftmod_sangoma_ss7_mtp2_indicate(ftdmchan);
            }
            break;
        default:
            status = FTDM_NOTIMPL;
    }
    return status;
}
/******************************************************************************/
static FIO_CHANNEL_OUTGOING_CALL_FUNCTION(ftdm_sangoma_ss7_outgoing_call)
{
	sngss7_chan_data_t  *sngss7_info = ftdmchan->call_data;

	/* the core has this channel already locked so need to lock again */

	/* check if the channel sig state is UP */
	if (!ftdm_test_flag(ftdmchan, FTDM_CHANNEL_SIG_UP)) {
		SS7_ERROR_CHAN(ftdmchan, "Requested channel sig state is down, skipping channell!%s\n", " ");
		/* Sig state will be down due to a block.  
		   Right action is to hunt for another call */
		goto outgoing_break;
	}

	/* check if there is a remote block */
	if ((sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_RX)) ||
		(sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_RX)) ||
		(sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_MN_BLOCK_RX))) {

		/* the channel is blocked...can't send any calls here */
		SS7_ERROR_CHAN(ftdmchan, "Requested channel is remotely blocked, re-hunt channel!%s\n", " ");
		goto outgoing_break;
	}

	/* check if there is a local block */
	if ((sngss7_test_ckt_blk_flag(sngss7_info, FLAG_CKT_MN_BLOCK_TX)) ||
		(sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_HW_BLOCK_TX)) ||
		(sngss7_test_ckt_blk_flag(sngss7_info, FLAG_GRP_MN_BLOCK_TX))) {

		/* KONRAD FIX ME : we should check if this is a TEST call and allow it */

		/* the channel is blocked...can't send any calls here */
		SS7_ERROR_CHAN(ftdmchan, "Requested channel is locally blocked, re-hunt channel!%s\n", " ");
		goto outgoing_break;
	}


    /* This is a gracefull stack resource check.
       Removing this function will cause unpredictable
	   ungracefule errors. */
	if (sng_cc_resource_check()) {
		goto outgoing_fail;
	}

	/* check the state of the channel */
	switch (ftdmchan->state){
	/**************************************************************************/
	case FTDM_CHANNEL_STATE_DOWN:
		/* inform the monitor thread that we want to make a call by returning FTDM_SUCCESS */
		
		goto outgoing_successful;
		break;
	/**************************************************************************/
	default:
		/* the channel is already used...this can't be, end the request */
		SS7_ERROR("Outgoing call requested channel in already in use...indicating glare on span=%d,chan=%d\n",
					ftdmchan->physical_span_id,
					ftdmchan->physical_chan_id);

		goto outgoing_break;
		break;
	/**************************************************************************/
	} /* switch (ftdmchan->state) (original call) */

outgoing_fail:
	SS7_DEBUG_CHAN(ftdmchan, "Call Request failed%s\n", " ");
	return FTDM_FAIL;

outgoing_break:
	SS7_DEBUG_CHAN(ftdmchan, "Call Request re-hunt%s\n", " ");
	return FTDM_BREAK;

outgoing_successful:
	SS7_DEBUG_CHAN(ftdmchan, "Call Request successful%s\n", " ");
	return FTDM_SUCCESS;
}

/******************************************************************************/
#if 0
	  static FIO_CHANNEL_REQUEST_FUNCTION (ftdm_sangoma_ss7_request_chan)
	  {
	SS7_INFO ("KONRAD-> I got called %s\n", __FUNCTION__);
	return FTDM_SUCCESS;
	  }

#endif

/******************************************************************************/

/* FT-CORE SIG STATUS FUNCTIONS ********************************************** */
static FIO_CHANNEL_GET_SIG_STATUS_FUNCTION(ftdm_sangoma_ss7_get_sig_status)
{
	if (ftdm_test_flag (ftdmchan, FTDM_CHANNEL_SIG_UP)) {
		*status = FTDM_SIG_STATE_UP;
	} else {
		*status = FTDM_SIG_STATE_DOWN;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static FIO_CHANNEL_SET_SIG_STATUS_FUNCTION(ftdm_sangoma_ss7_set_sig_status)
{
    ftdm_status_t rstatus = FTDM_FAIL;
    if (SNG_SS7_OPR_MODE_MTP2_API == g_ftdm_operating_mode) {
        rstatus = ftmod_sangoma_ss7_mtp2_set_sig_status(ftdmchan, status);
    } else {
        SS7_ERROR ("Cannot set channel status in this module\n");
        rstatus = FTDM_NOTIMPL;
    }

    return rstatus;
}

/* FT-CORE SIG FUNCTIONS ******************************************************/
static ftdm_status_t ftdm_sangoma_ss7_start(ftdm_span_t * span)
{
	ftdm_channel_t		*ftdmchan = NULL;
	sngss7_chan_data_t	*sngss7_info = NULL;
	sngss7_span_data_t 	*sngss7_span = NULL;
	sng_isup_inf_t		*sngss7_intf = NULL;
	int 			x;
	int			first_channel;

	first_channel=0;


	SS7_INFO ("Starting span %s:%u.\n", span->name, span->span_id);

    if (SNG_SS7_OPR_MODE_MTP2_API == g_ftdm_operating_mode) {
        return sngss7_activate_mtp2api(span);
    }

	/* clear the monitor thread stop flag */
	ftdm_clear_flag (span, FTDM_SPAN_STOP_THREAD);
	ftdm_clear_flag (span, FTDM_SPAN_IN_THREAD);

	/* check the status of all isup interfaces */
	check_status_of_all_isup_intf();

	/* throw the channels in pause */
	for (x = 1; x < (span->chan_count + 1); x++) {
		/* extract the channel structure and sngss7 channel data */
		ftdmchan = span->channels[x];

		if (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_OPEN)) {
			continue;
		}

		/* if there is no sig mod data move along */
		if (ftdmchan->call_data == NULL) continue;

		sngss7_info = ftdmchan->call_data;
		sngss7_span = ftdmchan->span->signal_data;
		sngss7_intf = &g_ftdm_sngss7_data.cfg.isupIntf[sngss7_info->circuit->infId];

		/* flag the circuit as active so we can receieve events on it */
		sngss7_set_flag(sngss7_info->circuit, SNGSS7_ACTIVE);

		/* if this is a non-voice channel, move along cause we're done with it */
		if (sngss7_info->circuit->type != SNG_CKT_VOICE) continue;

		/* lock the channel */
		ftdm_mutex_lock(ftdmchan->mutex);

		/* check if the interface is paused or resumed */
		if (sngss7_test_flag(sngss7_intf, SNGSS7_PAUSED)) {
			SS7_DEBUG_CHAN(ftdmchan, "ISUP intf %d is PAUSED\n", sngss7_intf->id);
			/* throw the pause flag */
			sngss7_clear_ckt_flag(sngss7_info, FLAG_INFID_RESUME);
			sngss7_set_ckt_flag(sngss7_info, FLAG_INFID_PAUSED);
		} else {
			SS7_DEBUG_CHAN(ftdmchan, "ISUP intf %d is RESUMED\n", sngss7_intf->id);
			/* throw the resume flag */
			sngss7_clear_ckt_flag(sngss7_info, FLAG_INFID_PAUSED);
			sngss7_set_ckt_flag(sngss7_info, FLAG_INFID_RESUME);
		}
#if 0
		/* throw the grp reset flag */
		sngss7_set_ckt_flag(sngss7_info, FLAG_GRP_RESET_TX);
		if (first_channel == 0) {
			sngss7_chan_data_t *cinfo = ftdmchan->call_data;
			sngss7_set_ckt_flag(sngss7_info, FLAG_GRP_RESET_BASE);
			cinfo->tx_grs.circuit = sngss7_info->circuit->id;
			cinfo->tx_grs.range = span->chan_count -1;
			first_channel=1;
		}
#else
		/* throw the channel into reset */
		sngss7_set_ckt_flag(sngss7_info, FLAG_RESET_TX);
#endif
		/* throw the channel to suspend */
		ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);

		/* unlock the channel */
		ftdm_mutex_unlock(ftdmchan->mutex);
	}

	/* activate all the configured ss7 links */
	if (ft_to_sngss7_activate_all()) {
		SS7_CRITICAL ("Failed to activate LibSngSS7!\n");
		return FTDM_FAIL;
	}

	/*start the span monitor thread */
	if (ftdm_thread_create_detached (ftdm_sangoma_ss7_run, span) != FTDM_SUCCESS) {
		SS7_CRITICAL ("Failed to start Span Monitor Thread!\n");
		return FTDM_FAIL;
	}

	SS7_DEBUG ("Finished starting span %s:%u.\n", span->name, span->span_id);

	return FTDM_SUCCESS;
}

/******************************************************************************/
ftdm_status_t ftdm_sangoma_ss7_stop(ftdm_span_t * span)
{
	/*this function is called by the FT-Core to stop this span */
	int timeout=0;

	ftdm_log (FTDM_LOG_INFO, "Stopping span %s:%u.\n", span->name,span->span_id);

	/* throw the STOP_THREAD flag to signal monitor thread stop */
	ftdm_set_flag (span, FTDM_SPAN_STOP_THREAD);

	/* wait for the thread to stop */
	while (ftdm_test_flag (span, FTDM_SPAN_IN_THREAD)) {
		ftdm_set_flag (span, FTDM_SPAN_STOP_THREAD);
		ftdm_log (FTDM_LOG_DEBUG,"Waiting for monitor thread to end for %s:%u. [flags=0x%08X]\n",
									span->name,
									span->span_id,
									span->flags);
		/* Wait 50ms */
		ftdm_sleep (50);
		timeout++;

		/* timeout after 5 sec, better to crash than hang */
		ftdm_assert_return(timeout < 100, FTDM_FALSE, "SS7 Span stop timeout!\n");
	}

	/* KONRAD FIX ME - deconfigure any circuits, links, attached to this span */

    if (SNG_SS7_OPR_MODE_M2UA_SG == g_ftdm_operating_mode) {
        ftmod_ss7_m2ua_span_stop(span->span_id);
    }

	ftdm_log (FTDM_LOG_DEBUG, "Finished stopping span %s:%u.\n", span->name, span->span_id);

	return FTDM_SUCCESS;
}

/* SIG_FUNCTIONS ***************************************************************/
static FIO_CONFIGURE_SPAN_SIGNALING_FUNCTION(ftdm_sangoma_ss7_span_config)
{
	sngss7_span_data_t *ss7_span_info;
	ftdm_sngss7_call_reject_queue_t *reject_queue = NULL;

	ftdm_log (FTDM_LOG_INFO, "Configuring ftmod_sangoma_ss7 span = %s(%d)...\n",
			span->name,
			span->span_id);

	/* initalize the span's data structure */
	ss7_span_info = ftdm_calloc (1, sizeof (sngss7_span_data_t));

	/* create a timer schedule */
	if (ftdm_sched_create(&ss7_span_info->sched, "SngSS7_Schedule")) {
		SS7_CRITICAL("Unable to create timer schedule!\n");
		return FTDM_FAIL;
	}

	/* start the free run thread for the schedule */
	if (ftdm_sched_free_run(ss7_span_info->sched)) {
		SS7_CRITICAL("Unable to schedule free run!\n");
		return FTDM_FAIL;
	}

	/* create an event queue for this span */
	if ((ftdm_queue_create(&(ss7_span_info)->event_queue, SNGSS7_EVENT_QUEUE_SIZE)) != FTDM_SUCCESS) {
		SS7_CRITICAL("Unable to create event queue!\n");
		return FTDM_FAIL;
	}

	/*setup the span structure with the info so far */
	g_ftdm_sngss7_data.sig_cb 	= sig_cb;
	span->start 			= ftdm_sangoma_ss7_start;
	span->stop 			= ftdm_sangoma_ss7_stop;
	span->signal_type 		= FTDM_SIGTYPE_SS7;
	span->signal_data 		= NULL;
	span->indicate                  = ftdm_sangoma_ss7_indicate;
	span->outgoing_call 		= ftdm_sangoma_ss7_outgoing_call;
	span->channel_request 		= NULL;
	span->signal_cb 		= sig_cb;
	span->get_channel_sig_status	= ftdm_sangoma_ss7_get_sig_status;
	span->set_channel_sig_status 	= ftdm_sangoma_ss7_set_sig_status;
	span->state_map			= &sangoma_ss7_state_map;
	span->state_processor 		= ftdm_sangoma_ss7_process_state_change;
	span->signal_data		= ss7_span_info;

	/* set the flag to indicate that this span uses channel state change queues */
	ftdm_set_flag (span, FTDM_SPAN_USE_CHAN_QUEUE);
	/* set the flag to indicate that this span uses sig event queues */
	ftdm_set_flag (span, FTDM_SPAN_USE_SIGNALS_QUEUE);
	ftdm_set_flag (span, FTDM_SPAN_USE_SKIP_STATES);



	/* parse the configuration and apply to the global config structure */
	if (ftmod_ss7_parse_xml(ftdm_parameters, span)) {
		ftdm_log (FTDM_LOG_CRIT, "Failed to parse configuration!\n");
		ftdm_sleep (100);
		return FTDM_FAIL;
	}

	if(SNG_SS7_OPR_MODE_M2UA_SG == g_ftdm_operating_mode){
		ftdm_log (FTDM_LOG_INFO, "FreeTDM running as M2UA_SG mode, Setting Span type to FTDM_SIGTYPE_M2UA\n"); 
		span->signal_type = FTDM_SIGTYPE_M2UA;
	}

	if (SNG_SS7_OPR_MODE_MTP2_API == g_ftdm_operating_mode) {
		if(FTDM_SUCCESS != sngss7_cfg_mtp2api(span)){
			ftdm_log (FTDM_LOG_CRIT, "Failed to configure LibSngSS7: MTP2 API \n");
			return FTDM_FAIL;
		}
	} else if (ft_to_sngss7_cfg_all()) { /* configure libsngss7 */
		ftdm_log (FTDM_LOG_CRIT, "Failed to configure LibSngSS7!\n");
		ftdm_sleep (100);
		return FTDM_FAIL;
	}

	if ((g_ftdm_sngss7_data.cfg.sng_acc)) {
		sngss7_reject_queue.ss7_call_rej_qsize = 1000; /* Default it to 1000 value */
		reject_queue =&sngss7_reject_queue;
		if (!reject_queue->sngss7_call_rej_queue) {
			ftdm_log(FTDM_LOG_DEBUG, "Creating global SS7 ACC Call reject queue!\n");
		}

		/* create global SS7 Call Reject queue */
		if (ftdm_queue_create(&reject_queue->sngss7_call_rej_queue, sngss7_reject_queue.ss7_call_rej_qsize) != FTDM_SUCCESS) {
			ftdm_log(FTDM_LOG_ERROR, "Failed to create global SS7 ACC Call reject queue!\n");
			ftdm_sleep(100);
			return FTDM_FAIL;
		}
	}

	ftdm_log (FTDM_LOG_INFO, "Finished configuring ftmod_sangoma_ss7 span = %s(%d)...\n",
			span->name,
			span->span_id);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static FIO_SIG_LOAD_FUNCTION(ftdm_sangoma_ss7_init)
{
	/*this function is called by the FT-core to load the signaling module */
	uint32_t major = 0;
	uint32_t minor = 0;
	uint32_t build = 0;

	ftdm_log (FTDM_LOG_INFO, "Loading ftmod_sangoma_ss7...\n");

	/* default the global structure */
	memset (&g_ftdm_sngss7_data, 0x0, sizeof (ftdm_sngss7_data_t));

	/* initializing global variables */
	sngss7_id = 0;
	cmbLinkSetId = 0;

	/* initializing global ACC queue structure */
	memset(&sngss7_reject_queue, 0, sizeof(sngss7_reject_queue));

	/* initializing global ACC structure */
	ss7_rmtcong_lst = NULL;

	/* initalize the global gen_config flag */
	g_ftdm_sngss7_data.gen_config = 0;

	/* function trace initizalation */
	g_ftdm_sngss7_data.function_trace = 1;
	g_ftdm_sngss7_data.function_trace_level = 7;

	/* message (IAM, ACM, ANM, etc) trace initizalation */
	g_ftdm_sngss7_data.message_trace = 1;
	g_ftdm_sngss7_data.message_trace_level = 6;

	/* setup the call backs needed by Sangoma_SS7 library */
	sng_event.cc.sng_con_ind = sngss7_con_ind;
	sng_event.cc.sng_con_cfm = sngss7_con_cfm;
	sng_event.cc.sng_con_sta = sngss7_con_sta;
	sng_event.cc.sng_rel_ind = sngss7_rel_ind;
	sng_event.cc.sng_rel_cfm = sngss7_rel_cfm;
	sng_event.cc.sng_dat_ind = sngss7_dat_ind;
	sng_event.cc.sng_fac_ind = sngss7_fac_ind;
	sng_event.cc.sng_fac_cfm = sngss7_fac_cfm;
	sng_event.cc.sng_sta_ind = sngss7_sta_ind;
	sng_event.cc.sng_umsg_ind = sngss7_umsg_ind;
	sng_event.cc.sng_susp_ind = sngss7_susp_ind;
	sng_event.cc.sng_resm_ind = sngss7_resm_ind;

	sng_event.sm.sng_log = handle_sng_log;
	sng_event.sm.sng_mtp1_alarm = handle_sng_mtp1_alarm;
	sng_event.sm.sng_mtp2_alarm = handle_sng_mtp2_alarm;
	sng_event.sm.sng_mtp3_alarm = handle_sng_mtp3_alarm;
	sng_event.sm.sng_mtp3_li_alarm = handle_sng_mtp3_li_alarm;
	sng_event.sm.sng_isup_alarm = handle_sng_isup_alarm;
	sng_event.sm.sng_cc_alarm = handle_sng_cc_alarm;
	sng_event.sm.sng_relay_alarm = handle_sng_relay_alarm;
	sng_event.sm.sng_m2ua_alarm = handle_sng_m2ua_alarm;
	sng_event.sm.sng_nif_alarm  = handle_sng_nif_alarm;
	sng_event.sm.sng_tucl_alarm = handle_sng_tucl_alarm;
	sng_event.sm.sng_sctp_alarm = handle_sng_sctp_alarm;

	/* MTP2 API sng ss7 call backs */
	sng_event.ap.sng_ap_dat_ind     = sngss7_mtp2api_dat_ind;
	sng_event.ap.sng_ap_dat_cfm     = sngss7_mtp2api_dat_cfm;
	sng_event.ap.sng_ap_con_cfm     = sngss7_mtp2api_con_cfm;
	sng_event.ap.sng_ap_disc_ind    = sngss7_mtp2api_disc_ind;
	sng_event.ap.sng_ap_sta_ind     = sngss7_mtp2api_sta_ind;
	sng_event.ap.sng_ap_sta_cfm     = sngss7_mtp2api_sta_cfm;
	sng_event.ap.sng_ap_flc_ind     = sngss7_mtp2api_flc_ind;
	sng_event.ap.is_mtp2api_enable  = sngss7_is_mtp2api_enable;


	/* initalize sng_ss7 library */
	sng_isup_init_gen(&sng_event);

	/* print the version of the library being used */
	sng_isup_version(&major, &minor, &build);
	SS7_INFO("Loaded LibSng-SS7 %d.%d.%d\n", major, minor, build);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static FIO_SIG_UNLOAD_FUNCTION(ftdm_sangoma_ss7_unload)
{
	/*this function is called by the FT-core to unload the signaling module */

	int x;

	ftdm_log (FTDM_LOG_INFO, "Starting ftmod_sangoma_ss7 unload...\n");

	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_CC_STARTED)) {
		sng_isup_free_cc();
		sngss7_clear_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_CC_STARTED);
	}

	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_ISUP_STARTED)) {
		ftmod_ss7_shutdown_isup();
		sng_isup_free_isup();
		sngss7_clear_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_ISUP_STARTED);
	}

	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP3_STARTED)) {
		ftmod_ss7_shutdown_mtp3();
		sng_isup_free_mtp3();
		sngss7_clear_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP3_STARTED);
	}

	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP2_STARTED)) {
		ftmod_ss7_shutdown_mtp2();
		sng_isup_free_mtp2();
		sngss7_clear_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP2_STARTED);
	}

	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP1_STARTED)) {
        ftmod_ss7_shutdown_mtp1();
		sng_isup_free_mtp1();
		sngss7_clear_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP1_STARTED);
	}

	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_RY_STARTED)) {
		/* go through all the relays channels and disable them */
		x = 1;
		while (x < (MAX_RELAY_CHANNELS)) {
			/* check if this relay channel has been configured already */
			if ((g_ftdm_sngss7_data.cfg.relay[x].flags & SNGSS7_CONFIGURED)) {
	
				/* send the specific configuration */
				if (ftmod_ss7_disable_relay_channel(x)) {
					SS7_CRITICAL("Relay Channel %d disable failed!\n", x);
					/* jz: dont leave like this 
					 * return 1; 
					 * */
				} else {
					SS7_INFO("Relay Channel %d disable DONE!\n", x);
				}
	
				/* set the SNGSS7_CONFIGURED flag */
				g_ftdm_sngss7_data.cfg.relay[x].flags &= ~(SNGSS7_CONFIGURED);
			} /* if !SNGSS7_CONFIGURED */
			x++;
		} /* while (x < (MAX_RELAY_CHANNELS)) */
		
		ftmod_ss7_shutdown_relay();
		sng_isup_free_relay();
		sngss7_clear_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_RY_STARTED);
	}

	if(SNG_SS7_OPR_MODE_ISUP != g_ftdm_operating_mode){
		ftmod_ss7_m2ua_free();
	}


	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_SM_STARTED)) {
		sng_isup_free_sm();
		sngss7_clear_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_SM_STARTED);
	}

	sng_isup_free_gen();

	ftdm_log (FTDM_LOG_INFO, "Finished ftmod_sangoma_ss7 unload!\n");
	return FTDM_SUCCESS;
}

/******************************************************************************/
static FIO_API_FUNCTION(ftdm_sangoma_ss7_api)
{
	/* handle this in it's own file....so much to do */
	return (ftdm_sngss7_handle_cli_cmd (stream, data));
}

/******************************************************************************/
static FIO_IO_LOAD_FUNCTION(ftdm_sangoma_ss7_io_init)
{
	assert (fio != NULL);
	memset (&g_ftdm_sngss7_interface, 0, sizeof (g_ftdm_sngss7_interface));
	g_ftdm_sngss7_interface.name = "ss7";
	g_ftdm_sngss7_interface.api = ftdm_sangoma_ss7_api;

	*fio = &g_ftdm_sngss7_interface;

	return FTDM_SUCCESS;
}

/******************************************************************************/

/* START **********************************************************************/
ftdm_module_t ftdm_module = {
	"sangoma_ss7",			/* char name[256]; 		  */
	ftdm_sangoma_ss7_io_init,	/* fio_io_load_t	 	  */
	NULL,				/* fio_io_unload_t		  */
	ftdm_sangoma_ss7_init,		/* fio_sig_load_t		  */
	NULL,				/* fio_sig_configure_t		  */
	ftdm_sangoma_ss7_unload,	/* fio_sig_unload_t		  */
	ftdm_sangoma_ss7_span_config 	/* fio_configure_span_signaling_t */
};
/******************************************************************************/

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
