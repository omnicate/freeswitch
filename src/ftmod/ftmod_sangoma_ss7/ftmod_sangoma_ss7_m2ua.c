/*
 * Copyright (c) 2012, Kapil Gupta <kgupta@sangoma.com>
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

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/
/* FUNCTION PROTOTYPES ********************************************************/
static int ftmod_m2ua_gen_config(ftdm_sngss7_operating_modes_e opr_mode);
static int ftmod_m2ua_sctsap_config(int m2ua_cfg_id, int sct_sap_id, int sctp_id);
static int ftmod_m2ua_peer_config(int id, ftdm_sngss7_operating_modes_e opr_mode);
static int ftmod_m2ua_peer_config1(int m2ua_inf_id, int peer_id, ftdm_sngss7_operating_modes_e opr_mode);
static int ftmod_m2ua_cluster_config(int idx);
static int ftmod_m2ua_dlsap_config(int idx);
static int ftmod_nif_gen_config(void);
static int ftmod_nif_dlsap_config(int idx);
static int ftmod_m2ua_sctp_sctsap_bind(int idx);
static int ftmod_open_endpoint(int idx);
static int ftmod_init_sctp_assoc(int peer_id);
static int ftmod_nif_m2ua_dlsap_bind(int id, int action);
static int ftmod_nif_mtp2_dlsap_bind(int id, int action);
static int ftmod_m2ua_debug(int action);
static int ftmod_ss7_m2ua_shutdown(void);
static int ftmod_m2ua_enable_alarm(void);
static int ftmod_ss7_get_nif_id_by_mtp2_id(int mtp2_id);
static int ftmod_m2ua_contrl_request(int id ,int action);


/******************************************************************************/
ftdm_status_t ftmod_ss7_m2ua_init(int opr_mode)
{
	/****************************************************************************************************/
	if (SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) {
		if (sng_isup_init_nif()) {
			ftdm_log (FTDM_LOG_ERROR , "Failed to start NIF\n");
			return FTDM_FAIL;
		} else {
			ftdm_log (FTDM_LOG_INFO ,"Started NIF!\n");
		}
	}
	/****************************************************************************************************/

	if (sng_isup_init_m2ua()) {
		ftdm_log (FTDM_LOG_ERROR ,"Failed to start M2UA\n");
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"Started M2UA!\n");
	}
	/****************************************************************************************************/

	if (sng_isup_init_sctp()) {
		ftdm_log (FTDM_LOG_ERROR ,"Failed to start SCTP\n");
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"Started SCTP!\n");
	}
	/****************************************************************************************************/

	if (sng_isup_init_tucl()) {
		ftdm_log (FTDM_LOG_ERROR ,"Failed to start TUCL\n");
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"Started TUCL!\n");
		sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_TUCL_PRESENT);
	}
	/****************************************************************************************************/

	if (ftmod_tucl_gen_config()) {
		ftdm_log (FTDM_LOG_ERROR ,"TUCL GEN configuration: NOT OK\n");
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"TUCL GEN configuration: OK\n");
	}
	/****************************************************************************************************/
	if (ftmod_sctp_gen_config()) {
		ftdm_log (FTDM_LOG_ERROR ,"SCTP GEN configuration: NOT OK\n");
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"SCTP GEN configuration: OK\n");
	}
	/****************************************************************************************************/
	if (ftmod_m2ua_gen_config(opr_mode)) {
		ftdm_log (FTDM_LOG_ERROR ,"M2UA General configuration: NOT OK\n");
		return FTDM_FAIL;
	}else {
		ftdm_log (FTDM_LOG_INFO ,"M2UA General configuration: OK\n");
	}
	/****************************************************************************************************/
	if (SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) {
		if (ftmod_nif_gen_config()) {
			ftdm_log (FTDM_LOG_ERROR ,"NIF General configuration: NOT OK\n");
			return FTDM_FAIL;
		} else {
			ftdm_log (FTDM_LOG_INFO ,"NIF General configuration: OK\n");
		}
	}
	/****************************************************************************************************/


	return FTDM_SUCCESS;
}

/******************************************************************************/
void ftmod_ss7_m2ua_free()
{
	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_M2UA_PRESENT)) {
		ftmod_ss7_m2ua_shutdown();
		sng_isup_free_m2ua();
	}
	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_SCTP_PRESENT)) {
		ftmod_ss7_sctp_shutdown();
		sng_isup_free_sctp();
	}
	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_TUCL_PRESENT)) {
		ftmod_ss7_tucl_shutdown();
		sng_isup_free_tucl();
	}
}

/******************************************************************************/
static int ftmod_ss7_m2ua_shutdown()
{
	Pst pst;
	MwMgmt cntrl;  

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTMW;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTMW;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STMWGEN;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action = ASHUTDOWN;

	return (sng_cntrl_m2ua (&pst, &cntrl));
}
/***********************************************************************************************************************/
ftdm_status_t ftmod_ss7_m2ua_span_stop(int span_id)
{
    ftdm_sngss7_operating_modes_e opr_mode = SNG_SS7_OPR_MODE_NONE;
    int mtp1_cfg_id=0;
    int mtp2_cfg_id=0;
    int nif_cfg_id=0;
    int m2ua_cfg_id=0;

    /* get the operation type */
    opr_mode = ftmod_ss7_get_operating_mode_by_span_id(span_id);

    if (SNG_SS7_OPR_MODE_M2UA_SG != opr_mode) {

        /* valid only for m2ua sg as of now */

        return FTDM_SUCCESS;
    }

    /* as we dont have information which m2ua links belongs to this span so we need to perform
     * below serios of steps in order to get associated m2ua link */

    /* get MTP1 configuration against this span  */
    mtp1_cfg_id = ftmod_ss7_get_mtp1_id_by_span_id(span_id);
    /* get MTP2 configuration against mtp1_cfg_id   */
    mtp2_cfg_id = ftmod_ss7_get_mtp2_id_by_mtp1_id(mtp1_cfg_id);
    /* get NIF configuration against mtp2_cfg_id   */
    nif_cfg_id = ftmod_ss7_get_nif_id_by_mtp2_id(mtp2_cfg_id);

    /* get M2UA configuration id  */
    m2ua_cfg_id = g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[nif_cfg_id].m2uaLnkNmb;

    if (!m2ua_cfg_id) {
        ftdm_log (FTDM_LOG_ERROR ,"Invalid M2UA ID against span[%d]. mtp1_cfg_id[%d] mtp2_cfg_id[%d] nif_cfg_id[%d] \n", span_id,mtp1_cfg_id, mtp2_cfg_id, nif_cfg_id);
        return FTDM_FAIL;
    }

    ftdm_log (FTDM_LOG_DEBUG ,"Found M2UA ID[%d] against span[%d]. mtp1_cfg_id[%d] mtp2_cfg_id[%d] nif_cfg_id[%d] \n", m2ua_cfg_id,span_id,mtp1_cfg_id, mtp2_cfg_id, nif_cfg_id);

    /* Delete NIF sap operation will internally unbound NIF to MTP2 and M2UA */
    if (ftmod_nif_m2ua_dlsap_bind(nif_cfg_id, ADEL )) {
        ftdm_log (FTDM_LOG_ERROR ,"Control request to delete NIF NSAP[%d] : NOT OK\n", nif_cfg_id);
        return 1;
    } else {
        ftdm_log (FTDM_LOG_ERROR ,"Control request to delete NIF NSAP[%d] : OK\n", nif_cfg_id);
    }


    /* now delete M2UA sap */
    ftmod_m2ua_contrl_request(m2ua_cfg_id, ADEL);

    /* now delete associated MTP1 sap */
    ftmod_ss7_delete_mtp1_link(g_ftdm_sngss7_data.cfg.mtp1Link[mtp1_cfg_id].id);

    /* now delete MTP2 sap */
    ftmod_ss7_delete_mtp2_link(g_ftdm_sngss7_data.cfg.mtp2Link[mtp2_cfg_id].id);

    ftdm_log (FTDM_LOG_DEBUG ," Resetting m2ua[%d] , NIF[%d] configuration\n",m2ua_cfg_id,nif_cfg_id);
    
    /* disable all config flags for this span, so during reconfiguration we can perform
     * configuration against same configuration-id */
    memset(&g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[m2ua_cfg_id],0x00,sizeof(sng_m2ua_cfg_t));
    memset(&g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[nif_cfg_id],0x00,sizeof(sng_nif_cfg_t));
    memset(&g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[m2ua_cfg_id],0x00,sizeof(sng_m2ua_cfg_t));
    g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[m2ua_cfg_id].flags = 0x00;
    g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[nif_cfg_id].flags  = 0x00; 
    g_ftdm_sngss7_data.cfg.mtp2Link[mtp2_cfg_id].flags = 0x00;
    g_ftdm_sngss7_data.cfg.mtp1Link[mtp1_cfg_id].flags = 0x00;

    return FTDM_SUCCESS;
}

/******************************************************************************/

ftdm_status_t ftmod_ss7_m2ua_cfg(int opr_mode)
{
    int x=0;

    if (g_ftdm_sngss7_data.stack_logging_enable) {
        ftmod_ss7_enable_m2ua_sg_logging();
    }

    /* SCTP configuration */
    if (ftmod_cfg_sctp(opr_mode)) {
        ftdm_log (FTDM_LOG_ERROR ,"SCTP Configuration : NOT OK\n");
        return FTDM_FAIL;
    } else {
        ftdm_log (FTDM_LOG_INFO ,"SCTP Configuration : OK\n");
    }

    /****************************************************************************************************/
    /* M2UA SCTP SAP configurations */
    x = 1;
    while (x<MW_MAX_NUM_OF_INTF) {
        if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].id !=0) && 
                (!(g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].flags & SNGSS7_CONFIGURED))) {

            /****************************************************************************************************/
            /* M2UA PEER configurations */

            if (ftmod_m2ua_peer_config(x, opr_mode)) {
                ftdm_log (FTDM_LOG_ERROR ,"M2UA PEER configuration for M2UA INTF[%d] : NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M2UA PEER configuration for M2UA INTF[%d] : OK\n", x);
            }
            /****************************************************************************************************/
            /* M2UA Cluster configurations */

            if (ftmod_m2ua_cluster_config(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"M2UA CLUSTER configuration for M2UA INTF[%d] : NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M2UA CLUSTER configuration for M2UA INTF[%d]: OK\n", x);
            }

            /****************************************************************************************************/

            /* Send the USAP (DLSAP) configuration request for M2UA layer; fill the number
             * of saps required to be configured. Max is 3 */ 
            if (ftmod_m2ua_dlsap_config(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"M2UA DLSAP[%d] configuration: NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M2UA DLSAP[%d] configuration: OK\n", x);
            }

            g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].flags |= SNGSS7_CONFIGURED;
        } /* END - SNGSS7_CONFIGURED */
        x++;
    }/* END - M2UA Interfaces for loop*/
    /****************************************************************************************************/
    /* NIF DLSAP */

    if (SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) {
        x = 1;
        while (x<MW_MAX_NUM_OF_INTF) {
            if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].id !=0) && 
                    (!(g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].flags & SNGSS7_CONFIGURED))) {
                if (ftmod_nif_dlsap_config(x)) {
                    ftdm_log (FTDM_LOG_ERROR ,"NIF DLSAP[%d] configuration: NOT OK\n", x);
                    return FTDM_FAIL;
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"NIF DLSAP[%d] configuration: OK\n", x);
                }
                g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].flags |= SNGSS7_CONFIGURED;
            }
            x++;
        }
        sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_NIF_STARTED);
    }

    /* successfully started all the layers , not SET STARTED FLAGS */
    sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_M2UA_STARTED);
    sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_SCTP_STARTED);
    sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_TUCL_STARTED);


    return 0;
}

/****************************************************************************************************/

/****************************************************************************************************/
ftdm_status_t ftmod_m2ua_is_sctp_link_active(int sctp_id)
{
	int x     = 0x01;
	int y     = 0x01;
	int found = 0x00;
	int peer_id = 0x00;
	int clust_id = 0x00;
	sng_m2ua_cluster_cfg_t* clust = NULL;

	/* sctp link is active means 
	 * its associated with peer and 
	 * peer associated  with cluster 
	 * cluster associated with m2ua link 
	 */

	while (x<MW_MAX_NUM_OF_PEER) {
		if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[x].id !=0) && 
				(sctp_id == g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[x].sctpId)) {
			found = 0x01;
			peer_id = x;
			break;
		}
		x++;
	}
	if ( 0x00 == found ) return FTDM_FAIL;

	/* found active peer , now check if we have active cluster */

	found = 0x00;
	y = 1;
	while (y < MW_MAX_NUM_OF_CLUSTER) {
		clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[y];
		if (0 != clust->id) {
			for (x = 0; x < clust->numOfPeers;x++) {
				if (clust->peerIdLst[x] == peer_id) {
					found = 0x01;
					clust_id = clust->id;
				}

			}
		}
		y++;
	}

	if ( 0x00 == found ) return FTDM_FAIL;

	/* got active cluster , now check m2ua links */

	found = 0x00;
	y = 1;
	while (y<MW_MAX_NUM_OF_INTF) {

		if (g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[y].id !=0) {

			if (g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[y].clusterId == clust_id ) {
				found = 0x01;
				break;
			}
		}
		y++;
	}

	if ( 0x00 == found ) return FTDM_FAIL;

	return FTDM_SUCCESS;
}
/****************************************************************************************************/
/* M2UA - General configuration */
static int ftmod_m2ua_gen_config(ftdm_sngss7_operating_modes_e opr_mode)
{
    Pst    pst; 
    MwMgmt cfg;

    memset((U8 *)&cfg, 0, sizeof(MwMgmt));
    memset((U8 *)&pst, 0, sizeof(Pst));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cfg.hdr.msgType     = TCFG;           /* message type */
    cfg.hdr.entId.ent   = ENTMW;          /* entity */
    cfg.hdr.entId.inst  = 0;              /* instance */
    cfg.hdr.elmId.elmnt = STMWGEN;        /* General */
    cfg.hdr.transId     = 0;     /* transaction identifier */

    cfg.hdr.response.selector    = 0;
    cfg.hdr.response.prior       = PRIOR0;
    cfg.hdr.response.route       = RTESPEC;
    cfg.hdr.response.mem.region  = S_REG;
    cfg.hdr.response.mem.pool    = S_POOL;



    if (SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) {
        cfg.t.cfg.s.genCfg.nodeType          = LMW_TYPE_SGP; /* NodeType ==  SGP or ASP  */
    } else {
        cfg.t.cfg.s.genCfg.nodeType          = LMW_TYPE_ASP; /* NodeType ==  SGP or ASP  */
    }
    cfg.t.cfg.s.genCfg.maxNmbIntf        = MW_MAX_NUM_OF_INTF;
    cfg.t.cfg.s.genCfg.maxNmbCluster     = MW_MAX_NUM_OF_CLUSTER;
    cfg.t.cfg.s.genCfg.maxNmbPeer        = MW_MAX_NUM_OF_PEER;
    cfg.t.cfg.s.genCfg.maxNmbSctSap      = MW_MAX_NUM_OF_SCT_SAPS;
    cfg.t.cfg.s.genCfg.timeRes           = 1;              /* timer resolution */
    cfg.t.cfg.s.genCfg.maxClusterQSize   = MW_MAX_CLUSTER_Q_SIZE;
    cfg.t.cfg.s.genCfg.maxIntfQSize      = MW_MAX_INTF_Q_SIZE;

#ifdef LCMWMILMW  
    cfg.t.cfg.s.genCfg.reConfig.smPst.selector  = 0;     /* selector */
#else /* LCSBMILSB */
    cfg.t.cfg.s.genCfg.reConfig.smPst.selector  = 1;     /* selector */
#endif /* LCSBMILSB */

    cfg.t.cfg.s.genCfg.reConfig.smPst.region    = S_REG;   /* region */
    cfg.t.cfg.s.genCfg.reConfig.smPst.pool      = S_POOL;     /* pool */
    cfg.t.cfg.s.genCfg.reConfig.smPst.prior     = PRIOR0;        /* priority */
    cfg.t.cfg.s.genCfg.reConfig.smPst.route     = RTESPEC;       /* route */

    cfg.t.cfg.s.genCfg.reConfig.smPst.dstEnt    = ENTSM;         /* dst entity */
    cfg.t.cfg.s.genCfg.reConfig.smPst.dstInst   = S_INST;             /* dst inst */
    cfg.t.cfg.s.genCfg.reConfig.smPst.dstProcId = SFndProcId();  /* src proc id */

    cfg.t.cfg.s.genCfg.reConfig.smPst.srcEnt    = ENTMW;         /* src entity */
    cfg.t.cfg.s.genCfg.reConfig.smPst.srcInst   = S_INST;             /* src inst */
    cfg.t.cfg.s.genCfg.reConfig.smPst.srcProcId = SFndProcId();  /* src proc id */

    cfg.t.cfg.s.genCfg.reConfig.tmrFlcPoll.enb = TRUE;            /* SCTP Flc Poll timer */
    cfg.t.cfg.s.genCfg.reConfig.tmrFlcPoll.val = 10;


    if (SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode) {
        cfg.t.cfg.s.genCfg.reConfig.tmrAspm.enb    = TRUE;         /* ASPM  timer */
        cfg.t.cfg.s.genCfg.reConfig.tmrAspm.val    = 10;
        cfg.t.cfg.s.genCfg.reConfig.tmrHeartBeat.enb  = TRUE;       /* Heartbeat timer */
        cfg.t.cfg.s.genCfg.reConfig.tmrHeartBeat.val  = 10;
    }



    if(SNG_SS7_OPR_MODE_M2UA_SG == opr_mode){
        cfg.t.cfg.s.genCfg.reConfig.tmrAsPend.enb  = TRUE;   /* AS-PENDING timer */
        cfg.t.cfg.s.genCfg.reConfig.tmrAsPend.val  = 10;
        cfg.t.cfg.s.genCfg.reConfig.tmrCongPoll.enb = TRUE;  /* SS7 Congestion poll timer */
        cfg.t.cfg.s.genCfg.reConfig.tmrCongPoll.val = 10;
        cfg.t.cfg.s.genCfg.reConfig.tmrHeartBeat.enb  = FALSE;       /* HBtimer only at ASP */
    }

    cfg.t.cfg.s.genCfg.reConfig.aspmRetry = 5;

    return (sng_cfg_m2ua (&pst, &cfg));
}   

/**********************************************************************************************/
static int ftmod_m2ua_peer_config(int id, ftdm_sngss7_operating_modes_e opr_mode)
{
    int x = 0;
    int peer_id = 0;
    sng_m2ua_cfg_t* 	    m2ua  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[id];
    sng_m2ua_cluster_cfg_t*     clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[m2ua->clusterId];
    sng_m2ua_peer_cfg_t* 	    peer  = NULL;

    if ((clust->flags & SNGSS7_CONFIGURED)) {
        ftdm_log (FTDM_LOG_INFO, " ftmod_m2ua_peer_config: Cluster [%s] is already configured \n", clust->name);
        return 0x00;
    }

    /*NOTE : SCTSAP is based on per source address , so if we have same Cluster / peer shared across many <m2ua_interface> then 
     * we dont have do configuration for each time */

    /* loop through peer list from cluster to configure SCTSAP */

    for (x = 0; x < clust->numOfPeers;x++) {
        peer_id = clust->peerIdLst[x];
        peer = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[peer_id];
        if (ftmod_m2ua_sctsap_config(id, peer->sctpId, peer->sctpId)) {
            ftdm_log (FTDM_LOG_ERROR, " ftmod_m2ua_sctsap_config: M2UA SCTSAP for M2UA Intf Id[%d] config FAILED \n", id);
            return 0x01;
        } else {
            ftdm_log (FTDM_LOG_INFO, " ftmod_m2ua_sctsap_config: M2UA SCTSAP for M2UA Intf Id[%d] config SUCCESS \n", id);
        }
        if (ftmod_m2ua_peer_config1(peer->sctpId, peer_id, opr_mode)) {
            ftdm_log (FTDM_LOG_ERROR, " ftmod_m2ua_peer_config1: M2UA Peer[%d] configuration for M2UA Intf Id[%d] config FAILED \n", peer_id, id);
            return 0x01;
        } else {
            ftdm_log (FTDM_LOG_INFO, " ftmod_m2ua_peer_config1: M2UA Peer[%d] configuration for M2UA Intf Id[%d] config SUCCESS \n", peer_id, id);
        }

        clust->sct_sap_id = peer->sctpId;

        /* set configured flag for cluster and peer */
        clust->flags |= SNGSS7_CONFIGURED;
        peer->flags |= SNGSS7_CONFIGURED;
    }

    return 0x0;;
}


static int ftmod_m2ua_sctsap_config(int m2ua_cfg_id,int sct_sap_id, int sctp_id)
{
    int    i;
    int    ret;
    Pst    pst; 
    MwMgmt cfg;
    MwMgmt cfm;
    sng_sctp_link_t *sctp = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[sctp_id];



    memset((U8 *)&cfg, 0, sizeof(MwMgmt));
    memset((U8 *)&cfm, 0, sizeof(MwMgmt));
    memset((U8 *)&pst, 0, sizeof(Pst));

    /* check is sct_sap is already configured */
    /* ftmod_m2ua_ssta_req needs m2ua config id to fetch STMWSCTSAP status */
    if (!ftmod_m2ua_ssta_req(STMWSCTSAP, m2ua_cfg_id, &cfm )) {
        ftdm_log (FTDM_LOG_INFO, " ftmod_m2ua_sctsap_config: SCT SAP [%s] for m2ua_cfg_id[%d] sct_sap_id[%d] is already configured \n", sctp->name,m2ua_cfg_id, sct_sap_id);
        return 0x00;
    }

    if (LCM_REASON_INVALID_SAP == cfm.cfm.reason) {
        ftdm_log (FTDM_LOG_INFO, " ftmod_m2ua_sctsap_config: SCT SAP [%s] is not configured..configuring now \n", sctp->name);
    }

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cfg.hdr.msgType     = TCFG;           /* message type */
    cfg.hdr.entId.ent   = ENTMW;          /* entity */
    cfg.hdr.entId.inst  = 0;              /* instance */
    cfg.hdr.elmId.elmnt = STMWSCTSAP;     /* SCTSAP */
    cfg.hdr.transId     = 0;     /* transaction identifier */

    cfg.hdr.response.selector    = 0;
    cfg.hdr.response.prior       = PRIOR0;
    cfg.hdr.response.route       = RTESPEC;
    cfg.hdr.response.mem.region  = S_REG;
    cfg.hdr.response.mem.pool    = S_POOL;

    cfg.t.cfg.s.sctSapCfg.reConfig.selector     = 0;

    /* service user SAP ID */
    cfg.t.cfg.s.sctSapCfg.suId                   = sct_sap_id;
    /* service provider ID   */
    cfg.t.cfg.s.sctSapCfg.spId                   = sctp_id;
    /* source port number */
    cfg.t.cfg.s.sctSapCfg.srcPort                = sctp->port;
    /* interface address */
    /*For multiple IP address support */
#ifdef SCT_ENDP_MULTI_IPADDR
    cfg.t.cfg.s.sctSapCfg.srcAddrLst.nmb  	= sctp->numSrcAddr;
    for (i=0; i <= (sctp->numSrcAddr-1); i++) {
        cfg.t.cfg.s.sctSapCfg.srcAddrLst.nAddr[i].type = CM_NETADDR_IPV4;
        cfg.t.cfg.s.sctSapCfg.srcAddrLst.nAddr[i].u.ipv4NetAddr = sctp->srcAddrList[i+1];
    }
#else
    /* for single ip support ,src address will always be one */
    cfg.t.cfg.s.sctSapCfg.intfAddr.type          = CM_NETADDR_IPV4;
    cfg.t.cfg.s.sctSapCfg.intfAddr.u.ipv4NetAddr = sctp->srcAddrList[1];
#endif

    /* lower SAP primitive timer */
    cfg.t.cfg.s.sctSapCfg.reConfig.tmrPrim.enb   = TRUE;
    cfg.t.cfg.s.sctSapCfg.reConfig.tmrPrim.val   = 10;
    /* Association primitive timer */
    cfg.t.cfg.s.sctSapCfg.reConfig.tmrAssoc.enb   = TRUE;
    cfg.t.cfg.s.sctSapCfg.reConfig.tmrAssoc.val   = 10;
    /* maxnumber of retries */
    cfg.t.cfg.s.sctSapCfg.reConfig.nmbMaxPrimRetry  = 5;
    /* Life Time of Packets  */
    cfg.t.cfg.s.sctSapCfg.reConfig.lifeTime  = 200;
    /* priority */
    cfg.t.cfg.s.sctSapCfg.reConfig.prior       =  PRIOR0;
    /* route */
    cfg.t.cfg.s.sctSapCfg.reConfig.route       =  RTESPEC;
    cfg.t.cfg.s.sctSapCfg.reConfig.ent         =  ENTSB;
    cfg.t.cfg.s.sctSapCfg.reConfig.inst        =  0;
    cfg.t.cfg.s.sctSapCfg.reConfig.procId      =  SFndProcId();
    /* memory region and pool ID */
    cfg.t.cfg.s.sctSapCfg.reConfig.mem.region    = S_REG;
    cfg.t.cfg.s.sctSapCfg.reConfig.mem.pool      = S_POOL;

    if (0 == (ret = sng_cfg_m2ua (&pst, &cfg))){
        sctp->flags |= SNGSS7_CONFIGURED;
    }

    return ret;
}

/****************************************************************************************************/

/* M2UA - Peer configuration */
static int ftmod_m2ua_peer_config1(int m2ua_inf_id, int peer_id, ftdm_sngss7_operating_modes_e opr_mode)
{
    int    i;
    Pst    pst;
    MwMgmt cfg;
    sng_m2ua_peer_cfg_t* peer  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[peer_id];
    sng_sctp_link_t 	*sctp = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[peer->sctpId];

    memset((U8 *)&cfg, 0, sizeof(MwMgmt));
    memset((U8 *)&pst, 0, sizeof(Pst));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cfg.hdr.msgType     = TCFG;           /* message type */
    cfg.hdr.entId.ent   = ENTMW;          /* entity */
    cfg.hdr.entId.inst  = 0;              /* instance */
    cfg.hdr.elmId.elmnt = STMWPEER;       /* Peer */
    cfg.hdr.transId     = 0;     /* transaction identifier */

    cfg.hdr.response.selector    = 0;
    cfg.hdr.response.prior       = PRIOR0;
    cfg.hdr.response.route       = RTESPEC;
    cfg.hdr.response.mem.region  = S_REG;
    cfg.hdr.response.mem.pool    = S_POOL;



    cfg.t.cfg.s.peerCfg.peerId 		= peer->id;               /* peer id */
    cfg.t.cfg.s.peerCfg.aspIdFlag 	= peer->aspIdFlag;        /* aspId flag */

    if (SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode) {
        cfg.t.cfg.s.peerCfg.selfAspId 	= peer->selfAspId;  	  /* aspId */
    }

    cfg.t.cfg.s.peerCfg.assocCfg.suId    = peer->sctpId; 	  /* SCTSAP ID */
        ftdm_log (FTDM_LOG_INFO, " ftmod_m2ua_peer_config1: peerId [%d] with sctsap-id[%d] \n", peer->id,peer->sctpId);
    cfg.t.cfg.s.peerCfg.assocCfg.dstAddrLst.nmb = peer->numDestAddr;
    for (i=0; i <= (peer->numDestAddr); i++) {
        cfg.t.cfg.s.peerCfg.assocCfg.dstAddrLst.nAddr[i].type = CM_NETADDR_IPV4;
        cfg.t.cfg.s.peerCfg.assocCfg.dstAddrLst.nAddr[i].u.ipv4NetAddr = peer->destAddrList[i]; 
    }
#ifdef MW_CFG_DSTPORT
    cfg.t.cfg.s.peerCfg.assocCfg.dstPort = peer->port; /* Port on which M2UA runs */
#endif
    cfg.t.cfg.s.peerCfg.assocCfg.srcAddrLst.nmb = sctp->numSrcAddr; /* source address list */
    for (i=0; i <= (sctp->numSrcAddr-1); i++) {
        cfg.t.cfg.s.peerCfg.assocCfg.srcAddrLst.nAddr[i].type =  CM_NETADDR_IPV4;
        cfg.t.cfg.s.peerCfg.assocCfg.srcAddrLst.nAddr[i].u.ipv4NetAddr = sctp->srcAddrList[i+1]; 
    }

    cfg.t.cfg.s.peerCfg.assocCfg.priDstAddr.type = CM_NETADDR_IPV4;
    cfg.t.cfg.s.peerCfg.assocCfg.priDstAddr.u.ipv4NetAddr = cfg.t.cfg.s.peerCfg.assocCfg.dstAddrLst.nAddr[0].u.ipv4NetAddr;

    cfg.t.cfg.s.peerCfg.assocCfg.locOutStrms = peer->locOutStrms;
#ifdef SCT3
    cfg.t.cfg.s.peerCfg.assocCfg.tos = 0;
#endif

    return (sng_cfg_m2ua (&pst, &cfg));
}
/**********************************************************************************************/
/* M2UA - Cluster configuration */
static int ftmod_m2ua_cluster_config(int id)
{
    int i   = 0x00 ;
    int ret = 0x00;
    Pst    pst; 
    MwMgmt cfg;
    sng_m2ua_cfg_t* 	    m2ua  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[id];
    sng_m2ua_cluster_cfg_t*  clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[m2ua->clusterId];

    /* We only have to configure cluster one time, 
     * so that during run-time any other m2ua link can attach with existing active cluster.
     * */

    if ((clust->flags & SNGSS7_ACTIVE)) {
        ftdm_log (FTDM_LOG_INFO, " ftmod_m2ua_cluster_config: Cluster [%s] is already active \n", clust->name);
        return 0x00;
    }

    memset((U8 *)&cfg, 0, sizeof(MwMgmt));
    memset((U8 *)&pst, 0, sizeof(Pst));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cfg.hdr.msgType     = TCFG;           /* message type */
    cfg.hdr.entId.ent   = ENTMW;          /* entity */
    cfg.hdr.entId.inst  = 0;              /* instance */
    cfg.hdr.elmId.elmnt = STMWCLUSTER;    /* Cluster */
    cfg.hdr.transId     = 0;     /* transaction identifier */

    cfg.hdr.response.selector    = 0;
    cfg.hdr.response.prior       = PRIOR0;
    cfg.hdr.response.route       = RTESPEC;
    cfg.hdr.response.mem.region  = S_REG;
    cfg.hdr.response.mem.pool    = S_POOL;


    cfg.t.cfg.s.clusterCfg.clusterId 	= clust->id;
    cfg.t.cfg.s.clusterCfg.trfMode   	= clust->trfMode;
    cfg.t.cfg.s.clusterCfg.loadshareMode = clust->loadShareAlgo;
    cfg.t.cfg.s.clusterCfg.reConfig.nmbPeer = clust->numOfPeers;
    for (i=0; i<(clust->numOfPeers);i++) {
        cfg.t.cfg.s.clusterCfg.reConfig.peer[i] = clust->peerIdLst[i];
    }

    if (0 == (ret = sng_cfg_m2ua (&pst, &cfg))) {
        clust->flags |= SNGSS7_CONFIGURED;
    }

    return ret;
}

/**********************************************************************************************/

/* M2UA - DLSAP configuration */
static int ftmod_m2ua_dlsap_config(int id)
{
   Pst    pst; 
   MwMgmt cfg;
   sng_m2ua_cfg_t* 	    m2ua  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[id];

   memset((U8 *)&cfg, 0, sizeof(MwMgmt));
   memset((U8 *)&pst, 0, sizeof(Pst));

   smPstInit(&pst);

   pst.dstEnt = ENTMW;

   /* prepare header */
   cfg.hdr.msgType     = TCFG;           /* message type */
   cfg.hdr.entId.ent   = ENTMW;          /* entity */
   cfg.hdr.entId.inst  = 0;              /* instance */
   cfg.hdr.elmId.elmnt = STMWDLSAP;      /* DLSAP */
   cfg.hdr.transId     = 0;     /* transaction identifier */

   cfg.hdr.response.selector    = 0;
   cfg.hdr.response.prior       = PRIOR0;
   cfg.hdr.response.route       = RTESPEC;
   cfg.hdr.response.mem.region  = S_REG;
   cfg.hdr.response.mem.pool    = S_POOL;


   cfg.t.cfg.s.dlSapCfg.lnkNmb 	= id; /* SapId */
   cfg.t.cfg.s.dlSapCfg.intfId.type = LMW_INTFID_INT;
   cfg.t.cfg.s.dlSapCfg.intfId.id.intId = m2ua->iid;
   
   cfg.t.cfg.s.dlSapCfg.swtch = LMW_SAP_ITU;

   cfg.t.cfg.s.dlSapCfg.reConfig.clusterId =  m2ua->clusterId;
   cfg.t.cfg.s.dlSapCfg.reConfig.selector  =  0; /* Loosely couple mode */
   /* memory region and pool id*/
   cfg.t.cfg.s.dlSapCfg.reConfig.mem.region  =  S_REG;
   cfg.t.cfg.s.dlSapCfg.reConfig.mem.pool    =  S_POOL;
   /* priority */
   cfg.t.cfg.s.dlSapCfg.reConfig.prior       =  PRIOR0;
   /* route */
   cfg.t.cfg.s.dlSapCfg.reConfig.route       =  RTESPEC;

     return (sng_cfg_m2ua (&pst, &cfg));

}
/*****************************************************************************/
/* NIF - General configuration */
static int ftmod_nif_gen_config(void)
{
   Pst    pst; 
   NwMgmt cfg;

   memset((U8 *)&cfg, 0, sizeof(NwMgmt));
   memset((U8 *)&pst, 0, sizeof(Pst));

   smPstInit(&pst);

   pst.dstEnt = ENTNW;

   /* prepare header */
   cfg.hdr.msgType     = TCFG;           /* message type */
   cfg.hdr.entId.ent   = ENTNW;          /* entity */
   cfg.hdr.entId.inst  = 0;              /* instance */
   cfg.hdr.elmId.elmnt = STNWGEN;      /* DLSAP */
   cfg.hdr.transId     = 0;     /* transaction identifier */

   cfg.hdr.response.selector    = 0;
   cfg.hdr.response.prior       = PRIOR0;
   cfg.hdr.response.route       = RTESPEC;
   cfg.hdr.response.mem.region  = S_REG;
   cfg.hdr.response.mem.pool    = S_POOL;

   cfg.t.cfg.s.genCfg.maxNmbDlSap       = NW_MAX_NUM_OF_DLSAPS;
   cfg.t.cfg.s.genCfg.timeRes           = 1;    /* timer resolution */

   cfg.t.cfg.s.genCfg.reConfig.maxNmbRetry    = NW_MAX_NUM_OF_RETRY;
   cfg.t.cfg.s.genCfg.reConfig.tmrRetry.enb =   TRUE;     /* SS7 Congestion poll timer */
   cfg.t.cfg.s.genCfg.reConfig.tmrRetry.val =   NW_RETRY_TMR_VALUE;

#ifdef LCNWMILNW  
   cfg.t.cfg.s.genCfg.reConfig.smPst.selector  = 0;     /* selector */
#else /* LCSBMILSB */
   cfg.t.cfg.s.genCfg.reConfig.smPst.selector  = 1;     /* selector */
#endif /* LCSBMILSB */

   cfg.t.cfg.s.genCfg.reConfig.smPst.region    = S_REG;   /* region */
   cfg.t.cfg.s.genCfg.reConfig.smPst.pool      = S_POOL;     /* pool */
   cfg.t.cfg.s.genCfg.reConfig.smPst.prior     = PRIOR0;        /* priority */
   cfg.t.cfg.s.genCfg.reConfig.smPst.route     = RTESPEC;       /* route */

   cfg.t.cfg.s.genCfg.reConfig.smPst.dstEnt    = ENTSM;         /* dst entity */
   cfg.t.cfg.s.genCfg.reConfig.smPst.dstInst   = 0;             /* dst inst */
   cfg.t.cfg.s.genCfg.reConfig.smPst.dstProcId = SFndProcId();  /* src proc id */

   cfg.t.cfg.s.genCfg.reConfig.smPst.srcEnt    = ENTNW;         /* src entity */
   cfg.t.cfg.s.genCfg.reConfig.smPst.srcInst   = 0;             /* src inst */
   cfg.t.cfg.s.genCfg.reConfig.smPst.srcProcId = SFndProcId();  /* src proc id */

     return (sng_cfg_nif (&pst, &cfg));

}

/*****************************************************************************/

/* NIF - DLSAP configuration */
static int ftmod_nif_dlsap_config(int id)
{
   Pst    pst; 
   NwMgmt cfg;
   sng_nif_cfg_t* nif = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[id];

   memset((U8 *)&cfg, 0, sizeof(NwMgmt));
   memset((U8 *)&pst, 0, sizeof(Pst));

   smPstInit(&pst);

   pst.dstEnt = ENTNW;

   /* prepare header */
   cfg.hdr.msgType     = TCFG;           /* message type */
   cfg.hdr.entId.ent   = ENTNW;          /* entity */
   cfg.hdr.entId.inst  = 0;              /* instance */
   cfg.hdr.elmId.elmnt = STNWDLSAP;      /* DLSAP */
   cfg.hdr.transId     = 0;     /* transaction identifier */

   cfg.hdr.response.selector    = 0;
   cfg.hdr.response.prior       = PRIOR0;
   cfg.hdr.response.route       = RTESPEC;
   cfg.hdr.response.mem.region  = S_REG;
   cfg.hdr.response.mem.pool    = S_POOL;
   cfg.t.cfg.s.dlSapCfg.suId 	    = nif->id;           
   cfg.t.cfg.s.dlSapCfg.m2uaLnkNmb  = nif->m2uaLnkNmb; 
   cfg.t.cfg.s.dlSapCfg.mtp2LnkNmb  = nif->mtp2LnkNmb;
        
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.selector   = 0;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.region     = S_REG;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.pool       = S_POOL;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.route      = RTESPEC;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.prior      = PRIOR0;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.srcEnt     = ENTNW;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.srcInst    = 0;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.srcProcId  = SFndProcId();
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.dstEnt     = ENTMW;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.dstInst    = 0;
   cfg.t.cfg.s.dlSapCfg.reConfig.m2uaPst.dstProcId  = SFndProcId();

   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.selector   = 0;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.region     = S_REG;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.pool       = S_POOL;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.route      = RTESPEC;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.prior      = PRIOR0;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.srcEnt     = ENTNW;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.srcInst    = 0;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.srcProcId  = SFndProcId();
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.dstEnt     = ENTSD;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.dstInst    = 0;
   cfg.t.cfg.s.dlSapCfg.reConfig.mtp2Pst.dstProcId  = SFndProcId();

     return (sng_cfg_nif (&pst, &cfg));
}   

/*****************************************************************************/
uint32_t iptoul(const char *ip)
{
    char i,*tmp;
    int strl;
    char strIp[16];
    unsigned long val=0, cvt;
    if (!ip)
        return 0;

    memset(strIp, 0, sizeof(char)*16);
    strl = strlen(ip);
    strncpy(strIp, ip, strl>=15?15:strl);


    tmp=strtok(strIp, ".");
    for (i=0;i<4;i++)
    {
        sscanf(tmp, "%lu", &cvt);
        val <<= 8;
        val |= (unsigned char)cvt;
        tmp=strtok(NULL,".");
    }
    return (uint32_t)val;
}
/***********************************************************************************************************************/
void ftmod_ss7_enable_m2ua_sg_logging(void){

	/* Enable DEBUGs*/
	ftmod_sctp_debug(AENA);
	ftmod_m2ua_debug(AENA);
	ftmod_tucl_debug(AENA);
}

/***********************************************************************************************************************/
void ftmod_ss7_disable_m2ua_sg_logging(void){

	/* DISABLE DEBUGs*/
	ftmod_sctp_debug(ADISIMM);
	ftmod_m2ua_debug(ADISIMM);
	ftmod_tucl_debug(ADISIMM);

    g_ftdm_sngss7_data.stack_logging_enable = 0x00;
}

/***********************************************************************************************************************/
int ftmod_ss7_m2ua_start(int opr_mode) {
    int x = 0;

    /***********************************************************************************************************************/
    x = 1;
    while (x<MAX_SCTP_LINK) {
        if ((g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].id !=0) && 
                (!(g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].flags & SNGSS7_ACTIVE))) {

            if (FTDM_FAIL == ftmod_m2ua_is_sctp_link_active(g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].id)) {
                x++; 
                continue; 
            }

            /* Send a control request to bind the TSAP between SCTP and TUCL */
            if (ftmod_sctp_tucl_tsap_bind(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"\nControl request to bind TSAP[%d] of SCTP and TUCL : NOT OK\n", x);
                return 1;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"\nControl request to bind TSAP[%d] of SCTP and TUCL: OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].flags |= SNGSS7_ACTIVE;
        }
        x++;
    }

    /***********************************************************************************************************************/
    /* Send a control request to bind the SCTSAP between SCTP and M2UA */
    x = 1;
    while (x<MW_MAX_NUM_OF_INTF) {
        if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].id !=0) && 
                (!(g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].flags & SNGSS7_ACTIVE))) {
            if (ftmod_m2ua_sctp_sctsap_bind(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"Control request to bind SCTSAP[%d] of M2UA and SCTP : NOT OK\n", x);
                return 1;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"Control request to bind SCTSAP[%d] of M2UA and SCTP: OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].flags |= SNGSS7_ACTIVE;
        }
        x++;
    }/* END - M2UA Interfaces while loop*/
    /***********************************************************************************************************************/

    if (SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) {
        x = 1;
        while (x<MW_MAX_NUM_OF_INTF) {
            if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].id !=0) && 
                    (!(g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].flags & SNGSS7_ACTIVE))) {
                /* Send a control request to bind the DLSAP between NIF, M2UA and MTP-2 */
                if (ftmod_nif_m2ua_dlsap_bind(x, ABND )) {
                    ftdm_log (FTDM_LOG_ERROR ,"Control request to bind DLSAP[%d] between NIF and M2UA: NOT OK\n", x);
                    return 1;
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"Control request to bind DLSAP[%d] between NIF and M2UA : OK\n", x);
                }
                if (ftmod_nif_mtp2_dlsap_bind(x, ABND)) {
                    ftdm_log (FTDM_LOG_ERROR ,"Control request to bind DLSAP[%d] between NIF and MTP2: NOT OK\n", x);
                    return 1;
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"Control request to bind DLSAP[%d] between NIF and MTP2 : OK\n", x);
                }
                g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].flags |= SNGSS7_ACTIVE;
            }
            x++;
        }/* END - NIF Interfaces for loop*/
    }

    /***********************************************************************************************************************/

    x = 1;
    while (x<MW_MAX_NUM_OF_INTF) {
        if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].id !=0) && 
                (!(g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].end_point_opened))) {
            /* Send a control request to open endpoint */
            if (ftmod_open_endpoint(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"ftmod_open_endpoint FAIL  \n");
                return 1;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"ftmod_open_endpoint SUCCESS  \n");
            }
            g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[x].end_point_opened = 0x01;
        }
        x++;
    }

    /***********************************************************************************************************************/
    sleep(2);

    x = 1;
    while (x < (MW_MAX_NUM_OF_PEER)) {
        if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[x].id !=0) &&
                (g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[x].flags & SNGSS7_CONFIGURED ) &&
                (!(g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[x].flags & SNGSS7_M2UA_INIT_ASSOC_DONE)) && 
                (g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[x].init_sctp_assoc)) {
            if (ftmod_init_sctp_assoc(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"ftmod_init_sctp_assoc FAIL for peerId[%d] \n", x);
                return 1;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"ftmod_init_sctp_assoc SUCCESS for peerId[%d] \n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[x].flags |= SNGSS7_M2UA_INIT_ASSOC_DONE;
        }
        x++;
    }


    if (SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode) {
        /* enable M2UA Alarms as of now for ASP mode only...
         * ideally should be enable for both..will do it in nsg-4.4 */
        ftmod_m2ua_enable_alarm();
    }

    return 0;
}
/***********************************************************************************************************************/
static int ftmod_m2ua_contrl_request(int id, int action)
{
    int ret = 0x00;
    Pst pst;
    MwMgmt cntrl;  
    //sng_m2ua_cfg_t* m2ua  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[id];
    //sng_m2ua_cluster_cfg_t*     clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[m2ua->clusterId];

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cntrl.hdr.msgType     = TCNTRL;         /* message type */
    cntrl.hdr.entId.ent   = ENTMW;          /* entity */
    cntrl.hdr.entId.inst  = 0;              /* instance */
    cntrl.hdr.elmId.elmnt = STMWDLSAP;       /* General */
    cntrl.hdr.transId     = 1;     /* transaction identifier */

    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;


    cntrl.t.cntrl.action = action;
    cntrl.t.cntrl.s.suId = id; 


    if (0 != (ret = sng_cntrl_m2ua (&pst, &cntrl))) {
        ftdm_log (FTDM_LOG_ERROR ,"Failure in M2UA action[%s] \n", (action==ADEL)?"ADEL":"AUBND");
    } else {
        ftdm_log (FTDM_LOG_ERROR ," M2UA action[%s] succes \n", (action==ADEL)?"ADEL":"AUBND");
    }
    return ret;
}
/***********************************************************************************************************************/

static int ftmod_open_endpoint(int id)
{
    int ret = 0x00;
    Pst pst;
    MwMgmt cntrl;  
    sng_m2ua_cfg_t* m2ua  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[id];
    sng_m2ua_cluster_cfg_t*     clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[m2ua->clusterId];

    if (clust->flags & SNGSS7_M2UA_EP_OPENED) {
        ftdm_log (FTDM_LOG_INFO ," END-POINT already opened\n");
        return ret;
    }

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cntrl.hdr.msgType     = TCNTRL;         /* message type */
    cntrl.hdr.entId.ent   = ENTMW;          /* entity */
    cntrl.hdr.entId.inst  = 0;              /* instance */
    cntrl.hdr.elmId.elmnt = STMWSCTSAP;       /* General */
    cntrl.hdr.transId     = 1;     /* transaction identifier */

    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;


    cntrl.t.cntrl.action = AMWENDPOPEN;
    cntrl.t.cntrl.s.suId = clust->sct_sap_id; /* M2UA sct sap Id */


    if (0 == (ret = sng_cntrl_m2ua (&pst, &cntrl))) {
        clust->flags |= SNGSS7_M2UA_EP_OPENED;
    }

    return ret;
}

/***********************************************************************************************************************/
static int ftmod_init_sctp_assoc(int peer_id)
{

    Pst pst;
    MwMgmt cntrl;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cntrl.hdr.msgType     = TCNTRL;         /* message type */
    cntrl.hdr.entId.ent   = ENTMW;          /* entity */
    cntrl.hdr.entId.inst  = 0;              /* instance */
    cntrl.hdr.elmId.elmnt = STMWPEER;       /* General */
    cntrl.hdr.transId     = 1;     /* transaction identifier */

    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;


    cntrl.t.cntrl.action = AMWESTABLISH;
    /*cntrl.t.cntrl.s.suId = 1;*/

    cntrl.t.cntrl.s.peerId = (MwPeerId) peer_id;

    return (sng_cntrl_m2ua (&pst, &cntrl));
}

/***********************************************************************************************************************/
static int ftmod_m2ua_sctp_sctsap_bind(int id)
{
    int ret = 0x00;
    Pst pst;
    MwMgmt cntrl;  
    sng_m2ua_cfg_t* m2ua  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[id];
    sng_m2ua_cluster_cfg_t*     clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[m2ua->clusterId];

    if (clust->flags & SNGSS7_ACTIVE) {
        ftdm_log (FTDM_LOG_INFO ," SCT-SAP is already enabled\n");
        return ret;
    }


    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cntrl.hdr.msgType     = TCNTRL;         /* message type */
    cntrl.hdr.entId.ent   = ENTMW;          /* entity */
    cntrl.hdr.entId.inst  = 0;              /* instance */
    cntrl.hdr.elmId.elmnt = STMWSCTSAP;       /* General */
    cntrl.hdr.transId     = 1;     /* transaction identifier */

    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action = ABND;
    cntrl.t.cntrl.s.suId = clust->sct_sap_id;

    if (0 == (ret = sng_cntrl_m2ua (&pst, &cntrl))) {
        clust->flags |= SNGSS7_ACTIVE;
    }
    return ret;
}   
/***********************************************************************************************************************/
static int ftmod_nif_m2ua_dlsap_bind(int id , int action)
{
  Pst pst;
  NwMgmt cntrl;  
  sng_nif_cfg_t* nif = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[id];

  memset((U8 *)&pst, 0, sizeof(Pst));
  memset((U8 *)&cntrl, 0, sizeof(NwMgmt));

  smPstInit(&pst);

  pst.dstEnt = ENTNW;

  /* prepare header */
  cntrl.hdr.msgType     = TCNTRL;         /* message type */
   cntrl.hdr.entId.ent   = ENTNW;          /* entity */
   cntrl.hdr.entId.inst  = 0;              /* instance */
   cntrl.hdr.elmId.elmnt = STNWDLSAP;       /* General */
   cntrl.hdr.transId     = 1;     /* transaction identifier */

   cntrl.hdr.response.selector    = 0;
   cntrl.hdr.response.prior       = PRIOR0;
   cntrl.hdr.response.route       = RTESPEC;
   cntrl.hdr.response.mem.region  = S_REG;
   cntrl.hdr.response.mem.pool    = S_POOL;

   cntrl.t.cntrl.action = action;
   cntrl.t.cntrl.suId = nif->id;      /* NIF DL sap Id */
   cntrl.t.cntrl.entity = ENTMW; /* M2UA */

   return (sng_cntrl_nif (&pst, &cntrl));
}   

/***********************************************************************************************************************/
static int ftmod_nif_mtp2_dlsap_bind(int id, int action)
{
  Pst pst;
  NwMgmt cntrl;  
  sng_nif_cfg_t* nif = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[id];

  memset((U8 *)&pst, 0, sizeof(Pst));
  memset((U8 *)&cntrl, 0, sizeof(NwMgmt));

  smPstInit(&pst);

  pst.dstEnt = ENTNW;

  /* prepare header */
  cntrl.hdr.msgType     = TCNTRL;         /* message type */
   cntrl.hdr.entId.ent   = ENTNW;          /* entity */
   cntrl.hdr.entId.inst  = 0;              /* instance */
   cntrl.hdr.elmId.elmnt = STNWDLSAP;       /* General */
   cntrl.hdr.transId     = 1;     /* transaction identifier */

   cntrl.hdr.response.selector    = 0;
   cntrl.hdr.response.prior       = PRIOR0;
   cntrl.hdr.response.route       = RTESPEC;
   cntrl.hdr.response.mem.region  = S_REG;
   cntrl.hdr.response.mem.pool    = S_POOL;

   cntrl.t.cntrl.action = action;
   cntrl.t.cntrl.suId = nif->id;      /* NIF DL sap Id */
   cntrl.t.cntrl.entity = ENTSD;      /* MTP2 */

     return (sng_cntrl_nif (&pst, &cntrl));

}

/***********************************************************************************************************************/
static int ftmod_m2ua_enable_alarm()
{
	Pst pst;
	MwMgmt cntrl;  

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTMW;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTMW;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STMWGEN;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action = AENA;
	cntrl.t.cntrl.subAction = SAUSTA;

	return (sng_cntrl_m2ua (&pst, &cntrl));
}
/***********************************************************************************************************************/

static int ftmod_m2ua_debug(int action)
{
	Pst pst;
	MwMgmt cntrl;  

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTMW;

	/* prepare header */
	cntrl.hdr.msgType     = TCNTRL;         /* message type */
	cntrl.hdr.entId.ent   = ENTMW;          /* entity */
	cntrl.hdr.entId.inst  = 0;              /* instance */
	cntrl.hdr.elmId.elmnt = STMWGEN;       /* General */

	cntrl.hdr.response.selector    = 0;
	cntrl.hdr.response.prior       = PRIOR0;
	cntrl.hdr.response.route       = RTESPEC;
	cntrl.hdr.response.mem.region  = S_REG;
	cntrl.hdr.response.mem.pool    = S_POOL;

	cntrl.t.cntrl.action = action;
	cntrl.t.cntrl.subAction = SADBG;
	cntrl.t.cntrl.s.dbgMask   = 0xFFFF;

	return (sng_cntrl_m2ua (&pst, &cntrl));
}
/***********************************************************************************************************************/
int ftmod_m2ua_ssta_req(int elemt, int id, MwMgmt* cfm)
{
    MwMgmt ssta; 
    Pst pst;
    sng_m2ua_cfg_t* 	 m2ua  = NULL; 
    sng_m2ua_cluster_cfg_t*  clust = NULL; 

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&ssta, 0, sizeof(MwMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    ssta.hdr.msgType     = TSSTA;         /* message type */
    ssta.hdr.entId.ent   = ENTMW;          /* entity */
    ssta.hdr.entId.inst  = 0;              /* instance */
    ssta.hdr.elmId.elmnt = elemt; 	      /*STMWGEN */ /* Others are STMWSCTSAP, STMWCLUSTER, STMWPEER,STMWSID, STMWDLSAP */
    ssta.hdr.transId     = 1;     /* transaction identifier */

    ssta.hdr.response.selector    = 0;
    ssta.hdr.response.prior       = PRIOR0;
    ssta.hdr.response.route       = RTESPEC;
    ssta.hdr.response.mem.region  = S_REG;
    ssta.hdr.response.mem.pool    = S_POOL;

    switch(ssta.hdr.elmId.elmnt)
    {
        case STMWSCTSAP:
            {
                m2ua  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[id];
                clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[m2ua->clusterId];
                ssta.t.ssta.id.suId = clust->sct_sap_id ; /* lower sap Id */            
                break;
            }       
        case STMWDLSAP:
            {
                ssta.t.ssta.id.lnkNmb = id ; /* upper sap Id */            
                break;
            }
        case STMWPEER:
            {
                ssta.t.ssta.id.peerId = id ; /* peer Id */            
                break;
            }
        case STMWCLUSTER:
            {
                clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[id];
                ssta.t.ssta.id.clusterId = clust->id ; /* cluster Id */            
                break;
            }
        default:
            break;
    }

    return(sng_sta_m2ua(&pst,&ssta,cfm));
}

int ftmod_nif_ssta_req(int elemt, int id, NwMgmt* cfm)
{
	NwMgmt ssta; 
	Pst pst;
	sng_nif_cfg_t* nif = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[id];

	memset((U8 *)&pst, 0, sizeof(Pst));
	memset((U8 *)&ssta, 0, sizeof(NwMgmt));

	smPstInit(&pst);

	pst.dstEnt = ENTNW;

	/* prepare header */
	ssta.hdr.msgType     = TSSTA;         /* message type */
	ssta.hdr.entId.ent   = ENTNW;          /* entity */
	ssta.hdr.entId.inst  = 0;              /* instance */
	ssta.hdr.elmId.elmnt = elemt;

	ssta.hdr.response.selector    = 0;
	ssta.hdr.response.prior       = PRIOR0;
	ssta.hdr.response.route       = RTESPEC;
	ssta.hdr.response.mem.region  = S_REG;
	ssta.hdr.response.mem.pool    = S_POOL;
	ssta.t.ssta.suId = nif->id; /*  Lower sapId */

	return(sng_sta_nif(&pst,&ssta,cfm));
}
/************************************************************************************/
void ftdm_m2ua_start_timer(sng_m2ua_tmr_evt_types_e evt_type , int peer_id)
{
    char timer_name[128];
    sng_m2ua_tmr_sched_t  *sched = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.sched;
    sng_m2ua_peer_cfg_t   *peer  = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_peer[peer_id];

    memset(&timer_name[0],0,sizeof(timer_name));


    if (peer->tmr_running) {
        SS7_INFO (" Timer[%s] already running with timer-id[%d]\n",
                SNG_M2UA_PRINT_TIMER(peer->tmr_event), sched->tmr_id);
        return;
    }

    peer->tmr_event = evt_type;

    switch(evt_type)
    {
        case SNG_M2UA_TIMER_ASP_UP:
            {
                strcpy(&timer_name[0],"asp-up-timer");
                break;
            }
        case SNG_M2UA_TIMER_ASP_ACTIVE:
            {
                strcpy(&timer_name[0],"asp-active-timer");
                break;
            }
        case SNG_M2UA_TIMER_MTP3_LINKSET_BIND_ENABLE:
            {
                strcpy(&timer_name[0],"mtp3-linkset-bind-enable-timer");
                break;
            }
        default:
            return;
    }


    if (ftdm_sched_timer (sched->tmr_sched,
                &timer_name[0],	
                2000, /* time is in ms - starting tmr for 2 sec */	
                ftdm_m2ua_handle_tmr_expiry,	
                peer,	
                &sched->tmr_id))
    {
        SS7_ERROR ("Unable to schedule %s timer\n", &timer_name[0]);
    }else{
        SS7_INFO ("Timer[%s] started with timer-id[%d] for 100 ms for peer[%d]\n",
                &timer_name[0], sched->tmr_id,peer->id);
        peer->tmr_running = 0x01;
    }

    return;
}
/************************************************************************************/
void ftdm_m2ua_handle_tmr_expiry(void *userdata)
{
    int x , y , a , b, peer_id = 0;
    sng_m2ua_peer_cfg_t         *peer = (sng_m2ua_peer_cfg_t*)userdata;
    sng_m2ua_cluster_cfg_t*     clust = NULL; 
    sng_m2ua_cfg_t*             m2ua  = NULL; 

    if (NULL == peer) {
        ftdm_log (FTDM_LOG_ERROR ,"Invalid userdata \n");
        return;
    }

    peer->tmr_running = 0x00;

    ftdm_log (FTDM_LOG_INFO ,"Timer[%s] Expiry for peer[%d] \n", 
            SNG_M2UA_PRINT_TIMER(peer->tmr_event), peer->id);

    switch(peer->tmr_event)
    {
        case SNG_M2UA_TIMER_ASP_UP:
            {
                if (ftmod_asp_up(peer->id)) {
                    ftdm_log (FTDM_LOG_ERROR ,"ftmod_asp_up FAIL  \n");
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"ftmod_asp_up SUCCESS  \n");
                }
                break;
            }
        case SNG_M2UA_TIMER_ASP_ACTIVE:
            {
                if (ftmod_asp_ac(peer->id)) {
                    ftdm_log (FTDM_LOG_ERROR ,"ftmod_asp_ac FAIL  \n");
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"ftmod_asp_ac SUCCESS  \n");
                }
                break;
            }
        case SNG_M2UA_TIMER_MTP3_LINKSET_BIND_ENABLE:
            {
                /* reverse approach - peer_id to mtp3 linkset id 
                 * 1st - get cluster id
                 * 2nd - get m2ua id
                 * 3rd - get mtp3_link id 
                 * */

                x = 1;
                while (x < MW_MAX_NUM_OF_CLUSTER) {
                    clust = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua_clus[x];

                    for (y = 0; y < clust->numOfPeers;y++) {
                        peer_id = clust->peerIdLst[y];
                        if (peer_id == peer->id) {
                            /* got matched cluster-id, now try to get m2ua_id */
                            a = 1;
                            while ( a < MW_MAX_NUM_OF_INTF ) {
                                m2ua = &g_ftdm_sngss7_data.cfg.g_m2ua_cfg.m2ua[a];
                                if (clust->id == m2ua->clusterId) {
                                    /* got m2ua id */

                                    /* loop through mtp3_link to get linkset-id */
                                    b = 1;
                                    while (b < (MAX_MTP_LINKS)) {
                                        if ((g_ftdm_sngss7_data.cfg.mtp3Link[b].id != 0) &&
                                                ((g_ftdm_sngss7_data.cfg.mtp3Link[b].flags & SNGSS7_CONFIGURED))) {

                                            if (m2ua->id == g_ftdm_sngss7_data.cfg.mtp3Link[b].mtp2Id) {
                                                ftdm_log (FTDM_LOG_INFO ,"Configuring linkset-id[%d]\n",
                                                        g_ftdm_sngss7_data.cfg.mtp3Link[b].linkSetId);                                   
                                                ftmod_ss7_enable_linkset(g_ftdm_sngss7_data.cfg.mtp3Link[b].linkSetId);
                                            }
                                        }
                                        b++;
                                    } /* MAX_MTP_LINKS while loop */
                                }
                                a++;
                            } /* MW_MAX_NUM_OF_INTF while loop */

                        } /* if (peer_id == peer->id) */

                    } /* clust->numOfPeers for loop */

                    x++;
                } /* MW_MAX_NUM_OF_CLUSTER */

                break;
            }
        default:
            return;
    }

    return;
}

/************************************************************************************/
int ftmod_asp_up(int peer_id)
{
    Pst pst;
    MwMgmt cntrl;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTMW;

    /* prepare header */
    cntrl.hdr.msgType     = TCNTRL;         /* message type */
    cntrl.hdr.entId.ent   = ENTMW;          /* entity */
    cntrl.hdr.entId.inst  = 0;              /* instance */
    cntrl.hdr.elmId.elmnt = STMWPEER;       /* General */
    cntrl.hdr.transId     = 1;     /* transaction identifier */

    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;


    cntrl.t.cntrl.action = AMWASPUP;
    /* KAPIL - Not Requried 
     * cntrl.t.cntrl.s.suId = 0; */

    cntrl.t.cntrl.s.peerId = (MwPeerId) peer_id;

    cntrl.t.cntrl.aspm.autoCtx = TRUE;
    memcpy((U8 *)cntrl.t.cntrl.aspm.info, (CONSTANT U8 *)"PEER_0", sizeof("PEER_0"));


    return (sng_cntrl_m2ua (&pst, &cntrl));

}
/************************************************************************************/
int ftmod_asp_ac(int peer_id)
{
  Pst pst;
  MwMgmt cntrl;

  memset((U8 *)&pst, 0, sizeof(Pst));
  memset((U8 *)&cntrl, 0, sizeof(MwMgmt));

  smPstInit(&pst);

  pst.dstEnt = ENTMW;

  /* prepare header */
  cntrl.hdr.msgType     = TCNTRL;         /* message type */
   cntrl.hdr.entId.ent   = ENTMW;          /* entity */
   cntrl.hdr.entId.inst  = 0;              /* instance */
   cntrl.hdr.elmId.elmnt = STMWPEER;       /* General */
   cntrl.hdr.transId     = 1;     /* transaction identifier */

   cntrl.hdr.response.selector    = 0;
   cntrl.hdr.response.prior       = PRIOR0;
   cntrl.hdr.response.route       = RTESPEC;
   cntrl.hdr.response.mem.region  = S_REG;
   cntrl.hdr.response.mem.pool    = S_POOL;


   cntrl.t.cntrl.action = AMWASPAC;
  /* KAPIL - NOT REQUIRED  cntrl.t.cntrl.s.suId = 0; */

   cntrl.t.cntrl.s.peerId = (MwPeerId) peer_id;

   cntrl.t.cntrl.aspm.autoCtx = TRUE;
   cntrl.t.cntrl.aspm.type = LMW_TRF_MODE_LOADSHARE;
   memcpy((U8 *)cntrl.t.cntrl.aspm.info, (CONSTANT U8 *)"PEER_0", sizeof("PEER_0"));

   return (sng_cntrl_m2ua (&pst, &cntrl));
}
/************************************************************************************/
static int ftmod_ss7_get_nif_id_by_mtp2_id(int mtp2_id)
{
    int nif_id = 0x00;
    int x = 1;
    while (x<MW_MAX_NUM_OF_INTF) {
        if ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].id !=0) && 
                ((g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].flags & SNGSS7_CONFIGURED))) {

            if (mtp2_id == g_ftdm_sngss7_data.cfg.g_m2ua_cfg.nif[x].mtp2LnkNmb) {
                nif_id = x; 
                ftdm_log (FTDM_LOG_INFO ,"Matched associated NIF id [%d] \n", nif_id);
                break;
            }
        }
        x++;
    }

    return nif_id;
}

/************************************************************************************/
