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



#ifndef __RUN_MTP2_H__
#define __RUN_MTP2_H__

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>


#include <pthread.h>
#include <stdlib.h> /* required to generate random numbers */
#include <signal.h>

#include "src/ftmod/ftmod_sangoma_ss7/ftmod_sangoma_ss7_mtp2api.h"
#include "private/ftdm_core.h"

#define NUM_E1_CHANS 31
#define MAX_MSU_SIZE 272


typedef enum mtp2_link_flags {
	LINK_STATE_CONNECTED	= (1<<0),
	LINK_STATE_CONGESTED	= (1<<1),
} mtp2_link_flags_t;

typedef struct chan_data {
	uint8_t inuse;
	pthread_t thread;
	uint32_t span_id; /* Physical span number */
	uint32_t chan_id;	/* Physical chan number */
	mtp2_link_flags_t flags;
	unsigned long rx_frames;
	unsigned long tx_frames;
	uint8_t rx_seq;
	ftdm_channel_t *ftdmchan;
} chan_data_t;

typedef struct span_data {
	uint8_t inuse;
	ftdm_span_t *ftdmspan;
	chan_data_t channels[NUM_E1_CHANS+500];
} span_data_t;

void test_mtp2_links(void);
void print_status(void);
void print_frames(void);

FIO_SIGNAL_CB_FUNCTION(on_mtp2_signal);

#endif /* __RUN_MTP2_H__ */

