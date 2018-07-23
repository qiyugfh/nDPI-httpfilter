#include "mqclient.h"



MQClient::MQClient(const std::string &_brokerURI, const std::string &_destURI, bool _useTopic, bool _clientAck):
    connection(NULL), session(NULL), destination(NULL),
    producer(NULL), consumer(NULL), useTopic(_useTopic),
    clientAck(_clientAck), brokerURI(_brokerURI), destURI(_destURI){

}

MQClient::~MQClient(){
    this->cleanup();
}

int MQClient::open(){
    printf("begin open mq client ...\n");
    try {
        ActiveMQConnectionFactory* connectionFactory = new ActiveMQConnectionFactory( brokerURI );

		printf("begin create coinnection ...\n");
		connection = connectionFactory->createConnection();
        delete connectionFactory;

        ActiveMQConnection* amqConnection = dynamic_cast<ActiveMQConnection*>( connection );
        if( amqConnection != NULL )
        {
            amqConnection->addTransportListener( this );
        }

		printf("begin start connection ...\n");
        connection->start();
        connection->setExceptionListener(this);

		printf("begin create session ...\n");
        if( clientAck )
        {
            session = connection->createSession( Session::CLIENT_ACKNOWLEDGE );
        } else
        {
            session = connection->createSession( Session::AUTO_ACKNOWLEDGE );
        }
		
        printf("begin create queue...\n");
        if( useTopic )
        {
            destination = session->createTopic( destURI );
        } else
        {
            destination = session->createQueue( destURI );
        }
		
        printf("begin create producer ...\n");
        producer = session->createProducer( destination );
		
        printf("begin create consumer ...\n");
        consumer = session->createConsumer( destination );

    }catch ( CMSException& e ) {
        e.printStackTrace();
        return -1;
    }

	printf("open mq cleint success ...\n");
    return 0;
}


void MQClient::close(){
    printf("begin close mq client ...\n");
    this->cleanup();
}

string MQClient::recvTask(){
    string text = "";
    Message *message = NULL;
    TextMessage *textMessage = NULL;
    try{
        message = consumer->receive();
        textMessage = dynamic_cast<TextMessage *>(message);
        text = textMessage->getText();
    }catch(CMSException &e){
        e.printStackTrace();
    }
    delete textMessage;
    textMessage = NULL;
    return text;
}


void MQClient::sendTask(const string &msg, int deliveryMode, int priority, uint64_t timeToLive){
    TextMessage *message = session->createTextMessage(msg);
    producer->send(message, deliveryMode, priority, timeToLive);
    delete message;
    message = NULL;
}

void MQClient::onException(const CMSException& ex AMQCPP_UNUSED) {
    printf("CMS Exception occurred.  Shutting down client.\n");
    exit(1);
}


void MQClient::onException(const decaf::lang::Exception& ex) {
    printf("Transport Exception occurred: %s \n", ex.getMessage().c_str());
}

void MQClient::transportInterrupted() {
    printf("The Connection's Transport has been Interrupted.\n");
}

void MQClient::transportResumed() {
    printf("The Connection's Transport has been Restored.\n");
}

void MQClient::cleanup(){
    try{
        if( destination != NULL ) delete destination;
    }catch (CMSException& e) {}
    destination = NULL;

    try{
        if( consumer != NULL ) delete consumer;
    }catch (CMSException& e) {}
    consumer = NULL;

    try{
        if( session != NULL ) session->close();
        if( connection != NULL ) connection->close();
    }catch (CMSException& e) {}

    try{
        if( session != NULL ) delete session;
    }catch (CMSException& e) {}
    session = NULL;

    try{
        if( connection != NULL ) delete connection;
    }catch (CMSException& e) {}
    connection = NULL;
}
