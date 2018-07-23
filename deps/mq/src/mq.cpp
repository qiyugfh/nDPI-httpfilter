#include "mq.h"
#include "mqclient.h"
#include "CBase64Code.h"
#include "packet.h"
#include <sstream>



MQClient *client = NULL;
std::string brokerURI = "failover://(tcp://127.0.0.1:61616)";
std::string errInfo = "the mqclient point is NULL";
bool useTopic = false;


const int DELIVERY_MODE = 0;
const int PRIORITY = 4;
// the message's expire time is 1 minutes
const int EXPIRE_TIME_MS = 60000;



int initializeClient(const char *queue){
    if(queue == NULL || strlen(queue) == 0){
        printf("queue name cannot be NULL\n");
        return -1;
    }

    activemq::library::ActiveMQCPP::initializeLibrary();

    client = new MQClient(brokerURI, queue, useTopic);
    if(client->open() == -1){
		printf("can't open mq client");
        return -1;
    }

    return 0;
}


int sendMessage(uint64_t tv_sec, uint64_t tv_usec, uint32_t caplen, uint32_t len, const char *packet_data){
    if(client == NULL){
        printf("%s\n", errInfo);
        return -1;
    }

    std::stringstream ss;
    ss.clear();
    ss.str("");

    boost::archive::text_oarchive oa(ss);

    char *buffer = (char *)malloc(caplen * 2);
    memset(buffer, 0, caplen * 2);
    int n = Base64_Encode(buffer, packet_data, caplen);
    Packet packet(tv_sec, tv_usec, caplen, len, buffer);
    oa << packet;
    free(buffer);

    client->sendTask(ss.str(), DELIVERY_MODE, PRIORITY, EXPIRE_TIME_MS);
    return 0;
}

char *receiveMessage(uint64_t *tv_sec, uint64_t *tv_usec, uint32_t *caplen, uint32_t *len){
    if(client == NULL){
        printf("%s\n", errInfo);
        return NULL;
    }

    std::stringstream ss;
    ss.clear();
    ss.str("");
    std::string msg = client->recvTask();
    ss.str(msg);

    boost::archive::text_iarchive ia(ss);
    Packet packet;
    ia >> packet;

    *tv_sec = packet.tv_sec;
    *tv_usec = packet.tv_usec;
    *caplen = packet.caplen;
    *len = packet.len;

    char *packet_data = (char *)malloc(packet.caplen * 2);
    memset(packet_data, 0, packet.caplen * 2);
    Base64_Decode(packet_data, packet.buffer.c_str(), packet.buffer.length());
    return packet_data;
}

int shutdownClient(){
    if(client == NULL){
        printf("%s\n", errInfo);
        return -1;
    }

    client->close();
    delete client;
    client = NULL;

    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}
