/*
 * FreeWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2012, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Anthony Minessale II <anthm@freeswitch.org>
 * Kapil Gupta <kgupta@sangoma.com>
 * Pushkar Singh <psingh@sangoma.com>
 *
 * mod_freetdm.h -- FreeTDM Endpoint Module header
 *
 */

/******************************************************************************/
#ifndef __MOD_FREETDM_H__
#define __MOD_FREETDM_H__
/******************************************************************************/

/* INCLUDES *******************************************************************/
#include <switch.h>
#include "freetdm.h"

/* ENUMS **********************************************************************/
typedef enum {
	ANALOG_OPTION_NONE = 0,
	ANALOG_OPTION_3WAY = (1 << 0),
	ANALOG_OPTION_CALL_SWAP = (1 << 1)
} analog_option_t;

typedef enum {
	FTDM_LIMIT_RESET_ON_TIMEOUT = 0,
	FTDM_LIMIT_RESET_ON_ANSWER = 1
} limit_reset_event_t;

/* STRUCTURES *****************************************************************/
typedef struct chan_pvt {
	unsigned int flags;
} chan_pvt_t;

typedef struct {
	switch_memory_pool_t *pool;
	switch_pollset_t *pollset;
} tchan_general_t;

struct span_config {
	ftdm_span_t *span;
	char dialplan[80];
	char context[80];
	char dial_regex[256];
	char fail_dial_regex[256];
	char hold_music[256];
	char type[256];
	analog_option_t analog_options;
	const char *limit_backend;
	int limit_calls;
	int limit_seconds;
	limit_reset_event_t limit_reset_event;
	/* digital codec and digital sampling rate are used to configure the codec
	 * when bearer capability is set to unrestricted digital */
	const char *digital_codec;
	int digital_sampling_rate;
	/* to get span status i.e. is span is in start state or not */
	ftdm_bool_t span_start;
	chan_pvt_t pvts[FTDM_MAX_CHANNELS_SPAN];
	/* per span memory pool allocation & pollset created */
	tchan_general_t  tchan_gen;
	/* per span socket hash list based on socket fd created */
	switch_hash_t *sock_list;
};

/* FUNCTIONS ******************************************************************/
ftdm_status_t parse_transparent_spans(switch_xml_t cfg, switch_xml_t spans);
ftdm_status_t transparent_span_start(switch_xml_t cfg, switch_xml_t spans);

/* EXTERN VARIABLES ***********************************************************/
extern struct span_config SPAN_CONFIG[FTDM_MAX_SPANS_INTERFACE];
extern int span_configured[FTDM_MAX_CHANNELS_SPAN];

/******************************************************************************/
#endif /* __MOD_FREETDM_H__ */
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
