/*
 * Copyright (c) 2009, Konrad Hammel <konrad@sangoma.com>
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
 *
 * James Zhang <jzhang@sangoma.com>
 *
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/

/* GLOBALS ********************************************************************/
sng_ssf_type_t sng_ssf_type_map[] =
{
	{ 1, "nat"   , SSF_NAT   },
	{ 1, "int"   , SSF_INTL  },
	{ 1, "spare" , SSF_SPARE },
	{ 1, "res"   , SSF_RES   },
	{ 0, "", 0 },
};

sng_switch_type_t sng_switch_type_map[] =
{
	{ 1, "itu88"  , LSI_SW_ITU     , LSI_SW_ITU     },
	{ 1, "itu92"  , LSI_SW_ITU     , LSI_SW_ITU     },
	{ 1, "itu97"  , LSI_SW_ITU97   , LSI_SW_ITU97   },
	{ 1, "itu00"  , LSI_SW_ITU2000 , LSI_SW_ITU2000 },
	{ 1, "ansi88" , LSI_SW_ANS88   , LSI_SW_ANS88   },
	{ 1, "ansi92" , LSI_SW_ANS92   , LSI_SW_ANS92   },
	{ 1, "ansi95" , LSI_SW_ANS92   , LSI_SW_ANS92   },
	{ 1, "etsiv2" , LSI_SW_ETSI    , LSI_SW_ETSI    },
	{ 1, "etsiv3" , LSI_SW_ETSIV3  , LSI_SW_ETSIV3  },
	{ 1, "india"  , LSI_SW_INDIA   , LSI_SW_INDIA   },
	{ 1, "uk"     , LSI_SW_UK      , LSI_SW_UK      },
	{ 1, "russia" , LSI_SW_RUSSIA  , LSI_SW_RUSSIA  },
	{ 1, "china"  , LSI_SW_CHINA   , LSI_SW_CHINA   },
	{ 0, "", 0, 0 },
};

sng_link_type_t sng_link_type_map[] =
{
	{ 1, "itu88"  , LSD_SW_ITU88 , LSN_SW_ITU   },
	{ 1, "itu92"  , LSD_SW_ITU92 , LSN_SW_ITU   },
	{ 1, "etsi"   , LSD_SW_ITU92 , LSN_SW_ITU   },
	{ 1, "ansi88" , LSD_SW_ANSI88, LSN_SW_ANS   },
	{ 1, "ansi92" , LSD_SW_ANSI92, LSN_SW_ANS   },
	{ 1, "ansi96" , LSD_SW_ANSI92, LSN_SW_ANS96 },
	{ 1, "china"  , LSD_SW_CHINA , LSN_SW_CHINA },
	{ 0, "", 0, 0 },
};

sng_mtp2_error_type_t sng_mtp2_error_type_map[] =
{
	{ 1, "basic", SD_ERR_NRM },
	{ 1, "pcr"  , SD_ERR_CYC },
	{ 0, "", 0 },
};

sng_cic_cntrl_type_t sng_cic_cntrl_type_map[] =
{
	{ 1, "bothway"     , BOTHWAY     },
	{ 1, "incoming"    , INCOMING    },
	{ 1, "outgoing"    , OUTGOING    },
	{ 1, "controlling" , CONTROLLING },
	{ 1, "controlled"  , CONTROLLED  },
	{ 0, "", 0 },
};

typedef struct sng_timeslot
{
	int	 channel;
	int	 siglink;
	int	 gap;
	int	 hole;
	int 	 transparent;
}sng_timeslot_t;

typedef struct sng_span
{
	char			name[MAX_NAME_LEN];
	ftdm_span_t		*span;
	uint32_t		ccSpanId;
} sng_span_t;

typedef struct sng_ccSpan
{
	char			name[MAX_NAME_LEN];
	uint32_t		options;
	uint32_t		id;
	uint32_t		procId;
	uint32_t		isupInf;
	uint32_t		cicbase;
	char			ch_map[MAX_CIC_MAP_LENGTH];
	uint32_t		typeCntrl;
	uint32_t		switchType;
	uint32_t		ssf;
	uint32_t		clg_nadi;
	uint32_t		cld_nadi;
	uint32_t		rdnis_nadi;
	uint32_t		loc_nadi;
	uint32_t		min_digits;
	uint32_t		transparent_iam_max_size;
	uint8_t			transparent_iam;
	uint8_t         	cpg_on_progress_media;
	uint8_t         	cpg_on_progress;
	uint8_t         	cpg_on_alert;
	uint8_t         	ignore_alert_on_cpg;
	uint8_t			itx_auto_reply;
	uint8_t			bearcap_check;
	uint32_t		t3;
	uint32_t		t10;
	uint32_t		t12;
	uint32_t		t13;
	uint32_t		t14;
	uint32_t		t15;
	uint32_t		t16;
	uint32_t		t17;
	uint32_t		t35;
	uint32_t		t39;
	uint32_t		tval;
} sng_ccSpan_t;

int cmbLinkSetId;
/* Total number of cics configured */
uint32_t nmb_cics_cfg = 0;
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
int ftmod_ss7_parse_xml(ftdm_conf_parameter_t *ftdm_parameters, ftdm_span_t *span);

static int ftmod_ss7_parse_sng_isup(ftdm_conf_node_t *sng_isup, const char* operating_mode, ftdm_span_t *span);

static int ftmod_ss7_parse_sng_gen(ftdm_conf_node_t *sng_gen, char* operating_mode);

static int ftmod_ss7_parse_sng_relay(ftdm_conf_node_t *sng_relay);
static int ftmod_ss7_parse_relay_channel(ftdm_conf_node_t *relay_chan);

static int ftmod_ss7_parse_mtp1_links(ftdm_conf_node_t *mtp1_links);
static int ftmod_ss7_parse_mtp1_link(ftdm_conf_node_t *mtp1_link);

static int ftmod_ss7_parse_mtp2_links(ftdm_conf_node_t *mtp2_links);
static int ftmod_ss7_parse_mtp2_link(ftdm_conf_node_t *mtp2_link);

static int ftmod_ss7_parse_mtp3_links(ftdm_conf_node_t *mtp3_links);
static int ftmod_ss7_parse_mtp3_link(ftdm_conf_node_t *mtp3_link);

static int ftmod_ss7_parse_mtp_linksets(ftdm_conf_node_t *mtp_linksets);
static int ftmod_ss7_parse_mtp_linkset(ftdm_conf_node_t *mtp_linkset);

static int ftmod_ss7_parse_mtp_routes(ftdm_conf_node_t *mtp_routes, ftdm_span_t *span);
static int ftmod_ss7_parse_mtp_route(ftdm_conf_node_t *mtp_route, ftdm_span_t *span);

static int ftmod_ss7_parse_isup_interfaces(ftdm_conf_node_t *isup_interfaces);
static int ftmod_ss7_parse_isup_interface(ftdm_conf_node_t *isup_interface);

static int ftmod_ss7_parse_cc_spans(ftdm_conf_node_t *cc_spans);
static int ftmod_ss7_parse_cc_span(ftdm_conf_node_t *cc_span);

static int ftmod_ss7_fill_in_relay_channel(sng_relay_t *relay_channel);
static int ftmod_ss7_fill_in_mtp1_link(sng_mtp1_link_t *mtp1Link);
static int ftmod_ss7_fill_in_mtp2_link(sng_mtp2_link_t *mtp2Link);
static int ftmod_ss7_fill_in_mtp3_link(sng_mtp3_link_t *mtp3Link);
static int ftmod_ss7_fill_in_mtpLinkSet(sng_link_set_t *mtpLinkSet);
static int ftmod_ss7_fill_in_mtp3_route(sng_route_t *mtp3_route);
static int ftmod_ss7_fill_in_nsap(sng_route_t *mtp3_route);
/* Fill in ACC Timer */
static int ftmod_ss7_fill_in_acc_timer(sng_route_t *mtp3_route, ftdm_span_t *span);
static int ftmod_ss7_fill_in_isup_interface(sng_isup_inf_t *sng_isup);
static int ftmod_ss7_fill_in_isap(sng_isap_t *sng_isap);
static int ftmod_ss7_fill_in_ccSpan(sng_ccSpan_t *ccSpan);
static int ftmod_ss7_fill_in_self_route(int spc, int linkType, int switchType, int ssf);
static int ftmod_ss7_fill_in_circuits(sng_span_t *sngSpan);

static int ftmod_ss7_next_timeslot(char *ch_map, sng_timeslot_t *timeslot);
static void ftmod_ss7_set_glare_resolution (const char *method);
static int ftmod_ss7_set_operating_mode(const char* span_opr_mode, const char* sng_gen_opr_mode);

/******************************************************************************/

/* FUNCTIONS ******************************************************************/

static int ftmod_ss7_set_operating_mode(const char* span_opr_mode, const char* sng_gen_opr_mode)
{
    ftdm_sngss7_operating_modes_e mode = SNG_SS7_OPR_MODE_NONE;
    char opr_mode[128];

    memset(opr_mode, 0, sizeof(opr_mode));

    /* 1)  Initially operating mode will be set to INVALID
    *  2)  operating-mode can be the part of sng_gen and span configuration 
    *  3)  we are only supporting one operating mode per system, NOT supporting per span operating mode 
    *  4)  IF first span configuration or sng_ss7 we set operating mode to X and then subsequent span
    *      configuration defined another operating mode , return error 
    *  5)  If operating mode present in span config..this will take priority then configured in sng_gen
    *  */

    if (span_opr_mode && ('\0' != span_opr_mode[0])) {
        strcpy(opr_mode, span_opr_mode);
    } else if (sng_gen_opr_mode && ('\0' != sng_gen_opr_mode[0])) {
        strcpy(opr_mode, sng_gen_opr_mode);
    } else {
        strcpy(opr_mode, "ISUP");
    }

    if (!strcasecmp(opr_mode, "ISUP")) {
        mode = SNG_SS7_OPR_MODE_ISUP;
    } else if (!strcasecmp(opr_mode, "M2UA_SG")) {                                                       
        mode = SNG_SS7_OPR_MODE_M2UA_SG;
    } else if (!strcasecmp(opr_mode, "M2UA_ASP")) {                                                    
        mode = SNG_SS7_OPR_MODE_M2UA_ASP;
    } else if (!strcasecmp(opr_mode, "MTP2_API")) {                                                  
        mode = SNG_SS7_OPR_MODE_MTP2_API;
    } else {
        return FTDM_SUCCESS;
    }

    if ( SNG_SS7_OPR_MODE_NONE == g_ftdm_operating_mode ) {
        g_ftdm_operating_mode = mode;
    } else if (g_ftdm_operating_mode != mode) {
        SS7_ERROR("Different operating Mode[%s] then configured[%s] \n", 
                opr_mode, ftdm_opr_mode_tostr(g_ftdm_operating_mode));                                      
        return FTDM_FAIL;
    }

    return FTDM_SUCCESS;
}

int ftmod_ss7_parse_xml(ftdm_conf_parameter_t *ftdm_parameters, ftdm_span_t *span)
{
	sng_route_t			self_route;
	sng_span_t			sngSpan;
	int					i = 0;
	const char			*var = NULL;
	const char			*val = NULL;
	const char			*operating_mode = NULL;
	ftdm_conf_node_t	*ptr = NULL;

	/* clean out the isup ckt */
	memset(&sngSpan, 0x0, sizeof(sngSpan));

	/* clean out the self route */
	memset(&self_route, 0x0, sizeof(self_route));

	var = ftdm_parameters[i].var;
	val = ftdm_parameters[i].val;

    /* Default value will be set after processing of sng_gen  to
     * confirm that operating mode not defined neither in sng gen nor in span cfg */
	//g_ftdm_operating_mode = SNG_SS7_OPR_MODE_ISUP;
    
    /* operating mode can be present in sng_gen or per span as of now */
    /* if operating mode present in sng_gen and span then span has to take priority */

	/* confirm that the first parameter is the "operating-mode" */
	if (!strcasecmp(var, "operating-mode")) {
		operating_mode = val;
		i++;
	}

    var = ftdm_parameters[i].var;
    val = ftdm_parameters[i].val;
	ptr = (ftdm_conf_node_t *)ftdm_parameters[i].ptr;

	/* confirm that the 2nd parameter is the "confnode" */
	if (!strcasecmp(var, "confnode")) {
		/* parse the confnode and fill in the global libsng_ss7 config structure */
		if (ftmod_ss7_parse_sng_isup(ptr, operating_mode, span)) {
			SS7_ERROR("Failed to parse the \"confnode\"!\n");
			goto ftmod_ss7_parse_xml_error;
		}
	} else {
		/* ERROR...exit */
		SS7_ERROR("The \"confnode\" configuration was not the first parameter!\n");
		SS7_ERROR("\tFound \"%s\" in the first slot\n", var);
		goto ftmod_ss7_parse_xml_error;
	}

	i++;

	while (ftdm_parameters[i].var != NULL) {
		var = ftdm_parameters[i].var;
		val = ftdm_parameters[i].val;

		if (!strcasecmp(var, "dialplan")) {
			/* don't care for now */
		} else if (!strcasecmp(var, "context")) {
			/* don't care for now */
		} else if (!strcasecmp(var, "span-id") || !strcasecmp(var, "ccSpanId")) {
			sngSpan.ccSpanId = atoi(val);
			SS7_DEBUG("Found an ccSpanId  = %d\n",sngSpan.ccSpanId);
		} else {
			SS7_ERROR("Unknown parameter found =\"%s\"...ignoring it!\n", var);
		}

		i++;
	} /* while (ftdm_parameters[i].var != NULL) */

	/* fill the pointer to span into isupCkt */
	sngSpan.span = span;

    if ((SNG_SS7_OPR_MODE_ISUP == g_ftdm_operating_mode) || 
            (SNG_SS7_OPR_MODE_M2UA_ASP == g_ftdm_operating_mode)){
        /* setup the circuits structure */
        if(ftmod_ss7_fill_in_circuits(&sngSpan)) {
            SS7_ERROR("Failed to fill in circuits structure!\n");
            goto ftmod_ss7_parse_xml_error;
        }
    }

	return FTDM_SUCCESS;

ftmod_ss7_parse_xml_error:
	return FTDM_FAIL;
}

/******************************************************************************/
static int ftmod_ss7_parse_sng_isup(ftdm_conf_node_t *sng_isup, const char* span_opr_mode, ftdm_span_t *span)
{
	ftdm_conf_node_t	*gen_config = NULL;
	ftdm_conf_node_t	*relay_channels = NULL;
	ftdm_conf_node_t	*mtp1_links = NULL;
	ftdm_conf_node_t	*mtp2_links = NULL;
	ftdm_conf_node_t	*mtp3_links = NULL;
	ftdm_conf_node_t	*mtp_linksets = NULL;
	ftdm_conf_node_t	*mtp_routes = NULL;
	ftdm_conf_node_t	*isup_interfaces = NULL;
	ftdm_conf_node_t	*cc_spans = NULL;
	ftdm_conf_node_t	*tmp_node = NULL;
	ftdm_conf_node_t	*nif_ifaces = NULL;
	ftdm_conf_node_t	*m2ua_ifaces = NULL;
	ftdm_conf_node_t	*m2ua_peer_ifaces = NULL;
	ftdm_conf_node_t	*m2ua_clust_ifaces = NULL;
	ftdm_conf_node_t	*sctp_ifaces = NULL;
	char                 	sng_gen_opr_mode[128];

	memset(&sng_gen_opr_mode[0], 0, sizeof(sng_gen_opr_mode));

	/* confirm that we are looking at sng_isup */
	if (strcasecmp(sng_isup->name, "sng_isup")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"sng_isup\"!\n",sng_isup->name);
		return FTDM_FAIL;
	}  else {
		SS7_DEBUG("Parsing \"sng_isup\"...\n");
	}

	/* extract the main sections of the sng_isup block */
	tmp_node = sng_isup->child;

	while (tmp_node != NULL) {
		/**************************************************************************/
		/* check the type of structure */
		if (!strcasecmp(tmp_node->name, "sng_gen")) {
		/**********************************************************************/
			if (gen_config == NULL) {
				gen_config = tmp_node;
				SS7_DEBUG("Found a \"sng_gen\" section!\n");
			} else {
				SS7_ERROR("Found a second \"sng_gen\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_relay")) {
		/**********************************************************************/
			if (relay_channels == NULL) {
				relay_channels = tmp_node;
				SS7_DEBUG("Found a \"sng_relay\" section!\n");
			} else {
				SS7_ERROR("Found a second \"sng_relay\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		}else if (!strcasecmp(tmp_node->name, "mtp1_links")) {
		/**********************************************************************/
			if (mtp1_links == NULL) {
				mtp1_links = tmp_node;
				SS7_DEBUG("Found a \"mtp1_links\" section!\n");
			} else {
				SS7_ERROR("Found a second \"mtp1_links\" section!\n");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "mtp2_links")) {
		/**********************************************************************/
			if (mtp2_links == NULL) {
				mtp2_links = tmp_node;
				SS7_DEBUG("Found a \"mtp2_links\" section!\n");
			} else {
				SS7_ERROR("Found a second \"mtp2_links\" section!\n");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "mtp3_links")) {
		/**********************************************************************/
			if (mtp3_links == NULL) {
				mtp3_links = tmp_node;
				SS7_DEBUG("Found a \"mtp3_links\" section!\n");
			} else {
				SS7_ERROR("Found a second \"mtp3_links\" section!\n");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "mtp_linksets")) {
		/**********************************************************************/
			if (mtp_linksets == NULL) {
				mtp_linksets = tmp_node;
				SS7_DEBUG("Found a \"mtp_linksets\" section!\n");
			} else {
				SS7_ERROR("Found a second \"mtp_linksets\" section!\n");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "mtp_routes")) {
		/**********************************************************************/
			if (mtp_routes == NULL) {
				mtp_routes = tmp_node;
				SS7_DEBUG("Found a \"mtp_routes\" section!\n");
			} else {
				SS7_ERROR("Found a second \"mtp_routes\" section!\n");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "isup_interfaces")) {
		/**********************************************************************/
			if (isup_interfaces == NULL) {
				isup_interfaces = tmp_node;
				SS7_DEBUG("Found a \"isup_interfaces\" section!\n");
			} else {
				SS7_ERROR("Found a second \"isup_interfaces\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "cc_spans")) {
		/**********************************************************************/
			if (cc_spans == NULL) {
				cc_spans = tmp_node;
				SS7_DEBUG("Found a \"cc_spans\" section!\n");
			} else {
				SS7_ERROR("Found a second \"cc_spans\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_nif_interfaces")) {
		/**********************************************************************/
			if (nif_ifaces == NULL) {
				nif_ifaces = tmp_node;
				SS7_DEBUG("Found a \"sng_nif_interfaces\" section!\n");
			} else {
				SS7_ERROR("Found a second \"sng_nif_interfaces\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_m2ua_interfaces")) {
		/**********************************************************************/
			if (m2ua_ifaces == NULL) {
				m2ua_ifaces = tmp_node;
				SS7_DEBUG("Found a \"sng_m2ua_interfaces\" section!\n");
			} else {
				SS7_ERROR("Found a second \"sng_m2ua_interfaces\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_m2ua_peer_interfaces")) {
		/**********************************************************************/
			if (m2ua_peer_ifaces == NULL) {
				m2ua_peer_ifaces = tmp_node;
				SS7_DEBUG("Found a \"sng_m2ua_peer_interfaces\" section!\n");
			} else {
				SS7_ERROR("Found a second \"sng_m2ua_peer_interfaces\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_m2ua_cluster_interfaces")) {
		/**********************************************************************/
			if (m2ua_clust_ifaces == NULL) {
				m2ua_clust_ifaces = tmp_node;
				SS7_DEBUG("Found a \"sng_m2ua_cluster_interfaces\" section!\n");
			} else {
				SS7_ERROR("Found a second \"sng_m2ua_peer_interfaces\" section\n!");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_sctp_interfaces")) {
		/**********************************************************************/
			if (sctp_ifaces == NULL) {
				sctp_ifaces = tmp_node;
				SS7_DEBUG("Found a <sng_sctp_interfaces> section!\n");
			} else {
				SS7_ERROR("Found a second <sng_sctp_interfaces> section!\n");
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else {
		/**********************************************************************/
			SS7_ERROR("\tFound an unknown section \"%s\"!\n", tmp_node->name);
			return FTDM_FAIL;
		/**********************************************************************/
		}

		/* go to the next sibling */
		tmp_node = tmp_node->next;
	} /* while (tmp_node != NULL) */

	/* now try to parse the sections */
	if (ftmod_ss7_parse_sng_gen(gen_config, sng_gen_opr_mode)) {
		SS7_ERROR("Failed to parse \"gen_config\"!\n");
		return FTDM_FAIL;
	}

    if (FTDM_FAIL == ftmod_ss7_set_operating_mode(span_opr_mode, sng_gen_opr_mode)) {
        return FTDM_FAIL;
    } 

    SS7_INFO("Setting FreeTDM Operating Mode[%s]\n",ftdm_opr_mode_tostr(g_ftdm_operating_mode));

	if (relay_channels && ftmod_ss7_parse_sng_relay(relay_channels)) {
		SS7_ERROR("Failed to parse \"relay_channels\"!\n");
		return FTDM_FAIL;
	}


/****************************************************************************************************/
	if((SNG_SS7_OPR_MODE_ISUP == g_ftdm_operating_mode) || 
			(SNG_SS7_OPR_MODE_M2UA_SG == g_ftdm_operating_mode) ||
            (SNG_SS7_OPR_MODE_MTP2_API == g_ftdm_operating_mode)) {

		if (ftmod_ss7_parse_mtp1_links(mtp1_links)) {
			SS7_ERROR("Failed to parse \"mtp1_links\"!\n");
			return FTDM_FAIL;
		}

		if (ftmod_ss7_parse_mtp2_links(mtp2_links)) {
			SS7_ERROR("Failed to parse \"mtp2_links\"!\n");
			return FTDM_FAIL;
		}
	}

    if ( SNG_SS7_OPR_MODE_MTP2_API == g_ftdm_operating_mode ) goto done;
/****************************************************************************************************/

	if((SNG_SS7_OPR_MODE_ISUP == g_ftdm_operating_mode) || 
			(SNG_SS7_OPR_MODE_M2UA_ASP == g_ftdm_operating_mode)){

		if (mtp3_links && ftmod_ss7_parse_mtp3_links(mtp3_links)) {
			SS7_ERROR("Failed to parse \"mtp3_links\"!\n");
			return FTDM_FAIL;
		}

		if (ftmod_ss7_parse_mtp_linksets(mtp_linksets)) {
			SS7_ERROR("Failed to parse \"mtp_linksets\"!\n");
			return FTDM_FAIL;
		}

		if (ftmod_ss7_parse_mtp_routes(mtp_routes, span)) {
			SS7_ERROR("Failed to parse \"mtp_routes\"!\n");
			return FTDM_FAIL;
		}

		if (isup_interfaces && ftmod_ss7_parse_isup_interfaces(isup_interfaces)) {
			SS7_ERROR("Failed to parse \"isup_interfaces\"!\n");
			return FTDM_FAIL;
		}

		if (cc_spans && ftmod_ss7_parse_cc_spans(cc_spans)) {
			SS7_ERROR("Failed to parse \"cc_spans\"!\n");
			return FTDM_FAIL;
		}
	}
/****************************************************************************************************/

	if((SNG_SS7_OPR_MODE_M2UA_SG == g_ftdm_operating_mode) || 
			(SNG_SS7_OPR_MODE_M2UA_ASP == g_ftdm_operating_mode)){

		if (ftmod_ss7_parse_sctp_links(sctp_ifaces) != FTDM_SUCCESS) {
			SS7_ERROR("Failed to parse <sctp_links>!\n");
			return FTDM_FAIL;
		}

		if ((SNG_SS7_OPR_MODE_M2UA_SG == g_ftdm_operating_mode) && 
		    (nif_ifaces && ftmod_ss7_parse_nif_interfaces(nif_ifaces))) {
			SS7_ERROR("Failed to parse \"nif_ifaces\"!\n");
			return FTDM_FAIL;
		}

		if (m2ua_ifaces && ftmod_ss7_parse_m2ua_interfaces(m2ua_ifaces)) {
			SS7_ERROR("Failed to parse \"m2ua_ifaces\"!\n");
			return FTDM_FAIL;
		}
		if (m2ua_peer_ifaces && ftmod_ss7_parse_m2ua_peer_interfaces(m2ua_peer_ifaces)) {
			SS7_ERROR("Failed to parse \"m2ua_peer_ifaces\"!\n");
			return FTDM_FAIL;
		}
		if (m2ua_clust_ifaces && ftmod_ss7_parse_m2ua_clust_interfaces(m2ua_clust_ifaces)) {
			SS7_ERROR("Failed to parse \"m2ua_clust_ifaces\"!\n");
			return FTDM_FAIL;
		}
	}
/****************************************************************************************************/
done:
	return FTDM_SUCCESS;
}

static void ftmod_ss7_set_glare_resolution (const char *method)
{
	sng_glare_resolution iMethod=SNGSS7_GLARE_PC;
	if (!method || (strlen (method) <=0) ) {
		SS7_ERROR( "Wrong glare resolution parameter, using default. \n" );
	} else {
		if (!strcasecmp( method, "PointCode")) {
			iMethod = SNGSS7_GLARE_PC;
		} else if (!strcasecmp( method, "Down")) {
			iMethod = SNGSS7_GLARE_DOWN;
		} else if (!strcasecmp( method, "Control")) {
			iMethod = SNGSS7_GLARE_CONTROL;
		} else {
			SS7_ERROR( "Wrong glare resolution parameter, using default. \n" );
			iMethod = SNGSS7_GLARE_PC;			
		}
	}
	g_ftdm_sngss7_data.cfg.glareResolution = iMethod;
}

/******************************************************************************/
static int ftmod_ss7_parse_sng_gen(ftdm_conf_node_t *sng_gen, char* operating_mode)
{
	ftdm_conf_parameter_t	*parm = sng_gen->parameters;
	int						num_parms = sng_gen->n_parameters;
	int						i = 0;

	/* Set the transparent_iam_max_size to default value */
	g_ftdm_sngss7_data.cfg.transparent_iam_max_size=800;
	g_ftdm_sngss7_data.cfg.force_inr = 0;
	g_ftdm_sngss7_data.cfg.force_early_media = 0;

	/* By default automatic congestion control will be False */
	g_ftdm_sngss7_data.cfg.sng_acc = 0;

	/* By default message priority is default */
	g_ftdm_sngss7_data.cfg.msg_priority = 0;
	g_ftdm_sngss7_data.cfg.set_msg_priority = DEFAULT_MSG_PRIORITY;
	g_ftdm_sngss7_data.cfg.link_failure_action = SNGSS7_ACTION_RELEASE_CALLS;

	/* extract all the information from the parameters */
	for (i = 0; i < num_parms; i++) {
		if (!strcasecmp(parm->var, "procId")) {
			g_ftdm_sngss7_data.cfg.procId = atoi(parm->val);
			SS7_DEBUG("Found a procId = %d\n", g_ftdm_sngss7_data.cfg.procId);
		} 
		else if (!strcasecmp(parm->var, "license")) {
			ftdm_set_string(g_ftdm_sngss7_data.cfg.license, parm->val);
			snprintf(g_ftdm_sngss7_data.cfg.signature, sizeof(g_ftdm_sngss7_data.cfg.signature), "%s.sig", parm->val);
			SS7_DEBUG("Found license file = %s\n", g_ftdm_sngss7_data.cfg.license);
			SS7_DEBUG("Found signature file = %s\n", g_ftdm_sngss7_data.cfg.signature);	
		} 
		else if (!strcasecmp(parm->var, "transparent_iam_max_size")) {
			g_ftdm_sngss7_data.cfg.transparent_iam_max_size = atoi(parm->val);
			SS7_DEBUG("Found a transparent_iam max size = %d\n", g_ftdm_sngss7_data.cfg.transparent_iam_max_size);
		} 
		else if (!strcasecmp(parm->var, "glare-reso")) {
			ftmod_ss7_set_glare_resolution (parm->val);
			SS7_DEBUG("Found glare resolution configuration = %d  %s\n", g_ftdm_sngss7_data.cfg.glareResolution, parm->val );
		}
		else if (!strcasecmp(parm->var, "force_inr")) {
			if (ftdm_true(parm->val)) {
				g_ftdm_sngss7_data.cfg.force_inr = 1;
			} else {
				g_ftdm_sngss7_data.cfg.force_inr = 0;
			}
			SS7_DEBUG("Found INR force configuration = %s\n", parm->val);
		} else if (!strcasecmp(parm->var, "force_early_media")) {
			if (ftdm_true(parm->val)) {
				g_ftdm_sngss7_data.cfg.force_early_media = 1;
			} else {
				g_ftdm_sngss7_data.cfg.force_early_media = 0;
			}
			SS7_DEBUG("Found force early media configuration = %s\n", parm->val);
		} else if (!strcasecmp(parm->var, "operating_mode")) {
			strcpy(operating_mode, parm->val);
		} else if (!strcasecmp(parm->var, "stack-logging-enable")) {
			if (ftdm_true(parm->val)) {
				g_ftdm_sngss7_data.stack_logging_enable = 1;
			} else {
				g_ftdm_sngss7_data.stack_logging_enable = 0;
			}
			SS7_DEBUG("SS7 Stack Initial Logging [%s] \n", 
					(g_ftdm_sngss7_data.stack_logging_enable)?"ENABLE":"DISABLE");
		} else if (!strcasecmp(parm->var, "max-cpu-usage")) {
			g_ftdm_sngss7_data.cfg.max_cpu_usage = atoi(parm->val);
			SS7_DEBUG("Found maximum cpu usage limit = %d\n", g_ftdm_sngss7_data.cfg.max_cpu_usage);
		} else if (!strcasecmp(parm->var, "auto-congestion-control")) {
			if (ftdm_true(parm->val)) {
				g_ftdm_sngss7_data.cfg.sng_acc = 1;
			} else {
				g_ftdm_sngss7_data.cfg.sng_acc = 0;
			}
			SS7_DEBUG("Found Automatic Congestion Control configuration = %s\n", parm->val);
	       } else if (!strcasecmp(parm->var, "traffic-reduction-rate")) {
			if (g_ftdm_sngss7_data.cfg.sng_acc) {
				g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate = atoi(parm->val);
				SS7_DEBUG("Found Traffic reduction rate = %d for Automatic Congestion Control feature\n", g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate);
			} else {
				SS7_DEBUG("Found invalid configurable parameter Traffic reduction rate as Automatic congestion feature is not enable\n");
			}
		} else if (!strcasecmp(parm->var, "traffic-increment-rate")) {
			if (g_ftdm_sngss7_data.cfg.sng_acc) {
				g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate = atoi(parm->val);
				SS7_DEBUG("Found Traffic incremnent rate = %d for Automatic Congestion Control feature\n", g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate);
			} else {
				SS7_DEBUG("Found invalid configurable parameter Traffic increment rate as Automatic congestion feature is not enable\n");
			}
		} else if (!strcasecmp(parm->var, "first-level-reduction-rate")) {
				if (g_ftdm_sngss7_data.cfg.sng_acc) {
					g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate = atoi(parm->val);
					SS7_DEBUG("Found Congestion Level 1 traffic reduction rate = %d for Automatic Congestion Control feature\n", g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate);
				} else {
					SS7_DEBUG("Found invalid configurable parameter Congestion Level 1 Traffic Reduction Rate as Automatic congestion feature is not enable\n");
				}
		} else if (!strcasecmp(parm->var, "second-level-reduction-rate")) {
				if (g_ftdm_sngss7_data.cfg.sng_acc) {
					g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate = atoi(parm->val);
					SS7_DEBUG("Found Congestion Level 2 traffic reduction rate = %d for Automatic Congestion Control feature\n", g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate);
				} else {
					SS7_DEBUG("Found invalid configurable parameter Congestion Level 2 Traffic Reduction Rate as Automatic congestion feature is not enable\n");
				}
		} else if (!strcasecmp(parm->var, "set-isup-message-priority")) {
			if (!strcasecmp(parm->val, "default")) {
				g_ftdm_sngss7_data.cfg.set_msg_priority = 7;
				SS7_DEBUG("Found ISUP message prioriy = %d\n", g_ftdm_sngss7_data.cfg.set_msg_priority);
			} else if (!strcasecmp(parm->val, "all-zeros")) {
				g_ftdm_sngss7_data.cfg.set_msg_priority = 0;
				SS7_DEBUG("Found ISUP message prioriy = %d\n", g_ftdm_sngss7_data.cfg.set_msg_priority);
			} else if (!strcasecmp(parm->val, "all-ones")) {
				g_ftdm_sngss7_data.cfg.set_msg_priority = 87381;
				SS7_DEBUG("Found ISUP message prioriy = %d\n", g_ftdm_sngss7_data.cfg.set_msg_priority);
			} else {
				SS7_ERROR("Invalid ISUP message prioriy = %s\n", parm->val);
				return FTDM_FAIL;
			}
			g_ftdm_sngss7_data.cfg.msg_priority = 1;
		} else if (!strcasecmp(parm->var, "link_failure_action")) {
			g_ftdm_sngss7_data.cfg.link_failure_action = atoi(parm->val);
			if ((g_ftdm_sngss7_data.cfg.link_failure_action == SNGSS7_ACTION_RELEASE_CALLS) || 
					(g_ftdm_sngss7_data.cfg.link_failure_action == SNGSS7_ACTION_KEEP_CALLS)) {
				SS7_DEBUG("Found link_failure_action  = %d\n", g_ftdm_sngss7_data.cfg.link_failure_action);
			} else {
				SS7_DEBUG("Found invalid link_failure_action  = %d\n", g_ftdm_sngss7_data.cfg.link_failure_action);
				/* default value release calls (for backward compatibility) */
				g_ftdm_sngss7_data.cfg.link_failure_action = SNGSS7_ACTION_RELEASE_CALLS;
			}
		} else {
			SS7_ERROR("Found an invalid parameter \"%s\"!\n", parm->var);
			return FTDM_FAIL;
		}

		/* move to the next parmeter */
		parm = parm + 1;
	} /* for (i = 0; i < num_parms; i++) */

	/* Set default configuration for Automatic congestion control feature if any parameter is not configured properly */
	ftmod_ss7_acc_default_config();

	if (!g_ftdm_sngss7_data.cfg.max_cpu_usage) {
		g_ftdm_sngss7_data.cfg.max_cpu_usage = 80;
		SS7_DEBUG("Assigning default value to maximum cpu usage limit = %d\n", g_ftdm_sngss7_data.cfg.max_cpu_usage);
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_sng_relay(ftdm_conf_node_t *sng_relay)
{
	ftdm_conf_node_t	*relay_chan = NULL;

	/* confirm that we are looking at sng_relay */
	if (strcasecmp(sng_relay->name, "sng_relay")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"sng_relay\"!\n",sng_relay->name);
		return FTDM_FAIL;
	}  else {
		SS7_DEBUG("Parsing \"sng_relay\"...\n");
	}

	relay_chan = sng_relay->child;

	if (relay_chan != NULL) {
		while (relay_chan != NULL) {
			/* try to the parse relay_channel */
			if (ftmod_ss7_parse_relay_channel(relay_chan)) {
				SS7_ERROR("Failed to parse \"relay_channels\"!\n");
				return FTDM_FAIL;
			}

			/* move on to the next linkset */
			relay_chan = relay_chan->next;
		} /* while (relay_chan != NULL) */
	} /* if (relay_chan != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_relay_channel(ftdm_conf_node_t *relay_chan)
{
	sng_relay_t				tmp_chan;
	ftdm_conf_parameter_t	*parm = relay_chan->parameters;
	int						num_parms = relay_chan->n_parameters;
	int						i = 0;

	/* confirm that we are looking at relay_channel */
	if (strcasecmp(relay_chan->name, "relay_channel")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"relay_channel\"!\n",relay_chan->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"relay_channel\"...\n");
	}

	/* initalize the tmp_chan structure */
	memset(&tmp_chan, 0x0, sizeof(tmp_chan));

	/* extract all the information from the parameters */
	for (i = 0; i < num_parms; i++) {
	/**************************************************************************/

		if (!strcasecmp(parm->var, "name")) {
		/**********************************************************************/
			strcpy((char *)tmp_chan.name, parm->val);
			SS7_DEBUG("Found an relay_channel named = %s\n", tmp_chan.name);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "type")) {
		/**********************************************************************/
			if (!strcasecmp(parm->val, "listen")) {
				tmp_chan.type = LRY_CT_TCP_LISTEN;
				SS7_DEBUG("Found a type = LISTEN\n");
			} else if (!strcasecmp(parm->val, "server")) {
				tmp_chan.type = LRY_CT_TCP_SERVER;
				SS7_DEBUG("Found a type = SERVER\n");
			} else if (!strcasecmp(parm->val, "client")) {
				tmp_chan.type = LRY_CT_TCP_CLIENT;
				SS7_DEBUG("Found a type = CLIENT\n");
			} else {
				SS7_ERROR("Found an invalid \"type\" = %s\n", parm->var);
				return FTDM_FAIL;
			}
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "hostname")) {
		/**********************************************************************/
			strcpy((char *)tmp_chan.hostname, parm->val);
			SS7_DEBUG("Found a hostname = %s\n", tmp_chan.hostname);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "port")) {
		/**********************************************************************/
			tmp_chan.port = atoi(parm->val);
			SS7_DEBUG("Found a port = %d\n", tmp_chan.port);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "procId")) {
		/**********************************************************************/
			tmp_chan.procId = atoi(parm->val);
			SS7_DEBUG("Found a procId = %d\n", tmp_chan.procId);
		/**********************************************************************/
		} else {
		/**********************************************************************/
			SS7_ERROR("Found an invalid parameter \"%s\"!\n", parm->val);
			return FTDM_FAIL;
		/**********************************************************************/
		}

		/* move to the next parmeter */
		parm = parm + 1;
	} /* for (i = 0; i < num_parms; i++) */

	/* store the channel in the global structure */
	ftmod_ss7_fill_in_relay_channel(&tmp_chan);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp1_links(ftdm_conf_node_t *mtp1_links)
{
	ftdm_conf_node_t	*mtp1_link = NULL;

	/* confirm that we are looking at mtp1_links */
	if (strcasecmp(mtp1_links->name, "mtp1_links")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp1_links\"!\n",mtp1_links->name);
		return FTDM_FAIL;
	}  else {
		SS7_DEBUG("Parsing \"mtp1_links\"...\n");
	}

	/* extract the mtp_links */
	mtp1_link = mtp1_links->child;

	/* run through all of the links found  */
	while (mtp1_link != NULL) {
		/* try to the parse mtp_link */
		if (ftmod_ss7_parse_mtp1_link(mtp1_link)) {
			SS7_ERROR("Failed to parse \"mtp1_link\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next link */
		mtp1_link = mtp1_link->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp1_link(ftdm_conf_node_t *mtp1_link)
{
	sng_mtp1_link_t		 	mtp1Link;
	ftdm_conf_parameter_t	*parm = mtp1_link->parameters;
	int					 	num_parms = mtp1_link->n_parameters;
	int					 	i;
    ftdm_span_t *ftdmspan     = NULL;
    ftdm_iterator_t *chaniter = NULL;
    ftdm_iterator_t *curr     = NULL;


	/* initalize the mtp1Link structure */
	memset(&mtp1Link, 0x0, sizeof(mtp1Link));

	/* confirm that we are looking at an mtp_link */
	if (strcasecmp(mtp1_link->name, "mtp1_link")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp1_link\"!\n",mtp1_link->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"mtp1_link\"...\n");
	}

	for (i = 0; i < num_parms; i++) {
	/**************************************************************************/

		if (!strcasecmp(parm->var, "name")) {
		/**********************************************************************/
			strcpy((char *)mtp1Link.name, parm->val);
			SS7_DEBUG("Found an mtp1_link named = %s\n", mtp1Link.name);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "id")) {
		/**********************************************************************/
			mtp1Link.id = atoi(parm->val);
			SS7_DEBUG("Found an mtp1_link id = %d\n", mtp1Link.id);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "span")) {
		/**********************************************************************/
			mtp1Link.span = atoi(parm->val);
			SS7_DEBUG("Found an mtp1_link span = %d\n", mtp1Link.span);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "chan_no") || !strcasecmp(parm->var, "chan")){ 
		/**********************************************************************/
			mtp1Link.chan = atoi(parm->val);
			SS7_DEBUG("Found an mtp1_link chan = %d\n", mtp1Link.chan);
		/**********************************************************************/
        } else if (!strcasecmp(parm->var, "span_name")) {
            /**********************************************************************/
            strcpy((char *)mtp1Link.span_name, parm->val);
            SS7_DEBUG("Found an mtp1_link span = %s\n", mtp1Link.span_name);
            /**********************************************************************/
		} else {
		/**********************************************************************/
			SS7_ERROR("\tFound an invalid parameter \"%s\"!\n", parm->val);
			return FTDM_FAIL;
		/**********************************************************************/
		}

		/* move to the next parmeter */
		parm = parm + 1;

	/**************************************************************************/
	} /* for (i = 0; i < num_parms; i++) */

    if (mtp1Link.chan <= 0) {
        ftdm_log(FTDM_LOG_CRIT, "Channel number not specified for mtp1_link %s\n", mtp1Link.name);
        return FTDM_FAIL;
    }

    if (SNG_SS7_OPR_MODE_MTP2_API == g_ftdm_operating_mode) {
        if (!ftdm_strlen_zero(mtp1Link.span_name)) {
            ftdm_span_find_by_name(mtp1Link.span_name, &ftdmspan);
            if (!ftdmspan) {
                ftdm_log(FTDM_LOG_ERROR, "Could not find span with name:%s\n", mtp1Link.span_name);
                return FTDM_FAIL;
            }
        }

        if (!ftdmspan) {
            ftdm_log(FTDM_LOG_ERROR, "Could not find span for this mtp1 Link\n");
            return FTDM_FAIL;
        }

        chaniter = ftdm_span_get_chan_iterator(ftdmspan, NULL);
        for (curr = chaniter; curr; curr = ftdm_iterator_next(curr)) {
            ftdm_channel_t *ftdmchan = (ftdm_channel_t*)ftdm_iterator_current(curr);

            if (ftdmchan->physical_chan_id == mtp1Link.chan) {
                mtp1Link.ftdmchan = ftdmchan;
                break;
            }
        }
        ftdm_iterator_free(chaniter);

        if (!mtp1Link.ftdmchan) {
            ftdm_log(FTDM_LOG_CRIT, "Could not find mtp1_link chan = %s\n", parm->val);
            return FTDM_FAIL;
        }
    }

	/* store the link in global structure */
	ftmod_ss7_fill_in_mtp1_link(&mtp1Link);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp2_links(ftdm_conf_node_t *mtp2_links)
{
	ftdm_conf_node_t	*mtp2_link = NULL;

	/* confirm that we are looking at mtp2_links */
	if (strcasecmp(mtp2_links->name, "mtp2_links")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp2_links\"!\n",mtp2_links->name);
		return FTDM_FAIL;
	}  else {
		SS7_DEBUG("Parsing \"mtp2_links\"...\n");
	}

	/* extract the mtp_links */
	mtp2_link = mtp2_links->child;

	/* run through all of the links found  */
	while (mtp2_link != NULL) {
		/* try to the parse mtp_linkset */
		if (ftmod_ss7_parse_mtp2_link(mtp2_link)) {
			SS7_ERROR("Failed to parse \"mtp2_link\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next link */
		mtp2_link = mtp2_link->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp2_link(ftdm_conf_node_t *mtp2_link)
{
	sng_mtp2_link_t		 	mtp2Link;
	ftdm_conf_parameter_t	*parm = mtp2_link->parameters;
	int					 	num_parms = mtp2_link->n_parameters;
	int					 	i;
	int						ret;

	/* initalize the mtp2Link structure */
	memset(&mtp2Link, 0x0, sizeof(mtp2Link));

	/* confirm that we are looking at an mtp2_link */
	if (strcasecmp(mtp2_link->name, "mtp2_link")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp2_link\"!\n",mtp2_link->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"mtp2_link\"...\n");
	}

	for (i = 0; i < num_parms; i++) {
	/**************************************************************************/

		if (!strcasecmp(parm->var, "name")) {
		/**********************************************************************/
			strcpy((char *)mtp2Link.name, parm->val);
			SS7_DEBUG("Found an mtp2_link named = %s\n", mtp2Link.name);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "id")) {
		/**********************************************************************/
			mtp2Link.id = atoi(parm->val);
			SS7_DEBUG("Found an mtp2_link id = %d\n", mtp2Link.id);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "mtp1Id")) {
		/**********************************************************************/
			mtp2Link.mtp1Id = atoi(parm->val);
			SS7_DEBUG("Found an mtp2_link mtp1Id = %d\n", mtp2Link.mtp1Id);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "errorType")) {
		/**********************************************************************/
			ret = find_mtp2_error_type_in_map(parm->val);
			if (ret == -1) {
				SS7_ERROR("Found an invalid mtp2_link errorType = %s\n", parm->var);
				return FTDM_FAIL;
			} else {
				mtp2Link.errorType = sng_mtp2_error_type_map[ret].tril_type;
				SS7_DEBUG("Found an mtp2_link errorType = %s\n", sng_mtp2_error_type_map[ret].sng_type);
			}
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "lssuLength")) {
		/**********************************************************************/
			mtp2Link.lssuLength = atoi(parm->val);
			SS7_DEBUG("Found an mtp2_link lssuLength = %d\n", mtp2Link.lssuLength);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "linkType")) {
		/**********************************************************************/
			ret = find_link_type_in_map(parm->val);
			if (ret == -1) {
				SS7_ERROR("Found an invalid mtp2_link linkType = %s\n", parm->var);
				return FTDM_FAIL;
			} else {
				mtp2Link.linkType = sng_link_type_map[ret].tril_mtp2_type;
				SS7_DEBUG("Found an mtp2_link linkType = %s\n", sng_link_type_map[ret].sng_type);
			}
#ifdef SD_HSL
        } else if (!strcasecmp(parm->var, "sapType")) {
        /**********************************************************************/
            if (!strcasecmp(parm->val, "lsl")) {
                mtp2Link.sapType = SNGSS7_SAPTYPE_LSL;
                SS7_DEBUG("mtp2_link sapType = lsl\n");
            } else if (!strcasecmp(parm->val, "hsl")) {
                mtp2Link.sapType = SNGSS7_SAPTYPE_HSL;
                SS7_DEBUG("mtp2_link sapType = hsl\n");
            } else if (!strcasecmp(parm->val, "hsl-extended")) {
                mtp2Link.sapType = SNGSS7_SAPTYPE_HSL_EXT;
                SS7_DEBUG("mtp2_link sapType = hsl-extended\n");
            } else {
                SS7_DEBUG("Found an invalid mtp2_link sapType = %s\n", parm->val);
            }
        /**********************************************************************/
        } else if (!strcasecmp(parm->var, "thresCnt")) {
            mtp2Link.sdTe = atoi(parm->val);
            SS7_DEBUG("Found an mtp2 thresCnt = \"%d\"\n", mtp2Link.sdTe);
        } else if (!strcasecmp(parm->var, "upCnt")) {
            mtp2Link.sdUe = atoi(parm->val);
            SS7_DEBUG("Found an mtp2 upCnt = \"%d\"\n", mtp2Link.sdUe);
        } else if (!strcasecmp(parm->var, "deCnt")) {
            mtp2Link.sdDe = atoi(parm->val);
            SS7_DEBUG("Found an mtp2 deCnt = \"%d\"\n", mtp2Link.sdDe);
#endif
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t1")) {
		/**********************************************************************/
			mtp2Link.t1 = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t1 = \"%d\"\n",mtp2Link.t1);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t2")) {
		/**********************************************************************/
			mtp2Link.t2 = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t2 = \"%d\"\n",mtp2Link.t2);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t3")) {
		/**********************************************************************/
			mtp2Link.t3 = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t3 = \"%d\"\n",mtp2Link.t3);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t4n")) {
		/**********************************************************************/
			mtp2Link.t4n = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t4n = \"%d\"\n",mtp2Link.t4n);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t4e")) {
		/**********************************************************************/
			mtp2Link.t4e = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t4e = \"%d\"\n",mtp2Link.t4e);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t5")) {
		/**********************************************************************/
			mtp2Link.t5 = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t5 = \"%d\"\n",mtp2Link.t5);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t6")) {
		/**********************************************************************/
			mtp2Link.t6 = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t6 = \"%d\"\n",mtp2Link.t6);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t7")) {
		/**********************************************************************/
			mtp2Link.t7 = atoi(parm->val);
			SS7_DEBUG("Found an mtp2 t7 = \"%d\"\n",mtp2Link.t7);
		/**********************************************************************/
		} else {
		/**********************************************************************/
			SS7_ERROR("Found an invalid parameter \"%s\"!\n", parm->val);
			return FTDM_FAIL;
		/**********************************************************************/
		}

		/* move to the next parmeter */
		parm = parm + 1;

	/**************************************************************************/
	} /* for (i = 0; i < num_parms; i++) */

	/* store the link in global structure */
	ftmod_ss7_fill_in_mtp2_link(&mtp2Link);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp3_links(ftdm_conf_node_t *mtp3_links)
{
	ftdm_conf_node_t	*mtp3_link = NULL;

	/* confirm that we are looking at mtp3_links */
	if (strcasecmp(mtp3_links->name, "mtp3_links")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp3_links\"!\n",mtp3_links->name);
		return FTDM_FAIL;
	}  else {
		SS7_DEBUG("Parsing \"mtp3_links\"...\n");
	}

	/* extract the mtp_links */
	mtp3_link = mtp3_links->child;

	/* run through all of the links found  */
	while (mtp3_link != NULL) {
		/* try to the parse mtp_linkset */
		if (ftmod_ss7_parse_mtp3_link(mtp3_link)) {
			SS7_ERROR("Failed to parse \"mtp3_link\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next link */
		mtp3_link = mtp3_link->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp3_link(ftdm_conf_node_t *mtp3_link)
{
	sng_mtp3_link_t		 	mtp3Link;
	ftdm_conf_parameter_t	*parm = mtp3_link->parameters;
	int					 	num_parms = mtp3_link->n_parameters;
	int					 	i;
	int						ret;

	/* initalize the mtp3Link structure */
	memset(&mtp3Link, 0x0, sizeof(mtp3Link));

	/* confirm that we are looking at an mtp_link */
	if (strcasecmp(mtp3_link->name, "mtp3_link")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp3_link\"!\n",mtp3_link->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"mtp3_link\"...\n");
	}

	for (i = 0; i < num_parms; i++) {
	/**************************************************************************/
		if (!strcasecmp(parm->var, "name")) {
		/**********************************************************************/
			strcpy((char *)mtp3Link.name, parm->val);
			SS7_DEBUG("Found an mtp3_link named = %s\n", mtp3Link.name);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "id")) {
		/**********************************************************************/
			mtp3Link.id = atoi(parm->val);
			SS7_DEBUG("Found an mtp3_link id = %d\n", mtp3Link.id);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "mtp2Id")) {
		/**********************************************************************/
			mtp3Link.mtp2Id = atoi(parm->val);
			SS7_DEBUG("Found an mtp3_link mtp2Id = %d\n", mtp3Link.mtp2Id);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "mtp2ProcId")) {
		/**********************************************************************/
			mtp3Link.mtp2ProcId = atoi(parm->val);
			SS7_DEBUG("Found an mtp3_link mtp2ProcId = %d\n", mtp3Link.mtp2ProcId);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "priority")) {
		/**********************************************************************/
			mtp3Link.priority = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 Link priority = %d\n",mtp3Link.priority); 
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "linkType")) {
		/**********************************************************************/
			ret = find_link_type_in_map(parm->val);
			if (ret == -1) {
				SS7_ERROR("Found an invalid mtp3_link linkType = %s\n", parm->var);
				return FTDM_FAIL;
			} else {
				mtp3Link.linkType = sng_link_type_map[ret].tril_mtp3_type;
				SS7_DEBUG("Found an mtp3_link linkType = %s\n", sng_link_type_map[ret].sng_type);
			}
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "switchType")) {
		/**********************************************************************/
			ret = find_switch_type_in_map(parm->val);
			if (ret == -1) {
				SS7_ERROR("Found an invalid mtp3_link switchType = %s\n", parm->var);
				return FTDM_FAIL;
			} else {
				mtp3Link.switchType = sng_switch_type_map[ret].tril_mtp3_type;
				SS7_DEBUG("Found an mtp3_link switchType = %s\n", sng_switch_type_map[ret].sng_type);
			}
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "ssf")) {
		/**********************************************************************/
			ret = find_ssf_type_in_map(parm->val);
			if (ret == -1) {
				SS7_ERROR("Found an invalid mtp3_link ssf = %s\n", parm->var);
				return FTDM_FAIL;
			} else {
				mtp3Link.ssf = sng_ssf_type_map[ret].tril_type;
				SS7_DEBUG("Found an mtp3_link ssf = %s\n", sng_ssf_type_map[ret].sng_type);
			}
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "slc")) {
		/**********************************************************************/
			mtp3Link.slc = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 Link slc = %d\n",mtp3Link.slc);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "linkset")) {
		/**********************************************************************/
			mtp3Link.linkSetId = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 Link linkset = %d\n",mtp3Link.linkSetId);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t1")) {
		/**********************************************************************/
			mtp3Link.t1 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t1 = %d\n",mtp3Link.t1);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t2")) {
		/**********************************************************************/
			mtp3Link.t2 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t2 = %d\n",mtp3Link.t2);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t3")) {
		/**********************************************************************/
			mtp3Link.t3 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t3 = %d\n",mtp3Link.t3);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t4")) {
		/**********************************************************************/
			mtp3Link.t4 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t4 = %d\n",mtp3Link.t4);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t5")) {
		/**********************************************************************/
			mtp3Link.t5 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t5 = %d\n",mtp3Link.t5);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t7")) {
		/**********************************************************************/
			mtp3Link.t7 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t7 = %d\n",mtp3Link.t7);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t12")) {
		/**********************************************************************/
			mtp3Link.t12 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t12 = %d\n",mtp3Link.t12);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t13")) {
		/**********************************************************************/
			mtp3Link.t13 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t13 = %d\n",mtp3Link.t13);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t14")) {
		/**********************************************************************/
			mtp3Link.t14 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t14 = %d\n",mtp3Link.t14);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t17")) {
		/**********************************************************************/
			mtp3Link.t17 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t17 = %d\n",mtp3Link.t17);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t22")) {
		/**********************************************************************/
			mtp3Link.t22 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t22 = %d\n",mtp3Link.t22);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t23")) {
		/**********************************************************************/
			mtp3Link.t23 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t23 = %d\n",mtp3Link.t23);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t24")) {
		/**********************************************************************/
			mtp3Link.t24 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t24 = %d\n",mtp3Link.t24);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t31")) {
			mtp3Link.t31 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t31 = %d\n",mtp3Link.t31);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t32")) {
		/**********************************************************************/
			mtp3Link.t32 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t32 = %d\n",mtp3Link.t32);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t33")) {
		/**********************************************************************/
			mtp3Link.t33 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t33 = %d\n",mtp3Link.t33);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t34")) {
		/**********************************************************************/
			mtp3Link.t34 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t34 = %d\n",mtp3Link.t34);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t35")) {
		/**********************************************************************/
			mtp3Link.t35 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t35 = %d\n",mtp3Link.t35);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t36")) {
		/**********************************************************************/
			mtp3Link.t36 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t36 = %d\n",mtp3Link.t36);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "t37")) {
		/**********************************************************************/
			mtp3Link.t37 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t37 = %d\n",mtp3Link.t37);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "tcraft")) {
		/**********************************************************************/
			mtp3Link.tcraft = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 tcraft = %d\n",mtp3Link.tcraft);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "tflc")) {
		/**********************************************************************/
			mtp3Link.tflc = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 tflc = %d\n",mtp3Link.tflc);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "tbnd")) {
		/**********************************************************************/
			mtp3Link.tbnd = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 tbnd = %d\n",mtp3Link.tbnd);
		/**********************************************************************/
		} else {
		/**********************************************************************/
			SS7_ERROR("Found an invalid parameter %s!\n", parm->val);
			return FTDM_FAIL;
		/**********************************************************************/
		}

		/* move to the next parmeter */
		parm = parm + 1;

	/**************************************************************************/
	} /* for (i = 0; i < num_parms; i++) */

	/* store the link in global structure */
	ftmod_ss7_fill_in_mtp3_link(&mtp3Link);

	/* move the linktype, switchtype and ssf to the linkset */
	if (g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].linkType == 0) {
		g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].linkType = mtp3Link.linkType;
	} else if (g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].linkType != mtp3Link.linkType) {
		SS7_ERROR("Trying to add an MTP3 Link to a Linkset with a different linkType: mtp3.id=%d, mtp3.name=%s, mtp3.linktype=%d, linkset.id=%d, linkset.linktype=%d\n",
					mtp3Link.id, mtp3Link.name, mtp3Link.linkType,
					g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].id, g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].linkType);
		return FTDM_FAIL;
	} else {
		/* should print that all is ok...*/
	}

	if (g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].switchType == 0) {
		g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].switchType = mtp3Link.switchType;
	} else if (g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].switchType != mtp3Link.switchType) {
		SS7_ERROR("Trying to add an MTP3 Link to a Linkset with a different switchType: mtp3.id=%d, mtp3.name=%s, mtp3.switchtype=%d, linkset.id=%d, linkset.switchtype=%d\n",
					mtp3Link.id, mtp3Link.name, mtp3Link.switchType,
					g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].id, g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].switchType);
		return FTDM_FAIL;
	} else {
		/* should print that all is ok...*/
	}

	if (g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].ssf == 0) {
		g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].ssf = mtp3Link.ssf;
	} else if (g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].ssf != mtp3Link.ssf) {
		SS7_ERROR("Trying to add an MTP3 Link to a Linkset with a different ssf: mtp3.id=%d, mtp3.name=%s, mtp3.ssf=%d, linkset.id=%d, linkset.ssf=%d\n",
					mtp3Link.id, mtp3Link.name, mtp3Link.ssf,
					g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].id, g_ftdm_sngss7_data.cfg.mtpLinkSet[mtp3Link.linkSetId].ssf);
		return FTDM_FAIL;
	} else {
		/* should print that all is ok...*/
	}


	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_linksets(ftdm_conf_node_t *mtp_linksets)
{
	ftdm_conf_node_t	*mtp_linkset = NULL;

	/* confirm that we are looking at mtp_linksets */
	if (strcasecmp(mtp_linksets->name, "mtp_linksets")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp_linksets\"!\n",mtp_linksets->name);
		return FTDM_FAIL;
	}  else {
		SS7_DEBUG("Parsing \"mtp_linksets\"...\n");
	}

	/* extract the mtp_links */
	mtp_linkset = mtp_linksets->child;

	/* run through all of the mtp_linksets found  */
	while (mtp_linkset != NULL) {
		/* try to the parse mtp_linkset */
		if (ftmod_ss7_parse_mtp_linkset(mtp_linkset)) {
			SS7_ERROR("Failed to parse \"mtp_linkset\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next linkset */
		mtp_linkset = mtp_linkset->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_linkset(ftdm_conf_node_t *mtp_linkset)
{
	sng_link_set_t	 		mtpLinkSet;
	ftdm_conf_parameter_t	*parm = mtp_linkset->parameters;
	int					 	num_parms = mtp_linkset->n_parameters;
	int					 	i;

	/* initalize the mtpLinkSet structure */
	memset(&mtpLinkSet, 0x0, sizeof(mtpLinkSet));

	/* confirm that we are looking at mtp_linkset */
	if (strcasecmp(mtp_linkset->name, "mtp_linkset")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp_linkset\"!\n",mtp_linkset->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"mtp_linkset\"...\n");
	}

	/* extract all the information from the parameters */
	for (i = 0; i < num_parms; i++) {
	/**************************************************************************/

		if (!strcasecmp(parm->var, "name")) {
		/**********************************************************************/
			strcpy((char *)mtpLinkSet.name, parm->val);
			SS7_DEBUG("Found an mtpLinkSet named = %s\n", mtpLinkSet.name);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "id")) {
		/**********************************************************************/
			mtpLinkSet.id = atoi(parm->val);
			SS7_DEBUG("Found mtpLinkSet id = %d\n", mtpLinkSet.id);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "apc")) {
		/**********************************************************************/
			mtpLinkSet.apc = atoi(parm->val);
			SS7_DEBUG("Found mtpLinkSet apc = %d\n", mtpLinkSet.apc);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "minActive")) {
		/**********************************************************************/
			mtpLinkSet.minActive = atoi(parm->val);
			SS7_DEBUG("Found mtpLinkSet minActive = %d\n", mtpLinkSet.minActive);
		/**********************************************************************/
		} else {
		/**********************************************************************/
			SS7_ERROR("Found an invalid parameter \"%s\"!\n", parm->val);
			return FTDM_FAIL;
		/**********************************************************************/
		}

		/* move to the next parmeter */
		parm = parm + 1;
	/**************************************************************************/
	} /* for (i = 0; i < num_parms; i++) */

	ftmod_ss7_fill_in_mtpLinkSet(&mtpLinkSet);

	/* go through all the mtp3 links and fill in the apc */
	i = 1;
	while (i < (MAX_MTP_LINKS)) {
		if (g_ftdm_sngss7_data.cfg.mtp3Link[i].linkSetId == mtpLinkSet.id) {
			g_ftdm_sngss7_data.cfg.mtp3Link[i].apc = mtpLinkSet.apc;
		}

		i++;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_routes(ftdm_conf_node_t *mtp_routes, ftdm_span_t *span)
{
	ftdm_conf_node_t	*mtp_route = NULL;

	/* confirm that we are looking at an mtp_routes */
	if (strcasecmp(mtp_routes->name, "mtp_routes")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp_routes\"!\n",mtp_routes->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"mtp_routes\"...\n");
	}

	/* extract the mtp_routes */
	mtp_route = mtp_routes->child;

	while (mtp_route != NULL) {
		/* parse the found mtp_route */
		if (ftmod_ss7_parse_mtp_route(mtp_route, span)) {
			SS7_ERROR("Failed to parse \"mtp_route\"\n");
			return FTDM_FAIL;
		}

		/* go to the next mtp_route */
		mtp_route = mtp_route->next;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_route(ftdm_conf_node_t *mtp_route, ftdm_span_t *span)
{
	sng_route_t		 	mtpRoute;
	ftdm_conf_parameter_t	*parm = mtp_route->parameters;
	int					num_parms = mtp_route->n_parameters;
	int					i;
	sng_link_set_list_t		*lnkSet;

	ftdm_conf_node_t		*linkset;
	int					numLinks;

	memset(&mtpRoute, 0x0, sizeof(mtpRoute));
	if (strcasecmp(mtp_route->name, "mtp_route")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"mtp_route\"!\n",mtp_route->name);
		return FTDM_FAIL;
	} 

	/* Initializing max bucket size to 0 */
	mtpRoute.max_bkt_size = 0;
	SS7_DEBUG("Parsing \"mtp_route\"...\n");

	for (i = 0; i < num_parms; i++) {
		if (!strcasecmp(parm->var, "name")) {
			strcpy((char *)mtpRoute.name, parm->val);
			SS7_DEBUG("Found an mtpRoute named = %s\n", mtpRoute.name);
		} else if (!strcasecmp(parm->var, "id")) {
			mtpRoute.id = atoi(parm->val);
			SS7_DEBUG("Found an mtpRoute id = %d\n", mtpRoute.id);
		} else if (!strcasecmp(parm->var, "dpc")) {
			mtpRoute.dpc = atoi(parm->val);
			SS7_DEBUG("Found an mtpRoute dpc = %d\n", mtpRoute.dpc);
		} else if (!strcasecmp(parm->var, "isSTP")) {
			if (!strcasecmp(parm->val, "no")) {
				mtpRoute.isSTP = 0;
				SS7_DEBUG("Found an mtpRoute isSTP = no\n");
			} else if (!strcasecmp(parm->val, "yes")) {
				mtpRoute.isSTP = 1;
				SS7_DEBUG("Found an mtpRoute isSTP = yes\n");
			} else {
				SS7_ERROR("Found an invalid parameter for isSTP. Set to default value NO. %s!\n", parm->val);
				mtpRoute.isSTP = 0;
			}
		} else if (!strcasecmp(parm->var, "mtp3.t6")) {
			mtpRoute.t6 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t6 = %d\n",mtpRoute.t6);
		} else if (!strcasecmp(parm->var, "sls-bit-sel")) {
			/* possible values 1/2/4/8 */
			if ((mtpRoute.lsetSel == 1) || (mtpRoute.lsetSel == 2) || 
					(mtpRoute.lsetSel == 4) || (mtpRoute.lsetSel == 8) ) {
				SS7_DEBUG("Found an mtp3 lsetSel = %d\n",mtpRoute.lsetSel);
				mtpRoute.lsetSel = atoi(parm->val);
			} else {
				SS7_DEBUG("Found an Invalid mtp3 lsetSel = %d..possible values [1|2|4|8]\n",mtpRoute.lsetSel);
			}
		} else if (!strcasecmp(parm->var, "sls-link")) {
			/* When enabled, indicate that SLS mapping in ITU is done by equal distribution of SLS among links across linksets 
			 * Has to be true for Belgacom */
			if (ftdm_true(parm->val)) {
				mtpRoute.slsLnk = 1;
			} else {
				mtpRoute.slsLnk = 0;
			}
			SS7_DEBUG("Found an mtp3 slsLnk = %d\n",mtpRoute.slsLnk);
		} else if (!strcasecmp(parm->var, "tfr-req")) {
			/* Transfer Restrict Required flag */ 
			 /* Has to be true for Belgacom */
			if (ftdm_true(parm->val)) {
				mtpRoute.tfrReq = 1;
			} else {
				mtpRoute.tfrReq = 0;
			}
			SS7_DEBUG("Found an mtp3 tfrReq = %d\n",mtpRoute.tfrReq);
		} else if (!strcasecmp(parm->var, "acc-max-bucket")) {
			if (g_ftdm_sngss7_data.cfg.sng_acc) {
				mtpRoute.max_bkt_size = atoi(parm->val);
				SS7_DEBUG("Found Maximum Bucket Size = %d for Automatic Congestion Control feature\n", mtpRoute.max_bkt_size);
			} else {
				SS7_DEBUG("Found invalid configurable parameter Max Bucket Size as Automatic congestion feature is not enable\n");
			}
		} else if (!strcasecmp(parm->var, "mtp3.t8")) {
			mtpRoute.t8 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t8 = %d\n",mtpRoute.t8);
		} else if (!strcasecmp(parm->var, "mtp3.t10")) {
			mtpRoute.t10 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t10 = %d\n",mtpRoute.t10);
		} else if (!strcasecmp(parm->var, "mtp3.t11")) {
			mtpRoute.t11 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t11 = %d\n",mtpRoute.t11);
		} else if (!strcasecmp(parm->var, "mtp3.t15")) {
			mtpRoute.t15 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t15 = %d\n",mtpRoute.t15);
		} else if (!strcasecmp(parm->var, "mtp3.t16")) {
			mtpRoute.t16 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t16 = %d\n",mtpRoute.t16);
		} else if (!strcasecmp(parm->var, "mtp3.t18")) {
			mtpRoute.t18 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t18 = %d\n",mtpRoute.t18);
		} else if (!strcasecmp(parm->var, "mtp3.t19")) {
			mtpRoute.t19 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t19 = %d\n",mtpRoute.t19);
		} else if (!strcasecmp(parm->var, "mtp3.t21")) {
			mtpRoute.t21 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t21 = %d\n",mtpRoute.t21);
		} else if (!strcasecmp(parm->var, "mtp3.t25")) {
			mtpRoute.t25 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t25 = %d\n",mtpRoute.t25);
		} else if (!strcasecmp(parm->var, "mtp3.t26")) {
			mtpRoute.t26 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t26 = %d\n",mtpRoute.t26);
		} else if (!strcasecmp(parm->var, "dpc.t29")) {
			mtpRoute.t29 = atoi(parm->val);
			SS7_DEBUG("Found ACC t29 Timer = %d\n",mtpRoute.t29);
		} else if (!strcasecmp(parm->var, "dpc.t30")) {
			mtpRoute.t30 = atoi(parm->val);
			SS7_DEBUG("Found ACC t30 Timer = %d\n",mtpRoute.t30);
		} else if (!strcasecmp(parm->var, "dpc.acc-call-rate")) {
			mtpRoute.call_rate = atoi(parm->val);
			SS7_DEBUG("Found ACC call rate Timer = %d\n",mtpRoute.call_rate);
		} else {
			SS7_WARN("Found an invalid parameter \"%s\"!Ignoring it.\n", parm->var);
		}

		parm = parm + 1;
	}

	/* fill in the rest of the values in the mtpRoute struct  */
	mtpRoute.nwId = 0;
	mtpRoute.cmbLinkSetId = mtpRoute.id;

	/* parse in the list of linksets this route is reachable by */
	linkset = mtp_route->child->child;

	for (lnkSet = &mtpRoute.lnkSets; linkset != NULL;linkset = linkset->next) {
		/* extract the linkset Id */
		lnkSet->lsId = atoi(linkset->parameters->val);

		if (g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].id != 0) {
			SS7_DEBUG("Found mtpRoute linkset id = %d that is valid\n",lnkSet->lsId);
		} else {
			SS7_ERROR("Found mtpRoute linkset id = %d that is invalid\n",lnkSet->lsId);
			continue;
		}

		/* pull up the linktype, switchtype, and SSF from the linkset */
		mtpRoute.linkType 		= g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].linkType;
		mtpRoute.switchType 	= g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].switchType;
		mtpRoute.ssf 			= g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].ssf;
		
		/* extract the number of cmbLinkSetId aleady on this linkset */
		numLinks = g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks;
		
		/* add this routes cmbLinkSetId to the list */
		g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].links[numLinks] = mtpRoute.cmbLinkSetId;

		/* increment the number of cmbLinkSets on this linkset */
		g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks++;

		/* update the linked list */
		lnkSet->next = ftdm_malloc(sizeof(sng_link_set_list_t));
		lnkSet = lnkSet->next;
		lnkSet->lsId = 0;
		lnkSet->next = NULL;
	}

	ftmod_ss7_fill_in_mtp3_route(&mtpRoute);
	ftmod_ss7_fill_in_nsap(&mtpRoute);

	/* If automatic congestion is enable then only check fill in acc timer */
	if (g_ftdm_sngss7_data.cfg.sng_acc) {
		ftmod_ss7_fill_in_acc_timer(&mtpRoute, span);
	}
	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_isup_interfaces(ftdm_conf_node_t *isup_interfaces)
{
	ftdm_conf_node_t	*isup_interface = NULL;

	/* confirm that we are looking at isup_interfaces */
	if (strcasecmp(isup_interfaces->name, "isup_interfaces")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"isup_interfaces\"!\n",isup_interfaces->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"isup_interfaces\"...\n");
	}

	/* extract the isup_interfaces */
	isup_interface = isup_interfaces->child;

	while (isup_interface != NULL) {
		/* parse the found mtp_route */
		if (ftmod_ss7_parse_isup_interface(isup_interface)) {
			SS7_ERROR("Failed to parse \"isup_interface\"\n");
			return FTDM_FAIL;
		}

		/* go to the next mtp_route */
		isup_interface = isup_interface->next;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_isup_interface(ftdm_conf_node_t *isup_interface)
{
	sng_isup_inf_t			sng_isup;
	sng_isap_t				sng_isap;
	sng_link_set_list_t		*lnkSet;
	ftdm_conf_parameter_t	*parm = isup_interface->parameters;
	int						num_parms = isup_interface->n_parameters;
	int						i;
	int						ret;
	int						flag_def_rel_loc = 0;

	/* initalize the isup intf and isap structure */
	memset(&sng_isup, 0x0, sizeof(sng_isup));
	memset(&sng_isap, 0x0, sizeof(sng_isap));

	/* confirm that we are looking at an mtp_link */
	if (strcasecmp(isup_interface->name, "isup_interface")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"isup_interface\"!\n",isup_interface->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"isup_interface\"...\n");
	}


	for (i = 0; i < num_parms; i++, parm++) {
		if (!strcasecmp(parm->var, "name")) {
			strcpy((char *)sng_isup.name, parm->val);
			SS7_DEBUG("Found an isup_interface named = %s\n", sng_isup.name);
		} else if (!strcasecmp(parm->var, "id")) {
			sng_isup.id = atoi(parm->val);
			SS7_DEBUG("Found an isup id = %d\n", sng_isup.id);
		} else if (!strcasecmp(parm->var, "spc")) {
			sng_isup.spc = atoi(parm->val);
			SS7_DEBUG("Found an isup SPC = %d\n", sng_isup.spc);
		} else if (!strcasecmp(parm->var, "mtprouteId")) {
			sng_isup.mtpRouteId=atoi(parm->val);
			SS7_DEBUG("Found an isup mptRouteId = %d\n", sng_isup.mtpRouteId);
		} else if (!strcasecmp(parm->var, "ssf")) {
			ret = find_ssf_type_in_map(parm->val);
			if (ret == -1) {
				SS7_ERROR("Found an invalid isup ssf = %s\n", parm->var);
				return FTDM_FAIL;
			} else {
				sng_isup.ssf = sng_ssf_type_map[ret].tril_type;
				sng_isap.ssf = sng_ssf_type_map[ret].tril_type;
				SS7_DEBUG("Found an isup ssf = %s\n", sng_ssf_type_map[ret].sng_type);
			}
		} else if (!strcasecmp(parm->var, "pauseAction")) {
			sng_isup.pauseAction = atoi(parm->val);
			if ((SI_PAUSE_CLRDFLT == sng_isup.pauseAction ) || 
					(SI_PAUSE_CLRTRAN == sng_isup.pauseAction) || 
					(SI_PAUSE_CLRTMOUT == sng_isup.pauseAction)) {
				SS7_DEBUG("Found isup pauseAction = %d\n",sng_isup.pauseAction);
			}
		} else if (!strcasecmp(parm->var, "defRelLocation")) {
			flag_def_rel_loc = 1;
			sng_isap.defRelLocation = atoi(parm->val);
			SS7_DEBUG("Found isup defRelLocation = %d\n",sng_isap.defRelLocation);
		} else if (!strcasecmp(parm->var, "isup.t1")) {
			sng_isap.t1 = atoi(parm->val);
			SS7_DEBUG("Found isup t1 = %d\n",sng_isap.t1);
		} else if (!strcasecmp(parm->var, "isup.t2")) {
			sng_isap.t2 = atoi(parm->val);
			SS7_DEBUG("Found isup t2 = %d\n",sng_isap.t2);
		} else if (!strcasecmp(parm->var, "isup.t4")) {
			sng_isup.t4 = atoi(parm->val);
			SS7_DEBUG("Found isup t4 = %d\n",sng_isup.t4);
		} else if (!strcasecmp(parm->var, "isup.t5")) {
			sng_isap.t5 = atoi(parm->val);
			SS7_DEBUG("Found isup t5 = %d\n",sng_isap.t5);
		} else if (!strcasecmp(parm->var, "isup.t6")) {
			sng_isap.t6 = atoi(parm->val);
			SS7_DEBUG("Found isup t6 = %d\n",sng_isap.t6);
		} else if (!strcasecmp(parm->var, "isup.t7")) {
			sng_isap.t7 = atoi(parm->val);
			SS7_DEBUG("Found isup t7 = %d\n",sng_isap.t7);
		} else if (!strcasecmp(parm->var, "isup.t8")) {
			sng_isap.t8 = atoi(parm->val);
			SS7_DEBUG("Found isup t8 = %d\n",sng_isap.t8);
		} else if (!strcasecmp(parm->var, "isup.t9")) {
			sng_isap.t9 = atoi(parm->val);
			SS7_DEBUG("Found isup t9 = %d\n",sng_isap.t9);
		} else if (!strcasecmp(parm->var, "isup.t11")) {
			sng_isup.t11 = atoi(parm->val);
			SS7_DEBUG("Found isup t11 = %d\n",sng_isup.t11);
		} else if (!strcasecmp(parm->var, "isup.t18")) {
			sng_isup.t18 = atoi(parm->val);
			SS7_DEBUG("Found isup t18 = %d\n",sng_isup.t18);
		} else if (!strcasecmp(parm->var, "isup.t19")) {
			sng_isup.t19 = atoi(parm->val);
			SS7_DEBUG("Found isup t19 = %d\n",sng_isup.t19);
		} else if (!strcasecmp(parm->var, "isup.t20")) {
			sng_isup.t20 = atoi(parm->val);
			SS7_DEBUG("Found isup t20 = %d\n",sng_isup.t20);
		} else if (!strcasecmp(parm->var, "isup.t21")) {
			sng_isup.t21 = atoi(parm->val);
			SS7_DEBUG("Found isup t21 = %d\n",sng_isup.t21);
		} else if (!strcasecmp(parm->var, "isup.t22")) {
			sng_isup.t22 = atoi(parm->val);
			SS7_DEBUG("Found isup t22 = %d\n",sng_isup.t22);
		} else if (!strcasecmp(parm->var, "isup.t23")) {
			sng_isup.t23 = atoi(parm->val);
			SS7_DEBUG("Found isup t23 = %d\n",sng_isup.t23);
		} else if (!strcasecmp(parm->var, "isup.t24")) {
			sng_isup.t24 = atoi(parm->val);
			SS7_DEBUG("Found isup t24 = %d\n",sng_isup.t24);
		} else if (!strcasecmp(parm->var, "isup.t25")) {
			sng_isup.t25 = atoi(parm->val);
			SS7_DEBUG("Found isup t25 = %d\n",sng_isup.t25);
		} else if (!strcasecmp(parm->var, "isup.t26")) {
			sng_isup.t26 = atoi(parm->val);
			SS7_DEBUG("Found isup t26 = %d\n",sng_isup.t26);
		} else if (!strcasecmp(parm->var, "isup.t28")) {
			sng_isup.t28 = atoi(parm->val);
			SS7_DEBUG("Found isup t28 = %d\n",sng_isup.t28);
		} else if (!strcasecmp(parm->var, "isup.t29")) {
			sng_isup.t29 = atoi(parm->val);
			SS7_DEBUG("Found isup t29 = %d\n",sng_isup.t29);
		} else if (!strcasecmp(parm->var, "isup.t30")) {
			sng_isup.t30 = atoi(parm->val);
			SS7_DEBUG("Found isup t30 = %d\n",sng_isup.t30);
		} else if (!strcasecmp(parm->var, "isup.t31")) {
			sng_isap.t31 = atoi(parm->val);
			SS7_DEBUG("Found isup t31 = %d\n",sng_isap.t31);
		} else if (!strcasecmp(parm->var, "isup.t32")) {
			sng_isup.t32 = atoi(parm->val);
			SS7_DEBUG("Found isup t32 = %d\n",sng_isup.t32);
		} else if (!strcasecmp(parm->var, "isup.t33")) {
			sng_isap.t33 = atoi(parm->val);
			SS7_DEBUG("Found isup t33 = %d\n",sng_isap.t33);
		} else if (!strcasecmp(parm->var, "isup.t34")) {
			sng_isap.t34 = atoi(parm->val);
			SS7_DEBUG("Found isup t34 = %d\n",sng_isap.t34);
		} else if (!strcasecmp(parm->var, "isup.t36")) {
			sng_isap.t36 = atoi(parm->val);
			SS7_DEBUG("Found isup t36 = %d\n",sng_isap.t36);
		} else if (!strcasecmp(parm->var, "isup.t37")) {
			sng_isup.t37 = atoi(parm->val);
			SS7_DEBUG("Found isup t37 = %d\n",sng_isup.t37);
		} else if (!strcasecmp(parm->var, "isup.t38")) {
			sng_isup.t38 = atoi(parm->val);
			SS7_DEBUG("Found isup t38 = %d\n",sng_isup.t38);
		} else if (!strcasecmp(parm->var, "isup.t39")) {
			sng_isup.t39 = atoi(parm->val);
			SS7_DEBUG("Found isup t39 = %d\n",sng_isup.t39);
		} else if (!strcasecmp(parm->var, "isup.tccr")) {
			sng_isap.tccr = atoi(parm->val);
			SS7_DEBUG("Found isup tccr = %d\n",sng_isap.tccr);
		} else if (!strcasecmp(parm->var, "isup.tccrt")) {
			sng_isap.tccrt = atoi(parm->val);
			SS7_DEBUG("Found isup tccrt = %d\n",sng_isap.tccrt);
		} else if (!strcasecmp(parm->var, "isup.tex")) {
			sng_isap.tex = atoi(parm->val);
			SS7_DEBUG("Found isup tex = %d\n",sng_isap.tex);
		} else if (!strcasecmp(parm->var, "isup.tect")) {
			sng_isap.tect = atoi(parm->val);
			SS7_DEBUG("Found isup tect = %d\n",sng_isap.tect);
		} else if (!strcasecmp(parm->var, "isup.tcrm")) {
			sng_isap.tcrm = atoi(parm->val);
			SS7_DEBUG("Found isup tcrm = %d\n",sng_isap.tcrm);
		} else if (!strcasecmp(parm->var, "isup.tcra")) {
			sng_isap.tcra = atoi(parm->val);
			SS7_DEBUG("Found isup tcra = %d\n",sng_isap.tcra);
		} else if (!strcasecmp(parm->var, "isup.tfgr")) {
			sng_isup.tfgr = atoi(parm->val);
			SS7_DEBUG("Found isup tfgr = %d\n",sng_isup.tfgr);
		} else if (!strcasecmp(parm->var, "isup.trelrsp")) {
			sng_isap.trelrsp = atoi(parm->val);
			SS7_DEBUG("Found isup trelrsp = %d\n",sng_isap.trelrsp);
		} else if (!strcasecmp(parm->var, "isup.tfnlrelrsp")) {
			sng_isap.tfnlrelrsp = atoi(parm->val);
			SS7_DEBUG("Found isup tfnlrelrsp = %d\n",sng_isap.tfnlrelrsp);
		} else if (!strcasecmp(parm->var, "isup.tfnlrelrsp")) {
			sng_isap.tfnlrelrsp = atoi(parm->val);
			SS7_DEBUG("Found isup tfnlrelrsp = %d\n",sng_isap.tfnlrelrsp);
		} else if (!strcasecmp(parm->var, "isup.tpause")) {
			sng_isup.tpause = atoi(parm->val);
			SS7_DEBUG("Found isup tpause = %d\n",sng_isup.tpause);
		} else if (!strcasecmp(parm->var, "isup.tstaenq")) {
			sng_isup.tstaenq = atoi(parm->val);
			SS7_DEBUG("Found isup tstaenq = %d\n",sng_isup.tstaenq);
		} else {
			SS7_ERROR("Found an invalid parameter %s!Ignoring it.\n", parm->var);
		}
	} 

	/* default the interface to paused state */
	sngss7_set_flag(&sng_isup, SNGSS7_PAUSED);

	/* trickle down the SPC to all sub entities */
	lnkSet = &g_ftdm_sngss7_data.cfg.mtpRoute[sng_isup.mtpRouteId].lnkSets;
	while (lnkSet->next != NULL) {
		/* go through all the links and check if they belong to this linkset*/
		for (i = 1; i < MAX_MTP_LINKS;i++) {
			/* check if this link is in the linkset */
			if (g_ftdm_sngss7_data.cfg.mtp3Link[i].linkSetId == lnkSet->lsId) {
				/* fill in the spc */
				g_ftdm_sngss7_data.cfg.mtp3Link[i].spc = sng_isup.spc;
			}
		}

		lnkSet = lnkSet->next;
	} 

	if (!flag_def_rel_loc) {
		/* Add default value of release location */
		sng_isap.defRelLocation = ILOC_PRIVNETLU; 
	}

	/* pull values from the lower levels */
	sng_isap.switchType = g_ftdm_sngss7_data.cfg.mtpRoute[sng_isup.mtpRouteId].switchType;

	/* fill in the isap */
	ftmod_ss7_fill_in_isap(&sng_isap);

	/* pull values from the lower levels */
	sng_isup.isap 			= sng_isap.id;
	sng_isup.dpc 			= g_ftdm_sngss7_data.cfg.mtpRoute[sng_isup.mtpRouteId].dpc;
	sng_isup.switchType	= g_ftdm_sngss7_data.cfg.mtpRoute[sng_isup.mtpRouteId].switchType;
	sng_isup.nwId 		= g_ftdm_sngss7_data.cfg.mtpRoute[sng_isup.mtpRouteId].nwId;

	/* fill in the isup interface */
	ftmod_ss7_fill_in_isup_interface(&sng_isup);

	/* setup the self mtp3 route */
	i = sng_isup.mtpRouteId;
	if(ftmod_ss7_fill_in_self_route(sng_isup.spc,
									g_ftdm_sngss7_data.cfg.mtpRoute[i].linkType,
									g_ftdm_sngss7_data.cfg.mtpRoute[i].switchType,
									g_ftdm_sngss7_data.cfg.mtpRoute[i].ssf)) {

		SS7_ERROR("Failed to fill in self route structure!\n");
		return FTDM_FAIL;

	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_cc_spans(ftdm_conf_node_t *cc_spans)
{
	ftdm_conf_node_t	*cc_span = NULL;

	/* confirm that we are looking at cc_spans */
	if (strcasecmp(cc_spans->name, "cc_spans")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"cc_spans\"!\n",cc_spans->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"cc_spans\"...\n");
	}

	/* extract the cc_spans */
	cc_span = cc_spans->child;

	while (cc_span != NULL) {
		/* parse the found cc_span */
		if (ftmod_ss7_parse_cc_span(cc_span)) {
			SS7_ERROR("Failed to parse \"cc_span\"\n");
			return FTDM_FAIL;
		}

		/* go to the next cc_span */
		cc_span = cc_span->next;
	}

	return FTDM_SUCCESS;

}

/******************************************************************************/
static int ftmod_ss7_parse_cc_span(ftdm_conf_node_t *cc_span)
{
	sng_ccSpan_t		sng_ccSpan;
	ftdm_conf_parameter_t	*parm = cc_span->parameters;
	int			num_parms = cc_span->n_parameters;
	int			flag_clg_nadi = 0;
	int			flag_cld_nadi = 0;
	int			flag_rdnis_nadi = 0;
	int			flag_loc_nadi = 0;
	int			i;
	int			ret;
	ftdm_bool_t		lpa_on_cot = FTDM_FALSE;

	/* To get number of CIC's configured per DPC */
	nmb_cics_cfg = 0;
	/* initalize the ccSpan structure */
	memset(&sng_ccSpan, 0x0, sizeof(sng_ccSpan));


	/* confirm that we are looking at an mtp_link */
	if (strcasecmp(cc_span->name, "cc_span")) {
		SS7_ERROR("We're looking at \"%s\"...but we're supposed to be looking at \"cc_span\"!\n",cc_span->name);
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("Parsing \"cc_span\"...\n");
	}

	/* Backward compatible. 
     * If cpg_on_progress_media is not in the config file
     * default the cpg on progress_media to TRUE */
	sng_ccSpan.cpg_on_progress_media=FTDM_TRUE;
	/* If transparent_iam_max_size is not set in cc spans
     * use the global value */
 	sng_ccSpan.transparent_iam_max_size=g_ftdm_sngss7_data.cfg.transparent_iam_max_size;


	for (i = 0; i < num_parms; i++) {
	/**************************************************************************/

		/* try to match the parameter to what we expect */
		if (!strcasecmp(parm->var, "name")) {
		/**********************************************************************/
			strcpy((char *)sng_ccSpan.name, parm->val);
			SS7_DEBUG("Found an ccSpan named = %s\n", sng_ccSpan.name);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "id")) {
		/**********************************************************************/
			sng_ccSpan.id = atoi(parm->val);
			SS7_DEBUG("Found an ccSpan id = %d\n", sng_ccSpan.id);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "procid")) {
		/**********************************************************************/
			sng_ccSpan.procId = atoi(parm->val);
			SS7_DEBUG("Found an ccSpan procId = %d\n", sng_ccSpan.procId);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "ch_map")) {
		/**********************************************************************/
			strcpy(sng_ccSpan.ch_map, parm->val);
			SS7_DEBUG("Found channel map %s\n", sng_ccSpan.ch_map);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "typeCntrl")) {
		/**********************************************************************/
			ret = find_cic_cntrl_in_map(parm->val);
			if (ret == -1) {
				SS7_ERROR("Found an invalid ccSpan typeCntrl = %s\n", parm->var);
				return FTDM_FAIL;
			} else {
				sng_ccSpan.typeCntrl = sng_cic_cntrl_type_map[ret].tril_type;
				SS7_DEBUG("Found an ccSpan typeCntrl = %s\n", sng_cic_cntrl_type_map[ret].sng_type);
			}
		} else if (!strcasecmp(parm->var, "itx_auto_reply")) {
			sng_ccSpan.itx_auto_reply = ftdm_true(parm->val);
			SS7_DEBUG("Found itx_auto_reply %d\n", sng_ccSpan.itx_auto_reply);
		} else if (!strcasecmp(parm->var, "transparent_iam")) {
#ifndef HAVE_ZLIB
			SS7_CRIT("Cannot enable transparent IAM becauze zlib not installed\n");
#else
			sng_ccSpan.transparent_iam = ftdm_true(parm->val);
			SS7_DEBUG("Found transparent_iam %d\n", sng_ccSpan.transparent_iam);
#endif
		} else if (!strcasecmp(parm->var, "transparent_iam_max_size")) {
			sng_ccSpan.transparent_iam_max_size = atoi(parm->val);
			SS7_DEBUG("Found transparent_iam_max_size %d\n", sng_ccSpan.transparent_iam_max_size);
		} else if (!strcasecmp(parm->var, "cpg_on_progress_media")) {
			sng_ccSpan.cpg_on_progress_media = ftdm_true(parm->val);
			SS7_DEBUG("Found cpg_on_progress_media %d\n", sng_ccSpan.cpg_on_progress_media);
		} else if (!strcasecmp(parm->var, "ignore_alert_on_cpg")) {
			sng_ccSpan.ignore_alert_on_cpg = ftdm_true(parm->val);
			SS7_DEBUG("Found ignore_alert_on_cpg %d\n", sng_ccSpan.ignore_alert_on_cpg);
		} else if (!strcasecmp(parm->var, "cpg_on_progress")) {
			sng_ccSpan.cpg_on_progress = ftdm_true(parm->val);
			SS7_DEBUG("Found cpg_on_progress %d\n", sng_ccSpan.cpg_on_progress);
		} else if (!strcasecmp(parm->var, "cpg_on_alert")) {
			sng_ccSpan.cpg_on_alert = ftdm_true(parm->val);
			SS7_DEBUG("Found cpg_on_alert %d\n", sng_ccSpan.cpg_on_alert);
		} else if (!strcasecmp(parm->var, "cicbase")) {
		/**********************************************************************/
			sng_ccSpan.cicbase = atoi(parm->val);
			SS7_DEBUG("Found a cicbase = %d\n", sng_ccSpan.cicbase);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup_interface")) {
		/**********************************************************************/
			sng_ccSpan.isupInf = atoi(parm->val);
			SS7_DEBUG("Found an isup_interface = %d\n",sng_ccSpan.isupInf);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "min_digits")) {
		/**********************************************************************/
			sng_ccSpan.min_digits = atoi(parm->val);
			SS7_DEBUG("Found a min_digits = %d\n",sng_ccSpan.min_digits);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "bearcap_check_enable")) {
		/**********************************************************************/
			sng_ccSpan.bearcap_check = ftdm_true(parm->val); 
			SS7_DEBUG("Found a bearcap_check_enable = %d\n",sng_ccSpan.bearcap_check);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "clg_nadi")) {
		/**********************************************************************/
			/* throw the flag so that we know we got this optional parameter */
			flag_clg_nadi = 1;
			sng_ccSpan.clg_nadi = atoi(parm->val);
			SS7_DEBUG("Found default CLG_NADI parm->value = %d\n", sng_ccSpan.clg_nadi);
		} else if (!strcasecmp(parm->var, "cld_nadi")) {
			/* throw the flag so that we know we got this optional parameter */
			flag_cld_nadi = 1;
			sng_ccSpan.cld_nadi = atoi(parm->val);
			SS7_DEBUG("Found default CLD_NADI parm->value = %d\n", sng_ccSpan.cld_nadi);
		} else if (!strcasecmp(parm->var, "rdnis_nadi")) {
			/* throw the flag so that we know we got this optional parameter */
			flag_rdnis_nadi = 1;
			sng_ccSpan.rdnis_nadi = atoi(parm->val);
			SS7_DEBUG("Found default RDNIS_NADI parm->value = %d\n", sng_ccSpan.rdnis_nadi);
		} else if (!strcasecmp(parm->var, "obci_bita")) {
			if (*parm->val == '1') {
				sngss7_set_options(&sng_ccSpan, SNGSS7_ACM_OBCI_BITA);
				SS7_DEBUG("Found Optional Backwards Indicator: Bit A (early media) enable option\n");
			} else if (*parm->val == '0') {
				sngss7_clear_options(&sng_ccSpan, SNGSS7_ACM_OBCI_BITA);
				SS7_DEBUG("Found Optional Backwards Indicator: Bit A (early media) disable option\n");
			} else {
				SS7_DEBUG("Invalid parm->value for obci_bita option\n");
			}
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "loc_nadi")) {
			/* add location reference number */
			flag_loc_nadi = 1;
			sng_ccSpan.loc_nadi = atoi(parm->val);
			SS7_DEBUG("Found default LOC_NADI parm->value = %d\n", sng_ccSpan.loc_nadi);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "lpa_on_cot")) {
		/**********************************************************************/
			if (*parm->val == '1') {
				lpa_on_cot = FTDM_TRUE;
				sngss7_set_options(&sng_ccSpan, SNGSS7_LPA_FOR_COT);
				SS7_DEBUG("Found Tx LPA on COT enable option\n");
			} else if (*parm->val == '0') {
				lpa_on_cot = FTDM_FALSE;
				sngss7_clear_options(&sng_ccSpan, SNGSS7_LPA_FOR_COT);
				SS7_DEBUG("Found Tx LPA on COT disable option\n");
			} else {
				SS7_DEBUG("Invalid parm->value for lpa_on_cot option\n");
			}
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t3")) {
		/**********************************************************************/
			sng_ccSpan.t3 = atoi(parm->val);
			SS7_DEBUG("Found isup t3 = %d\n", sng_ccSpan.t3);
		} else if (!strcasecmp(parm->var, "isup.t10")) {
		/**********************************************************************/
			sng_ccSpan.t10 = atoi(parm->val);
			SS7_DEBUG("Found isup t10 = %d\n", sng_ccSpan.t10);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t12")) {
		/**********************************************************************/
			sng_ccSpan.t12 = atoi(parm->val);
			SS7_DEBUG("Found isup t12 = %d\n", sng_ccSpan.t12);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t13")) {
		/**********************************************************************/
			sng_ccSpan.t13 = atoi(parm->val);
			SS7_DEBUG("Found isup t13 = %d\n", sng_ccSpan.t13);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t14")) {
		/**********************************************************************/
			sng_ccSpan.t14 = atoi(parm->val);
			SS7_DEBUG("Found isup t14 = %d\n", sng_ccSpan.t14);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t15")) {
		/**********************************************************************/
			sng_ccSpan.t15 = atoi(parm->val);
			SS7_DEBUG("Found isup t15 = %d\n", sng_ccSpan.t15);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t16")) {
		/**********************************************************************/
			sng_ccSpan.t16 = atoi(parm->val);
			SS7_DEBUG("Found isup t16 = %d\n", sng_ccSpan.t16);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t17")) {
		/**********************************************************************/
			sng_ccSpan.t17 = atoi(parm->val);
			SS7_DEBUG("Found isup t17 = %d\n", sng_ccSpan.t17);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t35")) {
		/**********************************************************************/
			sng_ccSpan.t35 = atoi(parm->val);
			SS7_DEBUG("Found isup t35 = %d\n",sng_ccSpan.t35);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.t39")) {
		/**********************************************************************/
			sng_ccSpan.t39 = atoi(parm->val);
			SS7_DEBUG("Found isup t39 = %d\n",sng_ccSpan.t39);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "isup.tval")) {
		/**********************************************************************/
			sng_ccSpan.tval = atoi(parm->val);
			SS7_DEBUG("Found isup tval = %d\n", sng_ccSpan.tval);
		/**********************************************************************/
		} else {
		/**********************************************************************/
			SS7_ERROR("Found an invalid parameter %s!\n", parm->var);
			return FTDM_FAIL;
		/**********************************************************************/
		}

		/* move to the next parameter */
		parm = parm + 1;
	/**************************************************************************/
	} /* for (i = 0; i < num_parms; i++) */

	/* check if the user filled in a nadi value by looking at flag */
	if (!flag_cld_nadi) {
		/* default the nadi value to national */
		sng_ccSpan.cld_nadi = 0x03;
	}

	if (!flag_clg_nadi) {
		/* default the nadi value to national */
		sng_ccSpan.clg_nadi = 0x03;
	}

	if (!flag_rdnis_nadi) {
		/* default the nadi value to national */
		sng_ccSpan.rdnis_nadi = 0x03;
	}

	if (!flag_loc_nadi) {
		/* default the nadi value to national */
		sng_ccSpan.loc_nadi = 0x03;
	}

	/* pull up the SSF and Switchtype from the isup interface */
	sng_ccSpan.ssf = g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf].ssf;
	sng_ccSpan.switchType = g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf].switchType;

	/* add this span to our global listing */
	ftmod_ss7_fill_in_ccSpan(&sng_ccSpan);

	/* make sure the isup interface structure has something in it */
	if (g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf].id == 0) {
		/* fill in the id, so that we know it exists */
		g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf].id = sng_ccSpan.isupInf;

		/* default the status to PAUSED */
		sngss7_set_flag(&g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf], SNGSS7_PAUSED);
	}

	if (lpa_on_cot == FTDM_TRUE) {
		sngss7_set_options(&g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf], SNGSS7_LPA_FOR_COT);
		SS7_DEBUG("LPA on COT is set for interface %d\n", g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf].id);
	} else {
		SS7_DEBUG("LPA on COT is cleared for interface %d\n", g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf].id);
		sngss7_clear_options(&g_ftdm_sngss7_data.cfg.isupIntf[sng_ccSpan.isupInf], SNGSS7_LPA_FOR_COT);;
	}

	sng_acc_assign_max_bucket(sng_ccSpan.isupInf, nmb_cics_cfg);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_relay_channel(sng_relay_t *relay_channel)
{
	int i;

	/* go through all the existing channels and see if we find a match */
	i = 1;
	while (g_ftdm_sngss7_data.cfg.relay[i].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.relay[i].type == relay_channel->type) &&
			(g_ftdm_sngss7_data.cfg.relay[i].port == relay_channel->port) &&
			(g_ftdm_sngss7_data.cfg.relay[i].procId == relay_channel->procId) &&
			(!strcasecmp(g_ftdm_sngss7_data.cfg.relay[i].hostname, relay_channel->hostname))) {

			/* we have a match so break out of this loop */
			break;
		}
		/* move on to the next one */
		i++;
	}

		/* if the id value is 0 that means we didn't find the relay channel */
	if (g_ftdm_sngss7_data.cfg.relay[i].id  == 0) {
		relay_channel->id = i;
		SS7_DEBUG("found new relay channel on type:%d, hostname:%s, port:%d, procId:%d, id = %d\n",
					relay_channel->type,
					relay_channel->hostname,
					relay_channel->port,
					relay_channel->procId, 
					relay_channel->id);
		sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_RY_PRESENT);
	} else {
		relay_channel->id = i;
		SS7_DEBUG("found existing relay channel on type:%d, hostname:%s, port:%d, procId:%d, id = %d\n",
					relay_channel->type,
					relay_channel->hostname,
					relay_channel->port,
					relay_channel->procId, 
					relay_channel->id);
	}

	g_ftdm_sngss7_data.cfg.relay[i].id		= relay_channel->id;
	g_ftdm_sngss7_data.cfg.relay[i].type	= relay_channel->type;
	g_ftdm_sngss7_data.cfg.relay[i].port	= relay_channel->port;
	g_ftdm_sngss7_data.cfg.relay[i].procId	= relay_channel->procId;
	strcpy(g_ftdm_sngss7_data.cfg.relay[i].hostname, relay_channel->hostname);
	strcpy(g_ftdm_sngss7_data.cfg.relay[i].name, relay_channel->name);

	/* if this is THE listen channel grab the procId and use it */
	if (relay_channel->type == LRY_CT_TCP_LISTEN) {
		g_ftdm_sngss7_data.cfg.procId = relay_channel->procId;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp1_link(sng_mtp1_link_t *mtp1Link)
{
	int i = mtp1Link->id;

	/* check if this id value has been used already */
	if (g_ftdm_sngss7_data.cfg.mtp1Link[i].id == 0) {
		SS7_DEBUG("Found new MTP1 Link: id=%d, name=%s\n", mtp1Link->id, mtp1Link->name);
		sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP1_PRESENT);
	} else {
		SS7_DEBUG("Found an existing MTP1 Link: id=%d, name=%s (old name=%s)\n", 
					mtp1Link->id, 
					mtp1Link->name,
					g_ftdm_sngss7_data.cfg.mtp1Link[i].name);
	}

	/* copy over the values */
	strcpy((char *)g_ftdm_sngss7_data.cfg.mtp1Link[i].name, (char *)mtp1Link->name);

	g_ftdm_sngss7_data.cfg.mtp1Link[i].id		= mtp1Link->id;
	g_ftdm_sngss7_data.cfg.mtp1Link[i].span		= mtp1Link->span;
	g_ftdm_sngss7_data.cfg.mtp1Link[i].chan		= mtp1Link->chan;
    g_ftdm_sngss7_data.cfg.mtp1Link[i].ftdmchan     = mtp1Link->ftdmchan;
    strcpy(g_ftdm_sngss7_data.cfg.mtp1Link[i].span_name, mtp1Link->span_name);


	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp2_link(sng_mtp2_link_t *mtp2Link)
{
	int i = mtp2Link->id;

	/* check if this id value has been used already */
	if (g_ftdm_sngss7_data.cfg.mtp2Link[i].id == 0) {
		SS7_DEBUG("Found new MTP2 Link: id=%d, name=%s\n", mtp2Link->id, mtp2Link->name);
		sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP2_PRESENT);
	} else {
		SS7_DEBUG("Found an existing MTP2 Link: id=%d, name=%s (old name=%s)\n", 
					mtp2Link->id, 
					mtp2Link->name,
					g_ftdm_sngss7_data.cfg.mtp2Link[i].name);
	}

	/* copy over the values */
	strcpy((char *)g_ftdm_sngss7_data.cfg.mtp2Link[i].name, (char *)mtp2Link->name);

	g_ftdm_sngss7_data.cfg.mtp2Link[i].id			= mtp2Link->id;
	g_ftdm_sngss7_data.cfg.mtp2Link[i].lssuLength	= mtp2Link->lssuLength;
	g_ftdm_sngss7_data.cfg.mtp2Link[i].errorType	= mtp2Link->errorType;
	g_ftdm_sngss7_data.cfg.mtp2Link[i].linkType		= mtp2Link->linkType;
    g_ftdm_sngss7_data.cfg.mtp2Link[i].sapType      = mtp2Link->sapType;
	g_ftdm_sngss7_data.cfg.mtp2Link[i].mtp1Id		= mtp2Link->mtp1Id;
	g_ftdm_sngss7_data.cfg.mtp2Link[i].mtp1ProcId	= mtp2Link->mtp1ProcId;

	if ( mtp2Link->t1 != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t1		= mtp2Link->t1;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t1		= 500;
	}

	if ( mtp2Link->t2 != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t2		= mtp2Link->t2;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t2		= 250;
	}

	if ( mtp2Link->t3 != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t3		= mtp2Link->t3;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t3		= 20;
	}

	if ( mtp2Link->t4n != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t4n		= mtp2Link->t4n;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t4n		= 80;
	}

	if ( mtp2Link->t4e != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t4e		= mtp2Link->t4e;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t4e		= 5;
	}

	if ( mtp2Link->t5 != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t5		= mtp2Link->t5;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t5		= 1;
	}

	if ( mtp2Link->t6 != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t6		= mtp2Link->t6;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t6		= 60;
	}

	if ( mtp2Link->t7 != 0 ) {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t7		= mtp2Link->t7;
	}else {
		g_ftdm_sngss7_data.cfg.mtp2Link[i].t7		= 20;
	}

#ifdef SD_HSL
    if ( mtp2Link->t8 != 0 ) {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].t8       = mtp2Link->t8;
    }else {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].t8       = 1;    /* 100 msec */
    }

    /* Errored interval parameters from Q.703 A.10.2.5 */
    if ( mtp2Link->sdTe != 0 ) {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].sdTe     = mtp2Link->sdTe;
    }else {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].sdTe     = 793;
    }

    if ( mtp2Link->sdUe != 0 ) {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].sdUe     = mtp2Link->sdUe;
    }else {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].sdUe     = 198384;
    }

    if ( mtp2Link->sdDe != 0 ) {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].sdDe     = mtp2Link->sdDe;
    }else {
        g_ftdm_sngss7_data.cfg.mtp2Link[i].sdDe     = 11328;
    }
#endif


	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp3_link(sng_mtp3_link_t *mtp3Link)
{
	int i = mtp3Link->id;

	/* check if this id value has been used already */
	if (g_ftdm_sngss7_data.cfg.mtp3Link[i].id == 0) {
		SS7_DEBUG("Found new MTP3 Link: id=%d, name=%s\n", mtp3Link->id, mtp3Link->name);
		sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_MTP3_PRESENT);
	} else {
		SS7_DEBUG("Found an existing MTP3 Link: id=%d, name=%s (old name=%s)\n", 
					mtp3Link->id, 
					mtp3Link->name,
					g_ftdm_sngss7_data.cfg.mtp3Link[i].name);
	}

	/* copy over the values */
	strcpy((char *)g_ftdm_sngss7_data.cfg.mtp3Link[i].name, (char *)mtp3Link->name);

	g_ftdm_sngss7_data.cfg.mtp3Link[i].id			= mtp3Link->id;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].priority		= mtp3Link->priority;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].linkType		= mtp3Link->linkType;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].switchType	= mtp3Link->switchType;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].apc			= mtp3Link->apc;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].spc			= mtp3Link->spc;	/* unknown at this time */
	g_ftdm_sngss7_data.cfg.mtp3Link[i].ssf			= mtp3Link->ssf;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].slc			= mtp3Link->slc;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].linkSetId	= mtp3Link->linkSetId;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].mtp2Id		= mtp3Link->mtp2Id;
	g_ftdm_sngss7_data.cfg.mtp3Link[i].mtp2ProcId	= mtp3Link->mtp2ProcId;

	if (mtp3Link->t1 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t1		= mtp3Link->t1;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t1		= 8;
	}
	if (mtp3Link->t2 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t2		= mtp3Link->t2;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t2		= 14;
	}
	if (mtp3Link->t3 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t3		= mtp3Link->t3;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t3		= 8;
	}
	if (mtp3Link->t4 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t4		= mtp3Link->t4;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t4		= 8;
	}
	if (mtp3Link->t5 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t5		= mtp3Link->t5;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t5		= 8;
	}
	if (mtp3Link->t7 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t7		= mtp3Link->t7;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t7		= 15;
	}
	if (mtp3Link->t12 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t12		= mtp3Link->t12;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t12		= 15;
	}
	if (mtp3Link->t13 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t13		= mtp3Link->t13;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t13		= 15;
	}
	if (mtp3Link->t14 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t14		= mtp3Link->t14;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t14		= 30;
	}
	if (mtp3Link->t17 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t17		= mtp3Link->t17;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t17		= 15;
	}
	if (mtp3Link->t22 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t22		= mtp3Link->t22;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t22		= 1800;
	}
	if (mtp3Link->t23 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t23		= mtp3Link->t23;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t23		= 1800;
	}
	if (mtp3Link->t24 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t24		= mtp3Link->t24;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t24		= 5;
	}
	if (mtp3Link->t31 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t31		= mtp3Link->t31;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t31		= 50;
	}
	if (mtp3Link->t32 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t32		= mtp3Link->t32;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t32		= 120;
	}
	if (mtp3Link->t33 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t33		= mtp3Link->t33;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t33		= 3000;
	}
	if (mtp3Link->t34 != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t34		= mtp3Link->t34;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].t34		= 600;
	}
	if (mtp3Link->tbnd != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].tbnd		= mtp3Link->tbnd;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].tbnd		= 30000;
	}
	if (mtp3Link->tflc != 0) {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].tflc		= mtp3Link->tflc;
	} else {
		g_ftdm_sngss7_data.cfg.mtp3Link[i].tflc		= 300;
	}
	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtpLinkSet(sng_link_set_t *mtpLinkSet)
{
	int	i = mtpLinkSet->id;

	strncpy((char *)g_ftdm_sngss7_data.cfg.mtpLinkSet[i].name, (char *)mtpLinkSet->name, MAX_NAME_LEN-1);

	g_ftdm_sngss7_data.cfg.mtpLinkSet[i].id			= mtpLinkSet->id;
	g_ftdm_sngss7_data.cfg.mtpLinkSet[i].apc		= mtpLinkSet->apc;

	/* these values are filled in as we find routes and start allocating cmbLinkSetIds */
	g_ftdm_sngss7_data.cfg.mtpLinkSet[i].minActive	= mtpLinkSet->minActive;
	g_ftdm_sngss7_data.cfg.mtpLinkSet[i].numLinks	= 0;
	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp3_route(sng_route_t *mtp3_route)
{
	sng_link_set_list_t	*lnkSet = NULL;
	int 				i = mtp3_route->id;
	int					tmp = 0;


	/* check if this id value has been used already */
	if (g_ftdm_sngss7_data.cfg.mtpRoute[i].id == 0) {
		SS7_DEBUG("Found new MTP3 Link: id=%d, name=%s\n", mtp3_route->id, mtp3_route->name);
	} else {
		SS7_DEBUG("Found an existing MTP3 Link: id=%d, name=%s (old name=%s)\n", 
					mtp3_route->id, 
					mtp3_route->name,
					g_ftdm_sngss7_data.cfg.mtpRoute[i].name);
	}

	/* fill in the cmbLinkSet in the linkset structure */
	lnkSet = &mtp3_route->lnkSets;

	tmp = g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks;
	g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].links[tmp] = mtp3_route->cmbLinkSetId;
	g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks++;

	while (lnkSet->next != NULL) {
		tmp = g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks;
		g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].links[tmp] = mtp3_route->cmbLinkSetId;
		g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks++;

		lnkSet = lnkSet->next;
	}

	strcpy((char *)g_ftdm_sngss7_data.cfg.mtpRoute[i].name, (char *)mtp3_route->name);

	g_ftdm_sngss7_data.cfg.mtpRoute[i].id			= mtp3_route->id;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].dpc			= mtp3_route->dpc;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].linkType		= mtp3_route->linkType;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].switchType	= mtp3_route->switchType;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].cmbLinkSetId	= mtp3_route->cmbLinkSetId;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].isSTP		= mtp3_route->isSTP;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].nwId			= mtp3_route->nwId;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].lnkSets		= mtp3_route->lnkSets;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].ssf			= mtp3_route->ssf;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].dir			= SNG_RTE_DN;
	if (mtp3_route->t6 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t6		= mtp3_route->t6;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t6		= 8;
	}
	if (mtp3_route->t8 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t8		= mtp3_route->t8;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t8		= 12;
	}
	if (mtp3_route->t10 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t10		= mtp3_route->t10;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t10	   = 300;
	}
	if (mtp3_route->t11 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t11		= mtp3_route->t11;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t11	   = 300;
	}
	if (mtp3_route->t15 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t15		= mtp3_route->t15;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t15	   = 30;
	}
	if (mtp3_route->t16 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t16		= mtp3_route->t16;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t16	   = 20;
	}
	if (mtp3_route->t18 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t18		= mtp3_route->t18;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t18	   = 200;
	}
	if (mtp3_route->t19 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t19		= mtp3_route->t19;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t19	   = 690;
	}
	if (mtp3_route->t21 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t21		= mtp3_route->t21;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t21	   = 650; 
	}
	if (mtp3_route->t25 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t25		= mtp3_route->t25;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t25	   = 100;
	}
	if (mtp3_route->t26 != 0) {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t26		= mtp3_route->t26;
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].t26	   = 100;
	}

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_self_route(int spc, int linkType, int switchType, int ssf)
{
	int i = 1;

	while (i < (MAX_MTP_ROUTES)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[i].dpc == spc) &&
				(g_ftdm_sngss7_data.cfg.mtpRoute[i].switchType == switchType)) {
			/* we have a match so break out of this loop */
			break;
		}
		/* move on to the next one */
		i++;
	}

	if (g_ftdm_sngss7_data.cfg.mtpRoute[i].id == 0) {
		/* this is a new route...find the first free spot */
		i = 1;
		while (i < (MAX_MTP_ROUTES)) {
			if (g_ftdm_sngss7_data.cfg.mtpRoute[i].id == 0) {
				/* we have a match so break out of this loop */
				break;
			}
			/* move on to the next one */
			i++;
		}
		g_ftdm_sngss7_data.cfg.mtpRoute[i].id = i;
		SS7_DEBUG("found new mtp3 self route\n");
	} else {
		g_ftdm_sngss7_data.cfg.mtpRoute[i].id = i;
		SS7_DEBUG("found existing mtp3 self route\n");
	}

	strncpy((char *)g_ftdm_sngss7_data.cfg.mtpRoute[i].name, "self-route", MAX_NAME_LEN-1);

	g_ftdm_sngss7_data.cfg.mtpRoute[i].id			= i;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].dpc			= spc;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].linkType		= linkType;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].switchType	= switchType;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].cmbLinkSetId	= i;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].isSTP		= 0;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].ssf			= ssf;
	g_ftdm_sngss7_data.cfg.mtpRoute[i].dir			= SNG_RTE_UP;
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

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_nsap(sng_route_t *mtp3_route)
{
	int i;

	/* go through all the existing interfaces and see if we find a match */
	i = 1;
	while (g_ftdm_sngss7_data.cfg.nsap[i].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.nsap[i].linkType == mtp3_route->linkType) &&
			(g_ftdm_sngss7_data.cfg.nsap[i].switchType == mtp3_route->switchType) &&
			(g_ftdm_sngss7_data.cfg.nsap[i].ssf == mtp3_route->ssf)) {

			/* we have a match so break out of this loop */
			break;
		}
		/* move on to the next one */
		i++;
	}

	if (g_ftdm_sngss7_data.cfg.nsap[i].id == 0) {
		g_ftdm_sngss7_data.cfg.nsap[i].id = i;
		SS7_DEBUG("found new mtp3_isup interface, id is = %d\n", g_ftdm_sngss7_data.cfg.nsap[i].id);
	} else {
		g_ftdm_sngss7_data.cfg.nsap[i].id = i;
		SS7_DEBUG("found existing mtp3_isup interface, id is = %d\n", g_ftdm_sngss7_data.cfg.nsap[i].id);
	}
	
	g_ftdm_sngss7_data.cfg.nsap[i].spId			= g_ftdm_sngss7_data.cfg.nsap[i].id;
	g_ftdm_sngss7_data.cfg.nsap[i].suId			= g_ftdm_sngss7_data.cfg.nsap[i].id;
	g_ftdm_sngss7_data.cfg.nsap[i].nwId			= mtp3_route->nwId;
	g_ftdm_sngss7_data.cfg.nsap[i].linkType		= mtp3_route->linkType;
	g_ftdm_sngss7_data.cfg.nsap[i].switchType	= mtp3_route->switchType;
	g_ftdm_sngss7_data.cfg.nsap[i].ssf			= mtp3_route->ssf;

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_isup_interface(sng_isup_inf_t *sng_isup)
{
	int i = sng_isup->id;

	/* check if this id value has been used already */
	if (g_ftdm_sngss7_data.cfg.isupIntf[i].id == 0) {
		SS7_DEBUG("Found new ISUP Interface: id=%d, name=%s\n", sng_isup->id, sng_isup->name);
		sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_ISUP_PRESENT);
	} else {
		SS7_DEBUG("Found an existing ISUP Interface: id=%d, name=%s (old name=%s)\n", 
					sng_isup->id, 
					sng_isup->name,
					g_ftdm_sngss7_data.cfg.isupIntf[i].name);
	}

	strncpy((char *)g_ftdm_sngss7_data.cfg.isupIntf[i].name, (char *)sng_isup->name, MAX_NAME_LEN-1);

	g_ftdm_sngss7_data.cfg.isupIntf[i].id			= sng_isup->id;
	g_ftdm_sngss7_data.cfg.isupIntf[i].mtpRouteId	= sng_isup->mtpRouteId;
	g_ftdm_sngss7_data.cfg.isupIntf[i].nwId			= sng_isup->nwId;
	g_ftdm_sngss7_data.cfg.isupIntf[i].dpc			= sng_isup->dpc;
	g_ftdm_sngss7_data.cfg.isupIntf[i].spc			= sng_isup->spc;
	g_ftdm_sngss7_data.cfg.isupIntf[i].switchType	= sng_isup->switchType;
	g_ftdm_sngss7_data.cfg.isupIntf[i].ssf			= sng_isup->ssf;
	g_ftdm_sngss7_data.cfg.isupIntf[i].isap			= sng_isup->isap;
	g_ftdm_sngss7_data.cfg.isupIntf[i].options		= sng_isup->options;
	if (sng_isup->pauseAction != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].pauseAction	= sng_isup->pauseAction;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].pauseAction	= SI_PAUSE_CLRTRAN ;
	}
	if (sng_isup->t4 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t4		= sng_isup->t4;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t4		= 3000;
	}
	if (sng_isup->t11 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t11		= sng_isup->t11;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t11		= 170;
	}
	if (sng_isup->t18 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t18		= sng_isup->t18;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t18		= 300;
	}
	if (sng_isup->t19 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t19		= sng_isup->t19;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t19		= 3000;
	}
	if (sng_isup->t20 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t20		= sng_isup->t20;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t20		= 300;
	}
	if (sng_isup->t21 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t21		= sng_isup->t21;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t21		= 3000;
	}
	if (sng_isup->t22 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t22		= sng_isup->t22;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t22		= 300;
	}
	if (sng_isup->t23 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t23		= sng_isup->t23;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t23		= 3000;
	}
	if (sng_isup->t24 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t24		= sng_isup->t24;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t24		= 10;
	}
	if (sng_isup->t25 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t25		= sng_isup->t25;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t25		= 20;
	}
	if (sng_isup->t26 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t26		= sng_isup->t26;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t26		= 600;
	}
	if (sng_isup->t28 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t28		= sng_isup->t28;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t28		= 100;
	}
	if (sng_isup->t29 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t29		= sng_isup->t29;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t29		= 6;
	}
	if (sng_isup->t30 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t30		= sng_isup->t30;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t30		= 50;
	}
	if (sng_isup->t32 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t32		= sng_isup->t32;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t32		= 30;
	}
	if (sng_isup->t37 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t37		= sng_isup->t37;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t37		= 20;
	}
	if (sng_isup->t38 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t38		= sng_isup->t38;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t38		= 1200;
	}
	if (sng_isup->t39 != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t39		= sng_isup->t39;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].t39		= 300;
	}
	if (sng_isup->tfgr != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].tfgr		= sng_isup->tfgr;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].tfgr		= 50;
	}
	if (sng_isup->tpause != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].tpause	= sng_isup->tpause;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].tpause	= 3000;
	}
	if (sng_isup->tstaenq != 0) {
		g_ftdm_sngss7_data.cfg.isupIntf[i].tstaenq	= sng_isup->tstaenq;
	} else {
		g_ftdm_sngss7_data.cfg.isupIntf[i].tstaenq	= 5000;
	}

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_isap(sng_isap_t *sng_isap)
{
	int i;

	/* go through all the existing interfaces and see if we find a match */
	i = 1;
	while (g_ftdm_sngss7_data.cfg.isap[i].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.isap[i].switchType == sng_isap->switchType) &&
			       (g_ftdm_sngss7_data.cfg.isap[i].ssf == sng_isap->ssf)) {

			/* we have a match so break out of this loop */
			break;
		}
		/* move on to the next one */
		i++;
	}

	if (g_ftdm_sngss7_data.cfg.isap[i].id == 0) {
		sng_isap->id = i;
		SS7_DEBUG("found new isup to cc interface, id is = %d\n", sng_isap->id);
	} else {
		sng_isap->id = i;
		SS7_DEBUG("found existing isup to cc interface, id is = %d\n", sng_isap->id);
	}

	g_ftdm_sngss7_data.cfg.isap[i].id 			= sng_isap->id;
	g_ftdm_sngss7_data.cfg.isap[i].suId			= sng_isap->id;
	g_ftdm_sngss7_data.cfg.isap[i].spId			= sng_isap->id;
	g_ftdm_sngss7_data.cfg.isap[i].switchType	= sng_isap->switchType;
	g_ftdm_sngss7_data.cfg.isap[i].ssf			= sng_isap->ssf;
	g_ftdm_sngss7_data.cfg.isap[i].defRelLocation		= sng_isap->defRelLocation;

	if (sng_isap->t1 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t1		= sng_isap->t1;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t1		= 150;
	}
	if (sng_isap->t2 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t2		= sng_isap->t2;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t2		= 1800;
	}
	if (sng_isap->t5 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t5		= sng_isap->t5;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t5		= 3000;
	}
	if (sng_isap->t6 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t6		= sng_isap->t6;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t6		= 600;
	}
	if (sng_isap->t7 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t7		= sng_isap->t7;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t7		= 200;
	}
	if (sng_isap->t8 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t8		= sng_isap->t8;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t8		= 100;
	}
	if (sng_isap->t9 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t9		= sng_isap->t9;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t9		= 1800;
	}
	if (sng_isap->t27 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t27		= sng_isap->t27;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t27		= 2400;
	}
	if (sng_isap->t31 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t31		= sng_isap->t31;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t31		= 3650;
	}
	if (sng_isap->t33 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t33		= sng_isap->t33;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t33		= 120;
	}
	if (sng_isap->t34 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t34		= sng_isap->t34;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t34		= 40;
	}
	if (sng_isap->t36 != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].t36		= sng_isap->t36;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].t36		= 120;
	}
	if (sng_isap->tccr != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].tccr		= sng_isap->tccr;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].tccr		= 200;
	}
	if (sng_isap->tccrt != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].tccrt	= sng_isap->tccrt;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].tccrt	= 20;
	}
	if (sng_isap->tex != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].tex		= sng_isap->tex;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].tex		= 1000;
	}
	if (sng_isap->tcrm != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].tcrm		= sng_isap->tcrm;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].tcrm		= 30;
	}
	if (sng_isap->tcra != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].tcra		= sng_isap->tcra;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].tcra		= 100;
	}
	if (sng_isap->tect != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].tect		= sng_isap->tect;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].tect		= 10;
	}
	if (sng_isap->trelrsp != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].trelrsp	= sng_isap->trelrsp;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].trelrsp	= 10;
	}
	if (sng_isap->tfnlrelrsp != 0) {
		g_ftdm_sngss7_data.cfg.isap[i].tfnlrelrsp	= sng_isap->tfnlrelrsp;
	} else {
		g_ftdm_sngss7_data.cfg.isap[i].tfnlrelrsp	= 10;
	}

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_ccSpan(sng_ccSpan_t *ccSpan)
{
	sng_timeslot_t		timeslot;
	sngss7_chan_data_t	*ss7_info = NULL;
	int			x;
	int			count = 1;
	int			flag;
	int 			trans_idx = 0;
	int 			idx = 0;

	while (ccSpan->ch_map[0] != '\0') {
	/**************************************************************************/

		/* pull out the next timeslot */
		if (ftmod_ss7_next_timeslot(ccSpan->ch_map, &timeslot)) {
			SS7_ERROR("Failed to parse the channel map!\n");
			return FTDM_FAIL;
		}

		/* find a spot for this circuit in the global structure */
		x = ftmod_ss7_get_circuit_start_range(ccSpan->procId);
		flag = 0;
		while (flag == 0) {
			/* Skip channel if it is a transparent one */
			if (timeslot.transparent) {
				for (idx = 0; idx < MAX_TRANSPARENT_CKTS; idx++) {
					if (!g_ftdm_sngss7_data.cfg.transCkt[idx].ccSpan_id) {
						break;
					}

					/* since we always try to create circuit id for each span thus, in case of transparent
					 * circuit if the circuit is already configured please increment count by one in order
					 * to avoid invalid circuit ID creation
					 * for e.g. if channel 14 & 15 are transparent and if circuit id for channel 16 is
					 * already created then in case if the same code is repeated for another span
					 * configuration count must increase when same channel circuit creation is repeated
					 * NOTE: Count must increase as count is consider as channel ID */
					if ((g_ftdm_sngss7_data.cfg.transCkt[idx].ccSpan_id == ccSpan->id) &&
					    (g_ftdm_sngss7_data.cfg.transCkt[idx].chan_id == timeslot.channel) &&
					    (g_ftdm_sngss7_data.cfg.transCkt[idx].next_cktId = (x))) {
						count++;
						break;
					}
				}
				break;
			}
		/**********************************************************************/
			/* check the id value ( 0 = new, 0 > circuit can be existing) */
			if (g_ftdm_sngss7_data.cfg.isupCkt[x].id == 0) {
				if (timeslot.channel > count) {
					count = timeslot.channel;
				}

				nmb_cics_cfg++;
				/* we're at the end of the list of circuitsl aka this is new */
				SS7_DEBUG("Found a new circuit %d, ccSpanId=%d, chan=%d\n",
						x,
						ccSpan->id,
						count);

				/* throw the flag to end the loop */
				flag = 1;
			} else {
				/* check the ccspan.id and chan to see if the circuit already exists */
				if ((g_ftdm_sngss7_data.cfg.isupCkt[x].ccSpanId == ccSpan->id) &&
					(g_ftdm_sngss7_data.cfg.isupCkt[x].chan == count)) {

					/* we are processing a circuit that already exists */
					SS7_DEVEL_DEBUG("Found an existing circuit %d, ccSpanId=%d, chan%d\n",
								x, 
								ccSpan->id, 
								count);
	
					/* throw the flag to end the loop */
					flag = 1;

					/* not supporting reconfig at this time */
					SS7_DEVEL_DEBUG("Not supporting ckt reconfig at this time!\n");
					goto move_along;
				} else {
					/* this is not the droid you are looking for */
					x++;
				}
			}
		/**********************************************************************/
		} /* while (flag == 0) */

		/* Skip CIC configuration for transparent channel */
		if (timeslot.transparent) {
			g_ftdm_sngss7_data.cfg.transCkt[trans_idx].ccSpan_id = ccSpan->id;
			g_ftdm_sngss7_data.cfg.transCkt[trans_idx].chan_id = timeslot.channel;
			g_ftdm_sngss7_data.cfg.transCkt[trans_idx++].next_cktId = x;
			SS7_DEBUG("Ignoring circuit as channel configured as transparent %d\n", timeslot.channel);
			continue;
		}

		/* prepare the global info sturcture */
		ss7_info = ftdm_calloc(1, sizeof(sngss7_chan_data_t));
		ss7_info->ftdmchan = NULL;
		if (ftdm_queue_create(&ss7_info->event_queue, SNGSS7_CHAN_EVENT_QUEUE_SIZE) != FTDM_SUCCESS) {
			SS7_CRITICAL("Failed to create ss7 cic event queue\n");
		}
		ss7_info->circuit = &g_ftdm_sngss7_data.cfg.isupCkt[x];

		g_ftdm_sngss7_data.cfg.isupCkt[x].obj			= ss7_info;

		/* fill in the rest of the global structure */
		g_ftdm_sngss7_data.cfg.isupCkt[x].procId	  	= ccSpan->procId;
		g_ftdm_sngss7_data.cfg.isupCkt[x].id		  	= x;
		g_ftdm_sngss7_data.cfg.isupCkt[x].ccSpanId		= ccSpan->id;
		g_ftdm_sngss7_data.cfg.isupCkt[x].span			= 0;
		g_ftdm_sngss7_data.cfg.isupCkt[x].chan			= count;

		if (timeslot.siglink) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].type		= SNG_CKT_SIG;
		} else if (timeslot.gap) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].type		= SNG_CKT_HOLE;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].type		= SNG_CKT_VOICE;

			/* throw the flag to indicate that we need to start call control */
			sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_CC_PRESENT);
		}

		if (timeslot.channel) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].cic		= ccSpan->cicbase;
			ccSpan->cicbase++;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].cic		= 0;
		}

		g_ftdm_sngss7_data.cfg.isupCkt[x].infId						= ccSpan->isupInf;
		g_ftdm_sngss7_data.cfg.isupCkt[x].typeCntrl					= ccSpan->typeCntrl;
		g_ftdm_sngss7_data.cfg.isupCkt[x].ssf						= ccSpan->ssf;
		g_ftdm_sngss7_data.cfg.isupCkt[x].cld_nadi					= ccSpan->cld_nadi;
		g_ftdm_sngss7_data.cfg.isupCkt[x].clg_nadi					= ccSpan->clg_nadi;
		g_ftdm_sngss7_data.cfg.isupCkt[x].rdnis_nadi					= ccSpan->rdnis_nadi;
		g_ftdm_sngss7_data.cfg.isupCkt[x].loc_nadi					= ccSpan->loc_nadi;
		g_ftdm_sngss7_data.cfg.isupCkt[x].options					= ccSpan->options;
		g_ftdm_sngss7_data.cfg.isupCkt[x].switchType					= ccSpan->switchType;
		g_ftdm_sngss7_data.cfg.isupCkt[x].min_digits					= ccSpan->min_digits;
		g_ftdm_sngss7_data.cfg.isupCkt[x].bearcap_check					= ccSpan->bearcap_check;
		g_ftdm_sngss7_data.cfg.isupCkt[x].itx_auto_reply				= ccSpan->itx_auto_reply;
		g_ftdm_sngss7_data.cfg.isupCkt[x].transparent_iam				= ccSpan->transparent_iam;
		g_ftdm_sngss7_data.cfg.isupCkt[x].transparent_iam_max_size			= ccSpan->transparent_iam_max_size;
		g_ftdm_sngss7_data.cfg.isupCkt[x].cpg_on_progress_media				= ccSpan->cpg_on_progress_media;
		g_ftdm_sngss7_data.cfg.isupCkt[x].cpg_on_progress	 		    	= ccSpan->cpg_on_progress;
		g_ftdm_sngss7_data.cfg.isupCkt[x].cpg_on_alert	 		    		= ccSpan->cpg_on_alert;
		g_ftdm_sngss7_data.cfg.isupCkt[x].ignore_alert_on_cpg				= ccSpan->ignore_alert_on_cpg;

		if (ccSpan->t3 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t3			= 1200;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t3			= ccSpan->t3;
		}
		if (ccSpan->t10 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t10		= 50;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t10		= ccSpan->t10;
		}
		if (ccSpan->t12 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t12		= 300;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t12		= ccSpan->t12;
		}
		if (ccSpan->t13 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t13		= 3000;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t13		= ccSpan->t13;
		}
		if (ccSpan->t14 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t14		= 300;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t14		= ccSpan->t14;
		}
		if (ccSpan->t15 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t15		= 3000;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t15		= ccSpan->t15;
		}
		if (ccSpan->t16 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t16		= 300;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t16		= ccSpan->t16;
		}
		if (ccSpan->t17 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t17		= 3000;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t17		= ccSpan->t17;
		}

		if (ccSpan->t35 == 0) {
			/* Q.764 2.2.5 Address incomplete (T35 is 15-20 seconds according to Table A.1/Q.764) */
			g_ftdm_sngss7_data.cfg.isupCkt[x].t35		= 170;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t35		= ccSpan->t35;
		}
		if (ccSpan->t39 == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t39		= 120;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].t39		= ccSpan->t39;
		}
		
		if (ccSpan->tval == 0) {
			g_ftdm_sngss7_data.cfg.isupCkt[x].tval		= 10;
		} else {
			g_ftdm_sngss7_data.cfg.isupCkt[x].tval		= ccSpan->tval;
		}

		SS7_INFO("Added procId=%d, spanId = %d, chan = %d, cic = %d, ISUP cirId = %d\n",
					g_ftdm_sngss7_data.cfg.isupCkt[x].procId,
					g_ftdm_sngss7_data.cfg.isupCkt[x].ccSpanId,
					g_ftdm_sngss7_data.cfg.isupCkt[x].chan,
					g_ftdm_sngss7_data.cfg.isupCkt[x].cic,
					g_ftdm_sngss7_data.cfg.isupCkt[x].id);

move_along:
		/* increment the span channel count */
		count++;

	/**************************************************************************/
	} /* while (ccSpan->ch_map[0] != '\0') */


	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_circuits(sng_span_t *sngSpan)
{
	ftdm_channel_t		*ftdmchan = NULL;
	ftdm_span_t		*ftdmspan = sngSpan->span;
	sng_isup_ckt_t		*isupCkt = NULL;
	sngss7_chan_data_t	*ss7_info = NULL;
	ftdm_bool_t 		chan_skip = FTDM_FALSE;
	int 			trans_idx = 0;
	int			flag;
	int			i;
	int			x;

	/* go through all the channels on ftdm span */
	for (i = 1; i < (ftdmspan->chan_count+1); i++) {
	/**************************************************************************/
		
		chan_skip = FTDM_FALSE;
		/* extract the ftdmchan pointer */
		ftdmchan = ftdmspan->channels[i];

		/* find the equivalent channel in the global structure */
		x = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);
		flag = 0;
		while (g_ftdm_sngss7_data.cfg.isupCkt[x].id != 0) {
		/**********************************************************************/
			if (g_ftdm_sngss7_data.cfg.isupCkt[x].chan != ftdmchan->physical_chan_id)
				chan_skip = FTDM_FALSE;
			/* Skip CIC distribution if the channel is not an unsed channel and the channel is already open,
			 * as this might be a transparent channel */
			if (!(g_ftdm_sngss7_data.cfg.isupCkt[x].type == SNG_CKT_HOLE) && (ftdm_test_flag(ftdmchan, FTDM_CHANNEL_OPEN))) {
				/* If the physical channel Id is same as that of ISUP circuit channel ID then only check
				 * whether this channel is configured as tranaparent channel as per SS7 ISUP ccSpan channel map */
				if (g_ftdm_sngss7_data.cfg.isupCkt[x].chan == ftdmchan->physical_chan_id) {
					/* If transparent circuit is present in transCkt structure then please skip the channel as this is
					 * transparent channel but if it is not present then return Failure due to invalid configuration */
					for (trans_idx = 0; trans_idx < MAX_TRANSPARENT_CKTS; trans_idx++) {
						if (!g_ftdm_sngss7_data.cfg.transCkt[trans_idx].ccSpan_id) {
							if (g_ftdm_sngss7_data.cfg.isupCkt[x].ccSpanId == sngSpan->ccSpanId) {
								SS7_ERROR_CHAN(ftdmchan, "Invalid Configuration. Transparent circuit must not be a part of CIC distribution!%s\n","");
								return FTDM_FAIL;
							} else {
								chan_skip = FTDM_TRUE;
								break;
							}
						}

						if ((g_ftdm_sngss7_data.cfg.transCkt[trans_idx].ccSpan_id == sngSpan->ccSpanId) &&
						    (g_ftdm_sngss7_data.cfg.transCkt[trans_idx].chan_id == ftdmchan->physical_chan_id)) {
							chan_skip = FTDM_TRUE;
							break;
						} else {
							chan_skip = FTDM_FALSE;
						}
					}

					if (chan_skip == FTDM_FALSE) {
						SS7_ERROR_CHAN(ftdmchan, "Invalid Configuration. Transparent circuit must not be a part of CIC distribution!%s\n","");
						return FTDM_FAIL;
					}
				}

				SS7_INFO_CHAN(ftdmchan, "Skipping this channel in CIC configuration as this is a tranparent channel!\n","");
				x++;
				chan_skip = FTDM_TRUE;
				continue;
			} else {
				/* If transparent circuit is configured in SS7 module but due to any reason trasparent
				 * span is not present in freetdm.conf.xml then please failure with proper information */
				for (trans_idx = 0; trans_idx < MAX_TRANSPARENT_CKTS; trans_idx++) {
					if (!g_ftdm_sngss7_data.cfg.transCkt[trans_idx].ccSpan_id) {
						break;
					}

					if (g_ftdm_sngss7_data.cfg.transCkt[trans_idx].ccSpan_id == sngSpan->ccSpanId) {
						if ((g_ftdm_sngss7_data.cfg.transCkt[trans_idx].chan_id == ftdmchan->physical_chan_id) &&
						    (g_ftdm_sngss7_data.cfg.transCkt[trans_idx].next_cktId == x)) {
							SS7_ERROR_CHAN(ftdmchan, "Transparent circuit found in SS7 configuration but transparent channel is not configured!%s\n","");
							return FTDM_FAIL;
						}
					}
				}
			}

			/* pull out the circuit to make it easier to work with */
			isupCkt = &g_ftdm_sngss7_data.cfg.isupCkt[x];

			/* if the ccSpanId's match fill in the span value...this is for sigs
			 * because they will never have a channel that matches since they 
			 * have a ftdmchan at this time */
			if (sngSpan->ccSpanId == isupCkt->ccSpanId) {
				isupCkt->span = ftdmchan->physical_span_id;
			}

			/* check if the ccSpanId matches and the physical channel # match */
			if ((sngSpan->ccSpanId == isupCkt->ccSpanId) &&
				(ftdmchan->physical_chan_id == isupCkt->chan)) {

				/* we've found the channel in the ckt structure...raise the flag */
				flag = 1;

				/* now get out of the loop */
				break;
			}

			/* move to the next ckt */
			x++;

			/* check if we are outside of the range of possible indexes */
			if (x == (ftmod_ss7_get_circuit_end_range(g_ftdm_sngss7_data.cfg.procId))) {
				break;
			}
		/**********************************************************************/
		} /* while (g_ftdm_sngss7_data.cfg.isupCkt[x].id != 0) */

		/* skip this channel as it is suppose not to be included in ISUP CIC */
		if (chan_skip == FTDM_TRUE) {
			continue;
		}

		/* check we found the ckt or not */
		if (!flag) {
			SS7_ERROR_CHAN(ftdmchan, "Failed to find this channel in the global ckts!%s\n","");
			return FTDM_FAIL;
		}

		/* fill in the rest of the global sngss7_chan_data_t structure */
		ss7_info = (sngss7_chan_data_t *)isupCkt->obj;
		ss7_info->ftdmchan = ftdmchan;

		/* attach the sngss7_chan_data_t to the freetdm channel structure */
		ftdmchan->call_data = ss7_info;

		/* prepare the timer structures */
		ss7_info->t35.sched		= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t35.counter		= 1;
		ss7_info->t35.beat		= (isupCkt->t35) * 100; /* beat is in ms, t35 is in 100ms */
		ss7_info->t35.callback		= handle_isup_t35;
		ss7_info->t35.sngss7_info	= ss7_info;

		ss7_info->t10.sched		= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t10.counter		= 1;
		ss7_info->t10.beat		= (isupCkt->t10) * 100; /* beat is in ms, t10 is in 100ms */
		ss7_info->t10.callback		= handle_isup_t10;
		ss7_info->t10.sngss7_info	= ss7_info;

		/* prepare the timer structures */
		ss7_info->t39.sched		= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t39.counter		= 1;
		ss7_info->t39.beat		= (isupCkt->t39) * 100; /* beat is in ms, t39 is in 100ms */
		ss7_info->t39.callback		= handle_isup_t39;
		ss7_info->t39.sngss7_info	= ss7_info;

		/* Set up timer for blo re-transmission. 
		   If not receiving BLA in 5 seconds after sending out BLO, 
                   this timer kicks in to trigger BLO retransmission.
		*/
		ss7_info->t_waiting_bla.sched		= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t_waiting_bla.counter		= 1;
		ss7_info->t_waiting_bla.beat		= 5 * 1000;
		ss7_info->t_waiting_bla.callback	= handle_wait_bla_timeout;
		ss7_info->t_waiting_bla.sngss7_info	= ss7_info;
		
		/* Set up timer for UBL re-transmission. 
		   If not receiving UBA in 30 seconds after sending out UBL, 
                   this timer kicks in to trigger UBL retransmission.
		*/
		ss7_info->t_waiting_uba.sched		= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t_waiting_uba.counter		= 1;
		ss7_info->t_waiting_uba.beat		= 30 * 1000;
		ss7_info->t_waiting_uba.callback	= handle_wait_uba_timeout;
		ss7_info->t_waiting_uba.sngss7_info	= ss7_info;
		
		/* Set up timer for UBL transmission upon receiving BLA. 
		   UBL will not be sent in 5 seconds after receiving BLA, 
		   since trillium might do a BLO re-transmission without 
		   ftdm knowing, which possibly overwrite UBL sent by freetdm.
		   This timer to enable UBL transmission after 5 seconds from
		   receiving BLA.
		*/
		ss7_info->t_tx_ubl_on_rx_bla.sched	= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t_tx_ubl_on_rx_bla.counter	= 1;
		ss7_info->t_tx_ubl_on_rx_bla.beat	= 5 * 1000;
		ss7_info->t_tx_ubl_on_rx_bla.callback	= handle_tx_ubl_on_rx_bla_timer;
		ss7_info->t_tx_ubl_on_rx_bla.sngss7_info= ss7_info;
		
		/* Set up timer for RSC re-transmission. 
		   If not receiving RSCA in 60 seconds after sending out RSC, 
                   this timer kicks in to trigger RSC retransmission.
		*/
		ss7_info->t_waiting_rsca.sched		= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t_waiting_rsca.counter	= 1;
		ss7_info->t_waiting_rsca.beat		= 60 * 1000;
		ss7_info->t_waiting_rsca.callback	= handle_wait_rsca_timeout;
		ss7_info->t_waiting_rsca.sngss7_info	= ss7_info;

		/* Set up timer for block UBL transmission after BLO. 
		   When BLO has been sent out, freetdm will not send out UBL
		   in 12 seconds to prevent trillium BLO retransmission overriding
		   UBL message. 
		*/
		ss7_info->t_block_ubl.sched		= ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched;
		ss7_info->t_block_ubl.counter		= 1;
		ss7_info->t_block_ubl.beat		= 12 * 1000;
		ss7_info->t_block_ubl.callback		= handle_disable_ubl_timeout;
		ss7_info->t_block_ubl.sngss7_info	= ss7_info;

	/**************************************************************************/
	} /* for (i == 1; i < ftdmspan->chan_count; i++) */

	if(SNG_SS7_OPR_MODE_M2UA_ASP == g_ftdm_operating_mode){
		g_ftdm_sngss7_data.cfg.g_m2ua_cfg.sched.tmr_sched = ((sngss7_span_data_t *)(ftdmspan->signal_data))->sched; 
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_next_timeslot(char *ch_map, sng_timeslot_t *timeslot)
{
	int			i;
	int			x;
	int			lower;
	int			upper;
	char		tmp[5]; /*KONRAD FIX ME*/
	char		new_ch_map[MAX_CIC_MAP_LENGTH];

	memset(&tmp[0], '\0', sizeof(tmp));
	memset(&new_ch_map[0], '\0', sizeof(new_ch_map));
	memset(timeslot, 0x0, sizeof(sng_timeslot_t));

	SS7_DEBUG("Old channel map = \"%s\"\n", ch_map);

	/* start at the beginning of the ch_map */
	x = 0;

	switch (ch_map[x]) {
	/**************************************************************************/
	case 'S':
	case 's':   /* we have a sig link */
		timeslot->siglink = 1;

		/* check what comes next either a comma or a number */
		x++;
		if (ch_map[x] == ',') {
			timeslot->hole = 1;
			SS7_DEBUG(" Found a siglink in the channel map with a hole in the cic map\n");
		} else if (isdigit(ch_map[x])) {
			/* consume all digits until a comma as this is the channel */
			SS7_DEBUG(" Found a siglink in the channel map with out a hole in the cic map\n");
		} else {
			SS7_ERROR("Found an illegal channel map character after signal link flag = \"%c\"!\n", ch_map[x]);
			return FTDM_FAIL;
		}
		break;
	/**************************************************************************/
	case 'G':
	case 'g':   /* we have a channel gap */
		timeslot->gap = 1;

		/* check what comes next either a comma or a number */
		x++;
		if (ch_map[x] == ',') {
			timeslot->hole = 1;
			SS7_DEBUG(" Found a gap in the channel map with a hole in the cic map\n");
		} else if (isdigit(ch_map[x])) {
			SS7_DEBUG(" Found a gap in the channel map with out a hole in the cic map\n");
			/* consume all digits until a comma as this is the channel */
		} else {
			SS7_ERROR("Found an illegal channel map character after signal link flag = \"%c\"!\n", ch_map[x]);
			return FTDM_FAIL;
		}
		break;
	/**************************************************************************/
	case 'T':
	case 't':   /* we have a transparent channel */
		timeslot->transparent = 1;

		/* check what comes next either a comma or a number */
		x++;
		if (ch_map[x] == ',') {
			timeslot->hole = 1;
			SS7_DEBUG(" Found a transparent channel in the channel map with a hole in the cic map\n");
		} else if (isdigit(ch_map[x])) {
			SS7_DEBUG(" Found a transparent channel thus considering it as a hole in the cic map\n");
			/* consume all digits until a comma as this is the channel */
		} else {
			SS7_ERROR("Found an illegal channel map character after transpaent channel flag = \"%c\"!\n", ch_map[x]);
			return FTDM_FAIL;
		}
		break;
	/**************************************************************************/
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':   /* we have a channel */
		/* consume all digits until a comma or a dash */
		SS7_DEBUG("Found a starting channel in the channel map\n");
		break;
	/**************************************************************************/
	default:
		SS7_ERROR("Found an illegal channel map character = \"%c\"!\n", ch_map[x]);
		return FTDM_FAIL;
	/**************************************************************************/
	} /* switch (ch_map[x]) */

	/* grab the first number in the string */
	i = 0;
	while ((ch_map[x] != '\0') && (ch_map[x] != '-') && (ch_map[x] != ',')) {
		tmp[i] = ch_map[x];
		i++;
		x++;
	}
	tmp[i] = '\0';
	timeslot->channel = atoi(tmp);
	lower = timeslot->channel + 1;

	/* check the next value in the list */
	if (ch_map[x] == '-') {
		/* consume the number after the dash */
		x++;
		i = 0;
		while ((ch_map[x] != '\0') && (ch_map[x] != '-') && (ch_map[x] != ',')) {
			tmp[i] = ch_map[x];
			i++;
			x++;
		}
		tmp[i] = '\0';
		upper = atoi(tmp);

		/* check if the upper end of the range is the same as the lower end of the range */
		if (upper == lower) {
			/* the range is completed, eat the next comma or \0  and write it */
			sprintf(new_ch_map, "%d", lower);
		} else if ( upper > lower) {
			/* the list continues, add 1 from the channel map value and re-insert it to the list */
			sprintf(new_ch_map, "%d-%d", lower, upper);
		} else {
			SS7_ERROR("The upper is less then the lower end of the range...should not happen!\n");
			return FTDM_FAIL;
		}

		/* the the rest of ch_map to new_ch_map */
		strncat(new_ch_map, &ch_map[x], strlen(&ch_map[x]));


		/* set the new cic map to ch_map*/
		memset(ch_map, '\0', sizeof(ch_map));
		strcpy(ch_map, new_ch_map);

	} else if (ch_map[x] == ',') {
		/* move past the comma */
		x++;

		/* copy the rest of the list to new_ch_map */
		memset(new_ch_map, '\0', sizeof(new_ch_map));
		strcpy(new_ch_map, &ch_map[x]);

		/* copy the new_ch_map over the old one */
		memset(ch_map, '\0', sizeof(ch_map));
		strcpy(ch_map, new_ch_map);

	} else if (ch_map[x] == '\0') {

		/* we're at the end of the string...copy the rest of the list to new_ch_map */
		memset(new_ch_map, '\0', sizeof(new_ch_map));
		strcpy(new_ch_map, &ch_map[x]);

		/* set the new cic map to ch_map*/
		memset(ch_map, '\0', sizeof(ch_map));
		strcpy(ch_map, new_ch_map);
	} else { 
		/* nothing to do */
	}

	SS7_DEBUG("New channel map = \"%s\"\n", ch_map);

	return FTDM_SUCCESS;
}
/******************************************************************************/

int ftmod_ss7_get_mtp1_id_by_span_id(int span_id)
{
    int x = 0;
    int mtp1_cfg_id = 0;
    x = 1;
    while (x < (MAX_MTP_LINKS)) {
        if ((g_ftdm_sngss7_data.cfg.mtp1Link[x].id != 0) &&
                ((g_ftdm_sngss7_data.cfg.mtp1Link[x].flags & SNGSS7_CONFIGURED))) {

            if (span_id == g_ftdm_sngss7_data.cfg.mtp1Link[x].span) {
                SS7_DEVEL_DEBUG("Found matching mtp1 config index at[%d]\n", x);
                mtp1_cfg_id = x;
                break;
            }
        }
        x++;
    }

    return mtp1_cfg_id;
}

/******************************************************************************/
int ftmod_ss7_get_mtp2_id_by_mtp1_id(int mtp1Id)
{
    int x = 0;
    int mtp2_cfg_id = 0;
    x = 1;
    while (x < (MAX_MTP_LINKS)) {
        if ((g_ftdm_sngss7_data.cfg.mtp2Link[x].id != 0) &&
                ((g_ftdm_sngss7_data.cfg.mtp2Link[x].flags & SNGSS7_CONFIGURED))) {

            if (mtp1Id == g_ftdm_sngss7_data.cfg.mtp2Link[x].mtp1Id) {
                SS7_DEVEL_DEBUG("Found matching mtp2 config index at[%d]\n", x);
                mtp2_cfg_id = x;
                break;
            }
        }
        x++;
    }

    return mtp2_cfg_id;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_acc_timer(sng_route_t *mtp3_route, ftdm_span_t *span)
{
	uint32_t t29_val = 0;
	uint32_t t30_val = 0;
	uint32_t acc_call_rate = 0;
	ftdm_sngss7_rmt_cong_t *sngss7_rmt_cong = NULL;
	char dpc[MAX_DPC_CONFIGURED];
	char *dpc_key=NULL;
#ifdef ACC_TEST
	char file_path[512] = { 0 };
#endif

	memset(dpc, 0 , sizeof(dpc));

	sngss7_rmt_cong = ftdm_calloc(sizeof(*sngss7_rmt_cong), 1);

	if (!sngss7_rmt_cong) {
		SS7_DEBUG("Unable to allocate memory\n");
		return FTDM_FAIL;
	}

	if (!mtp3_route->dpc) {
		SS7_DEBUG("DPC not found in route configuration\n");
		ftdm_safe_free(sngss7_rmt_cong);
		return FTDM_FAIL;
	}

	/* prepare automatic congestion control structure */
	sngss7_rmt_cong->sngss7_rmtCongLvl = 0;
	sngss7_rmt_cong->dpc = mtp3_route->dpc;
	sngss7_rmt_cong->call_blk_rate = 0;
	sngss7_rmt_cong->calls_allowed = 0;
	sngss7_rmt_cong->calls_passed = 0;
	sngss7_rmt_cong->calls_received = 0;
	sngss7_rmt_cong->calls_rejected = 0;
	sngss7_rmt_cong->loc_calls_rejected = 0;
	sngss7_rmt_cong->max_bkt_size = 0;
	/* Gives the average calls per second when acc is enable */
	sngss7_rmt_cong->avg_call_rate = 0;

#ifdef ACC_TEST
	sngss7_rmt_cong->iam_recv = 0;
	sngss7_rmt_cong->iam_pri_recv = 0;
	sngss7_rmt_cong->iam_trans = 0;
	sngss7_rmt_cong->iam_pri_trans = 0;
	sngss7_rmt_cong->rel_recv = 0;
	sngss7_rmt_cong->rel_rcl1_recv = 0;
	sngss7_rmt_cong->rel_rcl2_recv = 0;
	sngss7_rmt_cong->log_file_ptr = NULL;
	sngss7_rmt_cong->debug_idx = 0;
#endif

	sprintf(dpc, "%d", sngss7_rmt_cong->dpc);
	if (hashtable_search(ss7_rmtcong_lst, (void *)dpc)) {
		SS7_DEBUG("DPC[%d] is already inserted in the hash tablearsing\n", mtp3_route->dpc);
		ftdm_safe_free(sngss7_rmt_cong);
		return FTDM_SUCCESS;
	}

	/* initializing global active calls during congestion */
	sngss7_rmt_cong->ss7_active_calls = create_hashtable(MAX_DPC_CONFIGURED, ftdm_hash_hashfromstring, ftdm_hash_equalkeys);

	if (!sngss7_rmt_cong->ss7_active_calls) {
		SS7_DEBUG("NSG-ACC: Failed to create hash list for ss7_active_calls \n");
		ftdm_safe_free(sngss7_rmt_cong);
		return FTDM_SUCCESS;
	} else {
		SS7_DEBUG("NSG-ACC: ss7_active_calls hash list successfully created for dpc[%d]\n", dpc);
	}

	/* Create mutex */
	ftdm_mutex_create(&sngss7_rmt_cong->mutex);

	if (mtp3_route->max_bkt_size) {
		sngss7_rmt_cong->max_bkt_size = mtp3_route->max_bkt_size;
		SS7_DEBUG("Found user supplied max bucket size as %d for DPC[%d]\n", sngss7_rmt_cong->max_bkt_size, mtp3_route->dpc);
	} else {
		SS7_DEBUG("No user supplied max bucket size for DPC[%d]\n", mtp3_route->dpc);
	}

	/* Timer for ACC Feature in ms can be 300-600ms */
	if (mtp3_route->t29 == 0) {
		t29_val	= 3;
	} else {
		t29_val = mtp3_route->t29;
	}

	/* Timer ofr implementing ACC Feature in sec can be 5-10sec*/
	if (mtp3_route->t30 == 0) {
		t30_val	= 50;
	} else {
		t30_val	= mtp3_route->t30;
	}

	/* Timer for getting calls per call rate timer expiry */
	if (mtp3_route->call_rate == 0) {
		acc_call_rate = 50;
	} else {
		acc_call_rate = mtp3_route->call_rate;
	}

	/* prepare the timer structures */
	sngss7_rmt_cong->t29.tmr_sched	= ((sngss7_span_data_t *)(span->signal_data))->sched;
	sngss7_rmt_cong->t29.counter	= 1;
	sngss7_rmt_cong->t29.beat	= (t29_val) * 100;	/* beat is in ms, t29 is in 100ms */
	sngss7_rmt_cong->t29.callback	= handle_route_t29;
	sngss7_rmt_cong->t29.sngss7_rmt_cong = sngss7_rmt_cong;

	/* prepare the timer structures */
	sngss7_rmt_cong->t30.tmr_sched	= ((sngss7_span_data_t *)(span->signal_data))->sched;
	sngss7_rmt_cong->t30.counter	= 1;
	sngss7_rmt_cong->t30.beat	= (t30_val) * 100; /* beat is in ms, t30 is in 100ms */
	sngss7_rmt_cong->t30.callback	= handle_route_t30;
	sngss7_rmt_cong->t30.sngss7_rmt_cong = sngss7_rmt_cong;

	/* prepare the timer structures */
	sngss7_rmt_cong->acc_call_rate.tmr_sched	= ((sngss7_span_data_t *)(span->signal_data))->sched;
	sngss7_rmt_cong->acc_call_rate.counter		= 1;
	sngss7_rmt_cong->acc_call_rate.beat		= (acc_call_rate) * 100; /* beat is in ms, acc_call_rate is in 100ms */
	sngss7_rmt_cong->acc_call_rate.callback		= handle_route_acc_call_rate;
	sngss7_rmt_cong->acc_call_rate.sngss7_rmt_cong 	= sngss7_rmt_cong;

#ifdef ACC_TEST
	/* prepare the timer structures */
	sngss7_rmt_cong->acc_debug.tmr_sched	= ((sngss7_span_data_t *)(span->signal_data))->sched;
	sngss7_rmt_cong->acc_debug.counter	= 1;
	sngss7_rmt_cong->acc_debug.beat		= 10 * 100; /* beat is in ms, t30 is in 100ms */
	sngss7_rmt_cong->acc_debug.callback	= handle_route_acc_debug;
	sngss7_rmt_cong->acc_debug.sngss7_rmt_cong = sngss7_rmt_cong;
#endif

	memset(dpc, 0 , sizeof(dpc));
	sprintf(dpc, "%d", sngss7_rmt_cong->dpc);

	dpc_key = ftdm_strdup(dpc);

	/* Insert the same in ACC hash list */
	hashtable_insert(ss7_rmtcong_lst, (void *)dpc_key, sngss7_rmt_cong, HASHTABLE_FLAG_FREE_KEY);
	SS7_DEBUG("DPC[%d] successfully inserted in ACC hash list\n", sngss7_rmt_cong->dpc);

#ifdef ACC_TEST
	memset(file_path, 0, sizeof(file_path));
	/* creating the file in which ACC debugs needs to be written */
	snprintf(file_path, sizeof(file_path), "/tmp/acc_debug-%d.txt", sngss7_rmt_cong->dpc);

	SS7_DEBUG("NSG-ACC: Open %s file and writting Call statistics in to it\n", file_path);
	if ((sngss7_rmt_cong->log_file_ptr = fopen(file_path, "a")) == NULL) {
		SS7_ERROR("NSG-ACC: Failed to Open Log File.\n");
		return FTDM_FAIL;
	} else {
		SS7_DEBUG("NSG-ACC: %s file is open successfully\n", file_path);
	}

	/* if timer is not started start the ACC DEBUG Timer */
	if (sngss7_rmt_cong->acc_debug.tmr_id) {
		SS7_DEBUG("NSG-ACC: ACC DEBUG Timer is already running for DPC[%d]\n", sngss7_rmt_cong->dpc);
	} else {
		SS7_DEBUG("NSG-ACC: Starting ACC DEBUG Timer for DPC[%d]\n", sngss7_rmt_cong->dpc);
		if (ftdm_sched_timer (sngss7_rmt_cong->acc_debug.tmr_sched,
					"acc_debug",
					sngss7_rmt_cong->acc_debug.beat,
					sngss7_rmt_cong->acc_debug.callback,
					&sngss7_rmt_cong->acc_debug,
					&sngss7_rmt_cong->acc_debug.tmr_id)) {
			SS7_ERROR ("NSG-ACC: Unable to schedule ACC DEBUG Timer\n");
		} else {
			SS7_INFO("NSG-ACC: ACC DEBUG Timer started with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t29.tmr_id, sngss7_rmt_cong->dpc);
		}
	}
#endif

	/* if timer is not started, start ACC Call rate timer in order to calculate number of average calls
	 * per socnd depending upon the number of call received with in call rate timer beat time */
	if (sngss7_rmt_cong->acc_call_rate.tmr_id) {
		SS7_DEBUG("NSG-ACC: ACC DEBUG Timer is already running for DPC[%d]\n", sngss7_rmt_cong->dpc);
	} else {
		SS7_DEBUG("NSG-ACC: Starting ACC DEBUG Timer for DPC[%d]\n", sngss7_rmt_cong->dpc);
		if (ftdm_sched_timer (sngss7_rmt_cong->acc_call_rate.tmr_sched,
					"acc_call_rate",
					sngss7_rmt_cong->acc_call_rate.beat,
					sngss7_rmt_cong->acc_call_rate.callback,
					&sngss7_rmt_cong->acc_call_rate,
					&sngss7_rmt_cong->acc_call_rate.tmr_id)) {
			SS7_ERROR ("NSG-ACC: Unable to schedule ACC Call Rate Timer\n");
		} else {
			SS7_INFO("NSG-ACC: ACC Call Rate Timer started with timer-id[%d] for dpc[%d]\n", sngss7_rmt_cong->t29.tmr_id, sngss7_rmt_cong->dpc);
		}
	}
	return FTDM_SUCCESS;
}

/*
 * Earlier circuit range per Proc Id is constraint to 1000 cic and it was hardcoded.
 * Thus, in case of configuring E1 having circuit Id more than 1000. SS7 CICs never
 * comes in UP state.
 *
 * ftmod_ss7_get_circuit_start_range() and ftmod_ss7_get_circuit_end_range() api
 * basically give the range of circuits ID depeding up on the proc ID's, base on
 * SNG_CIC_MAP_OFFSET as defined in sng_ss7.h. These are common api's so that
 * if in case there is any need to chage the Circuits range then there is no need
 * to change it on multiple places, just change the offset and user can achieve
 * CIC range in result.
 */

/* get circuit start range based on proc ID */
int ftmod_ss7_get_circuit_start_range(int procId)
{
	int start_range = 0;
	int offset	= (procId -1) * SNG_CIC_MAP_OFFSET;

	start_range = (offset + (procId * 1000)) + 1;

	return start_range;
}

/* get circuit end range based on proc ID */
int ftmod_ss7_get_circuit_end_range(int procId)
{
	int end_range = 0;
	int offset    = SNG_CIC_MAP_OFFSET;

	end_range = ftmod_ss7_get_circuit_start_range(procId) + offset - 1;

	return end_range;
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
