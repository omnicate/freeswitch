/*******************************************************************************
 * *
 * * Title       : ftmod_wanpipe_error.h
 * *
 * * Description : This file mainly maps the events/alarms to human readle format
 * * 		   and prints the same in the logs.
 * *
 * * Written By  : Pushkar Singh (psingh@sangoma.com)
 * *
 * * Date        : 21-10-2013
 * *
 * *******************************************************************************/

/* WANPIPE EVENT DECODE **********************************************************/
#define DECODE_WAN_EVENT(event) \
((event) == WP_API_EVENT_NONE)			?"No event to read" : \
((event) == WP_API_EVENT_RBS)			?"Read RBS event" : \
((event) == WP_API_EVENT_ALARM)			?"Read ALARM event" : \
((event) == WP_API_EVENT_DTMF)			?"Read Enable/Disable DTMF event" : \
((event) == WP_API_EVENT_RM_DTMF)		?"Read Remove DMTF event" : \
((event) == WP_API_EVENT_RXHOOK)		?"Read HOOK event" : \
((event) == WP_API_EVENT_RING)			?"Event received :RING" : \
((event) == WP_API_EVENT_RING_DETECT)		?"Event received: RING DETECTED" : \
((event) == WP_API_EVENT_RING_TRIP_DETECT)	?"Event received: RING TRIP DETECTED" : \
((event) == WP_API_EVENT_TONE)			?"Event received: TONE" : \
((event) == WP_API_EVENT_TXSIG_KEWL)		?"TXSIG_KEWL: Transmit signal KEWL" : \
((event) == WP_API_EVENT_TXSIG_START)		?"TXSIG_START: Start Transmitting signal" : \
((event) == WP_API_EVENT_TXSIG_OFFHOOK)		?"TXSIG_OFFHOOK: Transmit signal OFFHOOK" : \
((event) == WP_API_EVENT_TXSIG_ONHOOK)		?"TXSIG_ONHOOK: Transmit signal ONHOOK" : \
((event) == WP_API_EVENT_ONHOOKTRANSFER)	?"ONHOOKTRANSFER: " : \
((event) == WP_API_EVENT_SETPOLARITY)		?"SETPOLARITY: Set Popularity " : \
((event) == WP_API_EVENT_BRI_CHAN_LOOPBACK)	?"Read BRI_CHAN_LOOPBACK event" : \
((event) == WP_API_EVENT_LINK_STATUS)		?"Read Link status" : \
((event) == WP_API_EVENT_MODEM_STATUS)		?"Read Modem Status" : \
((event) == WP_API_EVENT_POLARITY_REVERSE)	?"POLARITY_REVERSE: Reverse the Popularity" : \
((event) == WP_API_EVENT_FAX_DETECT)		?"Read Enable/Disable HW Fax Detection" : \
((event) == WP_API_EVENT_SET_RM_TX_GAIN)	?"Set TX Gain for FXO/FXS" : \
((event) == WP_API_EVENT_SET_RM_RX_GAIN)	?"Set RX Gain for FXO/FXS" : \
((event) == WP_API_EVENT_FAX_1100)		?"Read FAX 1100 tone" : \
((event) == WP_API_EVENT_FAX_2100)		?"Read FAX_2100 tone" : \
((event) == WP_API_EVENT_FAX_2100_WSPR)		?"Read FAX 2100 WSPR tone" : \
"Unknown Wanpipe Event"

/* WANPIPE ALARM DECODE **********************************************************/
#define DECODE_WAN_ALARM(alarm) \
((alarm) & WAN_TE_BIT_ALARM_ALOS)		?"Loss of Singnalling(ALOS). Please check the cable" : \
((alarm) & WAN_TE_BIT_ALARM_LOS)		?"Loss of Singnalling(LOS). Please check the cable" : \
((alarm) & WAN_TE_BIT_ALARM_ALTLOS)		?"Loss of Singnalling(ATLOS). Please check the cable" : \
((alarm) & WAN_TE_BIT_ALARM_OOF)		?"Out Of Frmae (OOF). Please check the line settings/framing/coding" : \
((alarm) & WAN_TE_BIT_ALARM_RED)		?"RED i.e the device is in alarm. Please check physical connection" : \
((alarm) & WAN_TE_BIT_ALARM_AIS)		?"Alarm Indication Signal(AIS). Please check with remote end as it is in alarm" : \
((alarm) & WAN_TE_BIT_ALARM_OOSMF)		?"WAN_TE_BIT_ALARM_OOSMF" : \
((alarm) & WAN_TE_BIT_ALARM_OOCMF)		?"WAN_TE_BIT_ALARM_OOCMF" : \
((alarm) & WAN_TE_BIT_ALARM_OOOF)		?"WAN_TE_BIT_ALARM_OOOF" : \
((alarm) & WAN_TE_BIT_ALARM_RAI)		?"Remote Alarm Indication(RAI). Far end is in RED alarm state. Please check line framing once" : \
((alarm) & WAN_TE_BIT_ALARM_YEL)		?"Yellow Alarm(YEL). Other end of the connection is down. Please contact your TELCO or the connected PBX/Channel Bank" : \
((alarm) & WAN_TE_BIT_ALARM_LOF)		?"Loss of Framing(LOF). Please verify once that the proper line framing is selected" : \
((alarm) & WAN_TE_BIT_LOOPUP_CODE)		?"WAN_TE_BIT_LOOPUP_CODE" : \
((alarm) & WAN_TE_BIT_LOOPDOWN_CODE)		?"WAN_TE_BIT_LOOPDOWN_CODE" : \
((alarm) & WAN_TE_BIT_ALARM_LIU)		?"LIU. Please check the line once" : \
((alarm) & WAN_TE_BIT_ALARM_LIU_SC)		?"Shot Circuit(LIU_SC). Please check the line once" : \
((alarm) & WAN_TE_BIT_ALARM_LIU_OC)		?"Open Circuit(LIU_OC). Please check the line once" : \
((alarm) & WAN_TE_BIT_ALARM_LIU_LOS)		?"Los of Signalling(LIU_LOS). Please check the line once" : \
"Unknown Wanpipe Alarm"
