#define _GNU_SOURCE 
#include <sched.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pcap.h>
#include <signal.h>
#include <pthread.h>
#include "config.h"
#include "ndpi_util.h"
#include "mq.h"


// used memory counters
u_int32_t current_ndpi_memory = 0, max_ndpi_memory = 0;


const char *project_name = "ndpi-httpfilter";
const char *sniff_devs_key = "sniff_devs";
const char *filter_exp_key = "filter_exp";
char sniff_devs[MAX_SNIFF_DEVS_COUNT][MAX_BUF_LEN] = {0};
// "ether src D8:5D:4C:36:76:83 or ether dst D8:5D:4C:36:76:83";
char filter_exp[MAX_BUF_LEN] = {0};
zlog_category_t *root_category = NULL;
int num_process = 0;
int num_cpu = 1;
static int shutdown_app = 0;
static struct ndpi_detection_module_struct *ndpi_info_mod = NULL;


// struct associated to a workflow for a process
struct reader_process {
  struct ndpi_workflow *workflow;
  pthread_t pthread[MAX_NUM_PCAP_THREAD];
  u_int64_t last_idle_scan_time;
  u_int32_t idle_scan_idx;
  u_int32_t num_idle_flows;
  struct ndpi_flow_info *idle_flows[IDLE_SCAN_BUDGET];
};

// array for every process created for a flow
static struct reader_process ndpi_process_info[MAX_NUM_READER_PROCESS];



static int detect_process(char *process_name){
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

static int set_cpu(int cpu){
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);
  log_debug("thread %lu, cpu = %d", (unsigned long)pthread_self(), cpu);
  if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) == -1){
	log_error("set cpu error");
	return RET_ERROR;
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


static void setup_detection(u_int16_t process_id, pcap_t *pcap_handle){
  NDPI_PROTOCOL_BITMASK all;
  struct ndpi_workflow_prefs prefs;
  memset(&prefs, 0, sizeof(prefs));
  prefs.decode_tunnels = 0;
  prefs.num_roots = NUM_ROOTS;
  prefs.max_ndpi_flows = MAX_NDPI_FLOWS;
  prefs.quiet_mode = 1;

  memset(&ndpi_process_info[process_id], 0, sizeof(ndpi_process_info[process_id]));
  ndpi_process_info[process_id].workflow = ndpi_workflow_init(&prefs, pcap_handle);

  ndpi_set_detection_preferences(ndpi_process_info[process_id].workflow->ndpi_struct, 
  	ndpi_pref_http_dont_dissect_response, 0);
  ndpi_set_detection_preferences(ndpi_process_info[process_id].workflow->ndpi_struct, 
  	ndpi_pref_dns_dissect_response, 0);
  ndpi_set_detection_preferences(ndpi_process_info[process_id].workflow->ndpi_struct, 
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
  ndpi_set_protocol_detection_bitmask2(ndpi_process_info[process_id].workflow->ndpi_struct, &all);

}


/* *********************************************** */

/**
 * @brief Idle Scan Walker
 */
static void node_idle_scan_walker(const void *node, ndpi_VISIT which, int depth, void *user_data) {

  struct ndpi_flow_info *flow = *(struct ndpi_flow_info **) node;
  u_int16_t process_id = *((u_int16_t *) user_data);

  if(ndpi_process_info[process_id].num_idle_flows == IDLE_SCAN_BUDGET) /* TODO optimise with a budget-based walk */
    return;

  if((which == ndpi_preorder) || (which == ndpi_leaf)) { /* Avoid walking the same node multiple times */
    if(flow->last_seen + MAX_IDLE_TIME < ndpi_process_info[process_id].workflow->last_time) {

      ndpi_free_flow_info_half(flow);
      ndpi_process_info[process_id].workflow->stats.ndpi_flow_count--;

      /* adding to a queue (we can't delete it from the tree inline ) */
      ndpi_process_info[process_id].idle_flows[ndpi_process_info[process_id].num_idle_flows++] = flow;
    }
  }
}


static void pcap_caputre_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet){
	u_int16_t process_id = *((u_int16_t *)args);
	sendMessage(header->ts.tv_sec, header->ts.tv_usec, header->caplen, header->len, packet);
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
  u_int16_t process_id = *((u_int16_t *)args);
  /* allocate an exact size buffer to check overflows */
  u_char *packet_checked = malloc(header->caplen);

  memcpy(packet_checked, packet, header->caplen);
  p = ndpi_workflow_process_packet(ndpi_process_info[process_id].workflow, header, packet_checked);

  
  /* Idle flows cleanup */
  if(ndpi_process_info[process_id].last_idle_scan_time + IDLE_SCAN_PERIOD < ndpi_process_info[process_id].workflow->last_time){
    /* scan for idle flows */
	ndpi_twalk(ndpi_process_info[process_id].workflow->ndpi_flows_root[ndpi_process_info[process_id].idle_scan_idx], 
		node_idle_scan_walker, &process_id);

	/* remive idle flows (unfortunately we cannot do this inline) */
    while(ndpi_process_info[process_id].num_idle_flows > 0){
      /*  search and delete the idle flow from the "ndpi_flow_root" (see struct reader thread) - here flows are the node of a b-tree */
      ndpi_tdelete(ndpi_process_info[process_id].idle_flows[--ndpi_process_info[process_id].num_idle_flows], 
                    &ndpi_process_info[process_id].workflow->ndpi_flows_root[ndpi_process_info[process_id].idle_scan_idx],
      ndpi_workflow_node_cmp);

	  /* free the memory associated to idle flow in "idle_flows" - (see struct reader thread) */
      // ndpi_free_flow_info_half(ndpi_thread_info[thread_id].idle_flows[ndpi_thread_info[thread_id].num_idle_flows]); 
	  // ndpi_free(ndpi_thread_info[thread_id].idle_flows[ndpi_thread_info[thread_id].num_idle_flows]);
	  ndpi_flow_info_freer(ndpi_process_info[process_id].idle_flows[ndpi_process_info[process_id].num_idle_flows]);
	}

	if(++ndpi_process_info[process_id].idle_scan_idx == ndpi_process_info[process_id].workflow->prefs.num_roots){
	  ndpi_process_info[process_id].idle_scan_idx = 0;
	}

	ndpi_process_info[process_id].last_idle_scan_time = ndpi_process_info[process_id].workflow->last_time;
  }

  /*check for buffer changes*/
  if(memcmp(packet, packet_checked, header->caplen) != 0){
    log_error("ingress packet wad modified by nDPI: this should not happen [packetId=%lu, caplen=%u]",
		(unsigned long) ndpi_process_info[process_id].workflow->stats.raw_packet_count, header->caplen);
  }
  
  free(packet_checked);
}


static void caputring_packet_thread(void * _process_id){
  int process_id = *((int *)_process_id);
  log_debug("process %d capture packet thread %lu start to work", process_id, pthread_self());

  int cpu = (process_id * 2) % num_cpu;
  set_cpu(cpu);
  
  pcap_t *pcap_handle = ndpi_process_info[process_id].workflow->pcap_handle;
  if((!shutdown_app) && (pcap_handle != NULL)){
    log_debug("now start to listen device: %s", sniff_devs[process_id]);
    pcap_loop(pcap_handle, -1, &pcap_caputre_packet, (u_char*)&process_id);
  }
}


static void processing_packet_thread(void * _process_id){
  int process_id = *((int *)_process_id);
  log_debug("process %d analyze packet thread %lu start to work", process_id, pthread_self());
  int cpu = (process_id * 2 + 1) % num_cpu;
  set_cpu(cpu);

  pcap_t *pcap_handle = ndpi_process_info[process_id].workflow->pcap_handle;
  log_debug("now start to process device %s packet", sniff_devs[process_id]);
  while((!shutdown_app) && (pcap_handle != NULL)){
    struct pcap_pkthdr header;
	u_char *packet = receiveMessage(&header.ts.tv_sec, &header.ts.tv_usec, &header.caplen, &header.len);
	pcap_process_packet(_process_id, &header, packet);
	free(packet);
  }
}

static void child_process_do(int process_id){
  int ret = 0;
  void *thd_res = NULL;

  pcap_t *pcap = NULL;
  if((pcap = init_pcap(sniff_devs[process_id])) == NULL){
    exit(RET_ERROR);
  }
  
  setup_detection(process_id, pcap);
  if(initializeClient(sniff_devs[process_id]) == -1){
    log_error("can't open activemq client");
	exit(RET_ERROR);
  }
  
  
  ret = pthread_create(&ndpi_process_info[process_id].pthread[0], NULL, (void *)caputring_packet_thread, (void *) &process_id);
  if(ret != 0){
    log_error("error on create %d caputre packet thread", process_id);
    exit(RET_ERROR);
  }
  log_debug("process %d thread %lu create success", process_id, ndpi_process_info[process_id].pthread[0]);
  
  ret = pthread_create(&ndpi_process_info[process_id].pthread[1], NULL, (void *)processing_packet_thread, (void *) &process_id);
  if(ret != 0){
    log_error("error on create %d prcess packet thread", process_id);
    exit(RET_ERROR);
  }
  log_debug("process %d thread %lu create success", process_id, ndpi_process_info[process_id].pthread[1]);
  
  for(int thread_id = 0; thread_id < MAX_NUM_PCAP_THREAD; thread_id++){
    ret = pthread_join(ndpi_process_info[process_id].pthread[thread_id], &thd_res);
    if(ret != 0){
  	log_error("error on join process %d, thread %d", process_id, thread_id);
  	exit(RET_ERROR);
    }
  
    if(thd_res != NULL){
  	log_error("error on returned value when join process %d, thread %d", process_id, thread_id);
  	exit(RET_ERROR);
    }
  }

}

static void main_process_do(){
  int process_id = 0;
  int ret = 0;
  void *thd_res = NULL;

  memset(ndpi_process_info, 0, sizeof(ndpi_process_info));

  for(; process_id < num_process; process_id++){
    pid_t pid;
	if((pid = fork()) < 0){
      log_error("fork child process %d error", process_id);
	  exit(RET_ERROR);
	}else if(pid == 0){
	   log_debug("child process %d begin to work", process_id);
	   child_process_do(process_id);
	   exit(RET_SUCCESS);
	}else{
	   log_debug("child process %d, pid = %d", process_id, pid);
	}
  }
}


static void terminate_detection(u_int16_t process_id){
  ndpi_workflow_free(ndpi_process_info[process_id].workflow);
  shutdownClient();
}

static void threads_terminate(){
  for(int process_id = 0; process_id < num_process; process_id++){
	pcap_t *pcap = ndpi_process_info[process_id].workflow->pcap_handle;
    if(pcap != NULL){
	  pcap_breakloop(pcap);	
      pcap_close(pcap);
	}

	terminate_detection(process_id);
  }
}

static void main_process_terminate(int sid){
  log_debug("process %ld recive the ternimate signal %d, now exit ...\n", (long)getpid(), sid);
  zlog_close();

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

  char root_path[514] = {0};
  char config_path[512] = {0};
  char logging_path[512] = {0};
  // get current exe execute path
  if(readlink("/proc/self/exe", root_path, sizeof(root_path)) <= 0){
    return RET_ERROR;
  }
  char *path_end = strrchr(root_path, '/');
  if(path_end == NULL){
    return RET_ERROR;
  }	
  *path_end = '\0';

  sprintf(config_path, "%s/ndpi-httpfilter.conf", root_path);
  sprintf(logging_path, "%s/ndpi-logging.conf", root_path);
  printf("ndpi http filter configure file path: %s\n", config_path);
  printf("ndpi log configure file path: %s\n", logging_path);
  
  root_category= init_logger(logging_path, "main");
  if(root_category == NULL){
    return RET_ERROR;
  }

  log_debug("program start ...");
  
  if(get_sniff_devs_from_file(config_path) != RET_SUCCESS){
  	return RET_ERROR;
  }

  num_cpu= sysconf(_SC_NPROCESSORS_CONF);
  log_debug("cpu's number is %d", num_cpu);

  // sett number of threads = number of interfaces
  num_process = 0;
  for(; num_process < MAX_SNIFF_DEVS_COUNT; num_process++){
	if(strlen(sniff_devs[num_process]) == 0){
	  break;
	}
  }
  log_debug("child process's number is %d", num_process);
  
  if(num_process < 1 || num_process > MAX_SNIFF_DEVS_COUNT)
  {
  	log_warn("The number of child process is suggested to be 1-6, now I will set it 1");
	num_process = 1;
  }
  
  signal(SIGTERM, main_process_terminate);
  signal(SIGINT, main_process_terminate);

  main_process_do();

  return RET_SUCCESS;
}


