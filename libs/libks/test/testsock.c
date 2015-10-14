#include <ks.h>
#include <tap.h>

static char v4[48] = "";
static char v6[48] = "";
static int mask = 0;
static int sv_port = 8090;

static char MSG[] = "TESTING................................................................................/TESTING";

struct data {
	ks_socket_t sock;
	ks_sockaddr_t addr;
	int ready;
	char *ip;
};

void server_callback(ks_socket_t server_sock, ks_socket_t client_sock, ks_sockaddr_t *addr, void *user_data)
{
	//struct data *data = (struct data *) user_data;
	char buf[8192] = "";
	int x = 0;

	printf("SERVER SOCK %d connection from %s:%d\n", server_sock, addr->host, addr->port);

	do {
		x = read(client_sock, buf, sizeof(buf));
		if (x < 0) {
			printf("SERVER BAIL %s\n", strerror(errno));
			break;
		}
		printf("SERVER READ %d bytes [%s]\n", x, buf);
	} while(zstr_buf(buf) || strcmp(buf, MSG));

	x = write(client_sock, buf, strlen(buf));
	printf("SERVER WRITE %d bytes\n", x);

	ks_socket_close(&client_sock);

	printf("SERVER COMPLETE\n");
}



static void *sock_server(ks_thread_t *thread, void *thread_data)
{
	struct data *data = (struct data *) thread_data;

	data->ready = 1;
	ks_listen_sock(data->sock, &data->addr, 0, server_callback, data);

	printf("THREAD DONE\n");

	return NULL;
}


static int testit(char *ip)
{
	ks_thread_t *thread_p = NULL;
	ks_pool_t *pool;
	ks_sockaddr_t addr;
	int family = AF_INET;
	ks_socket_t cl_sock = KS_SOCK_INVALID;
	char buf[8192] = "";
	struct data data = { 0 };
	int r = 1, sanity = 100;

	ks_pool_open(&pool);

	if (strchr(ip, ':')) {
		family = AF_INET6;
	}

	if (ks_addr_set(&data.addr, ip, sv_port, family) != KS_STATUS_SUCCESS) {
		r = 0;
		printf("CLIENT Can't set ADDR\n");
		goto end;
	}
	
	if ((data.sock = socket(family, SOCK_STREAM, IPPROTO_TCP)) == KS_SOCK_INVALID) {
		r = 0;
		printf("CLIENT Can't create sock family %d\n", family);
		goto end;
	}

	ks_socket_option(data.sock, SO_REUSEADDR, KS_TRUE);
	ks_socket_option(data.sock, TCP_NODELAY, KS_TRUE);

	data.ip = ip;

	ks_thread_create(&thread_p, sock_server, &data, pool);

	while(!data.ready && --sanity > 0) {
		ks_sleep(10000);
	}

	ks_addr_set(&addr, ip, sv_port, family);
	cl_sock = ks_socket_connect(SOCK_STREAM, IPPROTO_TCP, &addr);
	
	int x;

	printf("CLIENT SOCKET %d %s %d\n", cl_sock, addr.host, addr.port);

	x = write(cl_sock, MSG, strlen(MSG));
	printf("CLIENT WRITE %d bytes\n", x);
	
	x = read(cl_sock, buf, sizeof(buf));
	printf("CLIENT READ %d bytes\n", x);
	
 end:

	if (data.sock != KS_SOCK_INVALID) {
		ks_socket_shutdown(data.sock, 2);
		ks_socket_close(&data.sock);
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

	plan(have_v4 + have_v6 + 1);

	ok(have_v4 || have_v6);

	if (have_v4) {
		ok(testit(v4));
	}

	if (have_v6) {
		ok(testit(v6));
	}


	return 0;
}
