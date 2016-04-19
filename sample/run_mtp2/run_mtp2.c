/*
 * Copyright (c) 2011, Moises Silva <moy@sangoma.com>
 * Sangoma Technologies
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
 *
 *
 * Contributors: David Yat Sin <dyatsin@sangoma.com>
 */

#include "run_mtp2.h"

#ifndef LIBXML_TREE_ENABLED
#error "XML tree support is required"
#endif

int g_running = 1;
int g_num_threads = 0;

void on_sigint(int signal);
void on_sigusr(int signal);

span_data_t SPANS[FTDM_MAX_SPANS_INTERFACE] = {{0}};

static ftdm_hash_t *ss7_configs;

static xmlNode *xml_child(xmlNode *node, const char *child_name)
{
	xmlNode *cur_node = NULL;
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		printf("node %s = %s\n", cur_node->name, child_name);
		if (cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (xmlChar *)child_name)) {
			printf("node %s found in node %p\n", child_name, node);
			return cur_node;
		}
	}
	printf("node %s not found in node %p\n", child_name, node);
	return NULL;
}

static int add_config_list_nodes(xmlNode *swnode,
								ftdm_conf_node_t *rootnode,
		 						const char *list_name,
		 						const char *list_element_name,
		 						const char *sub_list_name,
		 						const char *sub_list_element_name)
{
	xmlChar *var, *val;
	xmlNode *list;
	xmlNode *element;
	xmlNode *param;

	ftdm_conf_node_t *n_list;
	ftdm_conf_node_t *n_element;

	list = xml_child(swnode, list_name);
	if (!list) {
		fprintf(stderr, "no list %s found\n", list_name);
		return -1;
	}

	if ((FTDM_SUCCESS != ftdm_conf_node_create(list_name, &n_list, rootnode))) {
		fprintf(stderr, "failed to create %s node\n", list_name);
		return -1;
	}

	for (element = xml_child(list, list_element_name); element; element = element->next) {
		char *element_name = (char *)xmlGetProp(element, (xmlChar *)"name");

		if (!element_name) {
			continue;
		}

		if ((FTDM_SUCCESS != ftdm_conf_node_create(list_element_name, &n_element, n_list))) {
			fprintf(stderr, "failed to create %s node for %s\n", list_element_name, element_name);
			return -1;
		}
		ftdm_conf_node_add_param(n_element, "name", element_name);

		for (param = xml_child(element, "param"); param; param = param->next) {
			var = xmlGetProp(param, (xmlChar *)"name");
			val = xmlGetProp(param, (xmlChar *)"value");
			if (!var || !val) {
				continue;
			}
			ftdm_conf_node_add_param(n_element, (char *)var, (char *)val);
		}

		if (sub_list_name && sub_list_element_name) {
			if (add_config_list_nodes(element, n_element, sub_list_name, sub_list_element_name, NULL, NULL)) {
				return -1;
			}
		}
	}

	return 0;
}

static ftdm_conf_node_t *get_ss7_config_node(xmlNode *cfg, char *confname)
{
	xmlNode *signode, *ss7configs, *isup, *gen, *param;
	ftdm_conf_node_t *rootnode, *list;
	char *var, *val;	

	/* try to find the conf in the hash first */
	rootnode = hashtable_search(ss7_configs, (void *)confname);
	if (rootnode) {
		fprintf(stdout, "ss7 config %s was found in the hash already\n", confname);
		return rootnode;
	}
	fprintf(stdout, "not found %s config in hash, searching in xml ...\n", confname);

	signode = xml_child(cfg, "signaling_configs");
	if (!signode) {
		fprintf(stderr, "not found 'signaling_configs' XML config section\n");
		return NULL;
	}

	ss7configs = xml_child(signode, "sngss7_configs");
	if (!ss7configs) {
		fprintf(stderr, "not found 'sngss7_configs' XML config section\n");
		return NULL;
	}

	/* search the isup config */
	for (isup = xml_child(ss7configs, "sng_isup"); isup; isup = isup->next) {
		char *name = (char *)xmlGetProp(isup, (xmlChar *)"name");
		if (!name) {
			continue;
		}
		if (!strcasecmp(name, confname)) {
			break;
		}
	}

	if (!isup) {
		fprintf(stderr, "not found '%s' sng_isup XML config section\n", confname);
		return NULL;
	}

	/* found our XML chunk, create the root node */
	if ((FTDM_SUCCESS != ftdm_conf_node_create("sng_isup", &rootnode, NULL))) {
		fprintf(stderr, "failed to create root node for sng_isup config %s\n", confname);
		return NULL;
	}

	/* add sng_gen */
	gen = xml_child(isup, "sng_gen");
	if (gen == NULL) {
		fprintf(stderr, "failed to process sng_gen for sng_isup config %s\n", confname);
		ftdm_conf_node_destroy(rootnode);
		return NULL;
	}

	if ((FTDM_SUCCESS != ftdm_conf_node_create("sng_gen", &list, rootnode))) {
		fprintf(stderr, "failed to create %s node for %s\n", "sng_gen", confname);
		ftdm_conf_node_destroy(rootnode);
		return NULL;
	}

	for (param = xml_child(gen, "param"); param; param = param->next) {
		var = (char *)xmlGetProp(param, (xmlChar *)"name");
		val = (char *)xmlGetProp(param, (xmlChar *)"value");
		if (!var || !val) {
			continue;
		}
		ftdm_conf_node_add_param(list, var, val);
	}

	/* add mtp1 links */
	if (add_config_list_nodes(isup, rootnode, "mtp1_links", "mtp1_link", NULL, NULL)) {
		fprintf(stderr, "failed to process mtp1_links for sng_isup config %s\n", confname);
		ftdm_conf_node_destroy(rootnode);
		return NULL;
	}

	/* add mtp2 links */
	if (add_config_list_nodes(isup, rootnode, "mtp2_links", "mtp2_link", NULL, NULL)) {
		fprintf(stderr, "failed to process mtp2_links for sng_isup config %s\n", confname);
		ftdm_conf_node_destroy(rootnode);
		return NULL;
	}

	hashtable_insert(ss7_configs, confname, rootnode, HASHTABLE_FLAG_NONE);

	fprintf(stdout, "Added SS7 node configuration %s\n", confname);
	return rootnode;
}


static int load_config(const char *cf)
{
	xmlDoc *doc = NULL;
	xmlNode *cfg, *param, *spans, *myspan;
	ftdm_conf_node_t *ss7confnode = NULL;

	LIBXML_TEST_VERSION

	doc = xmlReadFile(cf, NULL, 0);	
	if (!doc) {
		fprintf(stderr, "failed to parse XML file '%s'\n", cf);
		return -1;

	}

	cfg = xmlDocGetRootElement(doc);

	if ((spans = xml_child(cfg, "sangoma_ss7_spans"))) {
		uint32_t span_id = 0;
		for (myspan = xml_child(spans, "span"); myspan; myspan = myspan->next) {
			ftdm_status_t zstatus = FTDM_FAIL;

			ftdm_conf_parameter_t spanparameters[30];
			char *name = (char *)xmlGetProp(myspan, (xmlChar *)"name");
			char *configname = (char *)xmlGetProp(myspan, (xmlChar *)"cfgprofile");
			ftdm_span_t *span = NULL;
			
			unsigned paramindex = 0;
			
			if (!name) {				
				continue;
			}

			if (!configname) {
				ftdm_log(FTDM_LOG_ERROR, "ss7 span missing required attribute, skipping ...\n");
				continue;
			}
			
			zstatus = ftdm_span_find_by_name(name, &span);
			if (zstatus != FTDM_SUCCESS) {
				ftdm_log(FTDM_LOG_ERROR, "could not find FreeTDM span %s\n", name);
				continue;
			}
			
			ss7confnode = get_ss7_config_node(cfg, configname);
			if (!ss7confnode) {
				ftdm_log(FTDM_LOG_ERROR, "Error finding ss7config '%s' for FreeTDM span: %s\n", configname, name);
				continue;
			}

			memset(spanparameters, 0, sizeof(spanparameters));
			paramindex = 0;
			spanparameters[paramindex].var = "confnode";
			spanparameters[paramindex].ptr = ss7confnode;
			paramindex++;
			for (param = xml_child(myspan, "param"); param; param = param->next) {
				char *var = (char *) xmlGetProp(param, (xmlChar *)"name");
				char *val = (char *) xmlGetProp(param, (xmlChar *)"value");

				if (ftdm_array_len(spanparameters) - 1 == paramindex) {
					fprintf(stderr, "Too many parameters for ss7 span, ignoring any parameter after %s\n", var);
					break;
				}

				spanparameters[paramindex].var = var;
				spanparameters[paramindex].val = val;
				paramindex++;
			}

			if (ftdm_configure_span_signaling(span, "sangoma_ss7", on_mtp2_signal, spanparameters) != FTDM_SUCCESS) {
	 			fprintf(stderr, "Error configuring ss7 FreeTDM span %d\n", span_id);
	 			break;
 			}
			SPANS[span_id].inuse = 1;
 			SPANS[span_id].ftdmspan = span;
			ftdm_span_start(span);
			
			span_id++;
		}
	}

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;
}

void on_sigint(int signal)
{
	g_running = 0;
	return;
}

void on_sigusr(int signal)
{
	switch(signal) {
		case SIGUSR1:
			print_frames();
			break;
		case SIGUSR2:
			print_status();
			break;
	}
	return;
}


#define INC_ARG(arg_i) \
	arg_i++; \
	if (arg_i >= argc) { \
		fprintf(stderr, "No option value was given for option %s (argi = %d, argc = %d)\n", argv[arg_i-1], arg_i, argc); \
		exit(1); \
	}

int main(int argc, char *argv[])
{
	char *conf_location = NULL;
	int arg_i = 0;
	int max_wait = 20;

	memset(SPANS, 0, FTDM_MAX_SPANS_INTERFACE*(sizeof(span_data_t)));

	for (arg_i = 1; arg_i < argc; arg_i++) {
		if (!strcasecmp(argv[arg_i], "-conf")) {
			INC_ARG(arg_i);
			conf_location = argv[arg_i];
		}
	}

	if (!conf_location) {
		fprintf(stderr, "Specify the -conf option with the XML configuration path\n");
		exit(1);
	}

	/* ftdm_global_set_default_logger(FTDM_LOG_LEVEL_DEBUG); */
	ftdm_global_set_default_logger(FTDM_LOG_LEVEL_INFO);
	ftdm_global_set_crash_policy(FTDM_CRASH_ON_ASSERT);
	
	ss7_configs = create_hashtable(16, ftdm_hash_hashfromstring, ftdm_hash_equalkeys);

	if (ftdm_global_init() != FTDM_SUCCESS) {
		fprintf(stderr, "Error loading FreeTDM\n");
		exit(1);
	}

	if (ftdm_global_configuration() != FTDM_SUCCESS) {
		fprintf(stderr, "Error loading FreeTDM base configuration\n");
		exit(1);
	}

	/* load application specific configuration */
	if (load_config(conf_location)) {
		fprintf(stderr, "Failed to load configurations\n");
		ftdm_global_destroy();
		return -1;
	}

	printf("We are running\n");

	signal(SIGINT, on_sigint);
	signal(SIGUSR1, on_sigusr);
	signal(SIGUSR2, on_sigusr);

	test_mtp2_links();

	while (g_running) {
		ftdm_sleep(1000);
	}

	while (g_num_threads && (max_wait-- > 0)) {
		printf("Waiting for all threads to finish (%d)\n", g_num_threads);
		ftdm_sleep(100);
	}
	print_frames();


	ftdm_global_destroy();
	return 0;
}

