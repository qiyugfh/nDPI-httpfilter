#include <stdio.h>
#include <unistd.h>
#include <pcap.h>
#include <signal.h>
#include <pthread.h>
#include "config.h"
#include "ndpi_util.h"


// used memory counters
u_int32_t current_ndpi_memory = 0, max_ndpi_memory = 0;


const char *project_name = "ndpi-httpfilter";
const char *sniff_devs_key = "sniff_devs";
const char *filter_exp_key = "filter_exp";
char sniff_devs[MAX_SNIFF_DEVS_COUNT][MAX_BUF_LEN] = {0};
// "ether src D8:5D:4C:36:76:83 or ether dst D8:5D:4C:36:76:83";
char filter_exp[MAX_BUF_LEN] = {0};
zlog_category_t *root_category = NULL;
int num_threads = 0;
static int shutdown_app = 0;
static struct ndpi_detection_module_struct *ndpi_info_mod = NULL;


// struct associated to a workflow for a thread
struct reader_thread {
  struct ndpi_workflow *workflow;
  pthread_t pthread;
  u_int64_t last_idle_scan_time;
  u_int32_t idle_scan_idx;
  u_int32_t num_idle_flows;
  struct ndpi_flow_info *idle_flows[IDLE_SCAN_BUDGET];
};

// array for every thread created for a flow
static struct reader_thread ndpi_thread_info[MAX_NUM_READER_THREADS];



static int detect_process(char *process_name)
{
  FILE *fp = NULL;
  char buf[512] = {0};
  char command[128] = {0};
  sprintf(command, "ps -ef | grep -w %s | grep -v grep | wc -l", process_name);

  if((fp = popen(command, "r")) != NULL){
    if(fgets(buf, 512, fp) != NULL){
      int count = atoi(buf);
      if(count >= 2){
        pclose(fp);
        printf("the process '%s' is already running, will not start new one\n", process_name);
        return RET_ERROR;
      }
    }

  pclose(fp);
  }
  return RET_SUCCESS;
}


static zlog_category_t *init_logger(const char *logging_path, const char *cat){
  int rc = zlog_init(logging_path);
  if(rc){
    print_out("init logger failed\n");
  	return NULL;
  }
  
  zlog_category_t *category = zlog_get_category(cat);
  if(!category) {
  	print_out("get logger category %s failed\n", cat);
  	zlog_fini();
  	return NULL;
  }
  return category;
}


static pcap_t *init_pcap(const char *device){
  bpf_u_int32 netp = 0;
  bpf_u_int32 maskp = 0;
  char errbuf[PCAP_ERRBUF_SIZE] = {0};
  if(pcap_lookupnet(device, &netp, &maskp, errbuf) == -1){
    log_error("lookup %s device failed, becase: %s", device, errbuf);
    return NULL;
  }

  pcap_t *pcap = pcap_open_live(device, 262144, 1, 1, errbuf);
  if(pcap == NULL){
  	log_error("cannot open the pcap device %s", device);
    return NULL;
  }

  if(strlen(filter_exp) > 0){
    struct bpf_program fp;
    if(pcap_compile(pcap, &fp, filter_exp, 1, netp) == -1){
      log_error("compile filter expression failed %s, becase: %s", filter_exp, pcap_geterr(pcap));
	  goto error;
	}

	if(pcap_setfilter(pcap, &fp) == -1){
	  log_error("Install filter failed %s", pcap_geterr(pcap));
	  goto error;
	}
  }

return pcap;
error:
  pcap_close(pcap);
  return NULL;
}


static void setup_detection(u_int16_t thread_id, pcap_t *pcap_handle){
  NDPI_PROTOCOL_BITMASK all;
  struct ndpi_workflow_prefs prefs;
  memset(&prefs, 0, sizeof(prefs));
  prefs.decode_tunnels = 0;
  prefs.num_roots = NUM_ROOTS;
  prefs.max_ndpi_flows = MAX_NDPI_FLOWS;
  prefs.quiet_mode = 1;

  memset(&ndpi_thread_info[thread_id], 0, sizeof(ndpi_thread_info[thread_id]));
  ndpi_thread_info[thread_id].workflow = ndpi_workflow_init(&prefs, pcap_handle);

  ndpi_set_detection_preferences(ndpi_thread_info[thread_id].workflow->ndpi_struct, 
  	ndpi_pref_http_dont_dissect_response, 0);
  ndpi_set_detection_preferences(ndpi_thread_info[thread_id].workflow->ndpi_struct, 
  	ndpi_pref_dns_dissect_response, 0);
  ndpi_set_detection_preferences(ndpi_thread_info[thread_id].workflow->ndpi_struct, 
  	ndpi_pref_enable_category_substring_match, 0);

  // enable all protocols
  //NDPI_BITMASK_SET_ALL(all);
  // only detect http protocols
  NDPI_BITMASK_RESET(all);
  NDPI_BITMASK_ADD(all, NDPI_PROTOCOL_HTTP);
  NDPI_BITMASK_ADD(all, NDPI_PROTOCOL_HTTP_CONNECT);
  NDPI_BITMASK_ADD(all, NDPI_PROTOCOL_HTTP_PROXY);
  NDPI_BITMASK_ADD(all, NDPI_PROTOCOL_HTTP_DOWNLOAD);
  NDPI_BITMASK_ADD(all, NDPI_PROTOCOL_HTTP_APPLICATION_ACTIVESYNC);
  ndpi_set_protocol_detection_bitmask2(ndpi_thread_info[thread_id].workflow->ndpi_struct, &all);

}


/* *********************************************** */

/**
 * @brief Idle Scan Walker
 */
static void node_idle_scan_walker(const void *node, ndpi_VISIT which, int depth, void *user_data) {

  struct ndpi_flow_info *flow = *(struct ndpi_flow_info **) node;
  u_int16_t thread_id = *((u_int16_t *) user_data);

  if(ndpi_thread_info[thread_id].num_idle_flows == IDLE_SCAN_BUDGET) /* TODO optimise with a budget-based walk */
    return;

  if((which == ndpi_preorder) || (which == ndpi_leaf)) { /* Avoid walking the same node multiple times */
    if(flow->last_seen + MAX_IDLE_TIME < ndpi_thread_info[thread_id].workflow->last_time) {

      ndpi_free_flow_info_half(flow);
      ndpi_thread_info[thread_id].workflow->stats.ndpi_flow_count--;

      /* adding to a queue (we can't delete it from the tree inline ) */
      ndpi_thread_info[thread_id].idle_flows[ndpi_thread_info[thread_id].num_idle_flows++] = flow;
    }
  }
}


//struct pcap_pkthdr {
//	struct timeval ts; /* time stamp */
//	bpf_u_int32 caplen; /* length of portion present */
//	bpf_u_int32 len; /* length this packet (off wire) */
//};

/**
 * @brief Check pcap packet
 */
static void pcap_process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet){
  struct ndpi_proto p;
  u_int16_t thread_id = *((u_int16_t *)args);
  /* allocate an exact size buffer to check overflows */
  u_char *packet_checked = malloc(header->caplen);

  memcpy(packet_checked, packet, header->caplen);
  p = ndpi_workflow_process_packet(ndpi_thread_info[thread_id].workflow, header, packet_checked);

  
  /* Idle flows cleanup */
  if(ndpi_thread_info[thread_id].last_idle_scan_time + IDLE_SCAN_PERIOD < ndpi_thread_info[thread_id].workflow->last_time){
    /* scan for idle flows */
	ndpi_twalk(ndpi_thread_info[thread_id].workflow->ndpi_flows_root[ndpi_thread_info[thread_id].idle_scan_idx], 
		node_idle_scan_walker, &thread_id);

	/* remive idle flows (unfortunately we cannot do this inline) */
    while(ndpi_thread_info[thread_id].num_idle_flows > 0){
      /*  search and delete the idle flow from the "ndpi_flow_root" (see struct reader thread) - here flows are the node of a b-tree */
      ndpi_tdelete(ndpi_thread_info[thread_id].idle_flows[--ndpi_thread_info[thread_id].num_idle_flows], 
                    &ndpi_thread_info[thread_id].workflow->ndpi_flows_root[ndpi_thread_info[thread_id].idle_scan_idx],
      ndpi_workflow_node_cmp);

	  /* free the memory associated to idle flow in "idle_flows" - (see struct reader thread) */
      // ndpi_free_flow_info_half(ndpi_thread_info[thread_id].idle_flows[ndpi_thread_info[thread_id].num_idle_flows]); 
	  // ndpi_free(ndpi_thread_info[thread_id].idle_flows[ndpi_thread_info[thread_id].num_idle_flows]);
	  ndpi_flow_info_freer(ndpi_thread_info[thread_id].idle_flows[ndpi_thread_info[thread_id].num_idle_flows]);
	}

	if(++ndpi_thread_info[thread_id].idle_scan_idx == ndpi_thread_info[thread_id].workflow->prefs.num_roots){
	  ndpi_thread_info[thread_id].idle_scan_idx = 0;
	}

	ndpi_thread_info[thread_id].last_idle_scan_time = ndpi_thread_info[thread_id].workflow->last_time;
  }

  /*check for buffer changes*/
  if(memcmp(packet, packet_checked, header->caplen) != 0){
    log_error("ingress packet wad modified by nDPI: this should not happen [packetId=%lu, caplen=%u]",
		(unsigned long) ndpi_thread_info[thread_id].workflow->stats.raw_packet_count, header->caplen);
  }
  
  free(packet_checked);
}


static void processing_thread(void * _thread_id){
  int thread_id = (int)_thread_id;
  pcap_t *pcap_handle = ndpi_thread_info[thread_id].workflow->pcap_handle;
  if((!shutdown_app) && (pcap_handle != NULL)){
    log_debug("now start to listen device: %s", sniff_devs[thread_id]);
    pcap_loop(pcap_handle, -1, &pcap_process_packet, (u_char*)&thread_id);
  }
}

static void main_process_do(){
  int thread_id = 0;
  int ret = 0;
  void *thd_res = NULL;

  memset(ndpi_thread_info, 0, sizeof(ndpi_thread_info));

  for(thread_id = 0; thread_id < num_threads; thread_id++){
	pcap_t *pcap = NULL;
    if((pcap = init_pcap(sniff_devs[thread_id])) == NULL){
      exit(RET_ERROR);
	}
	
	setup_detection(thread_id, pcap);
  }

  for(thread_id = 0; thread_id < num_threads; thread_id++){
    ret = pthread_create(&ndpi_thread_info[thread_id].pthread, NULL, processing_thread, (void *) thread_id);
    if(ret != 0){
	  log_error("error on create %d thread", thread_id);
	  exit(RET_ERROR);
    }
  }

  for(thread_id = 0; thread_id < num_threads; thread_id++){
  	ret = pthread_join(ndpi_thread_info[thread_id].pthread, &thd_res);
	if(ret != 0){
      log_error("error on join %d thread", thread_id);
	  exit(RET_ERROR);
	}

	if(thd_res != NULL){
      log_error("error on returned value of %d joined thread", thread_id);
	  exit(RET_ERROR);
	}
  }
}


static void terminate_detection(u_int16_t thread_id){
  ndpi_workflow_free(ndpi_thread_info[thread_id].workflow);
}

static void threads_terminate(){
  for(int thread_id = 0; thread_id < num_threads; thread_id++){
	pcap_t *pcap = ndpi_thread_info[thread_id].workflow->pcap_handle;
    if(pcap != NULL){
	  pcap_breakloop(pcap);	
      pcap_close(pcap);
	}

	terminate_detection(thread_id);
  }
}

static void main_process_terminate(int sid){
  log_debug("process %ld recive the ternimate signal %d, now exit ...\n", (long)getpid(), sid);
  shutdown_app = 1;
  threads_terminate();
  exit(RET_SUCCESS);
}


int main(int argc, char **argv){
#if 0
  if(detect_process(project_name) != RET_SUCCESS){
    return RET_ERROR;
  }  	
#endif

  char *config_path = "../ndpi-httpfilter.conf";
  char *logging_path = "../ndpi-logging.conf";

  root_category= init_logger(logging_path, "main");
  if(root_category == NULL){
    return RET_ERROR;
  }
  
  if(get_sniff_devs_from_file(config_path) != RET_SUCCESS){
  	return RET_ERROR;
  }

  // sett number of threads = number of interfaces
  num_threads = 0;
  for(; num_threads < MAX_SNIFF_DEVS_COUNT; num_threads++){
	if(strlen(sniff_devs[num_threads]) == 0){
	  break;
	}
  }
  log_debug("the threads number is %d", num_threads);
  
  if(num_threads < 1 || num_threads > MAX_SNIFF_DEVS_COUNT)
  {
  	log_warn("The number of threads per process is suggested to be 1-6, now I will set it 1");
	num_threads = 1;
  }
  
  signal(SIGTERM, main_process_terminate);
  signal(SIGINT, main_process_terminate);
 
  log_debug("program start ...");

  main_process_do();

  return RET_SUCCESS;
}


