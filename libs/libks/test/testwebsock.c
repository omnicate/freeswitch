#include <ks.h>
#include <tap.h>

static char v4[48] = "";
static char v6[48] = "";
static int mask = 0;
static int tcp_port = 8081;


static char __MSG[] = "TESTING................................................................................/TESTING";

struct tcp_data {
	ks_socket_t sock;
	ks_sockaddr_t addr;
	int ready;
	char *ip;
};

void server_callback(ks_socket_t server_sock, ks_socket_t client_sock, ks_sockaddr_t *addr, void *user_data)
{
	//struct tcp_data *tcp_data = (struct tcp_data *) user_data;
	char buf[8192] = "";
	ks_status_t status;
	ks_size_t bytes;

	printf("WS SERVER SOCK %d connection from %s:%u\n", (int)server_sock, addr->host, addr->port);

	do {
		bytes = sizeof(buf);;
		status = ks_socket_recv(client_sock, buf, &bytes);
		if (status != KS_STATUS_SUCCESS) {
			printf("WS SERVER BAIL %s\n", strerror(ks_errno()));
			break;
		}
		printf("WS SERVER READ %ld bytes [%s]\n", (long)bytes, buf);
	} while(zstr_buf(buf) || strcmp(buf, __MSG));

	bytes = strlen(buf);
	status = ks_socket_send(client_sock, buf, &bytes);
	printf("WS SERVER WRITE %ld bytes\n", (long)bytes);

	ks_socket_close(&client_sock);

	printf("WS SERVER COMPLETE\n");
}



static void *tcp_sock_server(ks_thread_t *thread, void *thread_data)
{
	struct tcp_data *tcp_data = (struct tcp_data *) thread_data;

	tcp_data->ready = 1;
	ks_listen_sock(tcp_data->sock, &tcp_data->addr, 0, server_callback, tcp_data);

	printf("WS THREAD DONE\n");

	return NULL;
}


static int test_ws(char *ip, int ssl)
{
	ks_thread_t *thread_p = NULL;
	ks_pool_t *pool;
	ks_sockaddr_t addr;
	int family = AF_INET;
	ks_socket_t cl_sock = KS_SOCK_INVALID;
	char buf[8192] = "";
	struct tcp_data tcp_data = { 0 };
	int r = 1, sanity = 100;
	kws_t *kws = NULL;

	ks_pool_open(&pool);

	if (strchr(ip, ':')) {
		family = AF_INET6;
	}

	if (ks_addr_set(&tcp_data.addr, ip, tcp_port, family) != KS_STATUS_SUCCESS) {
		r = 0;
		printf("WS CLIENT Can't set ADDR\n");
		goto end;
	}
	
	if ((tcp_data.sock = socket(family, SOCK_STREAM, IPPROTO_TCP)) == KS_SOCK_INVALID) {
		r = 0;
		printf("WS CLIENT Can't create sock family %d\n", family);
		goto end;
	}

	ks_socket_option(tcp_data.sock, SO_REUSEADDR, KS_TRUE);
	ks_socket_option(tcp_data.sock, TCP_NODELAY, KS_TRUE);

	tcp_data.ip = ip;

	ks_thread_create(&thread_p, tcp_sock_server, &tcp_data, pool);

	while(!tcp_data.ready && --sanity > 0) {
		ks_sleep(10000);
	}

	ks_addr_set(&addr, ip, tcp_port, family);
	cl_sock = ks_socket_connect(SOCK_STREAM, IPPROTO_TCP, &addr);
	
	int x;

	printf("WS CLIENT SOCKET %d %s %d\n", (int)cl_sock, addr.host, addr.port);

	kws_init(&kws, cl_sock, NULL, "/verto:tatooine.freeswitch.org:verto", KWS_BLOCK, pool);

	char json_text[] = "{lame: 12}";
	kws_write_frame(kws, WSOC_TEXT, json_text, strlen(json_text));

	kws_opcode_t oc;
	uint8_t *data;
	ks_ssize_t bytes;

	bytes = kws_read_frame(kws, &oc, &data);
	printf("WTF [%ld][%s]\n", bytes, data);

	sleep(10);

	x = write((int)cl_sock, __MSG, (unsigned)strlen(__MSG));
	printf("WS CLIENT WRITE %d bytes\n", x);
	
	x = read((int)cl_sock, buf, sizeof(buf));
	printf("WS CLIENT READ %d bytes [%s]\n", x, buf);
	
 end:

	if (tcp_data.sock != KS_SOCK_INVALID) {
		ks_socket_shutdown(tcp_data.sock, 2);
		ks_socket_close(&tcp_data.sock);
	}

	if (thread_p) {
		ks_thread_join(thread_p);
	}

	ks_socket_close(&cl_sock);

	ks_pool_close(&pool);

	return r;
}



int main(void)
{
	int have_v4 = 0, have_v6 = 0;
	ks_find_local_ip(v4, sizeof(v4), &mask, AF_INET);
	ks_find_local_ip(v6, sizeof(v6), NULL, AF_INET6);
	
	printf("IPS: v4: [%s] v6: [%s]\n", v4, v6);

	have_v4 = zstr_buf(v4) ? 0 : 1;
	have_v6 = zstr_buf(v6) ? 0 : 1;

	plan((have_v4) + (have_v6) + 1);

	ok(have_v4 || have_v6);

	if (have_v4) {
		ok(test_ws(v4, 0));
	}

	if (have_v6) {
		ok(test_ws(v6, 0));
	}


	return 0;
}
