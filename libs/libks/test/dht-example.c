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

#include "ks.h"
#include "histedit.h"
#include "sodium.h"

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

void json_cb(struct dht_handle_s *h, const cJSON *msg, void *arg)
{
  char *pretty = cJSON_Print((cJSON *)msg);
  
  printf("Received json msg: %s\n", pretty);

  free(pretty);
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

	  ks_dht_one_loop(h, h->tosleep * 1000);
  }

  return NULL;
}

int
main(int argc, char **argv)
{
  dht_globals_t globals = {0};
    int i, rc;
    //int have_id = 0;
    //char *id_file = "dht-example.id";
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
	int err = 0;
    unsigned char alice_publickey[crypto_box_PUBLICKEYBYTES] = {0};
    unsigned char alice_secretkey[crypto_box_SECRETKEYBYTES] = {0};

    ks_init();

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
			//case 'i':
            //id_file = optarg;
            break;
        default:
            goto usage;
        }
    }

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

	af_flags_t af_flags = 0;

	if (ipv4) {
		af_flags |= KS_DHT_AF_INET4;
	}

	if (ipv6) {
		af_flags |= KS_DHT_AF_INET6;
	}

    /* Init the dht.  This sets the socket into non-blocking mode. */
    rc = dht_init(&h, af_flags, (unsigned char*)"LIBKS");

    if(rc < 0) {
        perror("dht_init");
        exit(1);
    }

	ks_dht_set_port(h, globals.port);
	ks_dht_set_callback(h, callback, NULL);


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
        dht_ping_node(h, (struct sockaddr*)&bootstrap_nodes[i], sizeof(bootstrap_nodes[i]));
        usleep(random() % 100000);
    }

    printf("TESTING!!!\n");
    err = crypto_sign_keypair(alice_publickey, alice_secretkey);
    printf("Result of generating keypair %d\n", err);

    ks_dht_store_entry_json_cb_set(h, json_cb, NULL);
	
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
			} else if (!strncmp(line, "generate_identity", 17)) {
				/* usage: generate_identity [identity key: first_id] */
				/* requires an arg, checks identity hash for arg value. 
				   
				   if found, return already exists.
				   if not found, generate sodium public and private keys, and insert into identities hash.
				 */
			} else if (!strncmp(line, "print_identity_key", 18)) {
				/* usage: print_identity_key [identity key] */
			} else if (!strncmp(line, "message_mutable", 15)) {
			  char *input = strdup(line);
			  char *message_id = input + 16;
			  char *message = NULL;
			  cJSON *output = NULL;
			  int idx = 17; /* this should be the start of the message_id */
			  for ( idx = 17; idx < 100 && input[idx] != '\0'; idx++ ) {
			    if ( input[idx] == ' ' ) {
			      input[idx] = '\0';
			      message = input + 1 + idx;
			      break;
			    }
			  }

			  /* Hack for my testing, so that it chomps the new line. Makes debugging print nicer. */
			  for ( idx++; input[idx] != '\0'; idx++) {
			    if ( input[idx] == '\n' ) {
			      input[idx] = '\0';
			    }
			  }
				/* usage: message_mutable [identity key] [message id: asdf] [your message: Hello from DHT example]*/
				/*
				  takes an identity, a message id(salt) and a message, then sends out the announcement.
				 */
			  output = cJSON_CreateString(message);

			  ks_dht_send_message_mutable_cjson(h, alice_secretkey, alice_publickey,
						      (struct sockaddr*)&bootstrap_nodes[0], sizeof(bootstrap_nodes[0]),
						      message_id, 1, output, 600);
			  free(input);
			  cJSON_Delete(output);
			} else if (!strncmp(line, "message_immutable", 15)) {
			  /* usage: message_immutable [identity key] */
				/*
				  takes an identity, and a message, then sends out the announcement.
				 */
			} else if (!strncmp(line, "message_get", 11)) {
				/* usage: message_get [40 character sha1 digest b64 encoded]*/

				/* MUST RETURN BENCODE OBJECT */
			} else if (!strncmp(line, "message_get_mine", 16)) {
				/* usage: message_get [identity key] [message id: asdf]*/
				/* This looks up the message token from identity key and the message id(aka message salt) */
				
				/* MUST RETURN BENCODE OBJECT */
			} else if (!strncmp(line, "add_buddy", 9)) {
				/* usage: add_buddy [buddy key] [buddy public key] */

			} else if (!strncmp(line, "get_buddy_message", 17)) {
				/* usage: get_buddy_message [buddy key] [buddy message_id] */

				
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
    ks_shutdown();
    return 0;
    
 usage:
    printf("Usage: dht-example [-4] [-6] [-i filename] [-b address]...\n"
           "                   port [address port]...\n");
    exit(0);
}
