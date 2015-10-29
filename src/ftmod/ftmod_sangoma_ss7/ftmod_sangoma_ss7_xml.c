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
 * Kapil Gupta <kgupta@sangoma.com>
 * Pushkar Singh <psingh@sangoma.com>
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
	/*              MTP3             ISUP             M3UA-SU       M3UA-SU2        */
        { 1, "itu88"  , LSI_SW_ITU     , LSI_SW_ITU     , LIT_SW_ITU    , LIT_SW2_ITU     },
        { 1, "itu92"  , LSI_SW_ITU     , LSI_SW_ITU     , LIT_SW_ITU    , LIT_SW2_ITU     },
        { 1, "itu97"  , LSI_SW_ITU97   , LSI_SW_ITU97   , LIT_SW_ITU    , LIT_SW2_ITU     },
        { 1, "itu00"  , LSI_SW_ITU2000 , LSI_SW_ITU2000 , LIT_SW_ITU    , LIT_SW2_ITU     },
        { 1, "ansi88" , LSI_SW_ANS88   , LSI_SW_ANS88   , LIT_SW_ANS    , LIT_SW2_ANS     },
        { 1, "ansi92" , LSI_SW_ANS92   , LSI_SW_ANS92   , LIT_SW_ANS    , LIT_SW2_ANS     },
        { 1, "ansi95" , LSI_SW_ANS92   , LSI_SW_ANS92   , LIT_SW_ANS    , LIT_SW2_ANS     },
        { 1, "ansi96" , 0              , 0              , LIT_SW_ANS96  , LIT_SW2_ANS     },
        { 1, "etsiv2" , LSI_SW_ETSI    , LSI_SW_ETSI    , LSN_SW_ITU    , LIT_SW2_ETS     },
        { 1, "etsiv3" , LSI_SW_ETSIV3  , LSI_SW_ETSIV3  , LSN_SW_ITU    , LIT_SW2_ETS     },
        { 1, "india"  , LSI_SW_INDIA   , LSI_SW_INDIA   , 0             , LIT_SW2_UNUSED  },
        { 1, "uk"     , LSI_SW_UK      , LSI_SW_UK      , 0             , LIT_SW2_UNUSED  },
        { 1, "russia" , LSI_SW_RUSSIA  , LSI_SW_RUSSIA  , 0             , LIT_SW2_UNUSED  },
        { 1, "china"  , LSI_SW_CHINA   , LSI_SW_CHINA   , LIT_SW_CHINA  , LIT_SW2_UNUSED  },
        { 1, "itu"    , 0              , 0              , LIT_SW_ITU    , LIT_SW2_ITU     },
        { 1, "unused" , 0              , 0              , 0             , LIT_SW2_UNUSED  },
        { 0, ""       , 0              , 0              , 0             , LIT_SW2_UNUSED  },
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
	int 	 sigtran_sig;
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
	uint8_t 		blo_on_ckt_delete;	/* Send and blo voice circuit when needs to be deleted */
} sng_ccSpan_t;

int cmbLinkSetId;
/* Total number of cics configured */
uint32_t nmb_cics_cfg = 0;
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
static int ftmod_ss7_parse_sng_isup(ftdm_conf_node_t *sng_isup, const char* operating_mode, ftdm_span_t *span, int reload);

static int ftmod_ss7_parse_sng_gen(ftdm_conf_node_t *sng_gen, char* operating_mode);

static int ftmod_ss7_parse_sng_relay(ftdm_conf_node_t *sng_relay);
static int ftmod_ss7_parse_relay_channel(ftdm_conf_node_t *relay_chan);

static int ftmod_ss7_parse_mtp1_links(ftdm_conf_node_t *mtp1_links, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_parse_mtp1_link(ftdm_conf_node_t *mtp1_link, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg);

static int ftmod_ss7_parse_mtp2_links(ftdm_conf_node_t *mtp2_links, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_parse_mtp2_link(ftdm_conf_node_t *mtp2_link, sng_ss7_cfg_t *cfg);

static int ftmod_ss7_parse_mtp3_links(ftdm_conf_node_t *mtp3_links, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_parse_mtp3_link(ftdm_conf_node_t *mtp3_link, sng_ss7_cfg_t *cfg);

static int ftmod_ss7_parse_mtp_linksets(ftdm_conf_node_t *mtp_linksets, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_parse_mtp_linkset(ftdm_conf_node_t *mtp_linkset, sng_ss7_cfg_t *cfg);

static int ftmod_ss7_parse_mtp_routes(ftdm_conf_node_t *mtp_routes, ftdm_span_t *span, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload);
static int ftmod_ss7_parse_mtp_route(ftdm_conf_node_t *mtp_route, ftdm_span_t *span, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload);

static int ftmod_ss7_parse_isup_interfaces(ftdm_conf_node_t *isup_interfaces, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload);
static int ftmod_ss7_parse_isup_interface(ftdm_conf_node_t *isup_interface, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload);

static int ftmod_ss7_parse_cc_spans(ftdm_conf_node_t *cc_spans, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_parse_cc_span(ftdm_conf_node_t *cc_span, sng_ss7_cfg_t *cfg);

/* Fucntion related to parse configuration in case of realod (reconfiguration request) */
/* ------------------------------------------------------------------------------------------------------------ */
static ftdm_status_t ftmod_ss7_parse_sng_gen_on_reload(ftdm_conf_node_t *sng_gen, char *sng_gen_opr_mode, const char *span_opr_mode, ftdm_span_t *span);

/* validation functions */
static ftdm_status_t ftmod_ss7_copy_reconfig_changes(void);
/* apply reconfiguration */
static ftdm_status_t ftmod_ss7_copy_mtp1_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_mtp2_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_mtp3_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_linkset_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_route_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_self_route_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_nsap_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_isap_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_isup_interface_reconfig_changes(void);
static ftdm_status_t ftmod_ss7_copy_cc_span_reconfig_changes(void);
/* ------------------------------------------------------------------------------------------------------------ */

static int ftmod_ss7_fill_in_relay_channel(sng_relay_t *relay_channel, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_mtp1_link(sng_mtp1_link_t *mtp1Link, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_mtp2_link(sng_mtp2_link_t *mtp2Link, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_mtp3_link(sng_mtp3_link_t *mtp3Link, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_mtpLinkSet(sng_link_set_t *mtpLinkSet, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_mtp3_route(sng_route_t *mtp3_route, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_nsap(sng_route_t *mtp3_route, ftdm_sngss7_operating_modes_e opr_mode, int reconfig, sng_ss7_cfg_t *cfg);
/* Fill in ACC Timer */
static int ftmod_ss7_fill_in_acc_timer(sng_route_t *mtp3_route, ftdm_span_t *span);
static int ftmod_ss7_fill_in_isup_interface(sng_isup_inf_t *sng_isup, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_isap(sng_isap_t *sng_isap, int isup_intf_id, int reconfig, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_ccSpan(sng_ccSpan_t *ccSpan, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_self_route(int spc, int linkType, int switchType, int ssf, int tQuery, int mtp_route_id, int reconfig, sng_ss7_cfg_t *cfg);
static int ftmod_ss7_fill_in_circuits(sng_span_t *sngSpan, ftdm_sngss7_operating_modes_e opr_mode);

static int ftmod_ss7_next_timeslot(char *ch_map, sng_timeslot_t *timeslot);
static void ftmod_ss7_set_glare_resolution (const char *method);
static int ftmod_ss7_set_operating_mode(const char* span_opr_mode, const char* sng_gen_opr_mode, ftdm_span_t *span);

/* synchronise old configuration with new configuration */
static int ftmod_ss7_map_and_sync_existing_self_route(sng_ss7_cfg_t *cfg);
static int ftmod_ss7_map_and_sync_existing_nsap(sng_ss7_cfg_t *cfg);
static int ftmod_ss7_map_and_sync_existing_isap(sng_ss7_cfg_t *cfg);

/******************************************************************************/

/* FUNCTIONS ******************************************************************/

int ftmod_ss7_get_operatig_mode_cfg(const char* span_opr_mode, const char* sng_gen_opr_mode)
{
	ftdm_sngss7_operating_modes_e mode = SNG_SS7_OPR_MODE_NONE;
    char opr_mode[128];

    memset(opr_mode, 0, sizeof(opr_mode));

    /* 1)  Initially operating mode will be set to INVALID
    *  2)  operating-mode can be the part of sng_gen and span configuration 
    *  3)  IF first span configuration or sng_ss7 we set operating mode to X and then subsequent span
    *      configuration defined another operating mode , return error 
    *  4)  If operating mode present in span config..this will take priority then configured in sng_gen
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
    } else if (!strcasecmp(opr_mode, "M3UA_SG")) {
        mode = SNG_SS7_OPR_MODE_M3UA_SG;
    } else if (!strcasecmp(opr_mode, "M3UA_ASP")) {
        mode = SNG_SS7_OPR_MODE_M3UA_ASP;
    } else {
        return FTDM_SUCCESS;
    }

	SS7_DEBUG("%s operating mode received!\n", ftdm_opr_mode_tostr(mode));
	return mode;
}

static int ftmod_ss7_set_operating_mode(const char* span_opr_mode, const char* sng_gen_opr_mode, ftdm_span_t *span)
{
    int idx = 0;
	ftdm_sngss7_operating_modes_e mode = SNG_SS7_OPR_MODE_NONE;

	mode = ftmod_ss7_get_operatig_mode_cfg(span_opr_mode, sng_gen_opr_mode);

	if (mode == SNG_SS7_OPR_MODE_NONE) {
		return FTDM_SUCCESS;
	}

    for (idx = 0; idx < FTDM_MAX_CHANNELS_SPAN; idx++) {
	    if (g_ftdm_sngss7_span_data.span_data[idx].span_id) {
		    if (g_ftdm_sngss7_span_data.span_data[idx].span_id == span->span_id) {
			    if (g_ftdm_sngss7_span_data.span_data[idx].opr_mode == mode) {
				    return FTDM_SUCCESS;
			    } else {
				    SS7_ERROR("Different operating Mode[%s] than configured[%s] for span %d[%s]!\n",
				    ftdm_opr_mode_tostr(mode), ftdm_opr_mode_tostr(g_ftdm_sngss7_span_data.span_data[idx].opr_mode), span->span_id, span->name);
				    return FTDM_FAIL;
			    }
		    }
	    } else if (g_ftdm_sngss7_span_data.span_data[idx].opr_mode == SNG_SS7_OPR_MODE_NONE) {
		    g_ftdm_sngss7_span_data.span_data[idx].span_id = span->span_id;
		    g_ftdm_sngss7_span_data.span_data[idx].opr_mode = mode;
		    SS7_INFO("Setting operating Mode[%s] for span %d[%s]!\n",
			     ftdm_opr_mode_tostr(g_ftdm_sngss7_span_data.span_data[idx].opr_mode), span->span_id, span->name);
		    break;
	    } else {
		    SS7_ERROR("Different operating Mode[%s] than configured[%s] for span %d[%s]!\n",
			      ftdm_opr_mode_tostr(mode), ftdm_opr_mode_tostr(g_ftdm_sngss7_span_data.span_data[idx].opr_mode), span->span_id, span->name);
		    return FTDM_FAIL;
	    }
    }

    return FTDM_SUCCESS;
}

int ftmod_ss7_parse_xml(ftdm_conf_parameter_t *ftdm_parameters, ftdm_span_t *span, int span_list[], ftdm_bool_t check, int reload)
{
	sng_route_t		self_route;
	sng_span_t		sngSpan;
	int			i = 0;
	int			idx = 0;
	const char		*var = NULL;
	const char		*val = NULL;
	const char		*operating_mode = NULL;
	ftdm_conf_node_t 	*ptr = NULL;
	ftdm_status_t 		ret  = FTDM_FAIL;
	ftdm_sngss7_operating_modes_e opr_mode = SNG_SS7_OPR_MODE_NONE;

	/* clean out the isup ckt */
	memset(&sngSpan, 0x0, sizeof(sngSpan));

	/* clean out the self route */
	memset(&self_route, 0x0, sizeof(self_route));

	var = ftdm_parameters[i].var;
	val = ftdm_parameters[i].val;

	/* operating mode can be present in sng_gen or per span as of now */
	/* if operating mode present in sng_gen and span then span has to take priority */

	/* confirm that the first parameter is the "operating-mode" */
	/* Operating mode is not allowed to change in case of dynamic reconfiguration */
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
		if (ftmod_ss7_parse_sng_isup(ptr, operating_mode, span, reload)) {
			SS7_ERROR("Failed to parse the \"confnode\"!\n");
			goto ftmod_ss7_parse_xml_error;
		}
	} else {
		/* ERROR...exit */
		SS7_ERROR("%s The \"confnode\" configuration was not the first parameter!\n",
				  reload == FTDM_TRUE ? "[Reload]: " : "");
		SS7_ERROR("\tFound \"%s\" in the first slot\n", var);
		goto ftmod_ss7_parse_xml_error;
	}

	/* validate if the new changes are as per minimum bare standards and based on that mark all
	 * the protocol stack that needs to be reconfigured */
	if(reload) {
		if ((ret = ftmod_ss7_validate_reconfig_changes(span->span_id, span_list))) {
			if (ret != FTDM_BREAK) {
				SS7_ERROR("[Reload] Failed to reconfigue SS7 module for span(%s)%d!\n",
						span->name, span->span_id);
			}

			if ((ret == FTDM_BREAK) && (span_list[99] == NOT_CONFIGURED)) {
				/* possible timeout please treat this request as new configuration */
				ret = FTDM_TIMEOUT;
				goto ftmod_ss7_parse_xml_error;
			} else if (ret == FTDM_FAIL) {
				g_ftdm_sngss7_data.cfg.reload = FTDM_FALSE;
				return ret;
			}
		}

		if (check == FTDM_TRUE) {
			g_ftdm_sngss7_data.cfg.reload = FTDM_FALSE;
			return FTDM_SUCCESS;
		}

		for (idx = 0; idx < 100; idx++) {
			if (!span_list[idx]) {
				break;
			}

			if (span->span_id == span_list[idx]) {
				ret = FTDM_BREAK;
				goto ftmod_ss7_parse_xml_error;
			}
		}

		/* This is case when there is no change in SS7 configuration */
		if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
			SS7_DEBUG("[Reload] There is no change in SS7 configuration thus, no need to reload it\n");
			ret = FTDM_ECANCELED;
			goto ftmod_ss7_parse_xml_error;
		}

		/* If validation is good the apply copy the reconfiguration changes to actualy configured
		 * SS7 parameters */
		if ((ret = ftmod_ss7_copy_reconfig_changes())) {
			if (ret != FTDM_BREAK) {
				SS7_ERROR("[Reload] Failed to reconfigue SS7 module!\n");
			}
			goto ftmod_ss7_parse_xml_error;
		}
	}
	i++;

	while (ftdm_parameters[i].var != NULL) {
		var = ftdm_parameters[i].var;
		val = ftdm_parameters[i].val;

		if (!strcasecmp(var, "dialplan")) {
		} else if (!strcasecmp(var, "context")) {
		} else if (!strcasecmp(var, "span-id") || !strcasecmp(var, "ccSpanId")) {
			sngSpan.ccSpanId = atoi(val);
			SS7_DEBUG("%d Found an ccSpanId  = %s\n",
					  sngSpan.ccSpanId,
					  reload == FTDM_TRUE ? "[Reload]: " : "");
		} else {
			SS7_ERROR("%s Unknown parameter found =\"%s\"...ignoring it!\n",
					  var,
					  reload == FTDM_TRUE ? "[Reload]: " : "");
		}

		i++;
	} /* while (ftdm_parameters[i].var != NULL) */

	/* fill the pointer to span into isupCkt */
	sngSpan.span = span;

	/* Get operating mode on span basis */
	opr_mode = ftmod_ss7_get_operating_mode_by_span_id(span->span_id);
	if (SNG_SS7_OPR_MODE_NONE == opr_mode) {
		SS7_ERROR("Invalid Operating mode [%s] set for span %d[%s]!\n",
			  ftdm_opr_mode_tostr(opr_mode), span->span_id, span->name);
		goto ftmod_ss7_parse_xml_error;
	}

    if ((SNG_SS7_OPR_MODE_ISUP == opr_mode) ||
	(SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) ||
	(SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode) ||
	(SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode)) {
        /* setup the circuits structure */
        if (ftmod_ss7_fill_in_circuits(&sngSpan, opr_mode)) {
            SS7_ERROR("%s Failed to fill in circuits structure!\n",
					  reload == FTDM_TRUE ? "[Reload]: " : "");
            goto ftmod_ss7_parse_xml_error;
        }
    }

	ret = FTDM_SUCCESS;

ftmod_ss7_parse_xml_error:
	if (reload) {
		g_ftdm_sngss7_data.cfg.reload = FTDM_FALSE;
	}
	return ret;
}

/******************************************************************************/
static int ftmod_ss7_parse_sng_isup(ftdm_conf_node_t *sng_isup, const char* span_opr_mode, ftdm_span_t *span, int reload)
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
	char			sng_gen_opr_mode[128];
	ftdm_conf_node_t	*m3ua_network_ifaces = NULL;
	ftdm_conf_node_t	*m3ua_ps_ifaces = NULL;
	ftdm_conf_node_t	*m3ua_psp_ifaces = NULL;
	ftdm_conf_node_t	*m3ua_user_ifaces = NULL;
	ftdm_conf_node_t	*m3ua_nif_ifaces = NULL;
	ftdm_conf_node_t	*m3ua_route_ifaces = NULL;

	ftdm_sngss7_operating_modes_e opr_mode = SNG_SS7_OPR_MODE_NONE;


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
		} else if (!strcasecmp(tmp_node->name, "sng_m3ua_network_interfaces")) {
			/**********************************************************************/
			if (m3ua_network_ifaces == NULL) {
				m3ua_network_ifaces = tmp_node;
				SS7_DEBUG("Found a <sng_m3ua_network_interfaces> section!\n");
			} else {
				SS7_ERROR("Found a second <sng_m3ua_network_interfaces> section!\n");
				return FTDM_FAIL;
			}
			/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_m3ua_peer_server_interfaces")) {
			/**********************************************************************/
			if (m3ua_ps_ifaces == NULL) {
				m3ua_ps_ifaces = tmp_node;
				SS7_DEBUG("Found a <sng_m3ua_peer_server_interfaces> section!\n");
			} else {
				SS7_ERROR("Found a second <sng_m3ua_peer_server_interfaces> section!\n");
				return FTDM_FAIL;
			}
			/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_m3ua_peer_server_process_interfaces")) {
			/**********************************************************************/
			if (m3ua_psp_ifaces == NULL) {
				m3ua_psp_ifaces = tmp_node;
				SS7_DEBUG("Found a <sng_m3ua_peer_server_process_interfaces> section!\n");
			} else {
				SS7_ERROR("Found a second <sng_m3ua_peer_server_process_interfaces> section!\n");
				return FTDM_FAIL;
			}
			/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_m3ua_user_interfaces")) {
			/**********************************************************************/
			if (m3ua_user_ifaces == NULL) {
				m3ua_user_ifaces = tmp_node;
				SS7_DEBUG("Found a <sng_m3ua_user_interfaces> section!\n");
			} else {
				SS7_ERROR("Found a second <sng_m3ua_user_interfaces> section!\n");
				return FTDM_FAIL;
			}
			/**********************************************************************/
		} else if (!strcasecmp(tmp_node->name, "sng_m3ua_nif_interfaces")) {
			/**********************************************************************/
			if (m3ua_nif_ifaces == NULL) {
				m3ua_nif_ifaces = tmp_node;
				SS7_DEBUG("Found a <sng_m3ua_nif_interfaces> section!\n");
			} else {
				SS7_ERROR("Found a second <sng_m3ua_nif_interfaces> section!\n");
				return FTDM_FAIL;
			}
			/**********************************************************************/

		} else if (!strcasecmp(tmp_node->name, "sng_m3ua_route_interfaces")) {
			/**********************************************************************/
			if (m3ua_route_ifaces == NULL) {
				m3ua_route_ifaces = tmp_node;
				SS7_DEBUG("Found a <sng_m3ua_route_interfaces> section!\n");
			} else {
				SS7_ERROR("Found a second <sng_m3ua_route_interfaces> section!\n");
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
	if (!reload) {
		if (ftmod_ss7_parse_sng_gen(gen_config, sng_gen_opr_mode)) {
			SS7_ERROR("Failed to parse \"gen_config\"!\n");
			return FTDM_FAIL;
		}

		if (FTDM_FAIL == ftmod_ss7_set_operating_mode(span_opr_mode, sng_gen_opr_mode, span)) {
			return FTDM_FAIL;
		}

		opr_mode = ftmod_ss7_get_operating_mode_by_span_id(span->span_id);
		if (opr_mode == SNG_SS7_OPR_MODE_NONE) {
			SS7_ERROR("Invalid Operating mode [%s] set for span %d[%s]!\n",
					ftdm_opr_mode_tostr(opr_mode), span->span_id, span->name);
			return FTDM_FAIL;
		} else {
			SS7_INFO("Setting FreeTDM Operating Mode[%s]\n",ftdm_opr_mode_tostr(opr_mode));
		}

		if (relay_channels && ftmod_ss7_parse_sng_relay(relay_channels)) {
			SS7_ERROR("Failed to parse \"relay_channels\"!\n");
			return FTDM_FAIL;
		}

		/****************************************************************************************************/
		if ((SNG_SS7_OPR_MODE_ISUP == opr_mode) ||
				(SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) ||
				(SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) ||
				(SNG_SS7_OPR_MODE_MTP2_API == opr_mode)) {

			if (ftmod_ss7_parse_mtp1_links(mtp1_links, opr_mode, &g_ftdm_sngss7_data.cfg)) {
				SS7_ERROR("Failed to parse \"mtp1_links\"!\n");
				return FTDM_FAIL;
			}

			if (ftmod_ss7_parse_mtp2_links(mtp2_links, &g_ftdm_sngss7_data.cfg)) {
				SS7_ERROR("Failed to parse \"mtp2_links\"!\n");
				return FTDM_FAIL;
			}
		}

		/****************************************************************************************************/
		if ( SNG_SS7_OPR_MODE_MTP2_API == opr_mode ) goto done;
		/****************************************************************************************************/

		if ((SNG_SS7_OPR_MODE_ISUP == opr_mode) ||
				(SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) ||
				(SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode)) {

			if (mtp3_links && ftmod_ss7_parse_mtp3_links(mtp3_links, &g_ftdm_sngss7_data.cfg)) {
				SS7_ERROR("Failed to parse \"mtp3_links\"!\n");
				return FTDM_FAIL;
			}

			if (ftmod_ss7_parse_mtp_linksets(mtp_linksets, &g_ftdm_sngss7_data.cfg)) {
				SS7_ERROR("Failed to parse \"mtp_linksets\"!\n");
				return FTDM_FAIL;
			}

			if (ftmod_ss7_parse_mtp_routes(mtp_routes, span, opr_mode, &g_ftdm_sngss7_data.cfg, 0)) {
				SS7_ERROR("Failed to parse \"mtp_routes\"!\n");
				return FTDM_FAIL;
			}
		}
		/****************************************************************************************************/

		if ((SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) ||
		    (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) ||
		    (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) ||
		    (SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode)) {

			if (ftmod_ss7_parse_sctp_links(sctp_ifaces) != FTDM_SUCCESS) {
				SS7_ERROR("Failed to parse <sctp_links>!\n");
				return FTDM_FAIL;
			}
		}
		/****************************************************************************************************/

		if ((SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) ||
				(SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode)) {

			if ((SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) &&
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

		if ((SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) ||
				(SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode)) {

			if (m3ua_network_ifaces && (FTDM_SUCCESS != ftmod_ss7_parse_m3ua_networks(m3ua_network_ifaces))) {
				SS7_ERROR("Failed to parse <sng_m3ua_network_interfaces>!\n");
				return FTDM_FAIL;
			}

			if (m3ua_ps_ifaces && (FTDM_SUCCESS != ftmod_ss7_parse_m3ua_peerservers(m3ua_ps_ifaces))) {
				SS7_ERROR("Failed to parse <sng_m3ua_peer_server_interfaces>!\n");
				return FTDM_FAIL;
			}

			if (m3ua_psp_ifaces && (FTDM_SUCCESS != ftmod_ss7_parse_m3ua_psps(m3ua_psp_ifaces))) {
				SS7_ERROR("Failed to parse <sng_m3ua_peer_server_process_interfaces>!\n");
				return FTDM_FAIL;
			}

			if (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) {
				if (m3ua_user_ifaces && (FTDM_SUCCESS != ftmod_ss7_parse_m3ua_users(m3ua_user_ifaces))) {
					SS7_ERROR("Failed to parse <sng_m3ua_user_interfaces>!\n");
					return FTDM_FAIL;
				}
			} else {
				if (m3ua_nif_ifaces && (FTDM_SUCCESS != ftmod_ss7_parse_m3ua_nif(m3ua_nif_ifaces))) {
					SS7_ERROR("Failed to parse <sng_m3ua_nif_interfaces>!\n");
					return FTDM_FAIL;
				}
			}

			if (m3ua_route_ifaces && (FTDM_SUCCESS != ftmod_ss7_parse_m3ua_routes(m3ua_route_ifaces, opr_mode))) {
				SS7_ERROR("Failed to parse <sng_m3ua_route_interfaces>!\n");
				return FTDM_FAIL;
			}
		}

		/****************************************************************************************************/
		if ((SNG_SS7_OPR_MODE_ISUP == opr_mode) ||
		    (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) ||
		    (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) ||
		    (SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode)) {

			if (isup_interfaces && ftmod_ss7_parse_isup_interfaces(isup_interfaces, opr_mode, &g_ftdm_sngss7_data.cfg, 0)) {
				SS7_ERROR("Failed to parse \"isup_interfaces\"!\n");
				return FTDM_FAIL;
			}

			if (cc_spans && ftmod_ss7_parse_cc_spans(cc_spans, &g_ftdm_sngss7_data.cfg)) {
				SS7_ERROR("Failed to parse \"cc_spans\"!\n");
				return FTDM_FAIL;
			}
		}
		/****************************************************************************************************/
		if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
			/* now configure MTP3/M3UA related routes */
			ftmod_m3ua_cfg_mtp3_m3ua_routes(opr_mode);
		}

		/****************************************************************************************************/
	} else {
		/****************************************************************************************************/
		if (ftmod_ss7_parse_sng_gen_on_reload(gen_config, sng_gen_opr_mode, span_opr_mode, span)) {
			SS7_ERROR("[Reload] Failed to parse \"gen_config\"!\n");
			return FTDM_FAIL;
		}
		/****************************************************************************************************/

		/* As this can be a validating call for new configuration request therefore. no opreating mode will be
		 * present in global span structure.
		 * in such casde take out operating mode from configuration parameter */
		opr_mode = ftmod_ss7_get_operating_mode_by_span_id(span->span_id);
		if (opr_mode == SNG_SS7_OPR_MODE_NONE) {
			opr_mode = ftmod_ss7_get_operatig_mode_cfg(span_opr_mode, sng_gen_opr_mode);
			SS7_DEBUG("Operating mode %s find for new span %s(%d)!\n",
					  ftdm_opr_mode_tostr(opr_mode), span->name, span->span_id);
		} else{
			SS7_DEBUG("[Reload]: Operating mode %s find for span %s(%d)!\n",
					  ftdm_opr_mode_tostr(opr_mode), span->name, span->span_id);
		}

		if (SNG_SS7_OPR_MODE_ISUP == opr_mode) {
			/****************************************************************************************************/
			if (ftmod_ss7_parse_mtp1_links(mtp1_links, opr_mode, &g_ftdm_sngss7_data.cfg_reload)) {
				SS7_ERROR("[Reload] Failed to parse \"mtp1_links\"!\n");
				return FTDM_FAIL;
			}
			/****************************************************************************************************/

			/****************************************************************************************************/
			if (ftmod_ss7_parse_mtp2_links(mtp2_links, &g_ftdm_sngss7_data.cfg_reload)) {
				SS7_ERROR("[Reload] Failed to parse \"mtp2_links\"!\n");
				return FTDM_FAIL;
			}
			/****************************************************************************************************/

			/****************************************************************************************************/
			if (mtp3_links && ftmod_ss7_parse_mtp3_links(mtp3_links, &g_ftdm_sngss7_data.cfg_reload)) {
				SS7_ERROR("[Reload] Failed to parse \"mtp3_links\"!\n");
				return FTDM_FAIL;
			}
			/****************************************************************************************************/

			/****************************************************************************************************/
			if (ftmod_ss7_parse_mtp_linksets(mtp_linksets, &g_ftdm_sngss7_data.cfg_reload)) {
				SS7_ERROR("[Reload] Failed to parse \"mtp_linksets\"!\n");
				return FTDM_FAIL;
			}
			/****************************************************************************************************/

			/****************************************************************************************************/
			if (ftmod_ss7_parse_mtp_routes(mtp_routes, span, opr_mode, &g_ftdm_sngss7_data.cfg_reload, 1)) {
				SS7_ERROR("[Reload] Failed to parse \"mtp_routes\"!\n");
				return FTDM_FAIL;
			}
			/****************************************************************************************************/

			/****************************************************************************************************/
			if (isup_interfaces && ftmod_ss7_parse_isup_interfaces(isup_interfaces, opr_mode, &g_ftdm_sngss7_data.cfg_reload, 1)) {
				SS7_ERROR("[Reload] Failed to parse \"isup_interfaces\"!\n");
				return FTDM_FAIL;
			}
			/****************************************************************************************************/

			/****************************************************************************************************/
			if (cc_spans && ftmod_ss7_parse_cc_spans(cc_spans, &g_ftdm_sngss7_data.cfg_reload)) {
				SS7_ERROR("[Reload] Failed to parse \"cc_spans\"!\n");
				return FTDM_FAIL;
			}
			/****************************************************************************************************/
		} else {
			SS7_ERROR("[Reload] Reconfiguration is not supported for operating Mode %s!\n", ftdm_opr_mode_tostr(opr_mode));
			return FTDM_FAIL;
		}
	}

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
	ftmod_ss7_fill_in_relay_channel(&tmp_chan, &g_ftdm_sngss7_data.cfg);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp1_links(ftdm_conf_node_t *mtp1_links, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg)
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
		if (ftmod_ss7_parse_mtp1_link(mtp1_link, opr_mode, cfg)) {
			SS7_ERROR("Failed to parse \"mtp1_link\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next link */
		mtp1_link = mtp1_link->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp1_link(ftdm_conf_node_t *mtp1_link, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg)
{
	sng_mtp1_link_t		 	mtp1Link;
	ftdm_conf_parameter_t	*parm = mtp1_link->parameters;
	int						num_parms = mtp1_link->n_parameters;
	int					 	i = 0;
    ftdm_span_t				*ftdmspan     = NULL;
    ftdm_iterator_t			*chaniter = NULL;
    ftdm_iterator_t			*curr     = NULL;


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

    if (SNG_SS7_OPR_MODE_MTP2_API == opr_mode) {
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
	ftmod_ss7_fill_in_mtp1_link(&mtp1Link, cfg);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp2_links(ftdm_conf_node_t *mtp2_links, sng_ss7_cfg_t *cfg)
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
		if (ftmod_ss7_parse_mtp2_link(mtp2_link, cfg)) {
			SS7_ERROR("Failed to parse \"mtp2_link\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next link */
		mtp2_link = mtp2_link->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp2_link(ftdm_conf_node_t *mtp2_link, sng_ss7_cfg_t *cfg)
{
	sng_mtp2_link_t			mtp2Link;
	ftdm_conf_parameter_t	*parm = mtp2_link->parameters;
	int					 	num_parms = mtp2_link->n_parameters;
	int						i = 0;
	int						ret = 0;

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
	ftmod_ss7_fill_in_mtp2_link(&mtp2Link, cfg);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp3_links(ftdm_conf_node_t *mtp3_links, sng_ss7_cfg_t *cfg)
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
		if (ftmod_ss7_parse_mtp3_link(mtp3_link, cfg)) {
			SS7_ERROR("Failed to parse \"mtp3_link\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next link */
		mtp3_link = mtp3_link->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp3_link(ftdm_conf_node_t *mtp3_link, sng_ss7_cfg_t *cfg)
{
	sng_mtp3_link_t			mtp3Link;
	ftdm_conf_parameter_t	*parm = mtp3_link->parameters;
	int						num_parms = mtp3_link->n_parameters;
	int						i = 0;
	int						ret = 0;

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
	ftmod_ss7_fill_in_mtp3_link(&mtp3Link, cfg);

	/* move the linktype, switchtype and ssf to the linkset */
	if (cfg->mtpLinkSet[mtp3Link.linkSetId].linkType == 0) {
		cfg->mtpLinkSet[mtp3Link.linkSetId].linkType = mtp3Link.linkType;
	} else if (cfg->mtpLinkSet[mtp3Link.linkSetId].linkType != mtp3Link.linkType) {
		SS7_ERROR("Trying to add an MTP3 Link to a Linkset with a different linkType: mtp3.id=%d, mtp3.name=%s, mtp3.linktype=%d, linkset.id=%d, linkset.linktype=%d\n",
					mtp3Link.id, mtp3Link.name, mtp3Link.linkType,
					cfg->mtpLinkSet[mtp3Link.linkSetId].id, cfg->mtpLinkSet[mtp3Link.linkSetId].linkType);
		return FTDM_FAIL;
	} else {
		/* should print that all is ok...*/
	}

	if (cfg->mtpLinkSet[mtp3Link.linkSetId].switchType == 0) {
		cfg->mtpLinkSet[mtp3Link.linkSetId].switchType = mtp3Link.switchType;
	} else if (cfg->mtpLinkSet[mtp3Link.linkSetId].switchType != mtp3Link.switchType) {
		SS7_ERROR("Trying to add an MTP3 Link to a Linkset with a different switchType: mtp3.id=%d, mtp3.name=%s, mtp3.switchtype=%d, linkset.id=%d, linkset.switchtype=%d\n",
					mtp3Link.id, mtp3Link.name, mtp3Link.switchType,
					cfg->mtpLinkSet[mtp3Link.linkSetId].id, cfg->mtpLinkSet[mtp3Link.linkSetId].switchType);
		return FTDM_FAIL;
	} else {
		/* should print that all is ok...*/
	}

	if (cfg->mtpLinkSet[mtp3Link.linkSetId].ssf == 0) {
		cfg->mtpLinkSet[mtp3Link.linkSetId].ssf = mtp3Link.ssf;
	} else if (cfg->mtpLinkSet[mtp3Link.linkSetId].ssf != mtp3Link.ssf) {
		SS7_ERROR("Trying to add an MTP3 Link to a Linkset with a different ssf: mtp3.id=%d, mtp3.name=%s, mtp3.ssf=%d, linkset.id=%d, linkset.ssf=%d\n",
					mtp3Link.id, mtp3Link.name, mtp3Link.ssf,
					cfg->mtpLinkSet[mtp3Link.linkSetId].id, cfg->mtpLinkSet[mtp3Link.linkSetId].ssf);
		return FTDM_FAIL;
	} else {
		/* should print that all is ok...*/
	}


	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_linksets(ftdm_conf_node_t *mtp_linksets, sng_ss7_cfg_t *cfg)
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
		if (ftmod_ss7_parse_mtp_linkset(mtp_linkset, cfg)) {
			SS7_ERROR("Failed to parse \"mtp_linkset\"!\n");
			return FTDM_FAIL;
		}

		/* move on to the next linkset */
		mtp_linkset = mtp_linkset->next;

	} /* while (mtp_linkset != NULL) */

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_linkset(ftdm_conf_node_t *mtp_linkset, sng_ss7_cfg_t *cfg)
{
	sng_link_set_t			mtpLinkSet;
	ftdm_conf_parameter_t	*parm = mtp_linkset->parameters;
	int						num_parms = mtp_linkset->n_parameters;
	int						i = 0;

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

	ftmod_ss7_fill_in_mtpLinkSet(&mtpLinkSet, cfg);

	/* go through all the mtp3 links and fill in the apc */
	i = 1;
	while (i < (MAX_MTP_LINKS)) {
		if (cfg->mtp3Link[i].linkSetId == mtpLinkSet.id) {
			cfg->mtp3Link[i].apc = mtpLinkSet.apc;
		}

		i++;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_routes(ftdm_conf_node_t *mtp_routes, ftdm_span_t *span, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload)
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
		if (ftmod_ss7_parse_mtp_route(mtp_route, span, opr_mode, cfg, reload)) {
			SS7_ERROR("Failed to parse \"mtp_route\"\n");
			return FTDM_FAIL;
		}

		/* go to the next mtp_route */
		mtp_route = mtp_route->next;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_mtp_route(ftdm_conf_node_t *mtp_route, ftdm_span_t *span, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload)
{
	sng_route_t				mtpRoute;
	ftdm_conf_parameter_t	*parm = mtp_route->parameters;
	int						num_parms = mtp_route->n_parameters;
	int						i = 0;
	sng_link_set_list_t		*lnkSet = NULL;

	ftdm_conf_node_t		*linkset = NULL;
	int						numLinks = 0;
	int						spc=0;
	int						idx=0;
	/* By default the mode for which the span is configured is the considered as the Route config mode */
	ftdm_sngss7_operating_modes_e   tmp_mode = opr_mode;

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
		} else if (!strcasecmp(parm->var, "operating-mode")) {
			if (!strcasecmp(parm->val, "M3UA_SG")) {
				tmp_mode = SNG_SS7_OPR_MODE_M3UA_SG;
			} else if (!strcasecmp(parm->val, "ISUP")) {
				tmp_mode = SNG_SS7_OPR_MODE_ISUP;
			}
			SS7_DEBUG("Found an mtp route operation Mode = %d\n", tmp_mode);
		} else if (!strcasecmp(parm->var, "acc-max-bucket")) {
			if ((g_ftdm_sngss7_data.cfg.sng_acc) && (!reload)) {
				mtpRoute.max_bkt_size = atoi(parm->val);
				SS7_DEBUG("Found Maximum Bucket Size = %d for Automatic Congestion Control feature\n", mtpRoute.max_bkt_size);
			} else {
				if (!reload) {
					SS7_DEBUG("Found invalid configurable parameter Max Bucket Size as Automatic congestion feature is not enable\n");
				} else {
					SS7_ERROR("[Reload] Reconfiguration of ACC parameters are not allowed yet!\n");
				}
			}
		} else if (!strcasecmp(parm->var, "mtp3.t8")) {
			mtpRoute.t8 = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 t8 = %d\n",mtpRoute.t8);
		} else if (!strcasecmp(parm->var, "mtp3.tQuery")) {
			mtpRoute.tQuery = atoi(parm->val);
			SS7_DEBUG("Found an mtp3 tQuery = %d\n",mtpRoute.tQuery);
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
		} else if (!strcasecmp(parm->var, "spc")) {
		/**********************************************************************/
			spc = atoi(parm->val);
			SS7_DEBUG("Found an spc = %d\n",spc);
		/**********************************************************************/
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

		if (!reload) {
			if (g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].id != 0) {
				SS7_DEBUG("Found mtpRoute linkset id = %d that is valid\n",lnkSet->lsId);
			} else {
				SS7_ERROR("Found mtpRoute linkset id = %d that is invalid\n",lnkSet->lsId);
				continue;
			}

			/* pull up the linktype, switchtype, and SSF from the linkset */
			mtpRoute.linkType	= g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].linkType;
			mtpRoute.switchType = g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].switchType;
			mtpRoute.ssf		= g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].ssf;

			/* extract the number of cmbLinkSetId aleady on this linkset */
			numLinks = g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks;

			/* add this routes cmbLinkSetId to the list */
			g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].links[numLinks] = mtpRoute.cmbLinkSetId;

			/* increment the number of cmbLinkSets on this linkset */
			g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks++;
		} else {
			if (g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].id != 0) {
				SS7_DEBUG("[Reload] Found mtpRoute linkset id = %d that is valid\n",lnkSet->lsId);
			} else {
				SS7_ERROR("[Reload] Found mtpRoute linkset id = %d that is invalid\n",lnkSet->lsId);
				continue;
			}

			/* pull up the linktype, switchtype, and SSF from the linkset */
			mtpRoute.linkType	= g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].linkType;
			mtpRoute.switchType = g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].switchType;
			mtpRoute.ssf		= g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].ssf;

			/* extract the number of cmbLinkSetId aleady on this linkset */
			numLinks = g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].numLinks;
			/* add this routes cmbLinkSetId to the list */
			g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].links[numLinks] = mtpRoute.cmbLinkSetId;
			/* increment the number of cmbLinkSets on this linkset */
			g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].numLinks++;
		}

		/* NOTE: We must fill spc if present in mtp3-route configuration
		 * 	 as in case when ISUP is configured first before M3UA
		 * 	 spc is set to 0 */
		/* This is important in case when DLSAP is getting configured while
		 * operating mode is set as ISUP therefore we must set spc if present
		 * at Route level */
		/* fill spc information to mtp3 link config block
		 * mtp3 config block will be identify based on linkset-id*/
		idx = 1;
		while (idx < (MAX_MTP_LINKS)) {
			if ((g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0) &&
					(g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId == lnkSet->lsId)) {
				SS7_DEBUG("Found mtp3Link linkset id = %d \n", lnkSet->lsId);
				g_ftdm_sngss7_data.cfg.mtp3Link[idx].spc = spc;
				SS7_DEBUG("Updated spc[%d] in mtp3Link[%d] \n", spc, idx);
			}
			idx++;
		}
		/* update the linked list */
		lnkSet->next = ftdm_malloc(sizeof(sng_link_set_list_t));
		lnkSet = lnkSet->next;
		lnkSet->lsId = 0;
		lnkSet->next = NULL;
	}

	if (!reload) {
		ftmod_ss7_fill_in_mtp3_route(&mtpRoute, cfg);
		ftmod_ss7_fill_in_nsap(&mtpRoute, tmp_mode, reload, cfg);
	} else {
		ftmod_ss7_fill_in_mtp3_route(&mtpRoute, cfg);

		/* syncronise existing nsap to config reload structure and then fill
		 * the reconfigured nsap if required */
		ftmod_ss7_map_and_sync_existing_nsap(cfg);

		ftmod_ss7_fill_in_nsap(&mtpRoute, tmp_mode, reload, cfg);
	}

	/* If automatic congestion is enable then only check fill in acc timer */
	if ((cfg->sng_acc) && (!reload)) {
		ftmod_ss7_fill_in_acc_timer(&mtpRoute, span);
	}
	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_isup_interfaces(ftdm_conf_node_t *isup_interfaces, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload)
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
		if (ftmod_ss7_parse_isup_interface(isup_interface, opr_mode, cfg, reload)) {
			SS7_ERROR("Failed to parse \"isup_interface\"\n");
			return FTDM_FAIL;
		}

		/* go to the next mtp_route */
		isup_interface = isup_interface->next;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_isup_interface(ftdm_conf_node_t *isup_interface, ftdm_sngss7_operating_modes_e opr_mode, sng_ss7_cfg_t *cfg, int reload)
{
	sng_isup_inf_t		sng_isup;
	sng_isap_t		sng_isap;
	sng_link_set_list_t	*lnkSet = NULL;
	ftdm_conf_parameter_t	*parm = isup_interface->parameters;
	int			num_parms = isup_interface->n_parameters;
	int			i = 0;
	int			ret = 0;
	int			flag_def_rel_loc = 0;
	sng_m3ua_gbl_cfg_t     	*c=NULL;

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
	if (!reload) {
		sngss7_set_flag(&sng_isup, SNGSS7_PAUSED);
	}

	/**************************************************************************/
	if (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) {

		c = &g_ftdm_sngss7_data.cfg.g_m3ua_cfg;
		sng_isup.dpc            = c->rteCfg[sng_isup.mtpRouteId].dpc;
		sng_isup.switchType     = c->nwkCfg[c->rteCfg[sng_isup.mtpRouteId].nwkId].isup_switch_type;
		sng_isap.switchType     = c->nwkCfg[c->rteCfg[sng_isup.mtpRouteId].nwkId].isup_switch_type;
		sng_isup.nwId           = c->rteCfg[sng_isup.mtpRouteId].nwkId;
		sng_isup.ssf            = c->nwkCfg[c->rteCfg[sng_isup.mtpRouteId].nwkId].ssf;
		SS7_INFO("dpc[%d] SwitchType[%d], ssf[%d]\n",sng_isup.dpc, sng_isup.switchType,sng_isup.ssf);

		ftmod_ss7_fill_in_isap(&sng_isap, sng_isup.id, reload, cfg);
		sng_isup.isap 		= sng_isap.id;
		ftmod_ss7_fill_in_isup_interface(&sng_isup, cfg);

		goto done;
	}
	/* trickle down the SPC to all sub entities */
	lnkSet = &cfg->mtpRoute[sng_isup.mtpRouteId].lnkSets;
	while (lnkSet->next != NULL) {
		/* go through all the links and check if they belong to this linkset*/
		for (i = 1; i < MAX_MTP_LINKS;i++) {
			/* check if this link is in the linkset */
			if (cfg->mtp3Link[i].linkSetId == lnkSet->lsId) {
				/* fill in the spc */
				cfg->mtp3Link[i].spc = sng_isup.spc;
			}
		}

		lnkSet = lnkSet->next;
	}

	if (!flag_def_rel_loc) {
		/* Add default value of release location */
		sng_isap.defRelLocation = ILOC_PRIVNETLU; 
	}

	if (reload) {
		ftmod_ss7_map_and_sync_existing_isap(cfg);
		ftmod_ss7_map_and_sync_existing_self_route(&g_ftdm_sngss7_data.cfg_reload);
	}

	/* pull values from the lower levels */
	sng_isap.switchType = cfg->mtpRoute[sng_isup.mtpRouteId].switchType;

	/* fill in the isap */
	ftmod_ss7_fill_in_isap(&sng_isap, sng_isup.id, reload, cfg);

	/* pull values from the lower levels */
	sng_isup.isap 			= sng_isap.id;
	sng_isup.dpc 			= cfg->mtpRoute[sng_isup.mtpRouteId].dpc;
	sng_isup.switchType		= cfg->mtpRoute[sng_isup.mtpRouteId].switchType;
	sng_isup.nwId			= cfg->mtpRoute[sng_isup.mtpRouteId].nwId;

	/* fill in the isup interface */
	ftmod_ss7_fill_in_isup_interface(&sng_isup, cfg);

	/* setup the self mtp3 route */
	i = sng_isup.mtpRouteId;
	if(ftmod_ss7_fill_in_self_route(sng_isup.spc,
									cfg->mtpRoute[i].linkType,
									cfg->mtpRoute[i].switchType,
									cfg->mtpRoute[i].ssf,
									cfg->mtpRoute[i].tQuery,
									sng_isup.mtpRouteId,
									reload,
									cfg)) {

		SS7_ERROR("Failed to fill in self route structure!\n");
		return FTDM_FAIL;

	}

done:
	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_parse_cc_spans(ftdm_conf_node_t *cc_spans, sng_ss7_cfg_t *cfg)
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
		if (ftmod_ss7_parse_cc_span(cc_span, cfg)) {
			SS7_ERROR("Failed to parse \"cc_span\"\n");
			return FTDM_FAIL;
		}

		/* go to the next cc_span */
		cc_span = cc_span->next;
	}

	return FTDM_SUCCESS;

}

/******************************************************************************/
static int ftmod_ss7_parse_cc_span(ftdm_conf_node_t *cc_span, sng_ss7_cfg_t *cfg)
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
	sng_ccSpan.transparent_iam_max_size = g_ftdm_sngss7_data.cfg.transparent_iam_max_size;

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
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "cld_nadi")) {
		/**********************************************************************/
			/* throw the flag so that we know we got this optional parameter */
			flag_cld_nadi = 1;
			sng_ccSpan.cld_nadi = atoi(parm->val);
			SS7_DEBUG("Found default CLD_NADI parm->value = %d\n", sng_ccSpan.cld_nadi);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "rdnis_nadi")) {
		/**********************************************************************/
			/* throw the flag so that we know we got this optional parameter */
			flag_rdnis_nadi = 1;
			sng_ccSpan.rdnis_nadi = atoi(parm->val);
			SS7_DEBUG("Found default RDNIS_NADI parm->value = %d\n", sng_ccSpan.rdnis_nadi);
		/**********************************************************************/
		} else if (!strcasecmp(parm->var, "obci_bita")) {
		/**********************************************************************/
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
		} else if (!strcasecmp(parm->var, "blo_on_ckt_delete")) {
		/**********************************************************************/
			sng_ccSpan.blo_on_ckt_delete = ftdm_true(parm->val);
			SS7_DEBUG("Found blo_on_ckt_delete %d\n", sng_ccSpan.blo_on_ckt_delete);
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
	sng_ccSpan.ssf = cfg->isupIntf[sng_ccSpan.isupInf].ssf;
	sng_ccSpan.switchType = cfg->isupIntf[sng_ccSpan.isupInf].switchType;

	/* add this span to our global listing */
	ftmod_ss7_fill_in_ccSpan(&sng_ccSpan, cfg);

	/* make sure the isup interface structure has something in it */
	if (cfg->isupIntf[sng_ccSpan.isupInf].id == 0) {
		/* fill in the id, so that we know it exists */
		cfg->isupIntf[sng_ccSpan.isupInf].id = sng_ccSpan.isupInf;

		/* default the status to PAUSED */
		sngss7_set_flag(&cfg->isupIntf[sng_ccSpan.isupInf], SNGSS7_PAUSED);
	}

	if (lpa_on_cot == FTDM_TRUE) {
		sngss7_set_options(&cfg->isupIntf[sng_ccSpan.isupInf], SNGSS7_LPA_FOR_COT);
		SS7_DEBUG("LPA on COT is set for interface %d\n", cfg->isupIntf[sng_ccSpan.isupInf].id);
	} else {
		SS7_DEBUG("LPA on COT is cleared for interface %d\n", cfg->isupIntf[sng_ccSpan.isupInf].id);
		sngss7_clear_options(&cfg->isupIntf[sng_ccSpan.isupInf], SNGSS7_LPA_FOR_COT);
	}

	sng_acc_assign_max_bucket(sng_ccSpan.isupInf, nmb_cics_cfg);

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_relay_channel(sng_relay_t *relay_channel, sng_ss7_cfg_t *cfg)
{
	int i;

	/* go through all the existing channels and see if we find a match */
	i = 1;
	while (cfg->relay[i].id != 0) {
		if ((cfg->relay[i].type == relay_channel->type) &&
			(cfg->relay[i].port == relay_channel->port) &&
			(cfg->relay[i].procId == relay_channel->procId) &&
			(!strcasecmp(cfg->relay[i].hostname, relay_channel->hostname))) {

			/* we have a match so break out of this loop */
			break;
		}
		/* move on to the next one */
		i++;
	}

		/* if the id value is 0 that means we didn't find the relay channel */
	if (cfg->relay[i].id  == 0) {
		relay_channel->id = i;
		SS7_DEBUG("found new relay channel on type:%d, hostname:%s, port:%d, procId:%d, id = %d\n",
					relay_channel->type,
					relay_channel->hostname,
					relay_channel->port,
					relay_channel->procId, 
					relay_channel->id);
		sngss7_set_flag(cfg, SNGSS7_RY_PRESENT);
	} else {
		relay_channel->id = i;
		SS7_DEBUG("found existing relay channel on type:%d, hostname:%s, port:%d, procId:%d, id = %d\n",
					relay_channel->type,
					relay_channel->hostname,
					relay_channel->port,
					relay_channel->procId, 
					relay_channel->id);
	}

	cfg->relay[i].id		= relay_channel->id;
	cfg->relay[i].type	= relay_channel->type;
	cfg->relay[i].port	= relay_channel->port;
	cfg->relay[i].procId	= relay_channel->procId;
	strcpy(cfg->relay[i].hostname, relay_channel->hostname);
	strcpy(cfg->relay[i].name, relay_channel->name);

	/* if this is THE listen channel grab the procId and use it */
	if (relay_channel->type == LRY_CT_TCP_LISTEN) {
		cfg->procId = relay_channel->procId;
	}

	/* filling up reconfiguration flag */
	cfg->relay[i].reload = FTDM_FALSE;

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp1_link(sng_mtp1_link_t *mtp1Link, sng_ss7_cfg_t *cfg)
{
	int i = mtp1Link->id;

	/* check if this id value has been used already */
	if (cfg->mtp1Link[i].id == 0) {
		SS7_DEBUG("Found new MTP1 Link: id=%d, name=%s\n", mtp1Link->id, mtp1Link->name);
		sngss7_set_flag(cfg, SNGSS7_MTP1_PRESENT);
	} else {
		SS7_DEBUG("Found an existing MTP1 Link: id=%d, name=%s (old name=%s)\n", 
				   mtp1Link->id, 
				   mtp1Link->name,
				   cfg->mtp1Link[i].name);
	}

	/* copy over the values */
	strcpy((char *)cfg->mtp1Link[i].name, (char *)mtp1Link->name);

	cfg->mtp1Link[i].id			= mtp1Link->id;
	cfg->mtp1Link[i].span		= mtp1Link->span;
	cfg->mtp1Link[i].chan		= mtp1Link->chan;
	cfg->mtp1Link[i].ftdmchan   = mtp1Link->ftdmchan;
	strcpy(cfg->mtp1Link[i].span_name, mtp1Link->span_name);
	/* filling up reconfiguration flag */
	cfg->mtp1Link[i].reload     = FTDM_FALSE;

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp2_link(sng_mtp2_link_t *mtp2Link, sng_ss7_cfg_t *cfg)
{
	int i = mtp2Link->id;

	/* check if this id value has been used already */
	if (cfg->mtp2Link[i].id == 0) {
		SS7_DEBUG("Found new MTP2 Link: id=%d, name=%s\n", mtp2Link->id, mtp2Link->name);
		sngss7_set_flag(cfg, SNGSS7_MTP2_PRESENT);
	} else {
		SS7_DEBUG("Found an existing MTP2 Link: id=%d, name=%s (old name=%s)\n", 
					mtp2Link->id, 
					mtp2Link->name,
					cfg->mtp2Link[i].name);
	}

	/* copy over the values */
	strcpy((char *)cfg->mtp2Link[i].name, (char *)mtp2Link->name);

	cfg->mtp2Link[i].id			= mtp2Link->id;
	cfg->mtp2Link[i].lssuLength	= mtp2Link->lssuLength;
	cfg->mtp2Link[i].errorType	= mtp2Link->errorType;
	cfg->mtp2Link[i].linkType	= mtp2Link->linkType;
	cfg->mtp2Link[i].sapType	= mtp2Link->sapType;
	cfg->mtp2Link[i].mtp1Id		= mtp2Link->mtp1Id;
	cfg->mtp2Link[i].mtp1ProcId	= mtp2Link->mtp1ProcId;

	if ( mtp2Link->t1 != 0 ) {
		cfg->mtp2Link[i].t1	= mtp2Link->t1;
	} else {
		cfg->mtp2Link[i].t1	= 500;
	}

	if ( mtp2Link->t2 != 0 ) {
		cfg->mtp2Link[i].t2		= mtp2Link->t2;
	} else {
		cfg->mtp2Link[i].t2		= 250;
	}

	if ( mtp2Link->t3 != 0 ) {
		cfg->mtp2Link[i].t3		= mtp2Link->t3;
	} else {
		cfg->mtp2Link[i].t3		= 20;
	}

	if ( mtp2Link->t4n != 0 ) {
		cfg->mtp2Link[i].t4n	= mtp2Link->t4n;
	} else {
		cfg->mtp2Link[i].t4n	= 80;
	}

	if ( mtp2Link->t4e != 0 ) {
		cfg->mtp2Link[i].t4e	= mtp2Link->t4e;
	} else {
		cfg->mtp2Link[i].t4e	= 5;
	}

	if ( mtp2Link->t5 != 0 ) {
		cfg->mtp2Link[i].t5		= mtp2Link->t5;
	} else {
		cfg->mtp2Link[i].t5		= 1;
	}

	if ( mtp2Link->t6 != 0 ) {
		cfg->mtp2Link[i].t6		= mtp2Link->t6;
	} else {
		cfg->mtp2Link[i].t6		= 60;
	}

	if ( mtp2Link->t7 != 0 ) {
		cfg->mtp2Link[i].t7		= mtp2Link->t7;
	} else {
		cfg->mtp2Link[i].t7		= 20;
	}

#ifdef SD_HSL
    if ( mtp2Link->t8 != 0 ) {
        cfg->mtp2Link[i].t8     = mtp2Link->t8;
    } else {
        cfg->mtp2Link[i].t8     = 1;    /* 100 msec */
    }

    /* Errored interval parameters from Q.703 A.10.2.5 */
    if ( mtp2Link->sdTe != 0 ) {
        cfg->mtp2Link[i].sdTe   = mtp2Link->sdTe;
    } else {
        cfg->mtp2Link[i].sdTe   = 793;
    }

    if ( mtp2Link->sdUe != 0 ) {
		cfg->mtp2Link[i].sdUe   = mtp2Link->sdUe;
    } else {
		cfg->mtp2Link[i].sdUe   = 198384;
    }

    if ( mtp2Link->sdDe != 0 ) {
        cfg->mtp2Link[i].sdDe   = mtp2Link->sdDe;
    } else {
        cfg->mtp2Link[i].sdDe   = 11328;
    }
#endif
	/* filling up reconfiguration flag */
	cfg->mtp2Link[i].reload 			= FTDM_FALSE;

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp3_link(sng_mtp3_link_t *mtp3Link, sng_ss7_cfg_t *cfg)
{
	int i = mtp3Link->id;

	/* check if this id value has been used already */
	if (cfg->mtp3Link[i].id == 0) {
		SS7_DEBUG("Found new MTP3 Link: id=%d, name=%s\n", mtp3Link->id, mtp3Link->name);
		sngss7_set_flag(cfg, SNGSS7_MTP3_PRESENT);
	} else {
		SS7_DEBUG("Found an existing MTP3 Link: id=%d, name=%s (old name=%s)\n", 
					mtp3Link->id, 
					mtp3Link->name,
					cfg->mtp3Link[i].name);
	}

	/* copy over the values */
	strcpy((char *)cfg->mtp3Link[i].name, (char *)mtp3Link->name);

	cfg->mtp3Link[i].id			= mtp3Link->id;
	cfg->mtp3Link[i].priority	= mtp3Link->priority;
	cfg->mtp3Link[i].linkType	= mtp3Link->linkType;
	cfg->mtp3Link[i].switchType	= mtp3Link->switchType;
	cfg->mtp3Link[i].apc		= mtp3Link->apc;
	cfg->mtp3Link[i].spc		= mtp3Link->spc;	/* unknown at this time */
	cfg->mtp3Link[i].ssf		= mtp3Link->ssf;
	cfg->mtp3Link[i].slc		= mtp3Link->slc;
	cfg->mtp3Link[i].linkSetId	= mtp3Link->linkSetId;
	cfg->mtp3Link[i].mtp2Id		= mtp3Link->mtp2Id;
	cfg->mtp3Link[i].mtp2ProcId	= mtp3Link->mtp2ProcId;

	if (mtp3Link->t1 != 0) {
		cfg->mtp3Link[i].t1		= mtp3Link->t1;
	} else {
		cfg->mtp3Link[i].t1		= 8;
	}
	if (mtp3Link->t2 != 0) {
		cfg->mtp3Link[i].t2		= mtp3Link->t2;
	} else {
		cfg->mtp3Link[i].t2		= 14;
	}
	if (mtp3Link->t3 != 0) {
		cfg->mtp3Link[i].t3		= mtp3Link->t3;
	} else {
		cfg->mtp3Link[i].t3		= 8;
	}
	if (mtp3Link->t4 != 0) {
		cfg->mtp3Link[i].t4		= mtp3Link->t4;
	} else {
		cfg->mtp3Link[i].t4		= 8;
	}
	if (mtp3Link->t5 != 0) {
		cfg->mtp3Link[i].t5		= mtp3Link->t5;
	} else {
		cfg->mtp3Link[i].t5		= 8;
	}
	if (mtp3Link->t7 != 0) {
		cfg->mtp3Link[i].t7		= mtp3Link->t7;
	} else {
		cfg->mtp3Link[i].t7		= 15;
	}
	if (mtp3Link->t12 != 0) {
		cfg->mtp3Link[i].t12	= mtp3Link->t12;
	} else {
		cfg->mtp3Link[i].t12	= 15;
	}
	if (mtp3Link->t13 != 0) {
		cfg->mtp3Link[i].t13	= mtp3Link->t13;
	} else {
		cfg->mtp3Link[i].t13	= 15;
	}
	if (mtp3Link->t14 != 0) {
		cfg->mtp3Link[i].t14	= mtp3Link->t14;
	} else {
		cfg->mtp3Link[i].t14	= 30;
	}
	if (mtp3Link->t17 != 0) {
		cfg->mtp3Link[i].t17	= mtp3Link->t17;
	} else {
		cfg->mtp3Link[i].t17	= 15;
	}
	if (mtp3Link->t22 != 0) {
		cfg->mtp3Link[i].t22	= mtp3Link->t22;
	} else {
		cfg->mtp3Link[i].t22	= 1800;
	}
	if (mtp3Link->t23 != 0) {
		cfg->mtp3Link[i].t23	= mtp3Link->t23;
	} else {
		cfg->mtp3Link[i].t23	= 1800;
	}
	if (mtp3Link->t24 != 0) {
		cfg->mtp3Link[i].t24	= mtp3Link->t24;
	} else {
		cfg->mtp3Link[i].t24	= 5;
	}
	if (mtp3Link->t31 != 0) {
		cfg->mtp3Link[i].t31	= mtp3Link->t31;
	} else {
		cfg->mtp3Link[i].t31	= 50;
	}
	if (mtp3Link->t32 != 0) {
		cfg->mtp3Link[i].t32	= mtp3Link->t32;
	} else {
		cfg->mtp3Link[i].t32	= 120;
	}
	if (mtp3Link->t33 != 0) {
		cfg->mtp3Link[i].t33	= mtp3Link->t33;
	} else {
		cfg->mtp3Link[i].t33	= 3000;
	}
	if (mtp3Link->t34 != 0) {
		cfg->mtp3Link[i].t34	= mtp3Link->t34;
	} else {
		cfg->mtp3Link[i].t34	= 600;
	}
	if (mtp3Link->tbnd != 0) {
		cfg->mtp3Link[i].tbnd	= mtp3Link->tbnd;
	} else {
		cfg->mtp3Link[i].tbnd	= 30000;
	}
	if (mtp3Link->tflc != 0) {
		cfg->mtp3Link[i].tflc	= mtp3Link->tflc;
	} else {
		cfg->mtp3Link[i].tflc	= 300;
	}

	/* filling up reconfiguration flag */
	cfg->mtp3Link[i].reload 	= FTDM_FALSE;

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtpLinkSet(sng_link_set_t *mtpLinkSet, sng_ss7_cfg_t *cfg)
{
	int	i = mtpLinkSet->id;

	strncpy((char *)cfg->mtpLinkSet[i].name, (char *)mtpLinkSet->name, MAX_NAME_LEN-1);

	cfg->mtpLinkSet[i].id		= mtpLinkSet->id;
	cfg->mtpLinkSet[i].apc		= mtpLinkSet->apc;

	/* these values are filled in as we find routes and start allocating cmbLinkSetIds */
	cfg->mtpLinkSet[i].minActive	= mtpLinkSet->minActive;
	cfg->mtpLinkSet[i].numLinks	= 0;

	/* filling up reconfiguration flag */
	cfg->mtpLinkSet[i].reload 	= FTDM_FALSE;

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_mtp3_route(sng_route_t *mtp3_route, sng_ss7_cfg_t *cfg)
{
	sng_link_set_list_t	*lnkSet = NULL;
	int					i = mtp3_route->id;
	int					tmp = 0;


	/* check if this id value has been used already */
	if (cfg->mtpRoute[i].id == 0) {
		SS7_DEBUG("Found new MTP3 Link: id=%d, name=%s\n", mtp3_route->id, mtp3_route->name);
	} else {
		SS7_DEBUG("Found an existing MTP3 Link: id=%d, name=%s (old name=%s)\n", 
					mtp3_route->id, 
					mtp3_route->name,
					cfg->mtpRoute[i].name);
	}

	/* fill in the cmbLinkSet in the linkset structure */
	lnkSet = &mtp3_route->lnkSets;

	tmp = cfg->mtpLinkSet[lnkSet->lsId].numLinks;
	cfg->mtpLinkSet[lnkSet->lsId].links[tmp] = mtp3_route->cmbLinkSetId;
	cfg->mtpLinkSet[lnkSet->lsId].numLinks++;

	while (lnkSet->next != NULL) {
		tmp = cfg->mtpLinkSet[lnkSet->lsId].numLinks;
		cfg->mtpLinkSet[lnkSet->lsId].links[tmp] = mtp3_route->cmbLinkSetId;
		cfg->mtpLinkSet[lnkSet->lsId].numLinks++;

		lnkSet = lnkSet->next;
	}

	strcpy((char *)cfg->mtpRoute[i].name, (char *)mtp3_route->name);

	cfg->mtpRoute[i].id				= mtp3_route->id;
	cfg->mtpRoute[i].dpc			= mtp3_route->dpc;
	cfg->mtpRoute[i].linkType		= mtp3_route->linkType;
	cfg->mtpRoute[i].switchType		= mtp3_route->switchType;
	cfg->mtpRoute[i].cmbLinkSetId	= mtp3_route->cmbLinkSetId;
	cfg->mtpRoute[i].isSTP 			= mtp3_route->isSTP;
	cfg->mtpRoute[i].nwId			= mtp3_route->nwId;
	cfg->mtpRoute[i].lnkSets		= mtp3_route->lnkSets;
	cfg->mtpRoute[i].ssf			= mtp3_route->ssf;
	cfg->mtpRoute[i].dir			= SNG_RTE_DN;

	if (mtp3_route->tQuery != 0) {
		cfg->mtpRoute[i].tQuery 	= mtp3_route->tQuery;
	} else {
		cfg->mtpRoute[i].tQuery 	= 500; /* 5 sec */
	}
	if (mtp3_route->t6 != 0) {
		cfg->mtpRoute[i].t6		= mtp3_route->t6;
	} else {
		cfg->mtpRoute[i].t6		= 8;
	}
	if (mtp3_route->t8 != 0) {
		cfg->mtpRoute[i].t8		= mtp3_route->t8;
	} else {
		cfg->mtpRoute[i].t8		= 12;
	}
	if (mtp3_route->t10 != 0) {
		cfg->mtpRoute[i].t10		= mtp3_route->t10;
	} else {
		cfg->mtpRoute[i].t10 		= 300;
	}
	if (mtp3_route->t11 != 0) {
		cfg->mtpRoute[i].t11		= mtp3_route->t11;
	} else {
		cfg->mtpRoute[i].t11 		= 300;
	}
	if (mtp3_route->t15 != 0) {
		cfg->mtpRoute[i].t15		= mtp3_route->t15;
	} else {
		cfg->mtpRoute[i].t15 		= 30;
	}
	if (mtp3_route->t16 != 0) {
		cfg->mtpRoute[i].t16		= mtp3_route->t16;
	} else {
		cfg->mtpRoute[i].t16 		= 20;
	}
	if (mtp3_route->t18 != 0) {
		cfg->mtpRoute[i].t18		= mtp3_route->t18;
	} else {
		cfg->mtpRoute[i].t18 		= 200;
	}
	if (mtp3_route->t19 != 0) {
		cfg->mtpRoute[i].t19		= mtp3_route->t19;
	} else {
		cfg->mtpRoute[i].t19 		= 690;
	}
	if (mtp3_route->t21 != 0) {
		cfg->mtpRoute[i].t21		= mtp3_route->t21;
	} else {
		cfg->mtpRoute[i].t21 		= 650;
	}
	if (mtp3_route->t25 != 0) {
		cfg->mtpRoute[i].t25		= mtp3_route->t25;
	} else {
		cfg->mtpRoute[i].t25 		= 100;
	}
	if (mtp3_route->t26 != 0) {
		cfg->mtpRoute[i].t26		= mtp3_route->t26;
	} else {
		cfg->mtpRoute[i].t26 		= 100;
	}

	/* Since this is not self route there for there should not be
	 * associated route for this one */
	memset(cfg->mtpRoute[i].associated_route_id, 0, sizeof(cfg->mtpRoute[i].associated_route_id));

	/* filling up reconfiguration flag */
	cfg->mtpRoute[i].reload 		= FTDM_FALSE;

	return 0;
}

/******************************************************************************/
/* syncronise existing self route configuration so that there will be no mismatch
 * of self route in case of adding or deleting a new span */
static int ftmod_ss7_map_and_sync_existing_self_route(sng_ss7_cfg_t *cfg)
{
	int idx = MAX_MTP_ROUTES + 1;
	int i   = 0;

	while (idx < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1)) {
		if (g_ftdm_sngss7_data.cfg.mtpRoute[idx].id == 0) {
			idx++;
			continue;
		}

		strncpy((char *)cfg->mtpRoute[idx].name, "self-route", MAX_NAME_LEN-1);

		cfg->mtpRoute[idx].id			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].id;
		cfg->mtpRoute[idx].dpc			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].dpc;
		cfg->mtpRoute[idx].linkType		= g_ftdm_sngss7_data.cfg.mtpRoute[idx].linkType;
		cfg->mtpRoute[idx].switchType	= g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType;
		cfg->mtpRoute[idx].cmbLinkSetId	= g_ftdm_sngss7_data.cfg.mtpRoute[idx].id;
		cfg->mtpRoute[idx].isSTP 		= g_ftdm_sngss7_data.cfg.mtpRoute[idx].isSTP;
		cfg->mtpRoute[idx].ssf			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf;
		cfg->mtpRoute[idx].dir			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].dir;
		cfg->mtpRoute[idx].t6			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t6;
		cfg->mtpRoute[idx].t8			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t8;
		cfg->mtpRoute[idx].t10			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t10;
		cfg->mtpRoute[idx].t11			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t11;
		cfg->mtpRoute[idx].t15			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t15;
		cfg->mtpRoute[idx].t16			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t16;
		cfg->mtpRoute[idx].t18			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t18;
		cfg->mtpRoute[idx].t19			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t19;
		cfg->mtpRoute[idx].t21			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t21;
		cfg->mtpRoute[idx].t25			= g_ftdm_sngss7_data.cfg.mtpRoute[idx].t25;

		for (i = 0 ; i < (MAX_MTP_ROUTES + 1); i++) {
			if (!(g_ftdm_sngss7_data.cfg.mtpRoute[idx].associated_route_id[i])) {
				break;
			}

			/* Fill in associated route ID in case of self route ID for reference */
			cfg->mtpRoute[idx].associated_route_id[i] = g_ftdm_sngss7_data.cfg.mtpRoute[idx].associated_route_id[i];
		}

		/* filling up reconfiguration flag */
		cfg->mtpRoute[idx].reload 		= g_ftdm_sngss7_data.cfg.mtpRoute[idx].reload;

		idx++;
	}

	return 0;
}

/******************************************************************************/
/* NOTE: MTP Routes and MTP Self routes must have different start index values.
 * 	 If MTP Routes ranges from index 1 to 100 then MTP SELf Route must
 * 	 range from 101 to 200 i.e. for MTP Route at index 1 must have its
 * 	 corresponding MTP SELF Route ar index 101 instead of 2/3 or anything
 * 	 in between 100 as that is reserved only for MTP Routes.
 *
 * 	 Reason for this implementation can be expalined by below e.g.:
 *
 * 	 Case: If NSG is configured with single signalling span and after some
 * 	       time another signalling span is configured using Dynamic
 * 	       configuration feature.
 *
 * 	 Configuration & Result:
 * 	 	1) With self route previous implentation:
 *
 * 	 	  1.1) Route Configuration when single signalling span is
 * 	 	       configured:
 *
 * 	 	       g_ftdm_sngss7_data.cfg.mtpRoute[1] will have MTP Route 1
 * 	 	       g_ftdm_sngss7_data.cfg.mtpRoute[2] will have the
 * 	 	       respective MTP Self route
 *
 * 	 	  1.2) Route Configuration after addition of new signalling
 * 	 	       span using Dynamic Configuration:
 *
 * 	 	       g_ftdm_sngss7_data.cfg.mtpRoute[1] will have MTP Route 1
 * 	 	       g_ftdm_sngss7_data.cfg.mtpRoute[2] will have MTP Route 2
 * 	 	       g_ftdm_sngss7_data.cfg.mtpRoute[3] will have self route
 * 	 	       for MTP ROUTE 1
 * 	 	       g_ftdm_sngss7_data.cfg.mtpRoute[4] will have self route
 * 	 	       for MTP ROUTE 4
 *
 * 	 	  Result: This is INVALID as g_ftdm_sngss7_data.cfg.mtpRoute[2]
 * 	 	  	  is already configured previously as a self route and
 * 	 	  	  therefore the updated configuration will not work
 *
 *
 * 	 	2) With self route current implentation:
 *
 * 	 	  2.1) Route Configuration when single signalling span is
 * 	 	       configured:
 *
 * 	 	       cfg->mtpRoute[1] will have MTP Route 1
 * 	 	       cfg->mtpRoute[101] will have the
 * 	 	       respective MTP Self route
 *
 * 	 	  2.2) Route Configuration after addition of new signalling
 * 	 	       span using Dynamic Configuration:
 *
 * 	 	       cfg->mtpRoute[1] will have MTP Route 1
 * 	 	       cfg->mtpRoute[2] will have MTP Route 2
 * 	 	       cfg->mtpRoute[101] will have self route
 * 	 	       for MTP ROUTE 1
 * 	 	       cfg->mtpRoute[102] will have self route
 * 	 	       for MTP ROUTE 4
 *
 * 	 	  Result: This is VALID and configuration works SUCCESSFULLY
 */
static int ftmod_ss7_fill_in_self_route(int spc, int linkType, int switchType, int ssf, int tQuery, int mtp_route_id, int reconfig, sng_ss7_cfg_t *cfg)
{
	ftdm_bool_t status = FTDM_FALSE;
	int			i = MAX_MTP_ROUTES + 1;
	int			idx = 0;

	if (!reconfig) {
		/* check if there is any self present in list with safe dpc and switchtype */
		while ((i > MAX_MTP_ROUTES) && (i < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1))) {
			if ((cfg->mtpRoute[i].dpc == spc) &&
				(cfg->mtpRoute[i].switchType == switchType)) {
				/* we have a match so break out of this loop */
				break;
			}
			/* move on to the next one */
			i++;
		}
	} else {
		/* Check if this is a reconfiguration request and there is any self
		 * route present mapped to mtp route passed, reconfigure it. If not
		 * then please check from whole list of self route if there is any
		 * self route present with same dpc and switchType value then add
		 * this mtp route to associated route list and reconfigure self route */
		while ((i > MAX_MTP_ROUTES) && (i < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1))) {
			for (idx = 0; idx < (MAX_MTP_ROUTES + 1); idx++) {
				if (!(cfg->mtpRoute[i].associated_route_id[idx])) {
					break;
				}

				if (cfg->mtpRoute[i].associated_route_id[idx] == mtp_route_id) {
					/* we have a match so break out of this loop */
					status = FTDM_TRUE;
					break;
				}
			}
			if (status == FTDM_TRUE) {
				break;
			} else {
				/* move on to the next one */
				i++;
			}
		}

		if ((i == (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1)) || (status == FTDM_FALSE)) {
			/* sice we didnot find any matching self route, now check is there is any self
			 * present with matching dpc and switch type value in self route list */
			i = MAX_MTP_ROUTES + 1;
			while ((i > MAX_MTP_ROUTES) && (i < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1))) {
				if ((cfg->mtpRoute[i].dpc == spc) &&
				    (cfg->mtpRoute[i].switchType == switchType)) {
					/* we have a match so break out of this loop */
					break;
				}
				/* move on to the next one */
				i++;
			}
		}
	}

	if (cfg->mtpRoute[i].id == 0) {
		/* this is a new route...find the first free spot */
		i = MAX_MTP_ROUTES + 1;
		while ((i > MAX_MTP_ROUTES) && (i < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1))) {
			if (cfg->mtpRoute[i].id == 0) {
				/* we have a match so break out of this loop */
				break;
			}
			/* move on to the next one */
			i++;
		}
		cfg->mtpRoute[i].id = i;
		SS7_DEBUG("found new mtp3 self route\n");
	} else {
		cfg->mtpRoute[i].id = i;
		SS7_DEBUG("found existing mtp3 self route\n");
	}

	strncpy((char *)cfg->mtpRoute[i].name, "self-route", MAX_NAME_LEN-1);

	cfg->mtpRoute[i].id				= i;
	cfg->mtpRoute[i].dpc			= spc;
	cfg->mtpRoute[i].linkType		= linkType;
	cfg->mtpRoute[i].switchType		= switchType;
	cfg->mtpRoute[i].cmbLinkSetId	= i;
	cfg->mtpRoute[i].isSTP 			= 0;
	cfg->mtpRoute[i].ssf			= ssf;
	cfg->mtpRoute[i].dir			= SNG_RTE_UP;
	cfg->mtpRoute[i].t6				= 8;
	cfg->mtpRoute[i].t8				= 12;
	cfg->mtpRoute[i].t10			= 300;
	cfg->mtpRoute[i].t11			= 300;
	cfg->mtpRoute[i].t15			= 30;
	cfg->mtpRoute[i].t16			= 20;
	cfg->mtpRoute[i].t18			= 200;
	cfg->mtpRoute[i].t19			= 690;
	cfg->mtpRoute[i].t21			= 650;
	cfg->mtpRoute[i].t25			= 100;
	cfg->mtpRoute[i].tQuery			= tQuery;

	for (idx = 0; idx < (MAX_MTP_ROUTES + 1); idx++) {
		if (cfg->mtpRoute[i].associated_route_id[idx] != 0) {
			if (cfg->mtpRoute[i].associated_route_id[idx] == mtp_route_id) {
				break;
			} else {
				continue;
			}
		}
		/* Fill in associated route ID in case of self route ID for reference */
		cfg->mtpRoute[i].associated_route_id[idx] = mtp_route_id;
		break;
	}

	/* filling up reconfiguration flag */
	cfg->mtpRoute[i].reload 		= FTDM_FALSE;

	return 0;
}

/******************************************************************************/
/* syncronise existing nsap configuration so that there will be no mismatch
 * of nsap in case of adding or deleting a new span */
static int ftmod_ss7_map_and_sync_existing_nsap(sng_ss7_cfg_t *cfg)
{
	int idx = 1;
	int i   = 0;

	while (idx < (MAX_NSAPS)) {
		if (g_ftdm_sngss7_data.cfg.nsap[idx].id == 0) {
			idx++;
			continue;
		}

		memset (cfg->nsap[idx].associated_route_id, 0, sizeof(cfg->nsap[idx].associated_route_id));

		cfg->nsap[idx].id 		= g_ftdm_sngss7_data.cfg.nsap[idx].id;
		cfg->nsap[idx].spId		= g_ftdm_sngss7_data.cfg.nsap[idx].spId;
		cfg->nsap[idx].suId		= g_ftdm_sngss7_data.cfg.nsap[idx].suId;
		cfg->nsap[idx].nwId		= g_ftdm_sngss7_data.cfg.nsap[idx].nwId;
		cfg->nsap[idx].linkType		= g_ftdm_sngss7_data.cfg.nsap[idx].linkType;
		cfg->nsap[idx].switchType	= g_ftdm_sngss7_data.cfg.nsap[idx].switchType;
		cfg->nsap[idx].ssf		= g_ftdm_sngss7_data.cfg.nsap[idx].ssf;

		for (i = 0; i < (MAX_MTP_ROUTES + 1); i++) {
			if (!(g_ftdm_sngss7_data.cfg.mtpRoute[idx].associated_route_id[i])) {
				break;
			}

			/* Fill in associated route ID in case of nsap configuration for reference */
			cfg->mtpRoute[idx].associated_route_id[i] = g_ftdm_sngss7_data.cfg.mtpRoute[idx].associated_route_id[i];
		}

		/* filling up reconfiguration flag */
		cfg->nsap[i].reload 			= FTDM_FALSE;

		idx++;
	}

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_nsap(sng_route_t *mtp3_route, ftdm_sngss7_operating_modes_e opr_mode, int reconfig, sng_ss7_cfg_t *cfg)
{
	ftdm_bool_t status = FTDM_FALSE;
	int idx        = 0;
	int i          = 1;

	if (!reconfig) {
		/* check is there is any NSAP present with same linkType, switchType and ssf value */
		while (i < (MAX_NSAPS)) {
			if ((cfg->nsap[i].linkType == mtp3_route->linkType) &&
			    (cfg->nsap[i].switchType == mtp3_route->switchType) &&
			    (cfg->nsap[i].ssf == mtp3_route->ssf)) {

				/* we have a match so break out of this loop */
				break;
			}
			/* move on to the next one */
			i++;
		}

		if (i != (MAX_NSAPS)) {
		       goto start_config;
		}

		i = 1;
		/* go through all the existing interfaces and see if we find a match */
		while (cfg->nsap[i].id != 0) {
			if ((cfg->nsap[i].linkType == mtp3_route->linkType) &&
			    (cfg->nsap[i].switchType == mtp3_route->switchType) &&
			    (cfg->nsap[i].ssf == mtp3_route->ssf) &&
				(cfg->nsap[i].opr_mode == opr_mode)) {

				/* we have a match so break out of this loop */
				break;
			}
			/* move on to the next one */
			i++;
		}
	} else {
		/* Check if this is a reconfiguration request and there is any nsap
		 * present mapped to mtp route passed, reconfigure it. If not then
		 * please check from whole list of NSAP is there is any nsap
		 * present with same linkType, switchType and ssf value then add
		 * this interface to associated route list and reconfigure NSAP */
		while (i < (MAX_NSAPS)) {
			for (idx = 0; idx < (MAX_MTP_ROUTES + 1); idx++) {
				if (!(cfg->nsap[i].associated_route_id[idx])) {
					break;
				}

				if (cfg->nsap[i].associated_route_id[idx] == mtp3_route->id) {
					/* we have a match so break out of this loop */
					status = FTDM_TRUE;
					break;
				}
			}
			if (status == FTDM_TRUE) {
				break;
			} else {
				/* move on to the next one */
				i++;
			}
		}

		if ((i == (MAX_NSAPS)) || (status == FTDM_FALSE)) {
			/* sice we didnot find any matching NSAP there now check is there is any nsap
			 * present with matching linkType, switch type and ssf value in NSAP list */
			i = 1;
			while (i < (MAX_NSAPS)) {
				if ((cfg->nsap[i].linkType == mtp3_route->linkType) &&
				    (cfg->nsap[i].switchType == mtp3_route->switchType) &&
				    (cfg->nsap[i].ssf == mtp3_route->ssf) &&
					(cfg->nsap[i].opr_mode == opr_mode)) {

					/* we have a match so break out of this loop */
					break;
				}
				/* move on to the next one */
				i++;
			}
		}
	}

start_config:
	if (cfg->nsap[i].id == 0) {
		cfg->nsap[i].id = i;
		SS7_DEBUG("found new mtp3_isup interface, id is = %d\n", cfg->nsap[i].id);
	} else {
		cfg->nsap[i].id = i;
		SS7_DEBUG("found existing mtp3_isup interface, id is = %d\n", cfg->nsap[i].id);
	}

	cfg->nsap[i].spId			= cfg->nsap[i].id;
	cfg->nsap[i].suId			= cfg->nsap[i].id;
	cfg->nsap[i].nwId			= mtp3_route->nwId;
	cfg->nsap[i].linkType		= mtp3_route->linkType;
	cfg->nsap[i].switchType		= mtp3_route->switchType;
	cfg->nsap[i].ssf			= mtp3_route->ssf;
	cfg->nsap[i].opr_mode		= opr_mode;

	for (idx = 0; idx < (MAX_MTP_ROUTES + 1); idx++) {
		if (cfg->nsap[i].associated_route_id[idx] != 0) {
			if (cfg->nsap[i].associated_route_id[idx] == mtp3_route->id) {
				break;
			} else {
				continue;
			}
		}
		/* Fill in associated route ID in case of self route ID for reference */
		cfg->nsap[i].associated_route_id[idx] = mtp3_route->id;
		break;
	}

	/* filling up reconfiguration flag */
	cfg->nsap[i].reload 			= FTDM_FALSE;

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_isup_interface(sng_isup_inf_t *sng_isup, sng_ss7_cfg_t *cfg)
{
	int i = sng_isup->id;

	/* check if this id value has been used already */
	if (cfg->isupIntf[i].id == 0) {
		SS7_DEBUG("Found new ISUP Interface: id=%d, name=%s\n", sng_isup->id, sng_isup->name);
		sngss7_set_flag(cfg, SNGSS7_ISUP_PRESENT);
	} else {
		SS7_DEBUG("Found an existing ISUP Interface: id=%d, name=%s (old name=%s)\n", 
					sng_isup->id, 
					sng_isup->name,
					cfg->isupIntf[i].name);
	}

	strncpy((char *)cfg->isupIntf[i].name, (char *)sng_isup->name, MAX_NAME_LEN-1);

	cfg->isupIntf[i].id				= sng_isup->id;
	cfg->isupIntf[i].mtpRouteId		= sng_isup->mtpRouteId;
	cfg->isupIntf[i].nwId			= sng_isup->nwId;
	cfg->isupIntf[i].dpc			= sng_isup->dpc;
	cfg->isupIntf[i].spc			= sng_isup->spc;
	cfg->isupIntf[i].switchType		= sng_isup->switchType;
	cfg->isupIntf[i].ssf			= sng_isup->ssf;
	cfg->isupIntf[i].isap			= sng_isup->isap;
	cfg->isupIntf[i].options		= sng_isup->options;
	if (sng_isup->pauseAction != 0) {
		cfg->isupIntf[i].pauseAction	= sng_isup->pauseAction;
	} else {
		cfg->isupIntf[i].pauseAction	= SI_PAUSE_CLRTRAN ;
	}
	if (sng_isup->t4 != 0) {
		cfg->isupIntf[i].t4		= sng_isup->t4;
	} else {
		cfg->isupIntf[i].t4		= 3000;
	}
	if (sng_isup->t11 != 0) {
		cfg->isupIntf[i].t11		= sng_isup->t11;
	} else {
		cfg->isupIntf[i].t11		= 170;
	}
	if (sng_isup->t18 != 0) {
		cfg->isupIntf[i].t18		= sng_isup->t18;
	} else {
		cfg->isupIntf[i].t18		= 300;
	}
	if (sng_isup->t19 != 0) {
		cfg->isupIntf[i].t19		= sng_isup->t19;
	} else {
		cfg->isupIntf[i].t19		= 3000;
	}
	if (sng_isup->t20 != 0) {
		cfg->isupIntf[i].t20		= sng_isup->t20;
	} else {
		cfg->isupIntf[i].t20		= 300;
	}
	if (sng_isup->t21 != 0) {
		cfg->isupIntf[i].t21		= sng_isup->t21;
	} else {
		cfg->isupIntf[i].t21		= 3000;
	}
	if (sng_isup->t22 != 0) {
		cfg->isupIntf[i].t22		= sng_isup->t22;
	} else {
		cfg->isupIntf[i].t22		= 300;
	}
	if (sng_isup->t23 != 0) {
		cfg->isupIntf[i].t23		= sng_isup->t23;
	} else {
		cfg->isupIntf[i].t23		= 3000;
	}
	if (sng_isup->t24 != 0) {
		cfg->isupIntf[i].t24		= sng_isup->t24;
	} else {
		cfg->isupIntf[i].t24		= 10;
	}
	if (sng_isup->t25 != 0) {
		cfg->isupIntf[i].t25		= sng_isup->t25;
	} else {
		cfg->isupIntf[i].t25		= 20;
	}
	if (sng_isup->t26 != 0) {
		cfg->isupIntf[i].t26		= sng_isup->t26;
	} else {
		cfg->isupIntf[i].t26		= 600;
	}
	if (sng_isup->t28 != 0) {
		cfg->isupIntf[i].t28		= sng_isup->t28;
	} else {
		cfg->isupIntf[i].t28		= 100;
	}
	if (sng_isup->t29 != 0) {
		cfg->isupIntf[i].t29		= sng_isup->t29;
	} else {
		cfg->isupIntf[i].t29		= 6;
	}
	if (sng_isup->t30 != 0) {
		cfg->isupIntf[i].t30		= sng_isup->t30;
	} else {
		cfg->isupIntf[i].t30		= 50;
	}
	if (sng_isup->t32 != 0) {
		cfg->isupIntf[i].t32		= sng_isup->t32;
	} else {
		cfg->isupIntf[i].t32		= 30;
	}
	if (sng_isup->t37 != 0) {
		cfg->isupIntf[i].t37		= sng_isup->t37;
	} else {
		cfg->isupIntf[i].t37		= 20;
	}
	if (sng_isup->t38 != 0) {
		cfg->isupIntf[i].t38		= sng_isup->t38;
	} else {
		cfg->isupIntf[i].t38		= 1200;
	}
	if (sng_isup->t39 != 0) {
		cfg->isupIntf[i].t39		= sng_isup->t39;
	} else {
		cfg->isupIntf[i].t39		= 300;
	}
	if (sng_isup->tfgr != 0) {
		cfg->isupIntf[i].tfgr		= sng_isup->tfgr;
	} else {
		cfg->isupIntf[i].tfgr		= 50;
	}
	if (sng_isup->tpause != 0) {
		cfg->isupIntf[i].tpause		= sng_isup->tpause;
	} else {
		cfg->isupIntf[i].tpause		= 3000;
	}
	if (sng_isup->tstaenq != 0) {
		cfg->isupIntf[i].tstaenq	= sng_isup->tstaenq;
	} else {
		cfg->isupIntf[i].tstaenq	= 5000;
	}

	/* filling up reconfiguration flag */
	cfg->isupIntf[i].reload 		= FTDM_FALSE;

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_map_and_sync_existing_isap(sng_ss7_cfg_t *cfg)
{
	int idx = 1;
	int i;

	/* go through all the existing interfaces and see if we find a match */
	while (idx < (MAX_ISAPS)) {
		if (g_ftdm_sngss7_data.cfg.isap[idx].id == 0) {
			idx++;
			continue;
		}

		memset (cfg->isap[idx].isup_intf_id, 0, sizeof(cfg->isap[idx].isup_intf_id));

		cfg->isap[idx].id				= g_ftdm_sngss7_data.cfg.isap[idx].id;
		cfg->isap[idx].suId				= g_ftdm_sngss7_data.cfg.isap[idx].suId;
		cfg->isap[idx].spId				= g_ftdm_sngss7_data.cfg.isap[idx].spId;
		cfg->isap[idx].switchType		= g_ftdm_sngss7_data.cfg.isap[idx].switchType;
		cfg->isap[idx].ssf				= g_ftdm_sngss7_data.cfg.isap[idx].ssf;
		cfg->isap[idx].defRelLocation	= g_ftdm_sngss7_data.cfg.isap[idx].defRelLocation;
		cfg->isap[idx].t1				= g_ftdm_sngss7_data.cfg.isap[idx].t1;
		cfg->isap[idx].t2				= g_ftdm_sngss7_data.cfg.isap[idx].t2;
		cfg->isap[idx].t5				= g_ftdm_sngss7_data.cfg.isap[idx].t5;
		cfg->isap[idx].t6				= g_ftdm_sngss7_data.cfg.isap[idx].t6;
		cfg->isap[idx].t7				= g_ftdm_sngss7_data.cfg.isap[idx].t7;
		cfg->isap[idx].t8				= g_ftdm_sngss7_data.cfg.isap[idx].t8;
		cfg->isap[idx].t9				= g_ftdm_sngss7_data.cfg.isap[idx].t9;
		cfg->isap[idx].t27				= g_ftdm_sngss7_data.cfg.isap[idx].t27;
		cfg->isap[idx].t31				= g_ftdm_sngss7_data.cfg.isap[idx].t31;
		cfg->isap[idx].t33				= g_ftdm_sngss7_data.cfg.isap[idx].t33;
		cfg->isap[idx].t34				= g_ftdm_sngss7_data.cfg.isap[idx].t34;
		cfg->isap[idx].t36				= g_ftdm_sngss7_data.cfg.isap[idx].t36;
		cfg->isap[idx].tccr				= g_ftdm_sngss7_data.cfg.isap[idx].tccr;
		cfg->isap[idx].tccrt			= g_ftdm_sngss7_data.cfg.isap[idx].tccrt;
		cfg->isap[idx].tex				= g_ftdm_sngss7_data.cfg.isap[idx].tex;
		cfg->isap[idx].tcrm				= g_ftdm_sngss7_data.cfg.isap[idx].tcrm;
		cfg->isap[idx].tcra				= g_ftdm_sngss7_data.cfg.isap[idx].tcra;
		cfg->isap[idx].tect				= g_ftdm_sngss7_data.cfg.isap[idx].tect;
		cfg->isap[idx].trelrsp			= g_ftdm_sngss7_data.cfg.isap[idx].trelrsp;
		cfg->isap[idx].tfnlrelrsp		= g_ftdm_sngss7_data.cfg.isap[idx].tfnlrelrsp;

		for (i = 0; i < (MAX_ISUP_INFS + 1); i++) {
			if (!(g_ftdm_sngss7_data.cfg.isap[idx].isup_intf_id[i])) {
				break;
			}

			/* Fill in associated ISUP interface ID in case of nsap configuration for reference */
			cfg->isap[idx].isup_intf_id[i] = g_ftdm_sngss7_data.cfg.isap[idx].isup_intf_id[i];
			break;
		}

		/* filling up reconfiguration flag */
		cfg->isap[idx].reload = FTDM_FALSE;

		idx++;
	}

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_isap(sng_isap_t *sng_isap, int isup_intf_id, int reconfig, sng_ss7_cfg_t *cfg)
{
	ftdm_bool_t status = FTDM_FALSE;
	int			idx	   = 0;
	int			i  	   = 1;

	if (!reconfig) {
		/* check if there is any SAP present where we have matching switch type and ssf value */
		while (i < (MAX_ISAPS)) {
			if ((cfg->isap[i].switchType == sng_isap->switchType) &&
			    (cfg->isap[i].ssf == sng_isap->ssf)) {

				/* we have a match so break out of this loop */
				break;
			}
			/* move on to the next one */
			i++;
		}
		if (i != (MAX_ISAPS)) {
			goto start_config;
		}

		i = 1;
		/* go through all the existing interfaces and see if we find a match */
		while (cfg->isap[i].id != 0) {
			if ((cfg->isap[i].switchType == sng_isap->switchType) &&
			    (cfg->isap[i].ssf == sng_isap->ssf)) {

				/* we have a match so break out of this loop */
				break;
			}
			/* move on to the next one */
			i++;
		}
	} else {
		/* Check if this is a reconfiguration request and there is any isap
		 * present mapped to this interface to reconfigure it. If not then
		 * please check from whole list of ISAP is there is any isap
		 * present with same switchType and ssf value then add this interface
		 * to isap interface list and then recofigure it */
		while (i < (MAX_ISAPS)) {
			for (idx = 0; idx < (MAX_ISUP_INFS + 1); idx++) {
				if (!(cfg->isap[i].isup_intf_id[idx])) {
					break;
				}

				if (cfg->isap[i].isup_intf_id[idx] == isup_intf_id) {
					/* we have a match so break out of this loop */
					status = FTDM_TRUE;
					break;
				}
			}
			if (status == FTDM_TRUE) {
				break;
			} else {
				/* move on to the next one */
				i++;
			}
		}

		if ((i == MAX_ISAPS) || (status == FTDM_FALSE)) {
			/* sice we didnot find any matching ISAP there now check is there is any isap
			 * present with matching switch type and ssf value in ISAP list */
			i = 1;
			while (i < (MAX_ISAPS)) {
				if ((cfg->isap[i].switchType == sng_isap->switchType) &&
				    (cfg->isap[i].ssf == sng_isap->ssf)) {

					/* we have a match so break out of this loop */
					break;
				}
				/* move on to the next one */
				i++;
			}
		}
	}

start_config:
	if (cfg->isap[i].id == 0) {
		sng_isap->id = i;
		SS7_DEBUG("found new isup to cc interface, id is = %d\n", sng_isap->id);
	} else {
		sng_isap->id = i;
		SS7_DEBUG("found existing isup to cc interface, id is = %d\n", sng_isap->id);
	}

	cfg->isap[i].id 			= sng_isap->id;
	cfg->isap[i].suId			= sng_isap->id;
	cfg->isap[i].spId			= sng_isap->id;
	cfg->isap[i].switchType	= sng_isap->switchType;
	cfg->isap[i].ssf			= sng_isap->ssf;
	cfg->isap[i].defRelLocation		= sng_isap->defRelLocation;

	if (sng_isap->t1 != 0) {
		cfg->isap[i].t1		= sng_isap->t1;
	} else {
		cfg->isap[i].t1		= 150;
	}
	if (sng_isap->t2 != 0) {
		cfg->isap[i].t2		= sng_isap->t2;
	} else {
		cfg->isap[i].t2		= 1800;
	}
	if (sng_isap->t5 != 0) {
		cfg->isap[i].t5		= sng_isap->t5;
	} else {
		cfg->isap[i].t5		= 3000;
	}
	if (sng_isap->t6 != 0) {
		cfg->isap[i].t6		= sng_isap->t6;
	} else {
		cfg->isap[i].t6		= 600;
	}
	if (sng_isap->t7 != 0) {
		cfg->isap[i].t7		= sng_isap->t7;
	} else {
		cfg->isap[i].t7		= 200;
	}
	if (sng_isap->t8 != 0) {
		cfg->isap[i].t8		= sng_isap->t8;
	} else {
		cfg->isap[i].t8		= 100;
	}
	if (sng_isap->t9 != 0) {
		cfg->isap[i].t9		= sng_isap->t9;
	} else {
		cfg->isap[i].t9		= 1800;
	}
	if (sng_isap->t27 != 0) {
		cfg->isap[i].t27	= sng_isap->t27;
	} else {
		cfg->isap[i].t27	= 2400;
	}
	if (sng_isap->t31 != 0) {
		cfg->isap[i].t31	= sng_isap->t31;
	} else {
		cfg->isap[i].t31	= 3650;
	}
	if (sng_isap->t33 != 0) {
		cfg->isap[i].t33	= sng_isap->t33;
	} else {
		cfg->isap[i].t33	= 120;
	}
	if (sng_isap->t34 != 0) {
		cfg->isap[i].t34	= sng_isap->t34;
	} else {
		cfg->isap[i].t34	= 40;
	}
	if (sng_isap->t36 != 0) {
		cfg->isap[i].t36	= sng_isap->t36;
	} else {
		cfg->isap[i].t36	= 120;
	}
	if (sng_isap->tccr != 0) {
		cfg->isap[i].tccr	= sng_isap->tccr;
	} else {
		cfg->isap[i].tccr	= 200;
	}
	if (sng_isap->tccrt != 0) {
		cfg->isap[i].tccrt	= sng_isap->tccrt;
	} else {
		cfg->isap[i].tccrt	= 20;
	}
	if (sng_isap->tex != 0) {
		cfg->isap[i].tex	= sng_isap->tex;
	} else {
		cfg->isap[i].tex	= 1000;
	}
	if (sng_isap->tcrm != 0) {
		cfg->isap[i].tcrm	= sng_isap->tcrm;
	} else {
		cfg->isap[i].tcrm	= 30;
	}
	if (sng_isap->tcra != 0) {
		cfg->isap[i].tcra	= sng_isap->tcra;
	} else {
		cfg->isap[i].tcra	= 100;
	}
	if (sng_isap->tect != 0) {
		cfg->isap[i].tect	= sng_isap->tect;
	} else {
		cfg->isap[i].tect	= 10;
	}
	if (sng_isap->trelrsp != 0) {
		cfg->isap[i].trelrsp	= sng_isap->trelrsp;
	} else {
		cfg->isap[i].trelrsp	= 10;
	}
	if (sng_isap->tfnlrelrsp != 0) {
		cfg->isap[i].tfnlrelrsp	= sng_isap->tfnlrelrsp;
	} else {
		cfg->isap[i].tfnlrelrsp	= 10;
	}

	for (idx = 0; idx < (MAX_ISUP_INFS + 1); idx++) {
		if (cfg->isap[i].isup_intf_id[idx]) {
			if (cfg->isap[i].isup_intf_id[idx] == isup_intf_id) {
				break;
			} else {
				continue;
			}
		}

		/* Fill in associated ISUP interface ID in case of nsap configuration for reference */
		cfg->isap[i].isup_intf_id[idx] = isup_intf_id;
		break;
	}

	/* filling up reconfiguration flag */
	cfg->isap[i].reload 		= FTDM_FALSE;

	return 0;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_ccSpan(sng_ccSpan_t *ccSpan, sng_ss7_cfg_t *cfg)
{
	sng_timeslot_t		timeslot;
	sngss7_chan_data_t	*ss7_info = NULL;
	int			x = 0;
	int			count = 1;
	int			flag;
	int			trans_idx = 0;
	int			idx = 0;
	int			span_id = 0;

	/* It is possible that for the ccSpan circuits are already being configured in such
	 * cases please start cic at that point itself */
	span_id = ftmod_ss7_get_span_id_by_cc_span_id(ccSpan->id);

	if (span_id) {
		SS7_DEBUG("CC span %d is already configure mapped to span %d\n",
				  ccSpan->id, span_id);
	}

	while (ccSpan->ch_map[0] != '\0') {
	/**************************************************************************/

		/* pull out the next timeslot */
		if (ftmod_ss7_next_timeslot(ccSpan->ch_map, &timeslot)) {
			SS7_ERROR("Failed to parse the channel map!\n");
			return FTDM_FAIL;
		}

		/* find a spot for this circuit in the global structure */
		x = 0;

		if (span_id) {
			x = ftmod_ss7_get_start_cic_value_by_span_id(span_id);
		}

		if (!x) {
			/* find a spot for this circuit in the global structure */
			x = (ccSpan->procId * 1000) + 1;
		}

		flag = 0;
		while (flag == 0) {
			/* Skip channel if it is a transparent one */
			if (timeslot.transparent) {
				for (idx = 0; idx < MAX_TRANSPARENT_CKTS; idx++) {
					if (!cfg->transCkt[idx].ccSpan_id) {
						break;
					}

					/* since we always try to create circuit id for each span thus, in case of transparent
					 * circuit if the circuit is already configured please increment count by one in order
					 * to avoid invalid circuit ID creation
					 * for e.g. if channel 14 & 15 are transparent and if circuit id for channel 16 is
					 * already created then in case if the same code is repeated for another span
					 * configuration count must increase when same channel circuit creation is repeated
					 * NOTE: Count must increase as count is consider as channel ID */
					if ((cfg->transCkt[idx].ccSpan_id == ccSpan->id) &&
					    (cfg->transCkt[idx].chan_id == timeslot.channel) &&
					    (cfg->transCkt[idx].next_cktId = (x))) {
						count++;
						break;
					}
				}
				break;
			}

			if (timeslot.sigtran_sig) {
				count++;
				break;
			}
			/**********************************************************************/
			/* check the id value ( 0 = new, 0 > circuit can be existing) */
			if (cfg->isupCkt[x].id == 0) {
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
				if ((cfg->isupCkt[x].ccSpanId == ccSpan->id) &&
					(cfg->isupCkt[x].chan == count)) {

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
			cfg->transCkt[trans_idx].ccSpan_id = ccSpan->id;
			cfg->transCkt[trans_idx].chan_id = timeslot.channel;
			cfg->transCkt[trans_idx++].next_cktId = x;
			SS7_DEBUG("Ignoring circuit as channel configured as transparent %d\n", timeslot.channel);
			continue;
		} else if (timeslot.sigtran_sig) {
			/* if it has to be consider as a gap then please donot assign this cic Id to any channel */
			if (timeslot.gap) {
				SS7_DEBUG("Ignoring circuit as channel is configured as SIGTRAN signalling %d\n", timeslot.channel);
				ccSpan->cicbase++;
			} else {
				SS7_DEBUG("Forwarding CIC Id to adjacent circuit as channel is configured as SIGTRAN signalling %d\n", timeslot.channel);
			}
			continue;
		}

		/* prepare the global info sturcture */
		ss7_info = ftdm_calloc(1, sizeof(sngss7_chan_data_t));
		ss7_info->ftdmchan = NULL;
		if (ftdm_queue_create(&ss7_info->event_queue, SNGSS7_CHAN_EVENT_QUEUE_SIZE) != FTDM_SUCCESS) {
			SS7_CRITICAL("Failed to create ss7 cic event queue\n");
		}
		ss7_info->circuit = &cfg->isupCkt[x];

		cfg->isupCkt[x].obj			= ss7_info;

		/* fill in the rest of the global structure */
		cfg->isupCkt[x].procId	  	= ccSpan->procId;
		cfg->isupCkt[x].id			= x;
		cfg->isupCkt[x].ccSpanId	= ccSpan->id;
		cfg->isupCkt[x].span		= 0;
		cfg->isupCkt[x].chan		= count;

		if (timeslot.siglink) {
			cfg->isupCkt[x].type	= SNG_CKT_SIG;
		} else if (timeslot.gap) {
			cfg->isupCkt[x].type	= SNG_CKT_HOLE;
		} else {
			cfg->isupCkt[x].type	= SNG_CKT_VOICE;

			/* throw the flag to indicate that we need to start call control */
			sngss7_set_flag(cfg, SNGSS7_CC_PRESENT);
		}

		if (timeslot.channel) {
			cfg->isupCkt[x].cic		= ccSpan->cicbase;
			ccSpan->cicbase++;
		} else {
			cfg->isupCkt[x].cic		= 0;
		}

		cfg->isupCkt[x].infId						= ccSpan->isupInf;
		cfg->isupCkt[x].typeCntrl					= ccSpan->typeCntrl;
		cfg->isupCkt[x].ssf							= ccSpan->ssf;
		cfg->isupCkt[x].cld_nadi					= ccSpan->cld_nadi;
		cfg->isupCkt[x].clg_nadi					= ccSpan->clg_nadi;
		cfg->isupCkt[x].rdnis_nadi					= ccSpan->rdnis_nadi;
		cfg->isupCkt[x].loc_nadi					= ccSpan->loc_nadi;
		cfg->isupCkt[x].options						= ccSpan->options;
		cfg->isupCkt[x].switchType					= ccSpan->switchType;
		cfg->isupCkt[x].min_digits					= ccSpan->min_digits;
		cfg->isupCkt[x].bearcap_check				= ccSpan->bearcap_check;
		cfg->isupCkt[x].itx_auto_reply				= ccSpan->itx_auto_reply;
		cfg->isupCkt[x].transparent_iam				= ccSpan->transparent_iam;
		cfg->isupCkt[x].transparent_iam_max_size	= ccSpan->transparent_iam_max_size;
		cfg->isupCkt[x].cpg_on_progress_media		= ccSpan->cpg_on_progress_media;
		cfg->isupCkt[x].cpg_on_progress				= ccSpan->cpg_on_progress;
		cfg->isupCkt[x].ignore_alert_on_cpg			= ccSpan->ignore_alert_on_cpg;
		cfg->isupCkt[x].blo_on_ckt_delete			= ccSpan->blo_on_ckt_delete;

		if (ccSpan->t3 == 0) {
			cfg->isupCkt[x].t3		= 1200;
		} else {
			cfg->isupCkt[x].t3		= ccSpan->t3;
		}
		if (ccSpan->t10 == 0) {
			cfg->isupCkt[x].t10		= 50;
		} else {
			cfg->isupCkt[x].t10		= ccSpan->t10;
		}
		if (ccSpan->t12 == 0) {
			cfg->isupCkt[x].t12		= 300;
		} else {
			cfg->isupCkt[x].t12		= ccSpan->t12;
		}
		if (ccSpan->t13 == 0) {
			cfg->isupCkt[x].t13		= 3000;
		} else {
			cfg->isupCkt[x].t13		= ccSpan->t13;
		}
		if (ccSpan->t14 == 0) {
			cfg->isupCkt[x].t14		= 300;
		} else {
			cfg->isupCkt[x].t14		= ccSpan->t14;
		}
		if (ccSpan->t15 == 0) {
			cfg->isupCkt[x].t15		= 3000;
		} else {
			cfg->isupCkt[x].t15		= ccSpan->t15;
		}
		if (ccSpan->t16 == 0) {
			cfg->isupCkt[x].t16		= 300;
		} else {
			cfg->isupCkt[x].t16		= ccSpan->t16;
		}
		if (ccSpan->t17 == 0) {
			cfg->isupCkt[x].t17		= 3000;
		} else {
			cfg->isupCkt[x].t17		= ccSpan->t17;
		}

		if (ccSpan->t35 == 0) {
			/* Q.764 2.2.5 Address incomplete (T35 is 15-20 seconds according to Table A.1/Q.764) */
			cfg->isupCkt[x].t35		= 170;
		} else {
			cfg->isupCkt[x].t35		= ccSpan->t35;
		}
		if (ccSpan->t39 == 0) {
			cfg->isupCkt[x].t39		= 120;
		} else {
			cfg->isupCkt[x].t39		= ccSpan->t39;
		}

		if (ccSpan->tval == 0) {
			cfg->isupCkt[x].tval	= 10;
		} else {
			cfg->isupCkt[x].tval	= ccSpan->tval;
		}

		/* filling up reconfiguration flag */
		cfg->isupCkt[x].reload 		= FTDM_FALSE;

		SS7_INFO("Added procId=%d, spanId = %d, chan = %d, cic = %d, ISUP cirId = %d\n",
				  cfg->isupCkt[x].procId,
				  cfg->isupCkt[x].ccSpanId,
				  cfg->isupCkt[x].chan,
				  cfg->isupCkt[x].cic,
				  cfg->isupCkt[x].id);

move_along:
		/* increment the span channel count */
		count++;

	/**************************************************************************/
	} /* while (ccSpan->ch_map[0] != '\0') */


	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_insert_cc_span_info(sng_isup_ckt_t *isupCkt, ftdm_span_t *span, sng_ss7_cfg_t *cfg)
{
	int idx = 1;

	while (idx < (MAX_ISUP_INFS)) {
		if (cfg->isupCc[idx].span_id == span->span_id) {
			break;
		}
		idx++;
	}

	if (cfg->isupCc[idx].span_id != 0) {
		SS7_DEBUG("Found existing CC span %d information at index %d\n",
				  cfg->isupCc[idx].cc_span_id, idx);
	} else {
		idx = 1;
		while (idx < (MAX_ISUP_INFS)) {
			if (cfg->isupCc[idx].span_id == 0) {
				SS7_DEBUG("Insert new CC Span %d info at index %d to info list\n", isupCkt->ccSpanId, idx);
				break;
			}
			idx++;
		}

		if (idx == (MAX_ISUP_INFS)) {
			SS7_ERROR("No more space to enter new CC span %d information!\n", isupCkt->ccSpanId);
			return FTDM_FAIL;
		}

		cfg->isupCc[idx].infId = isupCkt->infId;
		cfg->isupCc[idx].span_id = span->span_id;
		cfg->isupCc[idx].cc_span_id = isupCkt->ccSpanId;
		cfg->isupCc[idx].ckt_start_val = isupCkt->id;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************/
static int ftmod_ss7_fill_in_circuits(sng_span_t *sngSpan, ftdm_sngss7_operating_modes_e opr_mode)
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
			if (SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) {
				return FTDM_SUCCESS;
			} else {
				SS7_ERROR_CHAN(ftdmchan, "Failed to find this channel in the global ckts!%s\n","");
				return FTDM_FAIL;
			}
		}

		/* fill in the rest of the global sngss7_chan_data_t structure */
		ss7_info = (sngss7_chan_data_t *)isupCkt->obj;
		ss7_info->ftdmchan = ftdmchan;

		/* check if there is any call data already present then please clear that call data first */
		if ((ftdmchan->call_data) && (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE)) {
			SS7_INFO_CHAN(ftdmchan, "Clearing call data as already present %s!\n", "");
			ftdm_safe_free(ftdmchan->call_data);
		}
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

		if (FTDM_SUCCESS != ftmod_ss7_insert_cc_span_info(isupCkt, ftdmspan, &g_ftdm_sngss7_data.cfg)) {
			return FTDM_FAIL;
		}
	/**************************************************************************/
	} /* for (i == 1; i < ftdmspan->chan_count; i++) */

	if(SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode){
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
	case 'M':
	case 'm':
		/* we have a transparent channel */
		timeslot->sigtran_sig = 1;

		/* check what comes next either a comma or a number */
		x++;
		if ((ch_map[1] == 'g') || (ch_map[1] == 'G')) {
			timeslot->gap = 1;
			SS7_DEBUG(" Found a SIGTRAN signalling channel in the channel map with a gap in ISUP cic map\n");
			x++;
		}

		if (ch_map[x] == ',') {
			 SS7_DEBUG(" Found a SIGTRAN signalling channel in ISUP cic map\n");
		} else if (isdigit(ch_map[x])) {
			SS7_DEBUG(" Found a SIGTRAN signalling channel channel thus considering it as a hole in ISUP cic map\n");
			/* consume all digits until a comma as this is the channel */
		} else {
			SS7_ERROR("Found an illegal channel map character after transpaent channel flag = \"%c\"!\n", ch_map[x]);
			return FTDM_FAIL;
		}
		break;
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

	start_range = (offset +  1000) + 1;

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
ftdm_sngss7_operating_modes_e ftmod_ss7_get_operating_mode_by_span_id(int span_id)
{
    ftdm_sngss7_operating_modes_e opr_mode = SNG_SS7_OPR_MODE_NONE;
    int idx = 0;

    for (idx = 0; idx < FTDM_MAX_CHANNELS_SPAN; idx++) {
	    if (g_ftdm_sngss7_span_data.span_data[idx].span_id) {
		    if (g_ftdm_sngss7_span_data.span_data[idx].span_id == span_id) {
			    opr_mode = g_ftdm_sngss7_span_data.span_data[idx].opr_mode;
			    break;
		    }
	    }
    }

    if (opr_mode == SNG_SS7_OPR_MODE_NONE) {
	    SS7_ERROR("Operating mode is not set for span %d!\n",
		      span_id);
    }

    return opr_mode;
}

/******************************************************************************/
/* To get if operating mode present in current span configuration list */
int ftmod_ss7_is_operating_mode_pres(ftdm_sngss7_operating_modes_e opr_mode)
{
    int idx = 0;

    for (idx = 0; idx < FTDM_MAX_CHANNELS_SPAN; idx++) {
	    if (g_ftdm_sngss7_span_data.span_data[idx].span_id) {
		    if (opr_mode == g_ftdm_sngss7_span_data.span_data[idx].opr_mode) {
			    return 1;
		    }
	    }
    }

    return 0;
}

/******************************************************************************/
/* check is any of the stack is in pending status then please then consider
 * general configuration failed */
ftdm_status_t ftmod_ss7_check_gen_config_status()
{
    if ((g_ftdm_sngss7_data.gen_config.cm == SNG_GEN_CFG_STATUS_PENDING) ||
	(g_ftdm_sngss7_data.gen_config.mtp == SNG_GEN_CFG_STATUS_PENDING) ||
	(g_ftdm_sngss7_data.gen_config.mtp3 == SNG_GEN_CFG_STATUS_PENDING) ||
	(g_ftdm_sngss7_data.gen_config.isup == SNG_GEN_CFG_STATUS_PENDING) ||
	(g_ftdm_sngss7_data.gen_config.cc == SNG_GEN_CFG_STATUS_PENDING) ||
	(g_ftdm_sngss7_data.gen_config.m2ua == SNG_GEN_CFG_STATUS_PENDING) ||
	(g_ftdm_sngss7_data.gen_config.m3ua == SNG_GEN_CFG_STATUS_PENDING) ||
	(g_ftdm_sngss7_data.gen_config.relay == SNG_GEN_CFG_STATUS_PENDING)) {
		return FTDM_FAIL;
	}

	return FTDM_SUCCESS;
}

/******************************************************************************
		Parsing SS7 Configuration in case of RELOAD
			 i.e. Dynamic Configuration
******************************************************************************/
/******************************************************************************/
static ftdm_status_t ftmod_ss7_check_glare_resolution(const char *method)
{
	sng_glare_resolution iMethod = SNGSS7_GLARE_PC;
	ftdm_status_t 	     ret     = FTDM_FAIL;

	if (!method || (strlen (method) <=0) ) {
		SS7_ERROR( "Wrong glare resolution parameter passed \n" );
		goto done;
	} else {
		if (!strcasecmp( method, "PointCode")) {
			iMethod = SNGSS7_GLARE_PC;
		} else if (!strcasecmp( method, "Down")) {
			iMethod = SNGSS7_GLARE_DOWN;
		} else if (!strcasecmp( method, "Control")) {
			iMethod = SNGSS7_GLARE_CONTROL;
		} else {
			SS7_ERROR( "Wrong glare resolution parameter passed. Thus, using default. \n" );
			iMethod = SNGSS7_GLARE_PC;
		}
	}

	if (g_ftdm_sngss7_data.cfg.glareResolution == iMethod) {
		ret = FTDM_SUCCESS;
	} else {
		ret = FTDM_FAIL;
	}

done:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_parse_sng_gen_on_reload(ftdm_conf_node_t *sng_gen, char *sng_gen_opr_mode, const char *span_opr_mode, ftdm_span_t *span)
{
	ftdm_conf_parameter_t   *parm = sng_gen->parameters;
	int                     num_parms = sng_gen->n_parameters;
	int                     i = 0;
	ftdm_status_t			ret = FTDM_FAIL;
	ftdm_sngss7_operating_modes_e opr_mode = SNG_SS7_OPR_MODE_NONE;

	/* extract all the information from the parameters */
	for (i = 0; i < num_parms; i++) {
		if (!strcasecmp(parm->var, "procId")) {
			if (g_ftdm_sngss7_data.cfg.procId != atoi(parm->val)) {
				SS7_ERROR("[Reload] Invalid reconfiguation request to change procId!\n");
				ret = FTDM_FAIL;
				goto done;
			}
		} else if (!strcasecmp(parm->var, "license")) {
			if (strcasecmp(parm->val,g_ftdm_sngss7_data.cfg.license)) {
				SS7_ERROR("[Reload] Invalid reconfiguration request to change License file path!\n");
				ret = FTDM_FAIL;
				goto done;
			}
		} else if (!strcasecmp(parm->var, "transparent_iam_max_size")) {
			if (g_ftdm_sngss7_data.cfg.transparent_iam_max_size != atoi(parm->val)) {
				SS7_DEBUG("[Reload] Reconfiguring transparent_iam max size from %d to %d\n",
					  g_ftdm_sngss7_data.cfg.transparent_iam_max_size, atoi(parm->val));
				g_ftdm_sngss7_data.cfg.transparent_iam_max_size = atoi(parm->val);
			}
		} else if (!strcasecmp(parm->var, "glare-reso")) {
			if (FTDM_FAIL == ftmod_ss7_check_glare_resolution(parm->val)) {
				ftmod_ss7_set_glare_resolution (parm->val);
				SS7_DEBUG("[Reload] Reconfiguring Glare resolution to %s\n", parm->val);
			}
		} else if (!strcasecmp(parm->var, "force_inr")) {
			if (ftdm_true(parm->val)) {
				if (g_ftdm_sngss7_data.cfg.force_inr != 1) {
					SS7_DEBUG("[Reload] Reconfiguring Force INR to TRUE\n");
				}
			} else {
				if (g_ftdm_sngss7_data.cfg.force_inr != 0) {
					SS7_DEBUG("[Reload] Reconfiguring Force INR to FALSE\n");
				}
			}
		} else if (!strcasecmp(parm->var, "operating_mode")) {
			strcpy(sng_gen_opr_mode, parm->val);
			if (FTDM_FAIL == ftmod_ss7_set_operating_mode(span_opr_mode, sng_gen_opr_mode, span)) {
				SS7_DEBUG("[Reload] Reconfiguring Operating mode is not allowed!\n");
				ret = FTDM_FAIL;
				goto done;
			}
		} else if (!strcasecmp(parm->var, "stack-logging-enable")) {
			if (ftdm_true(parm->val)) {
				if (g_ftdm_sngss7_data.stack_logging_enable != 1) {
					SS7_DEBUG("[Reload] Reconfiguring stack configuration to TRUE\n");
				}
			} else {
				if (g_ftdm_sngss7_data.stack_logging_enable != 0) {
					SS7_DEBUG("[Reload] Reconfiguring stack configuration to FALSE\n");
				}
			}
		} else if (!strcasecmp(parm->var, "max-cpu-usage")) {
			if (g_ftdm_sngss7_data.cfg.max_cpu_usage != atoi(parm->val)) {
				SS7_DEBUG("[Reload] Reconfiguring maximum cpu usage limit from %d to %d\n", g_ftdm_sngss7_data.cfg.max_cpu_usage);
				g_ftdm_sngss7_data.cfg.max_cpu_usage = atoi(parm->val);
			}
		} else if (!strcasecmp(parm->var, "auto-congestion-control")) {
			if (ftdm_true(parm->val)) {
				if (g_ftdm_sngss7_data.cfg.sng_acc != 1) {
					SS7_ERROR("[Reload] Reconfiguration of Automatic Congestion Control is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else {
				if (g_ftdm_sngss7_data.cfg.sng_acc != 0) {
					SS7_ERROR("[Reload] Reconfiguration of Automatic Congestion Control is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			}
		} else if (!strcasecmp(parm->var, "traffic-reduction-rate")) {
			if (g_ftdm_sngss7_data.cfg.sng_acc) {
				if (g_ftdm_sngss7_data.cfg.accCfg.trf_red_rate != atoi(parm->val)) {
					SS7_ERROR("[Reload] Reconfiguration of ACC parameter (Traffic Reduction Rate) is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else {
				SS7_DEBUG("Found invalid configurable parameter Traffic reduction rate as Automatic congestion feature is not enable\n");
			}
		} else if (!strcasecmp(parm->var, "traffic-increment-rate")) {
			if (g_ftdm_sngss7_data.cfg.sng_acc) {
				if (g_ftdm_sngss7_data.cfg.accCfg.trf_inc_rate != atoi(parm->val)) {
					SS7_ERROR("[Reload] Reconfiguration of ACC parameter (Traffic Increment Rate) is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else {
				SS7_DEBUG("Found invalid configurable parameter Traffic increment rate as Automatic congestion feature is not enable\n");
			}
		} else if (!strcasecmp(parm->var, "first-level-reduction-rate")) {
			if (g_ftdm_sngss7_data.cfg.sng_acc) {
				if (g_ftdm_sngss7_data.cfg.accCfg.cnglvl1_red_rate != atoi(parm->val)) {
					SS7_ERROR("[Reload] Reconfiguration of ACC parameter (Level 1 Reduction Rate) is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else {
				SS7_DEBUG("[Reload] Found invalid configurable parameter Congestion Level 1 Traffic Reduction Rate as Automatic congestion feature is not enable\n");
			}
		} else if (!strcasecmp(parm->var, "second-level-reduction-rate")) {
			if (g_ftdm_sngss7_data.cfg.sng_acc) {
				if (g_ftdm_sngss7_data.cfg.accCfg.cnglvl2_red_rate != atoi(parm->val)) {
					SS7_ERROR("[Reload] Reconfiguration of ACC parameter (Level 2 Reduction Rate) is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else {
				SS7_DEBUG("[Reload] Found invalid configurable parameter Congestion Level 2 Traffic Reduction Rate as Automatic congestion feature is not enable\n");
			}
		} else if (!strcasecmp(parm->var, "set-isup-message-priority")) {
			if (!strcasecmp(parm->val, "default")) {
				if (g_ftdm_sngss7_data.cfg.set_msg_priority != 7) {
					SS7_ERROR("[Reload] Reconfiguration of ISUP message priority is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else if (!strcasecmp(parm->val, "all-zeros")) {
				if (g_ftdm_sngss7_data.cfg.set_msg_priority != 0) {
					SS7_ERROR("[Reload] Reconfiguration of ISUP message priority is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else if (!strcasecmp(parm->val, "all-ones")) {
				if (g_ftdm_sngss7_data.cfg.set_msg_priority != 87381) {
					SS7_ERROR("[Reload] Reconfiguration of ISUP message priority is not supported yet!\n");
					ret = FTDM_FAIL;
					goto done;
				}
			} else {
				SS7_ERROR("[Reload] Invalid ISUP message prioriy = %s\n", parm->val);
				ret = FTDM_FAIL;
				goto done;
			}
		} else if (!strcasecmp(parm->var, "link_failure_action")) {
			if (g_ftdm_sngss7_data.cfg.link_failure_action != atoi(parm->val)) {
				SS7_DEBUG("[Reload]: Reconfiguring Link Failure Action from %d to %d\n",
					  g_ftdm_sngss7_data.cfg.link_failure_action, atoi(parm->val));
				g_ftdm_sngss7_data.cfg.link_failure_action = atoi(parm->val);
			}
			if ((g_ftdm_sngss7_data.cfg.link_failure_action == SNGSS7_ACTION_RELEASE_CALLS) ||
			    (g_ftdm_sngss7_data.cfg.link_failure_action == SNGSS7_ACTION_KEEP_CALLS)) {
			} else {
				SS7_DEBUG("[Reload] Found invalid link_failure_action  = %d\n", g_ftdm_sngss7_data.cfg.link_failure_action);
				ret = FTDM_FAIL;
				goto done;
			}
		} else {
			SS7_ERROR("[Reload] Found an invalid parameter \"%s\" for reconfiguration!\n", parm->var);
			return FTDM_FAIL;
		}

		/* move to the next parmeter */
		parm = parm + 1;
	} /* for (i = 0; i < num_parms; i++) */

	opr_mode = ftmod_ss7_get_operating_mode_by_span_id(span->span_id);
	if (opr_mode == SNG_SS7_OPR_MODE_NONE) {
		opr_mode = ftmod_ss7_get_operatig_mode_cfg(span_opr_mode, sng_gen_opr_mode);
	}

	if (SNG_SS7_OPR_MODE_ISUP != opr_mode) {
		SS7_ERROR("[Reload] Reconfiguration is not supported for operating Mode %s!\n", ftdm_opr_mode_tostr(opr_mode));
		ret = FTDM_FAIL;
	} else {
		ret = FTDM_SUCCESS;
	}

done:
	return ret;
}

static ftdm_status_t ftmod_ss7_copy_reconfig_changes()
{
	ftdm_status_t ret 	= FTDM_FAIL;

	/* check if newly parsed mtp 1 configuration is valid or not */
	if ((ret = ftmod_ss7_copy_mtp1_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed mtp 2 configuration is valid or not */
	if ((ret = ftmod_ss7_copy_mtp2_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed mtp 3 configuration is valid or not */
	if ((ret = ftmod_ss7_copy_mtp3_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed linkset configuration is valid or not */
	if ((ret = ftmod_ss7_copy_linkset_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed route configuration is valid or not */
	if ((ret = ftmod_ss7_copy_route_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed self route configuration is valid or not */
	if ((ret = ftmod_ss7_copy_self_route_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed nsap configuration is valid or not */
	if ((ret = ftmod_ss7_copy_nsap_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed isap configuration is valid or not */
	if ((ret = ftmod_ss7_copy_isap_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed isup interface configuration is valid or not */
	if ((ret = ftmod_ss7_copy_isup_interface_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed cc interface configuration is valid or not */
	if ((ret = ftmod_ss7_copy_cc_span_reconfig_changes()) != FTDM_SUCCESS) {
		goto end;
	}

	ret = FTDM_SUCCESS;

end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_mtp1_reconfig_changes()
{
	sng_mtp1_link_t *mtp1Link = NULL;
	ftdm_status_t ret         = FTDM_FAIL;
	int idx                   = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp1Link[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].id != 0)) {

			mtp1Link = &g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx];

			if (mtp1Link->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP1-%d configuration found\n", mtp1Link->id);
				} else if (mtp1Link->id != g_ftdm_sngss7_data.cfg.mtp1Link[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP1-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtp1Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (mtp1Link->reload == FTDM_FALSE) {
				goto reset;
			}

			/* copy over the values */
			strcpy((char *)g_ftdm_sngss7_data.cfg.mtp1Link[idx].name, (char *)mtp1Link->name);

			g_ftdm_sngss7_data.cfg.mtp1Link[idx].id 	= mtp1Link->id;
			g_ftdm_sngss7_data.cfg.mtp1Link[idx].span 	= mtp1Link->span;
			g_ftdm_sngss7_data.cfg.mtp1Link[idx].chan 	= mtp1Link->chan;
			g_ftdm_sngss7_data.cfg.mtp1Link[idx].ftdmchan 	= mtp1Link->ftdmchan;
			strcpy(g_ftdm_sngss7_data.cfg.mtp1Link[idx].span_name, mtp1Link->span_name);

			g_ftdm_sngss7_data.cfg.mtp1Link[idx].reload 	= mtp1Link->reload;

			if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add MTP1 Link %d\n",
					 g_ftdm_sngss7_data.cfg.mtp1Link[idx].id);
			}
reset:
			/* resetting MTP1 reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx], 0, sizeof(sng_mtp1_link_t));
			mtp1Link = NULL;
		}
		idx++;

	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_mtp2_reconfig_changes()
{
	ftdm_status_t 	ret 	    = FTDM_FAIL;
	sng_mtp2_link_t *mtp2Link   = NULL;
	int 		mtp3Idx     = 0;
	int 		linkset_idx = 0;
	int 		idx 	    = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp2Link[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx].id != 0)) {

			mtp2Link = &g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx];

			if (mtp2Link->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtp2Link[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP2-%d configuration found\n", mtp2Link->id);
				} else if (mtp2Link->id != g_ftdm_sngss7_data.cfg.mtp2Link[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP2-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtp2Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (mtp2Link->reload == FTDM_FALSE) {
				goto reset;
			}

			if (mtp2Link->disable_sap == FTDM_TRUE) {
				/* get its respective mtp3Idx */
				mtp3Idx = ftmod_ss7_get_mtp3_id_by_mtp2_id(idx);

				if (!mtp3Idx) {
					SS7_ERROR("[Reload]: No corresponding mtp3 id found for respective MTP2-%d\n",
						  g_ftdm_sngss7_data.cfg.mtp2Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}

				/* get linkset index mapped to this mtp3 link */
				linkset_idx = ftmod_ss7_get_linkset_id_by_mtp3_id(idx);
				if (!linkset_idx) {
					SS7_ERROR("No Linkset found mapped to MTP3-%d for MTP2-%d!\n",
						  g_ftdm_sngss7_data.cfg.mtp3Link[idx].id,
						  g_ftdm_sngss7_data.cfg.mtp2Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}

				if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx], SNGSS7_ACTIVE)) {

					/* Unbind DLSAP */
					ret = ftmod_ss7_unbind_mtp3link(mtp3Idx);
					SS7_DEBUG("[Reload]: Unbind mtp3-%d DLSAP and deactivate MTP-%d linkset for reconfiguration request.. Ret[%d].\n",
						  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3Idx].id,
						  g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].id,
						  ret);

					/* Clearing Linkset ACTIVE flag */
					sngss7_clear_flag(&g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx], SNGSS7_ACTIVE);
					g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].deactivate_linkset = mtp2Link->disable_sap;
				}

			}

			/* copy over the values */
			strcpy((char *)g_ftdm_sngss7_data.cfg.mtp2Link[idx].name, (char *)mtp2Link->name);

			g_ftdm_sngss7_data.cfg.mtp2Link[idx].id           = mtp2Link->id;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].lssuLength   = mtp2Link->lssuLength;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].errorType    = mtp2Link->errorType;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].linkType     = mtp2Link->linkType;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].sapType      = mtp2Link->sapType;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].mtp1Id       = mtp2Link->mtp1Id;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].mtp1ProcId   = mtp2Link->mtp1ProcId;

			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t1           = mtp2Link->t1;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t2           = mtp2Link->t2;

			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t3           = mtp2Link->t3;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t4n          = mtp2Link->t4n;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t4e          = mtp2Link->t4e;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t5           = mtp2Link->t5;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t6           = mtp2Link->t6;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t7           = mtp2Link->t7;
#ifdef SD_HSL
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].t8       	  = mtp2Link->t8;

			/* Errored interval parameters from Q.703 A.10.2.5 */
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].sdTe     	  = mtp2Link->sdTe;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].sdUe     	  = mtp2Link->sdUe;
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].sdDe     	  = mtp2Link->sdDe;
#endif
			g_ftdm_sngss7_data.cfg.mtp2Link[idx].reload 	  = mtp2Link->reload;

			if (g_ftdm_sngss7_data.cfg.mtp2Link[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add MTP2 Link %d\n",
					 g_ftdm_sngss7_data.cfg.mtp2Link[idx].id);
			}
reset:
			/* resetting MTP2 reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx], 0, sizeof(sng_mtp2_link_t));
			mtp2Link = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_mtp3_reconfig_changes()
{
	ftdm_status_t 	ret 		= FTDM_FAIL;
	sng_mtp3_link_t *mtp3Link 	= NULL;
	int 		nsap_cfg_idx 	= 0;
	int 		linkset_idx 	= 0;
	int 		idx 		= 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0) &&
		    (g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx].id != 0)) {

			mtp3Link = &g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx];

			if (mtp3Link->id != 0) {
				/* check if this is a new MTP3 link configured */
				if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP2-%d configuration found\n", mtp3Link->id);
				} else if (mtp3Link->id != g_ftdm_sngss7_data.cfg.mtp3Link[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP3-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (mtp3Link->reload == FTDM_FALSE) {
				goto reset;
			}

			if (mtp3Link->disable_sap == FTDM_TRUE) {
				/* get linkset index mapped to this mtp3 link */
				linkset_idx = ftmod_ss7_get_linkset_id_by_mtp3_id(idx);
				if (!linkset_idx) {
					SS7_ERROR("No Linkset found mapped to MTP3-%d!\n",
						  g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}

				/* get nsap index mapped to this mtp3 link */
				nsap_cfg_idx = ftmod_ss7_get_nsap_id_by_mtp3_id(idx);

				if (!nsap_cfg_idx) {
					SS7_ERROR("No NSAP found mapped to MTP3-%d!\n",
						  g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}

				if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx], SNGSS7_ACTIVE)) {
					/* disable and unbind DLSAP */
					ret = ftmod_ss7_unbind_mtp3link(idx);
					SS7_DEBUG("[Reload]: Unbind mtp3-%d DLSAP and deactivating MTP-%d linkset for reconfiguration request.. Ret[%d].\n",
						  g_ftdm_sngss7_data.cfg.mtp3Link[idx].id,
						  g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].id,
						  ret);

					/* Clearing Linkset ACTIVE flag */
					sngss7_clear_flag(&g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx], SNGSS7_ACTIVE);
					g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].deactivate_linkset = mtp3Link->disable_sap;
				}

				if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg.nsap[nsap_cfg_idx], SNGSS7_ACTIVE)) {
					/* disable and unbind NSAP mapped to this mtp3link */
					ret = ftmod_ss7_disable_nsap(g_ftdm_sngss7_data.cfg.nsap[nsap_cfg_idx].id);
					SS7_DEBUG("[Reload]: Unbind and Disablle NSAP-%d.. Ret[%d]\n",
						  g_ftdm_sngss7_data.cfg.nsap[nsap_cfg_idx].id, ret);

					/* Clearing NSAP ACTIVE flag */
					sngss7_clear_flag(&g_ftdm_sngss7_data.cfg.nsap[nsap_cfg_idx], SNGSS7_ACTIVE);

					g_ftdm_sngss7_data.cfg.nsap[nsap_cfg_idx].disable_sap = mtp3Link->disable_sap;
				}
			}

			/* copy over the values */
			strcpy((char *)g_ftdm_sngss7_data.cfg.mtp3Link[idx].name, (char *)mtp3Link->name);

			g_ftdm_sngss7_data.cfg.mtp3Link[idx].id           = mtp3Link->id;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].priority     = mtp3Link->priority;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkType     = mtp3Link->linkType;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].switchType   = mtp3Link->switchType;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].apc          = mtp3Link->apc;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].spc          = mtp3Link->spc;        /* unknown at this time */
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].ssf          = mtp3Link->ssf;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].slc          = mtp3Link->slc;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId    = mtp3Link->linkSetId;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].mtp2Id       = mtp3Link->mtp2Id;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].mtp2ProcId   = mtp3Link->mtp2ProcId;

			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t1   	  = mtp3Link->t1;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t2   	  = mtp3Link->t2;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t3   	  = mtp3Link->t3;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t4   	  = mtp3Link->t4;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t5   	  = mtp3Link->t5;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t7   	  = mtp3Link->t7;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t12  	  = mtp3Link->t12;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t13  	  = mtp3Link->t13;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t14  	  = mtp3Link->t14;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t17  	  = mtp3Link->t17;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t22  	  = mtp3Link->t22;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t23  	  = mtp3Link->t23;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t24  	  = mtp3Link->t24;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t31  	  = mtp3Link->t31;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t32  	  = mtp3Link->t32;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t33  	  = mtp3Link->t33;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].t34  	  = mtp3Link->t34;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].tbnd 	  = mtp3Link->tbnd;
			g_ftdm_sngss7_data.cfg.mtp3Link[idx].tflc 	  = mtp3Link->tflc;

			g_ftdm_sngss7_data.cfg.mtp3Link[idx].reload 	  = mtp3Link->reload;

			if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add MTP3 Link %d\n",
					 g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
			}
reset:
			/* resetting MTP3 reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx], 0, sizeof(sng_mtp3_link_t));
			mtp3Link = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_linkset_reconfig_changes()
{
	sng_link_set_t *mtpLinkSet = NULL;
	ftdm_status_t ret 	   = FTDM_FAIL;
	int idx 		   = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKSETS+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx].id != 0)) {

			mtpLinkSet = &g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx];

			if (mtpLinkSet->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP Linkset-%d configuration found\n", mtpLinkSet->id);
				} else if (mtpLinkSet->id != g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP Linkset-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (mtpLinkSet->reload == FTDM_FALSE) {
				idx++;
				continue;
			}

			strncpy((char *)g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].name, (char *)mtpLinkSet->name, MAX_NAME_LEN-1);

			g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id         = mtpLinkSet->id;
			g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].apc        = mtpLinkSet->apc;

			/* these values are filled in as we find routes and start allocating cmbLinkSetIds */
			g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].minActive  = mtpLinkSet->minActive;
			g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].numLinks   = mtpLinkSet->numLinks;
			g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].reload 	  = mtpLinkSet->reload;

			if (g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add MTP Linkset %d\n",
					 g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id);
			}
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_route_reconfig_changes()
{
	sng_link_set_list_t *lnkSet 	= NULL;
	sng_route_t 	    *mtp3_route = NULL;
	ftdm_status_t 	    ret 	= FTDM_FAIL;
	int 		    idx 	= 1;
	int 		    tmp 	= 0;

	/* check if mtp self routes configuration is valid or not */
	while (idx < (MAX_MTP_ROUTES+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0)) {

			mtp3_route = &g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx];

			if (mtp3_route->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtpRoute[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP Route-%d configuration found\n", mtp3_route->id);
				} else if (mtp3_route->id != g_ftdm_sngss7_data.cfg.mtpRoute[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP Route-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtpRoute[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (mtp3_route->reload == FTDM_FALSE) {
				goto reset;
			}

			/* fill in the cmbLinkSet in the linkset structure */
			lnkSet = &mtp3_route->lnkSets;

			tmp = g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks;
			g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].links[tmp] = mtp3_route->cmbLinkSetId;
			g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks = g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].numLinks;

			while (lnkSet->next != NULL) {
				tmp = g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks;
				g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].links[tmp] = mtp3_route->cmbLinkSetId;
				g_ftdm_sngss7_data.cfg.mtpLinkSet[lnkSet->lsId].numLinks = g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId].numLinks;

				lnkSet = lnkSet->next;
			}

			for (lnkSet = &mtp3_route->lnkSets; lnkSet != NULL; lnkSet = lnkSet->next) {
				/* resetting MTP Linkset reload configuration */
				memset(&g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[lnkSet->lsId], 0, sizeof(sng_link_set_t));
			}

			strcpy((char *)g_ftdm_sngss7_data.cfg.mtpRoute[idx].name, (char *)mtp3_route->name);

			g_ftdm_sngss7_data.cfg.mtpRoute[idx].id 		= mtp3_route->id;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].dpc 		= mtp3_route->dpc;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].linkType 		= mtp3_route->linkType;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType 	= mtp3_route->switchType;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].cmbLinkSetId 	= mtp3_route->cmbLinkSetId;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].isSTP 		= mtp3_route->isSTP;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].nwId 		= mtp3_route->nwId;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].lnkSets 		= mtp3_route->lnkSets;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf 		= mtp3_route->ssf;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].dir 		= SNG_RTE_DN;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t6 		= mtp3_route->t6;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t8 		= mtp3_route->t8;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t10 		= mtp3_route->t10;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t11 		= mtp3_route->t11;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t15 		= mtp3_route->t15;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t16 		= mtp3_route->t16;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t18 		= mtp3_route->t18;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t19 		= mtp3_route->t19;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t21 		= mtp3_route->t21;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t25 		= mtp3_route->t25;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t26 		= mtp3_route->t26;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].reload 		= mtp3_route->reload;

			if (g_ftdm_sngss7_data.cfg.mtpRoute[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add MTP Route %d\n",
					 g_ftdm_sngss7_data.cfg.mtpRoute[idx].id);
			}
reset:
			/* resetting MTP Linkset reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx], 0, sizeof(sng_route_t));
			mtp3_route = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_self_route_reconfig_changes()
{
	sng_route_t *mtp3_route 	= NULL;
	ftdm_status_t ret 		= FTDM_FAIL;
	int idx 			= MAX_MTP_ROUTES + 1;

	/* check if mtp routes configuration is valid or not */
	while (idx < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0)) {

			mtp3_route = &g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx];

			if (mtp3_route->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtpRoute[idx].id == 0) {
					SS7_DEBUG("[Reload]: New Self MTP Route-%d configuration found\n", mtp3_route->id);
				} else if (mtp3_route->id != g_ftdm_sngss7_data.cfg.mtpRoute[idx].id) {
					SS7_ERROR("[Reload]: Invalid Self MTP Route-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtpRoute[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (mtp3_route->reload == FTDM_FALSE) {
				goto reset;
			}

			strncpy((char *)g_ftdm_sngss7_data.cfg.mtpRoute[idx].name, "self-route", MAX_NAME_LEN-1);

			g_ftdm_sngss7_data.cfg.mtpRoute[idx].id                 = mtp3_route->id;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].dpc                = mtp3_route->dpc;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].linkType           = mtp3_route->linkType;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType         = mtp3_route->switchType;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].cmbLinkSetId       = mtp3_route->cmbLinkSetId;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].isSTP              = 0;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf                = mtp3_route->ssf;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].dir                = SNG_RTE_UP;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t6                 = 8;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t8                 = 12;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t10                = 300;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t11                = 300;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t15                = 30;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t16                = 20;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t18                = 200;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t19                = 690;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t21                = 650;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].t25                = 100;
			g_ftdm_sngss7_data.cfg.mtpRoute[idx].reload 		= mtp3_route->reload;

			if (g_ftdm_sngss7_data.cfg.mtpRoute[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add Self MTP Route %d\n",
					 g_ftdm_sngss7_data.cfg.mtpRoute[idx].id);
			}
reset:
			/* resetting MTP Linkset reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx], 0, sizeof(sng_route_t));
			mtp3_route = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_nsap_reconfig_changes()
{
	ftdm_status_t ret 	= FTDM_FAIL;
	sng_nsap_t *nsap 	= NULL;
	int idx 		= 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_NSAPS)) {
		if ((g_ftdm_sngss7_data.cfg.nsap[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.nsap[idx].id != 0)) {

			nsap = &g_ftdm_sngss7_data.cfg_reload.nsap[idx];

			if (nsap->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.nsap[idx].id == 0) {
					SS7_DEBUG("[Reload]: New NSAP-%d configuration found\n", nsap->id);
				} else if (nsap->id != g_ftdm_sngss7_data.cfg.nsap[idx].id) {
					SS7_ERROR("[Reload]: Invalid NSAP-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.nsap[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (nsap->reload == FTDM_FALSE) {
				goto reset;
			}

			if (nsap->disable_sap == FTDM_TRUE) {
				if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg.nsap[idx], SNGSS7_ACTIVE)) {
					/* Unbind and disable NSAP */
					ret = ftmod_ss7_disable_nsap(nsap->id);
					SS7_DEBUG("[Reload]: Unbind and Disablle NSAP-%d.. Ret[%d]\n",
						  g_ftdm_sngss7_data.cfg.nsap[idx].id, ret);

					/* clearing NSAP Flag */
					sngss7_clear_flag(&g_ftdm_sngss7_data.cfg.nsap[idx], SNGSS7_ACTIVE);
					g_ftdm_sngss7_data.cfg.nsap[idx].disable_sap = nsap->disable_sap;
				}
			}

			g_ftdm_sngss7_data.cfg.nsap[idx].id 		= nsap->id;
			g_ftdm_sngss7_data.cfg.nsap[idx].spId 		= nsap->spId;
			g_ftdm_sngss7_data.cfg.nsap[idx].suId 		= nsap->suId;
			g_ftdm_sngss7_data.cfg.nsap[idx].nwId 		= nsap->nwId;
			g_ftdm_sngss7_data.cfg.nsap[idx].linkType 	= nsap->linkType;
			g_ftdm_sngss7_data.cfg.nsap[idx].switchType 	= nsap->switchType;
			g_ftdm_sngss7_data.cfg.nsap[idx].ssf 		= nsap->ssf;
			g_ftdm_sngss7_data.cfg.nsap[idx].reload         = nsap->reload;

			if (g_ftdm_sngss7_data.cfg.nsap[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add NSAP %d\n",
					 g_ftdm_sngss7_data.cfg.nsap[idx].id);
			}
reset:
			/* resetting NSAP reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.nsap[idx], 0, sizeof(sng_nsap_t));
			nsap = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_isap_reconfig_changes()
{
	sng_isap_t *isap 	= NULL;
	ftdm_status_t ret 	= FTDM_FAIL;
	int idx 		= 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_NSAPS)) {
		if ((g_ftdm_sngss7_data.cfg.isap[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.isap[idx].id != 0)) {

			isap = &g_ftdm_sngss7_data.cfg_reload.isap[idx];

			if (isap->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.isap[idx].id == 0) {
					SS7_DEBUG("[Reload]: New NSAP-%d configuration found\n", isap->id);
				} else if (isap->id != g_ftdm_sngss7_data.cfg.isap[idx].id) {
					SS7_ERROR("[Reload]: Invalid NSAP-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.isap[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (isap->reload == FTDM_FALSE) {
				goto reset;
			}

			if (isap->disable_sap == FTDM_TRUE) {
				if (sngss7_test_flag(&g_ftdm_sngss7_data.cfg.isap[idx], SNGSS7_ACTIVE)) {
					/* unbind and disable ISAP */
					ret = ftmod_ss7_disable_isap(isap->id);
					SS7_DEBUG("[Reload]: Unbind and Disable ISAP-%d.. Ret[%d]\n",
						  g_ftdm_sngss7_data.cfg.isap[idx].id, ret);

					/* Clearing ISAP Active flag */
					sngss7_clear_flag(&g_ftdm_sngss7_data.cfg.isap[idx], SNGSS7_ACTIVE);
					g_ftdm_sngss7_data.cfg.isap[idx].disable_sap = isap->disable_sap;
				}
			}

			g_ftdm_sngss7_data.cfg.isap[idx].id                       = isap->id;
			g_ftdm_sngss7_data.cfg.isap[idx].suId                     = isap->id;
			g_ftdm_sngss7_data.cfg.isap[idx].spId                     = isap->id;
			g_ftdm_sngss7_data.cfg.isap[idx].switchType               = isap->switchType;
			g_ftdm_sngss7_data.cfg.isap[idx].ssf                      = isap->ssf;
			g_ftdm_sngss7_data.cfg.isap[idx].defRelLocation           = isap->defRelLocation;

			g_ftdm_sngss7_data.cfg.isap[idx].t1 			  = isap->t1;
			g_ftdm_sngss7_data.cfg.isap[idx].t2               	  = isap->t2;
			g_ftdm_sngss7_data.cfg.isap[idx].t5               	  = isap->t5;
			g_ftdm_sngss7_data.cfg.isap[idx].t6               	  = isap->t6;
			g_ftdm_sngss7_data.cfg.isap[idx].t7            		  = isap->t7;
			g_ftdm_sngss7_data.cfg.isap[idx].t8             	  = isap->t8;
			g_ftdm_sngss7_data.cfg.isap[idx].t9             	  = isap->t9;
			g_ftdm_sngss7_data.cfg.isap[idx].t27            	  = isap->t27;
			g_ftdm_sngss7_data.cfg.isap[idx].t31            	  = isap->t31;
			g_ftdm_sngss7_data.cfg.isap[idx].t33            	  = isap->t33;
			g_ftdm_sngss7_data.cfg.isap[idx].t34            	  = isap->t34;
			g_ftdm_sngss7_data.cfg.isap[idx].t36            	  = isap->t36;
			g_ftdm_sngss7_data.cfg.isap[idx].tccr           	  = isap->tccr;
			g_ftdm_sngss7_data.cfg.isap[idx].tccrt          	  = isap->tccrt;
			g_ftdm_sngss7_data.cfg.isap[idx].tex            	  = isap->tex;
			g_ftdm_sngss7_data.cfg.isap[idx].tcrm           	  = isap->tcrm;
			g_ftdm_sngss7_data.cfg.isap[idx].tcra           	  = isap->tcra;
			g_ftdm_sngss7_data.cfg.isap[idx].tect           	  = isap->tect;
			g_ftdm_sngss7_data.cfg.isap[idx].trelrsp        	  = isap->trelrsp;
			g_ftdm_sngss7_data.cfg.isap[idx].tfnlrelrsp     	  = isap->tfnlrelrsp;
			g_ftdm_sngss7_data.cfg.isap[idx].reload 		  = isap->reload;

			if (g_ftdm_sngss7_data.cfg.isap[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add ISAP %d\n",
					 g_ftdm_sngss7_data.cfg.isap[idx].id);
			}
reset:
			/* resetting ISAP reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.isap[idx], 0, sizeof(sng_isap_t));
			isap = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_isup_interface_reconfig_changes()
{
	sng_isup_inf_t *isupIntf = NULL;
	ftdm_status_t ret 	 = FTDM_FAIL;
	int idx 		 = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_ISUP_INFS)) {
		if ((g_ftdm_sngss7_data.cfg.isupIntf[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.isupIntf[idx].id != 0)) {

			isupIntf = &g_ftdm_sngss7_data.cfg_reload.isupIntf[idx];

			if (isupIntf->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.isupIntf[idx].id == 0) {
					SS7_DEBUG("[Reload]: New ISUP interface-%d configuration found\n", isupIntf->id);
				} else if (isupIntf->id != g_ftdm_sngss7_data.cfg.isupIntf[idx].id) {
					SS7_ERROR("[Reload]: Invalid ISUP Interface-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.isupIntf[idx].id);
					ret = FTDM_FAIL;
					goto end;
				}
			}

			/* check if there is any change then only continue for applying new configuration */
			if (isupIntf->reload == FTDM_FALSE) {
				goto reset;
			}

			strncpy((char *)g_ftdm_sngss7_data.cfg.isupIntf[idx].name, (char *)isupIntf->name, MAX_NAME_LEN-1);

			g_ftdm_sngss7_data.cfg.isupIntf[idx].id           = isupIntf->id;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].mtpRouteId   = isupIntf->mtpRouteId;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].nwId         = isupIntf->nwId;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].dpc          = isupIntf->dpc;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].spc          = isupIntf->spc;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].switchType   = isupIntf->switchType;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].ssf          = isupIntf->ssf;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].isap         = isupIntf->isap;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].options      = isupIntf->options;

			g_ftdm_sngss7_data.cfg.isupIntf[idx].pauseAction  = isupIntf->pauseAction;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t4           = isupIntf->t4;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t11          = isupIntf->t11;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t18          = isupIntf->t18;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t19          = isupIntf->t19;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t20          = isupIntf->t20;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t21          = isupIntf->t21;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t22          = isupIntf->t22;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t23          = isupIntf->t23;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t24          = isupIntf->t24;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t25          = isupIntf->t25;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t26          = isupIntf->t26;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t28          = isupIntf->t28;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t29          = isupIntf->t29;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t30          = isupIntf->t30;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t32          = isupIntf->t32;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t37          = isupIntf->t37;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t38          = isupIntf->t38;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].t39          = isupIntf->t39;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].tfgr         = isupIntf->tfgr;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].tpause       = isupIntf->tpause;
			g_ftdm_sngss7_data.cfg.isupIntf[idx].reload 	  = isupIntf->reload;

			if (g_ftdm_sngss7_data.cfg.isupIntf[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Add ISUP interface %d\n",
						g_ftdm_sngss7_data.cfg.isupIntf[idx].id);
			}
reset:
			/* resetting ISUP Interface reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.isupIntf[idx], 0, sizeof(sng_isup_inf_t));
			isupIntf = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/******************************************************************************/
static ftdm_status_t ftmod_ss7_copy_cc_span_reconfig_changes()
{
	sng_isup_ckt_t *isupCkt 	= NULL;
	ftdm_bool_t reload 		= FTDM_FALSE;
	ftdm_status_t ret 		= FTDM_FAIL;
	int idx 			= 1;

	/* check if cc span i.e. interface configurtion is valid or not */
	while (idx < (MAX_ISUP_INFS)) {
		if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.isupCkt[idx].id != 0)) {

			reload = FTDM_FALSE;
			isupCkt = &g_ftdm_sngss7_data.cfg_reload.isupCkt[idx];

			if (g_ftdm_sngss7_data.cfg_reload.transCkt[idx - 1].ccSpan_id) {
				g_ftdm_sngss7_data.cfg.transCkt[idx - 1].ccSpan_id  = g_ftdm_sngss7_data.cfg_reload.transCkt[idx - 1].ccSpan_id;
				g_ftdm_sngss7_data.cfg.transCkt[idx - 1].chan_id    = g_ftdm_sngss7_data.cfg_reload.transCkt[idx - 1].chan_id;
				g_ftdm_sngss7_data.cfg.transCkt[idx - 1].next_cktId = g_ftdm_sngss7_data.cfg_reload.transCkt[idx - 1].next_cktId;

				SS7_DEBUG("[Reload]: Ignoring circuit as channel configured as transparent %d\n", g_ftdm_sngss7_data.cfg_reload.transCkt[idx - 1].chan_id);
				continue;
			}

			/* check if there is any change then only continue for applying new configuration */
			if (isupCkt->reload == FTDM_FALSE) {
				goto reset;
			}

			if (isupCkt->obj) {
				g_ftdm_sngss7_data.cfg.isupCkt[idx].obj = isupCkt->obj;
			}

			/* fill in the rest of the global structure */
			g_ftdm_sngss7_data.cfg.isupCkt[idx].procId = isupCkt->procId;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].id = isupCkt->id;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId = isupCkt->ccSpanId;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].chan = isupCkt->chan;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].type = isupCkt->type;

			if (isupCkt->type == SNG_CKT_VOICE) {
				/* throw the flag to indicate that we need to start call control */
				sngss7_set_flag(&g_ftdm_sngss7_data.cfg, SNGSS7_CC_PRESENT);
			}

			g_ftdm_sngss7_data.cfg.isupCkt[idx].cic 			= isupCkt->cic;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].typeCntrl 			= isupCkt->typeCntrl;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].infId 			= isupCkt->infId;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].ssf 			= isupCkt->ssf;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].cld_nadi 			= isupCkt->cld_nadi;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].clg_nadi 			= isupCkt->clg_nadi;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].rdnis_nadi 			= isupCkt->rdnis_nadi;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].loc_nadi 			= isupCkt->loc_nadi;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].options 			= isupCkt->options;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].switchType 			= isupCkt->switchType;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].min_digits 			= isupCkt->min_digits;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].bearcap_check 		= isupCkt->bearcap_check;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].itx_auto_reply 		= isupCkt->itx_auto_reply;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam 		= isupCkt->transparent_iam;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam_max_size 	= isupCkt->transparent_iam_max_size;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress_media 	= isupCkt->cpg_on_progress_media;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress 		= isupCkt->cpg_on_progress;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].ignore_alert_on_cpg 	= isupCkt->ignore_alert_on_cpg;

			g_ftdm_sngss7_data.cfg.isupCkt[idx].t3 				= isupCkt->t3;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t10 			= isupCkt->t10;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t12 			= isupCkt->t12;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t13 			= isupCkt->t13;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t14 			= isupCkt->t14;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t15 			= isupCkt->t15;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t16 			= isupCkt->t16;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t17 			= isupCkt->t17;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t35 			= isupCkt->t35;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].t39 			= isupCkt->t39;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].tval 			= isupCkt->tval;
			g_ftdm_sngss7_data.cfg.isupCkt[idx].reload 			= isupCkt->reload;

			if (g_ftdm_sngss7_data.cfg.isupCkt[idx].reload == FTDM_TRUE) {
				SS7_INFO("[Reload] Reconfigured/Added procId=%d, spanId = %d, chan = %d, cic = %d, ISUP cirId = %d\n",
					 g_ftdm_sngss7_data.cfg.isupCkt[idx].procId,
					 g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId,
					 g_ftdm_sngss7_data.cfg.isupCkt[idx].chan,
					 g_ftdm_sngss7_data.cfg.isupCkt[idx].cic,
					 g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
			}
reset:
			/* resetting ISUP Interface reload configuration */
			memset(&g_ftdm_sngss7_data.cfg_reload.isupCkt[idx], 0, sizeof(sng_isup_ckt_t));
			isupCkt = NULL;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
	return ret;
}
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
