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
 * 		Pushkar Singh <psingh@sangoma.com>
 *
 */

/* INCLUDE **********************************************************************/
#include "ftmod_sangoma_ss7_main.h"

/********************************************************************************/

/* DEFINES **********************************************************************/
/********************************************************************************/

/* GLOBALS **********************************************************************/
/********************************************************************************/

/* PROTOTYPES *******************************************************************/
/********************************************************************************/

/* FUNCTIONS ********************************************************************/

/*********************************************************************************
*
* Desc: Get starting CIC value for specific span ID passed
*
* Ret : Circuit ID(integer)
*
**********************************************************************************/
int ftmod_ss7_get_start_cic_value_by_span_id(int span_id)
{
	int start_range = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);
	int idx 	= 1;

	if (!start_range) {
		start_range = ((g_ftdm_sngss7_data.cfg.procId * 1000) + 1);
	}

	while( idx < (MAX_ISUP_INFS)) {
		if (g_ftdm_sngss7_data.cfg.isupCc[idx].span_id == span_id) {
			idx = g_ftdm_sngss7_data.cfg.isupCc[idx].ckt_start_val;
			break;
		}
		idx++;
	}

	if (idx < start_range) {
		SS7_WARN("Unable to retrieve start CIC value for span %d!\n", span_id);
		idx = 0;
	} else {
		SS7_DEBUG("ISUP circuit start from CIC %d for span %d\n",
			  idx, span_id);
	}

	return idx;
}

/*********************************************************************************
*
* Desc: Get get configured span type i.e. whether the span is a pure voice
* 	span or a signalling span
*
* Ret : span type (SNG_SPAN_INVALID | SNG_SPAN_VOICE | SNG_SPAN_SIG)
*
**********************************************************************************/
sng_span_type ftmod_ss7_get_span_type(int span_id, int cc_span_id)
{
	sng_span_type span_type 	= SNG_SPAN_INVALID;
	int 	      idx 		= 0;

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);

	if (!idx) {
		goto done;
	}

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {

		if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].span == span_id) &&
			(g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id)) {
			if (g_ftdm_sngss7_data.cfg.isupCkt[idx].type == SNG_CKT_SIG) {
				span_type = SNG_SPAN_SIG;
				SS7_DEBUG("Span %d is a SIGNALLING span\n", span_id);
				break;
			} else if (g_ftdm_sngss7_data.cfg.isupCkt[idx].type == SNG_CKT_VOICE) {
				span_type = SNG_SPAN_VOICE;
			}
		} else if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].span == 0) &&
				(g_ftdm_sngss7_data.cfg.isupCkt[idx].chan) &&
				(g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id) &&
				(g_ftdm_sngss7_data.cfg.isupCkt[idx].type == SNG_CKT_SIG)) {
				span_type = SNG_SPAN_SIG;
				SS7_DEBUG("Span %d is a SIGNALLING span\n", span_id);
				break;
				}
		idx++;
	}

	if (span_type == SNG_SPAN_INVALID) {
		SS7_ERROR("%d Span invalid span type!\n", span_id);
	} else if (span_type == SNG_SPAN_VOICE) {
		SS7_DEBUG("Span %d is a pure VOICE span\n", span_id);
	}

done:
	return span_type;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the MTP2 passed is mapped to
*
* Ret : span_id | zero
*
**********************************************************************************/
int ftmod_get_span_id_by_mtp2_id(int mtp2_idx)
{
	int mtp1_id 	= 0;
	int span_id 	= 0;
	int idx 	= 1;

	if (!mtp2_idx) {
		goto done;
	}

	mtp1_id = g_ftdm_sngss7_data.cfg.mtp2Link[mtp2_idx].mtp1Id;
	if (!mtp1_id) {
		goto done;
	}

	while (idx < MAX_MTP_LINKS) {
		if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].id == mtp1_id) {
			if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].span) {
				span_id = g_ftdm_sngss7_data.cfg.mtp1Link[idx].span;
				SS7_INFO("Found span %d mapped to MTP2-%d, MTP1-%d\n",
					 span_id, g_ftdm_sngss7_data.cfg.mtp2Link[mtp2_idx].id,
					 mtp1_id);
			} else {
				SS7_ERROR("No span found mapped to MTP2-%d and MTP1-%d!\n",
					  g_ftdm_sngss7_data.cfg.mtp2Link[mtp2_idx].id,
					  mtp1_id);
			}
			break;
		}
		idx++;
	}

done:
	if (!mtp2_idx) {
		SS7_ERROR("Invalid %d MTP2 index passed!\n", mtp2_idx);
	} else if (!mtp1_id) {
		SS7_ERROR("No MTP1 Link found mapped to MTP2 Link %d!\n",
			  g_ftdm_sngss7_data.cfg.mtp2Link[mtp2_idx].id);
	}

	return span_id;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the MTP3 passed is mapped to
*
* Ret : span_id | zero
*
**********************************************************************************/
int ftmod_ss7_get_span_id_by_mtp3_id(int mtp3_idx)
{
	int mtp1_id 	= 0;
	int mtp2_id 	= 0;
	int span_id 	= 0;
	int idx 	= 1;

	if (!mtp3_idx) {
		goto done;
	}

	mtp2_id = g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].mtp2Id;
	if (!mtp2_id) {
		goto done;
	}

	while (idx < MAX_MTP_LINKS) {
		if (g_ftdm_sngss7_data.cfg.mtp2Link[idx].id == mtp2_id) {
			mtp1_id = g_ftdm_sngss7_data.cfg.mtp2Link[idx].mtp1Id;
			break;
		}
		idx++;
	}
	if (!mtp1_id) {
		goto done;
	}

	idx = 1;
	while (idx < MAX_MTP_LINKS) {
		if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].id == mtp1_id) {
			if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].span) {
				span_id = g_ftdm_sngss7_data.cfg.mtp1Link[idx].span;
				SS7_INFO("Found span %d mapped to MTP3-%d, MTP2-%d, MTP1-%d\n",
					  span_id, g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id,
					  mtp2_id, mtp1_id);
			} else {
				SS7_ERROR("No span found mapped to MTP3-%d, MTP2-%d and MTP3-%d!\n",
					  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id,
					  mtp2_id, mtp1_id);
			}
			break;
		}
		idx++;
	}

done:
	if (!mtp3_idx) {
		SS7_ERROR("Invalid %d MTP3 index passed!\n", mtp3_idx);
	} else if (!mtp2_id) {
		SS7_ERROR("No MTP2 Link found mapped to MTP3 Link %d!\n",
			  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id);
	} else if (!mtp1_id) {
		SS7_ERROR("No MTP1 Link found mapped to MTP2-%d, MTP3-%d!\n",
			  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id,
			  mtp2_id);
	}

	return span_id;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the Linkset passed is mapped to
*
* Ret : span_id | zero
*
**********************************************************************************/
void ftmod_ss7_get_span_id_by_linkset_id(int linkset_idx, int span_id[])
{
	int linkset_id   = 0;
	int idx 	 = 1;
	int span_idx 	 = 0;

	if (!linkset_idx) {
		goto done;
	}

	linkset_id = g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].id;
	if (!linkset_id) {
		goto done;
	}

	while (idx < (MAX_MTP_LINKSETS+1)) {
		if (linkset_id == g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId) {
			span_id[span_idx] = ftmod_ss7_get_span_id_by_mtp3_id(idx);

			if (span_id[span_idx]) {
				SS7_INFO("Found span %d mapped to Linkset %d at span index %d!\n",
					 span_id[span_idx], linkset_id, span_idx);
				span_idx++;
			}
		}
		idx++;
	}

done:
	if (!linkset_id) {
		SS7_ERROR("Invalid %d linkset index passed!\n", linkset_idx);
	} else if (span_id[0] == 0) {
		SS7_ERROR("No Span found mapped to MTP Linkset %d!\n",
			  linkset_id);
	}

	return;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the MTP Route passed is mapped to
*
* Ret : span_id | zero
*
**********************************************************************************/
void ftmod_ss7_get_span_id_by_route_id(int route_idx, int span_id[])
{
	int route_id 	= 0;
	int idx  	= 1;

	if (!route_idx) {
		goto done;
	}

	route_id = g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id;
	if (!route_id) {
		goto done;
	}

	while (idx < (MAX_ISUP_INFS)) {
		if (route_id == g_ftdm_sngss7_data.cfg.isupIntf[idx].mtpRouteId) {
			ftmod_ss7_get_span_id_by_isup_interface_id(idx, span_id);
			break;
		}
		idx++;
	}

done:
	if (!route_id) {
		SS7_ERROR("Invalid %d route index passed!\n", route_idx);
	} else if (span_id[0] == 0) {
		SS7_ERROR("No Span found mapped to MTP Route %d with index !\n",
			  route_id);
	}

	return;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the MTP Interface passed is mapped to
*
* Ret : void
*
**********************************************************************************/
void ftmod_ss7_get_span_id_by_isup_interface_id(int isup_intf_idx, int span_id[])
{
	ftdm_bool_t res  = FTDM_TRUE;
	int idx  	 = 1;
	int isup_intf_id = 0;
	int span_idx 	 = 0;

	if (!isup_intf_idx) {
		goto done;
	}

	isup_intf_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id;
	if (!isup_intf_id) {
		goto done;
	}

	while (idx < (MAX_ISUP_INFS)) {
		if (g_ftdm_sngss7_data.cfg.isupCc[idx].infId == isup_intf_id) {
			if (g_ftdm_sngss7_data.cfg.isupCc[idx].span_id) {
				span_id[span_idx] = g_ftdm_sngss7_data.cfg.isupCc[idx].span_id;
				SS7_INFO("Found span %d mapped to ISUP interface %d\n",
					  span_id[span_idx], isup_intf_id);
			} else {
				res = FTDM_FALSE;
				SS7_ERROR("No span found mapped to ISUP interface %d\n",
					  isup_intf_id);
			}
			span_idx++;
		}
		idx++;
	}

done:
	if (!isup_intf_id) {
		SS7_ERROR("Invalid %d ISUP interface index passed!\n", isup_intf_idx);
	}

	/* In case when more than one span needs to be configured it is possible that
	 * the reconfiguration request comes for span for which circuit is not been
	 * configured yet. In such case please take this request as a new configuration
	 * request */
	if ((res == FTDM_TRUE) && (!span_id[0])) {
		span_id[0] = NOT_CONFIGURED;
	}

	return;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the MTP Self Route passed is mapped to
*
* Ret : void
*
**********************************************************************************/
void ftmod_ss7_get_span_id_by_mtp_self_route(int self_route_idx, int span_id[])
{
	int idx    = 1;
	int spc 	 = 0;

	if (!self_route_idx) {
		goto done;
	}

	spc = g_ftdm_sngss7_data.cfg.mtpRoute[self_route_idx].dpc;
	if (!spc) {
		goto done;
	}

	while (idx < (MAX_ISUP_INFS)) {
		if (g_ftdm_sngss7_data.cfg.isupIntf[idx].spc == spc) {
			ftmod_ss7_get_span_id_by_isup_interface_id(idx, span_id);
			break;
		}
		idx++;
	}

done:
	if (!self_route_idx) {
		SS7_ERROR("Invalid %d MTP Route idx passed!\n", self_route_idx);
	} else if (!spc) {
		SS7_ERROR("Invalid %d MTP Route foudn without spc !\n",
			  g_ftdm_sngss7_data.cfg.mtpRoute[self_route_idx].id);
	} else if (!span_id[0]) {
		SS7_ERROR("No span found mapped to Self Route %d having SPC %d\n",
			  g_ftdm_sngss7_data.cfg.mtpRoute[self_route_idx].id,
			  spc);
	}

	return;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the NSAP passed is mapped to
*
* Ret : void
*
**********************************************************************************/
void ftmod_ss7_get_span_id_by_nsap_id(int nsap_idx, int span_id[])
{
	int switchType 	 = 0;
	int linkType 	 = 0;
	int ssf 	 = 0;
	int idx          = 1;

	if (!nsap_idx) {
		goto done;
	}

	switchType = g_ftdm_sngss7_data.cfg.nsap[nsap_idx].switchType;
	linkType = g_ftdm_sngss7_data.cfg.nsap[nsap_idx].linkType;
	ssf = g_ftdm_sngss7_data.cfg.nsap[nsap_idx].ssf;

	/* SSf can be zero for INTL type but linkeType and swtchType can only be zero
	 * only and only for testing which we donot support in field */
	if ((!switchType) || (!linkType)) {
		goto done;
	}

	while (idx < (MAX_MTP_ROUTES+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].linkType == linkType) &&
		    (g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType == switchType) &&
		    (g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf == ssf)) {
			ftmod_ss7_get_span_id_by_route_id(idx, span_id);
			break;
		}
		idx++;
	}

done:
	if (!nsap_idx) {
		SS7_ERROR("Invalid %d NSAP idx passed!\n", nsap_idx);
	} else if (!switchType) {
		SS7_ERROR("No switchType found for NSAP %d!\n",
			  g_ftdm_sngss7_data.cfg.nsap[nsap_idx].id);
	} else if (!linkType) {
		SS7_ERROR("No linkType found for NSAP %d!\n",
			  g_ftdm_sngss7_data.cfg.nsap[nsap_idx].id);
	} else if (!span_id[0]) {
		SS7_ERROR("No span found mapped to NSAP %d with switchType %d, linkType %d and ssf %d!\n",
			  g_ftdm_sngss7_data.cfg.nsap[nsap_idx].id, switchType, linkType, ssf);
	}

	return;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the ISAP passed is mapped to
*
* Ret : void
*
**********************************************************************************/
void ftmod_ss7_get_span_id_by_isap_id(int isap_idx, int span_id[])
{
	int isap_id 	 = 0;
	int idx          = 1;

	if (!isap_idx) {
		goto done;
	}

	isap_id = g_ftdm_sngss7_data.cfg.isap[isap_idx].id;
	if (!isap_id) {
		goto done;
	}

	while (idx < (MAX_ISUP_INFS)) {
		if (g_ftdm_sngss7_data.cfg.isupIntf[idx].isap == isap_id) {
			ftmod_ss7_get_span_id_by_isup_interface_id(idx, span_id);
			break;
		}
		idx++;
	}

done:
	if (!isap_idx) {
		SS7_ERROR("Invalid %d ISAP idx passed!\n", isap_idx);
	} else if (!isap_id) {
		SS7_ERROR("No ISAP is configured at index %d!\n",
			  isap_idx);
	} else if (!span_id[0]) {
		SS7_ERROR("No span found mapped to ISAP %d mapped to ISUP interface %d!\n",
			  isap_id, g_ftdm_sngss7_data.cfg.isupIntf[isap_idx].id);
	}

	return;
}

/*********************************************************************************
*
* Desc: Get get the span ID to which the CC Span passed is mapped to
*
* Ret : span_id | zero
*
**********************************************************************************/
int ftmod_ss7_get_span_id_by_cc_span_id(int cc_span_id)
{
	int idx  	 = 1;
	int span_id 	 = 0;

	if (!cc_span_id) {
		goto done;
	}

	while (idx < (MAX_ISUP_INFS)) {
		if (g_ftdm_sngss7_data.cfg.isupCc[idx].cc_span_id == cc_span_id) {
			if (g_ftdm_sngss7_data.cfg.isupCc[idx].span_id) {
				span_id = g_ftdm_sngss7_data.cfg.isupCc[idx].span_id;
				SS7_INFO("Found span %d mapped to CC Span %d\n",
					  span_id, cc_span_id);
			} else {
				SS7_ERROR("No span found mapped to CC Span %d\n",
					  cc_span_id);
			}
			break;
		}
		idx++;
	}

done:
	if (!cc_span_id) {
		SS7_ERROR("Invalid %d CC Span passed!\n", cc_span_id);
	}

	return span_id;
}

/*********************************************************************************
*
* Desc: Get respective signalling span status based on voice span passed
*
* Ret : FTDM_SUCCESS (in case signalling span is still up and running) else
* 	FTDM_FAIL
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_get_sig_span_status(int span_id, int cc_span_id)
{
	sng_span_type 	sp_type 	= SNG_SPAN_INVALID;
	ftdm_status_t   status 		= FTDM_FAIL;
	int 		isup_intf_idx 	= 0;
	int 		isup_intf_id  	= 0;
	int 		idx 		= 1;

	/* get isup interface configuration against this span */
	isup_intf_idx = ftmod_ss7_get_isup_intf_id_by_span_id(span_id);
	if (!isup_intf_idx) {
		SS7_ERROR("No Configured ISUP Interface found for span %d\n", span_id);
		goto done;
	}

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		isup_intf_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].mtpRouteId;
	}

	if (!isup_intf_id) {
		SS7_ERROR("No Configured ISUP Interface found at index %d\n", isup_intf_idx);
		goto done;
	}

	while( idx < (MAX_ISUP_INFS)) {
		if (g_ftdm_sngss7_data.cfg.isupCc[idx].infId == isup_intf_id) {
			sp_type = SNG_SPAN_INVALID;

			sp_type = ftmod_ss7_get_span_type(g_ftdm_sngss7_data.cfg.isupCc[idx].span_id, g_ftdm_sngss7_data.cfg.isupCc[idx].cc_span_id);

			if (sp_type == SNG_SPAN_SIG) {
				SS7_DEBUG("Delete ISUP circuits for span %d as its relative Signalling span %d is still present\n",
					  span_id, g_ftdm_sngss7_data.cfg.isupCc[idx].span_id);
				status = FTDM_SUCCESS;
				break;
			} else if (sp_type == SNG_SPAN_INVALID) {
				SS7_DEBUG("Span %d type is INVALID!\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id);
			}
		}
		idx++;
	}

done:
	return status;
}

/*********************************************************************************
*
* Desc: Get the index in the global structure (g_ftdm_sngss7_data.cfg.mtp1Link)
* 	where MTP1 Link is mapped to span passed is stored
*
* Ret : MTP1 Link index (integer)
*
**********************************************************************************/
int ftmod_ss7_get_mtp1_id_by_span_id(int span_id)
{
    int mtp1_cfg_id 	= 0;
    int idx 		= 1;

    while (idx < (MAX_MTP_LINKS)) {
        if ((g_ftdm_sngss7_data.cfg.mtp1Link[idx].id != 0) &&
                (g_ftdm_sngss7_data.cfg.mtp1Link[idx].flags & SNGSS7_CONFIGURED)) {

            if (span_id == g_ftdm_sngss7_data.cfg.mtp1Link[idx].span) {
                SS7_DEBUG("Found matching MTP1 config index at %d\n", idx);
                mtp1_cfg_id = idx;
                break;
            }
        }
        idx++;
    }

    if (!mtp1_cfg_id) {
	    SS7_ERROR("No MTP1 link found for span %d\n", span_id);
    } else {
	    SS7_DEBUG("MTP1-%d link found mapped to span %d\n", g_ftdm_sngss7_data.cfg.mtp1Link[mtp1_cfg_id].id,
		      span_id);
    }

    return mtp1_cfg_id;
}

/*********************************************************************************
*
* Desc: Get the index in the global structure (g_ftdm_sngss7_data.cfg.mtp2Link)
* 	where MTP2 Link is mapped to MTP1 Link passed is stored
*
* Ret : MTP2 Link index (integer)
*
**********************************************************************************/
int ftmod_ss7_get_mtp2_id_by_mtp1_id(int mtp1Idx)
{
    int mtp2_cfg_id 	= 0;
    int mtp1Id 		= 0;
    int idx 		= 1;

    if ((g_ftdm_sngss7_data.cfg.mtp1Link[mtp1Idx].id != 0) &&
	    (g_ftdm_sngss7_data.cfg.mtp1Link[mtp1Idx].flags & SNGSS7_CONFIGURED)) {
	    mtp1Id = g_ftdm_sngss7_data.cfg.mtp1Link[mtp1Idx].id;
    }


    if (!mtp1Id) {
	    SS7_ERROR("No Configured MTP1 link found at index %d\n", mtp1Idx);
	    goto done;
    }

    while (idx < (MAX_MTP_LINKS)) {
        if ((g_ftdm_sngss7_data.cfg.mtp2Link[idx].id != 0) &&
                (g_ftdm_sngss7_data.cfg.mtp2Link[idx].flags & SNGSS7_CONFIGURED)) {

            if (mtp1Id == g_ftdm_sngss7_data.cfg.mtp2Link[idx].mtp1Id) {
                SS7_DEBUG("Found matching mtp2 config index at %d\n", idx);
                mtp2_cfg_id = idx;
                break;
            }
        }
        idx++;
    }

done:
    if (!mtp2_cfg_id) {
	    SS7_ERROR("No MTP2 link found map to MTP1 link %d\n", mtp1Id);
    } else {
	    SS7_DEBUG("MTP2-%d link found mapped to MTP1-%d link\n", g_ftdm_sngss7_data.cfg.mtp2Link[mtp2_cfg_id].id,
		      mtp1Id);
    }

    return mtp2_cfg_id;
}

/*********************************************************************************
*
* Desc: Get the index in the global structure (g_ftdm_sngss7_data.cfg.mtp3Link)
* 	where MTP3 Link mapped to MTP2 Link passed is stored
*
* Ret : MTP3 Link index (integer)
*
**********************************************************************************/
int ftmod_ss7_get_mtp3_id_by_mtp2_id(int mtp2Idx)
{
    int mtp3_cfg_id 	= 0;
    int mtp2Id 		= 0;
    int idx 		= 1;

    if ((g_ftdm_sngss7_data.cfg.mtp2Link[mtp2Idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.mtp2Link[mtp2Idx].flags & SNGSS7_CONFIGURED)) {
	    mtp2Id = g_ftdm_sngss7_data.cfg.mtp2Link[mtp2Idx].id;
    }

    if (!mtp2Id) {
	    SS7_ERROR("No Configured MTP2 link found at index %d\n", mtp2Idx);
	    goto done;
    }

    while (idx < (MAX_MTP_LINKS)) {
        if ((g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0) &&
                (g_ftdm_sngss7_data.cfg.mtp3Link[idx].flags & SNGSS7_CONFIGURED)) {

            if (mtp2Id == g_ftdm_sngss7_data.cfg.mtp3Link[idx].mtp2Id) {
                SS7_DEBUG("Found matching mtp3 config index at %d\n", idx);
                mtp3_cfg_id = idx;
                break;
            }
        }
        idx++;
    }

done:
    if (!mtp3_cfg_id) {
	    SS7_ERROR("No MTP3 link found map to MTP2 link %d\n", mtp2Id);
    } else {
	    SS7_DEBUG("MTP3-%d link found mapped to MTP2-%d link\n", g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_cfg_id].id,
		      mtp2Id);
    }

    return mtp3_cfg_id;
}

/*********************************************************************************
*
* Desc: Get the index in the global structure (g_ftdm_sngss7_data.cfg.mtp3Link)
* 	where MTP3 Link mapped to LinkSet ID passed is stored
*
* Ret : MTP3 Link index (integer)
*
**********************************************************************************/
int ftmod_ss7_get_mtp3_link_by_linkset_id(int linkset_id)
{
	int mtp3_cfg_id 	= 0;
	int idx 		= 1;

	if (!linkset_id) {
		SS7_ERROR("Invalid linkSetId passed!\n");
		goto done;
	}

	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0) &&
		    (g_ftdm_sngss7_data.cfg.mtp3Link[idx].flags & SNGSS7_CONFIGURED)) {

			if (linkset_id == g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId) {
				SS7_DEBUG("Found matching mtp3 config index at %d\n", idx);
				mtp3_cfg_id = idx;
				break;
			}
		}
		idx++;
	}

done:
	if (!mtp3_cfg_id) {
		SS7_ERROR("No MTP3 Link found map to LinkSet-%d!\n", linkset_id);
	} else {
		SS7_DEBUG("Found MTP3 Link %d map to LinkSet-%d\n",
			  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_cfg_id].id, linkset_id);
	}

	return mtp3_cfg_id;
}

/*********************************************************************************
*
* Desc: Get the index in the global structure (g_ftdm_sngss7_data.cfg.mtpLinkSet)
* 	where MTP Linkset mapped to MTP3 Link passed is stored
*
* Ret : MTP Linkset index (integer)
*
**********************************************************************************/
int ftmod_ss7_get_linkset_id_by_mtp3_id(int mtp3Idx)
{
	int linkset_cfg_id 	= 0;
	int linkset_id 		= 0;
	int idx 		= 1;

	if ((g_ftdm_sngss7_data.cfg.mtp3Link[mtp3Idx].id) &&
		(g_ftdm_sngss7_data.cfg.mtp3Link[mtp3Idx].flags & SNGSS7_CONFIGURED)) {
		linkset_id = g_ftdm_sngss7_data.cfg.mtp3Link[mtp3Idx].linkSetId;
	}

	if (!linkset_id) {
		SS7_ERROR("No Configured MTP3 link found at index %d!\n", mtp3Idx);
		goto done;
	}

	while (idx < (MAX_MTP_LINKSETS+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].flags & SNGSS7_CONFIGURED)) {

			if (g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id == linkset_id) {
				SS7_DEBUG("Found matching MTP Linkset index at %d\n", idx);
				linkset_cfg_id = idx;
			}
		}
		idx++;
	}

done:
	if (!linkset_cfg_id) {
		SS7_ERROR("No Linkset found map to MTP3-%d link!\n", g_ftdm_sngss7_data.cfg.mtp3Link[mtp3Idx].id);
	} else {
		SS7_DEBUG("Found MTP Linkset %d map to MTP3-%d link\n", linkset_id,
			  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3Idx].id);
	}

	return linkset_id;
}

/*********************************************************************************
*
* Desc: Get list & number of MTP Linksets that are mapped to MTP ROUTE based on
* 	on the MTP ROUTE ID passed
*
* Ret : Array/List of MTP Linkset(integer)
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_get_linkset_id_by_route_id(int route_idx, int linkset_id[], int num_linksets)
{
	sng_link_set_list_t *lnkSet 	= NULL;
	ftdm_status_t 	     ret 	= FTDM_FAIL;
	int 		     idx 	= 0;

	if ((g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].flags & SNGSS7_CONFIGURED)) {

		if (&g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].lnkSets) {
			for (lnkSet = &g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].lnkSets; lnkSet != NULL; lnkSet = lnkSet->next) {
				linkset_id[idx] = lnkSet->lsId;
				SS7_DEBUG("Found maching MTP linkset %d map to route %d\n",
					  lnkSet->lsId, g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id);

				ret = FTDM_SUCCESS;
				idx++;
			}
		}
	} else {
		SS7_ERROR("No configured MTP ROUTE found at index %d!\n", route_idx);
	}

	if (ret == FTDM_FAIL) {
		SS7_ERROR("No linkset found map to route %d\n", g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id);
	}
	num_linksets = idx;

	return ret;
}

/*********************************************************************************
*
* Desc: Get the index in the global structure (g_ftdm_sngss7_data.cfg.mtpRoute)
* 	where MTP Route mapped to ISUP Interface passed is stored
*
* Ret : MTP Route Index(integer)
*
**********************************************************************************/
int ftmod_ss7_get_route_id_by_isup_intf_id(int isup_intf_idx)
{
	int route_idx 	= 0;
	int route_id 	= 0;
	int idx 	= 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		route_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].mtpRouteId;
	}

	if (!route_id) {
		SS7_ERROR("No Configured ISUP interface found at index %d!\n", isup_intf_idx);
		goto done;
	}

	while (idx < (MAX_MTP_ROUTES+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.mtpRoute[idx].flags & SNGSS7_CONFIGURED)) {

			if (route_id == g_ftdm_sngss7_data.cfg.mtpRoute[idx].id) {
				SS7_DEBUG("Found matching MTP Route at index at %d\n", idx);
				route_idx = idx;
				break;
			}
		}
		idx++;
	}

done:
	if (!route_idx) {
		SS7_ERROR("No MTP Route found map to ISUP interface %d\n", g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id);
	} else {
		SS7_DEBUG("Found MTP Route %d map to ISUP interface %d\n",
			  route_id, g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id);
	}

	return route_idx;
}

/*********************************************************************************
*
* Desc: Get list & number of MTP Routes that are mapped to particular MTP Linkset
*	based on MTP Linkset ID passed
*
* Ret : Array/List of MTP Route(integer)
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_get_route_id_by_linkset_id(int linkset_idx, int route_id[], int num_routes)
{
	ftdm_status_t 	ret = FTDM_FAIL;
	int 		idx = 0;

	if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].flags & SNGSS7_CONFIGURED)) {

		for (idx = 0; idx < g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].numLinks; idx++) {
			route_id[idx] = g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].links[idx];
			num_routes++;
			SS7_DEBUG("Found route %d map to linkset ID %d\n", route_id[idx], linkset_idx);

			ret = FTDM_SUCCESS;
		}
	} else {
		SS7_ERROR("No Configured MTP Linkset found at index %d!\n", linkset_idx);
	}

	if (ret == FTDM_FAIL) {
		SS7_ERROR("No MTP Route found map to linkset %d\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].id);
	}

	return FTDM_SUCCESS;
}

/*********************************************************************************
*
* Desc: Get the index in the global structure (g_ftdm_sngss7_data.cfg.mtpRoute)
* 	where MTP SELF Route mapped to ISUP Interface passed is stored
*
* Ret : MTP SELF Route Index(integer)
*
**********************************************************************************/
int ftmod_ss7_get_self_route_id_by_isup_intf_id(int isup_intf_idx)
{
	int self_route_idx = 0;
	int switchType = 0;
	int ssf = 0;
	int spc = 0;
	int idx = 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		switchType = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].switchType;
		ssf = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].ssf;
		spc = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].spc;

	} else {
		SS7_ERROR("No Configured ISUP Interface found at index %d!\n", isup_intf_idx);
		goto done;
	}

	/* search for self route list
	 * NOTE: self route list starts from MAX_MTP_ROUTES + 1 index */
	idx = MAX_MTP_ROUTES + 1;
	while ((idx > MAX_MTP_ROUTES) && idx < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.mtpRoute[idx].flags & SNGSS7_CONFIGURED)) {

			if ((switchType == g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType) &&
					(ssf == g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf) &&
					(spc == g_ftdm_sngss7_data.cfg.mtpRoute[idx].dpc)) {
				SS7_DEBUG("Found MTP Self Route at index %d with switchType[%d] ssf [%d] and spc[%d]\n",
					  idx, switchType, ssf, spc);
				self_route_idx = idx;
				break;
			}
		}
		idx++;
	}

done:
	if (!self_route_idx) {
		SS7_ERROR("No MTP Self Route found map to ISUP interface %d\n", g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id);
	} else {
		SS7_DEBUG("Found MTP Self Route %d map to ISUP interface %d\n", g_ftdm_sngss7_data.cfg.mtpRoute[self_route_idx].id,
			  g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id);
	}

	return self_route_idx;
}

/*********************************************************************************
*
* Desc: Get index in the global structure (g_ftdm_sngss7_data.cfg.isupIntf) where
* 	ISUP interface mapped to span Id passed is stored
*
* Ret : ISUP Interface Index(integer)
*
**********************************************************************************/
int ftmod_ss7_get_isup_intf_id_by_span_id(int span_id)
{
	int isup_intf_idx = 0;
	int isup_intf_id  = 0;
	int idx 	  = 0;

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);

	if (!idx) {
		goto done;
	}

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].span == span_id) {
			isup_intf_id = g_ftdm_sngss7_data.cfg.isupCkt[idx].infId;
			break;
		}
		idx++;
	}

	if (!isup_intf_id) {
		SS7_ERROR("No Configured ISUP Interface found in global ISUP circuits list!\n", span_id);
		goto done;
	}

	idx = 1;
	while (idx < (MAX_ISUP_INFS)) {
		if ((g_ftdm_sngss7_data.cfg.isupIntf[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.isupIntf[idx].flags & SNGSS7_CONFIGURED)) {
			if (isup_intf_id == g_ftdm_sngss7_data.cfg.isupIntf[idx].id) {
				isup_intf_idx = idx;
				SS7_DEBUG("Found matching ISUP Interface at index %d\n", idx);
				break;
			}
		}
		idx++;
	}

done:
	if (!isup_intf_idx) {
		SS7_ERROR("No Isup Interface found for span %d\n", span_id);
	} else {
		SS7_DEVEL_DEBUG("Found ISUP Interface %d map to span %d\n", isup_intf_id, span_id);
	}

	return isup_intf_idx;
}

/*********************************************************************************
*
* Desc: Get CC Span Id based on span Id passed
*
* Ret : CC Span ID(integer)
*
**********************************************************************************/
int ftmod_ss7_get_cc_span_id_by_span_id(int span_id)
{
	int cc_span_id 	= 0;
	int idx 	= 0;

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);

	if (!idx) {
		goto done;
	}

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].span == span_id) {
			if (g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId) {
				cc_span_id = g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId;
				SS7_DEBUG("Found matching CC Span at ISUP Interface index %d\n", idx);
				break;
			}
		}
		idx++;
	}

done:
	if (!cc_span_id) {
		if (ftdm_running()) {
			SS7_ERROR("No matching CC Span found for span %d!\n", span_id);
		} else {
			SS7_DEBUG("All Circuits already cleared for voice span %d\n", span_id);
			ftmod_ss7_clear_cc_span_info_by_span_id(span_id);
		}
	} else {
		SS7_DEBUG("Found CC Span %d for span %d\n", cc_span_id, span_id);
	}

	return cc_span_id;
}

/*********************************************************************************
*
* Desc: Get index in the global structure (g_ftdm_sngss7_data.cfg.nsap) where
* 	NSAP mapped to MTP3 Link passed is stored
*
* Ret : NSAP Index(integer)
*
**********************************************************************************/
int ftmod_ss7_get_nsap_id_by_mtp3_id(int mtp3_idx)
{
	int nsap_cfg_id = 0;
	int idx 	= 1;

	if ((g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].flags & SNGSS7_CONFIGURED)) {

		while (idx < (MAX_NSAPS)) {
			if ((g_ftdm_sngss7_data.cfg.nsap[idx].id != 0) &&
				(g_ftdm_sngss7_data.cfg.nsap[idx].flags & SNGSS7_CONFIGURED)) {
				if ((g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].ssf == g_ftdm_sngss7_data.cfg.nsap[idx].ssf) &&
				    (g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].switchType == g_ftdm_sngss7_data.cfg.nsap[idx].switchType) &&
				    (g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].linkType == g_ftdm_sngss7_data.cfg.nsap[idx].linkType)) { 
					    nsap_cfg_id = idx;
					    SS7_DEBUG("Found NSAP %d map to Mtp3-%d Link\n",
						      g_ftdm_sngss7_data.cfg.nsap[idx].id,
						      g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id);
					    break;
				    }
			}
			idx++;
		}
	} else {
		SS7_ERROR("No MTP3 Link present at index %d!\n", mtp3_idx);
		goto done;
	}

	if (!nsap_cfg_id) {
		SS7_ERROR("No NSAP found map to MTP3-%d Link\n",
			  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id);
	} else {
		SS7_DEBUG("NSAP found map to MTP3-%d Link at index %d\n",
			  g_ftdm_sngss7_data.cfg.mtp3Link[mtp3_idx].id,
			  nsap_cfg_id);
	}

done:
	return nsap_cfg_id;
}

/*********************************************************************************
*
* Desc: Get index in the global structure (g_ftdm_sngss7_data.cfg.nsap) where
* 	NSAP mapped to MTP Route ID passed is stored
*
* Ret : NSAP Index(integer)
*
**********************************************************************************/
int ftmod_ss7_get_nsap_id_by_mtp_route_id(int route_idx)
{
	int nsap_cfg_id = 0;
	int idx 	= 1;

	if ((g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].flags & SNGSS7_CONFIGURED)) {

		while (idx < (MAX_NSAPS)) {
			if ((g_ftdm_sngss7_data.cfg.nsap[idx].id != 0) &&
				(g_ftdm_sngss7_data.cfg.nsap[idx].flags & SNGSS7_CONFIGURED)) {
				if ((g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].ssf == g_ftdm_sngss7_data.cfg.nsap[idx].ssf) &&
				    (g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].switchType == g_ftdm_sngss7_data.cfg.nsap[idx].switchType) &&
				    (g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].linkType == g_ftdm_sngss7_data.cfg.nsap[idx].linkType)) { 
					    nsap_cfg_id = idx;
					    SS7_DEBUG("Found NSAP %d map to route Id %d\n",
						      g_ftdm_sngss7_data.cfg.nsap[idx].id,
						      g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id);
					    break;
				    }
			}
			idx++;
		}
	} else {
		SS7_ERROR("No MTP Route present at index %d!\n", route_idx);
		goto done;
	}

	if (!nsap_cfg_id) {
		SS7_ERROR("No NSAP found map to route Id %d\n", g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id);
	} else {
		SS7_DEBUG("NSAP found map to MTP route ID %d at index %d\n",
			  g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id,
			  nsap_cfg_id);
	}

done:
	return nsap_cfg_id;
}

/*********************************************************************************
*
* Desc: Get index in the global structure (g_ftdm_sngss7_data.cfg.isap) where
*	ISAP mapped to ISUP Interface passed is stored
*
* Ret : ISAP Index(integer)
*
**********************************************************************************/
int ftmod_ss7_get_isap_id_by_isup_intf_id(int isup_intf_idx)
{
	int isap_idx 	= 0;
	int isap_id 	= 0;
	int idx 	= 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		isap_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].isap;
	}

	if (!isap_id) {
		SS7_ERROR("No Configured ISUP Interface found at index %d!\n", isup_intf_idx);
		goto done;
	}

	while (idx < (MAX_ISAPS)) {
		if ((g_ftdm_sngss7_data.cfg.isap[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.isap[idx].flags & SNGSS7_CONFIGURED)) {

			if (isap_id == g_ftdm_sngss7_data.cfg.isap[idx].id) {
				SS7_DEBUG("Found matching ISAP at index %d\n", idx);
				isap_idx = idx;
				break;
			}
		}
		idx++;
	}

	if (!isap_idx) {
		SS7_ERROR("No route map found map to ISUP interface %d\n", g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id);
	} else {
		SS7_DEBUG("Found ISAP %d map to ISUP interface %d\n",
			  isap_id, g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id);
	}

done:
	return isap_idx;
}

/*********************************************************************************
*
* Desc: Get upper spId based on span_id passed
*
* Ret : Interger value
*
**********************************************************************************/
int ftmod_ss7_get_spId_by_span_id(int span_id)
{
	int isup_intf_idx = 0;
	int isap_idx 	  = 0;

	isup_intf_idx = ftmod_ss7_get_isup_intf_id_by_span_id(span_id);

	if (!isup_intf_idx) {
		goto done;
	}

	isap_idx = ftmod_ss7_get_isap_id_by_isup_intf_id(isup_intf_idx);

done:
	if (!isup_intf_idx) {
		SS7_ERROR("No isup interface found mapped to span %d!\n", span_id);
	} else if (!isap_idx) {
		SS7_ERROR("No ISAP found mapped to isup interface %d and span %d!\n",
			  g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id, span_id);
	} else {
		SS7_INFO("Found ISAP %d mapped to span %d with isup interface %d\n",
			 g_ftdm_sngss7_data.cfg.isap[isap_idx].id,
			 g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id,
			 span_id);
		return g_ftdm_sngss7_data.cfg.isap[isap_idx].spId;
	}

	return 0;

}

/*********************************************************************************
*
* Desc: Get number of MTP3 Links mapped to MTP Linkset passed
*
* Ret : Number of MTP3 Links(integer)
*
**********************************************************************************/
int ftmod_ss7_get_num_of_mtp3_links_by_linkset_id(int linkset_idx)
{
	int num_mtp3_links = 0;
	int linkset_id 	   = 0;
	int idx 	   = 1;

	if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].flags & SNGSS7_CONFIGURED)) {

		linkset_id = g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_idx].id;
	}

	if (!linkset_id) {
		SS7_ERROR("No Configured MTP Linkset present at index %d!\n", linkset_idx);
		goto done;
	}

	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.mtp3Link[idx].flags & SNGSS7_CONFIGURED)) {

			if (linkset_id == g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId) {
				num_mtp3_links++;
			}
		}
		idx++;
	}

	if (!num_mtp3_links) {
		SS7_ERROR("No MTP3 Link found mapped to linkset %d!\n", linkset_id);
	} else {
		SS7_DEBUG("%d MTP3 Links mapped to linkset %d\n", num_mtp3_links, linkset_id);
	}

done:
	return num_mtp3_links;
}

/*********************************************************************************
*
* Desc: Get number of MTP3 Routes mapped to NSAP passed
*
* Ret : Number of MTP3 Routes(integer)
*
**********************************************************************************/
int ftmod_ss7_get_num_of_mtp3_routes_using_nsap(int nsap_idx)
{
	uint32_t switchType = 0;
	uint32_t linkType   = 0;
	uint32_t ssf 	    = 0;
	int num_mtp3_route  = 0;
	int idx 	    = 1;

	if ((g_ftdm_sngss7_data.cfg.nsap[nsap_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.nsap[nsap_idx].flags & SNGSS7_CONFIGURED)) {
		switchType = g_ftdm_sngss7_data.cfg.nsap[nsap_idx].switchType;
		linkType = g_ftdm_sngss7_data.cfg.nsap[nsap_idx].linkType;
		ssf = g_ftdm_sngss7_data.cfg.nsap[nsap_idx].ssf;
	} else {
		SS7_ERROR("No configured NSAP found at index %d!\n", nsap_idx);
		goto done;
	}

	while (idx < (MAX_MTP_ROUTES+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.mtpRoute[idx].flags & SNGSS7_CONFIGURED)) {
			if ((switchType == g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType) &&
				(linkType == g_ftdm_sngss7_data.cfg.mtpRoute[idx].linkType) &&
				(ssf == g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf)) {

				/* Route must be an MTP route and not self route  as self route
				 * start from index SELF_ROUTE_INDEX */
				if (strcasecmp(g_ftdm_sngss7_data.cfg.mtpRoute[idx].name, "self-route")) {
					num_mtp3_route++;
				}
			}
		}
		idx++;
	}

	if (!num_mtp3_route) {
		SS7_ERROR("No MTP Route found mapped to NSAP with switchType %d, LinkType %d and ssf %d!\n",
			  switchType, linkType, ssf);
	} else {
		SS7_DEBUG("%d MTP3 Routes found mapped to NSAP with switchType %d, LinkType %d and ssf %d\n",
			  num_mtp3_route, switchType, linkType, ssf);
	}
done:
	return num_mtp3_route;
}

/*********************************************************************************
*
* Desc: Get number of ISUP Interfaces mapped to MTP Route passed
*
* Ret : Number of ISUP Interfaces(integer)
*
**********************************************************************************/
int ftmod_ss7_get_num_of_isup_intf_using_route(int route_idx)
{
	int num_isup_intf = 0;
	int route_id 	  = 0;
	int idx 	  = 1;

	if ((g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].flags & SNGSS7_CONFIGURED)) {
		route_id = g_ftdm_sngss7_data.cfg.mtpRoute[route_idx].id;
	}

	if (!route_id) {
		SS7_ERROR("No Configured MTP Route found at index %d!\n", route_idx);
		goto done;
	}

	while (idx < (MAX_ISUP_INFS)) {
		if ((g_ftdm_sngss7_data.cfg.isupIntf[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.isupIntf[idx].flags & SNGSS7_CONFIGURED)) {

			if (route_id == g_ftdm_sngss7_data.cfg.isupIntf[idx].mtpRouteId) {
				num_isup_intf++;
			}
		}
		idx++;
	}

	if (!num_isup_intf) {
		SS7_ERROR("No ISUP Interface found mapped to MTP Route %d!\n", route_id);
	} else {
		SS7_DEBUG("%d number of ISUP Interface mapped to MTP Route %d\n", num_isup_intf, route_id);
	}

done:
	return num_isup_intf;
}

/*********************************************************************************
*
* Desc: Get number of ISUP Interfaces mapped to ISAP passed
*
* Ret : Number of ISUP Interfaces(integer)
*
**********************************************************************************/
int ftmod_ss7_get_num_of_intf_using_isap(int isap_idx)
{
	uint32_t switchType = 0;
	uint32_t ssf 	    = 0;
	int num_isup_intf   = 0;
	int idx 	    = 1;

	if ((g_ftdm_sngss7_data.cfg.isap[isap_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isap[isap_idx].flags & SNGSS7_CONFIGURED)) {

		switchType = g_ftdm_sngss7_data.cfg.isap[isap_idx].switchType;
		ssf = g_ftdm_sngss7_data.cfg.isap[isap_idx].ssf;
	} else {
		SS7_ERROR("No configured ISAP found at index %d!\n", isap_idx);
		goto done;
	}

	while (idx < (MAX_ISUP_INFS)) {
		if ((g_ftdm_sngss7_data.cfg.isupIntf[idx].id != 0) &&
			(g_ftdm_sngss7_data.cfg.isupIntf[idx].flags & SNGSS7_CONFIGURED)) {
			if ((switchType == g_ftdm_sngss7_data.cfg.isupIntf[idx].switchType) &&
				(ssf == g_ftdm_sngss7_data.cfg.isupIntf[idx].ssf)) {
				num_isup_intf++;
			}
		}
		idx++;
	}

	if (!num_isup_intf) {
		SS7_ERROR("No ISUP Interface isap mapped to %d with Switch type %d and ssf %d!\n",
			  g_ftdm_sngss7_data.cfg.isap[isap_idx].id, switchType, ssf);
	} else {
		SS7_DEBUG("ISAP %d with Switch type %d and ssf %d is mapped to %d isup interface\n",
			  g_ftdm_sngss7_data.cfg.isap[isap_idx].id, num_isup_intf, switchType, ssf);
	}

done:
	return num_isup_intf;
}

/*********************************************************************************
*
* Desc: Get number of CC Spans mapped to ISUP Interface passed
*
* Ret : Number of CC Spans(integer)
*
**********************************************************************************/
int ftmod_ss7_get_num_of_cc_span_by_isup_intf(int isup_intf_idx, int span_id)
{
	sng_span_type 	sp_type      = SNG_SPAN_INVALID;
	ftdm_span_t 	*span 	     = NULL;
	int 		isup_intf_id = 0;
	int 		num_cc_span  = 0;
	int 		idx 	     = 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		isup_intf_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id;
	}

	if (!isup_intf_id) {
		SS7_ERROR("No Configured ISUP Interface found at index %d!\n", isup_intf_idx);
		goto done;
	}
	
	while (idx < (MAX_ISUP_INFS)) {
		if (isup_intf_id == g_ftdm_sngss7_data.cfg.isupCc[idx].infId) {
			if (span_id == g_ftdm_sngss7_data.cfg.isupCc[idx].cc_span_id) {
				num_cc_span++;
				idx++;
				continue;
			}

			sp_type = ftmod_ss7_get_span_type(g_ftdm_sngss7_data.cfg.isupCc[idx].span_id, g_ftdm_sngss7_data.cfg.isupCc[idx].cc_span_id);
			if (sp_type == SNG_SPAN_INVALID) {
				SS7_ERROR("Failed to get span type for span %d map to cc span %d and interface %d!\n",
					  g_ftdm_sngss7_data.cfg.isupCc[idx].span_id,
					  g_ftdm_sngss7_data.cfg.isupCc[idx].cc_span_id,
					  isup_intf_id);
				break;
			} else if (sp_type == SNG_SPAN_VOICE) {
				SS7_INFO("Foun associated pure voice span %d (ccspan = %d) for this span %d and interface %d!\n",
					 g_ftdm_sngss7_data.cfg.isupCc[idx].span_id,
					 g_ftdm_sngss7_data.cfg.isupCc[idx].cc_span_id,
					 span_id,
					 isup_intf_id);

				/* Since there is an associated voice span please stop that span too */
				ftdm_span_find(g_ftdm_sngss7_data.cfg.isupCc[idx].span_id, &span);
				if (!span) {
					SS7_ERROR("Cannot find span %d!\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id);
					idx++;
					continue;
				}

				ftdm_span_set_reconfig_flag(span);
				ftdm_sleep(100);
				ftdm_span_stop(span);
			} else {
				num_cc_span++;
			}
		}
		idx++;
	}

	if (!num_cc_span) {
		SS7_ERROR("No ISUP CC Span's found mapped to ISUP Interface %d!\n", isup_intf_id);
	} else {
		SS7_DEBUG("%d number of CC SPAN mapped to ISUP Interface %d\n", num_cc_span, isup_intf_id);
	}

done:
	return num_cc_span;
}

/*********************************************************************************
*
* Desc: Disable all ISUP Circuits mapped to ISUP interface and Span Id  passed
*
* Ret : Return Value from trillium(integer)
*
**********************************************************************************/
int ftmod_ss7_disable_isup_circuit_by_interface_id(int isup_intf_idx, int span_id)
{
	int 	ret 	= 0;
	int 	intf_id = 0;
	int 	tmp_idx = 0;
	int 	idx 	= 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		intf_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id;
	}

	if (!intf_id) {
		SS7_ERROR("No configured ISUP Interface found at index %d!\n", isup_intf_idx);
		goto done;
	}

	idx = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);

	if (!idx) {
		idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	}

	/* check if the start circuit is null then it means that the corresponding span
	 * is already destroyed successfully and therefore start with this span circuit value */
	for (tmp_idx = 1; tmp_idx < MAX_ISUP_INFS; tmp_idx++) {
		if (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].infId == intf_id) {
			idx = g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].ckt_start_val;
		} else {
			continue;
		}

		if (!idx) {
			goto done;
		}

		SS7_DEBUG("Disabling ISUP Circuit mapped to span %d\n", g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id);
		while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
			if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].infId == intf_id) &&
			    (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id == g_ftdm_sngss7_data.cfg.isupCkt[idx].span)) {
				if (g_ftdm_sngss7_data.cfg.isupCkt[idx].type != SNG_CKT_SIG) {
					g_ftdm_sngss7_data.cfg.isupCkt[idx].flags |= SNGSS7_CKT_DISABLE;
					ret = ftmod_ss7_disable_isup_ckt(g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				}
			}
			idx++;
		}
	}

done:
	return ret;
}

/*********************************************************************************
*
* Desc: Disable all ISUP Circuits mapped to Span ID passed
*
* Ret : Return Value from trillium(integer)
*
**********************************************************************************/
int ftmod_ss7_disable_isup_circuit_by_span_id(int span_id, int cc_span_id)
{
	int idx = 0;
	int ret = 0;

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);

	if (!idx) {
		return ret;
	}

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].span == span_id) &&
			(g_ftdm_sngss7_data.cfg.isupCkt[idx].type != SNG_CKT_SIG) &&
			(g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id)) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].flags |= SNGSS7_CKT_DISABLE;
			ret = ftmod_ss7_disable_isup_ckt(g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
		}
		idx++;
	}

	return ret;
}

/*********************************************************************************
*
* Desc: Delete all ISUP Circuits mapped to ISUP Interface Passed
*
* Ret : Return Value from trillium(integer)
*
**********************************************************************************/
int ftmod_ss7_delete_isup_circuit_by_interface_id(int isup_intf_idx, int span_id)
{
	ftdm_status_t 	ret 	= FTDM_FAIL;
	int 		intf_id = 0;
	int 		tmp_idx = 0;
	int 		idx 	= 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		intf_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id;
	}

	if (!intf_id) {
		SS7_ERROR("No configured ISUP Interface found at index %d!\n", isup_intf_idx);
		goto done;
	}

	idx = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);

	if (!idx) {
		idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	}

	/* check if the start circuit is null then it means that the corresponding span
	 * is already destroyed successfully and therefore start with this span circuit value */
	for (tmp_idx = 1; tmp_idx < MAX_ISUP_INFS; tmp_idx++) {
		if (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].infId == intf_id) {
			idx = g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].ckt_start_val;
		} else {
			continue;
		}

		if (!idx) {
			goto done;
		}

		SS7_DEBUG("Deleting ISUP Circuits mapped to span %d\n", g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id);
		while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
			if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].infId == intf_id) &&
			    (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id == g_ftdm_sngss7_data.cfg.isupCkt[idx].span)) {
				ret = ftmod_ss7_delete_isup_ckt(g_ftdm_sngss7_data.cfg.isupCkt[idx].id);

				/* deleting number of circuit configured from license file */
				sng_del_cfg(STICIR);
			}
			idx++;
		}
	}

done:
	return ret;
}

/*********************************************************************************
*
* Desc: Delete all ISUP Circuits mapped to Span ID passed
*
* Ret : Return Value from trillium(integer)
*
**********************************************************************************/
int ftmod_ss7_delete_isup_circuit_by_span_id(int span_id, int cc_span_id)
{
	int idx = 0;
	int ret = 0;

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);

	if (!idx) {
		goto done;
	}

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].span == span_id) &&
			(g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id)) {
			ret = ftmod_ss7_delete_isup_ckt(g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
			SS7_DEBUG("Deleting ISUP Circuit %d mapped to span %d... ret[%d]\n",
				  g_ftdm_sngss7_data.cfg.isupCkt[idx].id,
				  span_id, ret);

			/* deleting number of circuit configured from license file */
			sng_del_cfg(STICIR);
		} else if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].span == 0) &&
				(g_ftdm_sngss7_data.cfg.isupCkt[idx].chan) &&
				(g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id)) {
			ret = ftmod_ss7_delete_isup_ckt(g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
			SS7_DEBUG("Deleting ISUP Circuit %d mapped to span %d... ret[%d]\n",
				  g_ftdm_sngss7_data.cfg.isupCkt[idx].id,
				  span_id, ret);

			/* deleting number of circuit configured from license file */
			sng_del_cfg(STICIR);
		}
		idx++;
	}

done:
	return ret;
}

/*********************************************************************************
*
* Desc: Send BLOCK on all circuits mapped to Span & CC Span ID passed
*
* Ret : VOID
*
**********************************************************************************/
void ftmod_ss7_send_circuit_block(int span_id, int cc_span_id)
{
	sngss7_chan_data_t *ss7_info = NULL;
	ftdm_channel_t 	   *ftdmchan = NULL;
	int 		   idx 	     = 0;

	idx = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);

	if (!idx) {
		idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	}

	/* check if the start circuit is null then it means that the corresponding span
	 * is already destroyed successfully and therefore start with this span circuit value */
	if (g_ftdm_sngss7_data.cfg.isupCkt[idx].id == 0) {
		idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);
	}

	if (!idx) {
		return;
	}

	SS7_DEBUG("Sending BLO for all voice circuits configured for span %d\n", span_id);

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id) &&
			(g_ftdm_sngss7_data.cfg.isupCkt[idx].type == SNG_CKT_VOICE)) {
			if (g_ftdm_sngss7_data.cfg.isupCkt[idx].blo_on_ckt_delete) {
				/* block circuit and inform the same to it peer */
				ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[idx].obj;
				ftdm_assert(ss7_info != NULL, "No ss7 info found!\n");

				ftdmchan = ss7_info->ftdmchan;
				ftdm_assert(ftdmchan != NULL, "No ftdmchan found!\n");

				ftdm_mutex_lock(ftdmchan->mutex);

				SS7_DEBUG_CHAN(ftdmchan, "Sending BLO... %s\n", " ");
				/* check if there is a pending state change|give it a bit to clear */
				if (check_for_state_change(ftdmchan)) {
					SS7_ERROR("Failed to wait for pending state change on CIC = %d\n", ss7_info->circuit->cic);
					ftdm_assert(0, "State change not completed\n");
					ftdm_mutex_unlock(ftdmchan->mutex);
					idx++;

					continue;
				} else {
					sngss7_set_ckt_blk_flag(ss7_info, FLAG_CKT_MN_BLOCK_TX);
					ftdm_set_state(ftdmchan, FTDM_CHANNEL_STATE_SUSPENDED);
				}

				ftdm_mutex_unlock(ftdmchan->mutex);
			}
		}
		idx++;
	}
}

/*********************************************************************************
*
* Desc: Clear all isup circuit related timers if already running depending upon
* 	the ISUP Interface ID passed
*
* Ret : FTDM_FAIL | FTDM_STATUS
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_clear_isup_circuit_timers_by_interface_id(int isup_intf_idx, int span_id)
{
	sngss7_chan_data_t *ss7_info = NULL;
	ftdm_channel_t 	   *ftdmchan = NULL;
	int 		   tmp_idx   = 0;
	int 		   intf_id   = 0;
	int 		   idx 	     = 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		intf_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id;
	}

	if (!intf_id) {
		SS7_ERROR("No configured ISUP Interface found at index %d!\n", isup_intf_idx);
		goto done;
	}

	idx = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);

	if (!idx) {
		idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	}

	/* check if the start circuit is null then it means that the corresponding span
	 * is already destroyed successfully and therefore start with this span circuit value */
	for (tmp_idx = 1; tmp_idx < MAX_ISUP_INFS; tmp_idx++) {
		if (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].infId == intf_id) {
			idx = g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].ckt_start_val;
		} else {
			continue;
		}

		if (!idx) {
			goto done;
		}

		SS7_DEBUG("Clearing all ISUP Circuit timers mapped to span %d\n", g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id);
		while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
			if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].infId == intf_id) &&
			    (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id == g_ftdm_sngss7_data.cfg.isupCkt[idx].span) &&
			    (g_ftdm_sngss7_data.cfg.isupCkt[idx].type != SNG_CKT_SIG)) {

				ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[idx].obj;

				if (!ss7_info) {
					ftdm_log(FTDM_LOG_WARNING, "No SS7 found for timer for span with Cricuit ID %d!\n",
							g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					idx++;

					continue;
				}

				if (!ss7_info->ftdmchan) {
					SS7_ERROR("Invalid ss7 info for span and circuit %d!\n", g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					break;
				}

				ftdmchan = ss7_info->ftdmchan;

				/* stop T35 timer */
				if (ss7_info->t35.hb_timer_id) {
					if (ftdm_sched_cancel_timer (ss7_info->t35.sched, ss7_info->t35.hb_timer_id)) {
						SS7_DEBUG("Unable to Stop t35 timer for span Circuit %d!\n", g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t35.hb_timer_id = 0;
					}
				}

				/* stop T10 timer */
				if (ss7_info->t10.hb_timer_id) {
					if (ftdm_sched_cancel_timer (ss7_info->t10.sched, ss7_info->t10.hb_timer_id)) {
						SS7_DEBUG("Unable to Stop t10 timer for span Circuit %d!\n", g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t10.hb_timer_id = 0;
					}
				}

				/* stop T39 timer */
				if (ss7_info->t39.hb_timer_id) {
					if (ftdm_sched_cancel_timer (ss7_info->t39.sched, ss7_info->t39.hb_timer_id)) {
						SS7_DEBUG("Unable to Stop t39 timer for span Circuit %d!\n", g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t39.hb_timer_id = 0;
					}
				}

				/* stop BLO retransmission timer */
				if (ss7_info->t_waiting_bla.hb_timer_id) {
					if (ftdm_sched_cancel_timer (ss7_info->t_waiting_bla.sched, ss7_info->t_waiting_bla.hb_timer_id)) {
						SS7_DEBUG("Unable to Stop BLO re-transmission timer for span Circuit %d!\n", g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t_waiting_bla.hb_timer_id = 0;
					}
				}

				/* stop UBL retransmission timer */
				if (ss7_info->t_waiting_uba.hb_timer_id) {
					if (ftdm_sched_cancel_timer (ss7_info->t_waiting_uba.sched, ss7_info->t_waiting_uba.hb_timer_id)) {
						SS7_DEBUG("Unable to Stop UBL re-transmission timer for span Circuit %d!\n", g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t_waiting_uba.hb_timer_id = 0;
					}
				}

				/* stop UBL transmission after 5 seconds from receiving BLA timer */
				if (ss7_info->t_tx_ubl_on_rx_bla.hb_timer_id) {
					if (ftdm_sched_cancel_timer (ss7_info->t_tx_ubl_on_rx_bla.sched, ss7_info->t_tx_ubl_on_rx_bla.hb_timer_id)) {
						SS7_DEBUG("Unable to Stop UBL transmission after 5 seconds from receiving BLA for span Circuit %d!\n",
								g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t_tx_ubl_on_rx_bla.hb_timer_id = 0;
					}
				}

				/* stop RSC retransmission timer */
				if (ss7_info->t_waiting_rsca.hb_timer_id) {

					if (ftdm_sched_cancel_timer (ss7_info->t_waiting_rsca.sched, ss7_info->t_waiting_rsca.hb_timer_id)) {

						SS7_DEBUG("Unable to Cancel RSC re-transmission timer for span %d Circuit %d!\n",
								(int)ss7_info->t_waiting_rsca.hb_timer_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t_waiting_rsca.hb_timer_id = 0;
					}
				}

				/* stop timer for block UBL transmission after BLO */
				if (ss7_info->t_block_ubl.hb_timer_id) {
					if (ftdm_sched_cancel_timer (ss7_info->t_block_ubl.sched, ss7_info->t_block_ubl.hb_timer_id)) {
						SS7_DEBUG("Unable to Stop Block UBL timer for span Circuit %d!\n", g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
					} else {
						ss7_info->t_block_ubl.hb_timer_id = 0;
					}
				}

				memset(ss7_info, 0x00, sizeof(sngss7_chan_data_t));
				ftdm_free(ss7_info);

				g_ftdm_sngss7_data.cfg.isupCkt[idx].obj = NULL;
				ftdmchan->call_data = NULL;
			}
			idx++;
		}
	}

done:
	return FTDM_SUCCESS;

}

/*********************************************************************************
*
* Desc: Clear all isup circuit related timers if already running depending upon
* 	the SPAN/CC Span ID passed
*
* Ret : FTDM_FAIL | FTDM_STATUS
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_clear_isup_circuit_timers_by_span_id(int span_id, int cc_span_id)
{
	sngss7_chan_data_t *ss7_info = NULL;
	ftdm_channel_t *ftdmchan     = NULL;
	int idx 		     = 1;

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);

	if (!idx) {
		return FTDM_FAIL;
	}

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id) &&
			(g_ftdm_sngss7_data.cfg.isupCkt[idx].type != SNG_CKT_SIG)) {

			ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[idx].obj;

			if (!ss7_info) {
				ftdm_log(FTDM_LOG_WARNING, "No SS7 found for timer for span %d with Cricuit ID %d!\n",
					 span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				idx++;
				continue;
			}

			if (!ss7_info->ftdmchan) {
				SS7_ERROR("Invalid ss7 info for span %d and circuit %d!\n", span_id,
					  g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				break;
			}

			ftdmchan = ss7_info->ftdmchan;

			/* stop T35 timer */
			if (ss7_info->t35.hb_timer_id) {
				if (ftdm_sched_cancel_timer (ss7_info->t35.sched, ss7_info->t35.hb_timer_id)) {
					SS7_DEBUG("Unable to Stop t35 timer for span %d Circuit %d!\n",
						  span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t35.hb_timer_id = 0;
				}
			}

			/* stop T10 timer */
			if (ss7_info->t10.hb_timer_id) {
				if (ftdm_sched_cancel_timer (ss7_info->t10.sched, ss7_info->t10.hb_timer_id)) {
					SS7_DEBUG("Unable to Stop t10 timer for span %d Circuit %d!\n",
						  span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t10.hb_timer_id = 0;
				}
			}

			/* stop T39 timer */
			if (ss7_info->t39.hb_timer_id) {
				if (ftdm_sched_cancel_timer (ss7_info->t39.sched, ss7_info->t39.hb_timer_id)) {
					SS7_DEBUG("Unable to Stop t39 timer for span %d Circuit %d!\n",
						  span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t39.hb_timer_id = 0;
				}
			}

			/* stop BLO retransmission timer */
			if (ss7_info->t_waiting_bla.hb_timer_id) {
				if (ftdm_sched_cancel_timer (ss7_info->t_waiting_bla.sched, ss7_info->t_waiting_bla.hb_timer_id)) {
					SS7_DEBUG("Unable to Stop BLO re-transmission timer for span %d Circuit %d!\n",
						  span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t_waiting_bla.hb_timer_id = 0;
				}
			}

			/* stop UBL retransmission timer */
			if (ss7_info->t_waiting_uba.hb_timer_id) {
				if (ftdm_sched_cancel_timer (ss7_info->t_waiting_uba.sched, ss7_info->t_waiting_uba.hb_timer_id)) {
					SS7_DEBUG("Unable to Stop UBL re-transmission timer for span %d Circuit %d!\n",
						  span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t_waiting_uba.hb_timer_id = 0;
				}
			}

			/* stop UBL transmission after 5 seconds from receiving BLA timer */
			if (ss7_info->t_tx_ubl_on_rx_bla.hb_timer_id) {
				if (ftdm_sched_cancel_timer (ss7_info->t_tx_ubl_on_rx_bla.sched, ss7_info->t_tx_ubl_on_rx_bla.hb_timer_id)) {
					SS7_DEBUG("Unable to Stop UBL transmission after 5 seconds from receiving BLA for span %d Circuit %d!\n",
						  span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t_tx_ubl_on_rx_bla.hb_timer_id = 0;
				}
			}

			/* stop RSC retransmission timer */
			if (ss7_info->t_waiting_rsca.hb_timer_id) {

				if (ftdm_sched_cancel_timer (ss7_info->t_waiting_rsca.sched, ss7_info->t_waiting_rsca.hb_timer_id)) {

					SS7_DEBUG("Unable to Cancel RSC re-transmission timer %d for span %d Circuit %d!\n",
						  (int)ss7_info->t_waiting_rsca.hb_timer_id, span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t_waiting_rsca.hb_timer_id = 0;
				}
			}

			/* stop timer for block UBL transmission after BLO */
			if (ss7_info->t_block_ubl.hb_timer_id) {
				if (ftdm_sched_cancel_timer (ss7_info->t_block_ubl.sched, ss7_info->t_block_ubl.hb_timer_id)) {
					SS7_DEBUG("Unable to Stop Block UBL timer for span %d Circuit %d!\n",
						  span_id, g_ftdm_sngss7_data.cfg.isupCkt[idx].id);
				} else {
					ss7_info->t_block_ubl.hb_timer_id = 0;
				}
			}

			memset(ss7_info, 0x00, sizeof(sngss7_chan_data_t));
			ftdm_free(ss7_info);

			g_ftdm_sngss7_data.cfg.isupCkt[idx].obj = NULL;
			ftdmchan->call_data = NULL;
		}
		idx++;
	}

	return FTDM_SUCCESS;
}

/*********************************************************************************
*
* Desc: Clear all isup circuits based on ISUP interface passed
*
* Ret : VOID
*
**********************************************************************************/
void ftmod_ss7_clear_all_isup_circuits(int isup_intf_idx, int span_id)
{
	int 			intf_id 	= 0;
	int 			tmp_idx 	= 0;
	int 			idx 		= 1;

	if ((g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id != 0) &&
		(g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].flags & SNGSS7_CONFIGURED)) {

		intf_id = g_ftdm_sngss7_data.cfg.isupIntf[isup_intf_idx].id;
	}

	if (!intf_id) {
		SS7_ERROR("No configured ISUP Interface found at index %d!\n", isup_intf_idx);
		goto done;
	}

	idx = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);

	if (!idx) {
		idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	}

	/* check if the start circuit is null then it means that the corresponding span
	 * is already destroyed successfully and therefore start with this span circuit value */
	for (tmp_idx = 1; tmp_idx < MAX_ISUP_INFS; tmp_idx++) {
		if (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].infId == intf_id) {
			idx = g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].ckt_start_val;
		} else {
			continue;
		}

		if (!idx) {
			goto done;
		}

		SS7_DEBUG("Clearing Circuit mapped to span %d\n", g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id);
		while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
			if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].infId == intf_id) &&
			    (g_ftdm_sngss7_data.cfg.isupCc[tmp_idx].span_id == g_ftdm_sngss7_data.cfg.isupCkt[idx].span)) {
#if 0
				ss7_info = NULL;
				ftdmchan = NULL;
				/* Clearing ss7_info if present */
				ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[idx].obj;

				if (ss7_info) {
					ftdmchan = ss7_info->ftdmchan;
					if (ftdmchan) {
						ftdm_safe_free(ss7_info);
						ftdmchan->call_data = NULL;
						SS7_DEBUG("[s%dc%d]Clearing ss7 info\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id, ftdmchan->chan_id);
					} else {
						ftdm_safe_free(ss7_info);
						SS7_DEBUG("[s%d]Clearing ss7 info\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id);
					}

					ss7_info = NULL;
				}
#endif
				memset(&g_ftdm_sngss7_data.cfg.isupCkt[idx], 0x00, sizeof(sng_isup_ckt_t));
				g_ftdm_sngss7_data.cfg.isupCkt[idx].flags = 0x00;
			}
			idx++;
		}
	}
done:
	return;
}

/*********************************************************************************
*
* Desc: Clear all isup circuits based on SPAN/CC Span ID passed
*
* Ret : VOID
*
**********************************************************************************/
void ftmod_ss7_clear_isup_circuits(int span_id, int cc_span_id)
{
	int 			idx 		= 0;

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);

	if (!idx) {
		return;
	}

	while (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) {
		if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].span == span_id) &&
		    (g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id)) {
#if 0
			ss7_info = NULL;
			ftdmchan = NULL;

			/* Clearing SS7 INFO if present */
			ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[idx].obj;

			if (ss7_info) {
				ftdmchan = ss7_info->ftdmchan;
				if (ftdmchan) {
					ftdm_safe_free(ftdmchan->call_data);
					ftdmchan->call_data = NULL;
					SS7_DEBUG("[s%dc%d]Clearing ss7 info\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id, ftdmchan->chan_id);
				} else {
					ftdm_safe_free(ss7_info);
					SS7_DEBUG("[s%d]Clearing ss7 info\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id);
				}

				ss7_info = NULL;
			}
#endif
			memset(&g_ftdm_sngss7_data.cfg.isupCkt[idx], 0x00, sizeof(sng_isup_ckt_t));
			g_ftdm_sngss7_data.cfg.isupCkt[idx].flags = 0x00;
		} else if ((g_ftdm_sngss7_data.cfg.isupCkt[idx].span == 0) &&
			   (g_ftdm_sngss7_data.cfg.isupCkt[idx].chan) &&
			   (g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId == cc_span_id)) {
#if 0
			ss7_info = NULL;
			ftdmchan = NULL;

			/* my changes */
			ss7_info = (sngss7_chan_data_t *)g_ftdm_sngss7_data.cfg.isupCkt[idx].obj;

			if (ss7_info) {
				ftdmchan = ss7_info->ftdmchan;
				if (ftdmchan) {
					ftdm_safe_free(ftdmchan->call_data);
					ftdmchan->call_data = NULL;
					SS7_DEBUG("[s%dc%d]Clearing ss7 info\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id, ftdmchan->chan_id);
				} else {
					ftdm_safe_free(ss7_info);
					SS7_DEBUG("[s%d]Clearing ss7 info\n", g_ftdm_sngss7_data.cfg.isupCc[idx].span_id);
				}

				ss7_info = NULL;
			}
#endif
			memset(&g_ftdm_sngss7_data.cfg.isupCkt[idx], 0x00, sizeof(sng_isup_ckt_t));
			g_ftdm_sngss7_data.cfg.isupCkt[idx].flags = 0x00;
		}
		idx++;
	}
}

/*********************************************************************************
*
* Desc: Clear CC Span related information from (g_ftdm_sngss7_data.cfg.isupCc)
* 	list
*
* Ret : VOID
*
**********************************************************************************/
void ftmod_ss7_clear_cc_span_info_by_span_id(int span_id)
{
	int idx = 1;

	while( idx < (MAX_ISUP_INFS)) {
		if (g_ftdm_sngss7_data.cfg.isupCc[idx].span_id == span_id) {
			SS7_DEBUG("Clearing CC span %d info at index %d from info list\n", g_ftdm_sngss7_data.cfg.isupCc[idx].cc_span_id, idx);
			memset(&g_ftdm_sngss7_data.cfg.isupCc[idx], 0x00, sizeof(sng_isup_cc_t));
			break;
		}
		idx++;
	}
}


/*********************************************************************************
				Reconfiguation Validation
*********************************************************************************/
/*********************************************************************************
*
* Desc: Validate reconfiguration changes if present any
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_reconfig_changes(int span_id, int span_list[])
{
	ftdm_status_t ret       = FTDM_FAIL;

	/* check if newly parsed mtp 1 configuration is valid or not */
	if ((ret = ftmod_ss7_validate_mtp1_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed mtp 2 configuration is valid or not */
	if ((ret = ftmod_ss7_validate_mtp2_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed mtp 3 configuration is valid or not */
	if ((ret = ftmod_ss7_validate_mtp3_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed linkset configuration is valid or not */
	if ((ret = ftmod_ss7_validate_linkset_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed route configuration is valid or not */
	if ((ret = ftmod_ss7_validate_route_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed self route configuration is valid or not */
	if ((ret = ftmod_ss7_validate_self_route_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed nsap configuration is valid or not */
	if ((ret = ftmod_ss7_validate_nsap_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed isap configuration is valid or not */
	if ((ret = ftmod_ss7_validate_isap_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed isup interface configuration is valid or not */
	if ((ret = ftmod_ss7_validate_isup_interface_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	/* check if newly parsed cc interface configuration is valid or not */
	if ((ret = ftmod_ss7_validate_cc_span_reconfig_changes(span_id, span_list)) != FTDM_SUCCESS) {
		goto end;
	}

	ret = FTDM_SUCCESS;

end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate MTP1 reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_mtp1_reconfig_changes(int span_id, int span_list[])
{
	sng_mtp1_link_t *mtp1Link = NULL;
	ftdm_bool_t reload        = FTDM_FALSE;
	ftdm_status_t ret         = FTDM_FAIL;
	int reconfig_val          = 0;
	int cfg_val               = 0;
	int idx                   = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp1Link[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].id != 0)) {

			reload = FTDM_FALSE;
			mtp1Link = &g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx];

			if (mtp1Link->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP1-%d configuration found\n", mtp1Link->id);
				} else if (mtp1Link->id != g_ftdm_sngss7_data.cfg.mtp1Link[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP1-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtp1Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* check span ID */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp1Link[idx].span;
					reconfig_val = mtp1Link->span;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP1-%d span Id from %d to %d\n", mtp1Link->id,
								cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check signalling channel ID */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp1Link[idx].chan;
					reconfig_val = mtp1Link->chan;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP1-%d signalling channel from %d to %d\n", mtp1Link->id,
								cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					if (reload == FTDM_TRUE) {
						ftmod_ss7_add_span_to_list(g_ftdm_sngss7_data.cfg.mtp1Link[idx].span, span_list);

						/* check if this is for same span_id */
						if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].span == span_id) {
							g_ftdm_sngss7_data.cfg.reload = FTDM_TRUE;
						} else {
							reload = FTDM_FALSE;
						}
					}

					if (g_ftdm_sngss7_data.cfg.mtp1Link[idx].span == span_id) {
						mtp1Link->reload = reload;
					} else {
						mtp1Link->reload = FTDM_FALSE;
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = mtp1Link->reload;
					}
				}
			}
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate MTP2 reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_mtp2_reconfig_changes(int span_id, int span_list[])
{
	ftdm_bool_t reload        = FTDM_FALSE;
	ftdm_status_t ret         = FTDM_FAIL;
	sng_mtp2_link_t *mtp2Link = NULL;
	int reconfig_val          = 0;
	int cfg_val               = 0;
	int span_no 		  = 0;
	int idx                   = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp2Link[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx].id != 0)) {

			reload = FTDM_FALSE;
			mtp2Link = &g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx];

			if (mtp2Link->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtp2Link[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP2-%d configuration found\n", mtp2Link->id);
				} else if (mtp2Link->id != g_ftdm_sngss7_data.cfg.mtp2Link[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP2-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtp2Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* check if the configuration is w.r.t this span only */
					span_no = ftmod_get_span_id_by_mtp2_id(idx);
					if (!span_no) {
						ret = FTDM_FAIL;
						goto end;
					}

					/* check Lssu Length */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp2Link[idx].lssuLength;
					reconfig_val = mtp2Link->lssuLength;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP2-%d LSSU length from %d to %d\n", mtp2Link->id,
								cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check error Type */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp2Link[idx].errorType;
					reconfig_val = mtp2Link->errorType;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP2-%d error Type is not allowed from %d to %d!\n", mtp2Link->id,
								cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check switch Type */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp2Link[idx].linkType;
					reconfig_val = mtp2Link->linkType;

					if (cfg_val != reconfig_val) {
						ftmod_ss7_add_span_to_list(span_no, span_list);

						if (span_no == span_id) {
							mtp2Link->disable_sap = FTDM_TRUE;

							SS7_INFO("[Reload]: Reconfiguring MTP2-%d switchType from %d to %d. Thus, stopping SS7 span %d\n",
								 mtp2Link->id, cfg_val, reconfig_val, span_id);
							idx++;

							continue;
						}
					}

					/* check sap Id */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp2Link[idx].sapType;
					reconfig_val = mtp2Link->sapType;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP2-%d sap Id is not allowed from %d to %d!\n", mtp2Link->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check mtp1Id */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp2Link[idx].mtp1Id;
					reconfig_val = mtp2Link->mtp1Id;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP2-%d MTP1 Id is not allowed from %d to %d!\n",
							  mtp2Link->id, cfg_val, reconfig_val);
						reload = FTDM_FAIL;
						goto end;
					}

					/* check mtp1ProcId */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp2Link[idx].mtp1ProcId;
					reconfig_val = mtp2Link->mtp1ProcId;

					/* NOTE: Trillium allow this value to get change but to make customer not to play with this value
					 * we are disabling it */
					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP2-%d MTP1 proc Id is not supported from %d to %d!\n", mtp2Link->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					if (span_no == span_id) {
						mtp2Link->reload = reload;
					} else {
						mtp2Link->reload = FTDM_FALSE;
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = mtp2Link->reload;
					}
				}
			}
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

int ftmod_ss7_is_new_link_for_existing_linkset(sng_mtp3_link_t *mtp3Link)
{
	int ret = 0;
	int idx = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0)) {
			if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId == mtp3Link->linkSetId) {
				/* get span mapped to this mtp3 */
				ret = ftmod_ss7_get_span_id_by_mtp3_id(idx);
				break;
			}
		}
		idx++;
	}

	return ret;
}

/*********************************************************************************
*
* Desc: Validate MTP3 reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_mtp3_reconfig_changes(int span_id, int span_list[])
{
	ftdm_bool_t reload        = FTDM_FALSE;
	ftdm_status_t ret         = FTDM_FAIL;
	sng_mtp3_link_t *mtp3Link = NULL;
	int reconfig_val          = 0;
	int nsap_cfg_idx          = 0;
	int span_no 		  = 0;
	int cfg_val               = 0;
	int idx                   = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKS)) {
		if ((g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx].id != 0)) {

			reload = FTDM_FALSE;
			mtp3Link = &g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx];

			if (mtp3Link->id != 0) {
				/* check if this is a new MTP3 link configured */
				if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP3-%d configuration found\n", mtp3Link->id);

					/* check if this configuration is with respect to already running linkset and route */
					span_no = ftmod_ss7_is_new_link_for_existing_linkset(mtp3Link);

					/* There are condition when a new span need to be included
					 * that is mapped to already exisiting linkset with same
					 * OPC and DPC in such cases we need to stop already running
					 * as this configuration is for both of them
					 * Stopping already running SS7 span is required in order to
					 * reconfig NSAP and DLSAP for respective switch and link Type
					 * and ssf value */
					if ((span_no) && (span_no != span_id)) {
						ftmod_ss7_add_span_to_list(span_no, span_list);
						idx++;

						continue;
					}
				} else if (mtp3Link->id != g_ftdm_sngss7_data.cfg.mtp3Link[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP3-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* get span mapped to this mtp3 */
					span_no = ftmod_ss7_get_span_id_by_mtp3_id(idx);
					if (!span_no) {
						ret = FTDM_FAIL;
						goto end;
					}

					/* get nsap index mapped to this mtp3 link */
					nsap_cfg_idx = ftmod_ss7_get_nsap_id_by_mtp3_id(idx);

					/* check lnkType */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkType;
					reconfig_val = mtp3Link->linkType;

					/* NOTE: If in case switchType/lnkType/ssf si reconfigured then please
					 * disable NSAP and DLSAP in order to avoid any inconsistency */
					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkType != mtp3Link->linkType) {
						SS7_DEBUG("[Reload]: Reconfigure MTP3-%d Link Type from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);

						mtp3Link->disable_sap = FTDM_TRUE;
						reload = FTDM_TRUE;
					}

					/* check switch Type */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].switchType;
					reconfig_val = mtp3Link->switchType;

					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].switchType != mtp3Link->switchType) {
						SS7_DEBUG("[Reload]: Reconfigure MTP3-%d Switch Type from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);
						if (mtp3Link->disable_sap != FTDM_TRUE) {
							mtp3Link->disable_sap = FTDM_TRUE;
						}
						reload = FTDM_TRUE;
					}

					/* check apc */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].apc;
					reconfig_val = mtp3Link->apc;

					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].apc != mtp3Link->apc) {
						ftmod_ss7_add_span_to_list(span_no, span_list);
						/* check if this MTP3 is for configured for same span ID */
						if (span_id == span_no) {
							SS7_INFO("[Reload]: Reconfiguring MTP3-%d adjacent DPC from %d to %d. Thus, Stopping SS7 to apply this config!\n",
								 mtp3Link->id, cfg_val, reconfig_val);
							idx++;

							continue;
						}
					}

					/* check spc */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].spc;
					reconfig_val = mtp3Link->spc;

					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].spc != mtp3Link->spc) {
						SS7_DEBUG("[Reload]: Reconfigure MTP3-%d SPC from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check ssf */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].ssf;
					reconfig_val = mtp3Link->ssf;

					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].ssf != mtp3Link->ssf) {
						SS7_DEBUG("[Reload]: Reconfigure MTP3-%d SSF from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);

						if (mtp3Link->disable_sap != FTDM_TRUE) {
							mtp3Link->disable_sap = FTDM_TRUE;
						}
						reload = FTDM_TRUE;
					}

					/* check slc */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].slc;
					reconfig_val = mtp3Link->slc;

					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].slc != mtp3Link->slc) {
						SS7_DEBUG("[Reload]: Reconfigure MTP3-%d SLC from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check link set ID */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId;
					reconfig_val = mtp3Link->linkSetId;

					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].linkSetId != mtp3Link->linkSetId) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP3-%d Linkset Id is not allowed from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check associated mtp2Id */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].mtp2Id;
					reconfig_val = mtp3Link->mtp2Id;

					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].mtp2Id != mtp3Link->mtp2Id) {
						SS7_ERROR("[Reload]: Reconfiguring MTP3-%d MTP2 ID from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check mtp2 proc ID */
					cfg_val = g_ftdm_sngss7_data.cfg.mtp3Link[idx].mtp2ProcId;
					reconfig_val = mtp3Link->mtp2ProcId;

					/* NOTE: Trillium allow this value to get change but to make customer not to play with this value
					 * we are disabling it */
					if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].mtp2ProcId != mtp3Link->mtp2ProcId) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP3-%d MTP2 Proc ID is not allowed from %d to %d\n", mtp3Link->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					if (span_no == span_id) {
						mtp3Link->reload = reload;
					} else {
						mtp3Link->reload = FTDM_FALSE;
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = mtp3Link->reload;
					}
				}
			}
		}
		idx++;
	}
	ret = FTDM_SUCCESS;
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate MTP Linkset reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_linkset_reconfig_changes(int span_id, int span_list[])
{
	sng_link_set_t *mtpLinkSet = NULL;
	ftdm_bool_t reload         = FTDM_FALSE;
	ftdm_status_t ret          = FTDM_FAIL;
	int reconfig_val           = 0;
	int span_no[100]	   = { 0 };
	int span_idx 		   = 0;
	int cfg_val                = 0;
	int idx                    = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_MTP_LINKSETS+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx].id != 0)) {

			reload = FTDM_FALSE;
			mtpLinkSet = &g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx];

			if (mtpLinkSet->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP Linkset-%d configuration found\n", mtpLinkSet->id);
				} else if (mtpLinkSet->id != g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP Linkset-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* reseting all spans */
					memset(span_no, 0 , sizeof(span_no));

					/* get all span mapped to this linkset */
					ftmod_ss7_get_span_id_by_linkset_id(idx, span_no);
					if (!span_no[0]) {
						ret = FTDM_FAIL;
						goto end;
					}

					/* check adjacent point code */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].apc;
					reconfig_val = mtpLinkSet->apc;

					if (cfg_val != reconfig_val) {
						for (span_idx = 0; span_idx < 100; span_idx++) {
							if (span_no[span_idx]) {
								ftmod_ss7_add_span_to_list(span_no[span_idx], span_list);
							}

							if (span_no[span_idx] == span_id) {
								SS7_INFO("[Reload]: Reconfiguring MTP Linkset-%d Adjacent point code from %d to %d. Thus, stopping SS7 to apply this config!\n",
									 mtpLinkSet->id, cfg_val, reconfig_val);
								continue;
							}
						}
					}

					/* check Number of links */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].numLinks;
					reconfig_val = mtpLinkSet->numLinks;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP Linkset-%d Number of links from %d to %d is not allowed!\n", mtpLinkSet->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check minimum number of active links */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpLinkSet[idx].minActive;
					reconfig_val = mtpLinkSet->minActive;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP Linkset-%d Minimum number of active links from %d to %d\n", mtpLinkSet->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					for (span_idx = 0; span_idx < 100; span_idx++) {
						if (span_no[span_idx] == span_id) {
							mtpLinkSet->reload = reload;
							break;
						} else {
							mtpLinkSet->reload = FTDM_FALSE;
						}
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = mtpLinkSet->reload;
					}
				}
			}
		}
		idx++;
	}
	ret = FTDM_SUCCESS;
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate MTP Route reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_route_reconfig_changes(int span_id, int span_list[])
{
	sng_route_t *mtp3_route         = NULL;
	ftdm_bool_t reload              = FTDM_FALSE;
	ftdm_bool_t span_found          = FTDM_FALSE;
	ftdm_status_t ret               = FTDM_FAIL;
	int reconfig_val                = 0;
	int span_no[100] 		= { 0 };
	int span_idx 			= 0;
	int cfg_val                     = 0;
	int idx                         = 1;

	/* check if mtp self routes configuration is valid or not */
	while (idx < (MAX_MTP_ROUTES+1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0)) {

			reload = FTDM_FALSE;
			mtp3_route = &g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx];

			if (mtp3_route->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtpRoute[idx].id == 0) {
					SS7_DEBUG("[Reload]: New MTP Route-%d configuration found\n", mtp3_route->id);
				} else if (mtp3_route->id != g_ftdm_sngss7_data.cfg.mtpRoute[idx].id) {
					SS7_ERROR("[Reload]: Invalid MTP Route-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtpRoute[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* reseting all spans */
					memset(span_no, 0 , sizeof(span_no));

					/* get all span mapped to this route */
					ftmod_ss7_get_span_id_by_route_id(idx, span_no);
					if (!span_no[0]) {
						ret = FTDM_FAIL;
						goto end;
					} else if (span_no[0] == NOT_CONFIGURED) {
						SS7_INFO("[Reload]: Circuits are not been configured. Please take this as a configuration request for span %d!\n",
							 span_id);
						ret = FTDM_FAIL;
						span_list[99] = NOT_CONFIGURED;
						idx++;
						continue;
					}

					/* check destination point code */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].dpc;
					reconfig_val = mtp3_route->dpc;

					if (cfg_val != reconfig_val) {
						for (span_idx = 0; span_idx < 100; span_idx++) {
							if (span_no[span_idx]) {
								ftmod_ss7_add_span_to_list(span_no[span_idx], span_list);
							}
							if (span_no[span_idx] == span_id) {
								SS7_INFO("[Reload]: Reconfiguring MTP Route-%d DPC from %d to %d. Thus, stopping SS7 to apply this config!\n",
									 mtp3_route->id, cfg_val, reconfig_val);
								continue;
							}
						}
					}

					/* check Number of link Type */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].linkType;
					reconfig_val = mtp3_route->linkType;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP Route-%d Link Type from %d to %d\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check switch Type */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType;
					reconfig_val = mtp3_route->switchType;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP Route-%d Switch Type from %d to %d\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check Combined LinkSet */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].cmbLinkSetId;
					reconfig_val = mtp3_route->cmbLinkSetId;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP Route-%d Combined Linkset from %d to %d is not allowed!\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check ssf */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf;
					reconfig_val = mtp3_route->ssf;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP Route-%d SSF from %d to %d\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check Link Set Selection */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].lsetSel;
					reconfig_val = mtp3_route->lsetSel;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure MTP Route-%d Link Set Selection from %d to %d\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check SLS Link */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].slsLnk;
					reconfig_val = mtp3_route->slsLnk;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of MTP Route-%d SLS link from %d to %d is not supported yet!\n", mtp3_route->id,
								cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					for (span_idx = 0; span_idx < 100; span_idx++) {
						if (span_no[span_idx] == span_id) {
							span_found = FTDM_TRUE;
							mtp3_route->reload = reload;
							break;
						} else {
							mtp3_route->reload = FTDM_FALSE;
						}
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = mtp3_route->reload;
					}
				}
			}
		}
		idx++;
	}

	if ((span_found != FTDM_TRUE) && (span_list[99] == NOT_CONFIGURED)) {
	       ret = FTDM_BREAK;
	} else {
		ret = FTDM_SUCCESS;
	}
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate MTP Self Route reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_self_route_reconfig_changes(int span_id, int span_list[])
{
	sng_route_t *mtp3_route         = NULL;
	ftdm_bool_t span_found          = FTDM_FALSE;
	ftdm_bool_t reload              = FTDM_FALSE;
	ftdm_status_t ret               = FTDM_FAIL;
	int reconfig_val                = 0;
	int cfg_val                     = 0;
	int span_no[100]		= { 0 };
	int span_idx 			= 0;
	int idx                         = MAX_MTP_ROUTES + 1;

	/* check if mtp routes configuration is valid or not */
	while (idx < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1)) {
		if ((g_ftdm_sngss7_data.cfg.mtpRoute[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0)) {

			reload = FTDM_FALSE;
			mtp3_route = &g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx];

			if (mtp3_route->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.mtpRoute[idx].id == 0) {
					SS7_DEBUG("[Reload]: New Self MTP Route-%d configuration found\n", mtp3_route->id);
				} else if (mtp3_route->id != g_ftdm_sngss7_data.cfg.mtpRoute[idx].id) {
					SS7_ERROR("[Reload]: Invalid Self MTP Route-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.mtpRoute[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* reseting all spans */
					memset(span_no, 0 , sizeof(span_no));

					/* get the span to which this configuration belong to */
					ftmod_ss7_get_span_id_by_mtp_self_route(idx, span_no);

					if (!span_no[0]) {
						ret = FTDM_FAIL;
						goto end;
					} else if (span_no[0] == NOT_CONFIGURED) {
						SS7_INFO("[Reload]: Circuits are not been configured. Please take this as a configuration request for span %d!\n",
							 span_id);
						span_list[99] = NOT_CONFIGURED;
						idx++;
						continue;
					}

					/* check destination point code */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].dpc;
					reconfig_val = mtp3_route->dpc;

					if (cfg_val != reconfig_val) {
						for (span_idx = 0; span_idx < 100; span_idx++) {
							if (span_no[span_idx]) {
								ftmod_ss7_add_span_to_list(span_no[span_idx], span_list);
							}
							if (span_no[span_idx] == span_id) {
								SS7_INFO("[Reload]: Reconfiguring of Self MTP Route-%d DPC from %d to %d. Thus, stopping SS7 to apply this config!\n",
									 mtp3_route->id, cfg_val, reconfig_val);
								continue;
							}
						}
					}

					/* check Number of link Type */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].linkType;
					reconfig_val = mtp3_route->linkType;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure Self MTP Route-%d Link Type from %d to %d\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check switch Type */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].switchType;
					reconfig_val = mtp3_route->switchType;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure Self MTP Route-%d Switch Type from %d to %d\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check Combined LinkSet */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].cmbLinkSetId;
					reconfig_val = mtp3_route->cmbLinkSetId;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of Self MTP Route-%d Combined Linkset from %d to %d is not allowed!\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check ssf */
					cfg_val = g_ftdm_sngss7_data.cfg.mtpRoute[idx].ssf;
					reconfig_val = mtp3_route->ssf;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure Self MTP Route-%d SSF from %d to %d\n", mtp3_route->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					for (span_idx = 0; span_idx < 100; span_idx++) {
						if (span_no[span_idx] == span_id) {
							span_found = FTDM_TRUE;
							mtp3_route->reload = reload;
							break;
						} else {
							mtp3_route->reload = FTDM_FALSE;
						}
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = mtp3_route->reload;
					}
				}
			}
		}
		idx++;
	}

	if ((span_found != FTDM_TRUE) && (span_list[99] == NOT_CONFIGURED)) {
		ret = FTDM_BREAK;
	} else {
		ret = FTDM_SUCCESS;
	}
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate NSAP reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_nsap_reconfig_changes(int span_id, int span_list[])
{
	ftdm_bool_t span_found  = FTDM_FALSE;
	ftdm_bool_t reload      = FTDM_FALSE;
	ftdm_status_t ret       = FTDM_FAIL;
	sng_nsap_t *nsap        = NULL;
	int reconfig_val        = 0;
	int cfg_val             = 0;
	int span_no[100]	= { 0 };
	int span_idx 		= 0;
	int idx                 = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_NSAPS)) {
		if ((g_ftdm_sngss7_data.cfg.nsap[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.nsap[idx].id != 0)) {

			reload = FTDM_FALSE;
			nsap = &g_ftdm_sngss7_data.cfg_reload.nsap[idx];

			if (nsap->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.nsap[idx].id == 0) {
					SS7_DEBUG("[Reload]: New NSAP-%d configuration found\n", nsap->id);
				} else if (nsap->id != g_ftdm_sngss7_data.cfg.nsap[idx].id) {
					SS7_ERROR("[Reload]: Invalid NSAP-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.nsap[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* reseting all spans */
					memset(span_no, 0 , sizeof(span_no));

					/* get span id to which this NSAP configuration belongs to */
					ftmod_ss7_get_span_id_by_nsap_id(idx, span_no);

					if (!span_no[0]) {
						ret = FTDM_FAIL;
						goto end;
					} else if (span_no[0] == NOT_CONFIGURED) {
						SS7_INFO("[Reload]: Circuits are not been configured. Please take this as a configuration request for span %d!\n",
							 span_id);
						span_list[99] = NOT_CONFIGURED;
						idx++;
						continue;
					}

					/* NSAP need to be unbind if in case nwID or SSF value are different */
					/* check spId */
					cfg_val = g_ftdm_sngss7_data.cfg.nsap[idx].spId;
					reconfig_val = nsap->spId;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of NSAP-%d ID from %d to %d is not allowed!\n", nsap->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check suId */
					cfg_val = g_ftdm_sngss7_data.cfg.nsap[idx].suId;
					reconfig_val = nsap->suId;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of NSAP-%d SuId from %d to %d is not allowed!\n", nsap->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check network ID */
					cfg_val = g_ftdm_sngss7_data.cfg.nsap[idx].nwId;
					reconfig_val = nsap->nwId;

					if (cfg_val != reconfig_val) {
						nsap->disable_sap = FTDM_TRUE;
						SS7_DEBUG("[Reload]: Reconfigure NSAP-%d network ID from %d to %d\n", nsap->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check SSF */
					cfg_val = g_ftdm_sngss7_data.cfg.nsap[idx].ssf;
					reconfig_val = nsap->ssf;

					if (cfg_val != reconfig_val) {
						if (nsap->disable_sap != FTDM_TRUE) {
							nsap->disable_sap = FTDM_TRUE;
						}
						reload = FTDM_TRUE;
						SS7_DEBUG("[Reload]: Reconfigure NSAP-%d SSF from %d to %d\n", nsap->id,
							  cfg_val, reconfig_val);
					}

					for (span_idx = 0; span_idx < 100; span_idx++) {
						if (span_no[span_idx] == span_id) {
							span_found = FTDM_TRUE;
							nsap->reload = reload;
							break;
						} else {
							nsap->reload = FTDM_FALSE;
						}
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = nsap->reload;
					}
				}
			}
		}
		idx++;
	}

	if ((span_found != FTDM_TRUE) && (span_list[99] == NOT_CONFIGURED)) {
	       ret = FTDM_BREAK;
	} else {
		ret = FTDM_SUCCESS;
	}
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate ISAP reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_isap_reconfig_changes(int span_id, int span_list[])
{
	sng_isap_t *isap        = NULL;
	ftdm_bool_t span_found  = FTDM_FALSE;
	ftdm_bool_t reload      = FTDM_FALSE;
	ftdm_status_t ret       = FTDM_FAIL;
	int reconfig_val        = 0;
	int cfg_val             = 0;
	int span_no[100]	= { 0 };
	int span_idx 		= 0;
	int idx                 = 1;

	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_NSAPS)) {
		if ((g_ftdm_sngss7_data.cfg.isap[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.isap[idx].id != 0)) {

			reload = FTDM_FALSE;
			isap = &g_ftdm_sngss7_data.cfg_reload.isap[idx];

			if (isap->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.isap[idx].id == 0) {
					SS7_DEBUG("[Reload]: New NSAP-%d configuration found\n", isap->id);
				} else if (isap->id != g_ftdm_sngss7_data.cfg.isap[idx].id) {
					SS7_ERROR("[Reload]: Invalid NSAP-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.isap[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* reseting all spans */
					memset(span_no, 0 , sizeof(span_no));

					/* get span id to which this NSAP configuration belongs to */
					ftmod_ss7_get_span_id_by_isap_id(idx, span_no);

					if (!span_no[0]) {
						ret = FTDM_FAIL;
						goto end;
					} else if (span_no[0] == NOT_CONFIGURED) {
						SS7_INFO("[Reload]: Circuits are not been configured. Please take this as a configuration request for span %d!\n",
							 span_id);
						span_list[99] = NOT_CONFIGURED;
						idx++;
						continue;
					}

					/* ISAP need to be unbind if in case switchType or SSF is reconfigured */

					/* check suId */
					cfg_val = g_ftdm_sngss7_data.cfg.isap[idx].suId;
					reconfig_val = isap->suId;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of ISAP-%d SuId from %d to %d is not allowed!\n", isap->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check spId */
					cfg_val = g_ftdm_sngss7_data.cfg.isap[idx].spId;
					reconfig_val = isap->spId;

					if (cfg_val != reconfig_val) {
						SS7_ERROR("[Reload]: Reconfiguration of NSAP-%d ID from %d to %d is not allowed!\n", isap->id,
							  cfg_val, reconfig_val);
						ret = FTDM_FAIL;
						goto end;
					}

					/* check switch Type ID */
					cfg_val = g_ftdm_sngss7_data.cfg.isap[idx].switchType;
					reconfig_val = isap->switchType;

					if (cfg_val != reconfig_val) {
						isap->disable_sap = FTDM_TRUE;

						SS7_DEBUG("[Reload]: Reconfigure ISAP-%d Switch Type from %d to %d\n", isap->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					/* check SSF */
					cfg_val = g_ftdm_sngss7_data.cfg.isap[idx].ssf;
					reconfig_val = isap->ssf;

					if (cfg_val != reconfig_val) {
						if (isap->disable_sap != FTDM_TRUE) {
							isap->disable_sap = FTDM_TRUE;
						}
						reload = FTDM_TRUE;
						SS7_DEBUG("[Reload]: Reconfigure ISAP-%d SSF from %d to %d\n", isap->id,
							  cfg_val, reconfig_val);
					}

					/* check def Rel Location */
					cfg_val = g_ftdm_sngss7_data.cfg.isap[idx].defRelLocation;
					reconfig_val = isap->defRelLocation;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure ISAP-%d Dereferencing location from %d to %d\n", isap->id,
							  cfg_val, reconfig_val);
						reload = FTDM_TRUE;
					}

					for (span_idx = 0; span_idx < 100; span_idx++) {
						if (span_no[span_idx] == span_id) {
							span_found = FTDM_TRUE;
							isap->reload = reload;
							break;
						} else {
							isap->reload = FTDM_FALSE;
						}
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = isap->reload;
					}
				}
			}
		}
		idx++;
	}

	if ((span_found != FTDM_TRUE) && (span_list[99] == NOT_CONFIGURED)) {
		ret = FTDM_BREAK;
	} else {
		ret = FTDM_SUCCESS;
	}
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate ISUP Interface reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_isup_interface_reconfig_changes(int span_id, int span_list[])
{
	sng_span_type sp_type 	 = SNG_SPAN_INVALID;
	ftdm_bool_t invalid_conf = FTDM_FALSE;
	ftdm_bool_t span_found   = FTDM_FALSE;
	ftdm_bool_t reload       = FTDM_FALSE;
	ftdm_status_t ret        = FTDM_FAIL;
	sng_isup_inf_t *isupIntf = NULL;
	int reconfig_val         = 0;
	int span_no[100]	 = { 0 };
	int span_idx 		 = 0;
	int cfg_val              = 0;
	int idx                  = 1;

	if (!span_id) {
		SS7_ERROR("[Reload]: Invalid span %d isup interface configuration received!\n",
			  span_id);
		ret = FTDM_FAIL;
		goto end;
	}
	/* Please check if the span is a pure voice span then please
	 * donot configure if, its respective signalling is still down */
	while (idx < MAX_MTP_LINKS) {
		if (g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].id) {
			if (g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].span == span_id) {
				sp_type = SNG_SPAN_SIG;
				break;
			} else {
				sp_type = SNG_SPAN_VOICE;
			}
		}
		idx++;
	}

	idx = 1;
	/* check if mtp1 configuration is valid or not */
	while (idx < (MAX_ISUP_INFS)) {
		if ((g_ftdm_sngss7_data.cfg.isupIntf[idx].id != 0) ||
		    (g_ftdm_sngss7_data.cfg_reload.isupIntf[idx].id != 0)) {

			reload = FTDM_FALSE;
			isupIntf = &g_ftdm_sngss7_data.cfg_reload.isupIntf[idx];

			if (isupIntf->id != 0) {
				/* check if this is a new span configured */
				if (g_ftdm_sngss7_data.cfg.isupIntf[idx].id == 0) {
					if (sp_type == SNG_SPAN_VOICE) {
						invalid_conf = FTDM_TRUE;
					}

					SS7_DEBUG("[Reload]: New ISUP interface-%d configuration found\n", isupIntf->id);
				} else if (isupIntf->id != g_ftdm_sngss7_data.cfg.isupIntf[idx].id) {
					SS7_ERROR("[Reload]: Invalid ISUP Interface-%d ID reconfiguration request!\n", g_ftdm_sngss7_data.cfg.isupIntf[idx].id);
					ret = FTDM_FAIL;
					goto end;
				} else {
					/* reset span number */
					memset(span_no, 0 , sizeof(span_no));

					ftmod_ss7_get_span_id_by_isup_interface_id(idx, span_no);
					if (!span_no[0]) {
						SS7_ERROR("[Reload]: No span found mapped to ISUP Interface-%d!\n",
							  isupIntf->id);
						ret = FTDM_FAIL;
						goto end;
					} else if (span_no[0] == NOT_CONFIGURED) {
						SS7_INFO("[Reload]: Circuits are not been configured. Please take this as a configuration request for span %d!\n",
							 span_id);
						span_list[99] = NOT_CONFIGURED;
						idx++;
						continue;
					}

					/* Cannot proceed further if nwId, ssf, spc or switchType is different */
					/* check nwId */
					cfg_val = g_ftdm_sngss7_data.cfg.isupIntf[idx].nwId;
					reconfig_val = isupIntf->nwId;

					if (cfg_val != reconfig_val) {
						for (span_idx = 0; span_idx < 100; span_idx++) {
							if (span_no[span_idx]) {
								ftmod_ss7_add_span_to_list(span_no[span_idx], span_list);
							}
							if (span_no[span_idx] == span_id) {
								SS7_INFO("[Reload]: Reconfiguration of ISUP Interface-%d network ID from %d to %d is not allowed during runtime. Thus, stopping SS7 to apply this config.!\n", isupIntf->id,
									 cfg_val, reconfig_val);
								continue;
							}
						}
					}

					/* check ssf */
					cfg_val = g_ftdm_sngss7_data.cfg.isupIntf[idx].ssf;
					reconfig_val = isupIntf->ssf;

					if (cfg_val != reconfig_val) {
						for (span_idx = 0; span_idx < 100; span_idx++) {
							if (span_no[span_idx]) {
								ftmod_ss7_add_span_to_list(span_no[span_idx], span_list);
							}
							if (span_no[span_idx] == span_id) {
								SS7_INFO("[Reload]: Reconfiguration of ISUP Interface-%d ssf from %d to %d is not allowed during runtime. Thus, stopping SS7 to apply this config!\n", isupIntf->id,
									 cfg_val, reconfig_val);

								continue;
							}
						}
					}

					/* check switch Type */
					cfg_val = g_ftdm_sngss7_data.cfg.isupIntf[idx].switchType;
					reconfig_val = isupIntf->switchType;

					if (cfg_val != reconfig_val) {
						for (span_idx = 0; span_idx < 100; span_idx++) {
							if (span_no[span_idx]) {
								ftmod_ss7_add_span_to_list(span_no[span_idx], span_list);
							}
							if (span_no[span_idx] == span_id) {
								SS7_INFO("[Reload]: Reconfiguration of ISUP Interface-%d switchType from %d to %d is not allowed on runtime. Thus, stopping SS7 to apply this config.!\n",
									 isupIntf->id, cfg_val, reconfig_val);

								continue;
							}
						}
					}

					/* check spc */
					cfg_val = g_ftdm_sngss7_data.cfg.isupIntf[idx].spc;
					reconfig_val = isupIntf->spc;

					if (cfg_val != reconfig_val) {
						for (span_idx = 0; span_idx < 100; span_idx++) {
							if (span_no[span_idx]) {
								ftmod_ss7_add_span_to_list(span_no[span_idx], span_list);
							}
							if (span_no[span_idx] == span_id) {
								SS7_INFO("[Reload]: Reconfiguring ISUP Interface-%d OPC from %d to %d. Thus, stopping SS7 to apply this config!\n",
									 isupIntf->id, cfg_val, reconfig_val);
								continue;
							}
						}
					}

					/* check route */
					cfg_val = g_ftdm_sngss7_data.cfg.isupIntf[idx].mtpRouteId;
					reconfig_val = isupIntf->mtpRouteId;

					/* NOTE: For this change there is no need to reload ISUP interface value */
					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfiguring ISUP Interface-%d associated MTP route from %d to %d!\n",
							  isupIntf->id, cfg_val, reconfig_val);
					}

					/* only reconfiguration of pause Action and dpcCbTmr is allowed */
					/* check pause Action */
					cfg_val = g_ftdm_sngss7_data.cfg.isupIntf[idx].pauseAction;
					reconfig_val = isupIntf->pauseAction;

					if (cfg_val != reconfig_val) {
						SS7_DEBUG("[Reload]: Reconfigure ISUP Interface-%d pauseAction from %d to %d\n",
							  isupIntf->id, g_ftdm_sngss7_data.cfg.isupIntf[idx].pauseAction, isupIntf->pauseAction);
						reload = FTDM_TRUE;
					}

					for (span_idx = 0; span_idx < 100; span_idx++) {
						if (span_no[span_idx] == span_id) {
							span_found = FTDM_TRUE;
							isupIntf->reload = reload;
							break;
						} else {
							isupIntf->reload = FTDM_FALSE;
						}
					}

					if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
						g_ftdm_sngss7_data.cfg.reload = isupIntf->reload;
					}
				}
			}
		}
		idx++;
	}

	if ((invalid_conf == FTDM_TRUE) && (span_found != FTDM_TRUE)) {
		SS7_INFO("[Reload]: This is an invalid configuration request for span %d. As its respective signalling is not configured yet!\n",
			 span_id);
		ret = FTDM_FAIL;
		goto end;
	}

	if ((span_found != FTDM_TRUE) && (span_list[99] == NOT_CONFIGURED)) {
		ret = FTDM_BREAK;
	} else {
		ret = FTDM_SUCCESS;
	}
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Validate CC Span reconfiguration changes
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_validate_cc_span_reconfig_changes(int span_id, int span_list[])
{
	ftdm_bool_t reload              = FTDM_FALSE;
	ftdm_status_t ret               = FTDM_FAIL;
	sng_isup_ckt_t *isupCkt         = NULL;
	int cc_span_id 			= 0;
	int span_no 			= 0;
	int idx                         = 1;

	cc_span_id = ftmod_ss7_get_cc_span_id_by_span_id(span_id);
	if (!cc_span_id) {
		SS7_INFO("This is a new configuration request for span %d !\n",
			 span_id);
		return FTDM_BREAK;
	}

	idx = ftmod_ss7_get_start_cic_value_by_span_id(span_id);
	if (!idx) {
		idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	}

	/* check if cc span i.e. interface configurtion is valid or not */
	while((g_ftdm_sngss7_data.cfg.isupCkt[idx].id != 0) ||
	      (g_ftdm_sngss7_data.cfg_reload.isupCkt[idx].id != 0)) {

		/* reset span_no */
		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].id) {
			span_no = ftmod_ss7_get_span_id_by_cc_span_id(g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId);

			if ((!span_no) && (cc_span_id == g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId)) {
				ret = FTDM_FAIL;
				goto end;
			} else if (cc_span_id != g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId) {
				idx++;
				continue;
			}
		} else {
			/* This is a new configuration request thus, please go ahead */
			ret = FTDM_SUCCESS;
			goto end;
		}

		reload = FTDM_FALSE;
		isupCkt = &g_ftdm_sngss7_data.cfg_reload.isupCkt[idx];

		if (!isupCkt->obj) {
			SS7_INFO("[Reload]: Invalid ss7 info at index %d for reconfiguration\n", idx);
		}

		/* fill in the rest of the global structure */
		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].procId != isupCkt->procId) {
			SS7_ERROR("[Reload]: Reconfigure procId from %d to %d for ISUP Circuit is not supported!\n",
				  g_ftdm_sngss7_data.cfg.isupCkt[idx].procId, isupCkt->procId);
			ret = FTDM_FAIL;
			goto end;
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].id != isupCkt->id) {
			SS7_ERROR("[Reload]: Reconfigure circuit ID from %d to %d for ISUP Circuit is not supported!\n",
				  g_ftdm_sngss7_data.cfg.isupCkt[idx].id, isupCkt->id);
			ret = FTDM_FAIL;
			goto end;
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId != isupCkt->ccSpanId) {
			SS7_ERROR("[Reload]: Reconfigure Span ID from %d to %d for ISUP Circuit %d is not supported!\n",
				  g_ftdm_sngss7_data.cfg.isupCkt[idx].ccSpanId, isupCkt->ccSpanId, isupCkt->id);
			ret = FTDM_FAIL;
			goto end;
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].infId != isupCkt->infId) {
			SS7_ERROR("[Reload]: Reconfigure ISUP Interface ID from %d to %d for ISUP Circuit %d is not supported!\n",
				  g_ftdm_sngss7_data.cfg.isupCkt[idx].infId, isupCkt->infId, isupCkt->id);
			ret = FTDM_FAIL;
			goto end;
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].chan != isupCkt->chan) {
			SS7_ERROR("[Reload]: Reconfigure Channel from %d to %d for ISUP Circuit %d is not supported!\n",
				  g_ftdm_sngss7_data.cfg.isupCkt[idx].chan, isupCkt->chan, isupCkt->id);
			ret = FTDM_FAIL;
			goto end;
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].cic != isupCkt->cic) {
			ftmod_ss7_add_span_to_list(span_no, span_list);
			if (span_no == span_id) {
				SS7_INFO("[Reload]: Reconfigure CIC from %d to %d for ISUP Circuit %d is not allowed. Thus, stopping SS7 to apply this configuration!\n",
					 g_ftdm_sngss7_data.cfg.isupCkt[idx].cic, isupCkt->cic, isupCkt->id);

				idx++;
				continue;
			}
		}

		/* for interface only reconfiguration of typeCntrl,bearProf, contReq, nonSS7Con, cllo, cirTmr,
		 * outTrkGrpN, cvrTrkClli, and bits 1, 6 and 7 of cirFlg is allowed 
		 * NOTE: Except typeCntrl all othere reconfiguration structures are hardcoded in
		 *       ftmod_ss7_isup_ckt_config */
		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].typeCntrl != isupCkt->typeCntrl) {
			SS7_DEBUG("[Reload]: Reconfigure ISUP Circuit %d type control from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].typeCntrl, isupCkt->typeCntrl);
			reload = FTDM_TRUE;
		}

		/******** Below are list of reconfigurable parameters for which no PROTOCOL calls layer needs to be reconfigured
		 * thus as soon as we get these respective reconfiguration changes, we apply them on that very moment and if there
		 * are no other reconfiguration that required protocol level changes/reconfiguration we will mark this configuration
		 * request same as that of earlier and will cancel this reconfiguration request ********/
		/* These values can directly be changes without effecting other parts, as it does not have any effect on
		 * PROTOCOL part */
		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress != isupCkt->cpg_on_progress) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress = isupCkt->cpg_on_progress;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d cpg_on_progress from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress, isupCkt->cpg_on_progress);
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress_media != isupCkt->cpg_on_progress_media) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress_media = isupCkt->cpg_on_progress_media;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d cpg_on_progress_media from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].cpg_on_progress_media, isupCkt->cpg_on_progress_media);
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].ignore_alert_on_cpg != isupCkt->ignore_alert_on_cpg) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].ignore_alert_on_cpg = isupCkt->ignore_alert_on_cpg;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d ignore_alert_on_cpg from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].ignore_alert_on_cpg, isupCkt->ignore_alert_on_cpg);
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam_max_size != isupCkt->transparent_iam_max_size) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam_max_size = isupCkt->transparent_iam_max_size;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d transparent_iam_max_size from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam_max_size, isupCkt->transparent_iam_max_size);
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam != isupCkt->transparent_iam) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam = isupCkt->transparent_iam;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d transparent_iam from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].transparent_iam, isupCkt->transparent_iam);
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].itx_auto_reply != isupCkt->itx_auto_reply) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].itx_auto_reply = isupCkt->itx_auto_reply;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d itx_auto_reply from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].itx_auto_reply, isupCkt->itx_auto_reply);
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].bearcap_check != isupCkt->bearcap_check) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].bearcap_check = isupCkt->bearcap_check;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d bearcap_check from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].bearcap_check, isupCkt->bearcap_check);
		}

		if (g_ftdm_sngss7_data.cfg.isupCkt[idx].min_digits != isupCkt->min_digits) {
			g_ftdm_sngss7_data.cfg.isupCkt[idx].min_digits = isupCkt->min_digits;
			SS7_DEBUG("[Reload]: Reconfigured ISUP Circuit %d min_digits from %d to %d\n",
				  isupCkt->id, g_ftdm_sngss7_data.cfg.isupCkt[idx].min_digits, isupCkt->min_digits);
		}


		if (span_no == span_id) {
			isupCkt->reload  = reload;
		} else {
			isupCkt->reload  = FTDM_FALSE;
		}

		if (g_ftdm_sngss7_data.cfg.reload == FTDM_FALSE) {
			g_ftdm_sngss7_data.cfg.reload = isupCkt->reload;
		}
		idx++;
	}

	ret = FTDM_SUCCESS;
end:
	return ret;
}

/*********************************************************************************
*
* Desc: Clear SS7 related reconfiguration structures
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftdm_ss7_clear_ss7_reload_config()
{
	ftdm_status_t 	ret 	= FTDM_FAIL;
	int 		idx 	= 0;

	/* clear MTP1 reload structure */
	idx = 1;
	while (idx < (MAX_MTP_LINKS)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx], 0, sizeof (sng_mtp1_link_t));
			g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].flags = 0x00;
		}
		idx++;
	}

	/* clear MTP2 reload structure */
	idx = 1;
	while (idx < (MAX_MTP_LINKS)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx], 0, sizeof (sng_mtp2_link_t));
			g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx].flags = 0x00;
		}
		idx++;
	}

	/* clear MTP3 reload structure */
	idx = 1;
	while (idx < (MAX_MTP_LINKS)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx], 0, sizeof (sng_mtp3_link_t));
			g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx].flags = 0x00;
		}
		idx++;
	}

	/* clear MTP Linksets */
	idx = 1;
	while (idx < (MAX_MTP_LINKSETS+1)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx], 0, sizeof (sng_link_set_t));
			g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx].flags = 0x00;
		}
		idx++;
	}

	/* clear MTP Routes */
	idx = 1;
	while (idx < (MAX_MTP_ROUTES+1)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx], 0 , sizeof (sng_route_t));
			g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].flags = 0x00;
		}
		idx++;
	}

	/* clear MTP Self Routes */
	idx = MAX_MTP_ROUTES + 1;
	while ((idx > MAX_MTP_ROUTES) && (idx < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1))) {
		if (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx], 0 , sizeof (sng_route_t));
			g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].flags = 0x00;
		}
		idx++;
	}

	/* clear ISUP Interface */
	 idx = 1;
	 while (idx < (MAX_ISUP_INFS)) {
		 if (g_ftdm_sngss7_data.cfg_reload.isupIntf[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.isupIntf[idx], 0 , sizeof (sng_isup_inf_t));
			g_ftdm_sngss7_data.cfg_reload.isupIntf[idx].flags = 0x00;
		}
		idx++;
	}

	/* clear ISUP interfaces */
	 idx = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);;

	 if (!idx) {
		 idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	 }

	 while (g_ftdm_sngss7_data.cfg_reload.isupCkt[idx].id != 0) {
		memset(&g_ftdm_sngss7_data.cfg_reload.isupCkt[idx], 0 , sizeof (sng_isup_ckt_t));
		g_ftdm_sngss7_data.cfg_reload.isupIntf[idx].flags = 0x00;
		idx++;
	 }
	 memset(&g_ftdm_sngss7_data.cfg_reload.isupCkt, 0, sizeof(g_ftdm_sngss7_data.cfg_reload.isupCkt));

	 /* clear NSAP */
	 idx = 1;
	 while (idx < (MAX_NSAPS)) {
		if (g_ftdm_sngss7_data.cfg_reload.nsap[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.nsap[idx], 0 , sizeof (sng_nsap_t));
			g_ftdm_sngss7_data.cfg_reload.nsap[idx].flags = 0x00;
		}
		idx++;
	 }

	 /* clear ISAP */
	 idx = 1;
	 while (idx < (MAX_ISAPS)) {
		if (g_ftdm_sngss7_data.cfg_reload.isap[idx].id != 0) {
			memset(&g_ftdm_sngss7_data.cfg_reload.isap[idx], 0, sizeof (sng_isap_t));
			g_ftdm_sngss7_data.cfg_reload.isap[idx].flags = 0x00;
		}
		idx++;
	 }

	/* clearing fill reload structrure at the end making sure that reload strucrure
	 * is totally clean and clear */
	memset(&g_ftdm_sngss7_data.cfg_reload, 0, sizeof (sng_ss7_cfg_t));

	/* clearing reload flag is set */
	g_ftdm_sngss7_data.cfg.reload = FTDM_FALSE;

	return ret;
}

/*********************************************************************************
*
* Desc: Clear SS7 related reload flag
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftdm_ss7_clear_ss7_flag()
{
	ftdm_status_t 	ret 	= FTDM_FAIL;
	int 		idx 	= 0;

	/* clear MTP1 reconfiguration related flag structure */
	idx = 1;
	while (idx < (MAX_MTP_LINKS)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.mtp1Link[idx].reload = FTDM_FALSE;
		}
		idx++;
	}

	/* clear MTP2 reconfiguration related flag structure */
	idx = 1;
	while (idx < (MAX_MTP_LINKS)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx].reload = FTDM_FALSE;
			g_ftdm_sngss7_data.cfg_reload.mtp2Link[idx].disable_sap = FTDM_FALSE;
		}
		idx++;
	}

	/* clear MTP3 reconfiguration related flag structure */
	idx = 1;
	while (idx < (MAX_MTP_LINKS)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx].reload = FTDM_FALSE;
			g_ftdm_sngss7_data.cfg_reload.mtp3Link[idx].disable_sap = FTDM_FALSE;
		}
		idx++;
	}

	/* clear MTP Linksets reconfiguration related flag */
	idx = 1;
	while (idx < (MAX_MTP_LINKSETS+1)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx].reload = FTDM_FALSE;
			g_ftdm_sngss7_data.cfg_reload.mtpLinkSet[idx].deactivate_linkset = FTDM_FALSE;
		}
		idx++;
	}

	/* clear MTP Routes reconfiguration related flag */
	idx = 1;
	while (idx < (MAX_MTP_ROUTES+1)) {
		if (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].reload = FTDM_FALSE;
		}
		idx++;
	}

	/* clear MTP Self Routes reconfiguration related flag */
	idx = MAX_MTP_ROUTES + 1;
	while ((idx > MAX_MTP_ROUTES) && (idx < (MAX_MTP_ROUTES + SELF_ROUTE_INDEX + 1))) {
		if (g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.mtpRoute[idx].reload = FTDM_FALSE;
		}
		idx++;
	}

	/* clear ISUP Interface reconfiguration related flag */
	 idx = 1;
	 while (idx < (MAX_ISUP_INFS)) {
		 if (g_ftdm_sngss7_data.cfg_reload.isupIntf[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.isupIntf[idx].reload = FTDM_FALSE;
		}
		idx++;
	}

	/* clear ISUP interfaces  reconfiguration related flag */
	 idx = ftmod_ss7_get_circuit_start_range(g_ftdm_sngss7_data.cfg.procId);

	 if (!idx) {
		 idx = (g_ftdm_sngss7_data.cfg.procId * 1000) + 1;
	 }

	 while (g_ftdm_sngss7_data.cfg_reload.isupCkt[idx].id != 0) {
		g_ftdm_sngss7_data.cfg_reload.isupCkt[idx].reload = FTDM_FALSE;
		idx++;
	 }

	 /* clear NSAP reconfiguration related flag */
	 idx = 1;
	 while (idx < (MAX_NSAPS)) {
		if (g_ftdm_sngss7_data.cfg_reload.nsap[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.nsap[idx].reload = FTDM_FALSE;
			g_ftdm_sngss7_data.cfg_reload.nsap[idx].disable_sap = FTDM_FALSE;
		}
		idx++;
	 }

	 /* clear ISAP reconfiguration related flag */
	 idx = 1;
	 while (idx < (MAX_ISAPS)) {
		if (g_ftdm_sngss7_data.cfg_reload.isap[idx].id != 0) {
			g_ftdm_sngss7_data.cfg_reload.isap[idx].reload = FTDM_FALSE;
			g_ftdm_sngss7_data.cfg_reload.isap[idx].disable_sap = FTDM_FALSE;
		}
		idx++;
	 }

	return ret;
}

/* Activation of MTP Link */
/*********************************************************************************
*
* Desc: Check if MTP Link is not active then activate MTP Link
*
* Ret : FTDM_FAIL | FTDM_SUCCESS | ret value
*
**********************************************************************************/
ftdm_status_t ftdm_ss7_check_and_activate_mtp3_link()
{
	int 	idx = 0;
	SnMngmt sta;

	for (idx = 0; idx < (MAX_MTP_LINKS + 1); idx++) {
		memset(&sta, 0, sizeof(sta));
		if (g_ftdm_sngss7_data.cfg.mtp3Link[idx].id != 0) {
			if (ftmod_ss7_mtp3link_sta(idx, &sta)) {
				SS7_ERROR("[Reload]: Failed to read status of MTP3 link MTP3-%d!\n", g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
				return FTDM_FAIL;
			}

			if (sta.t.ssta.s.snDLSAP.state == LSN_LST_INACTIVE) {
				if (ftmod_ss7_activate_mtp3link(idx)) {
					SS7_ERROR("[Reload]: Failed to activate MTP3 link MTP3-%d!\n", g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
					return FTDM_FAIL;
				} else {
					SS7_DEBUG("[Reload]: MTP3 Link MTP3-%d successfully activated.\n", g_ftdm_sngss7_data.cfg.mtp3Link[idx].id);
				}
			}
		}
	}

	return FTDM_SUCCESS;
}

/*********************************************************************************
*
* Desc: Add span to span list if there is a reconfiguration change for that span
*	for which span needs to deleted/stop first
*
* Ret : FTDM_FAIL | FTDM_SUCCESS
*
**********************************************************************************/
ftdm_status_t ftmod_ss7_add_span_to_list(int span_id, int span_list[])
{
	int 	idx = 0;

	for (idx = 0; idx < 100; idx++) {
		if (!span_list[idx]) {
			break;
		}

		if (span_id == span_list[idx]) {
			SS7_DEBUG("[Reload]: Span %d already present in span list!\n", span_id);
			return FTDM_SUCCESS;
		}
	}

	for (idx = 0; idx < 100; idx++) {
		if (!span_list[idx]) {
			SS7_DEBUG("[Reload]: Adding Span %d to span list!\n", span_id);
			span_list[idx] = span_id;
			break;
		} else if (span_list[idx] == span_id) {
			SS7_DEBUG("[Reload]: Span %d is already present in stop span list\n",
				  span_id);
			break;
		}
	}

	return FTDM_SUCCESS;
}


/*********************************************************************************/
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
/*********************************************************************************/
