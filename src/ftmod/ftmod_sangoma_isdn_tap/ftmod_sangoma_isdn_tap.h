/*
 * Copyright (c) 2015, Pushkar Singh <psingh@sangoma.com>
 * Sangoma Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the original author; nor the names of any contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
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
 * Contributors: Pushkar Singh <psingh@sangoma.com>
 */

#ifndef __SNG_ISDN_TAP_H__
#define __SNG_ISDN_TAP_H__

/********************************************************************************/
/*                                                                              */
/*                           libraries inclusion                                */
/*                                                                              */
/********************************************************************************/
#define _GNU_SOURCE
#ifndef WIN32
#include <poll.h>
#endif

#include "private/ftdm_core.h"
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#ifdef WIN32
#include <io.h>
#endif
#include "sng_decoder.h"

/********************************************************************************/
/*                                                                              */
/*   		 	      COMMAND HANDLER MACROS                            */
/*                                                                              */
/********************************************************************************/
/* Syntax message */
#define FT_SYNTAX "USAGE:\n" \
"--------------------------------------------------------------------------------\n" \
"ftdm isdn_tap statistics\n" \
"ftdm isdn_tap show nfas config\n" \
"ftdm isdn_tap show span <span_id/all>\n" \
"ftdm isdn_tap io_stats <show/enable/disable/flush> span <span_id/all>\n" \
"--------------------------------------------------------------------------------\n"

/* Used to declare command handler */
#define COMMAND_HANDLER(name) \
	ftdm_status_t fCMD_##name(ftdm_stream_handle_t *stream, char *argv[], int argc); \
	ftdm_status_t fCMD_##name(ftdm_stream_handle_t *stream, char *argv[], int argc)

/* Used to define command entry in the command map. */
#define COMMAND(name, argc)  {#name, argc, fCMD_##name}

#define MAX_PCALLS 120
#define ISDN_TAP_DCHAN_QUEUE_LEN 900
#define MAX_TAP_SPANS 50

/* NFAS related defines */
#define MAX_NFAS_GROUPS 16
#define MAX_NFAS_GROUP_NAME 50
/* ideally maximum number of NFAS LINK should be 31 but it is set to 16 right to
 * save some memory */
#define MAX_SPANS_PER_NFAS_LINK 16

/* command handler function type */
typedef ftdm_status_t (*fCMD)(ftdm_stream_handle_t *stream, char *argv[], int argc);

/********************************************************************************/
/*                                                                              */
/*                         Structure Declearation                               */
/*                                                                              */
/********************************************************************************/
/* D channel controller structure */
struct sngisdn_t {
	ftdm_socket_t fd;		/* File descriptor for D-Channel */
	void *userdata;
	int debug;                      /* Debug stuff */
	int state;                      /* State of D-channel */
	int switchtype;         	/* Switch type */
	int localtype;			/* local network type */
	int cref;                       /* Next call reference value */
} sngisdn_t;

typedef enum {
	ISDN_TAP_RUNNING = (1 << 0),
	ISDN_TAP_MASTER = (1 << 1),
} sngisdn_tap_flags_t;

typedef enum {
	ISDN_TAP_MIX_BOTH = 0,
	ISDN_TAP_MIX_PEER,
	ISDN_TAP_MIX_SELF,
	ISDN_TAP_MIX_NULL,
} sngisdn_tap_mix_mode_t;

typedef enum {
	CALL_DIRECTION_INBOUND,
	CALL_DIRECTION_OUTBOUND,
	CALL_DIRECTION_NONE
} tap_call_direction_t;

typedef struct {
	uint32_t chan_id;
	uint32_t span_id;
	tap_call_direction_t call_dir; /* Gives Call direction i.e. incoming or outgoing */
	int call_ref;
	uint8_t call_con;	       /* Gives information if call is connected or it is a missed call */
	char calling_num[FTDM_DIGITS_LIMIT];
	char called_num[FTDM_DIGITS_LIMIT];
} isdn_tap_msg_info_t;

typedef struct {
	ftdm_number_t callingnum;
	ftdm_number_t callednum;
	ftdm_channel_t *fchan;
	int inuse;
	int callref_val;
	int chanId;
	time_t t_setup_recv;
	time_t t_con_recv;
	time_t t_msg_recv;
	ftdm_bool_t recv_setup;
	ftdm_bool_t recv_conn;
	ftdm_bool_t recv_other_msg;
	isdn_tap_msg_info_t tap_msg;
	ftdm_bool_t chan_pres;
} passive_call_t;

typedef enum sngisdn_tap_iface {
	ISDN_TAP_IFACE_UNKNOWN = 0,
	ISDN_TAP_IFACE_CPE = 1,
	ISDN_TAP_IFACE_NET = 2,
} sngisdn_tap_iface_t;

typedef enum {
	ISDN_TAP_NORMAL = 0,
	ISDN_TAP_BI_DIRECTION = 1,
} sngisd_tap_mode_t;

typedef enum {
	SNGISDN_NFAS_DCHAN_NONE,
	SNGISDN_NFAS_DCHAN_PRIMARY,
	SNGISDN_NFAS_DCHAN_BACKUP,
} sngisdn_tap_nfas_sigchan_t;

struct sngisdn_tap_nfas_data;
typedef struct sngisdn_tap_nfas_data sngisdn_tap_nfas_data_t;

typedef struct isdn_tap {
	int32_t flags;
	struct sngisdn_t *pri;
	int debug;
	int switch_type;
	sngisdn_tap_mix_mode_t mixaudio;
	sngisd_tap_mode_t tap_mode;
	ftdm_channel_t *dchan;
	ftdm_span_t *span;
	ftdm_span_t *peerspan;
	ftdm_mutex_t *pcalls_lock;
	passive_call_t pcalls[MAX_PCALLS];
	sngisdn_tap_iface_t iface;
	void *decCb;		    /* ISDN Decoder Control Block */
	ftdm_hash_t *pend_call_lst; /* Hash List for Calls for which connect is already received before SETUP */
	/* Incuding NFAS functionality */
	struct nfas_info {
		sngisdn_tap_nfas_sigchan_t sigchan;
		sngisdn_tap_nfas_data_t *trunk;
		uint8_t interface_id;
	} nfas;
	ftdm_bool_t indicate_event; /* Indicate event when RELEASE/DISCONNECT is received with all the information about the call */
} sngisdn_tap_t;

struct sngisdn_tap_nfas_data {
	char name[MAX_NFAS_GROUP_NAME];
	char backup_span_name[20];
	char dchan_span_name[20];
	sngisdn_tap_t *dchan;		/* Span that contains primary d-channel */
	sngisdn_tap_t *backup;		/* Span that contains backup d-channel */
	uint8_t num_tap_spans;		/* Number of spans within this NFAS */
	uint8_t num_spans_configured;
	ftdm_span_t *tap_spans[MAX_SPANS_PER_NFAS_LINK + 1]; /* indexed by logical span id */
};

typedef struct con_call_info {
	int callref_val;
	time_t conrecv_time;
	int msgType;
	ftdm_bool_t recv_setup;
	ftdm_bool_t recv_con;
	ftdm_bool_t tap_proceed;
} con_call_info_t;

/* Global sngisdn tap data */
typedef struct ftdm_sngisdn_tap_data {
	sngisdn_tap_nfas_data_t nfas[MAX_NFAS_GROUPS + 1];
	sngisdn_tap_t *tap_spans[MAX_TAP_SPANS];
	uint8_t num_nfas;
	uint8_t num_spans_configured;
}ftdm_sngisdn_tap_data_t;

/********************************************************************************/
/*                                                                              */
/*                               Function Prototype                             */
/*                                                                              */
/********************************************************************************/
/* ftmod_sangoma_isdn_tap.c Functions ***********************************************************************************************************************/
/* Functions to start send tap raw messages  and reset pcall */
ftdm_status_t sangoma_isdn_tap_set_tap_msg_info(sngisdn_tap_t *tap, uint8_t msgType, passive_call_t *pcall, sng_decoder_interface_t *intfCb);
ftdm_status_t sangoma_isdn_tap_reset_pcall(passive_call_t *pcall);

/* create pcall by checking if there is any other stale call with same call reference */
passive_call_t *sangoma_isdn_tap_create_pcall(sngisdn_tap_t *tap, uint8_t msgType, sng_decoder_interface_t *intfCb, int crv);

/* create new control block */
struct sngisdn_t *sangoma_isdn_new_cb(ftdm_socket_t fd, int nodetype, int switchtype, void *userdata);

/* get user data and set error/debug message */
void sangoma_isdn_set_message(void (*__pri_error)(struct sngisdn_t *pri, char *));
void sangoma_isdn_set_error(void (*__pri_error)(struct sngisdn_t *pri, char *));
void *sangoma_isdn_get_userdata(struct sngisdn_t *pri);

/* check is connect event is already being received */
ftdm_status_t sangoma_isdn_tap_is_con_recv(sngisdn_tap_t *tap, int crv, uint8_t msgType);

/* get ftdm channel by channel ID passed */
ftdm_channel_t* sangoma_isdn_tap_get_fchan_by_chanid(sngisdn_tap_t *tap, int channel);

/* state handler functions */
ftdm_status_t ftmod_isdn_process_event(ftdm_span_t *span, ftdm_event_t *event);
ftdm_status_t state_advance(ftdm_channel_t *ftdmchan);

/* Get pcall for same call reference value */
passive_call_t *sangoma_isdn_tap_get_pcall_bycrv(sngisdn_tap_t *tap, int crv);
passive_call_t *sangoma_isdn_tap_get_pcall(sngisdn_tap_t *tap, void *callref);

/* print pcall values */
void sangoma_isdn_tap_print_pcall(sngisdn_tap_t *tap);

/* check span state */
void sangoma_isdn_tap_check_state(ftdm_span_t *span);

/* release pcall */
void sangoma_isdn_tap_put_pcall(sngisdn_tap_t *tap, void *callref);

/* send raw data i.e. tap message with all the information on receipt of
 * RELEASE/DISCONNECT message */
void send_protocol_raw_data(isdn_tap_msg_info_t *data, ftdm_channel_t *ftdmchan);

/* ftmod_sangoma_isdn_bidir_tap.c Functions ***********************************************************************************************************************/
/* functions based on ISDN messages */
ftdm_status_t sangoma_isdn_bidir_tap_handle_event_con(sngisdn_tap_t *tap, uint8_t msgType, sng_decoder_interface_t *intfCb, int crv);
ftdm_status_t sangoma_isdn_bidir_tap_generate_invite(sngisdn_tap_t *tap, passive_call_t  *pcall, int crv);
ftdm_status_t sangoma_isdn_bidir_tap_handle_event_rel(sngisdn_tap_t *tap, uint8_t msgType, int crv);

/* channel functions */
ftdm_channel_t *sangoma_isdn_bidir_tap_get_fchan(sngisdn_tap_t *tap, passive_call_t *pcall, int channel, int force_clear_chan, tap_call_direction_t call_dir, ftdm_bool_t to_open);

/********************************************************************************/
/*                                                                              */
/*                                     MACROS                                   */
/*                                                                              */
/********************************************************************************/
/* Node Types */
#define PRI_NETWORK             1
#define PRI_CPE                 2

#define SNG_ISDN_SPAN(p) (((p) >> 8) & 0xff)
#define SNG_ISDN_CHANNEL(p) ((p) & 0xff)

#define ISDN_TAP_NETWORK_ANSWER 0x1

/* Stale call duration - upto this time we will maintain call lists */
#define SNG_TAP_STALE_TME_DURATION 60
/* Timer duration to receive subsequent message */
#define SNG_TAP_SUBEQUENT_MSG_TME_DURATION 5

/* Debugging Levels */
#define DEBUG_Q921_RAW              (1 << 0)        /* Show raw HDLC frames */
#define DEBUG_ALL                   (0xffff)        /* Everything */

/* Transmit capabilities */
#define TRANS_CAP_SPEECH            	0x0
#define TRANS_CAP_DIGITAL           	0x08
#define TRANS_CAP_RESTRICTED_DIGITAL    0x09
#define TRANS_CAP_3_1K_AUDIO            0x10
#define TRANS_CAP_7K_AUDIO              0x11 	    /* Depriciated ITU Q.931 (05/1998)*/
#define TRANS_CAP_DIGITAL_W_TONES       0x11
#define TRANS_CAP_VIDEO                 0x18
#define LAYER_1_ITU_RATE_ADAPT      	0x21
#define LAYER_1_ULAW                    0x22
#define LAYER_1_ALAW                    0x23
#define LAYER_1_G721                    0x24
#define LAYER_1_G722_G725           	0x25
#define LAYER_1_H223_H245           	0x26
#define LAYER_1_NON_ITU_ADAPT       	0x27
#define LAYER_1_V120_RATE_ADAPT     	0x28
#define LAYER_1_X31_RATE_ADAPT      	0x29

/* ISDN MSGTYPE DECODE **********************************************************/
#define SNG_DECODE_ISDN_EVENT(msgType) \
((msgType) == Q931_ALERTING)        ?"ALERT" : \
((msgType) == Q931_PROCEEDING)      ?"CALL PROCEED" : \
((msgType) == Q931_PROGRESS)        ?"PROGRESS" : \
((msgType) == Q931_SETUP)           ?"SETUP" : \
((msgType) == Q931_CONNECT)         ?"CONNECT" : \
((msgType) == Q931_SETUP_ACK)       ?"SETUP ACK" : \
((msgType) == Q931_CONNECT_ACK)     ?"CONNECT ACK" : \
((msgType) == Q931_USER_INFO)       ?"USER INFO" : \
((msgType) == Q931_SUSPEND_REJ)     ?"SUSPEND REJECT" : \
((msgType) == Q931_RESUME_REJ)      ?"RESUME REJECT" : \
((msgType) == Q931_SUSPEND)         ?"SUSPEND" : \
((msgType) == Q931_RESUME)          ?"RESUME" : \
((msgType) == Q931_SUSPEND_ACK)     ?"SUSPEND ACK" : \
((msgType) == Q931_RESUME_ACK)      ?"RESUME ACK" : \
((msgType) == Q931_DISCONNECT)      ?"DISCONNECT" : \
((msgType) == Q931_RESTART)         ?"RESTART" : \
((msgType) == Q931_RELEASE)         ?"RELEASE" : \
((msgType) == Q931_RESTART_ACK)     ?"RESTART ACK" : \
((msgType) == Q931_RELEASE_COMPLETE)?"RELEASE COMPLETE" : \
((msgType) == Q931_SEGMENT)         ?"SEGMENT" : \
((msgType) == Q931_FACILITY)        ?"FACILITY" : \
((msgType) == Q931_NOTIFY)          ?"NOTIFY" : \
((msgType) == Q931_STATUS_ENQUIRY)  ?"STATUS ENQUIRY" : \
((msgType) == Q931_CONGESTION_CNTRL)?"CONGESTION CONTROL" : \
((msgType) == Q931_INFORMATION)     ?"INFORMATION" : \
((msgType) == Q931_STATUS)          ?"STATUS" : \
"Unknown ISDN event"

/* ISDN TAP MODE DECODE *******************************************************/
#define SNG_DECODE_ISDN_TAP_MODE(tapmode) \
((tapmode) == ISDN_TAP_NORMAL)	    	?"NORMAL (E1 T1 Tapping)" : \
((tapmode) == ISDN_TAP_BI_DIRECTION)    ?"BI DIRECTIONAL" : \
"Unknown ISDN Tapping Mode"

/* ISDN TAP INTERFACE TYPE DECODE *********************************************/
#define SNG_ISDN_TAP_GET_INTERFACE(iface) \
((iface) == ISDN_TAP_IFACE_CPE) ? "CPE" : \
((iface) == ISDN_TAP_IFACE_NET  ? "NET" : \
"Unknown interface"

/* ISDN TAP NFAS SPAN TYPE DECODE *********************************************/
#define SNG_ISDN_TAP_GET_NFAS_SPAN_TYPE(sigchan) \
((sigchan) == SNGISDN_NFAS_DCHAN_NONE)     ? "VOICE" : \
((sigchan) == SNGISDN_NFAS_DCHAN_PRIMARY)  ? "PRIMARY SIGNALLING" : \
((sigchan) == SNGISDN_NFAS_DCHAN_BACKUP)   ? "BACKUP SIGNALLING" : \
"Unknown NFAS span type"

/********************************************************************************/
/*                                                                              */
/*                               global declaration                             */
/*                                                                              */
/********************************************************************************/
extern sng_decoder_event_interface_t    sng_event;
extern ftdm_io_interface_t ftdm_sngisdn_tap_interface;

/* Global counters to get the exact number of calls received/tapped/released etc. */
extern uint64_t setup_recv;
extern uint64_t reuse_chan_con_recv;	/* Counter to keep track of connect recv on which we found channel in use error */
extern uint64_t setup_recv_invalid_chanId;
extern uint64_t setup_not_recv;
extern uint64_t connect_recv_with_chanId;
extern uint64_t connect_recv;
extern uint64_t early_connect_recv;
extern uint64_t late_invite_recv;
extern uint64_t call_proceed_recv;
extern uint64_t release_recv;
extern uint64_t disconnect_recv;
extern uint64_t call_tapped;
extern uint64_t msg_recv_aft_time;

#endif /* __SNG_ISDN_TAP_H__ */
