/*
 * Copyright (c) 2015, Kapil Gupta <kgupta@sangoma.com>
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
 * Contributors:
 * 		Kapil Gupta <kgupta@sangoma.com>
 * 		Pushkar Singh <psingh@sangoma.com>
 *
 */
/******************************************************************************/
#ifndef __FTMOD_SNG_SS7_SCTP_H__
#define __FTMOD_SNG_SS7_SCTP_H__
/******************************************************************************/
#define MAX_NAME_LEN			25
#define MAX_SCTP_LINK 			100

/**********************************************
sctp structures and data definitions
**********************************************/

typedef struct sng_sctp_gen_cfg {
} sng_sctp_gen_cfg_t;

typedef struct sng_sctp_link {
	char		name[MAX_NAME_LEN];
	uint32_t	flags;
	uint32_t	id;
	uint32_t	port;
	uint32_t	numSrcAddr;
	int 		freeze_tmr;
	uint32_t	srcAddrList[SCT_MAX_NET_ADDRS+1];
	uint32_t 	hbeat_interval_tmr;
} sng_sctp_link_t;

typedef struct sng_sctp_cfg {
	sng_sctp_gen_cfg_t	genCfg;
	sng_sctp_link_t		linkCfg[MAX_SCTP_LINK+1];
} sng_sctp_cfg_t;


/**********************************************
sctp/tucl apis
**********************************************/
int ftmod_sctp_gen_config(void);
int ftmod_tucl_gen_config(void);
int ftmod_tucl_sap_config(int id);
int ftmod_sctp_config(int id);
int ftmod_cfg_sctp(int opr_mode);
ftdm_status_t ftmod_sctp_sap_config(int id);
ftdm_status_t ftmod_sctp_tsap_config(int id);
int ftmod_sctp_tucl_tsap_bind(int idx);
int ftmod_tucl_debug(int action);
int ftmod_sctp_debug(int action);
int ftmod_ss7_sctp_shutdown(void);
int ftmod_ss7_tucl_shutdown(void);

int ftmod_ss7_parse_sctp_links(ftdm_conf_node_t *node);
int ftmod_sctp_ssta_req(int elemt, int id, SbMgmt* cfm);
ftdm_status_t ftmod_m2ua_is_sctp_link_active(int sctp_id);

#endif /*__FTMOD_SNG_SS7_SCTP_H__*/

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
