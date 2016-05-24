/* This example code was written by Juliusz Chroboczek.
   You are free to cut'n'paste from it to your heart's content. */

/* For crypt */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/signal.h>

#include "ks_dht.h"
#include "histedit.h"

#define MAX_BOOTSTRAP_NODES 20
static struct sockaddr_storage bootstrap_nodes[MAX_BOOTSTRAP_NODES];
static int num_bootstrap_nodes = 0;

/* The call-back function is called by the DHT whenever something
   interesting happens.  Right now, it only happens when we get a new value or
   when a search completes, but this may be extended in future versions. */
static void callback(void *closure, ks_dht_event_t event, const unsigned char *info_hash, const void *data, size_t data_len)
{
  if(event == KS_DHT_EVENT_SEARCH_DONE) {
    printf("Search done.\n");
  } else if(event == KS_DHT_EVENT_VALUES) {
    const uint8_t *bits_8 = data;
    const uint16_t *bits_16 = data;
    
    printf("Received %d values.\n", (int)(data_len / 6));
    printf("Recieved %u.%u.%u.%u:%u\n", bits_8[0], bits_8[1], bits_8[2], bits_8[3], ntohs(bits_16[2]));
  } else {
    printf("Unhandled event %d\n", event);
  }
}

static char * prompt(EditLine *e) {
  return "dht> ";
}

static dht_handle_t *h;
static unsigned char buf[4096];

typedef struct dht_globals_s {
  int s;
  int s6;
  int port;
  int exiting;
} dht_globals_t;

void *dht_event_thread(ks_thread_t *thread, void *data)
{
  time_t tosleep = 0;
  int rc = 0;
  socklen_t fromlen;
  struct sockaddr_storage from;
  dht_globals_t *globals = data;
  
  while(!globals->exiting) {
        struct timeval tv;
        fd_set readfds;
        tv.tv_sec = tosleep;
        tv.tv_usec = random() % 1000000;

        FD_ZERO(&readfds);
        if(globals->s >= 0)
            FD_SET(globals->s, &readfds);
        if(globals->s6 >= 0)
            FD_SET(globals->s6, &readfds);
        rc = select(globals->s > globals->s6 ? globals->s + 1 : globals->s6 + 1, &readfds, NULL, NULL, &tv);
        if(rc < 0) {
            if(errno != EINTR) {
                perror("select");
                sleep(1);
            }
        }
        
        if(rc > 0) {
            fromlen = sizeof(from);
            if(globals->s >= 0 && FD_ISSET(globals->s, &readfds))
                rc = recvfrom(globals->s, buf, sizeof(buf) - 1, 0,
                              (struct sockaddr*)&from, &fromlen);
            else if(globals->s6 >= 0 && FD_ISSET(globals->s6, &readfds))
                rc = recvfrom(globals->s6, buf, sizeof(buf) - 1, 0,
                              (struct sockaddr*)&from, &fromlen);
            else
                abort();
        }

        if(rc > 0) {
            buf[rc] = '\0';
            rc = dht_periodic(h, buf, rc, (struct sockaddr*)&from, fromlen,
                              &tosleep, callback, NULL);
        } else {
            rc = dht_periodic(h, NULL, 0, NULL, 0, &tosleep, callback, NULL);
        }
        if(rc < 0) {
            if(errno == EINTR) {
                continue;
            } else {
                perror("dht_periodic");
                if(rc == EINVAL || rc == EFAULT)
                    abort();
                tosleep = 1;
            }
        }
    }
      return NULL;
}

int
main(int argc, char **argv)
{
  dht_globals_t globals = {0};
    int i, rc, fd;
    int have_id = 0;
    unsigned char myid[20];
    char *id_file = "dht-example.id";
    int opt;
    int ipv4 = 1, ipv6 = 1;
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
    EditLine *el;
    History *myhistory;
    int count;
    const char *line;
    HistEvent ev;
    ks_status_t status;
    static ks_thread_t *threads[1]; /* Main dht event thread */
    ks_pool_t *pool;

    globals.s = -1;
    globals.s6 = -1;
    globals.exiting = 0;
    
    el = el_init("test", stdin, stdout, stderr);
    el_set(el, EL_PROMPT, &prompt);
    el_set(el, EL_EDITOR, "emacs");
    myhistory = history_init();
    history(myhistory, &ev, H_SETSIZE, 800);
    el_set(el, EL_HIST, history, myhistory);
    

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;

    ks_global_set_default_logger(7);

    while(1) {
        opt = getopt(argc, argv, "q46b:i:");
        if(opt < 0)
            break;

        switch(opt) {
        case '4': ipv6 = 0; break;
        case '6': ipv4 = 0; break;
        case 'b': {
            char buf[16];
            int rc;
            rc = inet_pton(AF_INET, optarg, buf);
            if(rc == 1) {
                memcpy(&sin.sin_addr, buf, 4);
                break;
            }
            rc = inet_pton(AF_INET6, optarg, buf);
            if(rc == 1) {
                memcpy(&sin6.sin6_addr, buf, 16);
                break;
            }
            goto usage;
        }
            break;
        case 'i':
            id_file = optarg;
            break;
        default:
            goto usage;
        }
    }

    /* Ids need to be distributed evenly, so you cannot just use your
       bittorrent id.  Either generate it randomly, or take the SHA-1 of
       something. */
    fd = open(id_file, O_RDONLY);
    if(fd >= 0) {
        rc = read(fd, myid, 20);
        if(rc == 20)
            have_id = 1;
        close(fd);
    }
    
    fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0) {
        perror("open(random)");
        exit(1);
    }

    if(!have_id) {
        int ofd;

        rc = read(fd, myid, 20);
        if(rc < 0) {
            perror("read(random)");
            exit(1);
        }
        have_id = 1;
        close(fd);

        ofd = open(id_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(ofd >= 0) {
            rc = write(ofd, myid, 20);
            if(rc < 20)
                unlink(id_file);
            close(ofd);
        }
    }

    {
        unsigned seed;
        read(fd, &seed, sizeof(seed));
        srandom(seed);
    }

    close(fd);

    if(argc < 2)
        goto usage;

    i = optind;

    if(argc < i + 1)
        goto usage;

    globals.port = atoi(argv[i++]);
    if(globals.port <= 0 || globals.port >= 0x10000)
        goto usage;

    while(i < argc) {
        struct addrinfo hints, *info, *infop;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        if(!ipv6)
            hints.ai_family = AF_INET;
        else if(!ipv4)
            hints.ai_family = AF_INET6;
        else
            hints.ai_family = 0;
        rc = getaddrinfo(argv[i], argv[i + 1], &hints, &info);
        if(rc != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
            exit(1);
        } else {
	  fprintf(stderr, "Bootstrapping with node %s:%s\n", argv[i], argv[i+1]);
	}

        i++;
        if(i >= argc)
            goto usage;

        infop = info;
        while(infop) {
            memcpy(&bootstrap_nodes[num_bootstrap_nodes],
                   infop->ai_addr, infop->ai_addrlen);
            infop = infop->ai_next;
            num_bootstrap_nodes++;
        }
        freeaddrinfo(info);

        i++;
    }

    /* We need an IPv4 and an IPv6 socket, bound to a stable port.  Rumour
       has it that uTorrent works better when it is the same as your
       Bittorrent port. */
    if(ipv4) {
        globals.s = socket(PF_INET, SOCK_DGRAM, 0);
        if(globals.s < 0) {
            perror("socket(IPv4)");
        }
    }

    if(ipv6) {
      globals.s6 = socket(PF_INET6, SOCK_DGRAM, 0);
        if(globals.s6 < 0) {
            perror("socket(IPv6)");
        }
    }

    if(globals.s < 0 && globals.s6 < 0) {
        fprintf(stderr, "Eek!");
        exit(1);
    }


    if(globals.s >= 0) {
        sin.sin_port = htons(globals.port);
        rc = bind(globals.s, (struct sockaddr*)&sin, sizeof(sin));
        if(rc < 0) {
            perror("bind(IPv4)");
            exit(1);
        }
    }

    if(globals.s6 >= 0) {
        int rc;
        int val = 1;

        rc = setsockopt(globals.s6, IPPROTO_IPV6, IPV6_V6ONLY,
                        (char *)&val, sizeof(val));
        if(rc < 0) {
            perror("setsockopt(IPV6_V6ONLY)");
            exit(1);
        }

        /* BEP-32 mandates that we should bind this socket to one of our
           global IPv6 addresses.  In this simple example, this only
           happens if the user used the -b flag. */

        sin6.sin6_port = htons(globals.port);
        rc = bind(globals.s6, (struct sockaddr*)&sin6, sizeof(sin6));
        if(rc < 0) {
            perror("bind(IPv6)");
            exit(1);
        }
    }

    /* Init the dht.  This sets the socket into non-blocking mode. */
    rc = dht_init(&h, globals.s, globals.s6, myid, (unsigned char*)"JC\0\0");
    if(rc < 0) {
        perror("dht_init");
        exit(1);
    }

    ks_pool_open(&pool);
    status = ks_thread_create_ex(&threads[0], dht_event_thread, &globals, KS_THREAD_FLAG_DETATCHED, KS_THREAD_DEFAULT_STACK, KS_PRI_NORMAL, pool);

    if ( status != KS_STATUS_SUCCESS) {
		printf("Failed to start DHT event thread\n");
		exit(1);
    }

    /* For bootstrapping, we need an initial list of nodes.  This could be
       hard-wired, but can also be obtained from the nodes key of a torrent
       file, or from the PORT bittorrent message.

       Dht_ping_node is the brutal way of bootstrapping -- it actually
       sends a message to the peer.  If you're going to bootstrap from
       a massive number of nodes (for example because you're restoring from
       a dump) and you already know their ids, it's better to use
       dht_insert_node.  If the ids are incorrect, the DHT will recover. */
    for(i = 0; i < num_bootstrap_nodes; i++) {
        dht_ping_node(h, (struct sockaddr*)&bootstrap_nodes[i],
                      sizeof(bootstrap_nodes[i]));
        usleep(random() % 100000);
    }
    while ( !globals.exiting ) {
		line = el_gets(el, &count);
      
		if (count > 1) {
			int line_len = (int)strlen(line) - 1;
			history(myhistory, &ev, H_ENTER, line);

			if (!strncmp(line, "quit", 4)) {
				globals.exiting = 1;
			} else if (!strncmp(line, "loglevel", 8)) {
				ks_global_set_default_logger(atoi(line + 9));
			} else if (!strncmp(line, "peer_dump", 9)) {
				dht_dump_tables(h, stdout);
			} else if (!strncmp(line, "search", 6)) {
				if ( line_len > 7 ) {
					unsigned char hash[20];
					memcpy(hash, line + 7, 20);

					if(globals.s >= 0) {
						dht_search(h, hash, 0, AF_INET, callback, NULL);
					}
				} else {
					printf("Your search string isn't a valid 20 character hash. You entered [%.*s] of length %d\n", line_len - 7, line + 7, line_len - 7);
				}
			} else if (!strncmp(line, "announce", 8)) {
				if ( line_len == 29 ) {
					unsigned char hash[20];
					memcpy(hash, line + 9, 20);

					if(globals.s >= 0) {
						dht_search(h, hash, globals.port, AF_INET, callback, NULL);
					}
				} else {
					printf("Your search string isn't a valid 20 character hash. You entered [%.*s]\n", line_len - 7, line + 7);
				}
			} else {
				printf("Unknown command entered[%.*s]\n", line_len, line);
			}
		}
    }

    {
        struct sockaddr_in sin[500];
        struct sockaddr_in6 sin6[500];
        int num = 500, num6 = 500;
        int i;
        i = dht_get_nodes(h, sin, &num, sin6, &num6);
        printf("Found %d (%d + %d) good nodes.\n", i, num, num6);
    }


    history_end(myhistory);
    el_end(el);
    dht_uninit(&h);
    return 0;
    
 usage:
    printf("Usage: dht-example [-4] [-6] [-i filename] [-b address]...\n"
           "                   port [address port]...\n");
    exit(1);
}
