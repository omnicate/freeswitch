/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2017, Seven Du <dujinfang@gmail.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Anthony Minessale II <anthm@freeswitch.org>
 *
 *
 * mod_pcap.c -- Play pcap file
 *
 */
#include <switch.h>

#define MAGIC 0xa1b2c3d4

/* Picky compiler */
#ifdef __ICC
#pragma warning (disable:167)
#endif

typedef struct rtp_data_s {
	switch_file_t *fd;
	switch_rtp_hdr_t rtp_hdr;
	char path[1024];
	int samples;
	int packets;
} rtp_data_t;

typedef struct pcap_hdr_s {
        uint32_t magic_number;   /* magic number */
        uint16_t version_major;  /* major version number */
        uint16_t version_minor;  /* minor version number */
        int32_t  thiszone;       /* GMT to local correction */
        uint32_t sigfigs;        /* accuracy of timestamps */
        uint32_t snaplen;        /* max length of captured packets, in octets */
        uint32_t network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

#define fail() r = 255; goto end

#define get_src_ip(ip, p) do { sprintf(ip, "%d.%d.%d.%d", *(p+26), *(p+27), *(p+28), *(p+29)); } while (0);
#define get_dst_ip(ip, p) do { sprintf(ip, "%d.%d.%d.%d", *(p+30), *(p+31), *(p+32), *(p+33)); } while (0);
#define get_src_port(port, p) do { *port = (switch_port_t)ntohs(*((switch_port_t *)(p+34))); } while (0);
#define get_dst_port(port, p) do { *port = (switch_port_t)ntohs(*((switch_port_t *)(p+36))); } while (0);

#define rtp_header_len 12
#define IS_IP(p) (*p == 0x08 && *(p+1) == 0x00)
#define IS_IPV4(p) ((*p) >> 4 == 0x04)
#define IS_MINIMAL_IP(p) (((*p) & 0x0F) == 0x05)
#define IS_UDP(p) (*p == 17)
#define IS_RTP_V2(p) ((*p) & 0x80)

int is_rtp(pcaprec_hdr_t *rec, uint8_t *buf)
{
	uint32_t len = rec->incl_len;
	uint8_t *p = buf;

	if (len < 42) return 0;

	p = buf + 12; // Type
	if (!IS_IP(p)) return 0; // Not IP

	p = buf + 14; // Version
	if (!IS_IPV4(p)) return 0;

	if (!IS_MINIMAL_IP(p)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Not supported IP packets. Len=%d\n", (*p) & 0x0F);
		return 0;
	}

	p = buf + 23; // Proto
	if (!IS_UDP(p)) return 0;

	p = buf + 42;
	if (!IS_RTP_V2(p)) return 0;

	// if (len == 60 || len == 58) return 0; // Ignore dtmf 2833

	if (len == 214) return 1; // PCM 20ms

	return 1;
}


SWITCH_STANDARD_APP(play_pcap_function)
{
	int fd = -1;
	switch_memory_pool_t *pool = switch_core_session_get_pool(session);
	switch_channel_t *channel = switch_core_session_get_channel(session);
	const char *file = data;
	pcap_hdr_t header;
	pcaprec_hdr_t rec;
	ssize_t len;
	uint8_t buf[65535];
	switch_port_t src_port;
	switch_port_t dst_port;
	char src_ip[20] = { 0 };
	char dst_ip[20] = { 0 };
	switch_rtp_hdr_t *rtp;
	switch_frame_t frame;
	uint32_t last_ts = 0;
	int ts_diff = 0;
	int64_t packet_time_diff = 0;
	int sanity = 10;

	if (!pool) {
		printf("No pool!\n");
		goto end;
	}

	fd = open(file, O_RDONLY);

	if (fd < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "File open error %d\n", errno);
		goto end;
	}

	len = read(fd, &header, sizeof(header));

	if (len != sizeof(header)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "read error %d\n", (int)len);

		if (len == -1) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "errno: %d\n", errno);
		}
		goto end;
	}

	if (header.magic_number != MAGIC) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "magic error: %d\n", header.magic_number);
		goto end;
	}

	while (sanity-- && !switch_channel_test_flag(channel, CF_VIDEO)) {
		switch_frame_t *read_frame;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "warning for video %d\n", sanity);
		switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);
		switch_yield(1000000);
	}

	// if (!switch_channel_test_flag(channel, CF_VIDEO)) {
	// 	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "channel has no video\n");
	// 	goto end;
	// }

	while(len > 0 && switch_channel_ready(channel)) {
		len = read(fd, &rec, sizeof(rec));

		if (len < sizeof(rec)) {
			break;
		}

		printf("packet len: %d %d\n", (int)rec.incl_len, (int)rec.orig_len);

		len = read(fd, buf, rec.incl_len);

		// printf("len: %d\n", (int)len);

		if (len < rec.incl_len) break;

		if (!is_rtp(&rec, buf)) {
			printf("ignore %d\n", rec.incl_len);
			continue;
		}

		get_src_ip(src_ip, buf);
		get_dst_ip(dst_ip, buf);
		get_src_port(&src_port, buf);
		get_dst_port(&dst_port, buf);

		// probably rtcp
		// if (src_port % 2 || dst_port % 2) continue;

		if (packet_time_diff == 0) packet_time_diff = switch_micro_time_now() - (int64_t)rec.ts_sec * 1000000 - rec.ts_usec;

		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "packet_time_diff: %lld\n", packet_time_diff);

		rtp = (switch_rtp_hdr_t *)(buf + 42);

		ts_diff = ntohl(rtp->ts) - last_ts;

		if (1) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "read rtp %4d: %s:%d -> %s:%d, t=%u.%06u v=%d, seq=%8d, ts=%10u, m=%d\n",
			rec.incl_len, src_ip, src_port, dst_ip, dst_port,
			rec.ts_sec, rec.ts_usec,
			rtp->version, ntohs(rtp->seq), ntohl(rtp->ts), rtp->m);


			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "bytes:%4d NALU:%02x %02x  tsdiff: %u\n",
				rec.incl_len, *(buf+42+12), *(buf+42+12+1), ts_diff);
		}

	    switch_set_flag(&frame, SFF_PROXY_PACKET);
	    switch_set_flag(&frame, SFF_RAW_RTP);

		frame.packet = buf + 42;
		// frame.packetlen = len - 42 - 4;  // fcs
		frame.packetlen = len - 42;
		// frame.data = buf + 42 + 12;
		// frame.datalen = len - 42 - 12;
		// frame.payload = 255;
		// frame.m = rtp->m;
		// frame.seq = ntohs(rtp->seq);
		// frame.timestamp = ntohl(rtp->ts);

		if (last_ts > 0 && ts_diff > 0) {
			while(switch_channel_ready(channel) && packet_time_diff > 0 &&
				((switch_micro_time_now() - (int64_t)rec.ts_sec * 1000000 - rec.ts_usec) < packet_time_diff) ) {
				switch_yield(10000);
			}
		}

		if (switch_core_session_write_encoded_video_frame(session, &frame, SWITCH_IO_FLAG_NONE, 0) != SWITCH_STATUS_SUCCESS) {
			break;
		}

		// if (rtp->m) switch_yield(9000);

		last_ts = ntohl(rtp->ts);
	}

end:
	if (fd > 0) close(fd);

}



SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_pcap_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_pcap_runtime);


SWITCH_MODULE_LOAD_FUNCTION(mod_pcap_load);
SWITCH_MODULE_DEFINITION(mod_pcap, mod_pcap_load, NULL, NULL);

SWITCH_MODULE_LOAD_FUNCTION(mod_pcap_load)
{
	switch_application_interface_t *app_interface;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	SWITCH_ADD_APP(app_interface, "play_pcap", "play an pcap file", "play an pcap file", play_pcap_function, "<file>", SAF_NONE);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}


  // Called when the system shuts down
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_pcap_shutdown)
{
	return SWITCH_STATUS_SUCCESS;
}


/*
  If it exists, this is called in it's own thread when the module-load completes
  If it returns anything but SWITCH_STATUS_TERM it will be called again automaticly
SWITCH_MODULE_RUNTIME_FUNCTION(mod_pcap_runtime);
{
	while(looping)
	{
		switch_yield(1000);
	}
	return SWITCH_STATUS_TERM;
}
*/

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet expandtab:
 */
