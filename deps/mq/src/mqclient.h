#ifndef _MQ_CLIENT_H_
#define _MQ_CLIENT_H_

#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/core/ActiveMQConnection.h>
#include <activemq/transport/DefaultTransportListener.h>
#include <activemq/library/ActiveMQCPP.h>
#include <decaf/lang/Integer.h>
#include <activemq/util/Config.h>
#include <decaf/util/Date.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <stdlib.h>
#include <stdio.h>


using namespace activemq;
using namespace activemq::core;
using namespace activemq::transport;
using namespace decaf;
using namespace decaf::lang;
using namespace decaf::util;
using namespace decaf::util::concurrent;
using namespace cms;
using namespace std;



class MQClient : public ExceptionListener, public DefaultTransportListener {
private:
    Connection* connection;
    Session* session;
    Destination* destination;
    MessageProducer* producer;
    MessageConsumer* consumer;
    bool useTopic;
    bool clientAck;
    std::string brokerURI;
    std::string destURI;

private:
    MQClient(const MQClient &);
    MQClient & operator=(const MQClient &);

public:

    MQClient(const std::string &_brokerURI, const std::string &_destURI,
             bool _useTopic = false, bool _clientAck = false);

    virtual ~MQClient();

    int open();
    void close();
    string recvTask();
    /**
     * 往队列里发送消息
     * deliveryMode为是否持久化，0为持久化，1为非持久化，默认为1，若要使用优先级，则要设置为0
     * priority为消息优先级，优先级为0～9，0为最低，9为最高，默认为4
     * timeToLive为消息过期时间，单位毫秒，默认为0表示永不过期
     */
    void sendTask(const string &msg, int deliveryMode = 1, int priority = 4, uint64_t timeToLive = 0);

    // If something bad happens you see it here as this class is also been
    // registered as an ExceptionListener with the connection.
    virtual void onException(const CMSException& ex AMQCPP_UNUSED);
    virtual void onException(const decaf::lang::Exception& ex);
    virtual void transportInterrupted();
    virtual void transportResumed();



private:
    void cleanup();
};


#endif // _MQ_CLIENT_H_


