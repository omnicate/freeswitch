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


/* Use select on windows and poll everywhere else.
   Select is the devil.  Especially if you are doing a lot of small socket connections.
   If your FD number is bigger than 1024 you will silently create memory corruption.

   If you have build errors on your platform because you don't have poll find a way to detect it and #define KS_USE_SELECT and #undef KS_USE_POLL
   All of this will be upgraded to autoheadache eventually.
*/

/* TBD for win32 figure out how to tell if you have WSAPoll (vista or higher) and use it when available by #defining KS_USE_WSAPOLL (see below) */

#ifdef _MSC_VER
#define FD_SETSIZE 8192
#define KS_USE_SELECT
#else
#define KS_USE_POLL
#endif

#include <ks.h>

#ifndef WIN32

#define closesocket(x) shutdown(x, 2); close(x)

#else /* WIN32 */

#pragma warning (disable:6386)
/* These warnings need to be ignored warning in sdk header */
#include <Ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#ifndef errno
#define errno WSAGetLastError()
#endif

#ifndef EINTR
#define EINTR WSAEINTR
#endif

#pragma warning (default:6386)

#endif /* WIN32 */

#ifdef KS_USE_POLL
#include <poll.h>
#endif

static int ks_socket_reuseaddr(ks_socket_t socket)
{
#ifdef WIN32
	BOOL reuse_addr = TRUE;
	return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse_addr, sizeof(reuse_addr));
#else
	int reuse_addr = 1;
	return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
#endif
}

struct thread_handler {
	ks_listen_callback_t callback;
	ks_socket_t server_sock;
	ks_socket_t client_sock;
	struct sockaddr_in addr;
};

static void *client_thread(ks_thread_t *me, void *obj)
{
	struct thread_handler *handler = (struct thread_handler *) obj;

	handler->callback(handler->server_sock, handler->client_sock, &handler->addr);
	free(handler);

	return NULL;

}

KS_DECLARE(ks_status_t) ks_listen(const char *host, ks_port_t port, ks_listen_callback_t callback)
{
	ks_socket_t server_sock = KS_SOCK_INVALID;
	struct sockaddr_in addr;
	ks_status_t status = KS_STATUS_SUCCESS;

	if ((server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) != KS_SOCK_INVALID) {
		return KS_STATUS_FAIL;
	}

	ks_socket_reuseaddr(server_sock);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(server_sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		status = KS_STATUS_FAIL;
		goto end;
	}

	if (listen(server_sock, 10000) < 0) {
		status = KS_STATUS_FAIL;
		goto end;
	}

	for (;;) {
		ks_socket_t client_sock;
		struct sockaddr_in echoClntAddr;
#ifdef WIN32
		int clntLen;
#else
		unsigned int clntLen;
#endif

		clntLen = sizeof(echoClntAddr);

		if ((client_sock = accept(server_sock, (struct sockaddr *) &echoClntAddr, &clntLen)) == KS_SOCK_INVALID) {
			status = KS_STATUS_FAIL;
			goto end;
		}

		callback(server_sock, client_sock, &echoClntAddr);
	}

  end:

	if (server_sock != KS_SOCK_INVALID) {
		closesocket(server_sock);
		server_sock = KS_SOCK_INVALID;
	}

	return status;

}

#if 0
KS_DECLARE(ks_status_t) ks_listen_threaded(const char *host, ks_port_t port, ks_listen_callback_t callback, int max)
{
	ks_socket_t server_sock = KS_SOCK_INVALID;
	struct sockaddr_in addr;
	ks_status_t status = KS_STATUS_SUCCESS;
	struct thread_handler *handler = NULL;

	if ((server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return KS_STATUS_FAIL;
	}

	ks_socket_reuseaddr(server_sock);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(server_sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		status = KS_STATUS_FAIL;
		goto end;
	}

	if (listen(server_sock, max) < 0) {
		status = KS_STATUS_FAIL;
		goto end;
	}

	for (;;) {
		int client_sock;
		struct sockaddr_in echoClntAddr;
#ifdef WIN32
		int clntLen;
#else
		unsigned int clntLen;
#endif

		clntLen = sizeof(echoClntAddr);

		if ((client_sock = accept(server_sock, (struct sockaddr *) &echoClntAddr, &clntLen)) == KS_SOCK_INVALID) {
			status = KS_STATUS_FAIL;
			goto end;
		}

		handler = malloc(sizeof(*handler));
		ks_assert(handler);

		memset(handler, 0, sizeof(*handler));
		handler->callback = callback;
		handler->server_sock = server_sock;
		handler->client_sock = client_sock;
		handler->addr = echoClntAddr;

		ks_thread_create_detached(client_thread, handler);
	}

  end:

	if (server_sock != KS_SOCK_INVALID) {
		closesocket(server_sock);
		server_sock = KS_SOCK_INVALID;
	}

	return status;

}
#endif

/* USE WSAPoll on vista or higher */
#ifdef KS_USE_WSAPOLL
KS_DECLARE(int) ks_wait_sock(ks_socket_t sock, uint32_t ms, ks_poll_t flags)
{
}
#endif

#ifdef KS_USE_SELECT
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 6262 )	/* warning C6262: Function uses '98348' bytes of stack: exceeds /analyze:stacksize'16384'. Consider moving some data to heap */
#endif
KS_DECLARE(int) ks_wait_sock(ks_socket_t sock, uint32_t ms, ks_poll_t flags)
{
	int s = 0, r = 0;
	fd_set rfds;
	fd_set wfds;
	fd_set efds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);

#ifndef WIN32
	/* Wouldn't you rather know?? */
	assert(sock <= FD_SETSIZE);
#endif

	if ((flags & KS_POLL_READ)) {

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4548 )
		FD_SET(sock, &rfds);
#pragma warning( pop )
#else
		FD_SET(sock, &rfds);
#endif
	}

	if ((flags & KS_POLL_WRITE)) {

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4548 )
		FD_SET(sock, &wfds);
#pragma warning( pop )
#else
		FD_SET(sock, &wfds);
#endif
	}

	if ((flags & KS_POLL_ERROR)) {

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4548 )
		FD_SET(sock, &efds);
#pragma warning( pop )
#else
		FD_SET(sock, &efds);
#endif
	}

	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * ms;

	s = select((int)sock + 1, (flags & KS_POLL_READ) ? &rfds : NULL, (flags & KS_POLL_WRITE) ? &wfds : NULL, (flags & KS_POLL_ERROR) ? &efds : NULL, &tv);

	if (s < 0) {
		r = s;
	} else if (s > 0) {
		if ((flags & KS_POLL_READ) && FD_ISSET(sock, &rfds)) {
			r |= KS_POLL_READ;
		}

		if ((flags & KS_POLL_WRITE) && FD_ISSET(sock, &wfds)) {
			r |= KS_POLL_WRITE;
		}

		if ((flags & KS_POLL_ERROR) && FD_ISSET(sock, &efds)) {
			r |= KS_POLL_ERROR;
		}
	}

	return r;

}

#ifdef WIN32
#pragma warning( pop )
#endif
#endif

#ifdef KS_USE_POLL
KS_DECLARE(int) ks_wait_sock(ks_socket_t sock, uint32_t ms, ks_poll_t flags)
{
	struct pollfd pfds[2] = { {0} };
	int s = 0, r = 0;

	pfds[0].fd = sock;

	if ((flags & KS_POLL_READ)) {
		pfds[0].events |= POLLIN;
	}

	if ((flags & KS_POLL_WRITE)) {
		pfds[0].events |= POLLOUT;
	}

	if ((flags & KS_POLL_ERROR)) {
		pfds[0].events |= POLLERR;
	}

	s = poll(pfds, 1, ms);

	if (s < 0) {
		r = s;
	} else if (s > 0) {
		if ((pfds[0].revents & POLLIN)) {
			r |= KS_POLL_READ;
		}
		if ((pfds[0].revents & POLLOUT)) {
			r |= KS_POLL_WRITE;
		}
		if ((pfds[0].revents & POLLERR)) {
			r |= KS_POLL_ERROR;
		}
	}

	return r;

}
#endif
