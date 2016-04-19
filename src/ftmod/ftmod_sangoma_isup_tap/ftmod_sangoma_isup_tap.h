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

#ifndef __SNG_ISUP_TAP_H__
#define __SNG_ISUP_TAP_H__

/********************************************************************************/
/*                                                                              */
/*                         Structure Declearation                               */
/*                                                                              */
/********************************************************************************/
/* Signalling channel controller structure */
typedef struct {
	int32_t spanno;
	int32_t channo;
} sng_sig_chan_t;

struct sngisup_t {
	ftdm_socket_t fd;		/* File descriptor for D-Channel */
	void *userdata;
	int debug;                      /* Debug stuff */
	int state;                      /* State of D-channel */
	int cic;                        /* Cic Value */
	int id;
} sngisup_t;

typedef enum {
	ISUP_TAP_RUNNING = (1 << 0),
	ISUP_TAP_MASTER = (1 << 1),
} sngisup_tap_flags_t;

typedef enum {
	ISUP_TAP_MIX_BOTH = 0,
	ISUP_TAP_MIX_PEER,
	ISUP_TAP_MIX_SELF,
} sngisup_tap_mix_mode_t;

typedef struct {
	ftdm_number_t callingnum;
	ftdm_number_t callednum;
	ftdm_channel_t *fchan;
	uint8_t proceeding;
	int inuse;
	int cic_val;
	int chanId;
} passive_call_t;

typedef struct voice_span {
	uint32_t span_id;
	ftdm_span_t *span;
}sngisup_voice_span_t;

typedef struct isup_tap {
	int32_t flags;
	struct sngisup_t *ss7_isup;
	int debug;
	int switch_type;
	int cicbase;
	sngisup_tap_mix_mode_t mixaudio;
	ftdm_channel_t *dchan;
	ftdm_span_t *span;
	sngisup_voice_span_t voice_span[100];
	ftdm_bool_t is_voice;
	ftdm_span_t *peerspan;
	ftdm_mutex_t *pcalls_lock;
	passive_call_t pcalls[FTDM_MAX_CHANNELS_PHYSICAL_SPAN];
	sng_sig_chan_t      sng;
	sangoma_wait_obj_t      *sng_wait_obj;
	sangoma_wait_obj_t      *tx_sng_wait_obj;
	void *decCb;    	/* ISUP Decoder Control Block */
} sngisup_tap_t;

/********************************************************************************/
/*                                                                              */
/*                               Function Prototype                             */
/*                                                                              */
/********************************************************************************/
void *sangoma_isup_get_userdata(struct sngisup_t *ss7_isup);
struct sngisup_t *sangoma_isup_new_cb(ftdm_socket_t fd, void *userdata);

/********************************************************************************/
/*                                                                              */
/*                                     MACROS                                   */
/*                                                                              */
/********************************************************************************/
#define SNG_ISUP_SPAN(p) (((p) >> 8) & 0xff)
#define SNG_ISUP_CHANNEL(p) ((p) & 0xff)

/* Debug Level */
#define DEBUG_ALL 	(0xffff)        /* Everything */

#define ISUP_TAP_NETWORK_ANSWER 0x1
#define SNG_ISUP 0x05

#define FTDM_IS_SS7_CHANNEL(fchan) ((fchan)->type != 8)
/* ISUP EVENT DECODE **********************************************************/
#define SNG_DECODE_ISUP_EVENT(msgType) \
((msgType) == M_ADDCOMP)        ?"ADDRESS COMPLETE" : \
((msgType) == M_ANSWER)         ?"ANSWER" : \
((msgType) == M_BLOCK)          ?"BLOCK" : \
((msgType) == M_BLOCKACK)       ?"BLOCK ACK" : \
((msgType) == M_CALLMODCOMP)    ?"CALL MODIFY COMPLETE" : \
((msgType) == M_CALLMODREQ)     ?"CALL MODIFY REQUEST" : \
((msgType) == M_CALLMODREJ)     ?"CALL MODIFY REJECT" : \
((msgType) == M_CALLPROG)       ?"CALL PROGRESS" : \
((msgType) == M_CIRGRPBLK)      ?"CIRCUIT GROUP BLOCK" : \
((msgType) == M_CIRGRPBLKACK)   ?"CIRCUIT GROUP BLOCK ACK" : \
((msgType) == M_CIRGRPQRY)      ?"CIRCUIT GROUP QUERY" : \
((msgType) == M_CIRGRPQRYRES)   ?"CIRCUIT GROUP QUERY RESPONSE" : \
((msgType) == M_CIRGRPRES)      ?"CIRCUIT GROUP RESET" : \
((msgType) == M_CIRGRPRESACK)   ?"CIRCUIT GROUP RESET ACK" : \
((msgType) == M_CIRGRPUBLK)     ?"CIRCUIT GROUP UNBLOCK" : \
((msgType) == M_CIRGRPUBLKACK)  ?"CIRCUIT GROUP UNBLOCK ACK" : \
((msgType) == M_CONFUSION)      ?"CONFUSION" : \
((msgType) == M_CONNCT)         ?"CONNECT" : \
((msgType) == M_CONTINUITY)     ?"CONTINUITY" : \
((msgType) == M_CONTCHKREQ)     ?"CONTINUITY CHECK REQUEST" : \
((msgType) == M_DELREL)         ?"DELAYED RELEASE" : \
((msgType) == M_FACACC)         ?"FACILITY ACCEPTED" : \
((msgType) == M_FACREJ)         ?"FACILITY REJECTED" : \
((msgType) == M_FACREQ)         ?"FACILITY REQUESTED" : \
((msgType) == M_FWDTFER)        ?"FORWARD TRANSFER" : \
((msgType) == M_INFORMTN)       ?"INFORMATION" : \
((msgType) == M_INFOREQ)        ?"INFORMATION REQUEST" : \
((msgType) == M_INIADDR)        ?"INITIAL ADDRESS" : \
((msgType) == M_LOOPBCKACK)     ?"LOOP BACK ACK" : \
((msgType) == M_OVERLOAD)       ?"OVERLOAD" : \
((msgType) == M_PASSALNG)       ?"PASS ALONG" : \
((msgType) == M_RELSE)          ?"RELEASE" : \
((msgType) == M_RELCOMP)        ?"RELEASE COMPLETE" : \
((msgType) == M_RESCIR)         ?"RESET CIRCUIT" : \
((msgType) == M_RESUME)         ?"RESUME" : \
((msgType) == M_SUBADDR)        ?"SUBSEQUENT ADDRESS" :\
((msgType) == M_SUSPND)         ?"SUSPEND" : \
((msgType) == M_UNBLK)          ?"UNBLOCK" : \
((msgType) == M_UNBLKACK)       ?"UNBLOCK ACK" : \
"Unknown ISUP Event"

#endif /* __SNG_ISUP_TAP_H__ */
