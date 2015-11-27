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
 * 		Pushkar Signh <psingh@sangoma.com>
 *
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/
/* FUNCTION PROTOTYPES ********************************************************/

static int ftmod_ss7_parse_sctp_link(ftdm_conf_node_t *node);

/******************************************************************************/
int ftmod_ss7_tucl_shutdown()
{
	Pst pst;
	HiMngmt cntrl;

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(HiMngmt));

	smPstInit(&pst);

	pst.dstEnt = ENTHI;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTHI;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STGEN;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action    = ASHUTDOWN;

	return (sng_cntrl_tucl (&pst, &cntrl));
}

/***********************************************************************************************************************/
int ftmod_ss7_sctp_shutdown()
{
	Pst pst;
	SbMgmt cntrl;  

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(SbMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTSB;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTSB;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STSBGEN;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action = ASHUTDOWN;

	return (sng_cntrl_sctp (&pst, &cntrl));
}

/******************************************************************************/
int ftmod_tucl_gen_config(void)
{
	HiMngmt cfg;
	Pst             pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTHI;

	/* clear the configuration structure */
	memset(&cfg, 0, sizeof(cfg));

	/* fill in the post structure */
	smPstInit(&cfg.t.cfg.s.hiGen.lmPst);
	/*fill in the specific fields of the header */
	cfg.hdr.msgType                 		= TCFG;
	cfg.hdr.entId.ent               		= ENTHI;
	cfg.hdr.entId.inst              		= S_INST;
	cfg.hdr.elmId.elmnt     			= STGEN;

	cfg.t.cfg.s.hiGen.numSaps                       = HI_MAX_SAPS;          		/* number of SAPs */
	cfg.t.cfg.s.hiGen.numCons                       = HI_MAX_NUM_OF_CON;    		/* maximum num of connections */
	cfg.t.cfg.s.hiGen.numFdsPerSet                  = HI_MAX_NUM_OF_FD_PER_SET;     	/* maximum num of fds to use per set */
	cfg.t.cfg.s.hiGen.numFdBins                     = HI_MAX_NUM_OF_FD_HASH_BINS;   	/* for fd hash lists */
	cfg.t.cfg.s.hiGen.numClToAccept                 = HI_MAX_NUM_OF_CLIENT_TO_ACCEPT; 	/* clients to accept simultaneously */
	cfg.t.cfg.s.hiGen.permTsk                       = TRUE;                 		/* schedule as perm task or timer */
	cfg.t.cfg.s.hiGen.schdTmrVal                    = HI_MAX_SCHED_TMR_VALUE;               /* if !permTsk - probably ignored */
	cfg.t.cfg.s.hiGen.selTimeout                    = HI_MAX_SELECT_TIMEOUT_VALUE;          /* select() timeout */

	/* number of raw/UDP messages to read in one iteration */
	cfg.t.cfg.s.hiGen.numRawMsgsToRead              = HI_MAX_RAW_MSG_TO_READ;
	cfg.t.cfg.s.hiGen.numUdpMsgsToRead              = HI_MAX_UDP_MSG_TO_READ;

	/* thresholds for congestion on the memory pool */
	cfg.t.cfg.s.hiGen.poolStrtThr                   = HI_MEM_POOL_START_THRESHOLD;
	cfg.t.cfg.s.hiGen.poolDropThr                   = HI_MEM_POOL_DROP_THRESHOLD;
	cfg.t.cfg.s.hiGen.poolStopThr                   = HI_MEM_POOL_STOP_THRESHOLD;

	cfg.t.cfg.s.hiGen.timeRes                       = SI_PERIOD;        /* time resolution */

#ifdef HI_SPECIFY_GENSOCK_ADDR
	cfg.t.cfg.s.hiGen.ipv4GenSockAddr.address = CM_INET_INADDR_ANY;
	cfg.t.cfg.s.hiGen.ipv4GenSockAddr.port  = 0;                            /* DAVIDY - why 0? */
#ifdef IPV6_SUPPORTED
	cfg.t.cfg.s.hiGen.ipv6GenSockAddr.address = CM_INET_INADDR6_ANY;
	cfg.t.cfg.s.hiGen.ipv4GenSockAddr.port  = 0;
#endif
#endif

	return(sng_cfg_tucl(&pst, &cfg));
}

/****************************************************************************************************/
int ftmod_tucl_sap_config(int id)
{
    HiMngmt cfg;
    Pst     pst;
    HiSapCfg  *pCfg;

    sng_sctp_link_t *k = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[id];

    /* initalize the post structure */
    smPstInit(&pst);

    /* insert the destination Entity */
    pst.dstEnt = ENTHI;

    /* clear the configuration structure */
    memset(&cfg, 0, sizeof(cfg));

    /*fill LM  post structure*/
    cfg.t.cfg.s.hiGen.lmPst.dstProcId   = SFndProcId();
    cfg.t.cfg.s.hiGen.lmPst.dstInst     = S_INST;

    cfg.t.cfg.s.hiGen.lmPst.dstProcId   = SFndProcId();
    cfg.t.cfg.s.hiGen.lmPst.dstEnt      = ENTSM;
    cfg.t.cfg.s.hiGen.lmPst.dstInst     = S_INST;

    cfg.t.cfg.s.hiGen.lmPst.prior       = PRIOR0;
    cfg.t.cfg.s.hiGen.lmPst.route       = RTESPEC;
    cfg.t.cfg.s.hiGen.lmPst.region      = S_REG;
    cfg.t.cfg.s.hiGen.lmPst.pool        = S_POOL;
    cfg.t.cfg.s.hiGen.lmPst.selector    = 0;


    /*fill in the specific fields of the header */
    cfg.hdr.msgType         = TCFG;
    cfg.hdr.entId.ent       = ENTHI;
    cfg.hdr.entId.inst      = 0;
    cfg.hdr.elmId.elmnt     = STTSAP;

    pCfg = &cfg.t.cfg.s.hiSap;

    pCfg->spId 	= k->id ; /* each SCTP link there will be one tucl sap */
    pCfg->uiSel 	= 0x00;  /*loosley coupled */
    pCfg->flcEnb = TRUE;
    pCfg->txqCongStrtLim = HI_SAP_TXN_QUEUE_CONG_START_LIMIT;
    pCfg->txqCongDropLim = HI_SAP_TXN_QUEUE_CONG_DROP_LIMIT;
    pCfg->txqCongStopLim = HI_SAP_TXN_QUEUE_CONG_STOP_LIMIT;
    pCfg->numBins = 10;

    pCfg->uiMemId.region = S_REG;
    pCfg->uiMemId.pool   = S_POOL;
    pCfg->uiPrior        = PRIOR0;
    pCfg->uiRoute        = RTESPEC;

    return(sng_cfg_tucl(&pst, &cfg));
}

/****************************************************************************************************/
int ftmod_sctp_gen_config(void)
{
	SbMgmt  cfg;
	Pst             pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSB;

	/* clear the configuration structure */
	memset(&cfg, 0, sizeof(cfg));

	/* fill in the post structure */
	smPstInit(&cfg.t.cfg.s.genCfg.smPst);
	/*fill in the specific fields of the header */
	cfg.hdr.msgType                                         = TCFG;
	cfg.hdr.entId.ent                                       = ENTSB;
	cfg.hdr.entId.inst                                      = S_INST;
	cfg.hdr.elmId.elmnt                             	= STSBGEN;

#ifdef SB_IPV6_SUPPORTED
	/* U8          ipv6SrvcReqdFlg; */ /* IPV6 service required for sctp */
#endif

	cfg.t.cfg.s.genCfg.serviceType                          = HI_SRVC_RAW_SCTP;             	/* Usr packetized TCP Data */    /* TUCL transport protocol (IP/UDP) */
	cfg.t.cfg.s.genCfg.maxNmbSctSaps                        = SB_MAX_SCT_SAPS;                      /* max no. SCT SAPS */
	cfg.t.cfg.s.genCfg.maxNmbTSaps                          = SB_MAX_T_SAPS;                        /* max no. Transport SAPS */
	cfg.t.cfg.s.genCfg.maxNmbEndp                           = SB_MAX_NUM_OF_ENDPOINTS;              /* max no. endpoints */
	cfg.t.cfg.s.genCfg.maxNmbAssoc                          = SB_MAX_NUM_OF_ASSOC;                  /* max no. associations */
	cfg.t.cfg.s.genCfg.maxNmbDstAddr                        = SB_MAX_NUM_OF_DST_ADDR;               /* max no. dest. addresses */
	cfg.t.cfg.s.genCfg.maxNmbSrcAddr                        = SB_MAX_NUM_OF_SRC_ADDR;               /* max no. src. addresses */
	cfg.t.cfg.s.genCfg.maxNmbTxChunks                       = SB_MAX_NUM_OF_TX_CHUNKS;
	cfg.t.cfg.s.genCfg.maxNmbRxChunks                       = SB_MAX_NUM_OF_RX_CHUNKS;
	cfg.t.cfg.s.genCfg.maxNmbInStrms                        = SB_MAX_INC_STREAMS;
	cfg.t.cfg.s.genCfg.maxNmbOutStrms                       = SB_MAX_OUT_STREAMS;
	cfg.t.cfg.s.genCfg.initARwnd                            = SB_MAX_RWND_SIZE;
	cfg.t.cfg.s.genCfg.mtuInitial                           = SB_MTU_INITIAL;
	cfg.t.cfg.s.genCfg.mtuMinInitial                        = SB_MTU_MIN_INITIAL;
	cfg.t.cfg.s.genCfg.mtuMaxInitial                        = SB_MTU_MAX_INITIAL;
	cfg.t.cfg.s.genCfg.performMtu                           = FALSE;
	cfg.t.cfg.s.genCfg.timeRes                              = 1;
	sprintf((char*)cfg.t.cfg.s.genCfg.hostname, "www.sangoma.com"); /* DAVIDY - Fix this later, probably ignored */
	cfg.t.cfg.s.genCfg.useHstName                           = FALSE;      /* Flag whether hostname is to be used in INIT and INITACK msg */
	cfg.t.cfg.s.genCfg.reConfig.maxInitReTx         = 8;
	cfg.t.cfg.s.genCfg.reConfig.maxAssocReTx        = 10;
	cfg.t.cfg.s.genCfg.reConfig.maxPathReTx         = 10;
	cfg.t.cfg.s.genCfg.reConfig.altAcceptFlg        = TRUE;
	cfg.t.cfg.s.genCfg.reConfig.keyTm                       = 600; /* initial value for MD5 Key expiry timer */
	cfg.t.cfg.s.genCfg.reConfig.alpha                       = 12;
	cfg.t.cfg.s.genCfg.reConfig.beta                        = 25;
#ifdef SB_ECN
	cfg.t.cfg.s.genCfg.reConfig.ecnFlg                      = TRUE;
#endif
#ifdef LSB13
	cfg.t.cfg.s.genCfg.checkSumFlg                      = TRUE; /* TODO - should be a configurable option */
	cfg.t.cfg.s.genCfg.checkSumType                     = SB_DUAL; /* or SB_CRC32 - TODO - should be a configurable option */
#endif

	return(sng_cfg_sctp(&pst, &cfg));
}

/****************************************************************************************************/
int ftmod_cfg_sctp(int opr_mode)
{
	int x = 0;

	x = 1;
	while (x<MAX_SCTP_LINK) {

		if ((g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].id !=0) &&
				(!(g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].flags & SNGSS7_CONFIGURED))) {

			/* check if this sctp is associated with any peer
			 * If not associated then simply ignore */

			if (SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) {
				if (FTDM_FAIL == ftmod_m2ua_is_sctp_link_active(g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].id)) {
					x++;
					continue;
				}
			}

			if (  ftmod_sctp_config(x) == FTDM_FAIL) {
				SS7_CRITICAL("SCTP %d configuration FAILED!\n", x);
				return FTDM_FAIL;
			} else {
				SS7_INFO("SCTP %d configuration DONE!\n", x);
			}
			g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].flags |= SNGSS7_CONFIGURED;
		}
		x++;
	}
	return FTDM_SUCCESS;
}

/****************************************************************************************************/
int ftmod_sctp_config(int id)
{
	if  (FTDM_SUCCESS != ftmod_sctp_tsap_config(id))
		return FTDM_FAIL;

	if  (FTDM_SUCCESS != ftmod_sctp_sap_config(id))
		return FTDM_FAIL;

	/* each sctp link there will be one tucl sap */
	if(ftmod_tucl_sap_config(id)){
		ftdm_log (FTDM_LOG_ERROR ,"TUCL SAP[%d] configuration: NOT OK\n", id);
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"TUCL SAP[%d] configuration: OK\n", id);
	}

	return FTDM_SUCCESS;
}

/****************************************************************************************************/
ftdm_status_t ftmod_sctp_tsap_config(int id)
{
	Pst		pst;
	SbMgmt		cfg;
	SbTSapCfg	*c;

	int 		i = 0;
	int		ret = -1;

	sng_sctp_link_t *k = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[id];

	smPstInit(&pst);
	pst.dstEnt = ENTSB;

	memset(&cfg, 0x0, sizeof(cfg));
	smHdrInit(&cfg.hdr);

	cfg.hdr.msgType			= TCFG;
	cfg.hdr.entId.ent		= ENTSB;
	cfg.hdr.entId.inst		= S_INST;
	cfg.hdr.elmId.elmnt		= STSBTSAP;
	cfg.hdr.elmId.elmntInst1 	= k->id;

	c = &cfg.t.cfg.s.tSapCfg;
	c->swtch			= LSB_SW_RFC_REL0;
	c->suId				= k->id;
	c->sel				= 0;
	c->ent				= ENTHI;
	c->inst				= S_INST;
	c->procId			= g_ftdm_sngss7_data.cfg.procId;
	c->memId.region			= S_REG;
	c->memId.pool			= S_POOL;
	c->prior			= PRIOR1;
	c->route			= RTESPEC;
	c->srcNAddrLst.nmb 		= k->numSrcAddr;
	for (i=0; i <= (k->numSrcAddr-1); i++) {
		c->srcNAddrLst.nAddr[i].type = CM_NETADDR_IPV4;
		c->srcNAddrLst.nAddr[i].u.ipv4NetAddr = k->srcAddrList[i+1];
	}

	c->reConfig.spId		= k->id;
	c->reConfig.maxBndRetry 	= 3;
	c->reConfig.tIntTmr 		= 200;

	ret = sng_cfg_sctp(&pst, &cfg);
	if (0 == ret) {
		SS7_INFO("SCTP TSAP [%d] configuration DONE!\n", id);
		return FTDM_SUCCESS;
	} else {
		SS7_CRITICAL("SCTP TSAP [%d] configuration FAILED!\n", id);
		return FTDM_FAIL;
	}
}

/****************************************************************************************************/
ftdm_status_t ftmod_sctp_sap_config(int id)
{
	Pst		pst;
	SbMgmt		cfg;
	SbSctSapCfg	*c;

	int		ret = -1;
	sng_sctp_link_t *k = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[id];

	smPstInit(&pst);
	pst.dstEnt = ENTSB;

	memset(&cfg, 0x0, sizeof(cfg));
	smHdrInit(&cfg.hdr);

	cfg.hdr.msgType			= TCFG;
	cfg.hdr.entId.ent		= ENTSB;
	cfg.hdr.entId.inst		= S_INST;
	cfg.hdr.elmId.elmnt		= STSBSCTSAP;
	cfg.hdr.elmId.elmntInst1 	= k->id;

	c = &cfg.t.cfg.s.sctSapCfg;
	c->swtch 			= LSB_SW_RFC_REL0;
	c->spId				= k->id;	/* Service Provider SAP Id */
	c->sel				= 0;
	c->memId.region			= S_REG;
	c->memId.pool			= S_POOL;
	c->prior			= PRIOR1;
	c->route			= RTESPEC;

	/* Maximum time to wait before the SCTP layer must send a Selective Acknowledgement (SACK) message. Valid range is 1 -165535. */
	c->reConfig.maxAckDelayTm 	= 200;
	/* Maximum number of messages to receive before the SCTP layer must send a SACK message. Valid range is 1 - 165535. */
	c->reConfig.maxAckDelayDg 	= 2;
	/* Initial value of the retransmission timer (RTO). The SCTP layer retransmits data after waiting for feedback during this time period. Valid range is 1 - 65535. */
	c->reConfig.rtoInitial 		= 3000;
	/* Minimum value used for the RTO. If the computed value of RTO is less than rtoMin, the computed value is rounded up to this value. */
	c->reConfig.rtoMin 		= 1000;
	/* Maxiumum value used for RTO. If the computed value of RTO is greater than rtoMax, the computed value is rounded down to this value. */
	c->reConfig.rtoMax 		= 10000;
	/* Default Freeze timer value = 0x00, set if user configured else set 0.. */
	c->reConfig.freezeTm 		= k->freeze_tmr;
	/* Base cookie lifetime for the cookie in the Initiation Acknowledgement (INIT ACK) message. */
	c->reConfig.cookieLife 		= 60000;
	/* Default heartbeat interval timer. Valid range is 1 - 65535. */
	c->reConfig.intervalTm 		= k->hbeat_interval_tmr;
	/* Maximum burst value. Valid range is 1 - 65535. */
	c->reConfig.maxBurst 		= 4;
	/*Maximum number of heartbeats sent at each retransmission timeout (RTO). Valid range is 1 - 65535. */
	c->reConfig.maxHbBurst 		= 1;
	/*Shutdown guard timer value for graceful shutdowns. */
	c->reConfig.t5SdownGrdTm 	= 15000;
	/*	Action to take when the receiver's number of incoming streams is less than the sender's number of outgoing streams. Valid values are:
		TRUE = Accept incoming stream and continue association.
		FALSE = Abort the association.
	*/
	c->reConfig.negAbrtFlg 		= FALSE;
	/* 	Whether to enable or disable heartbeat by default. Valid values are:
		TRUE = Enable heartbeat.
		FALSE = Disable heartbeat.
	*/
	c->reConfig.hBeatEnable 	= TRUE;
	/* Flow control start threshold. When the number of messages in SCTP message queue reaches this value, flow control starts. */
	c->reConfig.flcUpThr 		= 8;
	/* Flow control stop threshold. When the number of messages in SCTP message queue reaches this value, flow control stops. */
	c->reConfig.flcLowThr 		= 6;

	c->reConfig.handleInitFlg 	= FALSE;

#ifdef LSB13
	c->reConfig.checksumType        = TRUE; /* TODO - should be a configurable option */
#endif

	ret = sng_cfg_sctp(&pst, &cfg);
	if (0 == ret) {
		SS7_INFO("SCTP SAP [%d] configuration DONE with freeze timer as %d and heartbeat timer as %d!\n",
			 id, c->reConfig.freezeTm, c->reConfig.intervalTm);
		return FTDM_SUCCESS;
	} else {
		SS7_CRITICAL("SCTP SAP [%d] configuration FAILED!\n", id);
		return FTDM_FAIL;
	}
}

/**********************************************************************************************/
int ftmod_sctp_tucl_tsap_bind(int id)
{
	Pst pst;
	SbMgmt cntrl;
	sng_sctp_link_t *k = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[id];

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(SbMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTSB;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTSB;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STSBTSAP;       /* General */
	cntrl.hdr.transId     = 1;     /* transaction identifier */

	cntrl.hdr.response.selector    = 0;

	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action = ABND_ENA;
	cntrl.t.cntrl.sapId  = k->id;  /* SCT sap id configured at SCTP layer */

	return (sng_cntrl_sctp (&pst, &cntrl));
}

/***********************************************************************************************************************/
int ftmod_sctp_debug(int action)
{
	Pst pst;
	SbMgmt cntrl;

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(SbMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTSB;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTSB;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STSBGEN;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action = action;
	cntrl.t.cntrl.subAction = SADBG;
	cntrl.t.cntrl.dbgMask   = 0xFFFFFFFF;

	return (sng_cntrl_sctp (&pst, &cntrl));
}

/***********************************************************************************************************************/
int ftmod_tucl_debug(int action)
{
	Pst pst;
	HiMngmt cntrl;

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(HiMngmt));

	smPstInit(&pst);

	pst.dstEnt = ENTHI;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTHI;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STGEN;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action    = action;
	cntrl.t.cntrl.subAction = SADBG;
	cntrl.t.cntrl.ctlType.hiDbg.dbgMask = 0xFFFF;

	return (sng_cntrl_tucl (&pst, &cntrl));
}

/***********************************************************************************************************************/
int ftmod_sctp_ssta_req(int elemt, int id, SbMgmt* cfm)
{
	SbMgmt ssta;
	Pst pst;
	sng_sctp_link_t *k = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[id];

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&ssta, 0, sizeof(SbMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTSB;

	/* prepare header */
	ssta.hdr.msgType     = TSSTA;         /* message type */
	ssta.hdr.entId.ent   = ENTSB;          /* entity */
	ssta.hdr.entId.inst  = 0;              /* instance */
	ssta.hdr.elmId.elmnt = elemt;  		/* STSBGEN */ /* Others are STSBTSAP, STSBSCTSAP, STSBASSOC, STSBDTA, STSBTMR */ 
	ssta.hdr.transId     = 1;     /* transaction identifier */

	ssta.hdr.response.selector    = 0;
	ssta.hdr.response.prior       = PRIOR0;
	ssta.hdr.response.route       = RTESPEC;
	ssta.hdr.response.mem.region  = S_REG;
	ssta.hdr.response.mem.pool    = S_POOL;

	if((ssta.hdr.elmId.elmnt == STSBSCTSAP) || (ssta.hdr.elmId.elmnt == STSBTSAP))
	{
		ssta.t.ssta.sapId = k->id; /* SapId */
	}
        if(ssta.hdr.elmId.elmnt == STSBASSOC)
        {
		/*TODO - how to get assoc Id*/
                ssta.t.ssta.s.assocSta.assocId = 0; /* association id */
        }
	return(sng_sta_sctp(&pst,&ssta,cfm));
}

/***********************************************************************************************************************/
int ftmod_ss7_parse_sctp_links(ftdm_conf_node_t *node)
{
	ftdm_conf_node_t	*node_sctp_link = NULL;

	if (!node)
		return FTDM_FAIL;

	if (strcasecmp(node->name, "sng_sctp_interfaces")) {
		SS7_ERROR("SCTP - We're looking at <%s>...but we're supposed to be looking at <sng_sctp_interfaces>!\n", node->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> configurations\n");
	}

	for (node_sctp_link = node->child; node_sctp_link != NULL; node_sctp_link = node_sctp_link->next) {
		if (ftmod_ss7_parse_sctp_link(node_sctp_link) != FTDM_SUCCESS) {
			SS7_ERROR("SCTP - Failed to parse <node_sctp_link>. \n");
			return FTDM_FAIL;
		}
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_sctp_link(ftdm_conf_node_t *node)
{
	ftdm_conf_parameter_t	*param = NULL;
	int			num_params = 0;
	int 			i=0;

	if (!node){
		SS7_ERROR("SCTP - NULL XML Node pointer \n");
		return FTDM_FAIL;
	}

	param = node->parameters;
	num_params = node->n_parameters;

	sng_sctp_link_t		t_link;
	memset (&t_link, 0, sizeof(sng_sctp_link_t));

	if (strcasecmp(node->name, "sng_sctp_interface")) {
		SS7_ERROR("SCTP - We're looking at <%s>...but we're supposed to be looking at <sng_sctp_interface>!\n", node->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> configurations\n");
	}

	/* default value of heartbeat interval timer */
	t_link.hbeat_interval_tmr = 3000;
	for (i=0; i<num_params; i++, param++) {
		if (!strcasecmp(param->var, "name")) {
			int n_strlen = strlen(param->val);
			strncpy((char*)t_link.name, param->val, (n_strlen>MAX_NAME_LEN)?MAX_NAME_LEN:n_strlen);
			SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> with name = %s\n", param->val);
		} else if (!strcasecmp(param->var, "id")) {
			t_link.id = atoi(param->val);
			SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> with id = %s\n", param->val);
		} else if (!strcasecmp(param->var, "freeze_timer")) {
			t_link.freeze_tmr = atoi(param->val);
			SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> with Freeze Timer = %s\n", param->val);
		} else if (!strcasecmp(param->var, "address")) {
			if (t_link.numSrcAddr < SCT_MAX_NET_ADDRS) {
				t_link.srcAddrList[t_link.numSrcAddr+1] = iptoul (param->val);
				t_link.numSrcAddr++;
				SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> with source IP Address = %s\n", param->val);
			} else {
				SS7_ERROR("SCTP - too many source address configured. dropping %s \n", param->val);
			}
		} else if (!strcasecmp(param->var, "source-port")) {
			t_link.port = atoi(param->val);
			SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> with port = %s\n", param->val);
		} else if (!strcasecmp(param->var, "heartbeat-interval-timer")) {
			t_link.hbeat_interval_tmr = atoi(param->val);
			if ((t_link.hbeat_interval_tmr == 0) || (t_link.hbeat_interval_tmr > 65535)) {
				SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> with invalid heartbeat interval timer = %d\n", t_link.hbeat_interval_tmr);
				t_link.hbeat_interval_tmr = 3000;
			} else {
				SS7_DEBUG("SCTP - Parsing <sng_sctp_interface> with heartbeat interval timer = %d\n", t_link.hbeat_interval_tmr);
			}
		} else {
			SS7_ERROR("SCTP - Found an unknown parameter <%s>. Skipping it.\n", param->var);
		}
	}

	g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[t_link.id].id 		= t_link.id;
	g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[t_link.id].port   	= t_link.port;
	g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[t_link.id].freeze_tmr 	= t_link.freeze_tmr;
	g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[t_link.id].hbeat_interval_tmr = t_link.hbeat_interval_tmr;
	strncpy((char*)g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[t_link.id].name, t_link.name, strlen(t_link.name) );
	g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[t_link.id].numSrcAddr = t_link.numSrcAddr;
	for (i=1; i<=t_link.numSrcAddr; i++) {
		g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[t_link.id].srcAddrList[i] = t_link.srcAddrList[i];
	}

	sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_SCTP_PRESENT);

	return FTDM_SUCCESS;
}
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
