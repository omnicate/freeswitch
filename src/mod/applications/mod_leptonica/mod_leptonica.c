/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2015, Anthony Minessale II <anthm@freeswitch.org>
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
 * Seven Du <dujinfang@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * mod_leptonica.c -- distort video with Leptonica
 *
 */

#include <switch.h>
#include <libyuv.h>
#include "allheaders.h"


switch_loadable_module_interface_t *MODULE_INTERFACE;

SWITCH_MODULE_LOAD_FUNCTION(mod_leptonica_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_leptonica_shutdown);
SWITCH_MODULE_DEFINITION(mod_leptonica, mod_leptonica_load, mod_leptonica_shutdown, NULL);

typedef struct leptonica_context_s {
	PIX *pix;
	int w;
	int h;
	switch_core_session_t *session;
} leptonica_context_t;


static switch_status_t init_context(leptonica_context_t *context)
{
	return SWITCH_STATUS_SUCCESS;
}

static void uninit_context(leptonica_context_t *context)
{
	if (context->pix) pixDestroy(&context->pix);
}

static void makePtas(PTA **ppta1, PTA **ppta2, int w, int h)
{
	*ppta1 = ptaCreate(3);

	if (*ppta1) {
		ptaAddPt(*ppta1, 0, 0);
		ptaAddPt(*ppta1, w, h/4);
		ptaAddPt(*ppta1, 0, h);
	}

	*ppta2 = ptaCreate(3);

	if (*ppta2) {
		ptaAddPt(*ppta2, 0, 0);
		ptaAddPt(*ppta2, w, 0);
		ptaAddPt(*ppta2, 0, h);
	}
}

static PIX *distort(PIX *pix)
{
	PTA *pta1, *pta2;
	PIX *pix2 = NULL;

	makePtas(&pta1, &pta2, pix->w, pix->h);
	switch_assert(pta1);
	switch_assert(pta2);

	pix2 = pixAffineSequential(pix, pta2, pta1, 0, 0);

	ptaDestroy(&pta1);
	ptaDestroy(&pta2);

	return pix2;
}


static switch_status_t video_thread_callback(switch_core_session_t *session, switch_frame_t *frame, void *user_data)
{
	leptonica_context_t *context = (leptonica_context_t *) user_data;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	PIX *pix;

	if (!switch_channel_ready(channel)) {
		return SWITCH_STATUS_FALSE;
	}

	if (!frame->img) {
		return SWITCH_STATUS_SUCCESS;
	}

	if ((frame->img->d_w != context->w || frame->img->d_h != context->h)) {
		if (context->pix) pixDestroy(&context->pix);
	}

	if (!context->pix) context->pix = pixCreate(frame->img->d_w, frame->img->d_h, 32);

	if (context->pix) {
		I420ToRGBA(frame->img->planes[0], frame->img->stride[0],
						frame->img->planes[1], frame->img->stride[1],
						frame->img->planes[2], frame->img->stride[2],
						(uint8_t *)context->pix->data, context->pix->w * 4,
						context->pix->w, context->pix->h);

		pix = distort(context->pix);

		if (pix) {
			if (pix->w != frame->img->d_w || pix->h != frame->img->d_h) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "distored pix demension %dx%d\n", pix->w, pix->h);
				// pixWrite("/tmp/distored.png", pix, IFF_PNG);
				goto end;
			}

			RGBAToI420((uint8_t *)pix->data, pix->w * 4,
								frame->img->planes[0], frame->img->stride[0],
								frame->img->planes[1], frame->img->stride[1],
								frame->img->planes[2], frame->img->stride[2],
								pix->w, pix->h);
			pixDestroy(&pix);
		}
	}

end:
	return SWITCH_STATUS_SUCCESS;
}


SWITCH_STANDARD_APP(leptonica_start_function)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_frame_t *read_frame;
	leptonica_context_t context = { 0 };

	init_context(&context);

	switch_channel_answer(channel);
	switch_channel_set_flag_recursive(channel, CF_VIDEO_DECODED_READ);
	switch_channel_set_flag_recursive(channel, CF_VIDEO_ECHO);

	switch_core_session_raw_read(session);

	switch_core_session_set_video_read_callback(session, video_thread_callback, &context);

	while (switch_channel_ready(channel)) {
		switch_status_t status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}

		if (switch_test_flag(read_frame, SFF_CNG)) {
			continue;
		}

		memset(read_frame->data, 0, read_frame->datalen);
		switch_core_session_write_frame(session, read_frame, SWITCH_IO_FLAG_NONE, 0);
	}

	switch_core_session_set_video_read_callback(session, NULL, NULL);

	uninit_context(&context);

	switch_core_session_reset(session, SWITCH_TRUE, SWITCH_TRUE);
}

static switch_bool_t leptonica_bug_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type)
{
	leptonica_context_t *context = (leptonica_context_t *) user_data;

	switch_channel_t *channel = switch_core_session_get_channel(context->session);

	switch (type) {
	case SWITCH_ABC_TYPE_INIT:
		{
			switch_channel_set_flag_recursive(channel, CF_VIDEO_DECODED_READ);
		}
		break;
	case SWITCH_ABC_TYPE_CLOSE:
		{
			switch_thread_rwlock_unlock(MODULE_INTERFACE->rwlock);
			switch_channel_clear_flag_recursive(channel, CF_VIDEO_DECODED_READ);
			uninit_context(context);
		}
		break;
	case SWITCH_ABC_TYPE_READ_VIDEO_PING:
	case SWITCH_ABC_TYPE_VIDEO_PATCH:
		{
			switch_frame_t *frame = switch_core_media_bug_get_video_ping_frame(bug);
			video_thread_callback(context->session, frame, context);
		}
		break;
	default:
		break;
	}

	return SWITCH_TRUE;
}

SWITCH_STANDARD_APP(leptonica_bug_start_function)
{
	switch_media_bug_t *bug;
	switch_status_t status;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	leptonica_context_t *context;
	switch_media_bug_flag_t flags = SMBF_READ_VIDEO_PING;
	const char *function = "mod_leptonica";

	if ((bug = (switch_media_bug_t *) switch_channel_get_private(channel, "_leptonica_bug_"))) {
		if (!zstr(data) && !strcasecmp(data, "stop")) {
			switch_channel_set_private(channel, "_leptonica_bug_", NULL);
			switch_core_media_bug_remove(session, &bug);
		} else {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Cannot run 2 at once on the same channel!\n");
		}
		return;
	}

	switch_channel_wait_for_flag(channel, CF_VIDEO_READY, SWITCH_TRUE, 10000, NULL);

	context = (leptonica_context_t *) switch_core_session_alloc(session, sizeof(*context));
	assert(context != NULL);
	context->session = session;

	init_context(context);

	function = "patch:video";
	flags = SMBF_VIDEO_PATCH;

	switch_thread_rwlock_rdlock(MODULE_INTERFACE->rwlock);

	if ((status = switch_core_media_bug_add(session, function, NULL, leptonica_bug_callback, context, 0, flags, &bug)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Failure!\n");
		switch_thread_rwlock_unlock(MODULE_INTERFACE->rwlock);
		return;
	}

	switch_channel_set_private(channel, "_leptonica_bug_", bug);

}

/* API Interface Function */
#define LEPTONICA_BUG_API_SYNTAX "<uuid> [start|stop]"
SWITCH_STANDARD_API(leptonica_bug_api_function)
{
	switch_core_session_t *rsession = NULL;
	switch_channel_t *channel = NULL;
	switch_media_bug_t *bug;
	switch_status_t status;
	leptonica_context_t *context;
	char *mycmd = NULL;
	int argc = 0;
	char *argv[25] = { 0 };
	char *uuid = NULL;
	char *action = NULL;
	switch_media_bug_flag_t flags = SMBF_READ_VIDEO_PING;
	const char *function = "mod_leptonica";

	if (zstr(cmd)) {
		goto usage;
	}

	if (!(mycmd = strdup(cmd))) {
		goto usage;
	}

	if ((argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) < 2) {
		goto usage;
	}

	uuid = argv[0];
	action = argv[1];

	if (!(rsession = switch_core_session_locate(uuid))) {
		stream->write_function(stream, "-ERR Cannot locate session!\n");
		goto done;
	}

	channel = switch_core_session_get_channel(rsession);

	if ((bug = (switch_media_bug_t *) switch_channel_get_private(channel, "_leptonica_bug_"))) {
		if (!zstr(action)) {
			if (!strcasecmp(action, "stop")) {
				switch_channel_set_private(channel, "_leptonica_bug_", NULL);
				switch_core_media_bug_remove(rsession, &bug);
				stream->write_function(stream, "+OK Success\n");
			} else if (!strcasecmp(action, "start") || !strcasecmp(action, "mod") || !strcasecmp(action, "patch")) {
				context = (leptonica_context_t *) switch_core_media_bug_get_user_data(bug);
				switch_assert(context);
				stream->write_function(stream, "+OK Success\n");
			}
		} else {
			stream->write_function(stream, "-ERR Invalid action\n");
		}
		goto done;
	}

	if (!zstr(action) && strcasecmp(action, "start") && strcasecmp(action, "patch")) {
		goto usage;
	}

	context = (leptonica_context_t *) switch_core_session_alloc(rsession, sizeof(*context));
	assert(context != NULL);
	context->session = rsession;

	init_context(context);

	switch_thread_rwlock_rdlock(MODULE_INTERFACE->rwlock);

	if (!strcasecmp(action, "patch")) {
		function = "patch:video";
		flags = SMBF_VIDEO_PATCH;
	}

	if ((status = switch_core_media_bug_add(rsession, function, NULL,
											leptonica_bug_callback, context, 0, flags, &bug)) != SWITCH_STATUS_SUCCESS) {
		stream->write_function(stream, "-ERR Failure!\n");
		switch_thread_rwlock_unlock(MODULE_INTERFACE->rwlock);
		goto done;
	} else {
		switch_channel_set_private(channel, "_leptonica_bug_", bug);
		stream->write_function(stream, "+OK Success\n");
		goto done;
	}


 usage:
	stream->write_function(stream, "-USAGE: %s\n", LEPTONICA_BUG_API_SYNTAX);

 done:
	if (rsession) {
		switch_core_session_rwunlock(rsession);
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}


SWITCH_MODULE_LOAD_FUNCTION(mod_leptonica_load)
{
	switch_application_interface_t *app_interface;
	switch_api_interface_t *api_interface;

	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	MODULE_INTERFACE = *module_interface;

	SWITCH_ADD_APP(app_interface, "leptonica", "", "", leptonica_start_function, "", SAF_NONE);


	SWITCH_ADD_APP(app_interface, "leptonica_bug", "connect leptonica", "connect leptonica",
				   leptonica_bug_start_function, "", SAF_NONE);

	SWITCH_ADD_API(api_interface, "leptonica_bug", "leptonica_bug", leptonica_bug_api_function, LEPTONICA_BUG_API_SYNTAX);

	switch_console_set_complete("add leptonica_bug ::console::list_uuid ::[start:stop");

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_leptonica_shutdown)
{
	return SWITCH_STATUS_UNLOAD;
}


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

