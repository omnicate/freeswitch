/*
 * Copyright (c) 2011, David Yat Sin <dyatsin@sangoma.com>
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
 * 		Kapil Gupta <kgupta@sangoma.com>
 * 		Pushkar Singh <psingh@sangoma.com>
 */
/******************************************************************************/

/* This file is only required when running in mtp2 api mode */
#ifndef __FTMOD_SANGOMA_SS7_MTP2_H__
#define __FTMOD_SANGOMA_SS7_MTP2_H__

#include "freetdm.h" /* So the user does not have to include freetdm.h*/

typedef enum
{
	SNGSS7_REASON_UNUSED	= 0,			/* Not set */
	SNGSS7_REASON_SM,						/* A disconnect was requested for this link */
	SNGSS7_REASON_SUERM,					/* Intolerably high signal unit error rate (SUERM threshold reached) */
	SNGSS7_REASON_ACK,						/* Excessive delay of acknowledgements from remote MTP2 (T7 Expired) */
	SNGSS7_REASON_TE,						/* Failure of signalling terminal equipment (disconnect from L1 Layer) */
	SNGSS7_REASON_BSN,						/* 2 of 3 unreasonable BSN */
	SNGSS7_REASON_FIB,						/* 2 of 3 unreasonable FIB */
	SNGSS7_REASON_CONG,					/* Excessive periods of level 2 congestion (T6 Expired */
	SNGSS7_REASON_LSSU_SIOS,				/* SIO/SIOS received in link state machine */
	SNGSS7_REASON_TMR2_EXP,				/* Timer T2 expired waiting for SIO from remote during alignment */
	SNGSS7_REASON_TMR3_EXP,				/* Timer T3 expired waiting for SIN/SIE from remote during alignment */
	SNGSS7_REASON_LSSU_SIOS_IAC,			/* SIOS received during alignment */
	SNGSS7_REASON_PROV_FAIL,				/* Proving failure (ERM threshold reached) */
	SNGSS7_REASON_TMR1_EXP,				/* Timer T1 expired waiting for FISU/MSU from remote after successful proving */
	SNGSS7_REASON_LSSU_SIN,				/* SIN received in in-service state */
	SNGSS7_REASON_CTS_LOST,				/* CTS lost (disconnect from MAC layer) */
	SNGSS7_REASON_DAT_IN_OOS,				/* Request to transmit data in Out-Of-Service state */
	SNGSS7_REASON_DAT_IN_WAITFLUSHCONT,	/* Request to transmit data when MTP2 is waiting for flush/continue directive */
	SNGSS7_REASON_RETRV_IN_INS,			/* Received message retrieval request from user in in-service state of the link */
	SNGSS7_REASON_CON_IN_INS,				/* Received connect request in in-service state of the link */
	SNGSS7_REASON_UPPER_SAP_DIS,			/* Received management control request to disable SAP towards user */
	SNGSS7_REASON_REM_PROC_DN,			/* Remote MTP2 processor is down */
	SNGSS7_REASON_REM_PROC_UP,			/* Remote MTP2 processor is up */
	SNGSS7_REASON_START_FLC,			/* Start of flow control */
	SNGSS7_REASON_END_FLC,				/* End of flow control */
} sngss7_reason_t;

typedef enum
{
	SNGSS7_SIGSTATUS_NORM,					/* Request MTP2 link to initialize normal alignment procedure */
	SNGSS7_SIGSTATUS_EMERG,					/* Request MTP2 link to initialize alignment procedure in emergency mode*/
	SNGSS7_SIGSTATUS_FLC_STAT,				/* Request flow control status  */
	SNGSS7_SIGSTATUS_LOC_PROC_UP,			/* Indicate to remote side that local processor is up */
	SNGSS7_SIGSTATUS_LOC_PROC_DN,			/* Indicate to remote side that local processor is down */
} sngss7_sigstatus_t;


typedef enum
{
	SNGSS7_REQ_FLUSH_BUFFERS, 		/* Request to flush buffers */
	SNGSS7_REQ_CONTINUE,				/* Continue */
	SNGSS7_REQ_RETRV_BSN, 			/* Request to retrieve backwards sequence number */
	SNGSS7_REQ_RETRV_MSG,			/* Request to retrieve messages based on backward sequence number */
	SNGSS7_REQ_DROP_MSGQ,			/* Request MTP2 to drop transmit queue messages */
} sngss7_req_t;


typedef enum
{
	SNGSS7_IND_RETRV_BSN,			/* Response to a SNGSS7_RAW_ID_REQ_RETRV_BSN  */
	SNGSS7_IND_DATA_MORE,			/* Response to a SNGSS7_RAW_ID_REQ_RETRV_MSG, more frames to come */
	SNGSS7_IND_DATA_NO_MORE,			/* Response to a SNGSS7_RAW_ID_REQ_RETRV_MSG, this is the last frame */
	SNGSS7_IND_DATA_EMPTY,			/* Response to a SNGSS7_RAW_ID_REQ_RETRV_MSG, no messages to be returned */
} sngss7_ind_t;



#endif /* __FTMOD_SANGOMA_SS7_MTP2_H__ */

/******************************************************************************/
/* For Emacs:
 *  Local Variables:
 *  mode:c
 *  indent-tabs-mode:t
 *  tab-width:4
 *  c-basic-offset:4
 *  End:
 *  For VIM:
 *  vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
/******************************************************************************/
