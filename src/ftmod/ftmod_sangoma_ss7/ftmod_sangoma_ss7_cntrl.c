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
 */

/* INCLUDE ********************************************************************/
#include "ftmod_sangoma_ss7_main.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/

/* GLOBALS ********************************************************************/
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
int ft_to_sngss7_activate_all(ftdm_sngss7_operating_modes_e opr_mode);

static int ftmod_ss7_enable_isap(int suId);
static int ftmod_ss7_enable_nsap(int suId, ftdm_sngss7_operating_modes_e opr_mode);
static int ftmod_ss7_enable_mtpLinkSet(int lnkSetId);


/******************************************************************************/

/* FUNCTIONS ******************************************************************/
int ft_to_sngss7_activate_all(ftdm_sngss7_operating_modes_e opr_mode)
{
	int x;

	x = 1;
	while (x < (MAX_ISAPS)) {
		/* check if this link has already been actived */
		if ((g_ftdm_sngss7_data.cfg.isap[x].id != 0) &&
			(!(g_ftdm_sngss7_data.cfg.isap[x].flags & SNGSS7_ACTIVE) ||
			(g_ftdm_sngss7_data.cfg.isap[x].disable_sap == FTDM_TRUE))) {

			if (ftmod_ss7_enable_isap(x)) {	
				SS7_CRITICAL("ISAP %d %s: NOT OK\n",
					     x,
					     g_ftdm_sngss7_data.cfg.isap[x].reload == FTDM_TRUE ? "reconfig Enable" : "Enable");
				return 1;
			} else {
				SS7_INFO("ISAP %d %s: OK\n",
					 x,
					 g_ftdm_sngss7_data.cfg.isap[x].reload == FTDM_TRUE ? "reconfig Enable" : "Enable");
			}

			/* unset reconfiguration flag if already set */
			g_ftdm_sngss7_data.cfg.isap[x].disable_sap = FTDM_FALSE;
			/* set the SNGSS7_ACTIVE flag */
			g_ftdm_sngss7_data.cfg.isap[x].flags |= SNGSS7_ACTIVE;
		} /* if !SNGSS7_ACTIVE */

		x++;
	} /* while (x < (MAX_ISAPS)) */

	if ((SNG_SS7_OPR_MODE_M2UA_SG  != opr_mode) &&
		(SNG_SS7_OPR_MODE_M3UA_SG != opr_mode)) {

		x = 1;
		while (x < (MAX_NSAPS)) {
			if ((g_ftdm_sngss7_data.cfg.nsap[x].opr_mode == SNG_SS7_OPR_MODE_M2UA_SG) ||
			    (g_ftdm_sngss7_data.cfg.nsap[x].opr_mode == SNG_SS7_OPR_MODE_M3UA_SG)) {
				x++;
				continue;
			}

			/* check if this link has already been actived */
			if ((g_ftdm_sngss7_data.cfg.nsap[x].id != 0) &&
				(!(g_ftdm_sngss7_data.cfg.nsap[x].flags & SNGSS7_ACTIVE) ||
				(g_ftdm_sngss7_data.cfg.nsap[x].disable_sap == FTDM_TRUE))) {

				if (ftmod_ss7_enable_nsap(x, opr_mode)) {
					SS7_CRITICAL("NSAP %d %s: NOT OK\n",
						     x,
						     g_ftdm_sngss7_data.cfg.nsap[x].reload == FTDM_TRUE ? "reconfig Enable" : "Enable");
					return 1;
				} else {
					SS7_INFO("NSAP %d %s: OK\n",
						 x,
						 g_ftdm_sngss7_data.cfg.nsap[x].reload == FTDM_TRUE ? "reconfig Enable" : "Enable");
				}

				/* unset reconfiguration flag if already set */
				g_ftdm_sngss7_data.cfg.nsap[x].disable_sap = FTDM_FALSE;
				/* set the SNGSS7_ACTIVE flag */
				g_ftdm_sngss7_data.cfg.nsap[x].flags |= SNGSS7_ACTIVE;
			} /* if !SNGSS7_ACTIVE */

			x++;
		} /* while (x < (MAX_NSAPS)) */
	}

	if ((SNG_SS7_OPR_MODE_ISUP == opr_mode) ||
			(SNG_SS7_OPR_MODE_M3UA_SG == opr_mode)) {
		x = 1;
		while (x < (MAX_MTP_LINKSETS+1)) {
			/* check if this link has already been actived */
			if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[x].id != 0) &&
				(!(g_ftdm_sngss7_data.cfg.mtpLinkSet[x].flags & SNGSS7_ACTIVE) ||
				(g_ftdm_sngss7_data.cfg.mtpLinkSet[x].deactivate_linkset == FTDM_TRUE))) {

				if (ftmod_ss7_enable_mtpLinkSet(x)) {
					SS7_CRITICAL("LinkSet \"%s\" Enable: NOT OK\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[x].name);
					return 1;
				} else {
					SS7_INFO("LinkSet \"%s\" Enable: OK\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[x].name);
				}

				/* unset reconfiguration flag if already set */
				g_ftdm_sngss7_data.cfg.mtpLinkSet[x].deactivate_linkset = FTDM_FALSE;
				/* set the SNGSS7_ACTIVE flag */
				g_ftdm_sngss7_data.cfg.mtpLinkSet[x].flags |= SNGSS7_ACTIVE;
			} /* if !SNGSS7_ACTIVE */

			x++;
		} /* while (x < (MAX_MTP_LINKSETS+1)) */
	}

	if ((SNG_SS7_OPR_MODE_M2UA_SG == opr_mode) ||
			(SNG_SS7_OPR_MODE_M2UA_ASP == opr_mode)) {
		return ftmod_ss7_m2ua_start(opr_mode);
	}

	if ((SNG_SS7_OPR_MODE_M3UA_SG == opr_mode) ||
			(SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode)) {
		return ftmod_ss7_m3ua_start(opr_mode);
	}

	return 0;
}

/******************************************************************************/
void ftmod_ss7_enable_all_linksets()
{
	int x = 0x00;
	x = 1;
	while (x < (MAX_MTP_LINKSETS+1)) {
		/* check if this link has already been actived */
		if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[x].id != 0) &&
		    (!(g_ftdm_sngss7_data.cfg.mtpLinkSet[x].flags & SNGSS7_ACTIVE))) {

			if (ftmod_ss7_enable_mtpLinkSet(x)) {
				SS7_CRITICAL("LinkSet \"%s\" Enable: NOT OK\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[x].name);
				return ;
			} else {
				SS7_INFO("LinkSet \"%s\" Enable: OK\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[x].name);
			}

			/* set the SNGSS7_ACTIVE flag */
			g_ftdm_sngss7_data.cfg.mtpLinkSet[x].flags |= SNGSS7_ACTIVE;
		} /* if !SNGSS7_ACTIVE */

		x++;
	} /* while (x < (MAX_MTP_LINKSETS+1)) */
}

/******************************************************************************/
void ftmod_ss7_enable_linkset(int linkset_id)
{
	/* check if this link has already been actived */
	if ((g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_id].id != 0) &&
		(!(g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_id].flags & SNGSS7_ACTIVE))) {

		if (ftmod_ss7_enable_mtpLinkSet(linkset_id)) {
			SS7_CRITICAL("LinkSet \"%s\" Enable: NOT OK\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_id].name);
			return ;
		} else {
			SS7_INFO("LinkSet \"%s\" Enable: OK\n", g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_id].name);
		}

		/* set the SNGSS7_ACTIVE flag */
		g_ftdm_sngss7_data.cfg.mtpLinkSet[linkset_id].flags |= SNGSS7_ACTIVE;
	} /* if !SNGSS7_ACTIVE */
}

/******************************************************************************/
static int ftmod_ss7_enable_isap(int suId)
{
	CcMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTCC;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(CcMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;		/* this is a control request */
	cntrl.hdr.entId.ent			= ENTCC;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STISAP;

	cntrl.hdr.elmId.elmntInst1	= suId;			/* this is the SAP to bind */

	cntrl.t.cntrl.action		= ABND_ENA;		/* bind and activate */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_cc(&pst, &cntrl));
}

/* Disable ISUP ISAP */
/******************************************************************************/
int ftmod_ss7_disable_isap(int suId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType					= TCNTRL;		/* Message Type */
	cntrl.hdr.entId.ent					= ENTSI;		/* Element ID */
	cntrl.hdr.entId.inst					= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt					= STISAP;		/* Element ID */

	cntrl.t.cntrl.s.siElmnt.elmntId.sapId			= suId; 		/* Sap ID */

	cntrl.t.cntrl.action					= AUBND_DIS;		/* unbind and de-activate */
	cntrl.t.cntrl.subAction					= SAELMNT;		/* specificed element */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/* Delete ISUP ISAP */
/******************************************************************************/
int ftmod_ss7_delete_isap(int suId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType					= TCNTRL;		/* Message Type */
	cntrl.hdr.entId.ent					= ENTSI;		/* Element ID */
	cntrl.hdr.entId.inst					= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt					= STISAP;		/* Element ID */

	cntrl.t.cntrl.s.siElmnt.elmntId.sapId			= suId; 		/* Sap ID */

	cntrl.t.cntrl.action					= ADEL;			/* Delete */
	cntrl.t.cntrl.subAction					= SAELMNT;		/* specificed element */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/******************************************************************************/
static int ftmod_ss7_enable_nsap(int suId, ftdm_sngss7_operating_modes_e opr_mode)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	   /* this is a control request */
	cntrl.hdr.entId.ent			= ENTSI;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STNSAP;

	cntrl.t.cntrl.s.siElmnt.elmntId.sapId				= suId; 
	if (SNG_SS7_OPR_MODE_M3UA_ASP == opr_mode) {
		cntrl.t.cntrl.s.siElmnt.elmntParam.nsap.nsapType	= SAP_M3UA;
	} else {
		cntrl.t.cntrl.s.siElmnt.elmntParam.nsap.nsapType	= SAP_MTP;
	}


	cntrl.t.cntrl.action		= ABND_ENA;		/* bind and activate */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/* Disable ISUP NSAP */
/******************************************************************************/
int ftmod_ss7_disable_nsap(int suId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType					= TCNTRL;		/* Message Type */
	cntrl.hdr.entId.ent					= ENTSI;		/* Element ID */
	cntrl.hdr.entId.inst					= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt					= STNSAP;		/* Element ID */

	cntrl.t.cntrl.s.siElmnt.elmntId.sapId			= suId; 		/* Sap ID */
	cntrl.t.cntrl.s.siElmnt.elmntParam.nsap.nsapType	= SAP_MTP;		/* NSAP Type */

	cntrl.t.cntrl.action					= AUBND_DIS;		/* unbind and de-activate */
	cntrl.t.cntrl.subAction					= SAELMNT;		/* specificed element */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/* Delete ISUP NSAP */
/******************************************************************************/
int ftmod_ss7_delete_nsap(int suId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType					= TCNTRL;		/* Message Type */
	cntrl.hdr.entId.ent					= ENTSI;		/* Element ID */
	cntrl.hdr.entId.inst					= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt					= STNSAP;		/* Element ID */

	cntrl.t.cntrl.s.siElmnt.elmntId.sapId			= suId; 		/* Sap ID */
	cntrl.t.cntrl.s.siElmnt.elmntParam.nsap.nsapType	= SAP_MTP;		/* NSAP Type */

	cntrl.t.cntrl.action					= ADEL;			/* Delete */
	cntrl.t.cntrl.subAction					= SAELMNT;		/* specificed element */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/******************************************************************************/
static int ftmod_ss7_enable_mtpLinkSet(int lnkSetId)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;		/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STLNKSET;
	cntrl.hdr.elmId.elmntInst1	= lnkSetId;		/* this is the linkset to bind */

	cntrl.t.cntrl.action		= ABND_ENA;		/* bind and activate */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_inhibit_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= id;		/* the DSLAP to inhibit  */

	cntrl.t.cntrl.action		= AINH;		/* Inhibit */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_uninhibit_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= id;		/* the DSLAP to inhibit  */

	cntrl.t.cntrl.action		= AUNINH;		/* Inhibit */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_bind_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id;

	cntrl.t.cntrl.action		= ABND;		/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_unbind_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id;

	cntrl.t.cntrl.action		= AUBND_DIS;		/* unbind and disable */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_activate_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id;

	cntrl.t.cntrl.action		= AENA;		/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_deactivate_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id;

	cntrl.t.cntrl.action		= ADISIMM;	/* Deactivate */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_deactivate2_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id;

	cntrl.t.cntrl.action		= ADISIMM_L2;	/* Deactivate...layer 2 only */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/* Delete MTP3 Link */
/******************************************************************************/
int ftmod_ss7_delete_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType		= TCNTRL;	/* Message Type Control request */
	cntrl.hdr.entId.ent		= ENTSN;	/* Entity ID */
	cntrl.hdr.entId.inst		= S_INST;	/* Instance ID */
	cntrl.hdr.elmId.elmnt		= STDLSAP;	/* Element ID */
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id; /* Link Number */

	cntrl.t.cntrl.action		= ADELLNK;	/* Delete */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_activate_mtplinkSet(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STLNKSET;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtpLinkSet[id].id;

	cntrl.t.cntrl.action		= AACTLNKSET;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_deactivate_mtplinkSet(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STLNKSET;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtpLinkSet[id].id;

	cntrl.t.cntrl.action		= ADEACTLNKSET;	/* De-Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_deactivate2_mtplinkSet(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STLNKSET;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtpLinkSet[id].id;

	cntrl.t.cntrl.action		= ADEACTLNKSET_L2;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/* Delete LinkSet */
/******************************************************************************/
int ftmod_ss7_delete_mtpLinkSet(int lnkSetId)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType		= TCNTRL;		/* Control request */
	cntrl.hdr.entId.ent		= ENTSN;		/* Entity ID */
	cntrl.hdr.entId.inst		= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt		= STLNKSET;		/* Element ID */
	cntrl.hdr.elmId.elmntInst1	= lnkSetId;		/* Linkset ID to bind */

	cntrl.t.cntrl.action		= ADELLNKSET;		/* Action to Delete */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_lpo_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id;

	cntrl.t.cntrl.action		= ACTION_LPO;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_lpr_mtp3link(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= g_ftdm_sngss7_data.cfg.mtp3Link[id].id;

	cntrl.t.cntrl.action		= ACTION_LPR;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/* Delete Route */
/******************************************************************************/
int ftmod_ss7_delete_route(uint32_t id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType		= TCNTRL;	/* MsgType Control request */
	cntrl.hdr.entId.ent		= ENTSN;	/* Element ID */
	cntrl.hdr.entId.inst		= S_INST;	/* Instance ID */
	cntrl.hdr.elmId.elmnt		= STROUT;	/* Elemet ID */

	cntrl.t.cntrl.action		= ADELROUT;	/* Delete Route */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	cntrl.t.cntrl.ctlType.snRouteId.upSwtch = g_ftdm_sngss7_data.cfg.mtpRoute[id].switchType; /* Switch Type */
	cntrl.t.cntrl.ctlType.snRouteId.dpc 	 = g_ftdm_sngss7_data.cfg.mtpRoute[id].dpc;	   /* DPC */
	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_shutdown_isup(void)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	   /* this is a control request */
	cntrl.hdr.entId.ent			= ENTSI;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGEN;

	cntrl.t.cntrl.action		= ASHUTDOWN;	/* shutdown */
	cntrl.t.cntrl.subAction		= SAELMNT;		/* specificed element */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_shutdown_mtp3(void)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGEN;

	cntrl.t.cntrl.action		= ASHUTDOWN;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/* disable mtp2 link */
/******************************************************************************/
int ftmod_ss7_disable_mtp2_link(int lnkNmb)
{
	SdMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination entity */
	pst.dstEnt = ENTSD;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SdMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType		= TCNTRL;	/* message type control request */
	cntrl.hdr.entId.ent		= ENTSD;	/* entity id */
	cntrl.hdr.entId.inst		= S_INST;	/* instance id */
	cntrl.hdr.elmId.elmnt		= STDLSAP;	/* element id */
	cntrl.hdr.elmId.elmntInst1	= lnkNmb;	/* link number */

	cntrl.t.cntrl.action		= ADISIMM;	/* disable */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp2(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_delete_mtp2_link(int lnkNmb)
{
	SdMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSD;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SdMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType		= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent		= ENTSD;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STDLSAP;
	cntrl.hdr.elmId.elmntInst1	= lnkNmb;

	cntrl.t.cntrl.action		= ADEL;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp2(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_shutdown_mtp2(void)
{
	SdMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSD;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SdMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSD;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGEN;

	cntrl.t.cntrl.action		= ASHUTDOWN;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_mtp2(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_delete_mtp1_link (int sap_id)
{
    L1Mngmt cntrl;
    Pst pst;

    /* initalize the post structure */
    smPstInit(&pst);

    /* insert the destination Entity */
    pst.dstEnt = ENTL1;

    /* initalize the control structure */
    memset(&cntrl, 0x0, sizeof(cntrl));

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;   /* this is a control request */
    cntrl.hdr.entId.ent         = ENTL1;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STPSAP;

    cntrl.t.cntrl.action        = ADEL;    /* Activate */
    cntrl.t.cntrl.subAction     = SAELMNT; /* specificed element */

    cntrl.t.cntrl.spId			= sap_id;

    return (sng_cntrl_mtp1(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_shutdown_mtp1(void)
{
    L1Mngmt cntrl;
    Pst pst;

    /* initalize the post structure */
    smPstInit(&pst);

    /* insert the destination Entity */
    pst.dstEnt = ENTL1;

    /* initalize the control structure */
    memset(&cntrl, 0x0, sizeof(cntrl));

    /* initalize the control header */
    smHdrInit(&cntrl.hdr);

    cntrl.hdr.msgType           = TCNTRL;   /* this is a control request */
    cntrl.hdr.entId.ent         = ENTL1;
    cntrl.hdr.entId.inst        = S_INST;
    cntrl.hdr.elmId.elmnt       = STGEN;

    cntrl.t.cntrl.action        = ASHUTDOWN;    /* Activate */
    cntrl.t.cntrl.subAction     = SAELMNT;          /* specificed element */

    return (sng_cntrl_mtp1(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_shutdown_relay(void)
{
	RyMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTRY;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(RyMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTRY;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGEN;

	cntrl.t.cntrl.action		= ASHUTDOWN;	/* Activate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_relay(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_disable_relay_channel(uint32_t chanId)
{
	RyMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTRY;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(RyMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	/* this is a control request */
	cntrl.hdr.entId.ent			= ENTRY;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGEN;

	
	cntrl.hdr.elmId.elmntInst1	= chanId;

	cntrl.t.cntrl.action		= ADISIMM;			/* Deactivate */
	cntrl.t.cntrl.subAction		= SAELMNT;			/* specificed element */

	return (sng_cntrl_relay(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_disable_grp_mtp3Link(uint32_t procId)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;			/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGRDLSAP;		/* group DLSAP */

	cntrl.t.cntrl.ctlType.groupKey.dstProcId = procId;	/* all SAPS to this ProcId */

	cntrl.t.cntrl.action		= AUBND_DIS;		/* disable and unbind */
	cntrl.t.cntrl.subAction		= SAGR_DSTPROCID;			/* specificed element */

	if (g_ftdm_sngss7_data.cfg.procId == procId) {
		SS7_DEBUG("Executing MTP3 cntrl command local pid =%i\n",procId);
		return (sng_cntrl_mtp3(&pst, &cntrl));
	} else {
		SS7_WARN("Executing MTP3 cntrl command different local=%i target=%i\n",
				g_ftdm_sngss7_data.cfg.procId,procId);
		return (sng_cntrl_mtp3_nowait(&pst, &cntrl));
	}

}

/******************************************************************************/
int ftmod_ss7_enable_grp_mtp3Link(uint32_t procId)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;			/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGRDLSAP;		/* group DLSAP */

	cntrl.t.cntrl.ctlType.groupKey.dstProcId = procId;	/* all SAPS to this ProcId */

	cntrl.t.cntrl.action		= ABND_ENA;			/* bind and enable */
	cntrl.t.cntrl.subAction		= SAGR_DSTPROCID;			/* specificed element */

	if (g_ftdm_sngss7_data.cfg.procId == procId) {
		SS7_DEBUG("Executing MTP3 cntrl command local pid =%i\n",procId);
		return (sng_cntrl_mtp3(&pst, &cntrl));
	} else {
		SS7_WARN("Executing MTP3 cntrl command different local=%i target=%i\n",
				g_ftdm_sngss7_data.cfg.procId,procId);
		return (sng_cntrl_mtp3_nowait(&pst, &cntrl));
	}

}

/******************************************************************************/
int ftmod_ss7_disable_grp_mtp2Link(uint32_t procId)
{
	SdMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSD;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(cntrl));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;			/* this is a control request */
	cntrl.hdr.entId.ent			= ENTSD;
	cntrl.hdr.entId.inst		= S_INST;
	cntrl.hdr.elmId.elmnt		= STGRNSAP;			/* group NSAP */

	cntrl.t.cntrl.par.dstProcId = procId;			/* all SAPS to this ProcId */

	cntrl.t.cntrl.action		= AUBND_DIS;		/* disable and unbind */
	cntrl.t.cntrl.subAction		= SAGR_DSTPROCID;			/* specificed element */

	return (sng_cntrl_mtp2(&pst, &cntrl));

}

/* SANGOMA Disable and Unbind MTP3 NSAP */
/******************************************************************************/
int ftmod_ss7_disable_mtp3_nsap(uint32_t nsap_id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType		= TCNTRL;	/* Message Type Control request */
	cntrl.hdr.entId.ent		= ENTSN;	/* Entity ID */
	cntrl.hdr.entId.inst		= S_INST;	/* Instance ID */
	cntrl.hdr.elmId.elmnt		= STNSAP;	/* Element Id */
	cntrl.hdr.elmId.elmntInst1	= nsap_id; 	/* NSAP ID */

	cntrl.t.cntrl.action		= AUBND_DIS;	/* Unbind and Disable */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/* SANGOMA Delete MTP3 NSAP */
/******************************************************************************/
int ftmod_ss7_delete_mtp3_nsap(uint32_t nsap_id)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType		= TCNTRL;	/* Message Type Control request */
	cntrl.hdr.entId.ent		= ENTSN;	/* Entity ID */
	cntrl.hdr.entId.inst		= S_INST;	/* Instance ID */
	cntrl.hdr.elmId.elmnt		= STNSAP;	/* Element Id */
	cntrl.hdr.elmId.elmntInst1	= nsap_id;	/* NSAP ID */

	cntrl.t.cntrl.action		= ADEL;		/* Unbind and Disable */
	cntrl.t.cntrl.subAction		= SAELMNT;	/* specificed element */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int __ftmod_ss7_block_isup_ckt(uint32_t cktId, ftdm_bool_t wait)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType						= TCNTRL;		/* this is a control request */
	cntrl.hdr.entId.ent						= ENTSI;
	cntrl.hdr.entId.inst					= S_INST;
	cntrl.hdr.elmId.elmnt					= STICIR;

	cntrl.t.cntrl.s.siElmnt.elmntId.circuit	= cktId;
	cntrl.t.cntrl.s.siElmnt.elmntParam.cir.flag = LSI_CNTRL_CIR_FORCE;

	cntrl.t.cntrl.action					= ADISIMM;		/* block via BLO */
	cntrl.t.cntrl.subAction					= SAELMNT;		/* specificed element */

	if (wait == FTDM_TRUE) {
		return (sng_cntrl_isup(&pst, &cntrl));
	} else {
		return (sng_cntrl_isup_nowait(&pst, &cntrl));
	}
}

/******************************************************************************/
int ftmod_ss7_unblock_isup_ckt(uint32_t cktId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType						= TCNTRL;		/* this is a control request */
	cntrl.hdr.entId.ent						= ENTSI;
	cntrl.hdr.entId.inst					= S_INST;
	cntrl.hdr.elmId.elmnt					= STICIR;

	cntrl.t.cntrl.s.siElmnt.elmntId.circuit		= cktId;
	cntrl.t.cntrl.s.siElmnt.elmntParam.cir.flag = LSI_CNTRL_CIR_FORCE;

	cntrl.t.cntrl.action					= AENA;			/* unblock via UBL */
	cntrl.t.cntrl.subAction					= SAELMNT;		/* specificed element */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/* Disable ISUP Circuit */
/******************************************************************************/
int ftmod_ss7_disable_isup_ckt(uint32_t cktId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;		/* Message Type control request */
	cntrl.hdr.entId.ent			= ENTSI;		/* Entity ID */
	cntrl.hdr.entId.inst 			= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt 			= STICIR;		/* Element ID */

	cntrl.t.cntrl.action 			= ADISIMM;    		/* Disable */
	cntrl.t.cntrl.subAction 		= SAELMNT;		/* specificed element */
	cntrl.t.cntrl.s.siElmnt.elmntId.circuit	= cktId;		/* Circuit ID */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/* Delete ISUP Circuit */
/******************************************************************************/
int ftmod_ss7_delete_isup_ckt(uint32_t cktId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;		/* Message Type control request */
	cntrl.hdr.entId.ent			= ENTSI;		/* Entity ID */
	cntrl.hdr.entId.inst 			= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt 			= STICIR;		/* Element ID */

	cntrl.t.cntrl.action 			= ADEL;    		/* Delete */
	cntrl.t.cntrl.subAction 		= SAELMNT;		/* specificed element */
	cntrl.t.cntrl.s.siElmnt.elmntId.circuit	= cktId;		/* Circuit ID */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/* Disable ISUP Interface */
/******************************************************************************/
int ftmod_ss7_disable_isup_intf(uint32_t intfId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;		/* Message Type control request */
	cntrl.hdr.entId.ent			= ENTSI;		/* Entity ID */
	cntrl.hdr.entId.inst 			= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt 			= SI_STINTF;		/* Element ID */

	cntrl.t.cntrl.action 			= ADISIMM; 		/* Disable */
	cntrl.t.cntrl.subAction 		= SAELMNT;		/* specificed element */
	cntrl.t.cntrl.s.siElmnt.elmntId.intfId	= intfId;		/* Interface ID */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/* Delete ISUP Interface */
/******************************************************************************/
int ftmod_ss7_delete_isup_intf(uint32_t intfId)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;		/* Message Type control request */
	cntrl.hdr.entId.ent			= ENTSI;		/* Entity ID */
	cntrl.hdr.entId.inst 			= S_INST;		/* Instance ID */
	cntrl.hdr.elmId.elmnt 			= SI_STINTF;		/* Element ID */

	cntrl.t.cntrl.action 			= ADEL; 		/* Delete */
	cntrl.t.cntrl.subAction 		= SAELMNT;		/* specificed element */
	cntrl.t.cntrl.s.siElmnt.elmntId.intfId	= intfId;		/* Circuit ID */

	return (sng_cntrl_isup(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_isup_debug(int action)
{
	SiMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSI;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SiMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;	   /* this is a control request */
	cntrl.hdr.entId.ent			= ENTSI;
	cntrl.hdr.entId.inst		= S_INST;
	//cntrl.hdr.elmId.elmnt		= ;

	cntrl.t.cntrl.action		= action;		/* bind and activate */
	cntrl.t.cntrl.subAction		= SADBG;		/* specificed element */
#if (SI_LMINT3 || SMSI_LMINT3)
	cntrl.t.cntrl.s.siDbg.dbgMask		= 0xFFFF;		
#else
	cntrl.t.cntrl.param.siDbg.dbgMask   = 0xFFFF;
#endif

	return (sng_cntrl_isup(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_mtp3_debug(int action)
{
	SnMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSN;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SnMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType			= TCNTRL;       /* this is a control request */
	cntrl.hdr.entId.ent			= ENTSN;
	cntrl.hdr.entId.inst			= S_INST;
	cntrl.hdr.elmId.elmnt			= STGEN;

	cntrl.t.cntrl.action			= action;	/* Activate */
	cntrl.t.cntrl.subAction			= SADBG;	/* specificed element */

	cntrl.t.cntrl.ctlType.snDbg.dbgMask	= 0xFFFF;	/* Setting up the debug level */

	return (sng_cntrl_mtp3(&pst, &cntrl));
}

/******************************************************************************/
int ftmod_ss7_mtp2_debug(int action)
{
	SdMngmt cntrl;
	Pst pst;

	/* initalize the post structure */
	smPstInit(&pst);

	/* insert the destination Entity */
	pst.dstEnt = ENTSD;

	/* initalize the control structure */
	memset(&cntrl, 0x0, sizeof(SdMngmt));

	/* initalize the control header */
	smHdrInit(&cntrl.hdr);

	cntrl.hdr.msgType               = TCNTRL;       /* this is a control request */
	cntrl.hdr.entId.ent             = ENTSD;
	cntrl.hdr.entId.inst            = S_INST;
	cntrl.hdr.elmId.elmnt           = STGEN;

	cntrl.t.cntrl.action            = action;	/* Activate */
	cntrl.t.cntrl.subAction         = SADBG;	/* specificed element */

	cntrl.t.cntrl.sdDbg.dbgMask	= 0xFFFF;	/* Setting up the debug level */

	return (sng_cntrl_mtp2(&pst, &cntrl));
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

