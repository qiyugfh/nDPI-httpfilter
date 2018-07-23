#ifndef _MQ_PACKET_H_
#define _MQ_PACKET_H_

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <string>
#include <stdint.h>


class Packet {
public:
    uint64_t  tv_sec;
    uint64_t  tv_usec;
    uint32_t  caplen;
    uint32_t  len;
    string buffer;

    Packet(){
        tv_sec = 0;
        tv_usec = 0;
        caplen = 0;
        len = 0;
        buffer = "";
    }

    Packet(uint64_t _tv_sec, uint64_t _tv_usec, uint32_t _caplen, uint32_t _len, const string &_buffer){
        tv_sec = _tv_sec;
        tv_usec = _tv_usec;
        caplen = _caplen;
        len = _len;
        buffer = _buffer;
    }

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & tv_sec;
        ar & tv_usec;
        ar & caplen;
        ar & len;
        ar & buffer;
    }

};




#endif

