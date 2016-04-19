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
static ftdm_status_t ftmod_ss7_parse_m3ua_network(ftdm_conf_node_t *node);
static ftdm_status_t ftmod_ss7_parse_m3ua_peerserver(ftdm_conf_node_t *node);
static ftdm_status_t ftmod_ss7_parse_m3ua_psp(ftdm_conf_node_t *node);
static ftdm_status_t ftmod_ss7_parse_m3ua_user(ftdm_conf_node_t *node);
static ftdm_status_t ftmod_ss7_parse_m3ua_route(ftdm_conf_node_t *node, ftdm_sngss7_operating_modes_e opr_mode);
static ftdm_status_t ftmod_ss7_parse_nif(ftdm_conf_node_t *node);
static int ftmod_ss7_m3ua_fill_in_nsap(int nwId, int linkType, int ssf, ftdm_sngss7_operating_modes_e opr_mode);
static int ftmod_ss7_m3ua_fill_mtp3_ip_route(int dpc, int linkType, int switchType, int ssf, int tQuery);

/******************************************************************************************************/
ftdm_status_t ftmod_ss7_parse_m3ua_networks(ftdm_conf_node_t *node)
{
    ftdm_conf_node_t    *node_nwk = NULL;

    if (!node)
        return FTDM_FAIL;

    if (strcasecmp(node->name, "sng_m3ua_network_interfaces")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_network_interfaces>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_network_interfaces> configurations\n");
    }

    for (node_nwk = node->child; node_nwk != NULL; node_nwk = node_nwk->next) {
        if (ftmod_ss7_parse_m3ua_network(node_nwk) != FTDM_SUCCESS) {
            SS7_ERROR("M3UA - Failed to parse <m3ua_nwk>. \n");
            return FTDM_FAIL;
        }
    }

    return FTDM_SUCCESS;

}

/******************************************************************************************************/
static ftdm_status_t ftmod_ss7_parse_m3ua_network(ftdm_conf_node_t *node)
{
	ftdm_conf_parameter_t	*param = NULL;
	int			num_params = 0;
	int 			i=0;
	int 			ret=0;
	sng_m3ua_nwk_cfg_t      t_nwk;

	if (!node)
		return FTDM_FAIL;

	param = node->parameters;
	num_params = node->n_parameters;

	memset(&t_nwk, 0, sizeof(sng_m3ua_nwk_cfg_t));

/******************************************************************************************************/
	if (strcasecmp(node->name, "sng_m3ua_network_interface")) {
		SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_network_interface>!\n", node->name);
		return FTDM_FAIL;
	} else {
		SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> configurations\n");
	}
/******************************************************************************************************/

	for (i=0; i<num_params; i++, param++)
	{
/******************************************************************************************************/
		if (!strcasecmp(param->var, "name")) {
			int n_strlen = strlen(param->val);
			strncpy((char*)t_nwk.name, param->val, (n_strlen>MAX_NAME_LEN)?MAX_NAME_LEN:n_strlen);
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with name = %s\n", param->val);
		}
/******************************************************************************************************/
		else if (!strcasecmp(param->var, "id")) {
/******************************************************************************************************/
			t_nwk.id = atoi(param->val);
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with id = %s\n", param->val);
/******************************************************************************************************/
		} else if (!strcasecmp(param->var, "ssf")) {
/******************************************************************************************************/
			ret = find_ssf_type_in_map(param->val);
			if (-1 == ret) {
				SS7_ERROR("M3UA - wrong ssf type in configuration with value of  %s\n", param->val);
				return FTDM_FAIL;
			}
			t_nwk.ssf = sng_ssf_type_map[ret].tril_type;
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with ssf = %s\n", param->val);
/******************************************************************************************************/
		} else if (!strcasecmp(param->var, "dpc-len")) {
/******************************************************************************************************/
			t_nwk.dpcLen = atoi(param->val);
			if (t_nwk.dpcLen != 14 && t_nwk.dpcLen != 16 && t_nwk.dpcLen != 24 ) {
				SS7_ERROR("M3UA - wrong dpc-len in configuration with value of  %s\n", param->val);
				return FTDM_FAIL;
			}
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with dpcLen = %s\n", param->val);
/******************************************************************************************************/
		} else if (!strcasecmp(param->var, "sls-len")) {
/******************************************************************************************************/
			t_nwk.slsLen = atoi(param->val);
			if (t_nwk.slsLen != 4 && t_nwk.slsLen != 5 && t_nwk.slsLen != 8 ) {
				SS7_ERROR("M3UA - wrong sls-len in configuration with value of  %s\n", param->val);
				return FTDM_FAIL;
			}
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with sls-len = %s\n", param->val);
/******************************************************************************************************/
		} else if (!strcasecmp(param->var, "service-user-switch-type")) {
/******************************************************************************************************/
			ret = find_switch_type_in_map(param->val);
			if (-1 == ret) {
				SS7_ERROR("M3UA - wrong service-user-switch-type in configuration with value of  %s\n", param->val);
				return FTDM_FAIL;
			}
			t_nwk.suSwitchType = sng_switch_type_map[ret].tril_m3ua_su_type;
			t_nwk.isup_switch_type = sng_switch_type_map[ret].tril_isup_type;
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with service-user-switch-type = %d, isup_switch_type[%d]\n"
                        , t_nwk.suSwitchType, t_nwk.isup_switch_type);
/******************************************************************************************************/
		} else if (!strcasecmp(param->var, "user-of-service-user-switch-type")) {
/******************************************************************************************************/
			ret = find_switch_type_in_map(param->val);
			if (-1 == ret) {
				SS7_ERROR("M3UA - wrong su2 switch type in configuration with value of  %s\n", param->val);
				return FTDM_FAIL;
			}
			t_nwk.su2SwitchType = sng_switch_type_map[ret].tril_m3ua_su2_type;
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with user-of-service-user-switch-type = %s\n", param->val);
/******************************************************************************************************/
		} else if (!strcasecmp(param->var, "network-appearance-field")) {
/******************************************************************************************************/
			int n_strlen = strlen(param->val);
			strncpy((char*)t_nwk.appearance, param->val, (n_strlen>MAX_APPEARANCE_SIZE)?MAX_APPEARANCE_SIZE:n_strlen);
			SS7_INFO("M3UA - Parsing <sng_m3ua_network_interface> with network-appearance-field = %s\n", param->val);
/******************************************************************************************************/
		} else {
/******************************************************************************************************/
			SS7_ERROR("M3UA - Found an unknown parameter <%s>\n", param->var);
            return FTDM_FAIL;
		}
/******************************************************************************************************/
	}

	g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].id 			= t_nwk.id;
	g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].ssf 			= t_nwk.ssf;
	g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].dpcLen 		= t_nwk.dpcLen;
	g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].slsLen 			= t_nwk.slsLen;
	g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].suSwitchType 	= t_nwk.suSwitchType;
	g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].su2SwitchType 	= t_nwk.su2SwitchType;
	g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].isup_switch_type 	= t_nwk.isup_switch_type;

	strncpy(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].name, 		t_nwk.name, 		strlen(t_nwk.name) );
	strncpy(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[t_nwk.id].appearance,	t_nwk.appearance, strlen(t_nwk.appearance) );

	return FTDM_SUCCESS;
}

/******************************************************************************************************/
ftdm_status_t ftmod_ss7_parse_m3ua_peerservers(ftdm_conf_node_t *node)
{
    ftdm_conf_node_t    *node_ps = NULL;

    if (!node)
        return FTDM_FAIL;

    if (strcasecmp(node->name, "sng_m3ua_peer_server_interfaces")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_peer_server_interfaces>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_peer_server_interfaces> configurations\n");
    }

    for (node_ps = node->child; node_ps != NULL; node_ps = node_ps->next) {
        if (ftmod_ss7_parse_m3ua_peerserver(node_ps) != FTDM_SUCCESS) {
            SS7_ERROR("M3UA - Failed to parse <sng_m3ua_peer_server_interfaces>. \n");
            return FTDM_FAIL;
        }
    }

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
static ftdm_status_t ftmod_ss7_parse_m3ua_peerserver(ftdm_conf_node_t *node)
{
    ftdm_conf_parameter_t   *param = NULL;
    int                     num_params = 0;
    int                     i=0;
    int                     idx=0;
    sng_m3ua_ps_cfg_t       t_ps;

    if (!node)
        return FTDM_FAIL;

    param       = node->parameters;
    num_params  = node->n_parameters;

    memset(&t_ps, 0, sizeof(sng_m3ua_ps_cfg_t));

    if (strcasecmp(node->name, "sng_m3ua_peer_server_interface")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_peer_server_interface>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_peer_server_interface> configurations\n");
    }

    for (i=0; i<num_params; i++, param++)
    {
/******************************************************************************************************/
        if (!strcasecmp(param->var, "name")) {
/******************************************************************************************************/
            int n_strlen = strlen(param->val);
            strncpy((char*)t_ps.name, param->val, (n_strlen>MAX_NAME_LEN)?MAX_NAME_LEN:n_strlen);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with name = %s\n", param->val);
/******************************************************************************************************/
        } else if (!strcasecmp(param->var, "id")) {
/******************************************************************************************************/
            t_ps.id = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with id = %s\n", param->val);
/******************************************************************************************************/
        } else if (!strcasecmp(param->var, "is-local-ps")) {
/******************************************************************************************************/
            if (!strcasecmp(param->val, "false")) {
                t_ps.isLocal = 0;
            } else {
                t_ps.isLocal = 1;
            }
            SS7_INFO("M3UA - Parsing <m3ua_ps> with isLocal = %s\n", param->val);
/******************************************************************************************************/
        } else if (!strcasecmp(param->var, "mode")) {
/******************************************************************************************************/
            if (!strcasecmp(param->val, "active-standby")) {
                t_ps.mode = M3UA_MODE_ACTSTANDBY;
            } else if (!strcasecmp(param->val, "load-share")) {
                t_ps.mode = M3UA_MODE_LOADSHARE;
            } else {
                SS7_ERROR("M3UA - Invalid mode = %s. Please set to active-standby or load-share. \n", param->val);
                return FTDM_FALSE;
            }
            SS7_INFO("M3UA - Parsing <m3ua_ps> with mode = %s\n", param->val);
/******************************************************************************************************/
        } else if (!strcasecmp(param->var, "network-interface-id")) {
/******************************************************************************************************/
            t_ps.nwkId= atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with nwkId = %s\n", param->val);
/******************************************************************************************************/
        } else if (!strcasecmp(param->var, "peer-server-process-id")) {
/******************************************************************************************************/
            if(t_ps.pspNum < MAX_PSP_PER_PS) {
                t_ps.pspIdList[t_ps.pspNum] = atoi(param->val);
                t_ps.pspNum++;
            }else{
                SS7_ERROR("Psp List excedding max[%d] limit \n", MAX_PSP_PER_PS);
                return FTDM_FAIL;
            }
            SS7_INFO("M3UA - Parsing <m3ua_ps> with psp-id = %s\n", param->val);
/******************************************************************************************************/
        } else if (!strcasecmp(param->var, "route-context-id")) {
/******************************************************************************************************/
            t_ps.rteCtxId= atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with route-context-id = %s\n", param->val);
/******************************************************************************************************/
        } else {
/******************************************************************************************************/
            SS7_ERROR("M3UA - Found an unknown parameter <%s>.\n", param->var);
            return FTDM_FAIL;
        }
/******************************************************************************************************/
    }
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].id                = t_ps.id;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].isLocal           = t_ps.isLocal;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].mode              = t_ps.mode;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].nwkId             = t_ps.nwkId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].rteCtxId          = t_ps.rteCtxId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].pspNum            = t_ps.pspNum;
    for (idx=0; idx<t_ps.pspNum; idx++) {
        g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].pspIdList[idx]    = t_ps.pspIdList[idx];
    }
    strncpy(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[t_ps.id].name,         t_ps.name,      strlen(t_ps.name) );

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
ftdm_status_t ftmod_ss7_parse_m3ua_psps(ftdm_conf_node_t *node)
{
    ftdm_conf_node_t    *node_psp = NULL;

    if (!node)
        return FTDM_FAIL;

    if (strcasecmp(node->name, "sng_m3ua_peer_server_process_interfaces")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_peer_server_process_interfaces>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_peer_server_process_interfaces> configurations\n");
    }

    for (node_psp = node->child; node_psp != NULL; node_psp = node_psp->next) {
        if (ftmod_ss7_parse_m3ua_psp(node_psp) != FTDM_SUCCESS) {
            SS7_ERROR("M3UA - Failed to parse <sng_m3ua_peer_server_process_interfaces>. \n");
            return FTDM_FAIL;
        }
    }

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
static ftdm_status_t ftmod_ss7_parse_m3ua_psp(ftdm_conf_node_t *node)
{
    ftdm_conf_parameter_t   *param = NULL;
    int                     num_params = 0;
    int                     i=0;
    int                     optAddrCount=0;
    sng_m3ua_psp_cfg_t      t_psp;

    if (!node)
        return FTDM_FAIL;

    param       = node->parameters;
    num_params  = node->n_parameters;

    memset(&t_psp, 0, sizeof(sng_m3ua_psp_cfg_t));

    if (strcasecmp(node->name, "sng_m3ua_peer_server_process_interface")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_peer_server_process_interface>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_peer_server_process_interface> configurations\n");
    }

/*******************************************************************************************************/
    for (i=0; i<num_params; i++, param++)
    {
        /*******************************************************************************************************/
        if (!strcasecmp(param->var, "name")) {
            /*******************************************************************************************************/
            int n_strlen = strlen(param->val);
            strncpy((char*)t_psp.name, param->val, (n_strlen>MAX_NAME_LEN)?MAX_NAME_LEN:n_strlen);
            SS7_INFO("M3UA - Parsing <m3ua_psp> with name = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "id")) {
            /*******************************************************************************************************/
            t_psp.id = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_psp> with id = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "psp-type")) {
            /*******************************************************************************************************/
            if (!strcasecmp(param->val, "IPSP")) {
                t_psp.remoteType = LIT_PSPTYPE_IPSP;
            } else if (!strcasecmp(param->val, "SGP")) {
                t_psp.remoteType = LIT_PSPTYPE_SGP;
            } else if (!strcasecmp(param->val, "ASP")) {
                t_psp.remoteType = LIT_PSPTYPE_ASP;
            } else {
                SS7_ERROR("M3UA - Wrong psp type of %s \n", param->val);
                return FTDM_FAIL;
            }
            SS7_INFO("M3UA - Parsing <m3ua_psp> with type = %d\n", t_psp.remoteType);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "ipsp-mode")) {
            /*******************************************************************************************************/
            if (!strcasecmp(param->val, "single")) {
                t_psp.ipspMode = LIT_IPSPMODE_SE;
            } else if (!strcasecmp(param->val, "double")) {
                t_psp.ipspMode = LIT_IPSPMODE_DE;
            } else {
                t_psp.ipspMode = LIT_IPSPMODE_DE;
                SS7_ERROR("M3UA - Wrong psp ipspMode of %s. Set to default double. \n", param->val);
            }
            SS7_INFO("M3UA - Parsing <m3ua_psp> with ispsMode = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "drkm-allowed")) {
            /*******************************************************************************************************/
            if (!strcasecmp(param->val, "true")) {
                t_psp.drkmAllowed = 1;
            } else if (!strcasecmp(param->val, "false")) {
                t_psp.drkmAllowed = 0;
            } else {
                t_psp.drkmAllowed = 0;
                SS7_ERROR("M3UA - Wrong psp drkmAllowed of %s. Set to default false. \n", param->val);
            }
            SS7_INFO("M3UA - Parsing <m3ua_psp> with drkmAllowed = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "include-network-appearance")) {
            /*******************************************************************************************************/
            if (!strcasecmp(param->val, "true")) {
                t_psp.nwkAppearanceIncl = 1;
            } else if (!strcasecmp(param->val, "false")) {
                t_psp.nwkAppearanceIncl = 0;
            } else {
                t_psp.nwkAppearanceIncl = 1;
                SS7_ERROR("M3UA - Wrong psp nwkAppearanceIncl of %s. Set to default true. \n", param->val);
            }
            SS7_INFO("M3UA - Parsing <m3ua_psp> with nwkAppearanceIncl = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "tx-rx-aspid")) {
            /*******************************************************************************************************/
            if (!strcasecmp(param->val, "tx")) {
                t_psp.txRxSpId = M3UA_ASPUP_SPID_TX;
            } else if (!strcasecmp(param->val, "rx")) {
                t_psp.txRxSpId = M3UA_ASPUP_SPID_RX;
            } else if (!strcasecmp(param->val, "both")) {
                t_psp.txRxSpId = M3UA_ASPUP_SPID_BOTH;
            } else if (!strcasecmp(param->val, "none")) {
                t_psp.txRxSpId = M3UA_ASPUP_SPID_NONE;
            }else {
                t_psp.txRxSpId = M3UA_ASPUP_SPID_BOTH;
                SS7_ERROR("M3UA - Wrong psp txrxspId of %s. Set to default both. \n", param->val);
            }
            SS7_INFO("M3UA - Parsing <m3ua_psp> with txrxspId = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "self-asp-identifier")) {
            /*******************************************************************************************************/
            t_psp.selfSpId= atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_psp> with selfAspId = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "network-interface-id")) {
            /*******************************************************************************************************/
            t_psp.nwkId = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_psp> with nwkId = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "primary-destination-address")) {
            /*******************************************************************************************************/
            t_psp.primDestAddr = iptoul (param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with primeDestAddr = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "address")) {
            /*******************************************************************************************************/
            if (optAddrCount < SCT_MAX_NET_ADDRS-1) {
                t_psp.optDestAddr[optAddrCount] = iptoul (param->val);
                optAddrCount++;
                SS7_INFO("M3UA - Parsing <m3ua_ps> with optDestAddr = %s\n", param->val);
            } else {
                SS7_ERROR("M3UA - Parsing <m3ua_ps> with optDestAddr = %s.  "
                        "Exceeding maximum optional destination address number. Dropped. %d \n", param->val, optAddrCount);
            }
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "destination-port")) {
            /*******************************************************************************************************/
            t_psp.destPort = atoi (param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with destPort = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "number-of-outgoing-streams")) {
            /*******************************************************************************************************/
            t_psp.streamNum = atoi (param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with outStreamNum = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "clientSide")) {
            /*******************************************************************************************************/
            if (!strcasecmp(param->val, "true")) {
                t_psp.isClientSide = 1;
            } else if (!strcasecmp(param->val, "false")) {
                t_psp.isClientSide = 0;
            } else {
                t_psp.isClientSide = 0;
                SS7_ERROR("M3UA - Wrong psp clientSide of %s. Set to default true. \n", param->val);
            }
            SS7_INFO("M3UA - Parsing <m3ua_psp> with clientSide = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "sctp-type-of-service")) {
            /*******************************************************************************************************/
            t_psp.tos = atoi (param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with tos = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "sctp-interface-id")) {
            /*******************************************************************************************************/
            t_psp.sctpId = atoi (param->val);
            SS7_INFO("M3UA - Parsing <m3ua_ps> with sctp-id = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "init-sctp-association")) {
            /*******************************************************************************************************/
            if (!strcasecmp(param->val, "true")) {
                t_psp.init_sctp = 1;
            } else if (!strcasecmp(param->val, "false")) {
                t_psp.init_sctp = 0;
            } else {
                t_psp.init_sctp = 0;
            }
            SS7_INFO("M3UA - Parsing <m3ua_ps> with init_sctp = %s\n", param->val);
            /*******************************************************************************************************/
        } else {
            /*******************************************************************************************************/
            SS7_ERROR("M3UA - Found an unknown parameter <%s>. \n", param->var);
            return FTDM_FAIL;
        }
        /*******************************************************************************************************/
    }

    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].id                  = t_psp.id;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].remoteType          = t_psp.remoteType;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].ipspMode            = t_psp.ipspMode;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].drkmAllowed         = t_psp.drkmAllowed;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].nwkAppearanceIncl   = t_psp.nwkAppearanceIncl;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].txRxSpId            = t_psp.txRxSpId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].selfSpId            = t_psp.selfSpId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].nwkId               = t_psp.nwkId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].primDestAddr        = t_psp.primDestAddr;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].destPort                = t_psp.destPort;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].streamNum           = t_psp.streamNum;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].isClientSide        = t_psp.isClientSide;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].tos                 = t_psp.tos;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].sctpId              = t_psp.sctpId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].init_sctp           = t_psp.init_sctp;
    strncpy(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].name, t_psp.name, strlen(t_psp.name) );
    for (i=0; i<optAddrCount-1; i++) {
        g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].optDestAddr[i]  = t_psp.optDestAddr[i];
    }
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].optDestAddr[i]      = t_psp.primDestAddr;

    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.pspCfg[t_psp.id].numOptDestAddr      = optAddrCount;

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
ftdm_status_t ftmod_ss7_parse_m3ua_users(ftdm_conf_node_t *node)
{
    ftdm_conf_node_t    *node_sap = NULL;

    if (!node)
        return FTDM_FAIL;

    if (strcasecmp(node->name, "sng_m3ua_user_interfaces")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_user_interfaces>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_user_interfaces> configurations\n");
    }

    for (node_sap = node->child; node_sap != NULL; node_sap = node_sap->next) {
        if (ftmod_ss7_parse_m3ua_user(node_sap) != FTDM_SUCCESS) {
            SS7_ERROR("M3UA - Failed to parse <m3ua_saps>. \n");
            return FTDM_FAIL;
        }
    }

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
static ftdm_status_t ftmod_ss7_parse_m3ua_user(ftdm_conf_node_t *node)
{
    ftdm_conf_parameter_t   *param = NULL;
    int                     num_params = 0;
    int                     i=0;
    sng_m3ua_sap_cfg_t      t_sap;

    if (!node)
        return FTDM_FAIL;

    param = node->parameters;
    num_params = node->n_parameters;

    memset(&t_sap, 0, sizeof(sng_m3ua_sap_cfg_t));

    if (strcasecmp(node->name, "sng_m3ua_user_interface")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_user_interface>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_user_interface> configurations\n");
    }

    for (i=0; i<num_params; i++, param++)
    {
        /******************************************************************************************************/
        if (!strcasecmp(param->var, "name")) {
            /******************************************************************************************************/
            int n_strlen = strlen(param->val);
            strncpy((char*)t_sap.name, param->val, (n_strlen>MAX_NAME_LEN)?MAX_NAME_LEN:n_strlen);
            SS7_INFO("M3UA - Parsing <m3ua_sap> with name = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "id")) {
            /******************************************************************************************************/
            t_sap.id = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_sap> with id = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "network-interface-id")) {
            /******************************************************************************************************/
            t_sap.nwkId = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_sap> with nwkId = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "service-user-type")) {
            /******************************************************************************************************/
            /* cm/lit.h header file has all possible values */
            if (!strcasecmp(param->val, "SCCP") ) {
                t_sap.suType = LIT_SU_SCCP;
            } else if (!strcasecmp(param->val, "MTP3") ) {
                t_sap.suType = LIT_SP_MTP3;
            } else if (!strcasecmp(param->val, "ISUP") ) {
                t_sap.suType = LIT_SU_ISUP;
            } else if (!strcasecmp(param->val, "B_ISUP") ) {
                t_sap.suType = LIT_SU_B_ISUP;
            } else {
                SS7_CRITICAL("M3UA - SU Type of %s is not supported now\n", param->val);
                return FTDM_FAIL;
            }
            SS7_INFO("M3UA - Parsing <m3ua_sap> with suType = %s [%d]\n", param->val, t_sap.suType);
            /******************************************************************************************************/
        } else {
            /******************************************************************************************************/
            SS7_ERROR("M3UA - Found an unknown parameter <%s>. \n", param->var);
            return FTDM_FAIL;
        }
        /******************************************************************************************************/
    }

    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[t_sap.id].id              = t_sap.id;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[t_sap.id].nwkId           = t_sap.nwkId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[t_sap.id].suType          = t_sap.suType;
    strncpy(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.sapCfg[t_sap.id].name, t_sap.name, strlen(t_sap.name) );

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
ftdm_status_t ftmod_ss7_parse_m3ua_routes(ftdm_conf_node_t *node, int opr_mode)
{
    ftdm_conf_node_t    *node_psp = NULL;

    if (!node)
        return FTDM_FAIL;

    if (strcasecmp(node->name, "sng_m3ua_route_interfaces")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_route_interfaces>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_route_interfaces> configurations\n");
    }

    for (node_psp = node->child; node_psp != NULL; node_psp = node_psp->next) {
        if (ftmod_ss7_parse_m3ua_route(node_psp, opr_mode) != FTDM_SUCCESS) {
            SS7_ERROR("M3UA - Failed to parse <m3ua_routes>. \n");
            return FTDM_FAIL;
        }
    }

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
static ftdm_status_t ftmod_ss7_parse_m3ua_route(ftdm_conf_node_t *node, ftdm_sngss7_operating_modes_e opr_mode)
{
    ftdm_conf_parameter_t   *param = NULL;
    int                     num_params = 0;
    int                     i=0;
    sng_m3ua_rte_cfg_t      t_rte;

    if (!node)
        return FTDM_FAIL;

    param = node->parameters;
    num_params = node->n_parameters;

    memset(&t_rte, 0, sizeof(sng_m3ua_rte_cfg_t));

    if (strcasecmp(node->name, "sng_m3ua_route_interface")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <m3ua_route>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <m3ua_route> configurations\n");
    }

    /* By default setting both these values to 0 */
    t_rte.includeSsn = 0;
    t_rte.ssn 	     = 0;
    for (i=0; i<num_params; i++, param++)
    {
        /******************************************************************************************************/
        if (!strcasecmp(param->var, "name")) {
            /******************************************************************************************************/
            int n_strlen = strlen(param->val);
            strncpy((char*)t_rte.name, param->val, (n_strlen>MAX_NAME_LEN)?MAX_NAME_LEN:n_strlen);
            SS7_INFO("M3UA - Parsing <m3ua_route> with name = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "id")) {
            /******************************************************************************************************/
            t_rte.id = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_route> with id = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "dpc")) {
            /******************************************************************************************************/
            t_rte.dpc = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_route> with dpc = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "dpc-mask")) {
            /******************************************************************************************************/
            sscanf(param->val, "%x", &t_rte.dpcMask);
            SS7_INFO("M3UA - Parsing <m3ua_route> with dpcMask = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "spc")) {
            /******************************************************************************************************/
            t_rte.spc = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_route> with spc = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "spc-mask")) {
            /******************************************************************************************************/
            sscanf(param->val, "%x", &t_rte.spcMask);
            SS7_INFO("M3UA - Parsing <m3ua_route> with spcMask = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "service-identifier-octet")) {
            /******************************************************************************************************/
            if (!strcasecmp(param->val, "ISUP")) {
                t_rte.sio = 5;     /* SI_ISUP */
            } else if (!strcasecmp(param->val, "SCCP")) {
                t_rte.sio = 3;    /* SI_SCCP */
            } else if (!strcasecmp(param->val, "TUP")) {
                t_rte.sio = 4;   /* SI_TUP */
            } else {
                t_rte.sio = 5;   /* SI_ISUP */
                SS7_ERROR("M3UA - Wrong SIO of %s. Set to default ISUP. \n", param->val);
            }
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "sio-mask")) {
            /******************************************************************************************************/
            sscanf(param->val, "%x", &t_rte.sioMask);
            SS7_INFO("M3UA - Parsing <m3ua_route> with sioMask = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "network-interface-id")) {
            /******************************************************************************************************/
            t_rte.nwkId = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_route> with nwkId = %s\n", param->val);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "route-type")) {
            /******************************************************************************************************/
            if (!strcasecmp(param->val,"MTP3")) {
                t_rte.rtType = LIT_RTTYPE_MTP3;
            } else if (!strcasecmp(param->val,"PS")) {
                t_rte.rtType = LIT_RTTYPE_PS;
            } else if (!strcasecmp(param->val,"LOCAL")) {
                t_rte.rtType = LIT_RTTYPE_LOCAL;
            }
            SS7_INFO("M3UA - Parsing <m3ua_route> with rtType = %d\n", t_rte.rtType);
            /******************************************************************************************************/
        } else if (!strcasecmp(param->var, "peer-server-interface-id")) {
            /******************************************************************************************************/
            t_rte.psId = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_route> with psId = %s\n", param->val);
            /******************************************************************************************************/
	} else if (!strcasecmp(param->var, "include-ssn")) {
            /******************************************************************************************************/
	    if (ftdm_true(param->val)) {
		t_rte.includeSsn = 1;
	    } else {
		t_rte.includeSsn = 0;
	    }

            SS7_INFO("M3UA - Parsing <m3ua_route> with includeSsn = %s\n", param->val);
            /******************************************************************************************************/
	} else if (!strcasecmp(param->var, "ssn")) {
            /******************************************************************************************************/
            t_rte.ssn = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_route> with ssn = %s\n", param->val);
            /******************************************************************************************************/
        } else {
            /******************************************************************************************************/
            SS7_ERROR("M3UA - Found an unknown parameter <%s>. \n", param->var);
            return FTDM_FAIL;
        }
        /******************************************************************************************************/
    }

    ftmod_ss7_fill_m3ua_route(&t_rte, M3UA_ROUTE_IP);

    if (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) {
	    ftmod_ss7_m3ua_fill_in_nsap(t_rte.nwkId, 0, 0, opr_mode);
    }

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
ftdm_status_t ftmod_ss7_fill_m3ua_route(sng_m3ua_rte_cfg_t *t_rte, sng_m3ua_route_type_t type)
{
    int                     idx    = 1;
    int                     rttype = LIT_RTTYPE_NOTFOUND;
    char                    name[MAX_NAME_LEN];

    memset(&name[0], 0, sizeof(name));

    switch(type)
    {
        case M3UA_ROUTE_SELF:
            rttype  = LIT_RTTYPE_LOCAL;
            strcpy(name, "self-route");
            break;
        case M3UA_ROUTE_MTP3:
            rttype  = LIT_RTTYPE_MTP3;
            strcpy(name, "mtp3-route");
            break;
        case M3UA_ROUTE_IP:
            rttype  = LIT_RTTYPE_PS;
            strncpy(name, t_rte->name, strlen(t_rte->name));
            break;
        default:
            SS7_ERROR("M3UA - unknown route type [%d] \n", type);
            return FTDM_FAIL;
    }

    /* check id the route is already being configured */
    while (idx < (MAX_M3UA_RTE)) {
        if (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].m3ua_rte_id == t_rte->id) &&
		    (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].dpc == t_rte->dpc) &&
		    (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].spc == t_rte->spc) &&
		    (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].sio == t_rte->sio) &&
		    (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].nwkId == t_rte->nwkId) &&
		    (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].psId == t_rte->psId) &&
		    (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].rtType == rttype)) {
			SS7_DEBUG("Found an existing M3UA Route: id=%d, name=%s (old name=%s)\n",
				  t_rte->id, t_rte->name, g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].name);
			break;
		}
	}
	/* Increase the counter and search for exact match */
	idx++;
    }

    if (idx == MAX_M3UA_RTE) {
	    idx = 1;
	    /* find first free place */
	    while (idx < (MAX_M3UA_RTE)) {
		    if (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].id == 0) {
			    /* we have a match so break out of this loop */
			    SS7_DEBUG("Found new M3UA Route: id=%d, name=%s at index=%d\n", t_rte->id, t_rte->name, idx);
			    break;
		    }
		    /* move on to the next one */
		    idx++;
	    }
    }

    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].id          = idx;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].m3ua_rte_id = t_rte->id;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].dpc         = t_rte->dpc;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].dpcMask     = t_rte->dpcMask;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].spc         = t_rte->spc;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].spcMask     = t_rte->spcMask;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].sio         = t_rte->sio;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].sioMask     = t_rte->sioMask;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].nwkId       = t_rte->nwkId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].psId        = t_rte->psId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].nsapId      = t_rte->nsapId;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].rtType      = rttype;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].includeSsn  = t_rte->includeSsn;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].ssn 	      = t_rte->ssn;
    strncpy(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[idx].name, name, strlen(name) );

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
ftdm_status_t ftmod_ss7_parse_m3ua_nif(ftdm_conf_node_t *node)
{
    ftdm_conf_node_t    *node_nif = NULL;

    if (!node)
        return FTDM_FAIL;

    if (strcasecmp(node->name, "sng_m3ua_nif_interfaces")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_nif_interfaces>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_nif_interface> configurations\n");
    }

    for (node_nif = node->child; node_nif != NULL; node_nif = node_nif->next) {
        if (ftmod_ss7_parse_nif(node_nif) != FTDM_SUCCESS) {
            SS7_ERROR("M3UA - Failed to parse <sng_m3ua_nif_interface>. \n");
            return FTDM_FAIL;
        }
    }

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
static ftdm_status_t ftmod_ss7_parse_nif(ftdm_conf_node_t *node)
{
    ftdm_conf_parameter_t   *param = NULL;
    int                     num_params = 0;
    int                     i=0;
    sng_m3ua_nif_cfg_t      nif;

    if (!node)
        return FTDM_FAIL;

    param       = node->parameters;
    num_params  = node->n_parameters;

    memset(&nif, 0, sizeof(sng_m3ua_nif_cfg_t));

    if (strcasecmp(node->name, "sng_m3ua_nif_interface")) {
        SS7_ERROR("M3UA - We're looking at <%s>...but we're supposed to be looking at <sng_m3ua_nif_interface>!\n", node->name);
        return FTDM_FAIL;
    } else {
        SS7_INFO("M3UA - Parsing <sng_m3ua_nif_interface> configurations\n");
    }

/*******************************************************************************************************/
    for (i=0; i<num_params; i++, param++)
    {
        /*******************************************************************************************************/
        if (!strcasecmp(param->var, "name")) {
            /*******************************************************************************************************/
            int n_strlen = strlen(param->val);
            strncpy((char*)nif.name, param->val, (n_strlen>MAX_NAME_LEN)?MAX_NAME_LEN:n_strlen);
            SS7_INFO("M3UA - Parsing <m3ua_nif> with name = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "id")) {
            /*******************************************************************************************************/
            nif.id = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_nif> with id = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "m3ua-route-interface-id")) {
            /*******************************************************************************************************/
            nif.m3ua_route_id = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_nif> with m3ua-route-id = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "mtp3-route-interface-id")) {
            /*******************************************************************************************************/
            nif.mtp3_route_id = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_nif> with mtp3-route-id = %s\n", param->val);
            /*******************************************************************************************************/
        } else if (!strcasecmp(param->var, "network-interface-id")) {
            /*******************************************************************************************************/
            nif.nwkId = atoi(param->val);
            SS7_INFO("M3UA - Parsing <m3ua_nif> with network-interface-id = %s\n", param->val);
            /*******************************************************************************************************/
        } else {
            /*******************************************************************************************************/
            SS7_ERROR("M3UA - Found an unknown parameter <%s>. \n", param->var);
            return FTDM_FAIL;
        }
        /*******************************************************************************************************/
    }

    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[nif.id].id                        = nif.id;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[nif.id].m3ua_route_id             = nif.m3ua_route_id;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[nif.id].mtp3_route_id             = nif.mtp3_route_id;
    g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[nif.id].nwkId 		       = nif.nwkId;
    strncpy(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[nif.id].name, nif.name, strlen(nif.name) );

    return FTDM_SUCCESS;
}

/******************************************************************************************************/
static int ftmod_ss7_m3ua_fill_in_nsap(int nwId, int linkType, int ssf, ftdm_sngss7_operating_modes_e opr_mode)
{
	sng_m3ua_nwk_cfg_t *nwk_cfg = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nwkCfg[nwId];
	ftdm_status_t ret = FTDM_FAIL;
	int i = 1;

	/* get the network configuration structure based on nwId */
	if (!nwk_cfg) {
		SS7_ERROR("No network configuration found for network ID %d!\n",
			  nwId);
		goto done;
	}

	if (!nwk_cfg->id) {
		SS7_ERROR ("Invalid network configuration present at id %d!\n",
			   nwId);
		goto done;
	}

	/* validate if the respective ssf is same as that of present at its
	 * respective network configuration */
	if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
		/* NOTE: we must not compare on basis of switchType as it is possible
		 * MTP3 and M3UA have same switch type but the same are mapped to two
		 * different entities */
		if ((nwk_cfg->id != nwId) &&  (nwk_cfg->ssf != ssf)) {
			SS7_ERROR("Invalid ssf [m3ua-%d and mtp-%d] found!\n",
				  nwk_cfg->ssf, ssf);
			goto done;
		}
	}

	/* go through all the existing interfaces and see if we find a match */
	while ((i < MAX_NSAPS) && g_ftdm_sngss7_data.cfg.nsap[i].id != 0) {
		    /* NOTE: In MTP3 currently we are filling switch type for ISUP,
		     * hence we need to compare with MTP3 switchType Vs M3UA ISUP Switch type */
		    if ((g_ftdm_sngss7_data.cfg.nsap[i].switchType == nwk_cfg->isup_switch_type) &&
		    (g_ftdm_sngss7_data.cfg.nsap[i].ssf == nwk_cfg->ssf) &&
		    /* KAPIL: for this needs to be taken care as we should and must not create nsap for MTP3 when it is already
		     * present I donot think this is any how required for M3UA SG */
		    //(g_ftdm_sngss7_data.cfg.nsap[i].nwId == nwId)) { TODO : MTP3 always hardcoding nwId to 0..need to improve this..
		    (g_ftdm_sngss7_data.cfg.nsap[i].id == nwId)) {
			if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
				if (g_ftdm_sngss7_data.cfg.nsap[i].linkType == linkType) {
					/* we have a match so break out of this loop */
					break;
				}
			} else if (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) {
					/* we have a match so break out of this loop */
					break;
			}
		}
		/* move on to the next one */
		i++;
	}

	if (g_ftdm_sngss7_data.cfg.nsap[i].id == 0) {
		g_ftdm_sngss7_data.cfg.nsap[i].id = i;
		SS7_DEBUG("found new nsap interface, id is = %d\n", g_ftdm_sngss7_data.cfg.nsap[i].id);
	} else {
		g_ftdm_sngss7_data.cfg.nsap[i].id = i;
		SS7_DEBUG("found existing nsap interface, id is = %d\n", g_ftdm_sngss7_data.cfg.nsap[i].id);
	}
	g_ftdm_sngss7_data.cfg.nsap[i].spId		= g_ftdm_sngss7_data.cfg.nsap[i].id;
	g_ftdm_sngss7_data.cfg.nsap[i].suId		= g_ftdm_sngss7_data.cfg.nsap[i].id;
	g_ftdm_sngss7_data.cfg.nsap[i].nwId		= nwId;
	g_ftdm_sngss7_data.cfg.nsap[i].switchType	= nwk_cfg->isup_switch_type;
	g_ftdm_sngss7_data.cfg.nsap[i].ssf		= nwk_cfg->ssf;

	/* linkType is required only in case of when M3UA is configured in
	 * SG mode */
	if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
		g_ftdm_sngss7_data.cfg.nsap[i].linkType		= linkType;
	}

	ret = FTDM_SUCCESS;
done:
	return ret;
}

/******************************************************************************************************/
static int ftmod_ss7_m3ua_fill_mtp3_ip_route(int dpc, int linkType, int switchType, int ssf, int tQuery)
{
    int i = MTP_IP_ROUTES_START_IDX;

    while (i < (MAX_MTP_ROUTES)) {
        if ((g_ftdm_sngss7_data.cfg.mtpRoute[i].dpc == dpc) &&
                (g_ftdm_sngss7_data.cfg.mtpRoute[i].dir == SNG_RTE_IP)){
            /* we have a match so break out of this loop */
            break;
        }
        /* move on to the next one */
        i++;
    }

    if (g_ftdm_sngss7_data.cfg.mtpRoute[i].id == 0) {
        /* this is a new route...find the first free spot */
        i = MTP_IP_ROUTES_START_IDX;
        while (i < (MAX_MTP_ROUTES)) {
            if (g_ftdm_sngss7_data.cfg.mtpRoute[i].id == 0) {
                /* we have a match so break out of this loop */
                break;
            }
            /* move on to the next one */
            i++;
        }
        g_ftdm_sngss7_data.cfg.mtpRoute[i].id = i;
        SS7_DEBUG("found new mtp3 ip route\n");
    } else {
        g_ftdm_sngss7_data.cfg.mtpRoute[i].id = i;
        SS7_DEBUG("found existing mtp3 ip route\n");
    }

    strncpy((char *)g_ftdm_sngss7_data.cfg.mtpRoute[i].name, "ip-route", MAX_NAME_LEN-1);

    g_ftdm_sngss7_data.cfg.mtpRoute[i].id			= i;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].dpc			= dpc;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].linkType		= linkType;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].switchType	= switchType;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].cmbLinkSetId	= i;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].isSTP		= 0;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].ssf			= ssf;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].dir			= SNG_RTE_IP;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t6			= 8;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t8			= 12;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t10			= 300;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t11			= 300;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t15			= 30;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t16			= 20;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t18			= 200;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t19			= 690;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t21			= 650;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].t25			= 100;
    g_ftdm_sngss7_data.cfg.mtpRoute[i].tQuery			= tQuery;

    return 0;
}

/******************************************************************************************************/
int ftmod_m3ua_cfg_mtp3_m3ua_routes(int opr_mode)
{
    int idx = 0;
    int mtp3_rte_idx = 0;
    int m3ua_rte_idx = 0;
    sng_m3ua_rte_cfg_t m3ua_rte;
    //sng_route_t        mtp3_rte;

    /* for every mtp3 route we need to have m3ua routes and vice-versa if M3UA is configured
     * in SG mode  else only fill its respective MTP3 NSAP value */

    /* go through all NIFs and configure mtp3/m3ua (dependent to each other) routes */
    idx = 1;
    while (idx < MAX_M3UA_RTE) {

        if (g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].id !=0) {

          /*************************************************************************************************************/
            mtp3_rte_idx = g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].mtp3_route_id;
            m3ua_rte_idx = g_ftdm_sngss7_data.cfg.g_m3ua_cfg.nifCfg[idx].m3ua_route_id;

          /*************************************************************************************************************/
            /* m3ua routes needs other fields values also, so we need to match mtp3-id with nif configurations
             * and then pick the associated m3ua routes.. we can then re-use same associated m3ua routes values */
            /* first configure dependent m3ua routes for mtp3 routes */
            /* copy mtp3 route associated m3ua routes */
            memset(&m3ua_rte, 0, sizeof(m3ua_rte));
            memcpy(&m3ua_rte, &g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[m3ua_rte_idx], sizeof(m3ua_rte));

	    /*************************************************************************************************************/
	    /* Fill in mtp3_m3ua route Operating mode is M3UA SG */
	    if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
		    /* now change required values */
		    m3ua_rte.dpc     = g_ftdm_sngss7_data.cfg.mtpRoute[mtp3_rte_idx].dpc ;
		    m3ua_rte.dpcMask = LIT_DPC_SPECIFIED  ; /* wild card - pass all data from m3ua to mtp3 */
		    m3ua_rte.spcMask = 0x00  ;              /* wild card - pass all data from m3ua to mtp3 */
		    m3ua_rte.sioMask = 0x00  ;              /* wild card - pass all data from m3ua to mtp3 */

		    ftmod_ss7_fill_m3ua_route(&m3ua_rte, M3UA_ROUTE_MTP3);
		    /*************************************************************************************************************/
		    /* now configure MTP3 routes dependent on M3UA routes */

		    ftmod_ss7_m3ua_fill_mtp3_ip_route(g_ftdm_sngss7_data.cfg.g_m3ua_cfg.rteCfg[m3ua_rte_idx].dpc,
				    g_ftdm_sngss7_data.cfg.mtpRoute[mtp3_rte_idx].linkType,
				    g_ftdm_sngss7_data.cfg.mtpRoute[mtp3_rte_idx].switchType,
				    g_ftdm_sngss7_data.cfg.mtpRoute[mtp3_rte_idx].ssf,
				    g_ftdm_sngss7_data.cfg.mtpRoute[mtp3_rte_idx].tQuery
				    );

		    /*************************************************************************************************************/
	    }
	    /*************************************************************************************************************/

          /*************************************************************************************************************/

	    /* Also, create/fill its respective nsap informtaion if not present already in nsap list */
	    if (ftmod_ss7_m3ua_fill_in_nsap(m3ua_rte.nwkId,
					    g_ftdm_sngss7_data.cfg.mtpRoute[mtp3_rte_idx].linkType,
					    g_ftdm_sngss7_data.cfg.mtpRoute[mtp3_rte_idx].ssf,
					    opr_mode)) {
		    return 0x01;
	    }

          /*************************************************************************************************************/
        }
        idx++;
    }

    return 0x00;
}

/******************************************************************************************************/
ftdm_status_t ftmod_m3ua_is_local_ps(int psid)
{
    if (0x01 == g_ftdm_sngss7_data.cfg.g_m3ua_cfg.psCfg[psid].isLocal) {
        return FTDM_SUCCESS;
    }
    return FTDM_FAIL;
}
/******************************************************************************************************/

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
