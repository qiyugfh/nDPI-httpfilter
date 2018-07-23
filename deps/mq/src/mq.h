#ifndef _MQ_H_
#define _MQ_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif	

int initializeClient(const char *queue);

int sendMessage(uint64_t tv_sec, uint64_t tv_usec, uint32_t caplen, uint32_t len, const char *packet_data);

char *receiveMessage(uint64_t *tv_sec, uint64_t *tv_usec, uint32_t *caplen,uint32_t *len);

int shutdownClient(); 	

#ifdef __cplusplus	
}
#endif


#endif //_MQ_H_


