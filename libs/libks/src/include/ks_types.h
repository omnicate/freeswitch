/*
 * Copyright (c) 2007-2014, Anthony Minessale II
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

#ifndef _KS_TYPES_H_
#define _KS_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif							/* defined(__cplusplus) */

#define KS_STR2ENUM_P(_FUNC1, _FUNC2, _TYPE) FT_DECLARE(_TYPE) _FUNC1 (const char *name); FT_DECLARE(const char *) _FUNC2 (_TYPE type);                       

#define KS_STR2ENUM(_FUNC1, _FUNC2, _TYPE, _STRINGS, _MAX)  \
    FT_DECLARE(_TYPE) _FUNC1 (const char *name)             \
    {                                                       \
        int i;                                              \
        _TYPE t = _MAX ;                                    \
                                                            \
        for (i = 0; i < _MAX ; i++) {                       \
            if (!strcasecmp(name, _STRINGS[i])) {           \
                t = (_TYPE) i;                              \
                break;                                      \
            }                                               \
        }                                                   \
                                                            \
        return t;                                           \
    }                                                       \
    FT_DECLARE(const char *) _FUNC2 (_TYPE type)            \
    {                                                       \
        if (type > _MAX) {                                  \
            type = _MAX;                                    \
        }                                                   \
        return _STRINGS[(int)type];                         \
    }                                                       \

#define KS_ENUM_NAMES(_NAME, _STRINGS) static const char * _NAME [] = { _STRINGS , NULL };  

#define KS_VA_NONE "%s", ""

	typedef enum {
		KS_POLL_READ = (1 << 0),
		KS_POLL_WRITE = (1 << 1),
		KS_POLL_ERROR = (1 << 2)
	} ks_poll_t;

	typedef int16_t ks_port_t;
	typedef size_t ks_size_t;

	typedef enum {
		KS_SUCCESS,
		KS_FAIL,
		KS_BREAK,
		KS_DISCONNECTED,
		KS_GENERR
	} ks_status_t;

/*! \brief Used internally for truth test */
	typedef enum {
		KS_TRUE = 1,
		KS_FALSE = 0
	} ks_bool_t;

#ifndef __FUNCTION__
#define __FUNCTION__ (const char *)__func__
#endif

#define KS_PRE __FILE__, __FUNCTION__, __LINE__
#define KS_LOG_LEVEL_DEBUG 7
#define KS_LOG_LEVEL_INFO 6
#define KS_LOG_LEVEL_NOTICE 5
#define KS_LOG_LEVEL_WARNING 4
#define KS_LOG_LEVEL_ERROR 3
#define KS_LOG_LEVEL_CRIT 2
#define KS_LOG_LEVEL_ALERT 1
#define KS_LOG_LEVEL_EMERG 0

#define KS_LOG_DEBUG KS_PRE, KS_LOG_LEVEL_DEBUG
#define KS_LOG_INFO KS_PRE, KS_LOG_LEVEL_INFO
#define KS_LOG_NOTICE KS_PRE, KS_LOG_LEVEL_NOTICE
#define KS_LOG_WARNING KS_PRE, KS_LOG_LEVEL_WARNING
#define KS_LOG_ERROR KS_PRE, KS_LOG_LEVEL_ERROR
#define KS_LOG_CRIT KS_PRE, KS_LOG_LEVEL_CRIT
#define KS_LOG_ALERT KS_PRE, KS_LOG_LEVEL_ALERT
#define KS_LOG_EMERG KS_PRE, KS_LOG_LEVEL_EMERG

	struct ks_mpool_s;

	typedef struct ks_mpool_s ks_mpool_t;

	typedef enum {
		KS_MPCL_ANNOUNCE,
		KS_MPCL_TEARDOWN,
		KS_MPCL_DESTROY
	} ks_mpool_cleanup_action_t;

	typedef void (*ks_mpool_cleanup_fn_t) (ks_mpool_t *mpool, void *ptr, void *arg, int type, ks_mpool_cleanup_action_t action);

	typedef void (*ks_logger_t) (const char *file, const char *func, int line, int level, const char *fmt, ...);
	typedef void (*ks_listen_callback_t) (ks_socket_t server_sock, ks_socket_t client_sock, struct sockaddr_in *addr);


#ifdef __cplusplus
}
#endif							/* defined(__cplusplus) */
#endif							/* defined(_KS_TYPES_H_) */
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
