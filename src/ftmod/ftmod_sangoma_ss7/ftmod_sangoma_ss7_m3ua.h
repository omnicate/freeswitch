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
 * 		Kapil Gupta <kgupt@sangoma.com>
 * 		Pushkar Singh <psingh@sangoma.com>
 *
 */

/******************************************************************************/
#ifndef __FTMOD_SNG_SS7_M3UA_H__
#define __FTMOD_SNG_SS7_M3UA_H__
/******************************************************************************/
#include "private/ftdm_core.h"

/*********************************************************************
 * * M3UA definitions
 * *********************************************************************/
#define MAX_M3UA_PS     100
#define MAX_M3UA_PSP    100
#define M3UA_SELF_ROUTE_IDX    50
#define M3UA_MTP3_ROUTE_IDX    200
#define MAX_M3UA_RTE    500
#define MAX_M3UA_SAP    10
#define MAX_PSP_PER_PS  LIT_MAX_PSP /* 20 */
#define MAX_APPEARANCE_SIZE     32
#define MAX_PSP_LIST_LENGTH 1000
#define MAX_NAME_LEN			25

/**********************************************
  m3ua data structures and definitions
 **********************************************/
typedef enum {
	M3UA_ANSI = 1,
	M3UA_BICI,
	M3UA_ANSI96,
	M3UA_ITU,
	M3UA_CHINA,
	M3UA_NTT,
	M3UA_TCC
} sng_m3ua_sust_t;

typedef enum {
	UNUSED = 0,
	ANSI_NET,
	ANSI_TCAP,
	ETSI_TCAP,
	ITU_TCAP,
	ITU_NET,
	NSS_NET,
	TTC_TCAP
} sng_m3ua_su2st_t;

typedef enum {
	M3UA_NODETYPE_ASP = 1,
	M3UA_NODETYPE_SGP
} sng_m3ua_nodetype_t;

typedef struct sng_m3ua_nwk_cfg {
	int 			id;
	uint32_t           	flags;
	int 	  		ssf;
	uint32_t		dpcLen;
	uint32_t		slsLen;
	int	                suSwitchType;
	int               	isup_switch_type;  /* switch mapped to ISUP */
	int             	su2SwitchType;
	char			appearance[MAX_APPEARANCE_SIZE];
	char			name[MAX_NAME_LEN];
} sng_m3ua_nwk_cfg_t;

typedef enum {
	M3UA_MODE_ACTSTANDBY =0,
	M3UA_MODE_LOADSHARE
} sng_m3ua_mode_t;

typedef struct sng_m3ua_ps_cfg {
	int 				id;
	uint32_t    flags;
	int				isLocal;		/* true | false. if set to 0, that's false; if set to 1, that's true. */
	sng_m3ua_mode_t	mode;		/* activeStandby or loadShare */
	int				nwkId;
	int				rteCtxId;
	int 		    pspNum;
	int				pspIdList[MAX_PSP_PER_PS];
	char		    name[MAX_NAME_LEN];
} sng_m3ua_ps_cfg_t;

typedef enum {
	M3UA_ASPUP_SPID_TX =0,	/* include spId only in Tx */
	M3UA_ASPUP_SPID_RX,		/* include spId only in Rx */
	M3UA_ASPUP_SPID_BOTH,	/* include spId only in Tx and Rx */
	M3UA_ASPUP_SPID_NONE	/* do NOT include spId only in ASPUP/ASPUP ACK message */
} sng_m3ua_aspup_spid_t;


typedef struct sng_m3ua_psp_cfg {
	int 		id;
	uint32_t       	flags;
	int           	remoteType;
	int            	ipspMode;
	int 		drkmAllowed; 		/* Dynamic Routing Key Management. 0 is false not allowed; 1 is true allowed */
	int 		nwkAppearanceIncl; 	/* Network Appearance Included in communication with peers.
						0 is false not included; 1 is true included. */
	sng_m3ua_aspup_spid_t	    txRxSpId;
	int		selfSpId;
	int		nwkId;
	uint32_t 	primDestAddr;
	uint32_t 	optDestAddr[SCT_MAX_NET_ADDRS];
	uint32_t 	numOptDestAddr;
	int		destPort;
	int		sctpId;
	int		streamNum;
	int		isClientSide;	/* am I in client mode when communicating with this peer.
					   0 is false - no peer is client; 1 is true  - yes, I'm the client.*/
	int 		tos;	/* type of service */
	int 		init_sctp;	/* type of service */
	char		name[MAX_NAME_LEN];
}sng_m3ua_psp_cfg_t;

typedef enum {
	M3UA_ROUTE_INVALID = 0,
	M3UA_ROUTE_SELF    = 1,
	M3UA_ROUTE_MTP3    = 2,
	M3UA_ROUTE_IP      = 3,
	M3UA_ROUTE_UNKNOWN = 4,
} sng_m3ua_route_type_t;


typedef struct sng_m3ua_rte_cfg {
	int id;
	int m3ua_rte_id;        /* same as id..only in case of self-route it will be different */
	uint32_t    flags;
	int dpc;
	int dpcMask;
	int spc;
	int spcMask;
	int sio;
	int sioMask;
	int nwkId;
	int psId;
	int nsapId;
	int rtType;
	char name[MAX_NAME_LEN];
	int includeSsn;
	int ssn;
} sng_m3ua_rte_cfg_t;

typedef struct sng_m3ua_sap_cfg {
	int 					id;
	uint32_t                flags;
	int						nwkId;
	int                  	suType;
	char 					name[MAX_NAME_LEN];
}sng_m3ua_sap_cfg_t;

typedef struct sng_m3ua_nif_cfg {
	int 					id;
	char 					name[MAX_NAME_LEN];
	uint32_t                flags;
	int						m3ua_route_id;
	int						mtp3_route_id;
	int 			nwkId;
}sng_m3ua_nif_cfg_t;

typedef enum {
	SNG_M3UA_TIMER_ASP_UP        = 0x1,     /* ASP UP Timer Event */
	SNG_M3UA_TIMER_ASP_ACTIVE    = 0x2,     /* ASP ACTIVE Timer Event */
	SNG_M3UA_TIMER_SCTP_ASSOC    = 0x3,     /* SCTP Association establishment */
	SNG_M3UA_TIMER_REG_ROUTE_KEYS   = 0x4,  /* iM3UA Routing keys registration */
}sng_m3ua_tmr_evt_types_e;

typedef struct sng_m3ua_tmr {
	ftdm_sched_t            *tmr_sched;
	ftdm_timer_id_t         tmr_id;
    sng_m3ua_tmr_evt_types_e tmr_event;
    int                      id;            /* identifier could be peer_id/suId */
} sng_m3ua_tmr_sched_t;

typedef struct sng_m3ua_gen_cfg {
	ftdm_bool_t hbeat_tmr_enable;
	uint32_t    hbeat_tmr_val;
} sng_m3ua_gen_cfg_t;

typedef struct sng_m3ua_gbl_cfg{
	sng_m3ua_nwk_cfg_t 	nwkCfg[MAX_M3UA_NWK];   /* M3UA Network configuration                   */
	sng_m3ua_ps_cfg_t 	psCfg[MAX_M3UA_PS];     /* M3UA Peer Server Configuration               */
	sng_m3ua_psp_cfg_t 	pspCfg[MAX_M3UA_PSP];   /* M3UA Peer Server Process Configuration       */
	sng_m3ua_rte_cfg_t 	rteCfg[MAX_M3UA_RTE];   /* M3UA Route Configuration                     */
	sng_m3ua_sap_cfg_t	sapCfg[MAX_M3UA_SAP];   /* M3UA (User) Upper Sap Configuration          */
	sng_m3ua_nif_cfg_t	nifCfg[MAX_M3UA_RTE];   /* M3UA (User) Upper Sap Configuration          */
	sng_m3ua_tmr_sched_t	sched;
	sng_m3ua_gen_cfg_t 	gen_cfg;
}sng_m3ua_gbl_cfg_t;

/* m3ua stack logging enable/disable api's */
void ftmod_ss7_enable_m3ua_logging(void);
void ftmod_ss7_disable_m3ua_logging(void);

/* m3ua xml parsing APIs */
ftdm_status_t ftmod_ss7_parse_m3ua_networks(ftdm_conf_node_t *node);
ftdm_status_t ftmod_ss7_parse_m3ua_peerservers(ftdm_conf_node_t *node);
ftdm_status_t ftmod_ss7_parse_m3ua_psps(ftdm_conf_node_t *node);
ftdm_status_t ftmod_ss7_parse_m3ua_users(ftdm_conf_node_t *node);
ftdm_status_t ftmod_ss7_parse_m3ua_routes(ftdm_conf_node_t *node, int opr_mode);
ftdm_status_t ftmod_ss7_parse_m3ua_nif(ftdm_conf_node_t *node);

/* M3UA Stack config/control APIs */
ftdm_status_t ftmod_ss7_m3ua_init(int opr_mode);
ftdm_status_t ftmod_ss7_m3ua_cfg(int opr_mode);
ftdm_status_t ftmod_ss7_m3ua_start(int opr_mode);
ftdm_status_t ftmod_m3ua_debug(int action);
void ftmod_ss7_m3ua_free(void);

int ftmod_m3ua_ssta_req(int elemt, int id, ItMgmt* cfm);

void ftdm_m3ua_start_timer(sng_m3ua_tmr_evt_types_e evt_type , int peer_id);
ftdm_status_t ftmod_ss7_fill_m3ua_route(sng_m3ua_rte_cfg_t *t_rte, sng_m3ua_route_type_t type);
int ftmod_m3ua_cfg_mtp3_m3ua_routes(int opr_mode);
ftdm_status_t ftmod_m3ua_is_local_ps(int psid);
ftdm_status_t ftmod_m3ua_asp_active(int id, int action);
ftdm_status_t ftmod_m3ua_asp_up(int id, int action);

#endif /*__FTMOD_SNG_SS7_M3UA_H__*/

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
