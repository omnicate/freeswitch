/*
 * Copyright (c) 2011, David Yat Sin <dyatsin@sangoma.com>
 * Sangoma Technologies
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
 *
 */

#include "run_mtp2.h"

#define STACK_SIZE 1024*1024

extern span_data_t SPANS[FTDM_MAX_SPANS_INTERFACE];
extern int g_running;
extern int g_num_threads;

static void *mtp2_chan_run (void* chan_ptr);
void verify_receive_data(chan_data_t *chan, void *vdata, uint32_t datalen);

#define MIN_MSU_SIZE 8


void print_frames(void)
{
	int i,j;
	unsigned print_buf_len = 0;
	char print_buf [10000];
	memset(print_buf, 0, 10000);
	
	print_buf_len += sprintf(&print_buf[print_buf_len], "\n======================================================================================================================================\n");

	print_buf_len += sprintf(&print_buf[print_buf_len], "Format: <Timeslot>: <Number of Tx Frames>/<Number of Rx Frames>");
	for (i = 0; i < FTDM_MAX_SPANS_INTERFACE; i++) {
		if (SPANS[i].inuse) {
			print_buf_len += sprintf(&print_buf[print_buf_len], "\nSpan :%d\n\t",i);
			for (j = 0; j < NUM_E1_CHANS; j++) {
				if (SPANS[i].channels[j].inuse) {
					print_buf_len += sprintf(&print_buf[print_buf_len], "%02d:%06ld/%06ld    ", j, SPANS[i].channels[j].tx_frames, SPANS[i].channels[j].rx_frames);
					if (j && !((j+1)%6)) print_buf_len += sprintf(&print_buf[print_buf_len], "\n\t");
				}
			}
		}
	}	
	print_buf_len += sprintf(&print_buf[print_buf_len], "\n======================================================================================================================================\n");
	fprintf(stdout, "%s\n", print_buf);
	fflush(stdout);
	return;
}

void print_status(void)
{
	int i,j;
	unsigned print_buf_len = 0;
	char print_buf [1000];
	memset(print_buf, 0, 1000);
	
	
	for (i = 0; i < FTDM_MAX_SPANS_INTERFACE; i++) {
		if (SPANS[i].inuse) {
			print_buf_len += sprintf(&print_buf[print_buf_len], "\nSpan :%d\n\t",i);
			for (j = 0; j < NUM_E1_CHANS; j++) {
				if (SPANS[i].channels[j].inuse) {
					ftdm_signaling_status_t sigstatus;
					if (ftdm_channel_get_sig_status(SPANS[i].channels[j].ftdmchan, &sigstatus) != FTDM_SUCCESS) {
						fprintf(stderr, "s%dc%d: Failed to get signalling status on channel\n", SPANS[i].channels[j].span_id, SPANS[i].channels[j].chan_id);
					} else {
						print_buf_len += sprintf(&print_buf[print_buf_len], "%02d:%s \t",
															j,
															(sigstatus == FTDM_SIG_STATE_UP)?"Aligned":
															(sigstatus == FTDM_SIG_STATE_SUSPENDED)?"Suspended":
															(sigstatus == FTDM_SIG_STATE_DOWN)?"Disconnected":"Unknown");
					}
					
					if (j && !((j+1)%6)) print_buf_len += sprintf(&print_buf[print_buf_len], "\n\t");
				}
			}
		}
	}
	print_buf_len += sprintf(&print_buf[print_buf_len], "\n===========================================================================================================================================================\n");
	fprintf(stdout, "%s\n", print_buf);
	fflush(stdout);
	return;
}


static int launch_test_thread(chan_data_t *chan)
{
	pthread_attr_t attr;
	int result = -1;

	result = pthread_attr_init(&attr);
	if (result) {
		fprintf(stderr, "Failed to init attribute\n");
		return result;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, STACK_SIZE);
	result = pthread_create(&chan->thread, &attr, mtp2_chan_run, (void*)chan);
	if (result) {
		fprintf(stderr, "Failed to create thread!!\n");
	}
	pthread_attr_destroy(&attr);
	return result;
}


void test_mtp2_links(void)
{
	int i, j=0;
	ftdm_channel_t *ftdmchan = NULL;
	ftdm_iterator_t *chaniter = NULL;
	ftdm_iterator_t *curr = NULL;
	
	for (i = 0; i < FTDM_MAX_SPANS_INTERFACE; i++) {
		if (SPANS[i].inuse) {
			j = 0;
			chaniter = ftdm_span_get_chan_iterator(SPANS[i].ftdmspan, chaniter);
			for (curr = chaniter; curr; curr = ftdm_iterator_next(curr)) {
				ftdmchan = ftdm_iterator_current(curr);
				SPANS[i].channels[j].span_id = ftdm_channel_get_ph_span_id(ftdmchan);
				SPANS[i].channels[j].chan_id = ftdm_channel_get_ph_id(ftdmchan);
				SPANS[i].channels[j].ftdmchan = ftdmchan;
				SPANS[i].channels[j].inuse = 1;

				fprintf(stdout, "Launching test thread for span:%d channel:%d\n", SPANS[i].channels[j].span_id, SPANS[i].channels[j].chan_id);

				if (launch_test_thread(&SPANS[i].channels[j])) {
					g_running = 0;
					ftdm_iterator_free(chaniter);
					return;
				}

				j++;
				g_num_threads++;
			}
		}
	}
	ftdm_iterator_free(chaniter);
	fprintf(stdout, "Number of tests launched:%d\n", g_num_threads);
}

/* Request MTP2 to start alignment attempt */
static void request_mtp2_connect(ftdm_channel_t *ftdmchan)
{
	ftdm_usrmsg_t usrmsg;
	
	memset(&usrmsg, 0, sizeof(usrmsg));

	/* usrmsg.raw_id = SNGSS7_SIGSTATUS_UP_NORM; */
	usrmsg.raw_id = SNGSS7_SIGSTATUS_EMERG;

	ftdm_channel_set_sig_status_ex(ftdmchan, FTDM_SIG_STATE_UP,  &usrmsg);
	return;
}

/* Check MTP2 flow control status */
static void request_mtp2_flc_status(ftdm_channel_t *ftdmchan)
{
	ftdm_usrmsg_t usrmsg;
	
	memset(&usrmsg, 0, sizeof(usrmsg));

	/* usrmsg.raw_id = SNGSS7_SIGSTATUS_UP_NORM; */
	usrmsg.raw_id = SNGSS7_SIGSTATUS_FLC_STAT;

	ftdm_channel_set_sig_status_ex(ftdmchan, FTDM_SIG_STATE_UP,  &usrmsg);
	return;
}


void *mtp2_chan_run (void* chan_ptr)
{
	unsigned int span_id;
	unsigned int chan_id;
	ftdm_channel_t *ftdmchan;
	uint8_t *data;
	ftdm_size_t datalen;
	uint32_t sleeptime;
	uint8_t transmit_data = 0x00;
	chan_data_t *chan = (chan_data_t*)chan_ptr;
	ftdm_status_t status = FTDM_SUCCESS;
	
	data = malloc(MAX_MSU_SIZE);
	if (!data) {
		fprintf(stderr, "Failed to allocate memory\n");
		pthread_exit(NULL);
	}
	
	span_id = chan->span_id;
	chan_id = chan->chan_id;

	fprintf(stdout, "Opening channel (span:%d chan:%d)\n", span_id, chan_id);
	status = ftdm_channel_open(span_id, chan_id, &ftdmchan);

	if (!ftdmchan || (status != FTDM_SUCCESS && status != FTDM_EBUSY)) {
		fprintf(stdout,"-ERR Failed to open channel %d in span %d\n", chan_id, span_id);
		if(FTDM_EBUSY == status)
		{
			fprintf(stdout,"Channel already opened \n");
		}
		exit(1);
	}


	ftdm_channel_set_private(ftdmchan, chan);

	sleep(1);

	/* Transmit dummy MSU's */
	while(g_running) {
		if (!(chan->flags & LINK_STATE_CONNECTED)) {
			fprintf(stdout, "s%dc%d: Link is down\n", span_id, chan_id);

			request_mtp2_connect(ftdmchan);			
			sleep(1);
			continue;
		}
		if (chan->flags & LINK_STATE_CONGESTED) {
			fprintf(stdout, "s%dc%d: Link in congested state\n", span_id, chan_id);
			request_mtp2_flc_status(ftdmchan);			
			
			/* Cannot transmit when link is in congested state, wait for congested condition to clear */
			usleep(10000);
			continue;
		}
		datalen = (uint32_t)((float)rand()/(float)RAND_MAX*(float)(MAX_MSU_SIZE));

		if (datalen > MIN_MSU_SIZE) {
			datalen -= 1; /* So that datalen is never == MAX_MSU_SIZE; */

			/* Fill in the dummy buffer with some information so we can verify data when the MSU is received */
			data[0] = span_id;
			data[1] = chan_id;
			data[2] = (datalen & 0xFF);
			data[3] = ((datalen >> 8) & 0xFF);

			memset(&data[4], transmit_data++, (datalen-4));

			if (ftdm_channel_write(ftdmchan, data, MAX_MSU_SIZE, &datalen) != FTDM_SUCCESS) {
				fprintf(stderr, "Failed to write to channel\n");
				break;
			}
			chan->tx_frames++;

			/* Sleep, otherwise we will enter congestion state each time because we would transmit
			 * as fast as we can in the sample application. Sleep is not required in a real application */

			/* We transmit 8 bytes in 1 ms, so it takes datalen/8 ms to transmit a MSU + ~10 bytes for FISU*/
			sleeptime = (uint32_t)(((float)datalen/(float)8)+10);			
			usleep(sleeptime*1000);
		}
	}

	ftdm_channel_close(&ftdmchan);
	free(data);
	g_num_threads--;	
	pthread_exit(NULL);
}

void verify_receive_data(chan_data_t *chan, void *vdata, uint32_t datalen)
{

	//uint32_t rx_span_id;
	uint32_t rx_chan_id, rx_datalen;
	uint8_t *data = (uint8_t *)vdata;
	uint8_t rx_seq;
	
	if (datalen < MIN_MSU_SIZE) {
		fprintf(stderr, "Invalid MSU length received %d\n", datalen);
		g_running = 0;
		return;
	}

//	rx_span_id = data[0];
	rx_chan_id = data[1];
	rx_datalen = data[2];
	rx_datalen |= ((data[3] & 0xFF) << 8);
	rx_seq		= data[4];

#if 0
	if (chan->span_id != rx_span_id) {
		fprintf(stderr, "Invalid data: span id RX:%d expected: %d\n", rx_span_id, chan->span_id);
		g_running = 0;
		return;
	}
#endif

	if (chan->chan_id != rx_chan_id) {
		fprintf(stderr, "Invalid data: chan id RX:%d expected: %d\n", rx_chan_id, chan->chan_id);
		g_running = 0;
		return;
	}
	
	if (datalen != rx_datalen) {
		fprintf(stderr, "Invalid data: Data Length RX:%d expected: %d\n", rx_datalen, datalen);
		g_running = 0;
		return;
	}

	if (chan->rx_seq != rx_seq) {
		fprintf(stderr, "Dropped data: RX:0x%02x expected:0x%02x \n", rx_seq, chan->rx_seq);
		g_running = 0;
		return;
	}
	
	chan->rx_seq = rx_seq+1;
	chan->rx_frames++;
}

FIO_SIGNAL_CB_FUNCTION(on_mtp2_signal)
{
	unsigned int span_id, chan_id;
	ftdm_channel_t *ftdmchan;
	chan_data_t *chan;
	
	ftdmchan = sigmsg->channel;
	chan = (chan_data_t*)ftdm_channel_get_private(ftdmchan);

	span_id = ftdm_channel_get_ph_span_id(ftdmchan);
	chan_id = ftdm_channel_get_ph_id(ftdmchan);

	switch(sigmsg->event_id) {
		case FTDM_SIGEVENT_IO_INDATA:
			/* We received an incoming frame */
			verify_receive_data(chan, sigmsg->raw.data, sigmsg->raw.len);
			break;
		case FTDM_SIGEVENT_RAW:
			{
				switch (sigmsg->ev_data.raw_id) {
					case SNGSS7_IND_RETRV_BSN:			/* Response to a SNGSS7_RAW_ID_REQ_RETRV_BSN  */
						break;
					case SNGSS7_IND_DATA_MORE:			/* Response to a SNGSS7_RAW_ID_REQ_RETRV_MSG, more frames to come */
						break;
					case SNGSS7_IND_DATA_NO_MORE:		/* Response to a SNGSS7_RAW_ID_REQ_RETRV_MSG, this is the last frame */
						break;
				}
			}
			break;
		case FTDM_SIGEVENT_SIGSTATUS_CHANGED:
			{
				fprintf(stdout, "s%dc%d: Sig status changed to:%s\n", span_id, chan_id, ftdm_signaling_status2str(sigmsg->ev_data.sigstatus.status));
				switch (sigmsg->ev_data.sigstatus.status) {
					case FTDM_SIG_STATE_UP:
						chan->flags |= LINK_STATE_CONNECTED;
						chan->flags &= ~LINK_STATE_CONGESTED;
						
						switch (sigmsg->ev_data.sigstatus.raw_reason) {
							case SNGSS7_REASON_END_FLC:			/* End of flow control */
								fprintf(stdout, "s%dc%d: Flow control end\n", span_id, chan_id);
								break;
							case SNGSS7_REASON_REM_PROC_UP:			/* Remote MTP2 processor is UP */
								fprintf(stdout, "s%dc%d: Remote processor up\n", span_id, chan_id);
								break;
							default:
								fprintf(stdout, "s%dc%d: MTP2 Link aligned\n", span_id, chan_id);
								break;
						}
						break;
					case FTDM_SIG_STATE_DOWN:
						{
							uint8_t raw_reason = sigmsg->ev_data.sigstatus.raw_reason;
							chan->flags &= ~LINK_STATE_CONNECTED;
							
							fprintf(stdout, "s%dc%d: MTP2 Link down: \"%s\" \n", span_id, chan_id,
													(raw_reason == SNGSS7_REASON_SM) ?"Stack Manager requested DISC":
													(raw_reason == SNGSS7_REASON_SUERM) ?"SUERM threshold reached":
													(raw_reason == SNGSS7_REASON_ACK) ?"Excessive delay of acknowledgements from remote MTP2":
													(raw_reason == SNGSS7_REASON_TE) ?"Failure of Terminal equipment":
													(raw_reason == SNGSS7_REASON_BSN) ?"2 of 3 unreasonable BSN":
													(raw_reason == SNGSS7_REASON_FIB) ?"2 of 3 unreasonable FIB":
													(raw_reason == SNGSS7_REASON_CONG) ?"Excessive periods of congestion":
													(raw_reason == SNGSS7_REASON_LSSU_SIOS) ?"SIO/SIOS received in link state machine":
													(raw_reason == SNGSS7_REASON_TMR2_EXP) ?"Timer T2 expired":
													(raw_reason == SNGSS7_REASON_TMR3_EXP) ?"Timer T3 expired":
													(raw_reason == SNGSS7_REASON_LSSU_SIOS_IAC) ?"SIOS received during alignment":
													(raw_reason == SNGSS7_REASON_PROV_FAIL) ?"ERM threshold reached":
													(raw_reason == SNGSS7_REASON_TMR1_EXP) ?"Timer T1 expired":
													(raw_reason == SNGSS7_REASON_LSSU_SIN) ?"SIN received in in-service state":
													(raw_reason == SNGSS7_REASON_CTS_LOST) ?"Disconnect from L1":
													(raw_reason == SNGSS7_REASON_DAT_IN_OOS) ?"Request to transmit data in OOS":
													(raw_reason == SNGSS7_REASON_DAT_IN_WAITFLUSHCONT) ?"Request to transmit when MTP2 is waiting for flush/continue directive":
													(raw_reason == SNGSS7_REASON_RETRV_IN_INS) ?"Received message retrieval request while in-service state":
													(raw_reason == SNGSS7_REASON_CON_IN_INS) ?"Received Connect request in in-service state":
													(raw_reason == SNGSS7_REASON_UPPER_SAP_DIS) ?"Received management request to disable SAP" : "Unknown");
						}
						break;
					case FTDM_SIG_STATE_SUSPENDED:						
						switch (sigmsg->ev_data.sigstatus.raw_reason) {
							case SNGSS7_REASON_START_FLC:			/* Start of flow control */
								fprintf(stdout, "s%dc%d: Flow control start\n", span_id, chan_id);
								chan->flags |= LINK_STATE_CONGESTED;
								break;
							case SNGSS7_REASON_REM_PROC_DN:			/* Remote MTP2 processor is down */
								fprintf(stdout, "s%dc%d: Remote processor down\n", span_id, chan_id);
								break;
							default:
								fprintf(stdout, "s%dc%d: Invalid reason for link change state\n", span_id, chan_id);
						}
						break;
					default:
						break;
				}
			}
			break;
		default:
			break;
	}
	return FTDM_SUCCESS;
}
