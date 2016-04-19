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
 *		Kapil Gupta <kgupta@sangoma.com>
 *		Pushkar Singh <psingh@sangoma.com>
 *
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/
/* FUNCTION PROTOTYPES ********************************************************/
static int ftmod_ss7_m3ua_shutdown(void);
static ftdm_status_t ftmod_m3ua_network_config(int id);
static ftdm_status_t ftmod_m3ua_sctsap_config(int sct_sap_id, int sctp_id);
static ftdm_status_t ftmod_m3ua_nsap_config(int id);
static ftdm_status_t ftmod_m3ua_nif_nsap_config(int id);
static ftdm_status_t ftmod_m3ua_psp_config(int id);
static ftdm_status_t ftmod_m3ua_ps_config(int id);
static ftdm_status_t ftmod_m3ua_route_config(int id, int nsap_id, ftdm_sngss7_operating_modes_e opr_mode);
static ftdm_status_t ftmod_m3ua_sctp_sctsap_bind(int id);
static ftdm_status_t ftmod_m3ua_gen_config(ftdm_sngss7_operating_modes_e opr_mode);
static ftdm_status_t ftmod_nif_m3ua_nsap_bind(int id);
static ftdm_status_t ftmod_m3ua_open_endpoint(int id);
static ftdm_status_t ftmod_m3ua_establish_association(int id);
void ftdm_m3ua_handle_tmr_expiry(void *userdata);
static void ftmod_m3ua_get_nwk_appearance(int nwId, ItNwkApp nwkApp[], char* appearance);


/******************************************************************************/
ftdm_status_t ftmod_ss7_m3ua_init(int opr_mode)
{
	/****************************************************************************************************/
	if (sng_isup_init_m3ua()) {
		ftdm_log (FTDM_LOG_ERROR ,"Failed to start M3UA\n");
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"Started M3UA!\n");
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
	if (ftmod_m3ua_gen_config(opr_mode)) {
		ftdm_log (FTDM_LOG_ERROR ,"M3UA General configuration: NOT OK\n");
		return FTDM_FAIL;
	} else {
		ftdm_log (FTDM_LOG_INFO ,"M3UA General configuration: OK\n");
	}
	/****************************************************************************************************/

	return FTDM_SUCCESS;
}

/******************************************************************************/
ftdm_status_t ftmod_ss7_m3ua_cfg(int opr_mode)
{
    int x=0;
    int idx=0;
    int nsap_id=0;

    if (g_ftdm_sngss7_data.stack_logging_enable) {
	    ftmod_ss7_enable_m3ua_logging();
    }

    /****************************************************************************************************/
    /* SCTP/TUCL configuration */
    if (ftmod_cfg_sctp(opr_mode)) {
        ftdm_log (FTDM_LOG_ERROR ,"SCTP Configuration : NOT OK\n");
        return FTDM_FAIL;
    } else {
        ftdm_log (FTDM_LOG_INFO ,"SCTP Configuration : OK\n");
    }

    /****************************************************************************************************/
    /* M3UA Network configuration */
    x = 1;
    while (x<MAX_M3UA_NWK) {
        if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[x].id !=0) &&
                (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[x].flags & SNGSS7_CONFIGURED))) {

            if (ftmod_m3ua_network_config(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"M3UA Network configuration for Network-Id[%d] : NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M3UA Network configuration for Network-Id[%d] : OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[x].flags |= SNGSS7_CONFIGURED;
        }
        x++;
    }

    /****************************************************************************************************/
    /* M3UA SCTP SAP configurations */
    x = 1;
    while (x<MAX_M3UA_PSP) {

        if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].id !=0) &&
                (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].flags & SNGSS7_CONFIGURED))) {

            if (ftmod_m3ua_sctsap_config(x,g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].sctpId)) {
                ftdm_log (FTDM_LOG_ERROR ,"M3UA SCT SAP configuration for PSP ID[%d] : NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M3UA SCT SAP configuration for PSP ID[%d] : OK\n", x);
            }

            if (ftmod_m3ua_psp_config(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"M3UA PSP configuration for PSP ID[%d] : NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M3UA PSP configuration for PSP ID[%d] : OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].flags |= SNGSS7_CONFIGURED;
        }
        x++;
    }

    /****************************************************************************************************/
    /* M3UA N-SAP configurations */
    if (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) {

        x = 1;
        while (x<MAX_M3UA_SAP) {

            if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[x].id !=0) &&
                    (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[x].flags & SNGSS7_CONFIGURED))) {

                if (ftmod_m3ua_nsap_config(x)) {
                    ftdm_log (FTDM_LOG_ERROR ,"M3UA N-SAP configuration for M3UA User ID[%d] : NOT OK\n", x);
                    return FTDM_FAIL;
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"M3UA N-SAP configuration for M3UA User ID[%d] : OK\n", x);
                }
                g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[x].flags |= SNGSS7_CONFIGURED;
            }
            x++;
        }
    } else {
        x = 1;
        while (x<MAX_M3UA_RTE) {

            if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].id !=0) &&
                    (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].flags & SNGSS7_CONFIGURED))) {

		/* NIF Sap must based on network ID i.e. if a both the sap having same network then
		 * there is no need to configure Sap again as one sap will have the information of M3UA sap ID as well as MTP3
		 * sap ID based */
		/* It can be done in two ways either we can do this based on the new configuration like nif_sap or we can have a
		 * check here it self */
		idx = 1;
		while (idx < MAX_M3UA_RTE) {
		   if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].nwkId == g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].nwkId) &&
		       (x != idx) && (x > idx)) {
		       break;
		   }
		   idx++;
		}

		if ((idx != MAX_M3UA_RTE)) {
		   x++;
		   continue;
		}

		/* NIF nsap needs to be configured based on network ID configuration */
                if (ftmod_m3ua_nif_nsap_config(x)) {
                    ftdm_log (FTDM_LOG_ERROR ,"M3UA NIF N-SAP configuration for M3UA NIF ID[%d] : NOT OK\n", x);
                    return FTDM_FAIL;
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"M3UA NIF N-SAP configuration for M3UA NIF ID[%d] : OK\n", x);
                }
                g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].flags |= SNGSS7_CONFIGURED;
            }
            x++;
        }
    }


    /****************************************************************************************************/
    /* M3UA PS configurations */
    x = 1;
    while (x<MAX_M3UA_PS) {

        if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[x].id !=0) &&
                (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[x].flags & SNGSS7_CONFIGURED))) {

            if (ftmod_m3ua_ps_config(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"M3UA PS configuration for M3UA PS ID[%d] : NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M3UA PS configuration for M3UA PS ID[%d] : OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[x].flags |= SNGSS7_CONFIGURED;
        }
        x++;
    }
    /****************************************************************************************************/
    /* M3UA ROUTE configurations */
    x = 1;
    while (x<MAX_M3UA_RTE) {

        if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[x].id !=0) &&
                (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[x].flags & SNGSS7_CONFIGURED))) {

            if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
                /* nsap-id should be for each NIF sap whose route-id matches with x */
                idx = 1;
                while (idx<MAX_M3UA_RTE) {
                    if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].id !=0) &&
                            (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].m3ua_route_id ==
                                g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[x].m3ua_rte_id )) {
                        nsap_id = g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].nwkId;
                    }
                    idx++;
                }
            } else {
                /* nsap-id should be for each upper sap whose network-id matches with x's network-id */
                idx = 1;
                while (idx<MAX_M3UA_SAP) {
                    if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[idx].id !=0) &&
                            (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[x].nwkId ==
                             g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[idx].nwkId)) {
                        /* found associated nsap */
                        nsap_id = g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[idx].id;
                    }
                    idx++;
                }
            }

            if (ftmod_m3ua_route_config(x, nsap_id, opr_mode)) {
                ftdm_log (FTDM_LOG_ERROR ,"M3UA ROUTE configuration for M3UA ROUTE ID[%d] : NOT OK\n", x);
                return FTDM_FAIL;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"M3UA ROUTE configuration for M3UA ROUTE ID[%d] : OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[x].flags |= SNGSS7_CONFIGURED;
        }
        x++;
    }

    /****************************************************************************************************/
    sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_M3UA_PRESENT);
    /****************************************************************************************************/

    return FTDM_SUCCESS;
}

/***********************************************************************************************************************/
void ftmod_ss7_enable_m3ua_logging(void)
{
	/* Enable DEBUGs*/
	ftmod_m3ua_debug(AENA);
	ftmod_sctp_debug(AENA);
	ftmod_tucl_debug(AENA);
}

/***********************************************************************************************************************/
void ftmod_ss7_disable_m3ua_logging(void)
{
	/* DISABLE DEBUGs*/
	ftmod_m3ua_debug(ADISIMM);
	ftmod_sctp_debug(ADISIMM);
	ftmod_tucl_debug(ADISIMM);

	g_ftdm_sngss7_data.stack_logging_enable = 0x00;
}

/**********************************************************************************************/
ftdm_status_t ftmod_ss7_m3ua_start(int opr_mode)
{
    int x=0;
    int idx=0;

    /***********************************************************************************************************************/
    x = 1;
    while (x<MAX_SCTP_LINK) {
        if ((g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].id !=0) &&
                (!(g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[x].flags & SNGSS7_ACTIVE))) {

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
    /* Send a control request to bind the SCTSAP between SCTP and M3UA */
    x = 1;
    while (x<MAX_M3UA_PSP) {

        if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].id !=0) &&
                (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].flags & SNGSS7_ACTIVE))) {

            if (ftmod_m3ua_sctp_sctsap_bind(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"Control request to bind SCTSAP[%d] of M3UA and SCTP : NOT OK\n", x);
                return 1;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"Control request to bind SCTSAP[%d] of M3UA and SCTP: OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].flags |= SNGSS7_ACTIVE;
        }
        x++;
    }/* END - M3UA PSP Interfaces while loop*/
    /***********************************************************************************************************************/

    if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
        x = 1;
        while (x<MAX_M3UA_RTE) {
            if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].id !=0) &&
                    (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].flags & SNGSS7_ACTIVE))) {

		/* NIF Sap must based on network ID i.e. if a both the sap having same network then
		 * there is no need to configure Sap again as one sap will have the information of M3UA sap ID as well as MTP3
		 * sap ID based */
		/* It can be done in two ways either we can do this based on the new configuration like nif_sap or we can have a
		 * check here it self */
		idx = 1;
		while (idx < MAX_M3UA_RTE) {
		   if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].nwkId == g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].nwkId) &&
		       (x != idx) && (x > idx)) {
		       break;
		   }
		   idx++;
		}

		if (idx != MAX_M3UA_RTE) {
		   x++;
		   continue;
		}

                if (ftmod_nif_m3ua_nsap_bind(x)) {
                    ftdm_log (FTDM_LOG_ERROR ,"Control request to bind NSAP[%d] between M3UA and MTP3: NOT OK\n", x);
                    return 1;
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"Control request to bind NSAP[%d] between M3UA and MTP3 : OK\n", x);
                }

                g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[x].flags |= SNGSS7_ACTIVE;
            }
            x++;
        }/* END - NIF Interfaces for loop*/
    }

    /***********************************************************************************************************************/
    /* open sctp endpoint */
    x = 1;
    while (x<MAX_M3UA_PSP) {

        if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].id !=0) &&
                ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].flags & SNGSS7_ACTIVE))) {

            if (ftmod_m3ua_open_endpoint(x)) {
                ftdm_log (FTDM_LOG_ERROR ,"Control request to open endpoint for SCTSAP[%d] : NOT OK\n", x);
                return 1;
            } else {
                ftdm_log (FTDM_LOG_INFO ,"Control request to open endpoint for SCTSAP[%d] : OK\n", x);
            }
            g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].flags |= SNGSS7_ACTIVE;
        }
        x++;
    }/* END - M3UA PSP Interfaces while loop*/

    return FTDM_SUCCESS;
}

/***********************************************************************************************************************/
#if 0
static ftdm_status_t ftmod_m3ua_reg_key(int id)
{
    Pst pst;
    ItMgmt cntrl;
    sng_m3ua_psp_cfg_t*       psp = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id];
    int ret = 0x00;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STITPSP;
    cntrl.hdr.transId           = 1;     /* transaction identifier */
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = ARKREG;
    cntrl.t.cntrl.subAction     = SAELMNT;

    cntrl.t.cntrl.s.pspId       = psp->id;
    cntrl.t.cntrl.t.aspm.sctSuId = psp->id;


    cntrl.t.cntrl.t.dynRteKey.reqType = LIT_REG_REQ;

    cntrl.t.cntrl.t.dynRteKey.rkm.rk.nmbRk = ;

    ret =  sng_cntrl_m3ua(&pst, &cntrl);

    if ((LCM_PRIM_OK == ret) || (LCM_PRIM_OK_NDONE == ret )) return FTDM_SUCCESS;

    return ret;
}
#endif

/***********************************************************************************************************************/
static ftdm_status_t ftmod_m3ua_establish_association(int id)
{
    Pst pst;
    ItMgmt cntrl;
    sng_m3ua_psp_cfg_t*       psp = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id];
    int ret = 0x00;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STITPSP;
    cntrl.hdr.transId           = 1;     /* transaction identifier */
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = AESTABLISH;
    cntrl.t.cntrl.subAction     = SAELMNT;

    cntrl.t.cntrl.s.pspId       = psp->id;
    cntrl.t.cntrl.t.aspm.sctSuId = psp->id;

    ret =  sng_cntrl_m3ua(&pst, &cntrl);

    if ((LCM_PRIM_OK == ret) || (LCM_PRIM_OK_NDONE == ret )) return FTDM_SUCCESS;

    return ret;
}

/***********************************************************************************************************************/
static ftdm_status_t ftmod_m3ua_sctp_sctsap_bind(int id)
{
    Pst pst;
    ItMgmt cntrl;
    sng_m3ua_psp_cfg_t*       psp  = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id];
 //   sng_sctp_link_t 	     *sctp = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[psp->sctpId];
    int ret = 0x00;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));


    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STITSCTSAP;
    cntrl.hdr.transId           = 1;     /* transaction identifier */
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = ABND;
    cntrl.t.cntrl.subAction     = SAELMNT;

    cntrl.t.cntrl.s.suId        = psp->id;

    ret =  sng_cntrl_m3ua(&pst, &cntrl);

    if ((LCM_PRIM_OK == ret) || (LCM_PRIM_OK_NDONE == ret )) {
	    return FTDM_SUCCESS;
    }

    return ret;
}

/***********************************************************************************************************************/
static ftdm_status_t ftmod_m3ua_open_endpoint(int id)
{
    Pst pst;
    ItMgmt cntrl;
    sng_m3ua_psp_cfg_t*       psp = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id];
    int ret = 0x00;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STITSCTSAP;
    cntrl.hdr.transId           = 1;     /* transaction identifier */
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = AEOPENR;
    cntrl.t.cntrl.subAction     = SAELMNT;

    cntrl.t.cntrl.s.suId        = psp->id;

    ret =  sng_cntrl_m3ua(&pst, &cntrl);

    if ((LCM_PRIM_OK == ret) || (LCM_PRIM_OK_NDONE == ret )) return FTDM_SUCCESS;

    return ret;
}
/***********************************************************************************************************************/

void ftmod_ss7_m3ua_free()
{
	if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_M3UA_PRESENT)) {
		ftmod_ss7_m3ua_shutdown();
		sng_isup_free_m3ua();
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
static int ftmod_ss7_m3ua_shutdown()
{
    ItMgmt cntrl;
    Pst pst;

    smPstInit(&pst);
    pst.dstEnt = ENTIT;
    memset(&cntrl, 0x0, sizeof(ItMgmt));
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst            = S_INST;
    cntrl.hdr.elmId.elmnt       = STITGEN;

    cntrl.t.cntrl.action            = ASHUTDOWN;
    cntrl.t.cntrl.subAction     = SAELMNT;      /* specificed element */
    return (sng_cntrl_m3ua(&pst, &cntrl));
}

/***********************************************************************************************************************/
static ftdm_status_t ftmod_m3ua_gen_config(ftdm_sngss7_operating_modes_e opr_mode)
{
    ItMgmt cfg;
    Pst pst;
    int ret = -1;

    /* initalize the post structure */
    smPstInit(&pst);

    /* insert the destination Entity */
    pst.dstEnt = ENTIT;

    /* clear the configuration structure */
    memset(&cfg, 0, sizeof(cfg));

    /* fill in the post structure */
    smPstInit(&cfg.t.cfg.s.genCfg.smPst);
    /*fill in the specific fields of the header */
    cfg.hdr.msgType     = TCFG;
    cfg.hdr.entId.ent   = ENTIT;
    cfg.hdr.entId.inst  = S_INST;
    cfg.hdr.elmId.elmnt = STITGEN;

    cfg.t.cfg.s.genCfg.smPst.srcEnt     = ENTIT;
    cfg.t.cfg.s.genCfg.smPst.dstEnt     = ENTSM;

    switch(opr_mode)
    {
        case SNG_SS7_OPR_MODE_M3UA_ASP:
            cfg.t.cfg.s.genCfg.nodeType = LIT_TYPE_ASP;
            break;
        case SNG_SS7_OPR_MODE_M3UA_SG:
            cfg.t.cfg.s.genCfg.nodeType = LIT_TYPE_SGP;
            break;
        default:
            break;
    }

    /* DPC learning only applicable to ASP */
    if (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) {
        cfg.t.cfg.s.genCfg.dpcLrnFlag           = TRUE;                /* DPC learning mode enable */
    } else {
        cfg.t.cfg.s.genCfg.dpcLrnFlag           = FALSE;                /* DPC learning mode enable */
    }

    cfg.t.cfg.s.genCfg.maxNmbNSap           = 4; /* MAX_SN_VARIANTS; */

    cfg.t.cfg.s.genCfg.maxNmbSctSap         = 4; /*MAX_SN_VARIANTS;*/          /* number of lower SAPs */

    cfg.t.cfg.s.genCfg.maxNmbNwk            = MAX_M3UA_NWK;

    cfg.t.cfg.s.genCfg.maxNmbRtEnt          = MAX_SN_ROUTES;

    cfg.t.cfg.s.genCfg.maxNmbDpcEnt         = MAX_SN_ROUTES;

    cfg.t.cfg.s.genCfg.maxNmbPs             = MAX_IT_PS;

    cfg.t.cfg.s.genCfg.maxNmbPsp            = MAX_IT_PSP;

    cfg.t.cfg.s.genCfg.maxNmbMsg            = 100;                  /* max MTP3/M3UA messages in transit */
    cfg.t.cfg.s.genCfg.maxNmbRndRbnLs       = 0;                    /* number of Round Robin Loadshare  contexts */
    cfg.t.cfg.s.genCfg.maxNmbSlsLs          = 128;                  /* number of SLS based Loadshare contexts */
    cfg.t.cfg.s.genCfg.drkmSupp             = TRUE;                 /* DRKM supported */
    cfg.t.cfg.s.genCfg.drstSupp             = TRUE;                 /* DRST supported */
    cfg.t.cfg.s.genCfg.timeRes              = SI_PERIOD;            /* timer resolution - period used for timers - 10,000 = 0.1s */
    cfg.t.cfg.s.genCfg.maxNmbSls            = 128;                  /* number of SLS contexts */

    /* Note congLevel1 <= congLevel2 <= congLevel3 <= qSize */
    cfg.t.cfg.s.genCfg.qSize                = 100;                  /* congestion queue size in M3UA */
    cfg.t.cfg.s.genCfg.congLevel1           = 50;                   /* congestion level 1 in the queue */
    cfg.t.cfg.s.genCfg.congLevel2           = 60;                   /* congestion level 2 in the queue */
    cfg.t.cfg.s.genCfg.congLevel3           = 70;                   /* congestion level 3 in the queue */

    cfg.t.cfg.s.genCfg.tmr.tmrRestart.enb   = TRUE;                 /* restart hold-off time */
    cfg.t.cfg.s.genCfg.tmr.tmrRestart.val   = 500;                  /* Recommended value is 5 sec */

    cfg.t.cfg.s.genCfg.tmr.tmrMtp3Sta.enb   = TRUE;                 /* MTP3 status poll time */
    cfg.t.cfg.s.genCfg.tmr.tmrMtp3Sta.val   = 300;                  /* Recommended value is 3 sec */

    cfg.t.cfg.s.genCfg.tmr.tmrAsPend.enb    = TRUE;                 /* AS-PENDING time */
    cfg.t.cfg.s.genCfg.tmr.tmrAsPend.val    = 300;                  /* Recommended value is 3 sec */

    /* According to M3UA RFC 4666, Heartbeat (BEAT) is
       recommended when M3UA runs over a transport layer
       other than SCTP, which has its own heartbeat */
    cfg.t.cfg.s.genCfg.tmr.tmrHeartbeat.enb = g_ftdm_sngss7_data.cfg.g_m3ua_cfg.gen_cfg.hbeat_tmr_enable;  /* heartbeat period */
    cfg.t.cfg.s.genCfg.tmr.tmrHeartbeat.val = g_ftdm_sngss7_data.cfg.g_m3ua_cfg.gen_cfg.hbeat_tmr_val;

    cfg.t.cfg.s.genCfg.tmr.tmrAspDn.enb     = TRUE;                 /* time between ASPDN */
    cfg.t.cfg.s.genCfg.tmr.tmrAspDn.val     = 2000;                 /* Recommended value is 2 sec */

    cfg.t.cfg.s.genCfg.tmr.tmrAspM.enb      = TRUE;                 /* timeout value for ASPM messages */
    cfg.t.cfg.s.genCfg.tmr.tmrAspM.val      = 2000;                 /* Recommended value is 2 sec */

    /* Initial period between ASPUP messages when attempting to bring up the path to the SCP.*/
    cfg.t.cfg.s.genCfg.tmr.tmrAspUp1.enb    = TRUE;                 /* initial number of ASPUP */
    cfg.t.cfg.s.genCfg.tmr.tmrAspUp1.val    = 2000;                 /* Recommended value is 2 sec */

    cfg.t.cfg.s.genCfg.tmr.tmrAspUp2.enb    = TRUE;                 /* steady-state time between ASPUP */
    cfg.t.cfg.s.genCfg.tmr.tmrAspUp2.val    = 2000;                 /* Recommended value is 2 sec */

    cfg.t.cfg.s.genCfg.tmr.nmbAspUp1        = 3;                    /* initial time between ASPUP */

    cfg.t.cfg.s.genCfg.tmr.tmrDaud.enb      = TRUE;                 /* time between DAUD */
    cfg.t.cfg.s.genCfg.tmr.tmrDaud.val      = 200;                  /* Recommended value is 2 sec */

    cfg.t.cfg.s.genCfg.tmr.maxNmbRkTry      = 3;                    /* initial number of DRKM msgs */

    cfg.t.cfg.s.genCfg.tmr.tmrDrkm.enb      = TRUE;                 /* time between DRKM msgs */
    cfg.t.cfg.s.genCfg.tmr.tmrDrkm.val      = 200;                  /* Recommended value is 2 sec */

    cfg.t.cfg.s.genCfg.tmr.tmrDunaSettle.enb    = TRUE;             /* time to settle DUNA message */
    cfg.t.cfg.s.genCfg.tmr.tmrDunaSettle.val    = 120;              /* Recommended value is 800-1200 ms */

    cfg.t.cfg.s.genCfg.tmr.tmrSeqCntrl.enb  = TRUE;                 /* Sequence Control timer */
    cfg.t.cfg.s.genCfg.tmr.tmrSeqCntrl.val  = 120;                  /* Recommended value is 500ms to 1200 ms */

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA General configuration DONE with heartbeat timer as %d with value %d!\n",
		 cfg.t.cfg.s.genCfg.tmr.tmrHeartbeat.enb, cfg.t.cfg.s.genCfg.tmr.tmrHeartbeat.val);
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA General configuration FAILED!\n");
        return FTDM_FAIL;
    }
}

/***********************************************************************************************************************/
/* API will return total number of PSPs sharing same network */
static void ftmod_m3ua_get_nwk_appearance(int nwId, ItNwkApp nwkApp[], char* appearance)
{
	int x = 0;
	x = 1;
	while (x<MAX_M3UA_PSP) {
		if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].id != 0) &&
				(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].nwkId == nwId)) {
			if (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].nwkAppearanceIncl) {
				nwkApp[g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[x].id] = atoi(appearance);
			}
		}
		x++;
	}
}
/***********************************************************************************************************************/
static ftdm_status_t ftmod_m3ua_network_config(int id)
{
    Pst         pst;
    ItMgmt      cfg;
    int             ret = 0;
    int         appearanceLen = 0;

    sng_m3ua_nwk_cfg_t  *k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[id];

    smPstInit(&pst);
    pst.dstEnt = ENTIT;

    memset(&cfg, 0x0, sizeof(cfg));
    smHdrInit(&cfg.hdr);

    cfg.hdr.msgType             = TCFG;
    cfg.hdr.entId.ent           = ENTIT;
    cfg.hdr.entId.inst          = S_INST;
    cfg.hdr.elmId.elmnt         = STITNWK;

    ItNwkCfg *c = &cfg.t.cfg.s.nwkCfg;

    appearanceLen = strlen(k->appearance);
    if (appearanceLen > LIT_MAX_PSP) {
        SS7_ERROR("M3UA NWK  %d is trying to configure appearance word \"%s\" over maximum limit. "
                " Only taking first %d characters. \n", id, k->appearance, LIT_MAX_PSP-1);
        appearanceLen = LIT_MAX_PSP-1;
    }
    if (appearanceLen <= 0) {
        SS7_WARN("M3UA NWK  %d is trying to configure empty appearance word. \n", id);
    }

    ftmod_m3ua_get_nwk_appearance(k->id, c->nwkApp, k->appearance);

    c->nwkId    = k->id;
    c->ssf      = k->ssf;
    switch(k->dpcLen) {
        case 14:
            c->dpcLen = DPC14;
            break;
        case 24:
            c->dpcLen = DPC24;
            break;
        case 16:
            c->dpcLen = DPC16;
            break;
        default:
            c->dpcLen = DPC_DEFAULT;
            SS7_WARN("M3UA NWK  %d is trying to configure default ssf value. \n", id);
            break;
    }

    switch(k->slsLen) {
        case 4:
            c->slsLen = LIT_SLS4;
            break;
        case 5:
            c->slsLen = LIT_SLS5;
            break;
        case 8:
            c->slsLen = LIT_SLS8;
            break;
        default:
            SS7_WARN("M3UA NWK  %d is trying to configure invalid sls-len value. \n", id);
            break;
    }

    c->suSwtch  = k->suSwitchType;
    c->su2Swtch = k->su2SwitchType;

    SS7_INFO("\n\n\t\tM3UA NWK %d configuring with id = %d,  ssf = %d, dpcLen = %d, slsLen = %d \n"
            "\t\tsuSwtch = %d,  su2Swtch = %d, appearance = %s \n\n",
            id, c->nwkId, c->ssf, c->dpcLen, c->slsLen,
            c->suSwtch, c->su2Swtch, c->nwkApp);

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA NWK %d configuration DONE!%d \n", id, ret);
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA NWK  %d configuration FAILED! %d \n", id, ret);
        return FTDM_FAIL;
    }
}

/**********************************************************************************************/
static ftdm_status_t ftmod_m3ua_sctsap_config(int sct_sap_id, int sctp_id)
{
    Pst         pst;
    ItMgmt      cfg;
    int         i;
    int         ret = 0;
    ItSctSapCfg *c = NULL;
    ItMgmt cfm;
    //sng_m3ua_psp_cfg_t* k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[sct_sap_id];
    sng_sctp_link_t *sctp = &g_ftdm_sngss7_data.cfg.sctpCfg.linkCfg[sctp_id];

    smPstInit(&pst);
    pst.dstEnt = ENTIT;

    memset(&cfg, 0x0, sizeof(cfg));
    memset((U8 *)&cfm, 0, sizeof(ItMgmt));
    smHdrInit(&cfg.hdr);

    /* check is sct_sap is already configured */
    if(!ftmod_m3ua_ssta_req(STITSCTSAP , sctp_id, &cfm)) {
            SS7_DEBUG(" ftmod_m3ua_sctsap_config: SCT SAP [%s] for SuId[%d] is already configured \n", sctp->name, sctp_id);
            return FTDM_SUCCESS;
    } else {
            SS7_DEBUG(" ftmod_m3ua_sctsap_config: SCT SAP [%s] for SuId[%d] is NOT configured \n", sctp->name, sctp_id);
    }

    if(LCM_REASON_INVALID_SAP == cfm.cfm.reason){
            SS7_DEBUG ( " ftmod_m3ua_sctsap_config: SCT SAP [%s] is not configured..configuring now \n", sctp->name);
    }

    cfg.hdr.msgType                 = TCFG;
    cfg.hdr.entId.ent               = ENTIT;
    cfg.hdr.entId.inst              = S_INST;
    cfg.hdr.elmId.elmnt             = STITSCTSAP;

    c = &cfg.t.cfg.s.sctSapCfg;
    c->spId                         = sct_sap_id;
    c->suId                         = sctp_id;
    c->srcPort                      = sctp->port;
    c->tmrPrim.enb                  = TRUE;
    c->tmrPrim.val                  = 5040;
    c->tmrSta.enb                   = TRUE;
    c->tmrSta.val                   = 5075;
    c->mem.pool                     = DFLT_POOL;
    c->mem.region                   = DFLT_REGION;
    c->procId                       = SFndProcId();
    c->ent                          = ENTSB;
    c->inst                         = 0;
    c->prior                        = 0;
    c->route                        = 0;

#ifdef SCT_ENDP_MULTI_IPADDR
    c->srcAddrLst.nmb  	            = sctp->numSrcAddr;

    if ( 0 == c->srcAddrLst.nmb ) {
        SS7_CRITICAL("No SCTP Source address configured: M3UA SCT SAP configuration FAILED!\n");
        return FTDM_FAIL;
    }

    for (i=0; i <= (sctp->numSrcAddr-1); i++) {
        c->srcAddrLst.nAddr[i].type = CM_NETADDR_IPV4;
        c->srcAddrLst.nAddr[i].u.ipv4NetAddr = sctp->srcAddrList[i+1];
    }
#else
    /* for single ip support ,src address will always be one */
    cfg.t.cfg.s.sctSapCfg.intfAddr.type          = CM_NETADDR_IPV4;
    cfg.t.cfg.s.sctSapCfg.intfAddr.u.ipv4NetAddr = sctp->srcAddrList[1];
#endif


    SS7_INFO("\n\n\t\tM3UA SCT SAP configuring with: suId = %d, srcPort = %d, tmrPrim = %d, \n"
            "\t\ttmrSta = %d, procId = %d, inst = %d, spId = %d, srcAddrLst.nmb = %d \n"
            "\t\tsrcAddrLst[0] = %d, \n",
            c->suId, c->srcPort, c->tmrPrim.val,
            c->tmrSta.val, c->procId, c->inst, c->spId, c->srcAddrLst.nmb,
            c->srcAddrLst.nAddr[0].u.ipv4NetAddr
            );

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA SCT SAP configuration DONE!\n");
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA SCT SAP configuration FAILED!\n");
        return FTDM_FAIL;
    }

}

/**********************************************************************************************/
static ftdm_status_t ftmod_m3ua_nsap_config(int id)
{
    Pst         pst;
    ItMgmt      cfg;

    int         ret = 0;
    ItNSapCfg   *c = NULL;
    sng_m3ua_sap_cfg_t *k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[id];

    smPstInit(&pst);
    pst.dstEnt = ENTIT;

    memset(&cfg, 0x0, sizeof(cfg));
    smHdrInit(&cfg.hdr);

    cfg.hdr.msgType                 = TCFG;
    cfg.hdr.entId.ent               = ENTIT;
    cfg.hdr.entId.inst              = S_INST;
    cfg.hdr.elmId.elmnt             = STITNSAP;

    c = &cfg.t.cfg.s.nSapCfg;
    c->sapId                        = k->id;            /* NSAP id */
    c->nwkId                        = k->nwkId;         /* logical network ID */
    c->suType                       = k->suType;    /* service user protocol type */
    c->mem.region                   = S_REG;
    c->mem.pool                     = S_POOL;

    c->selector                     = 0;
    c->prior                        = PRIOR0;
    c->route                        = RTESPEC;
#ifdef ITSG
    c->tmrPrim.enb                  = TRUE;       /* lower SAP primitive timer */
    c->tmrPrim.val                  = 300;
    c->procId                       = g_ftdm_sngss7_data.cfg.procId;        /* processor ID */
    c->ent                          = ENTIT;           /* entity */
    c->inst                         = S_INST;          /* instance */
    c->mtp3SapId                    = 1;     /* MTP3 service provider ID -valid only for ASP/IPSP mode*/
#endif

    SS7_INFO("\n\n\t\tM3UA NSAP %d configuring with: sapId = %d, nwkId = %d, suType = %d, \n\n",
            id, c->sapId, c->nwkId, c->suType);

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA NSAP %d configuration DONE!\n", id);
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA NSAP %d configuration FAILED!\n", id);
        return FTDM_FAIL;
    }
}

/**********************************************************************************************/
static ftdm_status_t ftmod_m3ua_nif_nsap_config(int id)
{
    Pst         pst;
    ItMgmt      cfg;

    int         ret = 0;
    ItNSapCfg   *c = NULL;
    sng_m3ua_nif_cfg_t *k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[id];
    sng_m3ua_rte_cfg_t *rte = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[k->m3ua_route_id];

    smPstInit(&pst);
    pst.dstEnt = ENTIT;

    memset(&cfg, 0x0, sizeof(cfg));
    smHdrInit(&cfg.hdr);


    cfg.hdr.msgType                 = TCFG;
    cfg.hdr.entId.ent               = ENTIT;
    cfg.hdr.entId.inst              = S_INST;
    cfg.hdr.elmId.elmnt             = STITNSAP;

    c = &cfg.t.cfg.s.nSapCfg;
    c->sapId                        = k->m3ua_route_id;   /* NSAP id */
    c->nwkId                        = rte->nwkId;         /* logical network ID */
    c->suType                       = LIT_SP_MTP3;        /* service user protocol type */
    c->mem.region                   = S_REG;
    c->mem.pool                     = S_POOL;

    c->selector                     = 0;
    c->prior                        = PRIOR0;
    c->route                        = RTESPEC;
#ifdef ITSG
    c->tmrPrim.enb                  = TRUE;       /* lower SAP primitive timer */
    c->tmrPrim.val                  = 300;
    c->procId                       = g_ftdm_sngss7_data.cfg.procId;        /* processor ID */
    c->ent                          = ENTSN;           /* entity */
    c->inst                         = S_INST;          /* instance */
    c->mtp3SapId                    = k->mtp3_route_id;     /* MTP3 service provider ID */
#endif

    SS7_INFO("\n\n\t\tM3UA NIF NSAP %d configuring with: sapId = %d, nwkId = %d, suType = %d, \n\n",
            id, c->sapId, c->nwkId, c->suType);

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA NIF NSAP %d configuration DONE!\n", id);
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA NIF NSAP %d configuration FAILED!\n", id);
        return FTDM_FAIL;
    }
}

/**********************************************************************************************/
static ftdm_status_t ftmod_nif_m3ua_nsap_bind(int id)
{
    Pst         pst;
    ItMgmt      cntrl;
    sng_m3ua_nif_cfg_t *k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[id];
    //sng_m3ua_rte_cfg_t *rte = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[k->m3ua_route_id];

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));

    smPstInit(&pst);
    pst.dstEnt = ENTIT;
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType                 = TCNTRL;
    cntrl.hdr.entId.ent               = ENTIT;
    cntrl.hdr.entId.inst              = S_INST;
    cntrl.hdr.elmId.elmnt             = STITNSAP;
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = ABND;
    cntrl.t.cntrl.subAction     = SAELMNT;

    cntrl.t.cntrl.s.suId        = k->m3ua_route_id;

    return (sng_cntrl_m3ua(&pst, &cntrl));
}

/**********************************************************************************************/
static ftdm_status_t ftmod_m3ua_psp_config(int id)
{
    Pst         pst;
    ItMgmt      cfg;
    int         i=0;
    int         ret = 0;
    ItPspCfg    *c = NULL;
    sng_m3ua_psp_cfg_t *k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id];

    smPstInit(&pst);
    pst.dstEnt = ENTIT;

    memset(&cfg, 0x0, sizeof(cfg));
    smHdrInit(&cfg.hdr);

    cfg.hdr.msgType                 = TCFG;
    cfg.hdr.entId.ent               = ENTIT;
    cfg.hdr.entId.inst              = S_INST;
    cfg.hdr.elmId.elmnt             = STITPSP;

    c = &cfg.t.cfg.s.pspCfg;
    c->pspId    = k->id;

    c->pspType       = k->remoteType;
    c->ipspMode      = k->ipspMode;
    c->dynRegRkallwd = k->drkmAllowed?TRUE:FALSE;
    c->nwkAppIncl = k->nwkAppearanceIncl?TRUE:FALSE;
    switch (k->txRxSpId) {
        case M3UA_ASPUP_SPID_TX:
            c->rxTxAspId  = 0x01; /* IT_TX_ASPID */
            break;
        case M3UA_ASPUP_SPID_RX:
            c->rxTxAspId  = 0x02; /* IT_RX_ASPID */
            break;
        case M3UA_ASPUP_SPID_BOTH:
            c->rxTxAspId  = 0x02; /* IT_RX_ASPID */
            c->rxTxAspId |= 0x01; /* IT_TX_ASPID */;
            break;
        case M3UA_ASPUP_SPID_NONE:
        default:
            c->rxTxAspId = 0;
            break;
    }

    c->selfAspId[0] = k->selfSpId;
    c->assocCfg.priDstAddr.u.ipv4NetAddr = k->primDestAddr;
    c->assocCfg.priDstAddr.type = CM_NETADDR_IPV4;
    c->assocCfg.dstPort = k->destPort;
    c->assocCfg.locOutStrms = k->streamNum;
    c->assocCfg.tos = k->tos;

    c->assocCfg.dstAddrLst.nmb = k->numOptDestAddr;
    for (i=0; i<k->numOptDestAddr; i++) {
        c->assocCfg.dstAddrLst.nAddr[i].type = CM_NETADDR_IPV4;
        c->assocCfg.dstAddrLst.nAddr[i].u.ipv4NetAddr = k->optDestAddr[i];
    }

    c->rcIsMand = TRUE;
    c->modRegRkallwd = TRUE;
    c->nwkId = k->nwkId;
#ifdef ITASP
    c->cfgForAllLps = FALSE;
#endif

    SS7_INFO("\n\n\t\tM3UA PSP %d configuring with: pspId = %d, pspType = %d, ipspMode = %d, nwkId = %d, \n"
            "\t\t dynRegRkallwd = %d, nwkAppIncl = %d, rxTxAspId = %d, selfAspId[0] = %d,\n"
            "\t\t assocCfg.priDstAddr = %d, assocCfg.dstPort = %d, assocCfg.locOutStrms = %d, tos = %d\n"
            "\t\t rcIsMand = %d, modRegRkallwd = %d, assocCfg.dstAddrLst.nmb = %d\n",
            id, c->pspId, c->pspType, c->ipspMode, c->nwkId,
            c->dynRegRkallwd, c->nwkAppIncl, c->rxTxAspId, c->selfAspId[0],
            c->assocCfg.priDstAddr.u.ipv4NetAddr, c->assocCfg.dstPort, c->assocCfg.locOutStrms, c->assocCfg.tos,
            c->rcIsMand, c->modRegRkallwd, c->assocCfg.dstAddrLst.nmb);

    for (i=0; i<c->assocCfg.dstAddrLst.nmb; i++) {
        if (i==c->assocCfg.dstAddrLst.nmb-1) {
            SS7_INFO("\n\t\tdstAddrLst = %x \n\n", c->assocCfg.dstAddrLst.nAddr[i].u.ipv4NetAddr);
        } else {
            SS7_INFO("\n\t\tdstAddrLst = %x \n", c->assocCfg.dstAddrLst.nAddr[i].u.ipv4NetAddr);
        }
    }

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA PSP %d configuration DONE!\n", id);
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA PSP %d configuration FAILED!\n", id);
        return FTDM_FAIL;
    }

    return FTDM_FAIL;
}

/**********************************************************************************************/
static ftdm_status_t ftmod_m3ua_ps_config(int id)
{
    Pst         pst;
    ItMgmt      cfg;

    int         ret = 0;
    int             i = 0;
    ItPsCfg     *c = NULL;
    sng_m3ua_ps_cfg_t *k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[id];

    smPstInit(&pst);
    pst.dstEnt = ENTIT;

    memset(&cfg, 0x0, sizeof(cfg));
    smHdrInit(&cfg.hdr);

    cfg.hdr.msgType                 = TCFG;
    cfg.hdr.entId.ent               = ENTIT;
    cfg.hdr.entId.inst              = S_INST;
    cfg.hdr.elmId.elmnt             = STITPS;

    c = &cfg.t.cfg.s.psCfg;
    c->psId                         = k->id;
    c->nwkId                        = k->nwkId;
    c->nmbPsp                       = k->pspNum;
    c->routCtx                      = k->rteCtxId;
#ifdef ITASP
    c->lclFlag                      = (k->isLocal)?TRUE:FALSE;
#else
    c->lclFlag                      = FALSE;
#endif

    for (i = 0; i < k->pspNum; i++) {
        c->psp[i] = k->pspIdList[i];
    }

    switch (k->mode) {
        case M3UA_MODE_ACTSTANDBY:
            c->mode = LIT_MODE_ACTSTANDBY;
            break;
        case M3UA_MODE_LOADSHARE:
            c->mode = LIT_MODE_LOADSHARE;
            break;
        default:
            c->mode = LIT_MODE_ACTSTANDBY;
            break;
    }
    c->loadShareMode = LIT_LOADSH_RNDROBIN;
    c->reqAvail = FALSE;			/* If Set to TRUE, SPMC will be active only if PS is active, else SPMC will be marked ACTIVE even if PS is not active */
    c->nmbActPspReqd = 1;

    SS7_INFO("\n\n\t\tM3UA PS %d configuring with: psId = %d, nwkId = %d, nmbPsp = %d, \n"
            "\t\troutCtx = %d, lclFlag = %d, mode = %d, loadShareMode = %d, \n"
            "\t\treqAvail = %d, nmbActPspReqd = %d \n",
            id, c->psId, c->nwkId, c->nmbPsp,
            c->routCtx, c->lclFlag, c->mode, c->loadShareMode,
            c->reqAvail, c->nmbActPspReqd);

    for (i = 0; i < c->nmbPsp; i++) {
        if (i==c->nmbPsp-1) {
            SS7_INFO("\n\t\tpspId = %d \n\n", c->psp[i]);
        } else {
            SS7_INFO("\n\t\tpspId = %d \n", c->psp[i]);
        }
    }

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA PS %d configuration DONE!\n", id);
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA PS %d configuration FAILED!\n", id);
        return FTDM_FAIL;
    }

    return FTDM_FAIL;
}

/**********************************************************************************************/
static ftdm_status_t ftmod_m3ua_route_config(int id, int nsap_id, ftdm_sngss7_operating_modes_e opr_mode)
{
    Pst         pst;
    ItMgmt      cfg;

    int         ret = 0;
    ItRteCfg        *c = NULL;
    sng_m3ua_rte_cfg_t *k = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[id];

    smPstInit(&pst);
    pst.dstEnt = ENTIT;

    memset(&cfg, 0x0, sizeof(cfg));
    smHdrInit(&cfg.hdr);

    cfg.hdr.msgType                 = TCFG;
    cfg.hdr.entId.ent               = ENTIT;
    cfg.hdr.entId.inst              = S_INST;
    cfg.hdr.elmId.elmnt             = STITROUT;

    c = &cfg.t.cfg.s.rteCfg;
    c->nwkId                        = k->nwkId;

    c->rtType                       = k->rtType;


    /* for M3UA SG - we need to fill NSAP as in M3UA sg we will only have remote ps */
    if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
        c->nSapIdPres                   = TRUE;
        c->nSapId                       = nsap_id;
    } else if (FTDM_SUCCESS == ftmod_m3ua_is_local_ps(c->psId)) {
        c->nSapIdPres                   = TRUE;
        c->nSapId                       = nsap_id;
    } else {
        c->nSapIdPres                   = FALSE;
        c->nSapId                       = 0x00;
    }

    c->psId = k->psId;
    c->noStatus = FALSE;
    c->rtFilter.dpc = k->dpc;
    c->rtFilter.dpcMask = k->dpcMask;
    c->rtFilter.opc = k->spc;
    c->rtFilter.sio     = k->sio;
    c->rtFilter.sioMask = k->sioMask;
    c->rtFilter.opcMask = k->spcMask;
    c->rtFilter.slsMask = 0;
    c->rtFilter.sls = 0;
    c->rtFilter.includeCic = FALSE;
    c->rtFilter.cicStart = 0;
    c->rtFilter.cicEnd = 0;
    c->rtFilter.includeSsn = k->includeSsn;
    c->rtFilter.ssn = k->ssn;
    c->rtFilter.includeTrid = FALSE;
    c->rtFilter.tridStart = 0;
    c->rtFilter.tridEnd = 0;

    SS7_INFO("\n\n\t\tM3UA RTE %d configuring with: nwkId = %d, rtType = %d, psId = %d nSapIdPres = %d, \n"
            "\t\tnSapId = %d, noStatus = %d, rtFilter.dpc = %d, rcFilter.dpcMask = %x, rtFilter.opc = %d, rtFilter.opcMask = %x,\n"
            "\t\trtFilter.sls = %d, rtFilter.sio = %d, rtFilter.sioMask = %x, \n"
            "\t\trtFilter.includeCic = %d, rtFilter.cicStart = %d, rtFilter.cicEnd = %d, \n"
            "\t\trtFilter.includeSsn = %d, rtFilter.ssn = %d, rtFilter.includeTrid = %d, rtFilter.tridStart = %d, rtFilter.tridEnd = %d\n\n",
            id, c->nwkId, c->rtType, c->psId, c->nSapIdPres,
            c->nSapId, c->noStatus, c->rtFilter.dpc, c->rtFilter.dpcMask, c->rtFilter.opc, c->rtFilter.opcMask,
            c->rtFilter.sls, c->rtFilter.sio, c->rtFilter.sioMask,
            c->rtFilter.includeCic, c->rtFilter.cicStart, c->rtFilter.cicEnd,
            c->rtFilter.includeSsn, c->rtFilter.ssn, c->rtFilter.includeTrid, c->rtFilter.tridStart, c->rtFilter.tridEnd);

    ret = sng_cfg_m3ua(&pst, &cfg);
    if (ret==0) {
        SS7_INFO("M3UA RTE %d configuration DONE!\n", id);
        return FTDM_SUCCESS;
    } else {
        SS7_CRITICAL("M3UA RTE %d configuration FAILED!\n", id);
        return FTDM_FAIL;
    }
}

/**********************************************************************************************/
int ftmod_m3ua_ssta_req(int elemt, int id, ItMgmt* cfm)
{
    ItMgmt ssta;
    Pst pst;
    sng_m3ua_sap_cfg_t *user = NULL;
    sng_m3ua_nif_cfg_t *nif  = NULL;
    //sng_m3ua_rte_cfg_t *rte  = NULL;
    sng_m3ua_ps_cfg_t  *ps   = NULL;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&ssta, 0, sizeof(ssta));


    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* prepare header */
    ssta.hdr.msgType     = TSSTA;         /* message type */
    ssta.hdr.entId.ent   = ENTIT;          /* entity */
    ssta.hdr.entId.inst  = 0;              /* instance */
    ssta.hdr.elmId.elmnt = elemt; 	      /*STITNSAP, STMWCLUSTER, STMWPEER,STMWSID, STMWDLSAP */
    ssta.hdr.transId     = 1;     /* transaction identifier */

    ssta.hdr.response.selector    = 0;
    ssta.hdr.response.prior       = PRIOR0;
    ssta.hdr.response.route       = RTESPEC;
    ssta.hdr.response.mem.region  = S_REG;
    ssta.hdr.response.mem.pool    = S_POOL;

    switch(ssta.hdr.elmId.elmnt)
    {
        case STITNSAP:
            {
                if (ftmod_ss7_is_operating_mode_pres(SNG_SS7_OPR_MODE_M3UA_SG)) {
                    nif = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[id];
                    ssta.t.ssta.s.nSapSta.lclSapId = nif->m3ua_route_id ;
                } else {
                    user = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[id];
                    ssta.t.ssta.s.nSapSta.lclSapId = user->id;
                }
                break;
            }
        case STITPS:
            {
                ps = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[id];
                ssta.t.ssta.s.psSta.psId = ps->id ;
                break;
            }
        case STITSCTSAP:
            {
                ssta.t.ssta.s.sctSapSta.suId = id ;
                break;
            }
        default:
            break;
    }

    return(sng_sta_m3ua(&pst,&ssta,cfm));
}

/**********************************************************************************************/
void ftdm_m3ua_start_timer(sng_m3ua_tmr_evt_types_e evt_type , int id)
{
    char timer_name[128];
    sng_m3ua_tmr_sched_t  *sched = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sched;

    memset(&timer_name[0],0,sizeof(timer_name));

    sched->tmr_event = evt_type;

    switch(evt_type)
    {
        case SNG_M3UA_TIMER_ASP_UP:
            {
                if (FTDM_FAIL == ftmod_m3ua_is_local_ps(id)) {
                    return;
                }

                strcpy(&timer_name[0],"asp-up-timer");
                sched->id = id;
                break;
            }
        case SNG_M3UA_TIMER_ASP_ACTIVE:
            {
                if (FTDM_FAIL == ftmod_m3ua_is_local_ps(id)) {
                    return;
                }
                strcpy(&timer_name[0],"asp-active-timer");
                sched->id = id;
                break;
            }

        case SNG_M3UA_TIMER_SCTP_ASSOC:
            {
                if (!(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id].flags & SNGSS7_ACTIVE) ||
                        (0x00 == g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id].init_sctp)) {

                    SS7_INFO (" Not starting sctp assoc timer for psp-id[%d] with sctp_init flag[%d] \n",
                              id, g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id].init_sctp);
                    return;
                }
                strcpy(&timer_name[0],"sctp-assoc-timer");
                sched->id = id;
                break;
            }
        default:
            return;
    }

    if (ftdm_sched_timer (sched->tmr_sched,
                &timer_name[0],
                1000, /* time is in ms - starting tmr for 1 sec */
                ftdm_m3ua_handle_tmr_expiry,
                sched,
                &sched->tmr_id)) {
        SS7_ERROR ("Unable to schedule M3UA timer[%s]\n", &timer_name[0]);
    } else {
        SS7_INFO (" Timer[%s] started with timer-id[%d] for 1000 ms for ID[%d] \n",&timer_name[0], sched->tmr_id, id);
    }

    return;
}

/************************************************************************************/
void ftdm_m3ua_handle_tmr_expiry(void *userdata)
{
	sng_m3ua_tmr_sched_t  *sched = (sng_m3ua_tmr_sched_t*)userdata;

	if(NULL == sched) {
		ftdm_log (FTDM_LOG_ERROR ,"Invalid userdata \n");
		return;
	}

	switch(sched->tmr_event)
	{
		case SNG_M3UA_TIMER_ASP_UP:
			{
				ftdm_log (FTDM_LOG_INFO ,"M3UA ASP-UP Timer Expiry for ID[%d] \n", sched->id);

				if (ftmod_m3ua_asp_up(sched->id, AASPUP)) {
					ftdm_log (FTDM_LOG_ERROR ,"ftmod_m3ua_asp_up FAIL  \n");
				} else {
					ftdm_log (FTDM_LOG_INFO ,"ftmod_m3ua_asp_up SUCCESS  \n");
				}
				break;
			}
		case SNG_M3UA_TIMER_ASP_ACTIVE:
            {
                ftdm_log (FTDM_LOG_INFO ,"M3UA ASP-ACTIVE Timer Expiry for ID[%d] \n", sched->id);

                if (ftmod_m3ua_asp_active(sched->id, AASPAC)) {
                    ftdm_log (FTDM_LOG_ERROR ,"ftmod_m3ua_asp_active FAIL  \n");
                } else {
                    ftdm_log (FTDM_LOG_INFO ,"ftmod_m3ua_asp_active SUCCESS  \n");
                }
                break;
            }
		case SNG_M3UA_TIMER_SCTP_ASSOC:
			{
				ftdm_log (FTDM_LOG_INFO ,"M3UA SNG_M3UA_TIMER_SCTP_ASSOC Timer Expiry for SCT sap-id[%d]\n", sched->id);
				ftmod_m3ua_establish_association(sched->id);
				break;
			}
		default:
			return;
	}

	return;
}

/************************************************************************************/
ftdm_status_t ftmod_m3ua_asp_up(int id, int action)
{
    Pst pst;
    ItMgmt cntrl;
    sng_m3ua_psp_cfg_t*       psp = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id];
    int ret = 0x00;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STITPSP;
    cntrl.hdr.transId           = 1;     /* transaction identifier */
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = action;
    cntrl.t.cntrl.subAction     = SAELMNT;

    cntrl.t.cntrl.s.pspId        = psp->id;
    cntrl.t.cntrl.t.aspm.sctSuId = psp->id;

    //cntrl.t.cntrl.t.aspm.info  = ; /* Null terminated string */

    ret =  sng_cntrl_m3ua(&pst, &cntrl);

    if ((LCM_PRIM_OK == ret) || (LCM_PRIM_OK_NDONE == ret )) return FTDM_SUCCESS;

    return ret;
}

/***********************************************************************************************************************/
ftdm_status_t ftmod_m3ua_asp_active(int id, int action)
{
    Pst pst;
    ItMgmt cntrl;
    sng_m3ua_psp_cfg_t*       psp = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[id];
    int ret = 0x00;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STITPSP;
    cntrl.hdr.transId           = 1;     /* transaction identifier */
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = action;
    cntrl.t.cntrl.subAction     = SAELMNT;

    cntrl.t.cntrl.s.pspId        = psp->id;
    cntrl.t.cntrl.t.aspm.sctSuId = psp->id;

    /* auto context - If TRUE , M3UA layer will build routing context list
     * automatically from the local ps configuration */
    cntrl.t.cntrl.t.aspm.autoCtx = TRUE;

    //cntrl.t.cntrl.t.aspm.nmbPs = ; /*  not required if autoCtx=TRUE */
    //cntrl.t.cntrl.t.aspm.psLst = ; /*  not required if autoCtx=TRUE */
    //cntrl.t.cntrl.t.aspm.info  = ; /* Null terminated string */

    ret =  sng_cntrl_m3ua(&pst, &cntrl);

    if ((LCM_PRIM_OK == ret) || (LCM_PRIM_OK_NDONE == ret )) return FTDM_SUCCESS;

    return ret;
}

/***********************************************************************************************************************/
ftdm_status_t ftmod_m3ua_debug(int action)
{
    Pst pst;
    ItMgmt cntrl;

    memset((U8 *)&pst, 0, sizeof(Pst));
    memset((U8 *)&cntrl, 0, sizeof(ItMgmt));

    smPstInit(&pst);

    pst.dstEnt = ENTIT;

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;       /* this is a control request */
    cntrl.hdr.entId.ent         = ENTIT;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STITGEN;
    cntrl.hdr.transId           = 1;     /* transaction identifier */
    cntrl.hdr.response.selector    = 0;
    cntrl.hdr.response.prior       = PRIOR0;
    cntrl.hdr.response.route       = RTESPEC;
    cntrl.hdr.response.mem.region  = S_REG;
    cntrl.hdr.response.mem.pool    = S_POOL;

    cntrl.t.cntrl.action        = action;
    cntrl.t.cntrl.subAction     = SADBG;
    cntrl.t.cntrl.t.dbg.dbgMask	= 0xFFFF;

    return sng_cntrl_m3ua(&pst, &cntrl);
}
/***********************************************************************************************************************/

/******************************************************************************/
/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
/******************************************************************************/
